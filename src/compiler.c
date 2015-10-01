#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"
#include "ast.h"
#include "compiler.h"
#include "bytecode.h"

//#define DEBUG


// hashtable.c

Hashtable_i* ht_create_i(int size) {
    Hashtable_i* hashtable = NULL;
    
    if (size < 1) {
        return NULL;
    }
    
    if ((hashtable = malloc(sizeof(Hashtable_i))) == NULL) {
        return NULL;
    }
    
    if ((hashtable->table = malloc(sizeof(entry_t_i) * size)) == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < size; ++i) {
        hashtable->table[i] = NULL;
    }
    
    hashtable->size = size;
    
    return hashtable;
}

inline
int ht_hash_i(Hashtable_i* hashtable, char* key) {
    size_t hash, i;
    size_t length = strlen(key);
    
    for (hash = i = 0; i < length; ++i) {
        hash += key[i], hash += (hash << 10), hash ^= (hash >> 6);
    }
    hash += (hash << 3), hash ^= (hash >> 11), hash += (hash << 15);
    
    return hash % hashtable->size;
}

inline
entry_t_i* ht_newpair_i(char *key, int value) {
    entry_t_i* newpair;
    if ((newpair = malloc(sizeof(entry_t_i))) == NULL) {
        return NULL;
    }
    // No longer duplicate key string. Use it at your own risk :)
    /*
     if ((newpair->key = strdup(key)) == NULL) {
     return NULL;
     }
     */
    newpair->key = key;
    newpair->value = value;
    newpair->next = NULL;
    return newpair;
}

void ht_put_i(Hashtable_i* hashtable, char* key, int value) {
    entry_t_i* newpair = NULL;
    entry_t_i* current = NULL;
    entry_t_i* prev = NULL;
    
    int bin = ht_hash_i(hashtable, key);
    current = hashtable->table[bin];
    
    while (current != NULL && strcmp(key, current->key) != 0) {
        prev = current;
        current = current->next;
    }
    
    /* There's already a pair.  Let's replace that string. */
    if (current != NULL) {
        current->value = value;
    }
    /* Nope, could't find it.  Time to grow a pair. */
    else {
        newpair = ht_newpair_i(key, value);
        
        /* We're at the start of the linked list in this bin. */
        if (prev == NULL) {
            hashtable->table[bin] = newpair;
        }
        /* We're at the end of the linked list in this bin. */
        else {
            prev->next = newpair;
        }
    }
}

inline
int ht_get_local_i(Hashtable_i *hashtable, char *key, int bin) {
    entry_t_i* current = NULL;
    entry_t_i* prev = NULL;
    
    /* Step through the bin, looking for our value. */
    current = hashtable->table[bin];
    while (current != NULL && strcmp(key, current->key) != 0) {
        prev = current;
        current = current->next;
    }
    
    /* Did we actually find anything? */
    if (current == NULL)
        return -1;
    else
        return current->value;
}

int ht_get_i(Hashtable_i *hashtable, char *key) {
    int bin = ht_hash_i(hashtable, key);
    return ht_get_local_i(hashtable, key, bin);
}

void ht_clear_i(Hashtable_i *hashtable) {
    for (int i = 0; i < hashtable->size; ++i) {
        entry_t_i* current = hashtable->table[i];
        while (current != NULL) {
            entry_t_i* temp = current->next;
            //free(current->key);
            free(current);
            current = temp;
        }
        hashtable->table[i] = NULL;
    }
}

Program* pro;
Hashtable_i* names;
int nullId;

int make_int(int value){  // Can optimize
    IntValue* v = (IntValue*)malloc(sizeof(IntValue));
    v->tag = INT_VAL;
    v->value = value;
    vector_add(pro->values,v);
    return pro->values->size-1;
}

int make_null() {
    Value* v = (Value*)malloc(sizeof(Value));
    v->tag = NULL_VAL;
    vector_add(pro->values,v);
    return pro->values->size-1;
}

int get_null() {
    return nullId;
}

int getStrId(char* str){ // Return the string id in the constant pool, if non-exist, add it into the pool first.
    int sid = ht_get_i(names, str);
    if (sid < 0){
        StringValue* s = (StringValue*)malloc(sizeof(StringValue));
            s->tag = STRING_VAL;
        s->value = str;
        vector_add(pro->values, s);
        sid = pro->values->size-1;
        ht_put_i(names,str,sid);
    }
    return sid;
}

int make_method(int args,char* name){
    MethodValue* m = (MethodValue*)malloc(sizeof(MethodValue));
    m->code = make_vector();
    m->tag = METHOD_VAL;
    m->name = getStrId(name);
    m->nargs = args;
    m->nlocals = 0;
    vector_add(pro->values,m);
    return pro->values->size-1;
}

int make_slot(char* name){
    return -1;
}



void addIns(MethodValue* m, ByteIns*ins){
    vector_add(m->code, ins);
}

ByteIns* make_lit(int id) {
    LitIns* ins = (LitIns*)malloc(sizeof(LitIns));
    ins->tag = LIT_OP;
    ins->idx = id;
    return (ByteIns*)ins;
}

ByteIns* make_print(int id,int n) {
    PrintfIns* ins = (PrintfIns*)malloc(sizeof(PrintfIns));
    ins->tag = PRINTF_OP;
    ins->format = id;
    ins->arity = n;
    return (ByteIns*)ins;
}

ByteIns* make_array();
ByteIns* make_setLocal(int);
ByteIns* make_getLocal(int);
ByteIns* make_setGlobal(int);
ByteIns* make_getGlobal(int);
ByteIns* make_drop();

ByteIns* make_object(int);

ByteIns* make_call(int id,int n) {
    CallIns* ins = (CallIns*)malloc(sizeof(CallIns));
    ins->tag = CALL_OP;
    ins->name = id;
    ins->arity = n;
    return (ByteIns*)ins;
}


void compile_exp(Hashtable_i* genv, Hashtable_i* env, MethodValue* m, Exp* e){
    switch (e->tag) {
        case INT_EXP: {
#ifdef DEBUG
            printf("debug: INT\n");
#endif
            IntExp* e2 = (IntExp*)e;
            int id = make_int(e2->value);
            addIns(m, make_lit(id));
            break;
        }
        case NULL_EXP: {
            int id = get_null();
            addIns(m, make_lit(id));
            break;
        }
        case PRINTF_EXP: {
            PrintfExp* e2 = (PrintfExp*)e;
            int formatId = getStrId(e2->format);
            for(int i=0;i<e2->nexps;i++)
                compile_exp(genv, env, m, e2->exps[i]);
            addIns(m, make_print(formatId,e2->nexps));
            break;
        }
        /*case ARRAY_EXP: {
            ArrayExp* e2 = (ArrayExp*)e;
            compile_exp(genv, env, m, e2->length);
            compile_exp(genv, env, m, e2->init);
            addIns(m, make_array());
            break;
        }
        case OBJECT_EXP: {
#ifdef DEBUG
            printf("debug: OBJECT\n");
#endif
            ObjectExp* e2 = (ObjectExp*)e;
            int class_id = make_class();
            for(int i=0;i<e2->nslots;i++)
                compile_slotstmt(genv, env, m, e2->slots[i]);
            addIns(m, make_object(class_id));
            break;
        }*/
/*        case SLOT_EXP: {
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
            //add_entry((EnvObj*)obj, e2->name, make_var_entry(eval_exp(genv, env, e2->value)));
            ((VarEntry*)ent)->value = eval_exp(genv, env, e2->value);
            return (Obj*)make_null_obj();
        }*/
        /*case CALL_SLOT_EXP: {
#ifdef DEBUG
            printf("debug: Slot Call\n");
#endif
            CallSlotExp* e2 = (CallSlotExp*)e;
#ifdef DEBUG
            printf("debug: Calling %s\n", e2->name);
#endif
            Obj* obj = eval_exp(genv, env, e2->exp);
            
            switch (obj->type) {
                case Int: {
#ifdef DEBUG
                    ++int_calls;
#endif
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
                }
                    
                case Array: {
#ifdef DEBUG
                    ++array_calls;
#endif
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
                }
                    
                case Env: {
#ifdef DEBUG
                    ++env_calls;
#endif
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
                }
                    
                default:
                    printf("Error: Calling %s from a NULL reference.\n", e2->name);
                    exit(-1);
            }
        }*/
        case CALL_EXP: {
#ifdef DEBUG
            printf("debug: CALL\n");
#endif
            CallExp* e2 = (CallExp*)e;
#ifdef DEBUG
            printf("debug: %s\n", e2->name);
#endif
            int func_id = ht_get_i(genv, e2->name);
            if (func_id < 0) {
                printf("Error: Function %s not found\n", e2->name);
                exit(-1);
            }
            MethodValue* func = vector_get(pro->values, func_id);
            if (e2->nargs != func->nargs) {
                printf("Error: Args number not match when calling %s. Giving %d while %d is required.\n", e2->name, e2->nargs, func->nargs);
                exit(-1);
            }
            for (int i = 0; i < e2->nargs; i++) {
                compile_exp(genv, env, m, e2->args[i]);
            }
            addIns(m,make_call(getStrId(e2->name),e2->nargs));
            break;
        }
        /*case SET_EXP: {
#ifdef DEBUG
            printf("debug: SET\n");
#endif
            SetExp* e2 = (SetExp*)e;
            Obj* res = eval_exp(genv, env, e2->exp);
            Entry* ent;
            if (env != NULL && (ent = get_entry(env, e2->name)) != NULL) {
                //add_entry(env, e2->name, make_var_entry(res));
            }
            else
                if ((ent = get_entry(genv, e2->name)) != NULL) {
                    //add_entry(genv, e2->name, make_var_entry(res));
                }
                else {
                    printf("Error: variable not found when setting its value\n");
                    exit(-1);
                }
            if (ent->type != Var){
                printf("Error: Setting value to a function name %s\n",e2->name);
                exit(-1);
            }
            ((VarEntry*)ent)->value = res;
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
        }*/
        default:
            printf("Unrecognized Expression with tag %d\n", e->tag);
            exit(-1);
    }
}

inline
int compile_slotstmt(Hashtable_i* genv, Hashtable_i* env, MethodValue* m, SlotStmt* s){ // Return the slotValue id in the constant pool
    switch (s->tag) {
        case VAR_STMT: {
            SlotVar* s2 = (SlotVar*)s;
            int slot_id = make_slot(s2->name);
            compile_exp(genv, env, m, s2->exp);
            return slot_id;
        }
        case FN_STMT: {
            SlotMethod* s2 = (SlotMethod*)s;
            int func_id = make_method(s2->nargs, s2->name);
            MethodValue* func = vector_get(pro->values, func_id);
            compile_stmt(genv, ht_create_i(107), func, s2->body);
            return func_id;
        }
        default:
            printf("Unrecognized slot statement with tag %d\n", s->tag);
            exit(-1);
    }
}

void compile_stmt(Hashtable_i* genv, Hashtable_i* env, MethodValue* m, ScopeStmt* s){
    switch (s->tag) {
        case VAR_STMT: {
#ifdef DEBUG
            printf("debug: Var\n");
#endif
            ScopeVar* s2 = (ScopeVar*)s;
#ifdef DEBUG
            printf("debug: %s\n", s2->name);
#endif
            if (env == NULL){
                int slot_id = make_slot(s2->name);
                ht_put_i(genv, s2->name, slot_id);
                vector_add(pro->slots, (void*)slot_id);
            }
            else{
                ht_put_i(env, s2->name, m->nlocals);
                m->nlocals++;
                compile_exp(genv, env, m, s2->exp);
            }
            break;
        }
        case FN_STMT: {
#ifdef DEBUG
            printf("debug: FN\n");
#endif
            ScopeFn* s2 = (ScopeFn*)s;
            int func_id = make_method(s2->nargs, s2->name);
            MethodValue* func = vector_get(pro->values, func_id);
            compile_stmt(genv, ht_create_i(107), func, s2->body);
            ht_put_i(genv, s2->name, func_id);
            vector_add(pro->slots, (void*)func_id);
            break;
        }
        case SEQ_STMT: {
#ifdef DEBUG
            printf("debug: SEQ\n");
#endif
            ScopeSeq* s2 = (ScopeSeq*)s;
            compile_stmt(genv, env, m, s2->a);
            compile_stmt(genv, env, m, s2->b);
            break;
        }
        case EXP_STMT: {
#ifdef DEBUG
            printf("debug: EXP\n");
#endif
            ScopeExp* s2 = (ScopeExp*)s;
            compile_exp(genv, env, m, s2->exp);
            break;
        }
        default:
            printf("Unrecognized scope statement with tag %d\n", s->tag);
            exit(-1);
    }
}



Program* compile (ScopeStmt* stmt) {
    pro = (Program*)malloc(sizeof(Program));
    pro->slots = make_vector();
    pro->values = make_vector();
    names = ht_create_i(10007);
    pro->entry = make_method(0,"__ENTRY_FUNC__"); //Reserved name for entry func
    nullId = make_null();
    compile_stmt(ht_create_i(1007),NULL,vector_get(pro->values, pro->entry),stmt);
    print_prog(pro);
    printf("\n");
    return pro;
}


