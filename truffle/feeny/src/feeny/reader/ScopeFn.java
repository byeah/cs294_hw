package feeny.reader;

public class ScopeFn implements ScopeStmt {
  public final String name;
  public final String[] args;
  public final ScopeStmt body;
  public ScopeFn (String aName, String[] aArgs, ScopeStmt aBody){
    name = aName;
    args = aArgs;
    body = aBody;
  }
  
  public String toString () {
    return "defn " + name + "(" + Utils.commas(args) + ") : (" + body + ")";
  }    
}
