#include <stdlib.h>
#include <string.h>
#include "hashtable.h"


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

int ht_hash(Hashtable* hashtable, char* key) {
	unsigned long hashval = 0;
	size_t i = 0;

	while (hashval < ULONG_MAX && i < strlen(key)) {
		hashval = hashval << 8;
		hashval += key[i];
		++i;
	}

	return hashval % hashtable->size;
}

entry_t* ht_newpair(char *key, void *value) {
	entry_t* newpair;
	if ((newpair = malloc(sizeof(entry_t))) == NULL) {
		return NULL;
	}
	if ((newpair->key = strdup(key)) == NULL) {
		return NULL;
	}
	newpair->value = value;
	newpair->next = NULL;
	return newpair;
}

void ht_set(Hashtable* hashtable, char* key, void* value) {
	entry_t* newpair = NULL;
	entry_t* next = NULL;
	entry_t* last = NULL;

	int bin = ht_hash(hashtable, key);
	next = hashtable->table[bin];
	while (next != NULL && next->key != NULL && strcmp(key, next->key) > 0) {
		last = next;
		next = next->next;
	}
	/* There's already a pair.  Let's replace that string. */
	if (next != NULL && next->key != NULL && strcmp(key, next->key) == 0) {
		//free(next->value);
		next->value = value;
	}
	/* Nope, could't find it.  Time to grow a pair. */
	else {
		newpair = ht_newpair(key, value);

		/* We're at the start of the linked list in this bin. */
		if (next == hashtable->table[bin]) {
			newpair->next = next;
			hashtable->table[bin] = newpair;
		}
		/* We're at the end of the linked list in this bin. */
		else if (next == NULL) {
			last->next = newpair;
		}
		/* We're in the middle of the list. */
		else {
			newpair->next = next;
			last->next = newpair;
		}
	}
}

void* ht_get(Hashtable *hashtable, char *key) {
	int bin = 0;
	entry_t* pair;

	bin = ht_hash(hashtable, key);

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[bin];
	while (pair != NULL && pair->key != NULL && strcmp(key, pair->key) > 0) {
		pair = pair->next;
	}

	/* Did we actually find anything? */
	if (pair == NULL || pair->key == NULL || strcmp(key, pair->key) != 0) {
		return NULL;
	}
	else {
		return pair->value;
	}
}