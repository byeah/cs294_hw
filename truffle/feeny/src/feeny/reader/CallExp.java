package feeny.reader;

public class CallExp implements Exp {
  public final String name;
  public final Exp[] args;
  public CallExp (String aName, Exp[] aArgs){
    name = aName;
    args = aArgs;
  }
  
  public String toString () {    
    return name + "(" + Utils.commas(args) + ")";
  }
}
