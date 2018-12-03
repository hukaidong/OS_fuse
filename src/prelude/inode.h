#ifndef INODE_STRCT_H
#define INODE_STRCT_H

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#define INODE_SIZE 128
#define INODE_BLK_SIZE 512

#define DENTRY_MAX_SIZE 2086
#define FENTRY_MAX_SIZE 8344


typedef uint8_t  uchar;
typedef uint16_t ushort;
typedef uint32_t blknum_t;
typedef uint64_t ullong;

union  metadata    { struct { ushort ino, pno, size, fsize, nlink; uchar isdir;}; char _[16]; };
struct di_ent      { char filename[14]; ushort inum; };
struct di_ent      di_ent_c(const char *filename, ushort inum);

struct finode      { union metadata metadata; blknum_t blknum[24]; ushort fpnum[4], ex_fpnum[4]; };
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
void inode_load(int inum, union inode_t *inode);
void inode_dump(int inum, const union inode_t *inode);
void inode_get_attr(ushort inum, struct stat *statbuf);
void inode_get_attr_upc(ushort, int* mode, int* nlink, int* size);
void inode_set_attr_upc(ushort, int* mode, int* nlink, int* size);

void fnode_init(ushort inum, ushort pnum);
void dnode_init(ushort inum, ushort pnum);

blknum_t *fnode_listing(ushort inum, int* size);
struct di_ent *dnode_listing(ushort inum, int* size);
void fnode_listing_set(ushort inum, int newsize);
void dnode_listing_set(ushort inum, int newsize);
void dnode_append(int inum, const struct di_ent new_item);
void dnode_remove(int inum, const char* filename);

ushort free_inode_pop();
void free_inode_push(ushort inum);

#endif /* ifndef INODE_STRCT_H */
