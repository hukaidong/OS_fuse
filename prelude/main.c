#include "block.h"
#include "free_block.h"
#include "minunit.h"


void test_setup(void) {
}

void test_teardown(void) {
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

MU_TEST_SUITE(test_suite) {
  disk_open("/tmp/tempfsfile");
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
  MU_RUN_TEST(test_fb_init);
	MU_RUN_TEST(test_fb_pop);
  disk_close();
}

int main(int argc, char *argv[]) {
	MU_RUN_SUITE(test_suite);
	MU_REPORT();
	return 0;
}

