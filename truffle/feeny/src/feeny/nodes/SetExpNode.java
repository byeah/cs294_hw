package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class SetExpNode extends RootNode {
    FrameSlot slot;
    @Child RootNode expNode;

    public SetExpNode(String name, RootNode exp, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot(name);
        expNode = exp;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        // System.out.println("Set " + slot + " " + expNode);
        frame.setObject(slot, expNode.execute(frame));
        return new NullObj();
    }
}
