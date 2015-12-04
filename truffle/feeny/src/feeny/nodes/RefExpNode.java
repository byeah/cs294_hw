package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.Frame;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class RefExpNode extends RootNode {
    FrameSlot slot;
    String name;

    public RefExpNode(String name, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        this.slot = frameDescriptor.findFrameSlot(name);
        this.name = name;
    }

    public Frame getTopLevelFrame(VirtualFrame frame) {
        if (frame.getArguments().length == 0) {
            return frame.materialize();
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
        return lookupSlot(frame);
    }
}
