package feeny.reader;

public class ScopeExp implements ScopeStmt {
  public final Exp exp;
  public ScopeExp (Exp aExp){
    exp = aExp;
  }
  
  public String toString () {
    return exp.toString();
  }
}
