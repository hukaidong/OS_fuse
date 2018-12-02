#include <string.h>
#include <stdlib.h>

#include "block.h"
#include "free_block.h"
#include "minunit.h"
#include "inode.h"

void test_setup(void) { }
void test_teardown(void) { }

/*
 * WARNING:
 *  An unexcepted behavior here leads disk cannot
 *  Be reopened after it being closed. In block.c
 *  ln 21, the intention is to prevent multiple
 *  open to same (or different file) but induced
 *  this bug.
 */

MU_TEST(test_block_io) {
  const char *STR = "Hello, Block IO!";
  char buf[BLOCK_SIZE];

  memset(buf, 0, BLOCK_SIZE);
  strcpy(buf, STR);
  block_write(1, buf);

  memset(buf, 0, BLOCK_SIZE);
  block_read(1, buf);
  mu_assert_int_eq(0, strcmp(buf, STR));
}

MU_TEST_SUITE(test_vendor) {
  MU_RUN_SUITE(test_block_io);
}

MU_TEST(test_fb_init) {
  free_block_init();
}

MU_TEST(test_fb_pop) {
  free_block_init();
  mu_assert_int_eq(1, free_block_pop());
  mu_assert_int_eq(2, free_block_pop());
  mu_assert_int_eq(3, free_block_pop());
  mu_assert_int_eq(4, free_block_pop());
}

MU_TEST(test_fb_pop_till_empty) {
  free_block_init();
  int count = 0;
  for ( ; free_block_pop()>0; count++);
  mu_assert_int_eq(MAX_BLOCK_NUM - 1, count);
  mu_assert_int_eq(-1, free_block_pop());
  mu_assert_int_eq(0, free_block_size());
}

MU_TEST(test_fb_push) {
  free_block_init();
  int rand_blk_list[] = { 1, 6, 7, 14, 5, 8 }, blk_base = 512;
  for (int i=0; i<20 + blk_base; i++)
    free_block_pop();

  free_block_push(4+blk_base);
  mu_assert_int_eq(4+blk_base, free_block_pop());

  for (int i=0; i<6; i++)
    free_block_push(rand_blk_list[5-i]+blk_base);

  for (int i=0; i<6; i++)
    mu_assert_int_eq(rand_blk_list[i]+blk_base, free_block_pop());

  for (int i=0; i<MAX_BLOCK_NUM; i++)
    free_block_pop();

  free_block_push(20+blk_base);
  mu_assert_int_eq(1, free_block_size());
  mu_assert_int_eq(20+blk_base, free_block_pop());
}

MU_TEST(test_inode_struct) {
  mu_assert_int_eq(INODE_SIZE, sizeof(struct inode_ex1));
  mu_assert_int_eq(INODE_SIZE, sizeof(struct finode));
  mu_assert_int_eq(INODE_SIZE, sizeof(struct finode_pure));
  mu_assert_int_eq(INODE_SIZE, sizeof(struct dinode));
  mu_assert_int_eq(INODE_SIZE, sizeof(struct dinode_pure));
  mu_assert_int_eq(INODE_SIZE, sizeof(union inode_t));
}

MU_TEST(test_inode_init) {
  free_block_init();
  inode_init();
  union inode_t node;
  inode_load(2, &node);
  mu_assert_int_eq(1, node.metadata.size);
  mu_assert_int_eq(1, node.metadata.isdir);
  mu_assert_int_eq(2, node.metadata.nlink);
}

MU_TEST(test_inode_push_pop_create) {
  free_block_init();
  inode_init();
  mu_assert_int_eq(3, free_inode_pop());
  mu_assert_int_eq(4, free_inode_pop());
  free_inode_push(3);
  mu_assert_int_eq(3, free_inode_pop());
  mu_assert_int_eq(5, free_inode_pop());
  mu_assert_int_eq(6, free_inode_pop());
  mu_assert_int_eq(7, free_inode_pop());
  mu_assert_int_eq(8, free_inode_pop());
  mu_assert_int_eq(9, free_inode_pop());
  free_inode_push(5);
  free_inode_push(8);
  mu_assert_int_eq(5, free_inode_pop());
  mu_assert_int_eq(8, free_inode_pop());

  free_block_init();
  inode_init();
  int i = 0;
  while(free_inode_pop()) i++;
  mu_assert_int_eq(509, i);
  fnode_init(3);
  dnode_init(4, 2);
  union inode_t node;
  inode_load(3, &node);
  mu_assert_int_eq(0, node.metadata.size);
  mu_assert_int_eq(0, node.metadata.isdir);
  mu_assert_int_eq(1, node.metadata.nlink);
  inode_load(4, &node);
  mu_assert_int_eq(1, node.metadata.size);
  mu_assert_int_eq(1, node.metadata.isdir);
  mu_assert_int_eq(2, node.metadata.nlink);
}

MU_TEST(test_dnode_list_adhoc) {
  char rndbuf[4096];
  union inode_t node;
  int sizef, target_sizef;
  struct di_ent *blk;

  free_block_init();
  inode_init();
  for (int i=0; i<4096; i++) rndbuf[i] = rand();
  mu_assert_int_eq(3, free_inode_pop());
  dnode_init(3, 2);
  blk = dnode_listing(3, &sizef, NULL);
  mu_assert_int_eq(1, sizef);

  // just fill origin inode
  target_sizef = 6;
  memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
  dnode_listing_set(3, target_sizef);
  mu_assert_int_eq(4, free_inode_pop());  // no new inode created
  free_inode_push(4);
  // reading
  memset(blk, 0, sizeof(struct di_ent) * target_sizef);
  blk = dnode_listing(3, &sizef, NULL);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));
}

MU_TEST(test_dnode_list_lv1exp) {
  char rndbuf[4096];
  union inode_t node;
  int sizef, target_sizef;
  struct di_ent *blk;

  // just a new expanding node
  free_block_init();
  inode_init();
  target_sizef = 7;

  mu_assert_int_eq(3, free_inode_pop());
  dnode_init(3, 2);
  blk = dnode_listing(3, &sizef, NULL);
  memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
  dnode_listing_set(3, target_sizef);

  // verifing
  mu_assert_int_eq(5, free_inode_pop());  // 1 inode created
  free_inode_push(5);
  inode_load(3, &node);
  mu_assert_int_eq(4, node.ifile.fpnum[0]);
  inode_load(4, &node);
  mu_assert_int_eq(0, memcmp(node._buf, rndbuf+sizeof(struct di_ent)*6, sizeof(struct di_ent)));

  memset(blk, 0, sizeof(struct di_ent) * target_sizef);
  blk = dnode_listing(3, &sizef, NULL);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));

  // last lv1 expanding node
  target_sizef = 38;
  memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
  dnode_listing_set(3, target_sizef);

  // verifing
  mu_assert_int_eq(8, free_inode_pop());  // other 3 inodes created
  free_inode_push(8);
  inode_load(3, &node);
  mu_assert_int_eq(4, node.ifile.fpnum[0]);
  mu_assert_int_eq(5, node.ifile.fpnum[1]);
  mu_assert_int_eq(6, node.ifile.fpnum[2]);
  mu_assert_int_eq(7, node.ifile.fpnum[3]);

  // reading
  memset(blk, 0, sizeof(struct di_ent) * target_sizef);
  blk = dnode_listing(3, &sizef, NULL);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));

}

MU_TEST(test_dnode_list_lv2exp) {
  char rndbuf[65536];
  union inode_t node;
  int sizef, target_sizef;
  struct di_ent *blk;
  for (int i=0; i<65536; i++) rndbuf[i] = rand();

  // just a new expanding node
  free_block_init();
  inode_init();
  target_sizef = 39;

  mu_assert_int_eq(3, free_inode_pop());
  dnode_init(3, 2);
  blk = dnode_listing(3, &sizef, NULL);
  memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
  dnode_listing_set(3, target_sizef);

  // verifing
  mu_assert_int_eq(10, free_inode_pop());  // 3, (4, 5, 6, 7), (8 -> 9)
  free_inode_push(10);

  memset(blk, 0, sizeof(struct di_ent) * target_sizef);
  blk = dnode_listing(3, &sizef, NULL);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));

  // lv2 expanding node (in first expasion)
  target_sizef = 47;
  memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
  dnode_listing_set(3, target_sizef);

  // reading
  memset(blk, 0, sizeof(struct di_ent) * target_sizef);
  blk = dnode_listing(3, &sizef, NULL);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));

  // last lv2 expanding node
  target_sizef = 2086;
  memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
  dnode_listing_set(3, target_sizef);

  // reading
  memset(blk, 0, sizeof(struct di_ent) * target_sizef);
  blk = dnode_listing(3, &sizef, NULL);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));
}

MU_TEST(test_dnode_list_scale_and_srink) {
  char rndbuf[65536];
  union inode_t node;
  int sizef, target_sizef;
  struct di_ent *blk;
  free_block_init();
  inode_init();
  mu_assert_int_eq(3, free_inode_pop());
  dnode_init(3, 2);

  for (int repeat=0; repeat<10; repeat++) {

    for (int i=0; i<65536; i++) rndbuf[i] = rand();

    target_sizef = 1000;
    blk = dnode_listing(3, &sizef, NULL);
    memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
    dnode_listing_set(3, target_sizef);

    memset(blk, 0, sizeof(struct di_ent) * target_sizef);
    blk = dnode_listing(3, &sizef, NULL);
    mu_assert_int_eq(target_sizef, sizef);
    mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));

    target_sizef = 10;
    blk = dnode_listing(3, &sizef, NULL);
    memcpy(blk, rndbuf, sizeof(struct di_ent) * target_sizef);
    dnode_listing_set(3, target_sizef);

    memset(blk, 0, sizeof(struct di_ent) * target_sizef);
    blk = dnode_listing(3, &sizef, NULL);
    mu_assert_int_eq(target_sizef, sizef);
    mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(struct di_ent)*target_sizef));
  }
}

MU_TEST(test_fnode_list_adhoc) {
  char rndbuf[4096];
  union inode_t node;
  int sizef, target_sizef;
  blknum_t *blk;

  free_block_init();
  inode_init();
  for (int i=0; i<4096; i++) rndbuf[i] = rand();
  mu_assert_int_eq(3, free_inode_pop());
  fnode_init(3);

  blk = fnode_listing(3, &sizef);
  mu_assert_int_eq(0, sizef);

  // just fill origin inode
  target_sizef = 24;
  memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
  fnode_listing_set(3, target_sizef);
  mu_assert_int_eq(4, free_inode_pop());  // no new inode created
  free_inode_push(4);
  // reading
  memset(blk, 0, sizeof(blknum_t) * target_sizef);
  blk = fnode_listing(3, &sizef);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));
}

MU_TEST(test_fnode_list_lv1exp) {
  char rndbuf[4096];
  union inode_t node;
  int sizef, target_sizef;
  blknum_t *blk;

  // just a new expanding node
  free_block_init();
  inode_init();
  target_sizef = 25;

  mu_assert_int_eq(3, free_inode_pop());
  fnode_init(3);
  blk = fnode_listing(3, &sizef);
  memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
  fnode_listing_set(3, target_sizef);

  // verifing
  mu_assert_int_eq(5, free_inode_pop());  // 1 inode created
  free_inode_push(5);
  inode_load(3, &node);
  mu_assert_int_eq(4, node.ifile.fpnum[0]);
  inode_load(4, &node);
  mu_assert_int_eq(0, memcmp(node._buf, rndbuf+sizeof(blknum_t)*24, sizeof(blknum_t)));

  memset(blk, 0, sizeof(blknum_t) * target_sizef);
  blk = fnode_listing(3, &sizef);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));

  // last lv1 expanding node
  target_sizef = 151;
  memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
  fnode_listing_set(3, target_sizef);

  // verifing
  mu_assert_int_eq(8, free_inode_pop());  // other 3 inodes created
  free_inode_push(8);
  inode_load(3, &node);
  mu_assert_int_eq(4, node.ifile.fpnum[0]);
  mu_assert_int_eq(5, node.ifile.fpnum[1]);
  mu_assert_int_eq(6, node.ifile.fpnum[2]);
  mu_assert_int_eq(7, node.ifile.fpnum[3]);

  // reading
  memset(blk, 0, sizeof(blknum_t) * target_sizef);
  blk = fnode_listing(3, &sizef);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));

}

MU_TEST(test_fnode_list_lv2exp) {
  char rndbuf[65536];
  union inode_t node;
  int sizef, target_sizef;
  blknum_t *blk;
  for (int i=0; i<65536; i++) rndbuf[i] = rand();

  // just a new expanding node
  free_block_init();
  inode_init();
  target_sizef = 153;

  mu_assert_int_eq(3, free_inode_pop());
  fnode_init(3);
  blk = fnode_listing(3, &sizef);
  memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
  fnode_listing_set(3, target_sizef);

  // verifing
  mu_assert_int_eq(10, free_inode_pop());  // 3, (4, 5, 6, 7), (8 -> 9)
  free_inode_push(10);

  memset(blk, 0, sizeof(blknum_t) * target_sizef);
  blk = fnode_listing(3, &sizef);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));

  // lv2 expanding node (in first expasion)
  target_sizef = 185;
  memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
  fnode_listing_set(3, target_sizef);

  // reading
  memset(blk, 0, sizeof(blknum_t) * target_sizef);
  blk = fnode_listing(3, &sizef);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));

  // last lv2 expanding node
  target_sizef = 8344;
  memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
  fnode_listing_set(3, target_sizef);

  // reading
  memset(blk, 0, sizeof(blknum_t) * target_sizef);
  blk = fnode_listing(3, &sizef);
  mu_assert_int_eq(target_sizef, sizef);
  mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));
}

MU_TEST(test_fnode_list_scale_and_srink) {
  char rndbuf[65536];
  union inode_t node;
  int sizef, target_sizef;
  blknum_t *blk;
  free_block_init();
  inode_init();
  mu_assert_int_eq(3, free_inode_pop());
  fnode_init(3);

  for (int repeat=0; repeat<10; repeat++) {

    for (int i=0; i<65536; i++) rndbuf[i] = rand();

    target_sizef = 5000;
    blk = fnode_listing(3, &sizef);
    memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
    fnode_listing_set(3, target_sizef);

    memset(blk, 0, sizeof(blknum_t) * target_sizef);
    blk = fnode_listing(3, &sizef);
    mu_assert_int_eq(target_sizef, sizef);
    mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));

    target_sizef = 50;
    blk = fnode_listing(3, &sizef);
    memcpy(blk, rndbuf, sizeof(blknum_t) * target_sizef);
    fnode_listing_set(3, target_sizef);

    memset(blk, 0, sizeof(blknum_t) * target_sizef);
    blk = fnode_listing(3, &sizef);
    mu_assert_int_eq(target_sizef, sizef);
    mu_assert_int_eq(0, memcmp(blk, rndbuf, sizeof(blknum_t)*target_sizef));
  }
}

MU_TEST(test_inode_stat) {
  free_block_init();
  inode_init();
  int st_mode, st_nlink, st_size;
  inode_get_attr_upc(2, &st_mode, &st_nlink, &st_size);
  mu_assert_int_eq(2, st_nlink);
  mu_assert_int_eq(1, st_size);
}

MU_TEST_SUITE(test_suite) {
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
  MU_RUN_TEST(test_fb_init);
	MU_RUN_TEST(test_fb_pop);
	MU_RUN_TEST(test_fb_pop_till_empty);
	MU_RUN_TEST(test_fb_push);
	MU_RUN_TEST(test_inode_struct);
	MU_RUN_TEST(test_inode_init);
	MU_RUN_TEST(test_inode_stat);
	MU_RUN_TEST(test_inode_push_pop_create);
  MU_RUN_TEST(test_fnode_list_adhoc);
  MU_RUN_TEST(test_fnode_list_lv1exp);
  MU_RUN_TEST(test_fnode_list_lv2exp);
  MU_RUN_TEST(test_fnode_list_scale_and_srink);
  MU_RUN_TEST(test_dnode_list_adhoc);
  MU_RUN_TEST(test_dnode_list_lv1exp);
  MU_RUN_TEST(test_dnode_list_lv2exp);
  MU_RUN_TEST(test_dnode_list_scale_and_srink);
}

int main(int argc, char *argv[]) {
  disk_open("/tmp/tempfsfile");
  MU_RUN_SUITE(test_vendor);
	MU_RUN_SUITE(test_suite);
  disk_close();

	MU_REPORT();
	return 0;
}
