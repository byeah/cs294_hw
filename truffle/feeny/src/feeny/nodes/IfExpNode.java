package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class IfExpNode extends RootNode {
    @Child RootNode cond;
    @Child RootNode then;
    @Child RootNode alter;

    public IfExpNode(RootNode pred, RootNode conseq, RootNode alt, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        cond = pred;
        then = conseq;
        alter = alt;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        if ((cond.execute(frame) instanceof NullObj)) {
            return alter.execute(frame);
        } else {
            return then.execute(frame);
        }
    }
}
