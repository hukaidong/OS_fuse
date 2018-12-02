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
  if (_inodebuf.metadata.isdir == 1) {
    statbuf->st_mode = S_IFDIR | 0755;
    statbuf->st_nlink = _inodebuf.metadata.nlink;
    statbuf->st_size = _inodebuf.metadata.size;
  } else {
    statbuf->st_mode = S_IFREG | 0755;
    statbuf->st_nlink = _inodebuf.metadata.nlink;
    statbuf->st_size = _inodebuf.metadata.size;
  }

}

void inode_get_attr_upc(ushort inum, int *st_mode, int *st_nlink, int *st_size) {
  inode_load(inum, &_inodebuf);
  if (st_mode != NULL) {
    if (_inodebuf.metadata.isdir) {
      *st_mode = S_IFDIR | 0755;
    } else {
      *st_mode = S_IFREG | 0755;
    }
  }
  if (st_nlink) *st_nlink = _inodebuf.metadata.nlink;
  if (st_size) *st_size = _inodebuf.metadata.size;

}
void dnode_init(ushort inum, ushort pnum) {
  struct di_ent dotdot = { "..", pnum };

  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.metadata.isdir = 1;
  _inodebuf.metadata.nlink = 2;
  _inodebuf.metadata.ino = inum;
  _inodebuf.metadata.size = 1;
  _inodebuf.idir.dinum[0] = dotdot;
}

void fnode_init(ushort inum) {
  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.metadata.isdir = 0;
  _inodebuf.metadata.nlink = 1;
  _inodebuf.metadata.ino = inum;
  _inodebuf.metadata.size = 0;
}

struct di_ent d[2086];
blknum_t f[8344];
union inode_t lv1_buf, p_buf;

const struct di_ent *dnode_listing(ushort inum, int* st_size) {
  inode_load(inum, &_inodebuf);
  int size = _inodebuf.metadata.size;
  struct di_ent *ptr=d, *ptr_end=d+size+1;
  *st_size = size + 1;
  memcpy(ptr, ".", 2);
  ptr->inum = _inodebuf.metadata.ino;
  ptr ++;
  memcpy(ptr, _inodebuf.idir.dinum, sizeof(struct di_ent) * 6);
  ptr += 6;
  if (ptr >= ptr_end) return d;
  for (int i=0; i<4; i++) {
    inode_load(_inodebuf.idir.dpnum[i], &p_buf);
    memcpy(ptr, p_buf.pdir.dinum, sizeof(struct di_ent) * 8);
    ptr += 8;
    if (ptr >= ptr_end) return d;
  }

  for (int i=0; i<4; i++) {
    inode_load(_inodebuf.idir.dpnum[i], &lv1_buf);
    for (int j=0; j<64; j++) {
      inode_load(lv1_buf.iex.exnum[i], &p_buf);
      memcpy(ptr, p_buf.pdir.dinum, sizeof(struct di_ent) * 8);
      ptr += 8;
      if (ptr >= ptr_end) return d;
    }
  }
  return d;
}

const blknum_t *fnode_listing(ushort inum, int* st_size) {
  inode_load(inum, &_inodebuf);
  int size = _inodebuf.metadata.size;
  *st_size = size;
  blknum_t *ptr=f, *ptr_end=f+size;

  memcpy(ptr, _inodebuf.ifile.blknum, sizeof(blknum_t) * 24);
  ptr += 24;
  if (ptr >= ptr_end) return f;
  for (int i=0; i<4; i++) {
    inode_load(_inodebuf.ifile.fpnum[i], &p_buf);
    memcpy(ptr, p_buf.pfile.blknum, sizeof(blknum_t) * 32);
    ptr += 32;
    if (ptr >= ptr_end) return f;
  }

  for (int i=0; i<4; i++) {
    inode_load(_inodebuf.ifile.fpnum[i], &lv1_buf);
    for (int j=0; j<64; j++) {
      inode_load(lv1_buf.iex.exnum[i], &p_buf);
      memcpy(ptr, p_buf.pfile.blknum, sizeof(blknum_t) * 32);
      ptr += 32;
      if (ptr >= ptr_end) return f;
    }
  }
  return f;
}
