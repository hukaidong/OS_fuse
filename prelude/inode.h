#ifndef INODE_STRCT_H
#define INODE_STRCT_H

#include <stdint.h>
#include <sys/stat.h>

typedef uint32_t ulong;
typedef uint16_t ushort;

union metadata {
  struct {
    mode_t mod;
    ushort blksize;
  };
  char _[16];
};

struct finode {
  union metadata metadata;
  ulong          blknum[24];
  ushort         fpnum[4];
  ushort         lv2_fpnum[4];
};

struct inode_ex1 {
  ushort  exnum[64];
};

struct finode_pure {
  ulong          blknum[32];
};

struct di_ent {
  char filename[14];
  ushort inum;
};

struct dinode {
  union metadata metadata;
  struct di_ent  dinum[6];
  ushort         dpnum[4];
  ushort         ex_dpnum[4];
};

struct dinode_pure {
  struct di_ent  dinum[8];
};

union inode_t {
  struct inode_ex1 iex;
  struct dinode idir;
  struct finode ifile;
  struct dinode_pure pdir;
  struct finode_pure pfile;
};



#endif /* ifndef INODE_STRCT_H */
