//
// Created by sps5394 on 10/18/18.
//
#include "cache.h"

int global_cache_count = 0;

struct node *cache_get(char *s) {
  struct node *iterator = head, *temporary;
  while (iterator != NULL && iterator->name !=NULL && s!= NULL) {
    if(strlen(iterator->name) == strlen(s))
    if (strcmp(iterator->name, s) == 0) {
      //found in cache, move node to head of list
      // and return value of key

      if (iterator != head) {
        temporary = iterator->prev;
        if (iterator == tail) {
          temporary->next = NULL;
          tail = temporary;
        } else {
          temporary->next = iterator->next;
          iterator->next->prev = temporary;
        }

        head->prev = iterator;
        iterator->next = head;
        iterator->prev = NULL;
        head = iterator;
      }
      return head;
    }
    iterator = iterator->next;
  }
  // Key not found in cache
  return NULL;
}

void cache_put(char *name, char *defn) {
  struct node *cache_entry, *new_node;
  if (( cache_entry = cache_get(name)) == NULL) {
    // value not in cache
    new_node = (struct node *) malloc(sizeof(struct node));
    new_node->name = strdups(name);
    new_node->defn = strdups(defn);
    new_node->next = new_node->prev = NULL;
    if (head == NULL) {
      head = tail = new_node;
    } else {
      head->prev = new_node;
      new_node->next = head;
      new_node->prev = NULL;
      head = new_node;
    }
    if (global_cache_count >= CACHE_SIZE) {
      // cache is full, evict the last node
      // insert a new node in the list
      // This will not work for a cache size 1
      tail = tail->prev;
      free(tail->next);
      tail->next = NULL;
    } else {
      // Increase the count
      global_cache_count++;
    }
  } else {
    // update cache entry
    cache_entry->defn = strdups(defn);
  }
}

void cache_invalidate(char *key) {
  struct node *iter;
  if (cache_get(key) != NULL) { // if in cache, delete it
    iter = head;
    if (head->next != NULL) {
      head->next->prev = NULL;
      head = head->next;
    } else {
      head = NULL;
    }
    free(iter);
    global_cache_count--;
  }
}

char *strdups(char *s) {/* make a duplicate of s */
if(s != NULL){
  char *p;
  p = (char *) malloc(strlen(s) + 1); /* +1 for ’\0’ */
  if (p != NULL)
    strcpy(p, s);
  return p;
}
return NULL;
}
