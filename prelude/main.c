#include <string.h>

#include "block.h"
#include "free_block.h"
#include "minunit.h"


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
  int rand_blk_list[] = { 1, 6, 7, 14, 5, 8 };
  for (int i=0; i<20; i++)
    free_block_pop();

  free_block_push(4);
  mu_assert_int_eq(4, free_block_pop());

  for (int i=0; i<6; i++)
    free_block_push(rand_blk_list[5-i]);

  for (int i=0; i<6; i++)
    mu_assert_int_eq(rand_blk_list[i], free_block_pop());

  for (int i=0; i<MAX_BLOCK_NUM; i++)
    free_block_pop();

  free_block_push(20);
  mu_assert_int_eq(1, free_block_size());
  mu_assert_int_eq(20, free_block_pop());
}

MU_TEST_SUITE(test_suite) {
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
  MU_RUN_TEST(test_fb_init);
	MU_RUN_TEST(test_fb_pop);
	MU_RUN_TEST(test_fb_pop_till_empty);
	MU_RUN_TEST(test_fb_push);
}

int main(int argc, char *argv[]) {
  disk_open("/tmp/tempfsfile");
  MU_RUN_SUITE(test_vendor);
	MU_RUN_SUITE(test_suite);
  disk_close();

	MU_REPORT();
	return 0;
}
