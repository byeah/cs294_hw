package feeny.nodes;

import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlot;
import com.oracle.truffle.api.frame.FrameSlotTypeException;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;

import feeny.Feeny;

public class ReadLocalNode extends RootNode {
    FrameSlot slot;

    public ReadLocalNode(FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot("i");
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Read from variable i.");
        Object v = frame.getValue(slot);
        System.out.println(v);
        return null;
    }
}