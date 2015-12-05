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
        System.out.println("AST RefExpNode " + name + " " + slot);
    }

    public Frame getTopLevelFrame(VirtualFrame frame) {
        if (frame.getArguments().length == 0) {
            return frame;
        } else {
            return (Frame) frame.getArguments()[0];
        }
    }

    public Object lookupSlot(VirtualFrame frame) {
        Object o = frame.getValue(slot);
        if (o == null) {
            Frame tlf = getTopLevelFrame(frame);
            FrameSlot tlSlot = tlf.getFrameDescriptor().findFrameSlot(name);
            o = tlf.getValue(tlSlot);
        }
        return o;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Ref " + name + " " + slot);
        return lookupSlot(frame);
    }
}
