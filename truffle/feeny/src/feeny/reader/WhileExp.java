package feeny.reader;

public class WhileExp implements Exp {
  public final Exp pred;
  public final ScopeStmt body;
  public WhileExp (Exp aPred, ScopeStmt aBody){
    pred = aPred;
    body = aBody;
  }
  
  public String toString () {
    return "while " + pred + " : (" + body + ")";
  }     
}
