#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "err.h"
#include "inode.h"
#include "block.h"
#include "free_block.h"

static union inode_t _iblockbuf[4];

void inode_load(int inum, union inode_t *inode) {
  if (inum < 0) return;
  int bidx = inum / 4;
  int iidx = inum % 4;
  block_read(bidx, _iblockbuf);
  memcpy(inode, _iblockbuf+iidx, sizeof(union inode_t));
}

void inode_dump(int inum, const union inode_t *inode) {
  if (inum < 0) return;
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
  free_block_pop(); // bit map block

  union _blockbuf b;
  memset(&b, 0xff, sizeof(b));
  b.buf[0] = 0x01f;       // 0001 1111
  block_write(INODE_BLK_SIZE, &b);

  dnode_init(2, 2);
}

void inode_get_attr(ushort inum, struct stat *statbuf) {
  union inode_t _inodebuf;
  inode_load(inum, &_inodebuf);
  statbuf->st_blksize = BLOCK_SIZE;
  if (_inodebuf.metadata.isdir == 1) {
    statbuf->st_mode = S_IFDIR | 0755;
    statbuf->st_nlink = _inodebuf.metadata.nlink;
    statbuf->st_blocks = 0;
    statbuf->st_size = _inodebuf.metadata.size;
  } else {
    statbuf->st_mode = S_IFREG | 0644;
    statbuf->st_nlink = _inodebuf.metadata.nlink;
    statbuf->st_blocks = _inodebuf.metadata.size;
    statbuf->st_size = _inodebuf.metadata.fsize;
  }

}

void inode_get_attr_upc(ushort inum, int *st_mode, int *st_nlink, int *st_size) {
  union inode_t _inodebuf;
  inode_load(inum, &_inodebuf);
  if (_inodebuf.metadata.isdir) {
      if (st_mode) *st_mode = S_IFDIR | 0755;
      if (st_size) *st_size = _inodebuf.metadata.size;
  } else {
      if (st_mode) *st_mode = S_IFREG | 0644;
      if (st_size) *st_size = _inodebuf.metadata.fsize;
  }
  if (st_nlink) *st_nlink = _inodebuf.metadata.nlink;
}

void inode_set_attr_upc(ushort inum, int *st_mode, int *st_nlink, int *st_size) {
  union inode_t _inodebuf;
  inode_load(inum, &_inodebuf);
  if (_inodebuf.metadata.isdir) {
    if (st_size) {
      _inodebuf.metadata.size =
        *st_size > DENTRY_MAX_SIZE ? DENTRY_MAX_SIZE : *st_size;
    }
  } else {
    if (st_size) {
      _inodebuf.metadata.fsize =
        *st_size > FCONTENT_MAX_SIZE ? FCONTENT_MAX_SIZE : *st_size;
    }
  }
  if (st_nlink) _inodebuf.metadata.nlink = *st_nlink;
  inode_dump(inum, &_inodebuf);
}

void dnode_init(ushort inum, ushort pnum) {
  union inode_t _inodebuf;
  struct di_ent dot = { ".", inum };
  struct di_ent dotdot = { "..", pnum };

  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.metadata.isdir = 1;
  _inodebuf.metadata.nlink = 2;
  _inodebuf.metadata.ino = inum;
  _inodebuf.metadata.pno = pnum;
  _inodebuf.metadata.size = 2;
  _inodebuf.idir.dinum[0] = dot;
  _inodebuf.idir.dinum[1] = dotdot;
  inode_dump(inum, &_inodebuf);
}

void fnode_init(ushort inum, ushort pnum) {
  if (inum < 0) return;
  union inode_t _inodebuf;
  memset(&_inodebuf, 0, sizeof(union inode_t));
  _inodebuf.metadata.isdir = 0;
  _inodebuf.metadata.nlink = 1;
  _inodebuf.metadata.ino = inum;
  _inodebuf.metadata.pno = pnum;
  _inodebuf.metadata.size = 0;
  _inodebuf.metadata.fsize = 0;
  inode_dump(inum, &_inodebuf);
}

static struct di_ent d[DENTRY_MAX_SIZE];
static blknum_t f[FENTRY_MAX_SIZE];

struct di_ent di_ent_c(const char *filename, ushort inum) {
  struct di_ent di_ent;
  memcpy(&di_ent, filename, 14);
  di_ent.filename[13] = 0;
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
  } else {
    return _inodebuf.idir.dinum[lv0_offset];
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

struct di_ent *dnode_listing(ushort inum, int* st_size) {
  union inode_t _inodebuf, p_buf, lv1_buf;
  inode_load(inum, &_inodebuf);
  int size = _inodebuf.metadata.size;
  struct di_ent *ptr=d, *ptr_end=d+size;
  *st_size = size;

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
  if (newsize >= DENTRY_MAX_SIZE) {
    errno_push(-EFBIG);
    newsize = DENTRY_MAX_SIZE - 1;
  }
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
  if (newsize > FENTRY_MAX_SIZE) {
    errno_push(-EFBIG);
    newsize = FENTRY_MAX_SIZE;
  }
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
  union _blockbuf b;

  block_read(INODE_BLK_SIZE, &b);
  for (int i=0; i<(INODE_BLK_SIZE/8); i++) {
    if (b.buf[i] != 0) {
      char ptr = 0x80;
      for (int j=0; j<8; j++) {
        if (b.buf[i] & ptr) {
          b.buf[i] &= (~ptr);
          block_write(INODE_BLK_SIZE, &b);
          return 8*i + j;
        } else {
          ptr >>= 1;
        }
      }
    }
  }
  errno_push(-ENOSPC);
  return 0;
}

void free_inode_push(ushort inum) {
  if (inum < 2) return;
  union _blockbuf b;
  ushort idx = inum / 8, offset = inum % 8;
  block_read(INODE_BLK_SIZE, &b);
  b.buf[idx] |= (0x80 >> offset);
  block_write(INODE_BLK_SIZE, &b);
}


void dnode_append(int inum, const struct di_ent new_item) {
  int size;

  struct di_ent *d = dnode_listing(inum, &size);
  d[size] = new_item;
  dnode_listing_set(inum, size+1);

  union inode_t _inodebuf;
  inode_load(inum, &_inodebuf);
  _inodebuf.metadata.nlink++;
  inode_dump(inum, &_inodebuf);
}

void dnode_remove(int inum, const char* filename) {
  int size;

  struct di_ent *d = dnode_listing(inum, &size);
  struct di_ent *d_end = d + size;
  for ( ; d<d_end; d++) {
    if (!strncmp(d->filename, filename, 13)) {
      memcpy(d, d_end-1, sizeof(*d));

      dnode_listing_set(inum, size-1);
      union inode_t _inodebuf;
      inode_load(inum, &_inodebuf);
      _inodebuf.metadata.nlink--;
      inode_dump(inum, &_inodebuf);
    }
  }
}

