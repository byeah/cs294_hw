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
import feeny.reader.Reader;

public final class Feeny extends TruffleLanguage<ExecutionContext> {
    public static void main(String[] args) {
        // Example showing creating and executing a simple node
        test_hello();
        // Example showing creating and executing a node containing children nodes
        test_plus();
        // Example showing how to create, write, and read a variable
        test_variables();
        // Example showing how to create and call a function, and read an argument
        test_function();
        // Create a corresponding Truffle AST and execute it
        test_feeny();
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

    private static void test_hello() {
        System.out.println("=== Testing Hello ===");
        exec_node(new HelloNode(new FrameDescriptor()));
    }

    private static void test_plus() {
        System.out.println("=== Testing Plus ===");
        FrameDescriptor fd = new FrameDescriptor();
        PlusNode node = new PlusNode(new IntNode(10, fd), new IntNode(32, fd), fd);
        exec_node(node);
    }

    private static void test_variables() {
        System.out.println("=== Testing Variables ===");
        exec_node(new LocalsNode(new FrameDescriptor()));
    }

    private static void test_function() {
        System.out.println("=== Testing Functions ===");
        PrintArgNode body = new PrintArgNode(new FrameDescriptor());
        RootCallTarget ct = Truffle.getRuntime().createCallTarget(body);
        exec_node(new CallNode(ct, new FrameDescriptor()));
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
