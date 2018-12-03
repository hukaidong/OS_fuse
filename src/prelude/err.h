#ifndef ERRNO_H
#define ERRNO_H value

#include <errno.h>

void errno_push(int err);
int errno_pop();

#endif /* ifndef ERRNO_H */
