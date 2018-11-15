#include <stdlib.h>

#include "str_helpers.h"

size_t sanitize_str(char *in, char **out) {
  char *writep;

  size_t string_len = 0;
  char *buf_pass = in;
  char *copy_pass = in;

  *out = NULL;

  while (*buf_pass) {

    if ((((int) *buf_pass >= 0x00 && (int) *buf_pass <= 0x1F) || (int) *buf_pass == 0x7F)) {
      buf_pass++;
      continue;
    }

    if (*buf_pass == '"' || *buf_pass == '\\')
      string_len++;

    string_len++;
    buf_pass++;
  }

  if (string_len == 0)
    return 0;

  *out = (char *) calloc(string_len + 1, sizeof(char));
  writep = *out;

  if (*out == NULL)
    return 0;

  while (*copy_pass) {

    if (((int) *copy_pass >= 0x00 && (int) *copy_pass <= 0x1F) || (int) *copy_pass == 0x7F) {
      copy_pass++;
      continue;
    }

    if (*copy_pass == '"' || *copy_pass == '\\')
      *writep++ = '\\';

    *writep++ = *copy_pass++;
  }

  return string_len;
}
