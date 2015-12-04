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
    @Child RootNode stmt;

    public ScopeFnNode(String name, String[] args, RootNode body, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot(name);
        stmt = body;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        frame.setObject(slot, stmt);
        return null;
    }
}
