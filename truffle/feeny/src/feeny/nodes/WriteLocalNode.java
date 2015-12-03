package feeny.nodes;

import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlot;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;

import feeny.Feeny;

public class WriteLocalNode extends RootNode {
    FrameSlot slot;

    public WriteLocalNode(FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot("i");
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Write to variable i.");
        frame.setObject(slot, "Hello World");
        return null;
    }
}