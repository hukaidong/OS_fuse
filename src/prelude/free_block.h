#ifndef FREE_BLOCK_H
#define FREE_BLOCK_H

#include <string.h>
#include "block.h"

#define SUPERBLOCK_SIZE 128

#ifndef TESTING_CONFIG
#define MAX_BLOCK_NUM 32768
#else
#define MAX_BLOCK_NUM 32
#endif


extern union _superblockbuf {
  struct {
    int free_block_head;
    int free_block_size;

    int avail_inum;
  };
  char buf[SUPERBLOCK_SIZE];
} SuperBlockBuf;

extern union _blockbuf {
  struct {
    int free_block_next;
  };
  char buf[BLOCK_SIZE];
} BlockBuf;

void free_block_init();
int free_block_pop();
int free_block_push(int block_num);
int free_block_allocate(const char* buf);
int free_block_size();

#endif /* ifndef FREE_BLOCK_H */
