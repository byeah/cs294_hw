package feeny;

import java.io.FileNotFoundException;
import java.io.IOException;

import com.oracle.truffle.api.CallTarget;
import com.oracle.truffle.api.ExecutionContext;
import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.TruffleLanguage;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.MaterializedFrame;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.instrument.Visualizer;
import com.oracle.truffle.api.instrument.WrapperNode;
import com.oracle.truffle.api.nodes.DirectCallNode;
import com.oracle.truffle.api.nodes.Node;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.source.Source;
import feeny.nodes.*;
import feeny.reader.*;

public final class Feeny extends TruffleLanguage<ExecutionContext> {
    public static void main(String[] args) {
        // Create a corresponding Truffle AST and execute it
        test_feeny();
    }

    private static RootNode transform_expr(Exp expr, FrameDescriptor genv, FrameDescriptor env) {
        FrameDescriptor cenv = (env == null) ? genv : env;
        if (expr instanceof IntExp) {
            IntExp e = (IntExp) expr;
            return new IntExpNode(e.value, cenv);
        } else if (expr instanceof NullExp) {
            return new NullExpNode(cenv);
        } else if (expr instanceof PrintfExp) {
            PrintfExp e = (PrintfExp) expr;
            RootNode[] args = new RootNode[e.exps.length];
            for (int i = 0; i < e.exps.length; ++i) {
                args[i] = transform_expr(e.exps[i], genv, env);
            }
            return new PrintfExpNode(e.format, args, cenv);
        } else if (expr instanceof CallSlotExp) {
            CallSlotExp e = (CallSlotExp) expr;
            RootNode obj = transform_expr(e.exp, genv, env);
            RootNode[] args = new RootNode[e.args.length];
            for (int i = 0; i < e.args.length; ++i) {
                args[i] = transform_expr(e.args[i], genv, env);
            }
            return new CallSlotExpNode(e.name, obj, args, cenv);
        } else if (expr instanceof CallExp) {
            CallExp e = (CallExp) expr;
            RootNode[] args = new RootNode[e.args.length];
            for (int i = 0; i < e.args.length; ++i) {
                args[i] = transform_expr(e.args[i], genv, env);
            }
            return new CallExpNode(e.name, args, cenv);
        } else if (expr instanceof SetExp) {
            SetExp e = (SetExp) expr;
            RootNode arg = transform_expr(e.exp, genv, env);
            return new SetExpNode(e.name, arg, cenv);
        } else if (expr instanceof IfExp) {
            IfExp e = (IfExp) expr;
            // FrameDescriptor nenv1 = cenv.shallowCopy();
            // FrameDescriptor nenv2 = cenv.shallowCopy();
            RootNode cond = transform_expr(e.pred, genv, env);
            RootNode conseq = transform_stmt(e.conseq, genv, env);
            RootNode alt = transform_stmt(e.alt, genv, env);
            return new IfExpNode(cond, conseq, alt, cenv);
        } else if (expr instanceof WhileExp) {
            WhileExp e = (WhileExp) expr;
            // FrameDescriptor nenv = cenv.shallowCopy();
            RootNode cond = transform_expr(e.pred, genv, env);
            RootNode body = transform_stmt(e.body, genv, env);
            return new WhileExpNode(cond, body, cenv);
        } else if (expr instanceof RefExp) {
            RefExp e = (RefExp) expr;
            return new RefExpNode(e.name, cenv);
        } else {
            throw new RuntimeException("Unrecognized Exp.");
        }
    }

    private static RootNode transform_stmt(ScopeStmt stmt, FrameDescriptor genv, FrameDescriptor env) {
        FrameDescriptor cenv = (env == null) ? genv : env;
        if (stmt instanceof ScopeVar) {
            ScopeVar scope = (ScopeVar) stmt;
            return new ScopeVarNode(scope.name, transform_expr(scope.exp, genv, env), cenv);
        } else if (stmt instanceof ScopeFn) {
            ScopeFn scope = (ScopeFn) stmt;
            FrameDescriptor nenv = genv.shallowCopy();// new FrameDescriptor();
            assert (cenv == genv);
            FuncNode body = new FuncNode(transform_stmt(scope.body, genv, nenv), scope.args, nenv);
            return new ScopeFnNode(scope.name, body, cenv);
        } else if (stmt instanceof ScopeSeq) {
            ScopeSeq scope = (ScopeSeq) stmt;
            return new ScopeSeqNode(transform_stmt(scope.a, genv, env), transform_stmt(scope.b, genv, env), cenv);
        } else if (stmt instanceof ScopeExp) {
            ScopeExp scope = (ScopeExp) stmt;
            return new ScopeExpNode(transform_expr(scope.exp, genv, env), cenv);
        } else {
            throw new RuntimeException("Unrecognized ScopeStmt.");
        }
    }

    private static void test_feeny() {
        System.out.println("=== Testing Feeny ===");
        try {
            Reader reader = new Reader("tests/fibonacci.ast");
            ScopeStmt ast = reader.read();

            System.out.println(ast);
            System.out.println("Create and Execute Truffle Feeny AST");

            FrameDescriptor genv = new FrameDescriptor();
            RootNode root = transform_stmt(ast, genv, null);

            exec_node(root);
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        }
    }

    private static void exec_node(RootNode node) {
        RootCallTarget ct = Truffle.getRuntime().createCallTarget(node);
        ct.call();
    }

    @Override
    protected ExecutionContext createContext(com.oracle.truffle.api.TruffleLanguage.Env env) {
        return null;
    }

    @Override
    protected CallTarget parse(Source code, Node context, String... argumentNames) throws IOException {
        return null;
    }

    @Override
    protected Object findExportedSymbol(ExecutionContext context, String globalName, boolean onlyExplicit) {
        return null;
    }

    @Override
    protected Object getLanguageGlobal(ExecutionContext context) {
        return null;
    }

    @Override
    protected boolean isObjectOfLanguage(Object object) {
        return false;
    }

    @Override
    protected Visualizer getVisualizer() {
        return null;
    }

    @Override
    protected boolean isInstrumentable(Node node) {
        return false;
    }

    @Override
    protected WrapperNode createWrapperNode(Node node) {
        return null;
    }

    @Override
    protected Object evalInContext(Source source, Node node, MaterializedFrame mFrame) throws IOException {
        return null;
    }

}
