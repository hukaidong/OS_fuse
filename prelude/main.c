#include "minunit.h"
#include "free_block.h"


void test_setup(void) {
}

void test_teardown(void) {
}

MU_TEST(test_fb_init) {
  free_block_init();
}

MU_TEST_SUITE(test_suite) {
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
	MU_RUN_TEST(test_fb_init);
}

int main(int argc, char *argv[]) {
	MU_RUN_SUITE(test_suite);
	MU_REPORT();
	return 0;
}

