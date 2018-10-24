//
// Created by sps5394 on 10/18/18.
//
#include "cache.h"
int global_cache_count = 0;
struct node* cache_get(char *s) {
  curr = head;
  while (curr != NULL) {
    if (strcmp(curr->name, s) == 0) {
      //found in cache, move node to head of list
      // and return value of key

      if(curr != head) {
        temp_node = curr->prev;
        if (curr == tail) {
          temp_node->next = NULL;
          tail = temp_node;
        } else {
          temp_node->next = curr->next;
          curr->next->prev = temp_node;
        }

        head->prev = curr;
        curr->next = head;
        curr->prev = NULL;
        head = curr;
      }
      return head;
    }
    curr = curr->next;
  }
  // Key not found in cache
  return NULL;
}

void cache_put (char *name, char *defn) {
  struct node *cache_entry;
  if ((cache_entry = cache_get(name)) == NULL) {
    // value not in cache
    temp_node = (struct node*) malloc (sizeof(struct node));
    temp_node->name = strdups(name);
    temp_node->defn = strdups(defn);
    temp_node->next = temp_node->prev = NULL;
    if(head == NULL) {
      head = tail = temp_node;
    } else {
      head->prev = temp_node;
      temp_node->next = head;
      temp_node->prev = NULL;
      head = temp_node;
    }
    if (global_cache_count >= CACHE_SIZE) {
      // cache is full, evict the last node
      // insert a new node in the list
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
  struct node *temp_node;
  if (( temp_node = cache_get(key)) != NULL) { // if in cache, delete it
    curr = head;
    head->next->prev = NULL;
    head = head->next;
    free(curr);
  }
}

char *strdups(char *s) {/* make a duplicate of s */
  char *p;
  p = (char *) malloc(strlen(s)+1); /* +1 for ’\0’ */
  if (p != NULL)
    strcpy(p, s);
  return p;
}
