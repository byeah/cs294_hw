#ifndef INTERPRETER_H
#define INTERPRETER_H

typedef enum{
    Int,
    Array,
    Null
} ObjType;

typedef struct{
    ObjType type;
} Obj;

typedef struct{
    ObjType type;
    int value;
} IntObj;

typedef struct{
    ObjType type;
    int length;
    int* data;
} ArrayObj;

typedef struct{
    ObjType type;
} NullObj;


typedef enum{
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
    ScopeStmt* stmt;
    int numOfParas;
    char** paraNames;
} FuncEntry;

typedef struct{
    
} EnvObj;



void interpret (ScopeStmt* stmt);

#endif

