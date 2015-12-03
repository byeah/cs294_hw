package feeny.reader;

public class SlotExp implements Exp {
  public final String name;
  public final Exp exp;
  public SlotExp (String aName, Exp aExp){
    name = aName;
    exp = aExp;
  }
  
  public String toString () {
    return exp + "." + name;
  }    
}
