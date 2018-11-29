#include <string.h>
#include "block.h"
#include "free_block.h"


union _superblockbuf SuperBlockBuf;
union _blockbuf BlockBuf;

static int __freeblock_read(int block_num) {
  return block_read(block_num, &BlockBuf);
}

static int __freeblock_write(int block_num) {
  return block_write(block_num, &BlockBuf);
}

static void __superblock_write() {
  block_read(0, &BlockBuf);
  memcpy(&BlockBuf, &SuperBlockBuf, SUPERBLOCK_SIZE);
  block_write(0, &BlockBuf);
}

static void __superblock_read() {
  block_read(0, &BlockBuf);
  memcpy(&SuperBlockBuf, &BlockBuf, SUPERBLOCK_SIZE);
}

void free_block_init() {
  memset(&BlockBuf, 0, sizeof(BlockBuf));

  disk_open("/tmp/tempfsfile");
  for (int i=0; i<MAX_BLOCK_NUM; i++) {
    BlockBuf.free_block_next = i + 1;
    __freeblock_write(i);
  }
  disk_close();
}
