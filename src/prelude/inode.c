#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "inode.h"
#include "block.h"
#include "free_block.h"

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
  union inode_t _inodebuf;
  for (int i=1; i<INODE_BLK_SIZE; i++)
    free_block_pop();

  memset(&_inodebuf, 0xff, INODE_BLK_SIZE / 8);
  _inodebuf._buf[0] = 0x01f;       // 0001 1111
  inode_dump(1, &_inodebuf);

  dnode_init(2, 2);
}

void inode_get_attr(ushort inum, struct stat *statbuf) {
  union inode_t _inodebuf;
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
  union inode_t _inodebuf;
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
  union inode_t _inodebuf;
  struct di_ent dotdot = { "..", pnum };

  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.metadata.isdir = 1;
  _inodebuf.metadata.nlink = 2;
  _inodebuf.metadata.ino = inum;
  _inodebuf.metadata.size = 1;
  _inodebuf.idir.dinum[0] = dotdot;
  inode_dump(inum, &_inodebuf);
}

void fnode_init(ushort inum) {
  union inode_t _inodebuf;
  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.metadata.isdir = 0;
  _inodebuf.metadata.nlink = 1;
  _inodebuf.metadata.ino = inum;
  _inodebuf.metadata.size = 0;
  inode_dump(inum, &_inodebuf);
}

struct di_ent d[2086];
blknum_t f[8344];

static struct di_ent di_ent_c(const char *filename, ushort inum) {
  struct di_ent di_ent;
  memcpy(&di_ent, filename, 14);
  di_ent.inum = inum;
  return di_ent;
}

const struct di_ent dnode_entry_get(ushort inum, int lv0_offset) {
  int lv2_offset = lv0_offset - 6 - 4 * 8,
      lv1_offset = lv0_offset - 6;
  union inode_t _inodebuf;
  inode_load(inum, &_inodebuf);
  if (lv0_offset >= _inodebuf.metadata.size) {
    return di_ent_c("", 0);
  }

  if (lv2_offset >= 0) {
    int lv1_page = lv2_offset / 64 * 8,
        lv2_page = lv2_offset / 8;
        lv2_offset %= 8;

    inode_load(_inodebuf.idir.ex_dpnum[lv1_page], &_inodebuf);
    inode_load(_inodebuf.iex.exnum[lv2_page], &_inodebuf);
    return _inodebuf.pdir.dinum[lv2_offset];
  } else if (lv1_offset >= 0) {
    int lv1_page = lv1_offset / 8;
        lv1_offset %= 8;

    inode_load(_inodebuf.idir.dpnum[lv1_page], &_inodebuf);
    return _inodebuf.pdir.dinum[lv1_offset];
  } else if (lv0_offset >= 0) {
    return _inodebuf.idir.dinum[lv0_offset];
  } else {
    return di_ent_c(".", _inodebuf.metadata.ino);
  }
}

const blknum_t fnode_entry_get(ushort inum, int lv0_offset) {
  int lv2_offset = lv0_offset - 24 - 4 * 32,
      lv1_offset = lv0_offset - 24;
  union inode_t _inodebuf;
  inode_load(inum, &_inodebuf);
  if (lv0_offset >= _inodebuf.metadata.size) {
    return 0;
  }

  if (lv2_offset >= 0) {
    int lv1_page = lv2_offset / 64 * 32,
        lv2_page = lv2_offset / 32;
        lv2_offset %= 32;

    inode_load(_inodebuf.ifile.ex_fpnum[lv1_page], &_inodebuf);
    inode_load(_inodebuf.iex.exnum[lv2_page], &_inodebuf);
    return _inodebuf.pfile.blknum[lv2_offset];
  } else if (lv1_offset >= 0) {
    int lv1_page = lv1_offset / 32;
        lv1_offset %= 32;

    inode_load(_inodebuf.ifile.fpnum[lv1_page], &_inodebuf);
    return _inodebuf.pfile.blknum[lv1_offset];
  } else if (lv0_offset >= 0) {
    return _inodebuf.ifile.blknum[lv0_offset];
  } else {
    return 0;
  }
}

int dnode_entry_set(ushort inum, int lv0_offset, const char *filename, int ino) {
  int lv2_offset = lv0_offset - 6 - 4 * 8,
      lv1_offset = lv0_offset - 6,
      inodenum = inum;
  union inode_t _inodebuf;
  inode_load(inodenum, &_inodebuf);
  if (lv0_offset >= _inodebuf.metadata.size) {
    return 0;
  }

  if (lv2_offset >= 0) {
    int lv1_page = lv2_offset / 64 * 8,
        lv2_page = lv2_offset / 8;
        lv2_offset %= 8;
    inodenum = _inodebuf.idir.ex_dpnum[lv1_page];
    inode_load(inodenum, &_inodebuf);
    inodenum = _inodebuf.iex.exnum[lv2_page];
    inode_load(inodenum, &_inodebuf);
    _inodebuf.pdir.dinum[lv2_offset] = di_ent_c(filename, ino);

  } else if (lv1_offset >= 0) {
    int lv1_page = lv1_offset / 8;
        lv1_offset %= 8;
    inodenum = _inodebuf.idir.dpnum[lv1_page];
    inode_load(inodenum, &_inodebuf);
    _inodebuf.pdir.dinum[lv1_offset] = di_ent_c(filename, ino);
  } else {
    _inodebuf.idir.dinum[lv0_offset] = di_ent_c(filename, ino);
  }
  inode_dump(inodenum, &_inodebuf);
  return 1;
}

int fnode_entry_set(ushort inum, int lv0_offset, blknum_t blknum) {
  int lv2_offset = lv0_offset - 24 - 4 * 32,
      lv1_offset = lv0_offset - 24,
      inodenum = inum;
  union inode_t _inodebuf;
  inode_load(inum, &_inodebuf);
  if (lv0_offset >= _inodebuf.metadata.size) {
    return 0;
  }

  if (lv2_offset >= 0) {
    int lv1_page = lv2_offset / 64 * 32,
        lv2_page = lv2_offset / 32;
        lv2_offset %= 32;

    inodenum = _inodebuf.ifile.ex_fpnum[lv1_page];
    inode_load(inodenum, &_inodebuf);
    inodenum = _inodebuf.iex.exnum[lv2_page];
    inode_load(inodenum, &_inodebuf);
    _inodebuf.pfile.blknum[lv2_offset] = blknum;
  } else if (lv1_offset >= 0) {
    int lv1_page = lv1_offset / 32;
        lv1_offset %= 32;

    inodenum = _inodebuf.ifile.fpnum[lv1_page];
    inode_load(inodenum, &_inodebuf);
    _inodebuf.pfile.blknum[lv1_offset] = blknum;
  } else{
    _inodebuf.ifile.blknum[lv0_offset] = blknum;
  }
  inode_dump(inodenum, &_inodebuf);
  return 1;
}

struct di_ent *dnode_listing(ushort inum, int* st_size, struct di_ent* dot) {
  union inode_t _inodebuf, p_buf, lv1_buf;
  inode_load(inum, &_inodebuf);
  int size = _inodebuf.metadata.size;
  struct di_ent *ptr=d, *ptr_end=d+size;
  *st_size = size;
  if (dot != NULL)
    *dot = di_ent_c(".", _inodebuf.metadata.ino);

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
    inode_load(_inodebuf.idir.ex_dpnum[i], &lv1_buf);
    for (int j=0; j<64; j++) {
      inode_load(lv1_buf.iex.exnum[j], &p_buf);
      memcpy(ptr, p_buf.pdir.dinum, sizeof(struct di_ent) * 8);
      ptr += 8;
      if (ptr >= ptr_end) return d;
    }
  }
  return d;
}

void dnode_listing_set(ushort inum, int newsize) {
  union inode_t _inodebuf, p_buf, lv1_buf;
  inode_load(inum, &_inodebuf);
  int oldsize = _inodebuf.metadata.size;
      _inodebuf.metadata.size = newsize;
  struct di_ent *ptr=d,
                *ptr_end=d+newsize,
                *cache_end=d+oldsize;

  memcpy(_inodebuf.idir.dinum, ptr, sizeof(struct di_ent) * 6);
  ptr += 6;
  for (int i=0; i<4; i++) {
    if (ptr >= ptr_end) {
      if (ptr < cache_end) {
        free_inode_push(_inodebuf.idir.dpnum[i]);
      }
    } else {
      if (ptr >= cache_end) {
        _inodebuf.idir.dpnum[i] = free_inode_pop();
      }
      memcpy(p_buf.pdir.dinum, ptr, sizeof(struct di_ent) * 8);
      inode_dump(_inodebuf.idir.dpnum[i], &p_buf);
    }
    ptr += 8;
  }

  for (int i=0; i<4; i++) {
    if (ptr >= ptr_end) {
      if (ptr < cache_end) {
        inode_load(_inodebuf.idir.ex_dpnum[i], &lv1_buf);
        for (int j=0; j<64; j++) {
          if (ptr < cache_end) {
            free_inode_push(lv1_buf.iex.exnum[j]);
          }
          ptr += 8;
        }
        free_inode_push(_inodebuf.idir.ex_dpnum[i]);
      }
    } else {
      if (ptr >= cache_end) {
        _inodebuf.idir.ex_dpnum[i] = free_inode_pop();
        memset(&lv1_buf, 0, sizeof(lv1_buf));

        for (int j=0; j<64; j++) {
          if (ptr < ptr_end) {
            lv1_buf.iex.exnum[j] = free_inode_pop();
            memcpy(p_buf.pdir.dinum, ptr, sizeof(struct di_ent) * 8);
            inode_dump(lv1_buf.iex.exnum[j], &p_buf);
          }
          ptr += 8;
        }
      } else {
        inode_load(_inodebuf.idir.ex_dpnum[i], &lv1_buf);
        for (int j=0; j<64; j++) {
          if (ptr < ptr_end) {
            if (ptr >= cache_end) {
              lv1_buf.iex.exnum[j] = free_inode_pop();
            }
            memcpy(p_buf.pdir.dinum, ptr, sizeof(struct di_ent) * 8);
            inode_dump(lv1_buf.iex.exnum[j], &p_buf);
          }
          ptr += 8;
        }
      }
      inode_dump(_inodebuf.idir.ex_dpnum[i], &lv1_buf);
    }
  }
  inode_dump(_inodebuf.metadata.ino, &_inodebuf);
}

blknum_t *fnode_listing(ushort inum, int* st_size) {
  union inode_t _inodebuf, p_buf, lv1_buf;
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
    inode_load(_inodebuf.ifile.ex_fpnum[i], &lv1_buf);
    for (int j=0; j<64; j++) {
      inode_load(lv1_buf.iex.exnum[j], &p_buf);
      memcpy(ptr, p_buf.pfile.blknum, sizeof(blknum_t) * 32);
      ptr += 32;
      if (ptr >= ptr_end) return f;
    }
  }
  return f;
}

void fnode_listing_set(ushort inum, int newsize) {
  union inode_t _inodebuf, p_buf, lv1_buf;
  inode_load(inum, &_inodebuf);
  int oldsize = _inodebuf.metadata.size;
      _inodebuf.metadata.size = newsize;
  blknum_t *ptr=f,
           *ptr_end=f+newsize,
           *cache_end=f+oldsize;

  memcpy(_inodebuf.ifile.blknum, ptr, sizeof(blknum_t) * 24);
  ptr += 24;
  for (int i=0; i<4; i++) {
    if (ptr >= ptr_end) {
      if (ptr < cache_end) {
        free_inode_push(_inodebuf.ifile.fpnum[i]);
      }
    } else {
      if (ptr >= cache_end) {
        _inodebuf.ifile.fpnum[i] = free_inode_pop();
      }
      memcpy(p_buf.pdir.dinum, ptr, sizeof(blknum_t) * 32);
      inode_dump(_inodebuf.ifile.fpnum[i], &p_buf);
    }
    ptr += 32;
  }

  for (int i=0; i<4; i++) {
    if (ptr >= ptr_end) {
      if (ptr < cache_end) {
        inode_load(_inodebuf.ifile.ex_fpnum[i], &lv1_buf);
        for (int j=0; j<64; j++) {
          if (ptr < cache_end) {
            free_inode_push(lv1_buf.iex.exnum[j]);
          }
          ptr += 32;
        }
        free_inode_push(_inodebuf.ifile.ex_fpnum[i]);
      }
    } else {
      if (ptr >= cache_end) {
        _inodebuf.ifile.ex_fpnum[i] = free_inode_pop();
        memset(&lv1_buf, 0, sizeof(lv1_buf));

        for (int j=0; j<64; j++) {
          if (ptr < ptr_end) {
            lv1_buf.iex.exnum[j] = free_inode_pop();
            memcpy(p_buf.pfile.blknum, ptr, sizeof(blknum_t) * 32);
            inode_dump(lv1_buf.iex.exnum[j], &p_buf);
          }
          ptr += 32;
        }
      } else {
        inode_load(_inodebuf.ifile.ex_fpnum[i], &lv1_buf);
        for (int j=0; j<64; j++) {
          if (ptr < ptr_end) {
            if (ptr >= cache_end) {
              lv1_buf.iex.exnum[j] = free_inode_pop();
            }
            memcpy(p_buf.pdir.dinum, ptr, sizeof(blknum_t) * 32);
            inode_dump(lv1_buf.iex.exnum[j], &p_buf);
          }
          ptr += 32;
        }
      }
      inode_dump(_inodebuf.ifile.ex_fpnum[i], &lv1_buf);
    }
  }
  inode_dump(_inodebuf.metadata.ino, &_inodebuf);
}

ushort free_inode_pop() {
  union inode_t _inodebuf;
  inode_load(1, &_inodebuf);
  for (int i=0; i<(INODE_BLK_SIZE/8); i++) {
    if (_inodebuf._buf[i] != 0) {
      char ptr = 0x80;
      for (int j=0; j<8; j++) {
        if (_inodebuf._buf[i] & ptr) {
          _inodebuf._buf[i] &= (~ptr);
          inode_dump(1, &_inodebuf);
          return 8*i + j;
        } else {
          ptr >>= 1;
        }
      }
    }
  }
  return 0;
}

void free_inode_push(ushort inum) {
  assert(inum > 2 && inum < INODE_BLK_SIZE);
  union inode_t _inodebuf;
  ushort idx = inum / 8, offset = inum % 8;
  inode_load(1, &_inodebuf);
  _inodebuf._buf[idx] |= (0x80 >> offset);
  inode_dump(1, &_inodebuf);
}


