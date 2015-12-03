package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class ScopeFnNode extends RootNode {
    FrameSlot slot;
    @Child RootNode exp;

    public ScopeFnNode(String name, RootNode e, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot(name);
        exp = e;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        Integer i = (Integer) exp.execute(frame);
        frame.setObject(slot, i);
        return null;
    }
}