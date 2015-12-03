package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;

import feeny.Feeny;

public class CallNode extends RootNode {
    RootCallTarget target;

    public CallNode(RootCallTarget t, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        target = t;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        return target.call("Hello World");
    }
}
