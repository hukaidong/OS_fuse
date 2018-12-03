#ifndef PATH_H
#define PATH_H value

#include "inode.h"

int _path_to_inum_elem(const char* path, ushort dest_inum);
int path_to_inum(const char* path, int *parent);
void path_basename(const char* path, char* out);

#endif /* ifndef PATH_H */
