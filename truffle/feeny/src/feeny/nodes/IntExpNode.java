package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class IntExpNode extends RootNode {
    int value;

    public IntExpNode(int v, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        value = v;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        return new Integer(value);
    }
}
