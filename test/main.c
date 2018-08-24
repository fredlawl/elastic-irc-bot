#include <stdio.h>
#include <criterion/criterion.h>

void setup(void) {
  puts("Runs before the test");
}

void teardown(void) {
  puts("Runs after the test");
}

Test(suite_name, test_name, .init = setup, .fini = teardown) {
  cr_expect(strlen("Test") == 4, "Expected \"Test\" to have a length of 4.");
  cr_expect(strlen("Hello") == 4, "This will always fail, why did I add this?");
  cr_assert(strlen("") == 0);
}
