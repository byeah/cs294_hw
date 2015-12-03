package feeny.reader;

public class CallSlotExp implements Exp{
  public final String name;
  public final Exp exp;
  public final Exp[] args;
  public CallSlotExp (String aName, Exp aExp, Exp[] aArgs){
    name = aName;
    exp = aExp;
    args = aArgs;
  }
  
  public String toString () {    
    return exp + "." + name + "(" + Utils.commas(args) + ")";
  }  
}
