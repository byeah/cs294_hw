package feeny.nodes;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.Truffle;
import com.oracle.truffle.api.frame.FrameDescriptor;
import com.oracle.truffle.api.frame.VirtualFrame;
import com.oracle.truffle.api.nodes.RootNode;
import com.oracle.truffle.api.frame.FrameSlot;

import feeny.Feeny;

public class PrintExpNode extends RootNode {
    String format;
    @Children RootNode[] expNodes;

    public PrintExpNode(String fmt, RootNode[] exps, FrameDescriptor frameDescriptor) {
        super(Feeny.class, null, frameDescriptor);
        format = fmt;
        expNodes = exps;
    }

    @Override
    public Object execute(VirtualFrame frame) {
        Object[] args = new Object[expNodes.length];
        for (int i = 0; i < expNodes.length; ++i) {
            args[i] = expNodes[i].execute(frame).toString();
        }
        StringBuilder sb = new StringBuilder();
        int j = 0;
        for (int i = 0; i < format.length; ++i) {
            if (format.charAt(i) == '~') {
                sb.append(args[j++]);
            } else {
                sb.append(format.charAt(i));
            }
        }
        System.out.print(sb.toString());
        return null;
    }
}
