#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "hashtable.h"

typedef enum {
    Int,
    Array,
    Null,
    Env
} ObjType;

typedef struct {
    ObjType type;
} Obj;

int obj_type(Obj* o);

// Integer Objects

typedef struct {
    ObjType type;
    int value;
} IntObj;

IntObj* make_int_obj(int value);
IntObj* add(IntObj* x, IntObj *y);
IntObj* subtract(IntObj* x, IntObj *y);
IntObj* multiply(IntObj* x, IntObj *y);
IntObj* divide(IntObj* x, IntObj *y);
IntObj* modulo(IntObj* x, IntObj *y);
Obj* eq(IntObj* x, IntObj *y);
Obj* lt(IntObj* x, IntObj *y);
Obj* le(IntObj* x, IntObj *y);
Obj* gt(IntObj* x, IntObj *y);
Obj* ge(IntObj* x, IntObj *y);

// Null Objects

typedef struct {
	ObjType type;
} NullObj;

NullObj* make_null_obj();

// Array Objects

typedef struct {
    ObjType type;
    int length;
    int* data;
} ArrayObj;

ArrayObj* make_array_obj(IntObj *length, Obj* init);
IntObj* array_length(ArrayObj* a);
NullObj* array_set(ArrayObj* a, IntObj* i, Obj* v);
Obj* array_get(ArrayObj* a, IntObj* i);

// Environment Objects

typedef enum {
	Var,
	Func
} EntryType;

typedef struct {
	EntryType type;
} Entry;

typedef struct {
	EntryType type;
	Obj* value;
} VarEntry;

typedef struct {
	EntryType type;
	ScopeStmt* body;
	int nargs;
	char** args;
} FuncEntry;

typedef struct {
	ObjType type;
	Hashtable* table;
} EnvObj;

EnvObj* make_env_obj(Obj* parent);
void add_entry(EnvObj* env, char* name, Entry* entry);
Entry* get_entry(EnvObj* env, char* name);

Entry* make_var_entry(Obj* obj);
Entry* make_func_entry(ScopeStmt*, int,char**);


void exec_slotstmt (EnvObj* genv, EnvObj* env, EnvObj* obj, SlotStmt* s);

void exec_stmt (EnvObj* genv, EnvObj* env, ScopeStmt* s);
Obj* eval_stmt (EnvObj* genv, EnvObj* env, ScopeStmt* s);

Obj* eval_exp (EnvObj* genv, EnvObj* env, Exp* exp);

void interpret (ScopeStmt* stmt);

#endif

