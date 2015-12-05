package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class ScopeExpNode extends RootNode {
    @Child RootNode exp;

    public ScopeExpNode(RootNode e, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        exp = e;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Eval exp " + exp);
        return exp.execute(frame);
    }
}
