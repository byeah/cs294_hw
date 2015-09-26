#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/mman.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

#define DEBUG
#ifdef _MSC_VER
#define inline __inline
#endif

int debug = 0;

// hashtable.h


struct entry_s {
    char* key;
    void* value;
    struct entry_s* next;
};

typedef struct entry_s entry_t;

struct hashtable_s {
    int size;
    struct entry_s** table;
};

typedef struct hashtable_s Hashtable;

Hashtable* ht_create(int size);

void ht_put(Hashtable* hashtable, char* key, void* value);
void* ht_get(Hashtable *hashtable, char *key);
void ht_remove(Hashtable *hashtable, char *key);
void ht_free(Hashtable *hashtable, void(*free_val) (void *));

#ifdef DEBUG
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

#ifdef DEBUG
static int int_calls = 0;
static int array_calls = 0;
static int env_calls = 0;

static int int_count = 0;
static int array_count = 0;
static int null_count = 0;
static int env_count = 0;

static double lookup_time_in_ms = 0.0;
static double total_time_in_ms = 0.0;
#endif
#endif



// hashtable.c

Hashtable* ht_create(int size) {
    Hashtable* hashtable = NULL;

    if (size < 1) {
        return NULL;
    }

    if ((hashtable = malloc(sizeof(Hashtable))) == NULL) {
        return NULL;
    }

    if ((hashtable->table = malloc(sizeof(entry_t) * size)) == NULL) {
        return NULL;
    }

    for (int i = 0; i < size; ++i) {
        hashtable->table[i] = NULL;
    }

    hashtable->size = size;

    return hashtable;
}

inline
int ht_hash(Hashtable* hashtable, char* key) {
    size_t hash, i;
    size_t length = strlen(key);

    for (hash = i = 0; i < length; ++i) {
        hash += key[i], hash += (hash << 10), hash ^= (hash >> 6);
    }
    hash += (hash << 3), hash ^= (hash >> 11), hash += (hash << 15);

    return hash % hashtable->size;
}

inline
entry_t* ht_newpair(char *key, void *value) {
    entry_t* newpair;
    if ((newpair = malloc(sizeof(entry_t))) == NULL) {
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

void ht_put(Hashtable* hashtable, char* key, void* value) {
    entry_t* newpair = NULL;
    entry_t* current = NULL;
    entry_t* prev = NULL;

    int bin = ht_hash(hashtable, key);
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
        newpair = ht_newpair(key, value);

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
void* ht_get_local(Hashtable *hashtable, char *key, int bin) {
    entry_t* current = NULL;
    entry_t* prev = NULL;

    /* Step through the bin, looking for our value. */
    current = hashtable->table[bin];
    while (current != NULL && strcmp(key, current->key) != 0) {
        prev = current;
        current = current->next;
    }

    /* Did we actually find anything? */
    if (current == NULL) {
        return NULL;
    }
    else {
        return current->value;
    }
}

void* ht_get(Hashtable *hashtable, char *key) {
    int bin = ht_hash(hashtable, key);
    return ht_get_local(hashtable, key, bin);
}

void ht_remove(Hashtable *hashtable, char *key) {
    int bin = 0;
    entry_t* current = NULL;
    entry_t* prev = NULL;

    bin = ht_hash(hashtable, key);

    /* Step through the bin, looking for our value. */
    current = hashtable->table[bin];
    while (current != NULL && strcmp(key, current->key) != 0) {
        prev = current;
        current = current->next;
    }

    /* Did we actually find anything? */
    if (current == NULL) {
        return;
    }
    else {
        /* We're at the start of the linked list in this bin. */
        if (prev == NULL) {
            hashtable->table[bin] = current->next;
        }
        /* Otherwise */
        else {
            prev->next = current->next;
        }
        // free(current->key);
        free(current);
    }
}

void ht_free(Hashtable *hashtable, void(*free_val) (void *)) {
    for (int i = 0; i < hashtable->size; ++i) {
        entry_t* current = hashtable->table[i];
        while (current != NULL) {
            entry_t* temp = current->next;
            if (free_val != NULL)
                free_val(current->value);
            free(current);
            current = temp;
        }
        hashtable->table[i] = NULL;
    }
    free(hashtable);
}

typedef enum {
    Int,
    Array,
    Null,
    Env,
    Method
} ObjType;

typedef struct {
    ObjType type;
} Obj;

typedef struct {
    ObjType type;
    int value;
} IntObj;

typedef struct {
    ObjType type;
} NullObj;

typedef struct {
    ObjType type;
    int length;
    Obj** data;
} ArrayObj;

typedef struct {
    ObjType type;
    MethodValue* method;
} MethodObj;

typedef struct env_obj_s {
    ObjType type;
    Hashtable* table;
    struct env_obj_s* parent;
} EnvObj;

inline
int obj_type(Obj* o) {
    return o->type;
}

inline
MethodObj* make_method_obj(MethodValue* method) {
    MethodObj* o = malloc(sizeof(MethodObj));
    o->type = Method;
    o->method = method;
    return o;
}

static NullObj null_obj_singleton = { .type = Null };

inline
NullObj* make_null_obj() {
    return &null_obj_singleton;
}

inline
EnvObj* make_env_obj(EnvObj* parent) {
#ifdef DEBUG
    ++ env_count;
#endif
    if (parent && parent->type == Null)
        parent = NULL;

    if (parent && parent->type != Env) {
        printf("Error: Parent can only be Env or Null objects.\n");
        exit(-1);
    }

    EnvObj* env = malloc(sizeof(EnvObj));
    env->type = Env;
    env->table = ht_create(11);
    env->parent = parent;
    return env;
}

//inline
void add_entry(EnvObj* env, char* name, void* entry) {
    EnvObj* current_env = env;
    EnvObj* target_env = env;

    while (current_env != NULL) {
        void* entry = ht_get(current_env->table, name);
        if (entry != NULL) {
            target_env = current_env;
            break;
        }
        else {
            current_env = current_env->parent;
        }
    }

    ht_put(target_env->table, name, entry);
}

inline
void* get_entry(EnvObj* env, char* name) {
#ifdef DEBUG
    TIME_T t1, t2;
    FREQ_T freq;

    FREQ(freq);
    TIME(t1);
#endif
    EnvObj* current_env = env;
    while (current_env != NULL) {
        void* entry = ht_get(current_env->table, name);
        if (entry != NULL) {
#ifdef DEBUG
            TIME(t2);
            lookup_time_in_ms += ELASPED_TIME(t1, t2, freq);
#endif
            return entry;
        }
        else {
            current_env = current_env->parent;
        }
    }

#ifdef DEBUG
    TIME(t2);
    lookup_time_in_ms += ELASPED_TIME(t1, t2, freq);
#endif

    return NULL;
}

inline
IntObj* make_int_obj(int value) {
#ifdef DEBUG
    ++ int_count;
#endif
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
ArrayObj* make_array_obj(IntObj *length, Obj* init) {
#ifdef DEBUG
    ++ array_count;
#endif
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

// vm.c

typedef struct {
    Vector* code;
    int pc;
} LabelAddr;

LabelAddr* make_label(Vector* code, int pc) {
    LabelAddr* label = (LabelAddr *)malloc(sizeof(LabelAddr));
    label->code = code;
    label->pc = pc;
    return label;
}

void free_label(LabelAddr* label) {
    free(label);
}

typedef struct frame_t {
    Vector* slots; // Vector of Obj*
    LabelAddr return_addr;
    struct frame_t* parent;
} Frame;

Frame* make_frame(Frame* parent, Vector* code, int pc, int num_slots) {
    Frame* frame = (Frame *)malloc(sizeof(Frame));

    frame->slots = make_vector();
    vector_set_length(frame->slots, num_slots, NULL);

    frame->return_addr.code = code;
    frame->return_addr.pc = pc;

    frame->parent = parent;

    return frame;
}

void free_frame(Frame* frame) {
    vector_free(frame->slots);
    free(frame);
}

static Hashtable* global_vars = NULL;
static Hashtable* labels = NULL;
static Vector* code = NULL;
static int pc = 0;
static Vector* operand = NULL;
static Frame* local_frame = NULL;


static inline
void assert(int criteria, char *msg) {
    if (!criteria) {
        fprintf(stderr, msg);
        exit(1);
    }
}

static inline
void vm_init(Program* p) {
    global_vars = ht_create(13);
    operand = make_vector();

    // init entry
    MethodValue* entry_func = vector_get(p->values, p->entry);
    local_frame = make_frame(NULL, NULL, 0, entry_func->nargs + entry_func->nlocals);
    code = entry_func->code;
    pc = 0;

    // init global
    for (int i = 0; i < p->slots->size; ++i) {
        int index = (int)vector_get(p->slots, i);
        Value* value = vector_get(p->values, index);
        switch (value->tag) {
            case METHOD_VAL: {
                MethodValue* method_val = (MethodValue *)value;
                StringValue *name = vector_get(p->values, method_val->name);
                assert(name->tag == STRING_VAL, "Invalid object type.\n");
                MethodObj* method_obj = make_method_obj(method_val);
                ht_put(global_vars, name->value, method_obj);
                break;
            }
            case SLOT_VAL: {
                SlotValue* slot_val = (SlotValue *)value;
                StringValue *name = vector_get(p->values, slot_val->name);
                assert(name->tag == STRING_VAL, "Invalid object type.\n");
                ht_put(global_vars, name->value, make_null_obj());
                break;
            }
            default:
                assert(0, "Invalid value type.\n");
        }
    }

    // preprocess labels
    labels = ht_create(13);

    for (int i = 0; i < p->values->size; ++i) {
        Value* val = vector_get(p->values, i);
        if (val->tag != METHOD_VAL)
            continue;
        MethodValue* method = (MethodValue *)val;
        for (int j = 0; j < method->code->size; ++j) {
            ByteIns* ins = vector_get(method->code, j);
            if (ins->tag != LABEL_OP)
                continue;
            LabelIns* label_ins = (LabelIns *)ins;
            StringValue *label_string = vector_get(p->values, label_ins->name);
            assert(label_string->tag == STRING_VAL, "Invalid object type for LABEL.\n");

            ht_put(labels, label_string->value, make_label(method->code, j));
        }
    }
}

static inline
void vm_cleanup() {
    ht_free(global_vars, NULL);
    ht_free(labels, free);
    code = NULL;
    pc = 0;
    vector_free(operand);

    while (local_frame != NULL) {
        Frame* t = local_frame->parent;
        free_frame(local_frame);
        local_frame = t;
    }
}

static inline
void push(Obj* obj) {
    vector_add(operand, obj);
}

static inline
void* pop() {
    return vector_pop(operand);
}

static inline
void* peek() {
    return vector_peek(operand);
}

#ifdef DEBUG
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
#endif


void interpret_bc(Program* p) {

    vm_init(p);
    //printf("Interpreting Bytecode Program:\n");
    //print_prog(p);
    while (pc < code->size) {
        ByteIns* ins = vector_get(code, pc);
        switch (ins->tag)
        {
            case LIT_OP: {
                LitIns* lit_ins = (LitIns *)ins;
                int index = lit_ins->idx;
                Value* val = vector_get(p->values, index);
                if (val->tag == INT_VAL) {
                    push((Obj*)make_int_obj(((IntValue *)val)->value));
                }
                else if (val->tag == NULL_VAL) {
                    push((Obj*)make_null_obj());
                }
                else {
                    assert(0, "Invalid object type for LIT.\n");
                }
                break;
            }
            case ARRAY_OP: {
                Obj* init_val = pop();
                IntObj* length = pop();
                assert(length->type == Int, "Invalid length type for ARRAY.\n");
                ArrayObj* obj = make_array_obj(length, init_val);
                push((Obj*)obj);
                break;
            }
            case PRINTF_OP: {
                if (debug) printf("!!PRINT\n");
                PrintfIns* printf_ins = (PrintfIns *)ins;
                int* res = malloc(sizeof(int) * (printf_ins->arity));
                //                IntObj* ii = vector_peek(operand);
                //                printf("%d %d\n",operand->size,printf_ins->arity);
                for (int i = 0; i < printf_ins->arity; i++) {
                    IntObj* obj = pop();
                    assert(obj->type == Int, "Invalid object type for PRINTF.\n");
                    res[i] = obj->value;
                }
                int cur = printf_ins->arity;
                StringValue *format = vector_get(p->values, printf_ins->format);
                assert(format->tag == STRING_VAL, "Invalid object type for PRINTF_OP.\n");
                for (char* p = format->value; *p && cur >= 0; p++) {
                    if (*p != '~')
                        printf("%c", *p);
                    else
                        printf("%d", res[--cur]);
                }
                push((Obj*)make_null_obj());
                break;
            }
            case OBJECT_OP: {
                if (debug) printf("!!OBJECT\n");
                ObjectIns* obj_ins = (ObjectIns*)ins;
                ClassValue* class = vector_get(p->values, obj_ins->class);
                int slot_count = 0;
                for (int i = 0; i<class->slots->size; i++) {
                    Value* v = vector_get(p->values, (int)vector_get(class->slots, i));
                    if (v->tag == SLOT_VAL)
                        slot_count++;
                }
                Obj** args = malloc(sizeof(Obj *) * (slot_count + 1));
                for (int i = 0; i < slot_count + 1; i++) {
                    args[i] = pop();
                }
                EnvObj* obj = make_env_obj((EnvObj*)args[slot_count]);
                int cnt = 0;
                for (int i = 0; i<class->slots->size; i++) {
                    Value* v = vector_get(p->values, (int)vector_get(class->slots, i));
                    if (v->tag == METHOD_VAL) {
                        MethodValue* method_val = (MethodValue *)v;
                        StringValue *name = vector_get(p->values, method_val->name);
                        assert(name->tag == STRING_VAL, "Invalid object type.\n");
                        MethodObj* method_obj = make_method_obj(method_val);
                        add_entry(obj, name->value, method_obj);
                    }
                    else
                        if (v->tag == SLOT_VAL) {
                            SlotValue* slot_val = (SlotValue *)v;
                            StringValue *name = vector_get(p->values, slot_val->name);
                            assert(name->tag == STRING_VAL, "Invalid object type.\n");
                            add_entry(obj, name->value, (void*)args[slot_count - 1 - cnt]);
                            cnt++;
                        }
                }
                push((Obj*)obj);
                //printf("   object #%d", i->class);
                break;
            }
            case SLOT_OP: {
                if (debug) printf("!!SLOT_OP\n");
                SlotIns* slot_ins = (SlotIns*)ins;
                EnvObj* obj = pop();
                assert(obj->type == Env, "Get slot should be with an object!");
                StringValue *name = vector_get(p->values, slot_ins->name);
                assert(name->tag == STRING_VAL, "Invalid string type for SLOT_OP.\n");
                //                push(((SlotObj*)get_entry(obj, name->value))->slot);
                push(get_entry(obj, name->value));
                break;
            }
            case SET_SLOT_OP: {
                if (debug) printf("!!SET_SLOT_OP\n");
                SetSlotIns* set_slot_ins = (SetSlotIns*)ins;
                Obj* value = pop();
                EnvObj* obj = pop();
                assert(obj->type == Env, "Get slot should be with an object!");
                StringValue *name = vector_get(p->values, set_slot_ins->name);
                assert(name->tag == STRING_VAL, "Invalid string type for SET_SLOT_OP.\n");
                add_entry(obj, name->value, value);
                //                ((SlotObj*)get_entry(obj, name->value))->slot = value;
                push(value);
                break;
            }
            case CALL_SLOT_OP: {
                CallSlotIns* call_slot = (CallSlotIns*)ins;
                if (debug) printf("   call-slot #%d %d\n", call_slot->name, call_slot->arity);
                Obj** args = malloc(sizeof(Obj *) * (call_slot->arity));
                for (int i = 0; i < call_slot->arity; i++) {
                    args[i] = pop();
                }
                Obj* receiver = args[call_slot->arity - 1];
                StringValue *name = vector_get(p->values, call_slot->name);
                assert(name->tag == STRING_VAL, "Invalid string type for CALL_OP.\n");
                //printf("%d\n",receiver->type);
                switch (receiver->type) {
                    case Int: {
#ifdef DEBUG
                        ++ int_calls;
#endif
                        
                        if (debug) printf("Int call slot\n");
                        IntObj* iobj = (IntObj*)receiver;
                        assert(call_slot->arity == 2, "Invalid parameter number for CALL_Slot_OP.\n");
                        Obj* para = args[0];
                        if (para->type != Int) {
                            printf("Error: Operand %s to Int must be Int.\n", name->value);
                            exit(-1);
                        }
                        IntObj* ipara = (IntObj*)para;

                        if (strcmp(name->value, "add") == 0)
                            push((Obj*)add(iobj, ipara)); else
                            if (strcmp(name->value, "sub") == 0)
                            push((Obj*)subtract(iobj, ipara)); else
                            if (strcmp(name->value, "mul") == 0)
                            push((Obj*)multiply(iobj, ipara)); else
                            if (strcmp(name->value, "div") == 0)
                            push((Obj*)divide(iobj, ipara)); else
                            if (strcmp(name->value, "mod") == 0)
                            push((Obj*)modulo(iobj, ipara)); else
                            if (strcmp(name->value, "lt") == 0)
                            push((Obj*)lt(iobj, ipara)); else
                            if (strcmp(name->value, "gt") == 0)
                            push((Obj*)gt(iobj, ipara)); else
                            if (strcmp(name->value, "le") == 0)
                            push((Obj*)le(iobj, ipara)); else
                            if (strcmp(name->value, "ge") == 0)
                            push((Obj*)ge(iobj, ipara)); else
                            if (strcmp(name->value, "eq") == 0)
                            push((Obj*)eq(iobj, ipara));
                            else
                                printf("Error: Operator %s not recognized.\n", name->value);
                        break;
                    }

                    case Array: {
#ifdef DEBUG
                        ++ array_calls;
#endif
                        
                        if (debug) printf("Array call slot\n");
                        ArrayObj* aobj = (ArrayObj*)receiver;
                        if (strcmp(name->value, "length") == 0)
                            push((Obj*)array_length(aobj)); else
                            if (strcmp(name->value, "set") == 0) {
                                IntObj* index = (IntObj*)args[1];
                                array_set(aobj, index, args[0]);
                                push((Obj*)make_null_obj());
                            }
                            else
                                if (strcmp(name->value, "get") == 0)
                            push(array_get(aobj, (IntObj*)args[0]));
                                else
                                {
                                    printf("Error: Operator %s not recognized.\n", name->value);
                                    exit(-1);
                                }
                        break;
                    }

                    case Env: {
#ifdef DEBUG
                        ++ env_calls;
#endif

                        EnvObj* env = (EnvObj*)receiver;
                        StringValue *name = vector_get(p->values, call_slot->name);
                        assert(name->tag == STRING_VAL, "Invalid string type for CALL_OP.\n");

                        MethodObj *method_obj = get_entry(env, name->value);
                        assert(method_obj && obj_type((Obj*)method_obj) == Method, "Invalid method type for CALL_OP.\n");
                        Frame* new_frame = make_frame(local_frame, code, pc,
                            method_obj->method->nargs + method_obj->method->nlocals + 1);
                        vector_set(new_frame->slots, 0, env);
                        for (int i = 0; i < call_slot->arity - 1; i++) {
                            vector_set(new_frame->slots, i + 1, args[call_slot->arity - 2 - i]);
                        }
                        local_frame = new_frame;
                        code = method_obj->method->code;
                        pc = -1;
                        break;
                    }
                    default:
                        printf("Error: Calling %s from a NULL reference.\n", name->value);
                        exit(-1);
                }
                break;
            }
            case SET_LOCAL_OP: {
                SetLocalIns *set_local_ins = (SetLocalIns *)ins;
                vector_set(local_frame->slots, set_local_ins->idx, peek());
                break;
            }
            case GET_LOCAL_OP: {
                GetLocalIns *get_local_ins = (GetLocalIns *)ins;
                if (debug) printf("!!GET_LOCAL %d\n", get_local_ins->idx);
                push(vector_get(local_frame->slots, get_local_ins->idx));
                break;
            }
            case SET_GLOBAL_OP: {
                SetGlobalIns *set_global_ins = (SetGlobalIns *)ins;
                StringValue *name = vector_get(p->values, set_global_ins->name);
                assert(name->tag == STRING_VAL, "Invalid object type for SET_GLOBAL_OP.\n");
                ht_put(global_vars, name->value, peek());
                break;
            }
            case GET_GLOBAL_OP: {
                GetGlobalIns *get_global_ins = (GetGlobalIns *)ins;
                StringValue *name = vector_get(p->values, get_global_ins->name);
                assert(name->tag == STRING_VAL, "Invalid object type for GET_GLOBAL_OP.\n");
                push(ht_get(global_vars, name->value));
                break;
            }
            case DROP_OP: {
                pop();
                break;
            }
            case LABEL_OP: {
                // Already processed
                break;
            }
            case BRANCH_OP: {
                Obj* predicate = pop();
                if (predicate->type != Null) {
                    BranchIns* branch_ins = (BranchIns *)ins;
                    StringValue *name = vector_get(p->values, branch_ins->name);
                    assert(name->tag == STRING_VAL, "Invalid object type for BRANCH_OP.\n");
                    LabelAddr* label = ht_get(labels, name->value);
                    assert(label != NULL, "Label not found in BRANCH_OP.\n");
                    assert(label->code == code, "Cross method jump is not allowed for BRANCH_OP.\n");
                    pc = label->pc;
                }
                break;
            }
            case GOTO_OP: {
                GotoIns* goto_ins = (GotoIns *)ins;
                StringValue *name = vector_get(p->values, goto_ins->name);
                assert(name->tag == STRING_VAL, "Invalid object type for GOTO_OP.\n");
                LabelAddr* label = ht_get(labels, name->value);
                assert(label != NULL, "Label not found in GOTO_OP.\n");
                assert(label->code == code, "Cross method jump is not allowed for GOTO_OP.\n");
                pc = label->pc;
                break;
            }
            case CALL_OP: {
                CallIns* call_ins = (CallIns *)ins;

                Obj** args = malloc(sizeof(Obj *) * (call_ins->arity));
                for (int i = 0; i < call_ins->arity; i++) {
                    args[i] = pop();
                }

                StringValue *name = vector_get(p->values, call_ins->name);
                assert(name->tag == STRING_VAL, "Invalid string type for CALL_OP.\n");

                if (debug) printf("!!CALL %s\n", name->value);

                MethodObj *method_obj = ht_get(global_vars, name->value);
                assert(method_obj && obj_type((Obj*)method_obj) == Method, "Invalid method type for CALL_OP.\n");

                Frame* new_frame = make_frame(local_frame, code, pc,
                    method_obj->method->nargs + method_obj->method->nlocals);

                for (int i = 0; i < call_ins->arity; i++) {
                    vector_set(new_frame->slots, i, args[call_ins->arity - 1 - i]);
                }

                local_frame = new_frame;
                code = method_obj->method->code;
                pc = -1;
                break;
            }
            case RETURN_OP: {
                if (debug) printf("!!Return\n");
                Frame *t = local_frame;
                local_frame = t->parent;
                code = t->return_addr.code;
                pc = t->return_addr.pc;
                free_frame(t);

                if (local_frame == NULL){
#ifdef DEBUG
                    print_stats();
#endif
                    return;
                }

                break;
            }
            default: {
                assert(0, "Invalid instruction.\n");
                break;
            }
        }
        ++pc;
    }

    printf("\n");
}

