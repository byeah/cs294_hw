package feeny.nodes;

import feeny.*;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;

public class HelloNode extends RootNode {
    public HelloNode(FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Hello");
        return null;
    }
}
