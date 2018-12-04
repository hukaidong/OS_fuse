#include <stdlib.h>
#include <stdio.h>

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
    int fsize, b_size, i, blknum,
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
    for (i=b_start; i<b_end && i<FENTRY_MAX_SIZE; i++) {
      if (i < b_size) {
        block_write(f[i], ptr);
      } else {
        blknum = free_block_allocate(ptr);
        if (blknum < 0) break;
        f[i] = blknum;
      }
      ptr += BLOCK_SIZE;
    }

    b_size = b_size > i ? b_size : i;
    fsize = fsize > (size+offset) ? fsize : (size+offset);

    inode_set_attr_upc(inum, NULL, NULL, &fsize);
    fnode_listing_set(inum, b_size);

    int retstat = errno_pop();
    return retstat < 0? retstat : size;
}

void fstream_free(int inum, int new_b_size) {
  int b_size;
  blknum_t *f = fnode_listing(inum, &b_size);
  for (int i=new_b_size; i<b_size; i++) {
    free_block_push(f[i]);
    printf("free_block_push %d", f[i]);
  }
  fnode_listing_set(inum, new_b_size);
}
