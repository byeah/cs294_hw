package feeny.reader;

public class ScopeSeq implements ScopeStmt {
  public final ScopeStmt a;
  public final ScopeStmt b;
  public ScopeSeq (ScopeStmt sa, ScopeStmt sb){
    a = sa;
    b = sb;
  }
  
  public String toString () {
    return a + " " + b;
  }    
}
