package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.TruffleRuntime;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.DirectCallNode;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;
import com.oracle.truffle.api.frame.FrameSlotKind;

import feeny.Feeny;

public class FuncNode extends RootNode {
    @Child RootNode body;
    FrameSlot[] args;

    public FuncNode(RootNode body, String[] args, FrameDescriptor env) {
        super(Feeny.class, null, env);
        this.body = body;
        this.args = new FrameSlot[args.length];
        for (int i = 0; i < args.length; ++i) {
            this.args[i] = env.findOrAddFrameSlot(args[i], FrameSlotKind.Object);
        }
    }

    @Override
    public Object execute(VirtualFrame frame) {
        for (int i = 1; i < frame.getArguments().length; ++i) {
            frame.setObject(args[i - 1], frame.getArguments()[i]);
        }
        return body.execute(frame);
    }
}
