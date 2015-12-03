package feeny.reader;

public class ScopeVar implements ScopeStmt {
  public final String name;
  public final Exp exp;
  public ScopeVar (String aName, Exp aExp){
    name = aName;
    exp = aExp;
  }
  public String toString () {
    return "var " + name + " = " + exp;
  }    
}
