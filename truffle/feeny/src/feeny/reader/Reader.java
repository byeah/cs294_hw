package feeny.reader;
import java.io.*;

public class Reader {
   InputStream input;

   public Reader (InputStream stream){
      input = new BufferedInputStream(stream);
   }
   public Reader (String filename) throws FileNotFoundException {
      this(new FileInputStream(filename));
   }

   int read_byte () throws EOFException {
      try{
         int i = input.read();         
         if(i < 0) throw new EOFException();
         return i;
      }
      catch(EOFException e){
         throw(e);
      }
      catch(IOException e){
         throw new RuntimeException(e);
      }
   }

   int read_short () throws EOFException {
      int b0 = read_byte();
      int b1 = read_byte();
      return b0 + (b1 << 8);
   }

   int read_int () throws EOFException {
      int b0 = read_byte();
      int b1 = read_byte();
      int b2 = read_byte();
      int b3 = read_byte();
      return b0 + (b1 << 8) + (b2 << 16) + (b3 << 24);
   }

   String read_string () throws EOFException {
      int len = read_int();
      char[] str = new char[len];
      for(int i=0; i<len; i++)
         str[i] = (char)read_byte();
      return new String(str);
   }   

   //=== Op Tags ===   
   final static int INT_EXP = 0;
   final static int NULL_EXP = 1;
   final static int PRINTF_EXP = 2;
   final static int ARRAY_EXP = 3;
   final static int OBJECT_EXP = 4;
   final static int SLOT_EXP = 5;
   final static int SET_SLOT_EXP = 6;
   final static int CALL_SLOT_EXP = 7;
   final static int CALL_EXP = 8;
   final static int SET_EXP = 9;
   final static int IF_EXP = 10;
   final static int WHILE_EXP = 11;
   final static int REF_EXP = 12;
   final static int VAR_STMT = 13;
   final static int FN_STMT = 14;
   final static int SEQ_STMT = 15;
   final static int EXP_STMT = 16;

   Exp read_exp () throws EOFException {
      int tag = read_int();
      switch(tag){
      case INT_EXP: return new IntExp(read_int());
      case NULL_EXP: return new NullExp();
      case PRINTF_EXP: return new PrintfExp(read_string(), read_exps());
      case ARRAY_EXP: return new ArrayExp(read_exp(), read_exp());
      case OBJECT_EXP: return new ObjectExp(read_exp(), read_slots());
      case SLOT_EXP: return new SlotExp(read_string(), read_exp());
      case SET_SLOT_EXP: return new SetSlotExp(read_string(), read_exp(), read_exp());
      case CALL_SLOT_EXP: return new CallSlotExp(read_string(), read_exp(), read_exps());
      case CALL_EXP: return new CallExp(read_string(), read_exps());
      case SET_EXP: return new SetExp(read_string(), read_exp());
      case IF_EXP: return new IfExp(read_exp(), read_scopestmt(), read_scopestmt());
      case WHILE_EXP: return new WhileExp(read_exp() ,read_scopestmt());
      case REF_EXP: return new RefExp(read_string());
      default: throw new RuntimeException("Unrecognized Expression Tag: " + tag);
      }      
   }
   
   SlotStmt read_slot () throws EOFException {
     int tag = read_int();
     switch(tag){
     case VAR_STMT: return new SlotVar(read_string(), read_exp());
     case FN_STMT: return new SlotMethod(read_string(), read_strings(), read_scopestmt());     
     default: throw new RuntimeException("Unrecognized Slot Tag: " + tag); 
     }
   }

   ScopeStmt read_scopestmt () throws EOFException {
      int tag = read_int();
      switch(tag){
      case VAR_STMT: return new ScopeVar(read_string(), read_exp());
      case FN_STMT: return new ScopeFn(read_string(), read_strings(), read_scopestmt());
      case SEQ_STMT: return new ScopeSeq(read_scopestmt(), read_scopestmt());
      case EXP_STMT: return new ScopeExp(read_exp());
      default: throw new RuntimeException("Unrecognized Stmt Tag: " + tag);
      }
   }
   
   String[] read_strings () throws EOFException {
     int n = read_int();
     String[] strs = new String[n];
     for (int i=0; i<n; i++)
       strs[i] = read_string();
     return strs;
   }

   Exp[] read_exps () throws EOFException {
      int n = read_int();
      Exp[] exps = new Exp[n];
      for (int i=0; i<n; i++)
         exps[i] = read_exp();
      return exps;
   }

   SlotStmt[] read_slots () throws EOFException {
      int n = read_int();
      SlotStmt[] stmts = new SlotStmt[n];
      for (int i=0; i<n; i++)
         stmts[i] = read_slot();
      return stmts;
   }

   ScopeStmt[] read_scopestmts () throws EOFException {
      int n = read_int();
      ScopeStmt[] stmts = new ScopeStmt[n];
      for (int i=0; i<n; i++)
         stmts[i] = read_scopestmt();
      return stmts;
   }

   public ScopeStmt read () {      
      try{
        return read_scopestmt();
      }catch(EOFException e){
         throw new RuntimeException("Unexpected end of input while reading program.");
      }
   }
}
