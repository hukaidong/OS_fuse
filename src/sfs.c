/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"
#include "prelude/packer.cpacker"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"


///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "in bb-init\n");
    log_msg("\nsfs_init()\n");
    log_conn(conn);
    log_fuse_context(fuse_get_context());

    disk_open(SFS_DATA->diskfile);
    free_block_init();
    inode_init();

    return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
    disk_close();
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0, inum;
    char fpath[PATH_MAX];

    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
          path, statbuf);
    /* log_fuse_context(fuse_get_context()); */

    memset(statbuf, 0, sizeof(struct stat));
    inum = path_to_inum(path, NULL);
    if (inum < 0) return -ENOENT;
    inode_get_attr(inum, statbuf);
    statbuf->st_uid = fuse_get_context()->uid;
    statbuf->st_gid = fuse_get_context()->gid;

    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n", path, mode, fi);

    int retstat;
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    int inum, p_inum, size;
    inum = path_to_inum(path, &p_inum);
    if (inum > 0) return -EEXIST;
    inum = free_inode_pop();
    fnode_init(inum, p_inum);
    dnode_append(p_inum, di_ent_c(filename, inum));
    retstat = errno_pop();
    if (retstat < 0) {     // reverting
      dnode_remove(p_inum, filename);
      free_inode_push(inum);
    }
    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    int inum, p_inum, size;
    inum = path_to_inum(path, &p_inum);
    if (inum < 0) return -ENOENT;

    fnode_listing_set(inum, 0);
    free_inode_push(inum);
    dnode_remove(p_inum, filename);

    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi)
{
    // JUST MAKE EXISTANCE CHECK
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
            path, fi);
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    int inum, p_inum, size;
    inum = path_to_inum(path, &p_inum);
    if (inum > 0) return -EEXIST;
    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
          path, fi);
    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
            path, buf, size, offset, fi);

    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    int inum, fsize, b_size,
        b_start = offset / BLOCK_SIZE,
        b_end = (offset+size) / BLOCK_SIZE + 1;

    inum = path_to_inum(path, NULL);
    if (inum < 0) return -ENOENT;
    blknum_t *f = fnode_listing(inum, &b_size);
    inode_get_attr_upc(inum, NULL, NULL, &fsize);

    char *blkbuf = calloc((b_end-b_start), BLOCK_SIZE), *ptr=blkbuf;
    for (int i=b_start; i<b_size && i<b_end; i++) {
      block_read(f[i], ptr);
      /* log_msg("block_read %d, \"%s\"", f[i], ptr); */
      ptr += BLOCK_SIZE;
    }
    ptr = blkbuf;
    retstat = (fsize-offset)<size ? fsize-offset : size;
    retstat = retstat > 0 ? retstat : 0;
    memcpy(buf, ptr+offset % BLOCK_SIZE, retstat);
    /* log_msg("readed: (fsize=%d, bsize=%d, retstat=%d, f[0]=%d, blkbuf=\"%s\", buf=\"%s\")\n", */
        /* fsize, b_size, retstat, f[0], blkbuf, buf); */

    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
            path, buf, size, offset, fi);

    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    int inum, fsize, b_size,
        b_start = offset / BLOCK_SIZE,
        b_end = (offset+size) / BLOCK_SIZE + 1;

    inum = path_to_inum(path, NULL);
    inode_get_attr_upc(inum, NULL, NULL, &fsize);
    if (inum < 0) return -ENOENT;
    blknum_t *f = fnode_listing(inum, &b_size);

    char *blkbuf = calloc((b_end-b_start), BLOCK_SIZE), *ptr=blkbuf;
    for (int i=b_start; i<b_size && i<b_end; i++) {
      block_read(f[i], ptr);
      ptr += BLOCK_SIZE;
    }

    ptr=blkbuf;
    memcpy((ptr+(offset % BLOCK_SIZE)), buf, size);
    ptr=blkbuf;
    for (int i=b_start; i<b_end; i++) {
      if (i < b_size) {
        block_write(f[i], ptr);
      } else {
        f[i] = free_block_allocate(ptr);
      }
      ptr += BLOCK_SIZE;
    }
    fsize = fsize > (size+offset) ? fsize: (size+offset);
    fnode_listing_set(inum, b_end);
    inode_set_attr_upc(inum, NULL, NULL, &fsize);
    retstat = size;

    return retstat;
}

int sfs_truncate(const char* path, off_t offset) {
    int retstat = 0;

    log_msg("\nsfs_truncate(path=\"%s\", offset=%lld)\n",
            path, offset);
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;
    int inum, fsize = offset, b_size,
        new_b_size = (offset) / BLOCK_SIZE + 1;

    inum = path_to_inum(path, NULL);

    if (inum < 0) return -ENOENT;
    blknum_t *f = fnode_listing(inum, &b_size);

    char nothing[BLOCK_SIZE];
    memset(nothing, 0, BLOCK_SIZE);
    for (int i=0; i<new_b_size; i++) {
      if (i >= b_size) {
        f[i] = free_block_allocate(nothing);
      }
    }
    fnode_listing_set(inum, new_b_size);
    inode_set_attr_upc(inum, NULL, NULL, &fsize);

    return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
            path, mode);

    char filename[14];
    path_basename(path, filename);
    int inum, pinum;
    inum = path_to_inum(path, &pinum);
    if (inum > 0) return -EEXIST;
    inum = free_inode_pop();
    dnode_init(inum, pinum);
    dnode_append(pinum, di_ent_c(filename, inum));
    retstat = errno_pop();
    if (retstat < 0) {     // reverting
      dnode_remove(pinum, filename);
      free_inode_push(inum);
    }
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n", path);

    char filename[14];
    path_basename(path, filename);
    int inum, pinum;
    inum = path_to_inum(path, &pinum);
    if (inum < 0) return -ENOENT;
    dnode_listing_set(inum, 0);
    dnode_remove(pinum, filename);

    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
  // JUST DO EXISTANCE CHECK
    int retstat = 0;
    /* log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n", */
          /* path, fi); */
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    int inum, p_inum, size;
    inum = path_to_inum(path, &p_inum);
    if (inum > 0) return -EEXIST;
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi)
{
    int retstat = 0, size = 0, inum = 0;

    (void) offset;
    (void) fi;

    inum = path_to_inum(path, NULL);
    if (inum < 0) return -ENOENT;
    const struct di_ent *d = dnode_listing(inum, &size);
    for (int i=0; i<size; i++) {
      filler(buf, d[i].filename, NULL, 0);
    }

    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,
  .truncate = sfs_truncate,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct sfs_state *sfs_data;

    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
        sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
        perror("main calloc");
        abort();
    }

    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc-2];
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    sfs_data->logfile = log_open();

    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

    return fuse_stat;
}
