package feeny.reader;

public class ArrayExp implements Exp {
  public final Exp length;
  public final Exp init;
  public ArrayExp (Exp aLength, Exp aInit){
    length = aLength;
    init = aInit;
  }
  
  public String toString () {
    return "array(" + length + ", " + init + ")";
  }
}
