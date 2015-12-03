package feeny.reader;

public class SlotMethod implements SlotStmt {
  public final String name;
  public final String[] args;
  public final ScopeStmt body;
  public SlotMethod (String aName, String[] aArgs, ScopeStmt aBody){
    name = aName;
    args = aArgs;
    body = aBody;
  }
  
  public String toString () {
    return "method " + name + "(" + Utils.commas(args) + ") : (" + body + ")";
  }      
}
