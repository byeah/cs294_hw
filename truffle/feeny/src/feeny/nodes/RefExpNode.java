package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.Frame;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;
import com.oracle.truffle.api.frame.FrameSlotKind;

import feeny.Feeny;

public class RefExpNode extends RootNode {
    FrameSlot slot;
    String name;

    public RefExpNode(String name, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        this.slot = frameDescriptor.findOrAddFrameSlot(name, FrameSlotKind.Object);
        this.name = name;
        // System.out.println("AST RefExpNode " + name + " " + slot);
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
        // System.out.println("Ref " + name + " " + slot);
        // System.out.println(findSlot(frame));
        return findSlot(frame);
    }
}
