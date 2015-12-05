package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class WhileExpNode extends RootNode {
    @Child RootNode cond;
    @Child RootNode loop;

    public WhileExpNode(RootNode pred, RootNode body, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        cond = pred;
        loop = body;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        while (!(cond.execute(frame) instanceof NullObj)) {
            loop.execute(frame);
        }
        return new NullObj();
    }
}
