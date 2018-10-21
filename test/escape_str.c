#include <criterion/criterion.h>
#include <assert.h>


size_t escape_str(char *in, char **out)
{
  char *writep;

  size_t buflen = 0;
  char *bufpass = in;
  char *copypass = in;

  while (*bufpass) {

    if ((((int) *bufpass >= 0x01 && (int) *bufpass <= 0x1F) || (int) *bufpass == 0x7F) && buflen > 0) {
      buflen--;
      bufpass++;
      continue;
    }

    if (*bufpass == '"')
      buflen++;

    buflen++;
    bufpass++;
  }

  if (buflen == 0) {
    *out = NULL;
    return 0;
  }

  *out = (char *) calloc(buflen, sizeof(char));
  writep = *out;
  while (*copypass) {

    if (((int) *copypass >= 0x01 && (int) *copypass <= 0x1F) || (int) *copypass == 0x7F) {
      copypass++;
      continue;
    }

    if (*copypass == '"') {
      *writep = '\\';
      writep++;
      *writep = *copypass;
      copypass++;
      writep++;
      continue;
    }

    *writep = *copypass;
    writep++;
    copypass++;
  }

  return buflen;
}

Test(string_parsing, given_empty_string_test_empty) {
  char *input = "";
  char *expected = NULL;
  char *actual_string = NULL;

  size_t actual_length = escape_str(input, &actual_string);

  cr_assert_eq(actual_length, 0);
  cr_assert_eq(actual_string, expected);
}

Test(string_parsing, given_empty_string_of_control_chars_test_result_is_empty) {
  char input[] = { (char) 1, (char) 2, (char) 3, (char) 4, (char) 5, (char) 6, (char) 7, (char) 8, (char) 9, (char) 10, (char) 11, (char) 12, (char) 13, (char) 14, (char) 15, (char) 16, (char) 17, (char) 18, (char) 19, (char) 20, (char) 21, (char) 22, (char) 23, (char) 24, (char) 25, (char) 26, (char) 27, (char) 28, (char) 29, (char) 30, (char) 31, (char) 127, '\0' };
  char *expected = NULL;
  char *actual_string = NULL;

  size_t actual_length = escape_str(input, &actual_string);

  cr_assert_eq(actual_length, 0);
  cr_assert_eq(actual_string, expected);
}

Test(string_parsing, given_string_of_control_chars_and_something_test_result_is_escaped) {
  char input[] = { (char) 1, 'V', 'E', 'R', 'S', 'I', 'O', 'N', (char) 1, '\0' };
  char *expected = "VERSION";
  char *actual_string = NULL;

  size_t actual_length = escape_str(input, &actual_string);

  cr_assert_eq(actual_length, strlen(expected));
  cr_assert_str_eq(actual_string, expected);

  free(actual_string);
}

Test(string_parsing, given_string_test_escape_characters) {
  char *input = "testing \"some function\" parsing.";
  char *expected = "testing \\\"some function\\\" parsing.";
  char *actual_string = NULL;

  size_t actual_length = escape_str(input, &actual_string);

  cr_assert_eq(actual_length, strlen(expected));
  cr_assert_str_eq(actual_string, expected);

  free(actual_string);
}
