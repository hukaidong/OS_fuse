#include <string.h>
#include <linux/limits.h>

#include "path.h"

int _path_to_inum_elem(const char* path, ushort dest_inum) {
  if (dest_inum < 2) return -1;
  if (!strcmp(path, ".")) return dest_inum;
  int size;
  const struct di_ent* d = dnode_listing(dest_inum, &size, NULL);
  for (int i=0; i<size; i++) {
    if (!strncmp(path, d[i].filename, 14)) return d[i].inum;
  }
  return -1;
}

int path_to_inum(const char* path, int *parent) {
  int inum = 2;
  char path_dup[PATH_MAX], *head, *tail;
  strcpy(path_dup, path);
  head = path_dup + 1;        // skip proceeding "/"
  tail = path_dup + 1;

  if (path_dup[strlen(path_dup)-1] == '/')
    path_dup[strlen(path_dup)-1] = 0;

  while (*tail != 0) {
    if (*tail == '/') {
      *tail = 0;
      inum = _path_to_inum_elem(head, inum);
      head = tail + 1;
    }
    tail++;
  }
  if (parent != NULL) *parent = inum;
  if (head != tail)
    inum = _path_to_inum_elem(head, inum);
  return inum;

}

