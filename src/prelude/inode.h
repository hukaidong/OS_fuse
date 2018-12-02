#ifndef INODE_STRCT_H
#define INODE_STRCT_H

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#define INODE_SIZE 128
#define INODE_BLK_SIZE 512

typedef uint8_t  uchar;
typedef uint16_t ushort;
typedef uint32_t blknum_t;
typedef uint64_t ullong;

union  metadata    { struct { ushort ino, size, nlink; uchar isdir;}; char _[16]; };
struct di_ent      { char filename[14]; ushort inum; };

struct finode      { union metadata metadata; blknum_t blknum[24]; ushort fpnum[4], lv2_fpnum[4]; };
struct inode_ex1   { ushort exnum[64]; };
struct finode_pure { blknum_t blknum[32]; };
struct dinode      { union metadata metadata; struct di_ent dinum[6]; ushort dpnum[4], ex_dpnum[4]; };
struct dinode_pure { struct di_ent dinum[8]; };

union inode_t {
  union metadata metadata;
  struct dinode idir;
  struct finode ifile;
  struct dinode_pure pdir;
  struct finode_pure pfile;
  struct inode_ex1 iex;
  char _buf[INODE_SIZE];
};

void inode_init();
void inode_load(ushort inum, union inode_t *inode);
void inode_dump(ushort inum, const union inode_t *inode);
void inode_get_attr(ushort inum, struct stat *statbuf);
void inode_get_attr_upc(ushort, int* mode, int* nlink, int* size);

void fnode_init(ushort inum);
void dnode_init(ushort inum, ushort parent);

const struct di_ent *dnode_listing(ushort inum, int* size);
const blknum_t *fnode_listing(ushort inum, int* size);

#endif /* ifndef INODE_STRCT_H */
