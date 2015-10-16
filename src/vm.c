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

//#define DEBUG

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
} Obj;

typedef struct {
    int64_t type;
    int64_t value;
} IntObj;

typedef struct {
    int64_t type;
    void* forward;
} BrokenHeartObj;

typedef struct {
    int64_t type;
    int64_t space;
} NullObj;

typedef struct {
    int64_t type;
    int64_t length;
    void* slots[0];
} ArrayObj;

typedef struct {
    int64_t type;
    void* parent;
    void* varslots[0];
} ObjectObj;

static struct {
    char* head;
    int64_t used;
    int64_t total;
} heap, freespace;

inline static
void init_heap() {
    heap.head = malloc(1024 * 1024);
    heap.used = 0;
    heap.total = 1024 * 1024;
    freespace.head = malloc(1024 * 1024);
    freespace.used = 0;
    freespace.total = 1024 * 1024;
}

typedef struct frame_t {
    struct frame_t *parent;
    Vector* code;
    int64_t pc;
    void* slots[0];
} Frame;

static unsigned char *stack[1024 * 1024];
static Frame* sp = NULL;
static Frame* next_sp = (Frame *)stack;

static Hashtable* global_func_name = NULL;
static Hashtable* global_var_name = NULL;
static Hashtable* labels = NULL;

static void* operand[1024] = { NULL };
static int operand_count = 0;

static void* global_var[128] = { NULL };
static int global_var_count = 0;

typedef struct {
    int type;
    int varslot_count;
} ClassInfo;

static ClassInfo class_table[1024];
static int class_table_count = 0;

static inline
void assert(int criteria, char *msg) {
#ifdef DEBUG
    if (!criteria) {
        fprintf(stderr, msg);
        exit(1);
    }
#endif
}

inline static
void free_heap() {
    free(heap.head);
    free(freespace.head);
}

inline static
Obj* get_post_gc_ptr(Obj *obj) {
    // already copied?
    if (obj->type == -1) {
        return ((BrokenHeartObj *)obj)->forward;
    }

    // calculate size
    int64_t size = -1;
    if (obj->type == Int || obj->type == Null) {
        size = sizeof(IntObj);
    }
    else if (obj->type == Array) {
        size = sizeof(ArrayObj) + ((ArrayObj *)obj)->length * sizeof(void *);
    }
    else {
        int numslots = -1;
        for (int i = 0; i < class_table_count; ++i) {
            if (class_table[i].type == obj->type) {
                numslots = class_table[i].varslot_count;
                break;
            }
        }
        assert(numslots != -1, "Class not found.\n");
        size = sizeof(ObjectObj) + numslots * sizeof(void *);
    }

    // copy
    Obj* ret = (Obj *)(freespace.head + freespace.used);
    memcpy(ret, obj, size);
    freespace.used += size;

    // broken heart
    ((BrokenHeartObj *)obj)->type = -1;
    ((BrokenHeartObj *)obj)->forward = ret;

    return ret;
}

inline static
void scan_root_set() {
    // global variables
    for (int i = 0; i < global_var_count; ++i) {
        Obj* o = global_var[i];
        if (o) {
            Obj* post_gc_o = get_post_gc_ptr(o);
#ifdef DEBUG
            fprintf(stderr, "Global %d: %llx -> %llx\n", i, o, post_gc_o);
#endif
            global_var[i] = post_gc_o;
        }
    }
    // operand stack
    for (int i = 0; i < operand_count; ++i) {
        Obj* o = operand[i];
        Obj* post_gc_o = get_post_gc_ptr(o);
#ifdef DEBUG
        fprintf(stderr, "Operand: %llx -> %llx\n", o, post_gc_o);
#endif
        operand[i] = post_gc_o;
    }
    // local frames
    for (Frame *p = sp, *np = next_sp; p != NULL; np = p, p = p->parent) {
        for (int i = 0; &(p->slots[i]) < (void **)np; ++i) {
            Obj* o = p->slots[i];
            if (o) {
                Obj* post_gc_o = get_post_gc_ptr(o);
#ifdef DEBUG
                fprintf(stderr, "Local: %llx -> %llx\n", o, post_gc_o);
#endif
                p->slots[i] = post_gc_o;
            }
        }
    }
}

inline static
void garbage_collector() {
#ifdef DEBUG
    fprintf(stderr, "Heap: %llx - %llx.\n", heap.head, heap.head + heap.total);
    fprintf(stderr, "Free: %llx - %llx.\n", freespace.head, freespace.head + freespace.total);
    fprintf(stderr, "Scan root set.\n");
#endif
    // root set
    scan_root_set();
#ifdef DEBUG
    fprintf(stderr, "Scan free space.\n");
#endif
    // free space
    for (Obj *p = (Obj *)freespace.head; (Obj *)p < (Obj *)(freespace.head + freespace.used); ) {
        if (p->type == Int || p->type == Null) {
            p += 2;
        }
        else if (p->type == Array) {
            ArrayObj *arr_p = (ArrayObj *)p;
            for (int i = 0; i < arr_p->length; ++i) {
                Obj* post_gc_o = get_post_gc_ptr(arr_p->slots[i]);
#ifdef DEBUG
                fprintf(stderr, "Array slot: %llx -> %llx\n", arr_p->slots[i], post_gc_o);
#endif
                arr_p->slots[i] = post_gc_o;
            }
            p += (2 + arr_p->length);
        }
        else if (p->type > 2) {
            ObjectObj *obj_p = (ObjectObj *)p;
            int numslots = -1;
            for (int i = 0; i < class_table_count; ++i) {
                if (class_table[i].type == p->type) {
                    numslots = class_table[i].varslot_count;
                    break;
                }
            }
            assert(numslots != -1, "Class not found.\n");
            for (int i = 0; i < numslots; ++i) {
                Obj* post_gc_o = get_post_gc_ptr(obj_p->varslots[i]);
#ifdef DEBUG
                fprintf(stderr, "Object slot: %llx -> %llx\n", obj_p->varslots[i], post_gc_o);
#endif
                obj_p->varslots[i] = post_gc_o;
            }
            p += (2 + numslots);
        }
        else {
            printf("Unrecognized type: %lld.\n", p->type);
            exit(1);
        }
    }
#ifdef DEBUG
    fprintf(stderr, "Swap free space and heap.\n");
#endif
    void *tmp = heap.head;
    int64_t tmp_size = heap.total;

    heap.head = freespace.head;
    heap.total = freespace.total;
    heap.used = freespace.used;

    freespace.head = tmp;
    freespace.used = 0;
    freespace.total = tmp_size;
}

inline static
void* halloc(int64_t nbytes) {
    //if (heap.used + nbytes > heap.total) {
        garbage_collector();
    //}

    if (heap.used + nbytes > heap.total) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    void *p = heap.head + heap.used;
    heap.used += nbytes;
    return p;
}

inline static
IntObj* make_int_obj(int64_t value) {
    IntObj* o = halloc(sizeof(IntObj));
    o->type = Int;
    o->value = value;
    return o;
}

inline static
NullObj* make_null_obj() {
    NullObj* o = halloc(sizeof(NullObj));
    o->type = Null;
    return o;
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
    return (x->value == y->value) ? (void *)make_int_obj(0) : (void *)make_null_obj();
}

inline static
void* lt(IntObj* x, IntObj *y) {
    return (x->value < y->value) ? (void *)make_int_obj(0) : (void *)make_null_obj();
}

inline static
void* le(IntObj* x, IntObj *y) {
    return (x->value <= y->value) ? (void *)make_int_obj(0) : (void *)make_null_obj();
}

inline static
void* gt(IntObj* x, IntObj *y) {
    return (x->value > y->value) ? (void *)make_int_obj(0) : (void *)make_null_obj();
}

inline static
void* ge(IntObj* x, IntObj *y) {
    return (x->value >= y->value) ? (void *)make_int_obj(0) : (void *)make_null_obj();
}

inline static
ArrayObj* make_array_obj(int length) {
    ArrayObj* o = halloc(sizeof(ArrayObj) + length * sizeof(void *));

    o->type = Array;
    o->length = length;

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

static inline
void push_frame(Vector* code, int pc, int num_slots) {
    next_sp->parent = sp;
    next_sp->code = code;
    next_sp->pc = pc;

    sp = next_sp;
    next_sp = (Frame *)((unsigned char *)next_sp + sizeof(Frame) + num_slots * sizeof(void *));

    for (int i = 0; i < num_slots; ++i)
        sp->slots[i] = NULL;
}

static inline
void pop_frame() {
    next_sp = sp;
    sp = sp->parent;
}

static
int find_slot_index(Vector *values, ClassValue* cvalue, int name) {
    int slot_cnt = 0;

    for (int i = 0; i < cvalue->slots->size; ++i) {
        int value_index = (int)vector_get(cvalue->slots, i);
        Value *value = vector_get(values, value_index);

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
MethodValue* find_method(Vector *values, ClassValue* cvalue, int name) {
    for (int i = 0; i < cvalue->slots->size; ++i) {
        int value_index = (int)vector_get(cvalue->slots, i);
        Value *value = vector_get(values, value_index);

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
    // init heap and stack
    init_heap();

    // init global symbol tables
    global_func_name = ht_create(13);
    global_var_name = ht_create(13);

    // init entry func
    MethodValue* entry_func = vector_get(p->values, p->entry);
    push_frame(entry_func->code, 0, entry_func->nargs + entry_func->nlocals);

    // init global
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

                ht_put(global_var_name, name->value, (void *)(global_var_count++));

                break;
            }
            default:
                assert(0, "Invalid value type.\n");
        }
    }

    // init class info
    for (int i = 0; i < p->values->size; ++i) {
        Value* value = vector_get(p->values, i);
        if (value->tag == CLASS_VAL) {
            ClassValue *cval = (ClassValue *)value;

            ClassInfo *info = &class_table[class_table_count++];
            info->type = i + 3;
            info->varslot_count = 0;

            for (int j = 0; j < cval->slots->size; ++j) {
                int index = (int) vector_get(cval->slots, j);
                Value *v = vector_get(p->values, index);

                if (v->tag == SLOT_VAL) {
                    ++(info->varslot_count);
                }
            }
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
    free_heap();
}

static inline
void push(void* obj) {
    operand[operand_count++] = obj;
}

static inline
void* pop() {
    return operand[--operand_count];
}

static inline
void* peek() {
    return operand[operand_count - 1];
}

void interpret_bc(Program* p) {
    vm_init(p);
#ifdef DEBUG
    //printf("Interpreting Bytecode Program:\n");
    print_prog(p);
#endif
    while (sp->pc < sp->code->size) {
        ByteIns* ins = vector_get(sp->code, sp->pc);
#ifdef DEBUG
        //printf("Interpreting: ");
        print_ins(ins);
        //if (global_var[0]) printf(",  type: %lld, operand: %d", ((NullObj *)global_var[0])->type, operand_size);
        printf(", operand: %d\n", operand_count);
#endif
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
                //void* init_val = pop();
                
                // HACK
                IntObj* length = operand[operand_count-2];

                assert(length->type == Int, "Invalid length type for ARRAY.\n");
                ArrayObj* obj = make_array_obj(length->value);

                void* init_val = pop();
                pop();

                for (int i = 0; i < obj->length; ++i) {
                    obj->slots[i] = init_val;
                }

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

                int numslots = -1;
                for (int i = 0; i < class_table_count; ++i) {
                    if (class_table[i].type == class_type) {
                        numslots = class_table[i].varslot_count;
                        break;
                    }
                }
                assert(numslots != -1, "Class not found.\n");

                
                ObjectObj* obj = make_object_obj(class_type, NULL, numslots);

                void** args = malloc(sizeof(void *) * (numslots + 1));
                for (int i = 0; i < numslots + 1; i++) {
                    args[i] = pop();
                }

                obj->parent = args[numslots];

                for (int i = 0; i < numslots; i++) {
                    obj->varslots[i] = args[numslots - 1 - i];
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

                int idx = -1;

                for (ObjectObj* obj_for_search = obj; obj_for_search != NULL && idx == -1; obj_for_search = obj_for_search->parent) {
                    ClassValue* class = vector_get(p->values, obj_for_search->type - 3);
                    idx = find_slot_index(p->values, class, slot_ins->name);
                }

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

                int idx = -1;

                for (ObjectObj* obj_for_search = obj; obj_for_search != NULL && idx == -1; obj_for_search = obj_for_search->parent) {
                    ClassValue* class = vector_get(p->values, obj_for_search->type - 3);
                    idx = find_slot_index(p->values, class, set_slot_ins->name);
                }

                assert(idx >= 0, "Could not find the slot by name.\n");

                obj->varslots[idx] = value;
                push(value);

                break;
            }
            case CALL_SLOT_OP: {
                CallSlotIns* call_slot = (CallSlotIns*)ins;

                // HACK
                Obj* receiver = operand[operand_count - call_slot->arity];

                StringValue *name = vector_get(p->values, call_slot->name);
                assert(name->tag == STRING_VAL, "Invalid string type for CALL_SLOT_OP.\n");

                //printf("CALL_SLOT_OP receiver type: %lld\n", receiver->type);

                switch (receiver->type) {
                    case Int: {
                        IntObj* iobj = (IntObj*) receiver;

                        assert(call_slot->arity == 2, "Invalid parameter number for CALL_Slot_OP.\n");
                        void* args[2];
                        args[0] = pop();
                        args[1] = pop();

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
                        ArrayObj* aobj = (ArrayObj*)receiver;
                        assert(call_slot->arity <= 3, "Invalid parameter number for CALL_Slot_OP.\n");
                        
                        void* args[3];
                        
                        if (strcmp(name->value, "length") == 0) {
                            args[0] = pop();
                            push(array_length(aobj));
                        }
                        else if (strcmp(name->value, "set") == 0) {
                            args[0] = pop();
                            args[1] = pop();
                            args[2] = pop();
                            push(array_set(aobj, (IntObj*) args[1], args[0]));
                        }
                        else if (strcmp(name->value, "get") == 0) {
                            args[0] = pop();
                            args[1] = pop();
                            push(array_get(aobj, (IntObj*) args[0]));
                        }
                        else {
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
                        ObjectObj* oobj = (ObjectObj *)receiver;
                        MethodValue* method = NULL;

                        for (ObjectObj* obj_for_search = oobj; obj_for_search != NULL && method == NULL; obj_for_search = obj_for_search->parent) {
                            ClassValue* class = vector_get(p->values, obj_for_search->type - 3);
                            method = find_method(p->values, class, call_slot->name);
                        }

                        assert(method != NULL, "Could not find method for CALL_SLOT_OP.\n");
                        assert(method->tag == METHOD_VAL, "Invalid method type for CALL_SLOT_OP.\n");
                        assert(call_slot->arity <= method->nargs + method->nlocals + 1, "n <= num_slots + 1\n");

                        push_frame(method->code, -1, method->nargs + method->nlocals + 1);

                        void **args = malloc(sizeof(void *) * call_slot->arity);
                        for (int i = 0; i < call_slot->arity; i++) {
                            args[i] = pop();
                        }
                        for (int i = 0; i < call_slot->arity; i++) {
                            sp->slots[i] = args[call_slot->arity - 1 - i];
                        }
                        free(args);
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

                int var_idx = (int)ht_get(global_var_name, name->value);
                global_var[var_idx] = peek();

                //printf("SET_GLOBAL_OP index: %d, type: %lld\n", var_idx, ((NullObj *)global_var[var_idx])->type);

                break;
            }
            case GET_GLOBAL_OP: {
                GetGlobalIns *get_global_ins = (GetGlobalIns *)ins;
                StringValue *name = vector_get(p->values, get_global_ins->name);
                assert(name->tag == STRING_VAL, "Invalid object type for GET_GLOBAL_OP.\n");

                int var_idx = (int)ht_get(global_var_name, name->value);
                push(global_var[var_idx]);

                //printf("GET_GLOBAL_OP index: %d, type: %lld\n", var_idx, ((NullObj *)global_var[var_idx])->type);

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
                StringValue *name = vector_get(p->values, call_ins->name);
                assert(name->tag == STRING_VAL, "Invalid string type for CALL_OP.\n");

                MethodValue *method_val = ht_get(global_func_name, name->value);
                assert(method_val && method_val->tag == METHOD_VAL, "Invalid method type for CALL_OP.\n");

                push_frame(method_val->code, -1, method_val->nargs + method_val->nlocals);

                void** args = malloc(sizeof(void *) * (call_ins->arity));
                for (int i = 0; i < call_ins->arity; i++) {
                    args[i] = pop();
                }

                for (int i = 0; i < call_ins->arity; i++) {
                    sp->slots[i] = args[call_ins->arity - 1 - i];
                }

                free(args);
                break;
            }
            case RETURN_OP: {
                //assert(operand_count == 1, "Operand stack should contain only one element when return is called.\n");
                pop_frame();

                if (sp == NULL) {
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

