#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
    Int = 0,
    Null,
	Array,
} ObjType;

typedef struct {
    int64_t type;
	int64_t value;
} IntObj;

typedef struct {
	int64_t type;
	int64_t space;
} NullObj;

typedef struct {
	int64_t type;
	int64_t length;
	void* slots[0];
} ArrayObj;

typedef struct object_t {
	int64_t type;
    struct object_t* parent;
	void* varslots[0];
} ObjectObj;

static struct {
	void* head;
    int64_t used;
    int64_t total;
} heap;

inline static 
void init_heap() {
	heap.head = malloc(1024 * 1024);
	heap.used = 0;
	heap.total = 1024 * 1024;
}

inline static 
void free_heap() {
	free(heap.head);
}

inline static
void garbage_collector() {}

inline static
void* halloc(int64_t nbytes) {
	if (heap.used + nbytes > heap.total)
		garbage_collector();

	if (heap.used + nbytes > heap.total) {
		fprintf(stderr, "out of memory.\n");
		exit(1);
	}
	void *p = (unsigned char *)heap.head + heap.used;
	heap.used += nbytes;
	return p;
}

inline static
int64_t obj_type(NullObj* o) {
    return o->type;
}

inline static
IntObj* make_int_obj() {
	return halloc(sizeof(IntObj));
}

inline static
NullObj* make_null_obj() {
	return halloc(sizeof(NullObj));
}

inline static
IntObj* add(IntObj* x, IntObj *y) {
    return make_int_obj(x->value + y->value);
}

inline static
IntObj* subtract(IntObj* x, IntObj *y) {
    return make_int_obj(x->value - y->value);
}

inline static
IntObj* multiply(IntObj* x, IntObj *y) {
    return make_int_obj(x->value * y->value);
}

inline static
IntObj* divide(IntObj* x, IntObj *y) {
    return make_int_obj(x->value / y->value);
}

inline static
IntObj* modulo(IntObj* x, IntObj *y) {
    return make_int_obj(x->value % y->value);
}

inline static
void* eq(IntObj* x, IntObj *y) {
    return (x->value == y->value) ? (void *) make_int_obj(0) : (void *) make_null_obj();
}

inline static
void* lt(IntObj* x, IntObj *y) {
    return (x->value < y->value) ? (void *)make_int_obj(0) : (void *) make_null_obj();
}

inline static
void* le(IntObj* x, IntObj *y) {
    return (x->value <= y->value) ? (void *)make_int_obj(0) : (void *) make_null_obj();
}

inline static
void* gt(IntObj* x, IntObj *y) {
    return (x->value > y->value) ? (void *)make_int_obj(0) : (void *) make_null_obj();
}

inline static
void* ge(IntObj* x, IntObj *y) {
    return (x->value >= y->value) ? (void *)make_int_obj(0) : (void *) make_null_obj();
}

inline static
ArrayObj* make_array_obj(IntObj *length, void* init) {
    ArrayObj* o = halloc(sizeof(ArrayObj) + length->value * sizeof(void *));

    o->type = Array;
    o->length = length->value;

    for (int i = 0; i < o->length; i++) {
        o->slots[i] = init;
    }
    return o;
}

inline static
ObjectObj* make_object_obj(int64_t type, ObjectObj *parent, int num_slots) {
    ObjectObj* o = halloc(sizeof(ObjectObj) + num_slots * sizeof(void *));
    o->type = type;
    o->parent = parent;
    return o;
}

inline static
IntObj* array_length(ArrayObj* a) {
    return make_int_obj(a->length);
}

inline static
NullObj* array_set(ArrayObj* a, IntObj* i, void* v) {
    int64_t index = i->value;
    a->slots[index] = v;
    return make_null_obj();
}

inline static
void* array_get(ArrayObj* a, IntObj* i) {
    return a->slots[i->value];
}

// vm.c

typedef struct {
    Vector* code;
    int64_t pc;
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
    struct frame_t *parent;
    Vector* code;
    int64_t pc;
    void* slots[0];
} Frame;

static unsigned char stack[1024 * 1024];
static Frame* sp = NULL;
static Frame* next_sp = (Frame *) stack;

static Hashtable* global_func_name = NULL;
static Hashtable* global_var_name = NULL;
static Hashtable* labels = NULL;

static void* operand[1024];
static int operand_size = 0;

static void* global_var[1024];

static inline
void push_frame(Vector* code, int pc, int num_slots) {
    next_sp->parent = sp;
    next_sp->code = code;
    next_sp->pc = pc;

    sp = next_sp;
    next_sp = (Frame *)((unsigned char *) next_sp + sizeof(Frame) + num_slots * sizeof(void *));
}

static inline
void pop_frame() {
    next_sp = sp;
    sp = sp->parent;
}


static inline
void assert(int criteria, char *msg) {
#ifdef DEBUG
    if (!criteria) {
        fprintf(stderr, msg);
        exit(1);
    }
#endif
}

static 
int find_slot_index(ClassValue* cvalue, int name) {
    int slot_cnt = 0;

    for (int i = 0; i < cvalue->slots->size; ++i) {
        Value* value = vector_get(cvalue->slots, i);
        
        if (value->tag == SLOT_VAL) {
            SlotValue *sval = (SlotValue *)value;
            if (sval->name == name)
                return slot_cnt;
            ++slot_cnt;
        }
    }

    return -1;
}

static
MethodValue* find_method(ClassValue* cvalue, int name) {
    for (int i = 0; i < cvalue->slots->size; ++i) {
        Value* value = vector_get(cvalue->slots, i);
        if (value->tag == METHOD_VAL) {
            MethodValue *mval = (MethodValue *)value;
            if (mval->name == name)
                return mval;
        }
    }
    return NULL;
}

static inline
void vm_init(Program* p) {
    // init heap
    init_heap();

    // init global symbol tables
    global_func_name = ht_create(13);
    global_var_name = ht_create(13);

    // init entry func
    MethodValue* entry_func = vector_get(p->values, p->entry);
    push_frame(entry_func->code, 0, entry_func->nargs + entry_func->nlocals);
    
    // init global
    int global_var_cnt = 0;
    for (int i = 0; i < p->slots->size; ++i) {
        int index = (int)vector_get(p->slots, i);
        Value* value = vector_get(p->values, index);
        switch (value->tag) {
            case METHOD_VAL: {
                MethodValue* method_val = (MethodValue *)value;
                StringValue *name = vector_get(p->values, method_val->name);
                assert(name->tag == STRING_VAL, "Invalid object type.\n");

                ht_put(global_func_name, name->value, method_val);

                break;
            }
            case SLOT_VAL: {
                SlotValue* slot_val = (SlotValue *)value;
                StringValue *name = vector_get(p->values, slot_val->name);
                assert(name->tag == STRING_VAL, "Invalid object type.\n");

                ht_put(global_var_name, name->value, &global_var[global_var_cnt++]);

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
    ht_free(global_func_name, NULL);
    ht_free(global_var_name, NULL);
    ht_free(labels, free);
}

static inline
void push(void* obj) {
    operand[operand_size++] = obj;
}

static inline
void* pop() {
    return operand[--operand_size];
}

static inline
void* peek() {
    return operand[operand_size - 1];
}

void interpret_bc(Program* p) {
    vm_init(p);
    //printf("Interpreting Bytecode Program:\n");
    //print_prog(p);
    while (sp->pc < sp->code->size) {
        ByteIns* ins = vector_get(sp->code, sp->pc);
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
                void* init_val = pop();
                IntObj* length = pop();
                assert(length->type == Int, "Invalid length type for ARRAY.\n");
                ArrayObj* obj = make_array_obj(length, init_val);
                push(obj);
                break;
            }
            case PRINTF_OP: {
                PrintfIns* printf_ins = (PrintfIns *)ins;
                int64_t* res = malloc(sizeof(int64_t) * (printf_ins->arity));

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
                        printf("%lld", res[--cur]);
                }
                push(make_null_obj());

                free(res);
                break;
            }
            case OBJECT_OP: {
                ObjectIns* obj_ins = (ObjectIns*)ins;
                
                int64_t class_type = obj_ins->class + 3;

                ClassValue* class = vector_get(p->values, obj_ins->class);

                int slot_count = 0;
                for (int i = 0; i < class->slots->size; i++) {
                    Value* v = vector_get(p->values, (int)vector_get(class->slots, i));
                    if (v->tag == SLOT_VAL)
                        slot_count++;
                }

                void** args = malloc(sizeof(void *) * (slot_count + 1));
                for (int i = 0; i < slot_count + 1; i++) {
                    args[i] = pop();
                }

                ObjectObj* parent = args[slot_count];
                ObjectObj* obj = make_object_obj(class_type, parent, slot_count);

                int cnt = 0;
                for (int i = 0; i < class->slots->size; i++) {
                    Value* v = vector_get(p->values, (int)vector_get(class->slots, i));
                    if (v->tag == METHOD_VAL) {

                    }
                    else if (v->tag == SLOT_VAL) {
                        obj->varslots[cnt] = args[slot_count - 1 - cnt];
                        ++cnt;
                    }
                }

                push(obj);

                free(args);

                break;
            }
            case SLOT_OP: {
                SlotIns* slot_ins = (SlotIns*)ins;

                ObjectObj* obj = pop();
                assert(obj->type > 2, "Get slot should be with an object!");
                
                //StringValue *name = vector_get(p->values, slot_ins->name);
                //assert(name->tag == STRING_VAL, "Invalid string type for SLOT_OP.\n");
                
                ClassValue* class = vector_get(p->values, obj->type - 3);

                int idx = find_slot_index(class, slot_ins->name);
                assert(idx >= 0, "Could not find the slot by name.\n");

                push(obj->varslots[idx]);

                break;
            }
            case SET_SLOT_OP: {
                SetSlotIns* set_slot_ins = (SetSlotIns*)ins;

                void* value = pop();
                ObjectObj* obj = pop();
                assert(obj->type > 2, "Get slot should be with an object!");

                //StringValue *name = vector_get(p->values, set_slot_ins->name);
                //assert(name->tag == STRING_VAL, "Invalid string type for SET_SLOT_OP.\n");
                
                ClassValue* class = vector_get(p->values, obj->type - 3);

                int idx = find_slot_index(class, set_slot_ins->name);
                assert(idx >= 0, "Could not find the slot by name.\n");

                obj->varslots[idx] = value;
                push(value);

                break;
            }
            case CALL_SLOT_OP: {
                CallSlotIns* call_slot = (CallSlotIns*)ins;
                void** args = malloc(sizeof(void *) * (call_slot->arity));
                
                for (int i = 0; i < call_slot->arity; i++) {
                    args[i] = pop();
                }
                
                NullObj* receiver = args[call_slot->arity - 1];

                StringValue *name = vector_get(p->values, call_slot->name);
                
                assert(name->tag == STRING_VAL, "Invalid string type for CALL_OP.\n");
                //printf("%d\n",receiver->type);
                
                switch (receiver->type) {
                    case Int: {
                        IntObj* iobj = (IntObj*) receiver;

                        assert(call_slot->arity == 2, "Invalid parameter number for CALL_Slot_OP.\n");
                        
                        IntObj* ipara = args[0];
                        if (ipara->type != Int) {
                            printf("Error: Operand %s to Int must be Int.\n", name->value);
                            exit(-1);
                        }

                        if (strcmp(name->value, "add") == 0)
                            push(add(iobj, ipara)); 
                        else if (strcmp(name->value, "sub") == 0)
                            push(subtract(iobj, ipara)); 
                        else if (strcmp(name->value, "mul") == 0)
                            push(multiply(iobj, ipara)); 
                        else if (strcmp(name->value, "div") == 0)
                            push(divide(iobj, ipara)); 
                        else if (strcmp(name->value, "mod") == 0)
                            push(modulo(iobj, ipara)); 
                        else if (strcmp(name->value, "lt") == 0)
                            push(lt(iobj, ipara)); 
                        else if (strcmp(name->value, "gt") == 0)
                            push(gt(iobj, ipara)); 
                        else if (strcmp(name->value, "le") == 0)
                            push(le(iobj, ipara)); 
                        else if (strcmp(name->value, "ge") == 0)
                            push(ge(iobj, ipara)); 
                        else if (strcmp(name->value, "eq") == 0)
                            push(eq(iobj, ipara));
                        else
                            printf("Error: Operator %s not recognized.\n", name->value);

                        break;
                    }

                    case Array: {
                        ArrayObj* aobj = (ArrayObj*) receiver;

                        if (strcmp(name->value, "length") == 0)
                            push(array_length(aobj)); 
                        else if (strcmp(name->value, "set") == 0) {
                            IntObj* index = (IntObj*) args[1];
                            array_set(aobj, index, args[0]);
                            push(make_null_obj());
                        } else if (strcmp(name->value, "get") == 0) {
                            push(array_get(aobj, (IntObj*)args[0]));
                        } else {
                            printf("Error: Operator %s not recognized.\n", name->value);
                            exit(-1);
                        }
                        break;
                    }

                    case Null: {
                        printf("Error: Calling %s from a NULL reference.\n", name->value);
                        exit(-1);
                    }

                    default: {
                        ObjectObj* oobj = (ObjectObj *) receiver;

                        //StringValue *name = vector_get(p->values, call_slot->name);
                        //assert(name->tag == STRING_VAL, "Invalid string type for CALL_SLOT_OP.\n");

                        ClassValue* class = vector_get(p->values, oobj->type - 3);

                        MethodValue* method = find_method(class, call_slot->name);
                        assert(method && method->tag == METHOD_VAL, "Invalid method type for CALL_SLOT_OP.\n");
                        
                        push_frame(method->code, -1, method->nargs + method->nlocals + 1);
                      
                        sp->slots[0] = oobj;

                        for (int i = 1; i < call_slot->arity; i++) {
                            sp->slots[i] = args[call_slot->arity - 1 - i];
                        }
                        break;
                    }
                }
                break;
            }
            case SET_LOCAL_OP: {
                SetLocalIns *set_local_ins = (SetLocalIns *)ins;
                sp->slots[set_local_ins->idx] = peek();
                break;
            }
            case GET_LOCAL_OP: {
                GetLocalIns *get_local_ins = (GetLocalIns *)ins;
                push(sp->slots[get_local_ins->idx]);
                break;
            }
            case SET_GLOBAL_OP: {
                SetGlobalIns *set_global_ins = (SetGlobalIns *)ins;
                StringValue *name = vector_get(p->values, set_global_ins->name);
                assert(name->tag == STRING_VAL, "Invalid object type for SET_GLOBAL_OP.\n");

                void** global_var_slot = ht_get(global_var_name, name->value);
                *global_var_slot = peek();
                
                break;
            }
            case GET_GLOBAL_OP: {
                GetGlobalIns *get_global_ins = (GetGlobalIns *)ins;
                StringValue *name = vector_get(p->values, get_global_ins->name);
                assert(name->tag == STRING_VAL, "Invalid object type for GET_GLOBAL_OP.\n");

                void** global_var_slot = ht_get(global_var_name, name->value);
                push(*global_var_slot);

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
                NullObj* predicate = pop();
                if (predicate->type != Null) {
                    BranchIns* branch_ins = (BranchIns *)ins;
                    StringValue *name = vector_get(p->values, branch_ins->name);
                    
                    assert(name->tag == STRING_VAL, "Invalid object type for BRANCH_OP.\n");

                    LabelAddr* label = ht_get(labels, name->value);
                    
                    assert(label != NULL, "Label not found in BRANCH_OP.\n");
                    assert(label->code == sp->code, "Cross method jump is not allowed for BRANCH_OP.\n");
                    
                    sp->pc = label->pc;
                }
                break;
            }
            case GOTO_OP: {
                GotoIns* goto_ins = (GotoIns *)ins;
                StringValue *name = vector_get(p->values, goto_ins->name);

                assert(name->tag == STRING_VAL, "Invalid object type for GOTO_OP.\n");

                LabelAddr* label = ht_get(labels, name->value);

                assert(label != NULL, "Label not found in GOTO_OP.\n");
                assert(label->code == sp->code, "Cross method jump is not allowed for GOTO_OP.\n");

                sp->pc = label->pc;
                break;
            }
            case CALL_OP: {
                CallIns* call_ins = (CallIns *)ins;

                void** args = malloc(sizeof(void *) * (call_ins->arity));
                for (int i = 0; i < call_ins->arity; i++) {
                    args[i] = pop();
                }

                StringValue *name = vector_get(p->values, call_ins->name);
                assert(name->tag == STRING_VAL, "Invalid string type for CALL_OP.\n");

                MethodValue *method_val = ht_get(global_func_name, name->value);
                assert(method_val && method_val->tag == METHOD_VAL, "Invalid method type for CALL_OP.\n");

                push_frame(method_val->code, -1, method_val->nargs + method_val->nlocals);



                for (int i = 0; i < call_ins->arity; i++) {
                    sp->slots[i] = args[call_ins->arity - 1 - i];
                }

                free(args);
                break;
            }
            case RETURN_OP: {
                pop_frame();

                if (sp == NULL){
                    return;
                }

                break;
            }
            default: {
                assert(0, "Invalid instruction.\n");
                break;
            }
        }
        ++sp->pc;
    }

    printf("\n");
}

