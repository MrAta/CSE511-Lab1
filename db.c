//
// Created by sps5394 on 10/18/18.
//

#include "db.h"
char *filename = "names.txt";
// Lock the file when connected to it
pthread_mutex_t db_mutex;

void db_connect()  {
  pthread_mutex_lock(&db_mutex);
  file = fopen(filename, "r+");
}

void db_cleanup() {
  fclose(file);
  pthread_mutex_unlock(&db_mutex);
}


int db_get(char *key, char **ret_buf, int *ret_len) {
  char *tmp_line = NULL, *line_key = NULL, *line_val = NULL;
  tmp_line = (char *) calloc(MAX_ENTRY_SIZE, sizeof(char));
  *ret_buf = (char *) calloc(MAX_ENTRY_SIZE, sizeof(char));
  while (1) {
    if (fgets(tmp_line, MAX_ENTRY_SIZE, file) != NULL) {
      line_key = strtok(tmp_line, " ");
      if (strcmp(line_key, key) == 0) { // found key in db
        line_val = strtok(NULL, " ");
        strncpy(*ret_buf, line_val, strlen(line_val));
        strncpy(*ret_buf + strlen(line_val), "\0", 1);
        *ret_len = (int) strlen(line_val);
        free(tmp_line);
        rewind(file);
        return EXIT_SUCCESS;
      }
      continue;
    } else { // end of file (or other error reading); didnt find key
      strcpy(*ret_buf, "NOTFOUND");
      *ret_len = 13;
      free(tmp_line);
      rewind(file);
      return EXIT_FAILURE;
    }
  }
}

int db_insert(char *key, char *value, char **ret_buf, int *ret_len) {
  int found;
  int fbuf_bytes;

  char *fbuf;
  *ret_buf = (char *) calloc(MAX_ENTRY_SIZE, sizeof(char));
  found = db_search(key, &fbuf, &fbuf_bytes);

  if (found) { // duplicate key error
    strcpy(*ret_buf, "DUPLICATE");
    *ret_len = 13;
    free(fbuf);
    return EXIT_FAILURE;
  } else {
    // key not found in db, add the new k/v to the end of file
    memcpy(fbuf + fbuf_bytes, key, strlen(key));
    memcpy(fbuf + fbuf_bytes + strlen(key), " ", 1);
    memcpy(fbuf + fbuf_bytes + strlen(key) + 1, value, strlen(value));
    memcpy(fbuf + fbuf_bytes + strlen(key) + 1 + strlen(value), "\n", 1);

    rewind(file);
    fwrite(fbuf, sizeof(char), fbuf_bytes + strlen(key) + 1 + strlen(value) + 1, file);
    fflush(file);
    ftruncate(fileno(file), fbuf_bytes + strlen(key) + 1 + strlen(value) + 1);
    strcpy(*ret_buf, "SUCCESS");
    *ret_len = 7;
  }
  free(fbuf);
  return 0;
}

int db_put(char *key, char *value, char **ret_buf, int *ret_len) {
  int found = 0;
  int fbuf_bytes;

  char *fbuf;
  *ret_buf = (char *) calloc(MAX_ENTRY_SIZE, sizeof(char));
  found = db_search(key, &fbuf, &fbuf_bytes);
  if (!found) {
    strcpy(*ret_buf, "NOTFOUND"); // key not found in db to do put, return NULL, dont update to file
    *ret_len = 11;
    free(fbuf);
    fbuf = NULL;
    pthread_mutex_unlock(&db_mutex);
    return EXIT_FAILURE;
  } else {
    // key found in db, add the new k/v to the end of file
    memcpy(fbuf + fbuf_bytes, key, strlen(key));
    memcpy(fbuf + fbuf_bytes + strlen(key), " ", 1);
    memcpy(fbuf + fbuf_bytes + strlen(key) + 1, value, strlen(value));
    memcpy(fbuf + fbuf_bytes + strlen(key) + 1 + strlen(value), "\n", 1);

    rewind(file);
    fwrite(fbuf, sizeof(char), fbuf_bytes + strlen(key) + 1 + strlen(value) + 1, file);
    fflush(file);
    ftruncate(fileno(file), fbuf_bytes + strlen(key) + 1 + strlen(value) + 1);
    strncpy(*ret_buf, "SUCCESS", 7);
    *ret_len = 7;
  }
  free(fbuf);
  return EXIT_SUCCESS;
}

int db_delete(char *key, char **ret_buf, int *ret_len) {
  int found = 0;
  int fbuf_bytes;

  char *fbuf;
  *ret_buf = (char *) calloc(MAX_ENTRY_SIZE, sizeof(char));
  found = db_search(key, &fbuf, &fbuf_bytes);
  if (!found) {
    strcpy(*ret_buf, "NOTFOUND");
    *ret_len = 11;
    free(fbuf);
    pthread_mutex_unlock(&db_mutex);
    return EXIT_FAILURE;
  } else { // key found in db, write fbuf to file without that entry
    rewind(file);
    fwrite(fbuf, sizeof(char), fbuf_bytes, file);
    fflush(file);
    ftruncate(fileno(file), fbuf_bytes);
    strncpy(*ret_buf, "SUCCESS", 7);
    *ret_len = 7;
  }
  free(fbuf);
  return 0;
}

int db_search(const char *key, char **fbuf, int *fbuf_bytes) {
  char *tmp_line = NULL;
  char *tmp_line_copy = NULL;
  (*fbuf) = NULL;
  int fsz, found = 0;
  char *line_key = NULL;
  struct stat st;

  tmp_line = (char *) calloc(MAX_ENTRY_SIZE, sizeof(char));
  tmp_line_copy = (char *) calloc(MAX_ENTRY_SIZE, sizeof(char));

  stat("names.txt", &st);
  fsz = (int) st.st_size;
  (*fbuf) = (char *) calloc(fsz + MAX_ENTRY_SIZE, sizeof(char));
  (*fbuf_bytes) = 0;
  while (1) {
    if (fgets(tmp_line, MAX_ENTRY_SIZE, file) != NULL) {
      memset(tmp_line_copy, 0, MAX_ENTRY_SIZE);
      memcpy(tmp_line_copy, tmp_line, strlen(tmp_line));
      line_key = strtok(tmp_line, " ");
      if (strcmp(line_key, key) == 0) { // found key in db; error
        found = 1;
        continue;
      } else { // line key doesnt match search key, go to next line
        memcpy((*fbuf) + (*fbuf_bytes), tmp_line_copy, strlen(tmp_line_copy));
        (*fbuf_bytes) += strlen(tmp_line_copy);
      }
    } else { // end of file
      break;
    }
  }
  free(tmp_line_copy);
  free(tmp_line);
  return found;
}
