#include <stdlib.h>

#include "err.h"
#include "inode.h"
#include "free_block.h"

int fstream_read(int inum, char *buf, int size, int offset) {
    int f_size, b_size;
    blknum_t *f = fnode_listing(inum, &b_size);
    inode_get_attr_upc(inum, NULL, NULL, &f_size);

    int b_start = offset / BLOCK_SIZE,
        b_end = (offset + size) / BLOCK_SIZE + 1;

    char *blkbuf = calloc((b_end-b_start), BLOCK_SIZE), *ptr=blkbuf;

    for (int i=b_start; i<b_size && i<b_end; i++) {
      block_read(f[i], ptr);
      ptr += BLOCK_SIZE;
    }

    ptr = blkbuf;
    size = (f_size-offset) < size ? (f_size-offset) : size;
    size = size > 0 ? size : 0;
    memcpy(buf, ptr + (offset % BLOCK_SIZE), size);
    return size;
}

int fstream_write(int inum, const char *buf, int size, int offset) {
    int fsize, b_size,
        b_start = offset / BLOCK_SIZE,
        b_end = (offset+size-1) / BLOCK_SIZE + 1;

    blknum_t *f = fnode_listing(inum, &b_size);
    inode_get_attr_upc(inum, NULL, NULL, &fsize);

    char *blkbuf = calloc((b_end-b_start), BLOCK_SIZE), *ptr=blkbuf;
    for (int i=b_start; i<b_size && i<b_end; i++) {
      block_read(f[i], ptr);
      ptr += BLOCK_SIZE;
    }
    memcpy(blkbuf+(offset % BLOCK_SIZE), buf, size);

    ptr=blkbuf;
    for (int i=b_start; i<b_end && i<FENTRY_MAX_SIZE; i++) {
      if (i < b_size) {
        block_write(f[i], ptr);
      } else {
        f[i] = free_block_allocate(ptr);
      }
      ptr += BLOCK_SIZE;
    }

    b_size = b_size > b_end ? b_size : b_end;
    fsize = fsize > (size+offset) ? fsize : (size+offset);

    inode_set_attr_upc(inum, NULL, NULL, &fsize);
    fnode_listing_set(inum, b_size);

    int retstat = errno_pop();
    return retstat < 0? retstat : size;
}
