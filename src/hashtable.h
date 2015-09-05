#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

struct entry_s {
	char* key;
	void* value;
	struct entry_s* next;
};

typedef struct entry_s entry_t;

typedef struct {
	int size;
	struct entry_s** table;
} Hashtable;

Hashtable* ht_create(int size);

void ht_put(Hashtable* hashtable, char* key, void* value);
void* ht_get(Hashtable *hashtable, char *key);
void ht_remove(Hashtable *hashtable, char *key);
void ht_clear(Hashtable *hashtable);

#endif
