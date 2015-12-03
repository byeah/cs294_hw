package feeny.reader;

public class IfExp implements Exp {
  public final Exp pred;
  public final ScopeStmt conseq;
  public final ScopeStmt alt;
  public IfExp (Exp aPred, ScopeStmt aConseq, ScopeStmt aAlt){
    pred = aPred;
    conseq = aConseq;
    alt = aAlt;
  }
  
  public String toString () {
    return "if " + pred + " : (" + conseq + ") else : (" + alt + ")";
  }  
}
