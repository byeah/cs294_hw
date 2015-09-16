#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/mman.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

#ifdef _MSC_VER
#define inline __inline
#endif

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

void ht_free(Hashtable *hashtable, void (*free_val) (void *)) {
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
    Env
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

inline
int obj_type(Obj* o) {
    return o->type;
}

static NullObj null_obj_singleton = { .type = Null };

inline
NullObj* make_null_obj() {
    return &null_obj_singleton;
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

typedef struct frame_t {
    Vector* slots; // Vector of Obj*
    ByteIns* return_addr;
    struct frame_t* parent;
} Frame;

Frame* make_frame(Frame* parent, ByteIns* return_addr, int num_slots) {
    Frame* frame = (Frame *)malloc(sizeof(Frame));
    
    frame->slots = make_vector();
    vector_set_length(frame->slots, num_slots, NULL);
    frame->return_addr = return_addr;
    frame->parent = parent;
    
    return frame;
}

void free_frame(Frame* frame) {
    vector_free(frame->slots);
    free(frame);
}

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
    local_frame = make_frame(NULL, NULL, entry_func->nargs + entry_func->nlocals);
    code = entry_func->code;
    pc = 0;

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
                push(make_int_obj(((IntValue *)val)->value));
            }
            else if (val->tag == NULL_VAL) {
                push(make_null_obj());
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
            push(obj);
            break;
        }
        case PRINTF_OP: {
            PrintfIns* printf_ins = (PrintfIns *)ins;
            int* res = malloc(sizeof(int) * (printf_ins->arity + 1));
            for (int i = 0; i < printf_ins->arity; i++) {
                IntObj* obj = pop();
                assert(obj->type == Int, "Invalid object type for PRINTF.\n");
                res[i] = obj->value;
            }
            int cur = printf_ins->arity;
            StringValue *format = vector_get(p->values, printf_ins->format);
            assert(format->tag == STRING_VAL, "Invalid object type for PRINTF_OP.\n");
            for (char* p = format->value; *p && cur > 0; p++) {
                if (*p != '~')
                    printf("%c", *p);
                else
                    printf("%d", res[--cur]);
            }
            push(make_null_obj());
            break;
        }
        case SET_LOCAL_OP: {
            SetLocalIns *set_local_ins = (SetLocalIns *)ins;
            vector_set(local_frame->slots, set_local_ins->idx, peek());
            break;
        }
        case GET_LOCAL_OP: {
            GetLocalIns *get_local_ins = (GetLocalIns *)ins;
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
        default: {
            assert(0, "Invalid instruction.\n");
            break;
        }
        }
        ++pc;
    }
    
    printf("\n");
}

