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

    private static RootNode transform_expr(Exp expr, FrameDescriptor fd) {
        if (expr instanceof IntExp) {
            IntExp e = (IntExp) expr;
            return new IntExpNode(e.value, fd);
        } else if (expr instanceof NullExp) {
            return new NullExpNode(fd);
        } else if (expr instanceof PrintfExp) {
            PrintfExp e = (PrintfExp) expr;
            RootNode[] args = new RootNode[e.exps.length];
            for (int i = 0; i < e.exps.length; ++i) {
                args[i] = transform_expr(e.exps[i], fd);
            }
            return new PrintfExpNode(e.format, args, fd);
        }
        return null;
    }

    private static RootNode transform_stmt(ScopeStmt stmt, FrameDescriptor fd) {
        if (stmt instanceof ScopeVar) {
            ScopeVar scope = (ScopeVar) stmt;
            return new ScopeVarNode(scope.name, transform_expr(scope.exp, fd), fd);
        } else if (stmt instanceof ScopeFn) {
            ScopeFn scope = (ScopeFn) stmt;
            FrameDescriptor new_fd = new FrameDescriptor();
            return new ScopeFnNode(scope.name, scope.args, transform_stmt(scope.body, new_fd), fd);
        } else if (stmt instanceof ScopeSeq) {
            ScopeSeq scope = (ScopeSeq) stmt;
            return new ScopeSeqNode(transform_stmt(scope.a, fd), transform_stmt(scope.b, fd), fd);
        } else if (stmt instanceof ScopeExp) {
            ScopeExp scope = (ScopeExp) stmt;
            return new ScopeExpNode(transform_expr(scope.exp, fd), fd);
        } else {
            throw new RuntimeException("Unrecognized ScopeStmt.");
        }
    }

    private static void test_feeny() {
        System.out.println("=== Testing Feeny ===");
        try {
            Reader reader = new Reader("tests/fibonacci.ast");
            System.out.println(reader.read());
            System.out.println("Create and Execute Truffle Feeny AST");
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
