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
void ht_clear(Hashtable *hashtable);


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

void ht_clear(Hashtable *hashtable) {
    for (int i = 0; i < hashtable->size; ++i) {
        entry_t* current = hashtable->table[i];
        while (current != NULL) {
            entry_t* temp = current->next;
            //free(current->key);
            free(current);
            current = temp;
        }
        hashtable->table[i] = NULL;
    }
}

// vm.c

struct frame_t {
    Vector* slots;
    ByteIns* return_addr;
    struct frame_t* parent;
};
typedef struct frame_t Frame;

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

static Hashtable* global_vars = NULL;
static Vector* code = NULL;
static int pc = 0;
static Vector* operand = NULL;
static Frame* local_frame = NULL;

void vm_init(Program* p) {
    global_vars = ht_create(13);
    operand = make_vector();

    // init entry
    MethodValue* entry_func = vector_get(p->values, p->entry);
    local_frame = make_frame(NULL, NULL, entry_func->nargs + entry_func->nlocals);
    code = entry_func->code;
    pc = 0;
}



void interpret_bc(Program* p) {
    vm_init(p);
    //printf("Interpreting Bytecode Program:\n");
    //print_prog(p);
    while (pc < code->size) {
        ByteIns* ins = vector_get(code, pc);
        switch (ins->tag)
        {
        default:
            break;
        }
    }
    
    printf("\n");
}

