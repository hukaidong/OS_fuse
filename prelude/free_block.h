#include <string.h>
#include "block.h"

#define INODE_SIZE 128
#define SUPERBLOCK_SIZE 256
#define MAX_INODE_NUM 2048
#define MAX_BLOCK_NUM 32768

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
