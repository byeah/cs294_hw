package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.DirectCallNode;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class CallExpNode extends RootNode {
    FrameSlot slot;
    @Children RootNode[] argNodes;

    public CallExpNode(String name, RootNode[] args, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot(name);
        argNodes = args;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        Object[] args = new Object[argNodes.length];
        for (int i = 0; i < argNodes.length; ++i) {
            args[i] = argNodes[i].execute(frame);
        }
        DirectCallNode callNode = (DirectCallNode) frame.getValue(slot);
        return callNode.call(frame, args);
    }
}
