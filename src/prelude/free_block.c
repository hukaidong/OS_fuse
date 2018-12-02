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

  SuperBlockBuf.free_block_head = 1;
  SuperBlockBuf.free_block_size = MAX_BLOCK_NUM - 1;
  superblock_write();

  for (int i=1; i<MAX_BLOCK_NUM-1; i++) {
    BlockBuf.free_block_next = i + 1;
    freeblock_write(i);
  }

  BlockBuf.free_block_next = -1;
  freeblock_write(MAX_BLOCK_NUM - 1);
}

int free_block_pop() {
  superblock_read();
  int block_num = SuperBlockBuf.free_block_head;

  if (block_num > 0) {
    freeblock_read(block_num);
    SuperBlockBuf.free_block_size--;
    SuperBlockBuf.free_block_head = BlockBuf.free_block_next;
    superblock_write();
  }

  return block_num;
}

int free_block_push(int block_num) {
  superblock_read();
  BlockBuf.free_block_next = SuperBlockBuf.free_block_head;
  freeblock_write(block_num);

  SuperBlockBuf.free_block_size++;
  SuperBlockBuf.free_block_head = block_num;
  superblock_write();

  return SuperBlockBuf.free_block_size;
}

int free_block_size() {
  return SuperBlockBuf.free_block_size;
}
