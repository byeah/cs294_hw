package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.TruffleRuntime;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.DirectCallNode;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;
import com.oracle.truffle.api.frame.FrameSlotKind;

import feeny.Feeny;

public class ScopeFnNode extends RootNode {
    FrameSlot slot;
    RootCallTarget ct;

    public ScopeFnNode(String name, FuncNode func, FrameDescriptor env) {
        super(Feeny.class, null, env);
        slot = env.findOrAddFrameSlot(name, FrameSlotKind.Object);
        this.ct = Truffle.getRuntime().createCallTarget(func);
    }

    @Override
    public Object execute(VirtualFrame frame) {
        // System.out.println("Defining Func " + slot + " = " + ct);
        frame.setObject(slot, ct);
        return new NullObj();
    }
}
