package feeny.reader;

public class SlotVar implements SlotStmt {
  public final String name;
  public final Exp exp;
  public SlotVar (String aName, Exp aExp){
    name = aName;
    exp = aExp;
  }
  
  public String toString () {
    return "var " + name + " = " + exp;
  }     
}
