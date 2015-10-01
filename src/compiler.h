#ifndef COMPILER_H
#define COMPILER_H
#include "ast.h"
#include "bytecode.h"

// hashtable.h

struct entry_s_i {
    char* key;
    int value;
    struct entry_s_i* next;
};

typedef struct entry_s_i entry_t_i;

struct hashtable_s_i {
    int size;
    struct entry_s_i** table;
};

typedef struct hashtable_s_i Hashtable_i;

Hashtable_i* ht_create_i(int size);

void ht_put_i(Hashtable_i* hashtable, char* key, int value);
int ht_get_i(Hashtable_i *hashtable, char *key);   //!!!! This HashTable is different from the previous one!
void ht_clear_i(Hashtable_i *hashtable);



//Creating Constant values
int make_int(int value);

int make_null();
int get_null();

int getStrId(char* str); // Return the string id in the constant pool, if non-exist, add it into the pool first.

int make_method(int args,char* name); // Create a new method in the constant pool, and return the ID

int make_slot(char* name);

int make_class();


//Creating Bytecode Ins  (Copied from bytecode)
void addIns(MethodValue* m, ByteIns*ins);

ByteIns* make_lit(int);
ByteIns* make_print(int,int);
ByteIns* make_array();
ByteIns* make_setLocal(int);
ByteIns* make_getLocal(int);
ByteIns* make_setGlobal(int);
ByteIns* make_getGlobal(int);
ByteIns* make_drop();

ByteIns* make_object(int);

ByteIns* make_call(int,int);



int compile_slotstmt(Hashtable_i* genv, Hashtable_i* env, MethodValue* m, SlotStmt* s); // Return the slotValue id in the constant pool

void compile_stmt(Hashtable_i* genv, Hashtable_i* env, MethodValue* m, ScopeStmt* s);

void compile_exp(Hashtable_i* genv, Hashtable_i* env, MethodValue* m, Exp* exp);



Program* compile (ScopeStmt* stmt);

#endif
