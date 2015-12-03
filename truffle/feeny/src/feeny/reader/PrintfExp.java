package feeny.reader;

public class PrintfExp implements Exp {
  public final String format;
  public final Exp[] exps;
  public PrintfExp (String aFormat, Exp[] aExps){
    format = aFormat;
    exps = aExps;
  }
  
  public String toString () {
    if(exps.length == 0){
      return "printf(" + format + ")";
    }else{
      return "printf(" + format + ", " + Utils.commas(exps) + ")";
    }    
  }    
}
