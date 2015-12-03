package feeny.reader;

public class IntExp implements Exp {
  public final int value;
  public IntExp (int aValue){
    value = aValue;
  }
  
  public String toString () {
    return Integer.toString(value);
  }    
}
