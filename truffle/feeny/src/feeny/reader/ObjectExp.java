package feeny.reader;

public class ObjectExp implements Exp {
  public Exp parent;
  public final SlotStmt[] slots;
  public ObjectExp (Exp aParent, SlotStmt[] aSlots){
    parent = aParent;
    slots = aSlots;
  }
  
  public String toString () {
    return "object(" + parent + ") : (" + Utils.commas(slots) + ")";
  }  
}
