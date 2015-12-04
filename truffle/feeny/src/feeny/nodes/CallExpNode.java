package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.Frame;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.FrameSlotKind;
import com.oracle.truffle.api.frame.MaterializedFrame;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.DirectCallNode;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;
import javafx.util.Pair;

public class CallExpNode extends RootNode {
    FrameSlot slot;
    String name;
    @Children final RootNode[] argNodes;

    public CallExpNode(String name, RootNode[] args, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        slot = frameDescriptor.findFrameSlot(name);
        this.name = name;
        argNodes = args;
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
        System.out.println("Calling function " + slot.toString() + " under " + frame.getFrameDescriptor());

        Object[] args = new Object[argNodes.length];
        for (int i = 0; i < argNodes.length; ++i) {
            args[i] = argNodes[i].execute(frame);
        }

        ScopeFnNode fnNode = (ScopeFnNode) lookupSlot(frame);
        assert (fnNode != null);
        RootNode body = fnNode.body;
        String[] argName = fnNode.args;

        for (int i = 0; i < argName.length; ++i) {
            FrameSlot argSlot = frame.getFrameDescriptor().findOrAddFrameSlot(argName[i], FrameSlotKind.Object);
            frame.setObject(argSlot, args[i]);
        }

        System.out.println(body);

        RootCallTarget ct = Truffle.getRuntime().createCallTarget(body);
        return ct.call(frame, getTopLevelFrame(frame));
    }
}
