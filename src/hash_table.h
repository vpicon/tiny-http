#ifndef _HTTP_HASH_TABLE_H
#define _HTTP_HASH_TABLE_H

#include <stddef.h>

/**
 * Nodes of linked list in hash table.
 */
typedef struct node {
    struct node *next;
    struct node *prev;
    char *key;
    char *value;
} node_t;

/**
 * Hash table implemented using chaining.
 * I wanted to try this instead of open addressing.
 */
typedef struct {
    int m;           // table size
    int n;           // number of elements stored in the table
    node_t **table;  // hash table of size m, of lists. 
} hasht_t;


void init_hash(void);

node_t *new_hash_node(const char *key, const char *value);
void    free_hash_node(node_t *n);

hasht_t *new_hash_table(void);
void     free_hash_table(hasht_t *ht);

int   hash_contains(hasht_t *ht, const char *key);
char *hash_search(hasht_t *ht, const char *key);
void  hash_insert(hasht_t *ht, const char *key, const char *element);
void  hash_remove(hasht_t *ht, const char *key);

void  hash_print(hasht_t *ht);
// char *hash_to_string(hasht_t *ht);


#endif  // _HTTP_HASH_TABLE_H
