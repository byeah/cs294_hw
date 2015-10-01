#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"
#include "ast.h"
#include "compiler.h"
#include "bytecode.h"

//#define DEBUG
#ifdef _MSC_VER
#define inline __inline
#endif

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
int labelCnt;

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
    SlotValue* v = (SlotValue*)malloc(sizeof(SlotValue));
    v->tag = SLOT_VAL;
    v->name = getStrId(name);
    vector_add(pro->values,v);
    return pro->values->size-1;
}

int make_class() {
    ClassValue* v = (ClassValue*)malloc(sizeof(ClassValue));
    v->tag = CLASS_VAL;
    v->slots = make_vector();
    vector_add(pro->values,v);
    return pro->values->size-1;
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

ByteIns* make_array(){
    ByteIns* ins = (ByteIns*)malloc(sizeof(ByteIns));
    ins->tag = ARRAY_OP;
    return ins;
}

ByteIns* make_setLocal(int id) {
    SetLocalIns* ins = (SetLocalIns*)malloc(sizeof(SetLocalIns));
    ins->tag = SET_LOCAL_OP;
    ins->idx = id;
    return (ByteIns*) ins;
}

ByteIns* make_getLocal(int id) {
    GetLocalIns* ins = (GetLocalIns*)malloc(sizeof(GetLocalIns));
    ins->tag = GET_LOCAL_OP;
    ins->idx = id;
    return (ByteIns*) ins;
}

ByteIns* make_setGlobal(int id) {
    SetGlobalIns* ins = (SetGlobalIns*)malloc(sizeof(SetGlobalIns));
    ins->tag = SET_GLOBAL_OP;
    ins->name = id;
    return (ByteIns*) ins;
}

ByteIns* make_getGlobal(int id) {
    GetGlobalIns* ins = (GetGlobalIns*)malloc(sizeof(GetGlobalIns));
    ins->tag = GET_GLOBAL_OP;
    ins->name = id;
    return (ByteIns*) ins;
}

ByteIns* make_drop() {
    ByteIns* ins = (ByteIns*)malloc(sizeof(ByteIns));
    ins->tag = DROP_OP;
    return ins;
}

ByteIns* make_object(int id) {
    ObjectIns* ins = (ObjectIns*)malloc(sizeof(ObjectIns));
    ins->tag = OBJECT_OP;
    ins->class = id;
    return (ByteIns*)ins;
}

ByteIns* make_call_slot(int id,int n){
    CallSlotIns* ins = (CallSlotIns*)malloc(sizeof(CallSlotIns));
    ins->tag = CALL_SLOT_OP;
    ins->name = id;
    ins->arity = n;
    return (ByteIns*) ins;
}


ByteIns* make_label_ins(int id) {
    LabelIns* ins = (LabelIns*)malloc(sizeof(LabelIns));
    ins->tag = LABEL_OP;
    ins->name = id;
    return (ByteIns*) ins;
}

ByteIns* make_branch(int id) {
    BranchIns* ins = (BranchIns*)malloc(sizeof(BranchIns));
    ins->tag = BRANCH_OP;
    ins->name = id;
    return (ByteIns*) ins;
}

ByteIns* make_goto(int id) {
    GotoIns* ins = (GotoIns*)malloc(sizeof(GotoIns));
    ins->tag = GOTO_OP;
    ins->name = id;
    return (ByteIns*) ins;
}

ByteIns* make_return() {
    ByteIns* ins = (ByteIns*)malloc(sizeof(ByteIns));
    ins->tag = RETURN_OP;
    return ins;
}

ByteIns* make_call(int id,int n) {
    CallIns* ins = (CallIns*)malloc(sizeof(CallIns));
    ins->tag = CALL_OP;
    ins->name = id;
    ins->arity = n;
    return (ByteIns*)ins;
}

char* new_label() {
    char* name = malloc(sizeof(char)*15);
    sprintf(name,"__LABEL__%d",labelCnt++);
    return name;
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
        case ARRAY_EXP: {
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
            ClassValue* c = vector_get(pro->values, class_id);
            for(int i=0;i<e2->nslots;i++)
                vector_add(c->slots, (void*)compile_slotstmt(genv, env, m, e2->slots[i]));
            addIns(m, make_object(class_id));
            break;
        }
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
        case CALL_SLOT_EXP: {
#ifdef DEBUG
            printf("debug: Slot Call\n");
#endif
            CallSlotExp* e2 = (CallSlotExp*)e;
#ifdef DEBUG
            printf("debug: Calling %s\n", e2->name);
#endif
            compile_exp(genv, env, m, e2->exp);
            for (int i = 0; i < e2->nargs; i++)
                compile_exp(genv, env, m, e2->args[i]);
            int sid = getStrId(e2->name);
            addIns(m, make_call_slot(sid, e2->nargs+1));
            break;
        }
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
        case SET_EXP: {
#ifdef DEBUG
            printf("debug: SET\n");
#endif
            SetExp* e2 = (SetExp*)e;
            compile_exp(genv, env, m, e2->exp);
            int id;
            if (env != NULL && (id = ht_get_i(env, e2->name)) >=0) {
                addIns(m, make_setLocal(id));
            }
            else
                if ((id = ht_get_i(genv, e2->name)) >=0) {
                    addIns(m, make_setGlobal(getStrId(e2->name)));
                    //add_entry(genv, e2->name, make_var_entry(res));
                }
                else {
                    printf("Error: variable not found when setting its value\n");
                    exit(-1);
                }
            addIns(m, make_drop());
            break;
        }
        case IF_EXP: {
#ifdef DEBUG
            printf("debug: If\n");
#endif
            IfExp* e2 = (IfExp*)e;
            int label1 = getStrId(new_label());
            int label2 = getStrId(new_label());
            compile_exp(genv, env, m, e2->pred);
            addIns(m, make_branch(label1));
            compile_stmt(genv, env, m, e2->alt);
            addIns(m, make_goto(label2));
            addIns(m, make_label_ins(label1));
            compile_stmt(genv, env, m, e2->conseq);
            addIns(m, make_label_ins(label2));
            break;
        }
        case WHILE_EXP: {
#ifdef DEBUG
            printf("debug: While\n");
#endif
            WhileExp* e2 = (WhileExp*)e;
            int start = getStrId(new_label());
            int body = getStrId(new_label());
            int end = getStrId(new_label());
            addIns(m, make_label_ins(start));
            compile_exp(genv, env, m, e2->pred);
            addIns(m, make_branch(body));
            addIns(m, make_goto(end));
            addIns(m, make_label_ins(body));
            compile_stmt(genv, env, m, e2->body);
            addIns(m, make_goto(start));
            addIns(m, make_label_ins(end));
            break;
        }
        case REF_EXP: {
            RefExp* e2 = (RefExp*)e;
#ifdef DEBUG
            printf("debug: REF  %s\n",e2->name);
#endif
            int id = -1;
            if (env != NULL)
                id = ht_get_i(env, e2->name);
            if (id < 0) {
                id = ht_get_i(genv, e2->name);
                if (id  < 0) {
                    printf("Error: variable not found when referring\n");
                    exit(-1);
                }
                if (((Value*)vector_get(pro->values, id))->tag != SLOT_VAL) {
                    printf("Error: Referring to a non-variable\n");
                    exit(-1);
                }
                addIns(m, make_getGlobal(getStrId(e2->name)));
            }
            else {
                addIns(m, make_getLocal(id));
            }
            break;
        }
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
            Hashtable_i* new_env = ht_create_i(1007);
            for(int i=0;i<s2->nargs;i++)
                ht_put_i(new_env, s2->args[i], i);
            compile_stmt(genv, new_env, func, s2->body);
            addIns(func, make_return());
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
                compile_exp(genv, env, m, s2->exp);
                addIns(m, make_setGlobal(getStrId(s2->name)));
            }
            else{
                int id = m->nlocals+m->nargs;
                ht_put_i(env, s2->name, m->nlocals+m->nargs);
                m->nlocals++;
                compile_exp(genv, env, m, s2->exp);
                addIns(m, make_setLocal(id));
            }
            addIns(m, make_drop());
            break;
        }
        case FN_STMT: {
#ifdef DEBUG
            printf("debug: FN\n");
#endif
            ScopeFn* s2 = (ScopeFn*)s;
            int func_id = make_method(s2->nargs, s2->name);
            MethodValue* func = vector_get(pro->values, func_id);
            Hashtable_i* new_env = ht_create_i(1007);
            for(int i=0;i<s2->nargs;i++)
                ht_put_i(new_env, s2->args[i], i);
            ht_put_i(genv, s2->name, func_id);
            compile_stmt(genv, new_env, func, s2->body);
            vector_add(pro->slots, (void*)func_id);
            addIns(func, make_return());
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
    labelCnt = 0;
    compile_stmt(ht_create_i(1007),NULL,vector_get(pro->values, pro->entry),stmt);
    print_prog(pro);
    printf("\n");
    return pro;
}


