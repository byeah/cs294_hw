package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class CallSlotExpNode extends RootNode {
    String name;
    @Child RootNode expNode;
    @Children final RootNode[] argNodes;

    public CallSlotExpNode(String n, RootNode exp, RootNode[] args, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        name = n;
        expNode = exp;
        argNodes = args;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        Integer i = (Integer) expNode.execute(frame);
        Integer j = (Integer) argNodes[0].execute(frame);
        switch (name) {
            case "add":
                return i + j;
            case "sub":
                return i - j;
            case "mul":
                return i * j;
            case "div":
                return i / j;
            case "mod":
                return i % j;
            case "lt":
                return (i < j) ? new Integer(0) : null;
            case "le":
                return (i <= j) ? new Integer(0) : null;
            case "gt":
                return (i > j) ? new Integer(0) : null;
            case "ge":
                return (i >= j) ? new Integer(0) : null;
            case "eq":
                return (i == j) ? new Integer(0) : null;
        }
        return null;
    }
}
