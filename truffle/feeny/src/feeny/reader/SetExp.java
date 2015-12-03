package feeny.reader;

public class SetExp implements Exp {
  public final String name;
  public final Exp exp;
  public SetExp (String aName, Exp aExp){
    name = aName;
    exp = aExp;
  }
  
  public String toString () {
    return name + " = " + exp;
  }    
}
