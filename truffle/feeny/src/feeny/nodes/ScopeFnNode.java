package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.TruffleRuntime;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.DirectCallNode;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class ScopeFnNode extends RootNode {
    FrameSlot slot;
    @Child DirectCallNode callNode;

    public ScopeFnNode(String name, String[] args, RootNode body, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot(name);
        callNode = Truffle.getRuntime().createDirectCallNode(body.getCallTarget());
    }

    @Override
    public Object execute(VirtualFrame frame) {
        frame.setObject(slot, callNode);
        return null;
    }
}
