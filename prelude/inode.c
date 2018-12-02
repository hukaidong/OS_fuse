#include <string.h>

#include "inode.h"
#include "block.h"
#include "free_block.h"

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
  for (int i=1; i<INODE_BLK_SIZE; i++)
    free_block_pop();

  memset(&_inodebuf, 0, sizeof(union inode_t));
  memset(&_inodebuf, 1, INODE_SIZE / 8);
  _inodebuf._buf[0] = 0x01f;       // 0001 1111
  inode_dump(1, &_inodebuf);

  dnode_init(2, 2);
  inode_dump(2, &_inodebuf);
}

void inode_get_attr(ushort inum, struct stat *statbuf) {
  inode_load(inum, &_inodebuf);
  if (_inodebuf.metadata.isdir) {
    statbuf->st_mode = S_IFDIR | 0755;
    statbuf->st_nlink = _inodebuf.metadata.nlink;
    statbuf->st_size = _inodebuf.metadata.size;
  } else {
    statbuf->st_mode = S_IFREG | 0755;
    statbuf->st_nlink = _inodebuf.metadata.nlink;
    statbuf->st_size = _inodebuf.metadata.size;
  }

}

void dnode_init(ushort inum, ushort pnum) {
  struct di_ent dotdot = { "..", pnum };

  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.idir.metadata.isdir = 1;
  _inodebuf.idir.metadata.nlink = 2;
  _inodebuf.idir.metadata.ino = inum;
  _inodebuf.idir.metadata.size = 0;
  _inodebuf.idir.dinum[0] = dotdot;
}

void fnode_init(ushort inum) {
  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.ifile.metadata.isdir = 0;
  _inodebuf.ifile.metadata.nlink = 1;
  _inodebuf.ifile.metadata.ino = inum;
  _inodebuf.ifile.metadata.size = 0;
}
