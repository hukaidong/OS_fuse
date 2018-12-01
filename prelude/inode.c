#include <string.h>

#include "inode.h"
#include "block.h"

union inode_t _inodebuf;
union inode_t _iblockbuf[4];

void inode_load(ushort inum, union inode_t *inode) {
  int bidx = inum / 4;
  int iidx = inum % 4;
  block_read(bidx, _iblockbuf);
  memcpy(inode, _iblockbuf+iidx, sizeof(union inode_t));
}

void inode_dump(ushort inum, const union inode_t *inode) {
  int bidx = inum / 4;
  int iidx = inum % 4;
  block_read(bidx, _iblockbuf);
  memcpy(_iblockbuf+iidx, inode, sizeof(union inode_t));
  block_write(bidx, _iblockbuf);
}

void inode_init() {
  memset(&_inodebuf, 1, sizeof(union inode_t));
}

void dnode_init(ushort inum, ushort pnum) {
  struct di_ent dotdot = { "..", pnum };

  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.idir.metadata.isdir = 1;
  _inodebuf.idir.metadata.ino = inum;
  _inodebuf.idir.metadata.blksize = 0;
  _inodebuf.idir.dinum[0] = dotdot;
}

void fnode_init(ushort inum) {
  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.ifile.metadata.isdir = 0;
  _inodebuf.ifile.metadata.ino = inum;
  _inodebuf.ifile.metadata.blksize = 0;
}
