package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;

import feeny.Feeny;

public class PlusNode extends RootNode {
    @Child RootNode a_node;
    @Child RootNode b_node;

    public PlusNode(RootNode a, RootNode b, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        a_node = a;
        b_node = b;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        Integer ai = (Integer) a_node.execute(frame);
        Integer bi = (Integer) b_node.execute(frame);
        Integer ci = ai + bi;
        System.out.println(ai + " + " + bi + " = " + ci);
        return ci;
    }
}
