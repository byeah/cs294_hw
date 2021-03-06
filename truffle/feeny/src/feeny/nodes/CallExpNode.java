package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.Frame;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlotKind;
import com.oracle.truffle.api.frame.MaterializedFrame;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.DirectCallNode;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class CallExpNode extends RootNode {
    FrameSlot slot;
    String name;
    @Children final RootNode[] argNodes;

    public CallExpNode(String name, RootNode[] args, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot(name);
        this.name = name;
        argNodes = args;
    }

    public Object findSlot(VirtualFrame frame) {
        Object o = frame.getValue(slot);
        if (o == null && frame.getArguments().length > 0) {
            VirtualFrame parent = (VirtualFrame) frame.getArguments()[0];
            o = parent.getValue(slot);
        }
        return o;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        // System.out.println("Calling Exp " + slot + " under " +
        // frame.getFrameDescriptor().toString());

        Object[] args = new Object[argNodes.length + 1];
        args[0] = frame;

        for (int i = 1; i < args.length; ++i) {
            args[i] = argNodes[i - 1].execute(frame);
        }

        RootCallTarget ct = (RootCallTarget) findSlot(frame);
        // System.out.println(ct.toString());
        assert (ct != null);

        return ct.call(args);
    }
}
