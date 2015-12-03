package feeny.nodes;

import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlot;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;

import feeny.Feeny;

public class LocalsNode extends RootNode {
    @Child WriteLocalNode writer;
    @Child ReadLocalNode reader;

    public LocalsNode(FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        frameDescriptor.addFrameSlot("i");
        writer = new WriteLocalNode(frameDescriptor);
        reader = new ReadLocalNode(frameDescriptor);
    }

    @Override
    public Object execute(VirtualFrame frame) {
        System.out.println("Variable i created.");
        writer.execute(frame);
        reader.execute(frame);
        return null;
    }
}