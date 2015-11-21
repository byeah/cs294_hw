#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "utils.h"
#include "bytecode.h"
#include "vm.h"

#ifdef _MSC_VER
#define inline __inline
#endif

#define STAT

static int lookup_count = 0;

#define JIT

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

typedef struct {
    int key;
    void* value;
} ht_item_t;

typedef struct {
    ht_item_t bucket[1001];
} ht_t;

static inline
void ht_init(ht_t *ht) {
    for (int i = 0; i < 1001; ++i) {
        ht->bucket[i].key = -1;
    }
}

static inline
int find_slot(ht_t *ht, int key) {
    int i = key % 1001;
    while (ht->bucket[i].key != -1 && ht->bucket[i].key != key) {
        i = (i + 1) % 1001;
    }
    return i;
}

static inline
void ht_put(ht_t *ht, int key, void *value) {
    int i = find_slot(ht, key);
    ht->bucket[i].key = key;
    ht->bucket[i].value = value;
}

static inline
void* ht_get(ht_t *ht, int key) {
    int i = find_slot(ht, key);
    if (ht->bucket[i].key == key)
        return ht->bucket[i].value;
    else
        return NULL;
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

typedef enum {
    Array = 2,
} ObjType;

typedef struct {
    int64_t type;
} Obj;

typedef int64_t TaggedVal;

typedef enum {
    Int = 0,
    Null,
    Object
} TaggedType;

static inline
TaggedVal tag_int(int64_t val) {
    return val << 3;
}

static inline
TaggedVal tag_null() {
    return 2;
}

static inline
TaggedVal tag_object(void *p) {
    return ((int64_t)p) + 1;
}

static inline
TaggedType untag_type(TaggedVal val) {
    if ((val & 7) == 0)
        return Int;
    else if (val == 2)
        return Null;
    else
        return Object;
}

static inline
int is_primitive(TaggedVal val) {
    if ((val & 7) == 0 || val == 2)
        return 1;
    else
        return 0;
}

static inline
int64_t untag_int(TaggedVal val) {
    assert(untag_type(val) == Int, "Not Int tag!");
    return val >> 3;
}

static inline
Obj* untag_object(TaggedVal val) {
    assert(untag_type(val) == Object, "Not Object tag!");
    return (Obj *)(val - 1);
}

typedef struct {
    int64_t type;
    void* forward;
} BrokenHeartObj;

typedef struct {
    int64_t type;
    int64_t length;
    TaggedVal slots[0];
} ArrayObj;

typedef struct {
    int64_t type;
    void* parent;
    TaggedVal varslots[0];
} ObjectObj;

struct {
    char* head;
    int64_t used;
    int64_t total;
} heap, freespace;

static inline
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
    char* IP;
    TaggedVal slots[0];
} Frame;

static unsigned char *stack[1024 * 1024];
Frame* sp = NULL;
static Frame* next_sp = (Frame *)stack;

static ht_t global_func_name;
static ht_t global_var_name;
static ht_t labels;

TaggedVal operand[1024] = { 0 };
//static int operand_count = 0;
TaggedVal* operand_top = operand;

static TaggedVal global_var[128] = { 0 };
static int global_var_count = 0;

typedef struct {
    int type;
    int varslot_count;
} ClassInfo;

static ClassInfo class_table[1024];
static int class_table_count = 0;

static inline
void free_heap() {
    free(heap.head);
    free(freespace.head);
}

static inline
TaggedVal get_post_gc_ptr(TaggedVal val) {
    // primitive type?
    if (is_primitive(val)) {
        return val;
    }

    Obj* obj = untag_object(val);

    if (obj == NULL) {
        return tag_object(obj);
    }

    // already copied?
    if (obj->type == -1) {
        return tag_object(((BrokenHeartObj *)obj)->forward);
    }

    // calculate size
    int64_t size = -1;
    if (obj->type == Array) {
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
        size = sizeof(ObjectObj) + numslots * sizeof(TaggedVal);
    }

    // copy
    Obj* ret = (Obj *)(freespace.head + freespace.used);
    memcpy(ret, obj, size);
    freespace.used += size;

    // broken heart
    ((BrokenHeartObj *)obj)->type = -1;
    ((BrokenHeartObj *)obj)->forward = ret;

    return tag_object(ret);
}

static inline
void scan_root_set() {
    // global variables
    for (int i = 0; i < global_var_count; ++i) {
        TaggedVal o = global_var[i];

        TaggedVal post_gc_o = get_post_gc_ptr(o);
#ifdef DEBUG
        fprintf(stderr, "Global %d: %llx -> %llx\n", i, o, post_gc_o);
#endif
        global_var[i] = post_gc_o;
    }
    // operand stack
    for (int i = 0; i < operand_top - operand; ++i) {
        TaggedVal o = operand[i];
        TaggedVal post_gc_o = get_post_gc_ptr(o);
#ifdef DEBUG
        fprintf(stderr, "Operand: %llx -> %llx\n", o, post_gc_o);
#endif
        operand[i] = post_gc_o;
    }
    // local frames
    for (Frame *p = sp, *np = next_sp; p != NULL; np = p, p = p->parent) {
        for (int i = 0; &(p->slots[i]) < (TaggedVal *)np; ++i) {
            TaggedVal o = p->slots[i];
            TaggedVal post_gc_o = get_post_gc_ptr(o);
#ifdef DEBUG
            fprintf(stderr, "Local: %llx -> %llx\n", o, post_gc_o);
#endif
            p->slots[i] = post_gc_o;
        }
    }
}

static inline
void garbage_collector() {
#ifdef DEBUG
    fprintf(stderr, "Heap: %llx - %llx.\n", heap.head, heap.head + heap.total);
    fprintf(stderr, "Free: %llx - %llx.\n", freespace.head, freespace.head + freespace.total);
    fprintf(stderr, "Scan root set.\n");
#endif
    // root set
    printf("GC\n");
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
                TaggedVal post_gc_o = get_post_gc_ptr(arr_p->slots[i]);
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
                TaggedVal post_gc_o = get_post_gc_ptr(obj_p->varslots[i]);
#ifdef DEBUG
                fprintf(stderr, "Object slot: %llx -> %llx\n", obj_p->varslots[i], post_gc_o);
#endif
                obj_p->varslots[i] = post_gc_o;
            }
            p += (2 + numslots);
        }
        else {
            printf("Unrecognized type: %" PRId64 ".\n", p->type);
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

static inline
void* halloc(int64_t nbytes) {
    if (heap.used + nbytes > heap.total) {
        garbage_collector();
    }

    if (heap.used + nbytes > heap.total) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    void *p = heap.head + heap.used;
    heap.used += nbytes;
    return p;
}

static inline
ArrayObj* make_array_obj(int64_t length) {
    ArrayObj* o = halloc(sizeof(ArrayObj) + length * sizeof(void *));

    o->type = Array;
    o->length = length;

    return o;
}

static inline
ObjectObj* make_object_obj(int64_t type, ObjectObj *parent, int num_slots) {
    ObjectObj* o = halloc(sizeof(ObjectObj) + num_slots * sizeof(TaggedVal));
    o->type = type;
    o->parent = parent;
    return o;
}

static inline
int64_t array_length(ArrayObj* a) {
    return a->length;
}

static inline
void array_set(ArrayObj* a, int64_t i, TaggedVal v) {
    a->slots[i] = v;
}

static inline
TaggedVal array_get(ArrayObj* a, int64_t i) {
    return a->slots[i];
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

char* instruction_pointer = NULL;

static inline
void push_frame(Vector* code, int pc, int num_slots) {
    next_sp->parent = sp;
    next_sp->code = code;
    next_sp->pc = pc;
    if (sp != NULL)
        sp->IP = instruction_pointer;
    sp = next_sp;
    next_sp = (Frame *)((unsigned char *)next_sp + sizeof(Frame) + num_slots * sizeof(void *));

    for (int i = 0; i < num_slots; ++i)
        sp->slots[i] = 0;
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
        int64_t value_index = (int64_t)vector_get(cvalue->slots, i);
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
int find_method_id(Vector *values, ClassValue* cvalue, int name) {
    for (int i = 0; i < cvalue->slots->size; ++i) {
        int64_t value_index = (int64_t)vector_get(cvalue->slots, i);
        Value *value = vector_get(values, value_index);

        if (value->tag == METHOD_VAL) {
            MethodValue *mval = (MethodValue *)value;
            if (mval->name == name)
                return value_index;
            //return mval;
        }
    }
    return -1;
}

//char* compile_assembly_code(Program *p,Vector* code,int slots);


static inline
void vm_init(Program* p) {
    // init heap and stack
    init_heap();

    // init global symbol tables
    ht_init(&global_func_name);
    ht_init(&global_var_name);
    ht_init(&labels);

    // init entry func
    MethodValue* entry_func = vector_get(p->values, p->entry);
    push_frame(entry_func->code, 0, entry_func->nargs + entry_func->nlocals);

    // init global
    for (int i = 0; i < p->slots->size; ++i) {
        int64_t index = (int64_t)vector_get(p->slots, i);
        Value* value = vector_get(p->values, index);
        switch (value->tag) {
            case METHOD_VAL: {
                MethodValue* method_val = (MethodValue *)value;
                ht_put(&global_func_name, method_val->name, index);
                //func_code[index] = compile_assembly_code(p,method_val->code,method_val->nargs);
                break;
            }
            case SLOT_VAL: {
                SlotValue* slot_val = (SlotValue *)value;
                ht_put(&global_var_name, slot_val->name, (void *)(global_var_count++));
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
                int64_t index = (int64_t)vector_get(cval->slots, j);
                Value *v = vector_get(p->values, index);

                if (v->tag == SLOT_VAL) {
                    ++(info->varslot_count);
                }
            }
        }
    }

    // preprocess labels
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
            ht_put(&labels, label_ins->name, make_label(method->code, j));
        }
    }
}

static inline
void vm_cleanup() {
    free_heap();
}

static inline
void push(TaggedVal val) {
    //printf("Pushing val type: %d\n", untag_type(val));
    *(operand_top++) = val;
    //operand[operand_count++] = val;
}

static inline
TaggedVal pop() {
    //printf("Poping val type: %d\n", untag_type(operand[operand_count-1]));
    return *(--operand_top);
    //return operand[--operand_count];
}

static inline
TaggedVal peek() {
    return *(operand_top - 1);
    //return operand[operand_count - 1];
}


char code_buffer[1024 * 1024];

extern char label_code[];
extern char label_code_end[];
extern char code_placeholder[];
extern char code_placeholder_end[];
extern char goto_code[];
extern char goto_code_end[];
extern char branch_code[];
extern char branch_code_end[];
extern char lit_code[];
extern char lit_code_end[];
extern char set_local_code[];
extern char set_local_code_end[];
extern char get_local_code[];
extern char get_local_code_end[];
extern char set_global_code[];
extern char set_global_code_end[];
extern char get_global_code[];
extern char get_global_code_end[];
extern char drop_code[];
extern char drop_code_end[];
extern char return_code[];
extern char return_code_end[];
extern char call_code[];
extern char call_code_end[];
extern char call_slot_code[];
extern char call_slot_code_end[];
extern char array_code[];
extern char array_code_end[];
extern char call_init_code[];
extern char call_init_code_end[];
extern char object_code[];
extern char object_code_end[];
extern char slot_code[];
extern char slot_code_end[];
extern char set_slot_code[];
extern char set_slot_code_end[];
ht_t label_table;
char** func_code;


inline int fillcode(char* code, char*start, char*end) {
    int l = end - start;
    memcpy(code, start, l);
    return l;
}

inline void fillhole(char* code, int l, int64_t old, int64_t new) {
    for (int j = 0; j < l - 3; j++) {
        int64_t *p = (int64_t *)(code + j);
        if (*p == old) {
            *p = new;
            return;
        }
    }
}

int holes[65536];
int holes_loc[65536];
int debugV[1024];
int64_t object_buffer[1024];

char* compile_assembly_code(Program *p, Vector* code, int slots) {
#ifndef JIT
    return;
#endif
    int n = 0;
    int t = 0;
    ht_init(&label_table);
    for (int i = 0; i < slots; i++) {
        int l = fillcode(code_buffer + n, call_init_code, call_init_code_end);
        fillhole(code_buffer + n, l, 0xcafebabecafebabe, (slots - i - 1) * 8);
        n += l;
    }
    for (int i = 0; i < code->size; i++) {
        ByteIns* ins = vector_get(code, i);
        /*if (ins->tag!=CALL_OP)
        {		int l = fillcode(code_buffer+n,code_placeholder,code_placeholder_end);
                fillhole(code_buffer+n,l,0xcafebabecafebabe,&instruction_pointer);
                fillhole(code_buffer+n,l,0xbabecafebabecafe,ins);
                n+=l;
                continue;
            }*/
            //printf("Compling: %d\n",ins->tag);
        switch (ins->tag) {
            case LABEL_OP: {
                LabelIns* label_ins = (LabelIns *)ins;
                ht_put(&label_table, label_ins->name, n);
                break;
            }
            case GOTO_OP: {
                int l = fillcode(code_buffer + n, goto_code, goto_code_end);
                holes[t] = i;
                holes_loc[t++] = n;
                n += l;
                break;
            }
            case BRANCH_OP: {
                int l = fillcode(code_buffer + n, branch_code, branch_code_end);
                holes[t] = i;
                holes_loc[t++] = n;
                n += l;
                break;
            }
            case LIT_OP: {
                int l = fillcode(code_buffer + n, lit_code, lit_code_end);
                LitIns* lit_ins = (LitIns *)ins;
                int index = lit_ins->idx;
                Value* val = vector_get(p->values, index);
                int64_t v = 0;
                if (val->tag == INT_VAL) {
                    v = tag_int(((IntValue *)val)->value);
                }
                else if (val->tag == NULL_VAL) {
                    v = tag_null();
                }
                else {
                    assert(0, "Invalid object type for LIT.\n");
                }
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, v);
                n += l;
                break;
            }
            case SET_LOCAL_OP: {
                SetLocalIns *set_local_ins = (SetLocalIns *)ins;
                int l = fillcode(code_buffer + n, set_local_code, set_local_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, set_local_ins->idx);
                //sp->slots[set_local_ins->idx] = peek();
                n += l;
                break;
            }
            case GET_LOCAL_OP: {
                GetLocalIns *get_local_ins = (GetLocalIns *)ins;
                int l = fillcode(code_buffer + n, get_local_code, get_local_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, get_local_ins->idx);
                //push(sp->slots[get_local_ins->idx]);
                n += l;
                break;
            }
            case SET_GLOBAL_OP: {
                SetGlobalIns *set_global_ins = (SetGlobalIns *)ins;
                int64_t var_idx = (int64_t)ht_get(&global_var_name, set_global_ins->name);
                int l = fillcode(code_buffer + n, set_global_code, set_global_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, global_var + var_idx);
                //global_var[var_idx] = peek();
                n += l;
                break;
            }
            case GET_GLOBAL_OP: {
                GetGlobalIns *get_global_ins = (GetGlobalIns *)ins;

                int64_t var_idx = (int64_t)ht_get(&global_var_name, get_global_ins->name);
                int l = fillcode(code_buffer + n, get_global_code, get_global_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, global_var + var_idx);
                //push(global_var[var_idx]);
                n += l;
                break;
            }
            case DROP_OP: {
                int l = fillcode(code_buffer + n, drop_code, drop_code_end);
                //pop();
                n += l;
                break;
            }
            case RETURN_OP: {
                int l = fillcode(code_buffer + n, return_code, return_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &next_sp);
                n += l;
                break;
            }
            case CALL_OP: {
                int l = fillcode(code_buffer + n, call_code, call_code_end);
                CallIns* call_ins = (CallIns *)ins;
                int method_id = ht_get(&global_func_name, call_ins->name);
                MethodValue *method_val = vector_get(p->values, method_id);
                int64_t offset = sizeof(Frame) + sizeof(void*) * (method_val->nargs + method_val->nlocals);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, func_code + method_id);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &instruction_pointer);
                fillhole(code_buffer + n, l, 0xbabecafebabecafe, ins);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &next_sp);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, offset);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, debugV);
                n += l;
                break;
            }
            case CALL_SLOT_OP: {
                int l = fillcode(code_buffer + n, call_slot_code, call_slot_code_end);
                CallSlotIns* call_slot = (CallSlotIns*)ins;
                StringValue *name = vector_get(p->values, call_slot->name);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, call_slot->arity);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, name->value);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &instruction_pointer);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &next_sp);
                fillhole(code_buffer + n, l, 0xbabecafebabecafe, ins);

                n += l;
                break;
            }
            case ARRAY_OP: {
                int l = fillcode(code_buffer + n, array_code, array_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &instruction_pointer);
                fillhole(code_buffer + n, l, 0xbabecafebabecafe, ins);
                n += l;
                break;

            }
            case OBJECT_OP: {
                int l = fillcode(code_buffer + n, object_code, object_code_end);
                ObjectIns* obj_ins = (ObjectIns*)ins;
                int64_t class_type = obj_ins->class + 3;
                int numslots = -1;
                for (int i = 0; i < class_table_count; ++i) {
                    if (class_table[i].type == class_type) {
                        numslots = class_table[i].varslot_count;
                        break;
                    }
                }
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, numslots);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &instruction_pointer);
                fillhole(code_buffer + n, l, 0xbabecafebabecafe, ins);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, class_type);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &object_buffer);
                n += l;
                break;

            }
            case SLOT_OP: {
                int l = fillcode(code_buffer + n, slot_code, slot_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &instruction_pointer);
                fillhole(code_buffer + n, l, 0xbabecafebabecafe, ins);
                n += l;
                break;
            }
            case SET_SLOT_OP: {
                int l = fillcode(code_buffer + n, set_slot_code, set_slot_code_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &instruction_pointer);
                fillhole(code_buffer + n, l, 0xbabecafebabecafe, ins);
                n += l;
                break;
            }
            default: {
                int l = fillcode(code_buffer + n, code_placeholder, code_placeholder_end);
                fillhole(code_buffer + n, l, 0xcafebabecafebabe, &instruction_pointer);
                fillhole(code_buffer + n, l, 0xbabecafebabecafe, ins);
                n += l;
                break;
            }
        }
    }
    assert(n < 1024 * 1024, "Code too big to fit into buffer!");
    char* res = (char*)malloc(n + 1);
    for (int i = 0; i < t; i++) {
        ByteIns* ins = vector_get(code, holes[i]);
        switch (ins->tag) {
            case GOTO_OP: {
                GotoIns* goto_ins = (GotoIns *)ins;
                int label = ht_get(&label_table, goto_ins->name);
                int l = goto_code_end - goto_code;
                //fillhole(code_buffer + holes_loc[i], l, 0xcafebabecafebabe, &instruction_pointer);
                //fillhole(code_buffer + holes_loc[i], l, 0xbabecafebabecafe, -1);
                fillhole(code_buffer + holes_loc[i], l, 0xcafebabecafebabe, res + label);
                //printf("Comping GOTO offset: %d\n",label);
                break;
            }
            case BRANCH_OP: {
                BranchIns* branch_ins = (BranchIns *)ins;
                int label = ht_get(&label_table, branch_ins->name);
                int l = branch_code_end - branch_code;
                //fillhole(code_buffer + holes_loc[i], l, 0xcafebabecafebabe, &instruction_pointer);
                //fillhole(code_buffer + holes_loc[i], l, 0xbabecafebabecafe, -1);
                fillhole(code_buffer + holes_loc[i], l, 0xcafebabecafebabe, res + label);
                //printf("Comping BRANCH offset: %d\n",label);
                break;
            }
            default:
                break;
        }

    }
    //printf("Total: %d\n",n);
    memcpy(res, code_buffer, n);
    return res;
}

void drive(Program* p) {
    int running = 1;
    while (running) {
        //printf("%d\n",instruction_pointer);
        void *res = call_feeny(instruction_pointer);
        //printf("Running: %d\n",((ByteIns*)res)->tag);
        switch ((int)res) {
            case 0: //PROGRAM_FINISHED:
                running = 0;
                break;
            case -1: //PERFORM_OP1 :
                //perform_op1();
#ifdef DEBUG
                printf("GOTO/BRANCH\n");
#endif
                break;
            case -2: {//0x6461: {
                printf("!!!\n");
                printf("%d\n", sp);
                printf("%d\n", sp->parent->IP);
                printf("%d\n", sp->IP);
                printf("%d\n", next_sp);
                printf("%d\n", debugV[0]);
                printf("%d\n", instruction_pointer);
                return;
            }

            default:
                /*if (((ByteIns*)res)->tag == 15)
                    running = 0;
                else*/
                //printf("%d\n",instruction_pointer);
                running = runSingleIns(res, p);
                break;
        }
    }
}

void direct_interpret_bc(Program* p);

void interpret_bc(Program* p) {
#ifdef JIT
    func_code = malloc((p->values->size + 1) * sizeof(char *));
    memset(func_code, 0, (p->values->size + 1) * sizeof(char *));
    vm_init(p);

    instruction_pointer = compile_assembly_code(p, sp->code, 0);

    drive(p);
#else
    direct_interpret_bc(p);
#endif
#ifdef STAT
    fprintf(stderr, "Lookup: %d.\n", lookup_count);
#endif
}

char* getAssemblyCode(Program *p, int method_id, int slots) {
    if (func_code[method_id] == 0) {
        MethodValue* method = (MethodValue*)vector_get(p->values, method_id);
        func_code[method_id] = compile_assembly_code(p, method->code, slots);
    }
    return func_code[method_id];
}

int runSingleIns(ByteIns* ins, Program* p) {
#ifdef DEBUG
    //printf("Interpreting: ");
    if (ins->tag != LABEL_OP && ins->tag != BRANCH_OP && ins->tag != GOTO_OP) {
        print_ins(ins);
        printf(", operand: %d\n", operand_top - operand);
        printf("Heap used: %d\n", heap.used);
        //printf("R%d %d\n",ins->tag,operand_top);//,*operand_top);
    }
#endif
    switch (ins->tag)
    {
        case LIT_OP: {
            LitIns* lit_ins = (LitIns *)ins;
            int index = lit_ins->idx;
            Value* val = vector_get(p->values, index);
            if (val->tag == INT_VAL) {
                push(tag_int(((IntValue *)val)->value));
            }
            else if (val->tag == NULL_VAL) {
                push(tag_null());
            }
            else {
                assert(0, "Invalid object type for LIT.\n");
            }
            break;
        }
        case ARRAY_OP: {
            //void* init_val = pop();

            // HACK
            TaggedVal length = *(operand_top - 2);//operand[operand_count-2];

            assert(untag_type(length) == Int, "Invalid length type for ARRAY.\n");

            ArrayObj* obj = make_array_obj(untag_int(length));

            TaggedVal init_val = pop();
            pop();

            for (int i = 0; i < obj->length; ++i) {
                obj->slots[i] = init_val;
            }

            push(tag_object(obj));
            break;
        }
        case PRINTF_OP: {
            PrintfIns* printf_ins = (PrintfIns *)ins;
            // HACK
#ifdef _MSC_VER
            int64_t res[1024];
#else
            int64_t res[printf_ins->arity];
#endif
            for (int i = 0; i < printf_ins->arity; i++) {
                TaggedVal val = pop();
                assert(untag_type(val) == Int, "Invalid object type for PRINTF.\n");
                res[i] = untag_int(val);
            }

            int cur = printf_ins->arity;

            StringValue *format = vector_get(p->values, printf_ins->format);
            assert(format->tag == STRING_VAL, "Invalid object type for PRINTF_OP.\n");
            for (char* p = format->value; *p && cur >= 0; p++) {
                if (*p != '~')
                    printf("%c", *p);
                else
                    printf("%" PRId64, res[--cur]);
            }
            push(tag_null());

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

#ifdef _MSC_VER
            TaggedVal args[1024];
#else
            TaggedVal args[numslots + 1];
#endif

            for (int i = 0; i < numslots + 1; i++) {
                args[i] = pop();
            }

            assert(untag_type(args[numslots]) == Object || untag_type(args[numslots]) == Null, "Invalid parent type for OBJECT_OP.\n");
            if (untag_type(args[numslots]) == Null)
                obj->parent = NULL;
            else
                obj->parent = untag_object(args[numslots]);

            for (int i = 0; i < numslots; i++) {
                obj->varslots[i] = args[numslots - 1 - i];
            }

            push(tag_object(obj));

            break;
        }
        case SLOT_OP: {
#ifdef STAT
            ++lookup_count;
#endif
            SlotIns* slot_ins = (SlotIns*)ins;

            TaggedVal val = pop();
            assert(untag_type(val) == Object, "Invalid val type for SLOT_OP.\n");

            ObjectObj *obj = (ObjectObj *)untag_object(val);
            assert(obj->type > 2, "Get slot should be with an object!");

            int idx = -1;
            ObjectObj* obj_for_search = NULL;

            for (obj_for_search = obj; obj_for_search != NULL; obj_for_search = obj_for_search->parent) {
                ClassValue* class = vector_get(p->values, obj_for_search->type - 3);
                idx = find_slot_index(p->values, class, slot_ins->name);
                if (idx>=0)
                	break;
            }

            assert(idx >= 0, "Could not find the slot by name.\n");
			push(obj_for_search->varslots[idx]);
			int64_t* type = ((int64_t *)instruction_pointer) - 1;
            int64_t* slot_addr = type - 1;
            *type = val;
            *slot_addr = obj_for_search->varslots+idx;
            break;
        }
        case SET_SLOT_OP: {
#ifdef STAT
            ++lookup_count;
#endif
            SetSlotIns* set_slot_ins = (SetSlotIns*)ins;
            TaggedVal value = pop();
            TaggedVal tagged_obj = pop();

            ObjectObj *obj = (ObjectObj *)untag_object(tagged_obj);
            assert(obj->type > 2, "Get slot should be with an object!");

            int idx = -1;
            ObjectObj* obj_for_search = NULL;

            for (obj_for_search = obj; obj_for_search != NULL; obj_for_search = obj_for_search->parent) {
                ClassValue* class = vector_get(p->values, obj_for_search->type - 3);
                idx = find_slot_index(p->values, class, set_slot_ins->name);
                if (idx>=0)
            	break;
		    }
            assert(idx >= 0, "Could not find the slot by name.\n");

            obj_for_search->varslots[idx] = value;
            push(value);
            int64_t* type = ((int64_t *)instruction_pointer) - 1;
            int64_t* slot_addr = type - 1;
            *type = tagged_obj;
            *slot_addr = obj_for_search->varslots+idx;
            break;
        }
        case CALL_SLOT_OP: {
            CallSlotIns* call_slot = (CallSlotIns*)ins;

            // HACK
            TaggedVal receiver = *(operand_top - call_slot->arity);//operand[operand_count - call_slot->arity];

            StringValue *name = vector_get(p->values, call_slot->name);
            assert(name->tag == STRING_VAL, "Invalid string type for CALL_SLOT_OP.\n");
            switch (untag_type(receiver)) {
                case Int: {

                    assert(call_slot->arity == 2, "Invalid parameter number for CALL_Slot_OP.\n");

                    TaggedVal i, j;
                    j = pop();
                    i = pop();

                    assert(untag_type(j) == Int, "Error: Operand to Int must be Int.\n");

                    uint16_t *op = name->value;
                    switch (*op) {
                        case 0x6461: // ad
                            push(i + j);
                            break;
                        case 0x7573: // su
                            push(i - j);
                            break;
                        case 0x756d: // mu
                            push(i * (j >> 3));
                            break;
                        case 0x6964: // di
                            push((i / j) << 3);
                            break;
                        case 0x6f6d: // mo
                            push((i % j));
                            break;
                        case 0x746c: // lt
                            push((i < j) ? tag_int(0) : tag_null());
                            break;
                        case 0x7467: // gt
                            push((i > j) ? tag_int(0) : tag_null());
                            break;
                        case 0x656c: // le
                            push((i <= j) ? tag_int(0) : tag_null());
                            break;
                        case 0x6567: // ge
                            push((i >= j) ? tag_int(0) : tag_null());
                            break;
                        case 0x7165: // eq
                            push((i == j) ? tag_int(0) : tag_null());
                            break;
                        default:
                            printf("Error: Operator %s not recognized.\n", name->value);
                            break;
                    }

                    break;
                }

                case Null: {
                    printf("Error: Calling %s from a NULL reference.\n", name->value);
                    exit(-1);
                }

                case Object: {
                    Obj* r_obj = untag_object(receiver);
                    if (r_obj->type == Array) {
                        ArrayObj* aobj = (ArrayObj*)r_obj;
                        assert(call_slot->arity <= 3, "Invalid parameter number for CALL_Slot_OP.\n");

                        TaggedVal args[3];

                        switch (name->value[0]) {
                            case 'l': // length
                                args[0] = pop();
                                push(tag_int(array_length(aobj)));
                                break;
                            case 's': // set
                                args[0] = pop();
                                args[1] = pop();
                                args[2] = pop();
                                array_set(aobj, untag_int(args[1]), args[0]);
                                push(tag_null());
                                break;
                            case 'g': // get
                                args[0] = pop();
                                args[1] = pop();
                                push(array_get(aobj, untag_int(args[0])));
                                break;
                            default:
                                printf("Error: Operator %s not recognized.\n", name->value);
                                exit(-1);
                        }

                        break;
                    }
                    else {
#ifdef STAT
                        ++lookup_count;
#endif
                        ObjectObj* oobj = (ObjectObj *)r_obj;
                        MethodValue* method = NULL;
                        int method_id;

                        for (ObjectObj* obj_for_search = oobj; obj_for_search != NULL && method == NULL; obj_for_search = obj_for_search->parent) {
                            ClassValue* class = vector_get(p->values, obj_for_search->type - 3);
                            method_id = find_method_id(p->values, class, call_slot->name);
                            if (method_id >= 0) {
                                method = vector_get(p->values, method_id);
                            }
                        }

                        assert(method != NULL, "Could not find method for CALL_SLOT_OP.\n");
                        assert(method->tag == METHOD_VAL, "Invalid method type for CALL_SLOT_OP.\n");
                        assert(call_slot->arity <= method->nargs + method->nlocals + 1, "n <= num_slots + 1\n");
						assert(call_slot->arity == method->nargs  + 1, "n <= num_slots + 1\n");

                        push_frame(method->code, -1, method->nargs + method->nlocals + 1);
                        char *code = getAssemblyCode(p, method_id, call_slot->arity);
                        int64_t offset = sizeof(Frame) + sizeof(void*) * ( method->nargs + method->nlocals + 1);
                        int64_t* type = ((int64_t *)instruction_pointer) - 1;
                        int64_t* method_addr = type - 1;
						int64_t* offset_addr = method_addr - 1;

                        *type = oobj->type;
                        *method_addr = code;
						*offset_addr = offset;
                        instruction_pointer = code;
                        //HACK
#ifdef _MSC_VER
                        TaggedVal args[1024];
#else
                        TaggedVal args[call_slot->arity];
#endif

                        /*for (int i = 0; i < call_slot->arity; i++) {
                            args[i] = pop();
                        }

                        for (int i = 0; i < call_slot->arity; i++) {
                            sp->slots[i] = args[call_slot->arity - 1 - i];
                        }*/
                        break;
                    }
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

            int64_t var_idx = (int64_t)ht_get(&global_var_name, set_global_ins->name);
            global_var[var_idx] = peek();

            break;
        }
        case GET_GLOBAL_OP: {
            GetGlobalIns *get_global_ins = (GetGlobalIns *)ins;

            int64_t var_idx = (int64_t)ht_get(&global_var_name, get_global_ins->name);
            push(global_var[var_idx]);

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
            TaggedVal predicate = pop();

            if (untag_type(predicate) != Null) {
                BranchIns* branch_ins = (BranchIns *)ins;

                LabelAddr* label = ht_get(&labels, branch_ins->name);

                assert(label != NULL, "Label not found in BRANCH_OP.\n");
                assert(label->code == sp->code, "Cross method jump is not allowed for BRANCH_OP.\n");

                sp->pc = label->pc;
            }
            break;
        }
        case GOTO_OP: {
            GotoIns* goto_ins = (GotoIns *)ins;

            LabelAddr* label = ht_get(&labels, goto_ins->name);

            assert(label != NULL, "Label not found in GOTO_OP.\n");
            assert(label->code == sp->code, "Cross method jump is not allowed for GOTO_OP.\n");

            sp->pc = label->pc;
            break;
        }
        case CALL_OP: {
            CallIns* call_ins = (CallIns *)ins;
#ifdef DEBUG
            printf("Call %d\n", call_ins->name);
#endif                
            int method_id = ht_get(&global_func_name, call_ins->name);
            assert(method_id >= 0, "Didn't find the method");
            MethodValue *method_val = vector_get(p->values, method_id);
            assert(method_val && method_val->tag == METHOD_VAL, "Invalid method type for CALL_OP.\n");
            push_frame(method_val->code, -1, method_val->nargs + method_val->nlocals);
            instruction_pointer = getAssemblyCode(p, method_id, call_ins->arity);

            // HACK
#ifdef _MSC_VER
            TaggedVal args[1024];
#else
            TaggedVal args[call_ins->arity];
#endif

            /*for (int i = 0; i < call_ins->arity; i++) {
                args[i] = pop();
            }

            for (int i = 0; i < call_ins->arity; i++) {
                sp->slots[i] = args[call_ins->arity - 1 - i];
            }*/

            break;
        }
        case RETURN_OP: {
            //assert(operand_count == 1, "Operand stack should contain only one element when return is called.\n");
            pop_frame();

            if (sp == NULL) {
                return 0;
            }
            instruction_pointer = sp->IP;

            break;
        }
        default: {
            assert(0, "Invalid instruction.\n");
            break;
        }
    }
#ifdef DEBUG
    //	if (ins->tag!=LABEL_OP && ins->tag!=BRANCH_OP && ins->tag!=GOTO_OP)
    //    	printf("R%d %d\n",ins->tag,operand_top);//,*operand_top);
#endif
    return 1;
}

void direct_interpret_bc(Program* p) {
    vm_init(p);
#ifdef DEBUG
    //printf("Interpreting Bytecode Program:\n");
    //print_prog(p);
#endif
    while (sp->pc < sp->code->size) {
        ByteIns* ins = vector_get(sp->code, sp->pc);
        if (runSingleIns(ins, p) == 0)
            break;
        ++sp->pc;
    }
    printf("\n");
}




