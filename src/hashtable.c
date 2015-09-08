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
	size_t hash, i;
	size_t length = strlen(key);

	for (hash = i = 0; i < length; ++i) {
		hash += key[i], hash += (hash << 10), hash ^= (hash >> 6);
	}
	hash += (hash << 3), hash ^= (hash >> 11), hash += (hash << 15);

	return hash % hashtable->size;
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
		//free(next->value);
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

void* ht_get(Hashtable *hashtable, char *key) {
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
		return NULL;
	}
	else {
		return current->value;
	}
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
		free(current->key);
		free(current);
	}
}

void ht_clear(Hashtable *hashtable) {
	for (int i = 0; i < hashtable->size; ++i) {
		entry_t* current = hashtable->table[i];
		while (current != NULL) {
			entry_t* temp = current->next;
			free(current->key);
			free(current);
			current = temp;
		}
		hashtable->table[i] = NULL;
	}
}

Hashtable* ht_copy(Hashtable *hashtable) {
	Hashtable* copy = NULL;

	if ((copy = ht_create(hashtable->size) == NULL)) {
		return NULL;
	}

	for (int i = 0; i < hashtable->size; ++i) {
		entry_t* source = hashtable->table[i];
		entry_t* dest = NULL;
		while (source != NULL) {
			entry_t* newpair = ht_newpair(source->key, source->value);
			if (dest == NULL) {
				copy->table[i] = newpair;
			} 
			else {
				dest->next = newpair;
			}
			dest = newpair;
			source = source->next;
		}
	}

	return copy;
}