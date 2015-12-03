package feeny.nodes;

import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;

import feeny.Feeny;

public class PrintArgNode extends RootNode {
    public PrintArgNode(FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Inside Function.");
        System.out.println("First argument is " + frame.getArguments()[0]);
        return null;
    }
}
