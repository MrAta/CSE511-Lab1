//
// Created by sps5394 on 10/18/18.
//
#ifndef P1_CSRF_CACHE_H
#define P1_CSRF_CACHE_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define CACHE_SIZE 101

/********** GLOBAL DATA ***********/
int global_cache_count = 0;

struct node {
  char *name;
  char *defn;
  struct node *next, *prev;
} *head,*tail,*curr,*temp_node;

/************ INTERFACE *********/

/**
 * Duplicates the incoming string and returns
 * a char * to the newly allocated string
 */
char *strdups(char *s);

/**
 * Looks up the key value in the cache and returns the associated node
 * @param s key to lookup in the cache
 * @return node * to the node containing the data or NULL if cache miss
 */
struct node *cache_get(char *s);

/**
 * Adds the provided key and value to the cache. In case the cache is full,
 * it will evict the least recently used entry (LRU)
 * @param name The key to enter
 * @param defn The value for the provided key
 */
void cache_put (char *name, char *defn);

#endif //P1_CSRF_CACHE_H
