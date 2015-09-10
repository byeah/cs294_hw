#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "ast.h"
#include "interpreter.h"

//#define DEBUG
#ifdef _MSC_VER
#define inline __inline
#endif

#ifdef WIN32

#include <windows.h>
#define TIME_T LARGE_INTEGER 
#define FREQ_T LARGE_INTEGER 
#define TIME(t) QueryPerformanceCounter(&(t))
#define FREQ(f) QueryPerformanceFrequency(&(f))
#define ELASPED_TIME(t1, t2, freq) ((t2).QuadPart - (t1).QuadPart) * 1000.0 / (freq).QuadPart

#else

#include <sys/time.h>  
#define TIME_T struct timeval 
#define FREQ_T double 
#define TIME(t) gettimeofday(&(t), NULL)
#define FREQ(f) 1.0
#define ELASPED_TIME(t1, t2, freq) ((t2).tv_sec - (t1).tv_sec) * 1000.0 + ((t2).tv_usec - (t1).tv_usec) / 1000.0

#endif


static int int_calls = 0;
static int array_calls = 0;
static int env_calls = 0;

static int int_count = 0;
static int array_count = 0;
static int null_count = 0;
static int env_count = 0;

static double lookup_time_in_ms = 0.0;
static double total_time_in_ms = 0.0;

static NullObj null_obj_singleton = { .type = Null };

inline
int obj_type(Obj* o) {
    return o->type;
}

inline
IntObj* make_int_obj(int value) {
    ++ int_count;
    IntObj* o = malloc(sizeof(IntObj));
    o->type = Int;
    o->value = value;
    return o;
}

inline
IntObj* add(IntObj* x, IntObj *y) {
    return make_int_obj(x->value + y->value);
}

inline
IntObj* subtract(IntObj* x, IntObj *y) {
    return make_int_obj(x->value - y->value);
}

inline
IntObj* multiply(IntObj* x, IntObj *y) {
    return make_int_obj(x->value * y->value);
}

inline
IntObj* divide(IntObj* x, IntObj *y) {
    return make_int_obj(x->value / y->value);
}

inline
IntObj* modulo(IntObj* x, IntObj *y) {
    return make_int_obj(x->value % y->value);
}

inline
Obj* eq(IntObj* x, IntObj *y) {
    return (x->value == y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

inline
Obj* lt(IntObj* x, IntObj *y) {
    return (x->value < y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

inline
Obj* le(IntObj* x, IntObj *y) {
    return (x->value <= y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

inline
Obj* gt(IntObj* x, IntObj *y) {
    return (x->value > y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

inline
Obj* ge(IntObj* x, IntObj *y) {
    return (x->value >= y->value) ? (Obj*)make_int_obj(0) : (Obj*)make_null_obj();
}

inline
NullObj* make_null_obj() {
    return &null_obj_singleton;
}

inline
ArrayObj* make_array_obj(IntObj *length, Obj* init) { 
    ++ array_count;
    ArrayObj* o = malloc(sizeof(ArrayObj));

    o->type = Array;
    o->length = length->value;
    o->data = malloc(sizeof(Obj*) * o->length);

    for (int i = 0; i < o->length; i++) {
        o->data[i] = init;
    }
    return o;
}

inline
IntObj* array_length(ArrayObj* a) {
    return make_int_obj(a->length);
}

inline
NullObj* array_set(ArrayObj* a, IntObj* i, Obj* v) {
    int index = i->value;
    a->data[index] = v;
    return make_null_obj();
}

inline
Obj* array_get(ArrayObj* a, IntObj* i) {
    return a->data[i->value];
}

inline
EnvObj* make_env_obj(Obj* parent) {
    ++ env_count;
    EnvObj* env = malloc(sizeof(EnvObj));
    env->type = Env;
    if (parent == NULL || parent->type == Null)
        env->table = ht_create(11);
    else
        env->table = ht_copy(((EnvObj*)parent)->table);
    return env;
}

inline
void add_entry(EnvObj* env, char* name, Entry* entry) {
    ht_put(env->table, name, entry);
}

inline
Entry* get_entry(EnvObj* env, char* name) {
    TIME_T t1, t2;
    FREQ_T freq;

    FREQ(freq);
    TIME(t1);

    Entry* entry = (Entry*)ht_get(env->table, name);

    TIME(t2);
    lookup_time_in_ms += ELASPED_TIME(t1, t2, freq);

    return entry;
}

inline
Entry* make_var_entry(Obj* obj) {
    VarEntry* e = malloc(sizeof(VarEntry));
    e->type = Var;
    e->value = obj;
    return (Entry*)e;
}

inline
Entry* make_func_entry(ScopeStmt* s, int nargs, char** args) {
    FuncEntry* e = malloc(sizeof(FuncEntry));
    e->type = Func;
    e->body = s;
    e->nargs = nargs;
    e->args = args;
    return (Entry*)e;
}

Obj* eval_exp(EnvObj* genv, EnvObj* env, Exp* e) {
    switch (e->tag) {
    case INT_EXP: {
#ifdef DEBUG
        printf("debug: INT\n");
#endif
        IntExp* e2 = (IntExp*)e;
        return (Obj*)make_int_obj(e2->value);
    }
    case NULL_EXP: {
        return (Obj*)make_null_obj();
    }
    case PRINTF_EXP: {
        PrintfExp* e2 = (PrintfExp*)e;
        int* res = malloc(sizeof(int) * (e2->nexps + 1));
        for (int i = 0; i < e2->nexps; i++) {
            Obj* obj = eval_exp(genv, env, e2->exps[i]);
            if (obj->type == Int)
                res[i] = ((IntObj*)obj)->value;
            else {
                printf("Error: Can only print Int objects.\n");
                exit(-1);
            }
        }
        int cur = 0;
        for (char* p = e2->format;*p;p++)
            if (*p != '~')
            printf("%c", *p);
            else
                printf("%d", res[cur++]);
        return (Obj*)make_null_obj();
    }
    case ARRAY_EXP: {
        ArrayExp* e2 = (ArrayExp*)e;
        Obj* length = eval_exp(genv, env, e2->length);
        if (length->type != Int) {
            printf("Error: array length is not int.\n");
            exit(-1);
        }
        Obj* init = eval_exp(genv, env, e2->init);
        return (Obj*)make_array_obj((IntObj*)length, init);
    }
    case OBJECT_EXP: {
#ifdef DEBUG
        printf("debug: OBJECT\n");
#endif
        ObjectExp* e2 = (ObjectExp*)e;
        EnvObj* obj = make_env_obj(eval_exp(genv, env, e2->parent));
        for (int i = 0; i < e2->nslots; i++) {
            exec_slotstmt(genv, env, obj, e2->slots[i]);
        }
        return (Obj*)obj;
    }
    case SLOT_EXP: {
#ifdef DEBUG
        printf("debug: Slot exp\n");
#endif
        SlotExp* e2 = (SlotExp*)e;
        Obj* obj = eval_exp(genv, env, e2->exp);
        if (obj->type != Env) {
            printf("Error: Accessing %s from a non-object value.\n", e2->name);
            exit(-1);
        }
        Entry* ent = get_entry((EnvObj*)obj, e2->name);
        if (ent == NULL) {
            printf("Error: %s is not found in the object.\n", e2->name);
            exit(-1);
        }
        if (ent->type != Var) {
            printf("Error: %s should be Var in the object.\n", e2->name);
            exit(-1);
        }
        return ((VarEntry*)ent)->value;
    }
    case SET_SLOT_EXP: {
#ifdef DEBUG
        printf("debug: Slot Set\n");
#endif
        SetSlotExp* e2 = (SetSlotExp*)e;
        Obj* obj = eval_exp(genv, env, e2->exp);
        if (obj->type != Env) {
            printf("Error: Accessing %s from a non-object value.\n", e2->name);
            exit(-1);
        }
        Entry* ent = get_entry((EnvObj*)obj, e2->name);
        if (ent == NULL) {
            printf("Error: %s is not found in the object.\n", e2->name);
            exit(-1);
        }
        if (ent->type != Var) {
            printf("Error: %s should be Var in the object.\n", e2->name);
            exit(-1);
        }
        add_entry((EnvObj*)obj, e2->name, make_var_entry(eval_exp(genv, env, e2->value)));
        return (Obj*)make_null_obj();
    }
    case CALL_SLOT_EXP: {
#ifdef DEBUG
        printf("debug: Slot Call\n");
#endif
        CallSlotExp* e2 = (CallSlotExp*)e;
#ifdef DEBUG
        printf("debug: Calling %s\n", e2->name);
#endif
        Obj* obj = eval_exp(genv, env, e2->exp);

        switch (obj->type) {
        case Int:
            ++int_calls;
            IntObj* iobj = (IntObj*)obj;
            Obj* para = eval_exp(genv, env, e2->args[0]);

            if (para->type != Int) {
                printf("Error: Operand %s to Int must be Int.\n", e2->name);
                exit(-1);
            }
            IntObj* ipara = (IntObj*)para;

            if (strcmp(e2->name, "add") == 0)
                return (Obj*)add(iobj, ipara);
            if (strcmp(e2->name, "sub") == 0)
                return (Obj*)subtract(iobj, ipara);
            if (strcmp(e2->name, "mul") == 0)
                return (Obj*)multiply(iobj, ipara);
            if (strcmp(e2->name, "div") == 0)
                return (Obj*)divide(iobj, ipara);
            if (strcmp(e2->name, "mod") == 0)
                return (Obj*)modulo(iobj, ipara);
            if (strcmp(e2->name, "lt") == 0)
                return (Obj*)lt(iobj, ipara);
            if (strcmp(e2->name, "gt") == 0)
                return (Obj*)gt(iobj, ipara);
            if (strcmp(e2->name, "le") == 0)
                return (Obj*)le(iobj, ipara);
            if (strcmp(e2->name, "ge") == 0)
                return (Obj*)ge(iobj, ipara);
            if (strcmp(e2->name, "eq") == 0)
                return (Obj*)eq(iobj, ipara);

            printf("Error: Operator %s not recognized.\n", e2->name);
            exit(-1);

        case Array:
            ++array_calls;

            ArrayObj* aobj = (ArrayObj*)obj;
            if (strcmp(e2->name, "length") == 0)
                return (Obj*)array_length(aobj);
            IntObj* index = (IntObj*)eval_exp(genv, env, e2->args[0]);
            if (strcmp(e2->name, "set") == 0) {
                array_set(aobj, index, eval_exp(genv, env, e2->args[1]));
                return (Obj*)make_null_obj();
            }
            if (strcmp(e2->name, "get") == 0)
                return array_get(aobj, index);
            printf("Error: Operator %s not recognized.\n", e2->name);
            exit(-1);

        case Env:
            ++env_calls;

            Entry* ent = get_entry((EnvObj*)obj, e2->name);

            if (ent == NULL) {
                printf("Error: Function %s not found.\n", e2->name);
                exit(-1);
            }
            if (ent->type != Func) {
                printf("Error: Calling %s that is not a Function in the object.\n", e2->name);
                exit(-1);
            }

            FuncEntry* func = (FuncEntry*)ent;

            if (e2->nargs != func->nargs) {
                printf("Error: Args number not match when calling object method %s. Giving %d while %d is required.\n", e2->name, e2->nargs, func->nargs);
                exit(-1);
            }

            EnvObj* new_env = make_env_obj(NULL);
            for (int i = 0; i < e2->nargs; i++) {
                add_entry(new_env, func->args[i], make_var_entry(eval_exp(genv, env, e2->args[i])));
            }
            add_entry(new_env, "this", make_var_entry(obj));
            Obj* res = eval_stmt(genv, new_env, func->body);

            ht_clear(new_env->table);
            free(new_env);

            return res;

        default:
            printf("Error: Calling %s from a NULL reference.\n", e2->name);
            exit(-1);
        }
    }
    case CALL_EXP: {
#ifdef DEBUG
        printf("debug: CALL\n");
#endif
        CallExp* e2 = (CallExp*)e;
#ifdef DEBUG
        printf("debug: %s\n", e2->name);
#endif
        Entry* ent = get_entry(genv, e2->name);
        if (ent == NULL) {
            printf("Error: Function %s not found\n", e2->name);
            exit(-1);
        }
        if (ent->type != Func) {
            printf("Error: Calling %s that is not a Function \n", e2->name);
            exit(-1);
        }
        FuncEntry* func = (FuncEntry*)ent;
        if (e2->nargs != func->nargs) {
            printf("Error: Args number not match when calling %s. Giving %d while %d is required.\n", e2->name, e2->nargs, func->nargs);
            exit(-1);
        }
        EnvObj* new_env = make_env_obj(NULL);
        for (int i = 0; i < e2->nargs; i++) {
            add_entry(new_env, func->args[i], make_var_entry(eval_exp(genv, env, e2->args[i])));
        }
        Obj* res = eval_stmt(genv, new_env, func->body);
        ht_clear(new_env->table);
        free(new_env);
        return res;
    }
    case SET_EXP: {
#ifdef DEBUG
        printf("debug: SET\n");
#endif
        SetExp* e2 = (SetExp*)e;
        Obj* res = eval_exp(genv, env, e2->exp);
        if (env != NULL && get_entry(env, e2->name) != NULL) {
            add_entry(env, e2->name, make_var_entry(res));
        }
        else
            if (get_entry(genv, e2->name) != NULL) {
                add_entry(genv, e2->name, make_var_entry(res));
            }
            else {
                printf("Error: variable not found when setting its value\n");
                exit(-1);
            }
            return NULL;
    }
    case IF_EXP: {
#ifdef DEBUG 
        printf("debug: If\n");
#endif
        IfExp* e2 = (IfExp*)e;
        Obj* pred = eval_exp(genv, env, e2->pred);
        if (pred->type == Null)
            return eval_stmt(genv, env, e2->alt);
        else
            return eval_stmt(genv, env, e2->conseq);
    }
    case WHILE_EXP: {
#ifdef DEBUG
        printf("debug: While\n");
#endif
        WhileExp* e2 = (WhileExp*)e;
        while (eval_exp(genv, env, e2->pred)->type == Int) {
            eval_stmt(genv, env, e2->body);
        }
        return (Obj*)make_null_obj();
    }
    case REF_EXP: {
#ifdef DEBUG
        printf("debug: REF\n");
#endif
        RefExp* e2 = (RefExp*)e;
        Entry* ent = NULL;
        if (env != NULL)
            ent = get_entry(env, e2->name);
        if (ent == NULL)
            ent = get_entry(genv, e2->name);
        if (ent == NULL) {
            printf("Error: variable not found when referring\n");
            exit(-1);
        }
        if (ent->type == Func) {
            printf("Error: Should ref to variable rather than function\n");
            exit(-1);
        }
        return ((VarEntry*)ent)->value;
    }
    default:
        printf("Unrecognized Expression with tag %d\n", e->tag);
        exit(-1);
    }
    return NULL;
}

inline
void exec_slotstmt(EnvObj* genv, EnvObj* env, EnvObj* obj, SlotStmt* s) {
    switch (s->tag) {
    case VAR_STMT: {
        SlotVar* s2 = (SlotVar*)s;
        add_entry(obj, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
        break;
    }
    case FN_STMT: {
        SlotMethod* s2 = (SlotMethod*)s;
        add_entry(obj, s2->name, make_func_entry(s2->body, s2->nargs, s2->args));
        break;
    }
    default:
        printf("Unrecognized slot statement with tag %d\n", s->tag);
        exit(-1);
    }
}

void exec_stmt(EnvObj* genv, EnvObj* env, ScopeStmt* s) {
    switch (s->tag) {
    case VAR_STMT: {
        ScopeVar* s2 = (ScopeVar*)s;
        if (env == NULL)
            add_entry(genv, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
        else
            add_entry(env, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
        break;
    }
    case FN_STMT: {
#ifdef DEBUG
        printf("debug: FN\n");
#endif
        ScopeFn* s2 = (ScopeFn*)s;
        add_entry(genv, s2->name, make_func_entry(s2->body, s2->nargs, s2->args));
        break;
    }
    case SEQ_STMT: {
#ifdef DEBUG
        printf("debug: SEQ\n");
#endif
        ScopeSeq* s2 = (ScopeSeq*)s;
        exec_stmt(genv, env, s2->a);
        exec_stmt(genv, env, s2->b);
        break;
    }
    case EXP_STMT: {
        ScopeExp* s2 = (ScopeExp*)s;
        eval_exp(genv, env, s2->exp);
        break;
    }
    default:
        printf("Unrecognized scope statement with tag %d\n", s->tag);
        exit(-1);
    }
}

Obj* eval_stmt(EnvObj* genv, EnvObj* env, ScopeStmt* s) {
    switch (s->tag) {
    case VAR_STMT: {
#ifdef DEBUG
        printf("debug: Var\n");
#endif
        ScopeVar* s2 = (ScopeVar*)s;
#ifdef DEBUG
        printf("debug: %s\n", s2->name);
#endif
        if (env == NULL)
            add_entry(genv, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
        else
            add_entry(env, s2->name, make_var_entry(eval_exp(genv, env, s2->exp)));
        return (Obj*)make_null_obj();
    }
    case FN_STMT: {
#ifdef DEBUG
        printf("debug: FN\n");
#endif
        ScopeFn* s2 = (ScopeFn*)s;
        add_entry(genv, s2->name, make_func_entry(s2->body, s2->nargs, s2->args));
        return (Obj*)make_null_obj();
    }
    case SEQ_STMT: {
#ifdef DEBUG
        printf("debug: SEQ\n");
#endif
        ScopeSeq* s2 = (ScopeSeq*)s;
        eval_stmt(genv, env, s2->a);
        return eval_stmt(genv, env, s2->b);
    }
    case EXP_STMT: {
#ifdef DEBUG
        printf("debug: EXP\n");
#endif
        ScopeExp* s2 = (ScopeExp*)s;
        return eval_exp(genv, env, s2->exp);
    }
    default:
        printf("Unrecognized scope statement with tag %d\n", s->tag);
        exit(-1);
    }
}

void print_stats() {
    fprintf(stderr, "int calls: %d.\n", int_calls);
    fprintf(stderr, "array calls: %d.\n", array_calls);
    fprintf(stderr, "env calls: %d.\n", env_calls);
    fprintf(stderr, "total method calls: %d.\n", int_calls + array_calls + env_calls);
    
    fprintf(stderr, "int objects: %d.\n", int_count);
    fprintf(stderr, "array objects: %d.\n", array_count);
    fprintf(stderr, "null objects: %d.\n", null_count);
    fprintf(stderr, "env objects: %d.\n", env_count);
    fprintf(stderr, "total objects: %d.\n", int_count + array_count + null_count + env_count);

    fprintf(stderr, "lookup time: %f ms.\n", lookup_time_in_ms);
    fprintf(stderr, "total time: %f ms.\n", total_time_in_ms);
}

void interpret(ScopeStmt* s) {
    TIME_T t1, t2;
    FREQ_T freq;

    FREQ(freq);
    TIME(t1);

    eval_stmt(make_env_obj(NULL), NULL, s);

    TIME(t2);
    total_time_in_ms += ELASPED_TIME(t1, t2, freq);

    print_stats();
}




