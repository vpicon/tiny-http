#include "hash_table.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MIN_TABLE_SIZE 16

/* Hash function constants */
static int64_t p;     // prime > 2^32
static int64_t a, b;  // random numbers between {0, 1, ..., p - 1}


/**
 * Simple a mod b operation, with range [0, 1, ..., b - 1]
 */
static int64_t mod(int64_t x, int64_t y) {
    while (x < 0)
        x += y;
    return x % y;
}

/**
 * Initializes the library. Should be called only once.
 */
void init_hash() {
    srandom(time(NULL));
    p = 2305843009213693951; // 2^61 - 1, prime
    a = random();
    b = random();
}

/**
 * Prehash function for strings, by Dan Bernstein.
 */
static int64_t prehash(const char *str) {
    int64_t hash = 5381;
    int c;

    while ((c = *str++) != 0)
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c

    return hash;
}

/**
 * Universal hash function
 */
static int hash(const char *str, int m) {
    int64_t k = prehash(str);
    return mod(mod(a * k + b, p), m);
}

/**
 * Returns a newly allocated hash table node. It must be freed by calling
 * free_hash_table() below.
 */
node_t *new_hash_node(const char *key, const char *value) {
    node_t *n = (node_t *) malloc(sizeof(node_t));
    n->next = n->prev = NULL;

    char *keycpy = strdup(key);
    if (key == NULL) {
        // TODO: treat it as a deamon errors should be committed
        // to syslog, and not exit with error.
        perror("strndup");
        exit(1);
    }
    n->key = keycpy;

    char *valcpy = strdup(value);
    if (key == NULL) {
        // TODO: treat it as a deamon errors should be committed
        // to syslog, and not exit with error.
        perror("strndup");
        exit(1);
    }
    n->value = valcpy;

    return n;
}

/**
 * Frees the given node resources.
 */
void free_hash_node(node_t *n) {
    if (n != NULL) {
        free(n->key);
        free(n->value);
        free(n);
    }
}

/**
 * Returns a newly allocated hash table. It must be freed by calling
 * free_hash_table() below.
 */
hasht_t *new_hash_table() {
    hasht_t *t = (hasht_t *) malloc(sizeof(hasht_t));
    t->m = t->n = 0;
    t->table = NULL;

    return t;
}

/**
 * Frees the list of nodes given by the list head node.
 */
static void free_list_nodes(node_t *head) {
    node_t *n = head;
    while (n != NULL) {
        node_t *next = n->next;
        free_hash_node(n);
        n = next;
    }
}

/**
 * Frees a hash table and all its contents.
 */
void free_hash_table(hasht_t *ht) {
    for (int l = 0; l < ht->m; l++)
        free_list_nodes(ht->table[l]);

    free(ht->table);
    free(ht);
}

/**
 * Given hash table and an item, returns a pointer to the node of
 * the list in the hash table containin such key. Returns the NULL
 * poiner if no such element exists.
 */
static node_t *hash_search_node(hasht_t *ht, const char *key) {
    // Empty hash table case
    if (ht->m == 0)
        return NULL;

    // Get the list at hash(key) slot
    node_t *head = ht->table[hash(key, ht->m)];

    // seek for key in such list
    node_t *n = head;
    while (n != NULL) {
        if (strcmp(n->key, key) == 0)  // Found key
            return n;
        // keep searching in the list
        n = n->next;
    }

    // key not found
    return NULL;
}

/**
 * Given hash table and a key, returns 1 if there is an element in
 * the table with such key. Returns 0 otherwise.
 */
int hash_contains(hasht_t *ht, const char *key) {
    node_t *n = hash_search_node(ht, key);
    return (n == NULL) ? 0 : 1;
}

/**
 * Given hash table and an item, returns NULL if no element exists
 * in the hash table with such key, or a newly allocated copy of the
 * element in the table with such key. The returned ellement must
 * be freed(2).
 */
char *hash_search(hasht_t *ht, const char *key) {
    node_t *n = hash_search_node(ht, key);

    char *s = NULL;  // string to return
    if (n != NULL) { // key found
        s = strdup(n->value);
        if (!s) {
            perror("strdup");
            exit(1);  // TODO
        }
    }

    return s;
}

/**
 * Resizes the hash table given to the new value of m.
 * It does so by allocating a new table and freeing the prior one,
 * and rehashing all the prior elements.
 * It has O(ht->n) running time.
 */
static void resize_hash_table(hasht_t *ht, int newm) {
    node_t **t = (node_t **) malloc(newm * sizeof(node_t *));

    // Rehash elements to the new table
    for (int l = 0; l < ht->m; l++) {
        node_t *n = ht->table[l];  // get list in current slot
        // Check all elements in list
        while (n != NULL) {
            // Save next node in list
            node_t *next = n->next;

            // Add element to the head list in the new hashed table
            int slot = hash(n->key, newm);  // get slot in new table
            n->next = n->prev = NULL;
            if (t[slot] != NULL) { // there is a list in the slot
                t[slot]->prev = n;
                n->next = t[slot];
            }
            t[slot] = n;

            // Keep searching the list
            n = next;
        }
    }

    // Link new table with hasht struct and free old table
    free(ht->table);
    ht->table = t;
    ht->m = newm;
}

/**
 * Doubles table size and rehashed all elements.
 */
static void grow_hash_table(hasht_t *ht) {
    // Check empty hash table case
    if (ht->m == 0) {
        // Start with table with MINSIZE
        ht->m = MIN_TABLE_SIZE;
        ht->table = (node_t **) malloc(ht->m * sizeof(node_t *));
        return;
    }

    // Double table
    int newm = 2 * ht->m;  // double table size
    resize_hash_table(ht, newm);
}

/**
 * Given an element, stores the key with a new copy of the
 * given element, if the key was not previously in the table.
 * Otherwise, overrides the element stored in the table pointed
 * with the key.
 *
 * Table doubling is implemented. Hence it may take O(n) time to
 * perform some insertions. However O(1) amortized time is guaranteed.
 */
void hash_insert(hasht_t *ht, const char *key, const char *value) {
    // Check if element with key is already in table and override
    node_t *found = hash_search_node(ht, key);
    if (found != NULL) {
        // Copy given element and override it in table
        char *s = strdup(value);
        if (!s) {
            perror("strdup");
            exit(1);
        }
        // Free previously stored element and override it
        free(found->value);
        found->value = s;
        return;
    }

    // New insertion. Check first to grow hash table.
    if (ht->n == ht->m)
        grow_hash_table(ht);
    ht->n++;  // Increment number of elements in table

    // Create a new node for the pair (key element)
    node_t *n = new_hash_node(key, value);

    // Insert element in head of list in slot hash(key)
    int slot = hash(key, ht->m);
    node_t *head = ht->table[slot];
    if (head != NULL) {
        n->next = head;
        head->prev = n;
    }
    ht->table[slot] = n;
}

/**
 * Shrinks table size and rehashes elements.
 */
static void shrink_hash_table(hasht_t *ht) {
    // Check for small tables
    if (ht->m < MIN_TABLE_SIZE * 2) {
        // keep it at least MIN_TABLE_SIZE entries
        return;
    }

    // Half table
    int newm = ht->m/2;  // double table size
    resize_hash_table(ht, newm);
}

/**
 * Given a hash table and a key, removes the element in the table pointed
 * by the given key (if such element exists). Otherwise does nothing.
 */
void hash_remove(hasht_t *ht, const char *key) {
    // Check if element with key is in table
    node_t *n = hash_search_node(ht, key);
    if (n == NULL)
        return;

    // Check first to shrink hash table.
    if (ht->n/4 <= ht->m) {
        shrink_hash_table(ht);
        n = hash_search_node(ht, key);
    }
    ht->n--;  // Decrease number of elements


    // Link previous elemenet with next element of deletion node
    if (n->next != NULL)
        (n->next)->prev = n->prev;
    if (n->prev != NULL)
        (n->prev)->next = n->next;
    else {  // n->prev is NULL, hence n is head of list
        ht->table[hash(key, ht->m)] = n->next;
    }

    // Deallocate resources for the node
    free_hash_node(n);
}

void hash_print(hasht_t *ht) {
    printf("{");
    for (int l = 0; l < (int) ht->m; l++) { // print each list in the table
        node_t *n = ht->table[l];
        while (n != NULL) {
            printf("\"%s\":\"%s\", ", n->key, n->value);
            n = n->next;
        }
    }
    printf("}\n");
}

/**
 * Returns a newly allcoated nullterminated string with format
 * "{k1:v1, k2:v2, ...}". The returned string must be freed(2).
 *
 * Returns null pointer if the table contains too many elements
 * to be held in the format string.
 */
/*
char *hash_to_string(hasht_t *ht) {
    return NULL;
}
*/
