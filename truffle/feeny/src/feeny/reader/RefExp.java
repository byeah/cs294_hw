package feeny.reader;

public class RefExp implements Exp {
  public final String name;
  public RefExp (String aName){
    name = aName;
  }
  
  public String toString () {
    return name;
  }      
}
