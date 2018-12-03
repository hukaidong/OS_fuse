static int ERRNO;

void errno_push(int errno) {
  ERRNO = errno;
}

int errno_pop() {
  int errno = ERRNO;
  ERRNO = 0;
  return errno;
}
