package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class ScopeSeqNode extends RootNode {
    @Child RootNode stmt1;
    @Child RootNode stmt2;

    public ScopeSeqNode(RootNode a, RootNode b, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        stmt1 = a;
        stmt2 = b;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        stmt1.execute(frame);
        return stmt2.execute(frame);
    }
}
