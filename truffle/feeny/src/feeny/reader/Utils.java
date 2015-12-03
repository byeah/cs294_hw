package feeny.reader;

public class Utils {
  public static String commas (Object[] xs){
    StringBuffer s = new StringBuffer();
    for(int i=0; i<xs.length; i++){
      if(i>0) s.append(", ");
      s.append(xs[i]);
    }
    return s.toString();
  }
}
