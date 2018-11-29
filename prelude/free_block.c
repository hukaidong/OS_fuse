#include <string.h>
#include "block.h"
#include "free_block.h"


union _superblockbuf SuperBlockBuf;
union _blockbuf BlockBuf;

static int freeblock_read(int block_num) {
  return block_read(block_num, &BlockBuf);
}

static int freeblock_write(int block_num) {
  return block_write(block_num, &BlockBuf);
}

static void superblock_write() {
  block_read(0, &BlockBuf);
  memcpy(&BlockBuf, &SuperBlockBuf, SUPERBLOCK_SIZE);
  block_write(0, &BlockBuf);
}

static void superblock_read() {
  block_read(0, &BlockBuf);
  memcpy(&SuperBlockBuf, &BlockBuf, SUPERBLOCK_SIZE);
}

void free_block_init() {
  memset(&BlockBuf, 0, sizeof(BlockBuf));
  memset(&SuperBlockBuf, 0, sizeof(SuperBlockBuf));

  for (int i=0; i<MAX_BLOCK_NUM; i++) {
    BlockBuf.free_block_next = i + 1;
    freeblock_write(i);
  }

  SuperBlockBuf.free_block_head = 1;
  SuperBlockBuf.free_block_size = MAX_BLOCK_NUM - 2;
  superblock_write();
}

int free_block_pop() {
  superblock_read();
  int block_num = SuperBlockBuf.free_block_head;

  freeblock_read(block_num);
  SuperBlockBuf.free_block_size--;
  SuperBlockBuf.free_block_head = BlockBuf.free_block_next;
  superblock_write();
  return block_num;
}
