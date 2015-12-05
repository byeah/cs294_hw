package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;
import com.oracle.truffle.api.frame.FrameSlotKind;

import feeny.Feeny;

public class ScopeVarNode extends RootNode {
    FrameSlot slot;
    @Child RootNode exp;

    public ScopeVarNode(String name, RootNode e, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findOrAddFrameSlot(name, FrameSlotKind.Object);
        exp = e;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Defining Var " + slot + " = " + exp);
        Integer i = (Integer) exp.execute(frame);
        frame.setObject(slot, i);
        return null;
    }
}