#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *create_tmpfile() {
  char *input = tmpnam(NULL);
  size_t len = strlen(input);
  char *output = (char *)calloc(len+1, 1);
  memcpy(output, input, len);
  return output;
}

static int write_file(const char *filename, const char *contents) {
  FILE *f = fopen(filename, "w+");
  if (!f) {
    return 0;
  }

  size_t len = strlen(contents);
  size_t w = fwrite((void *)contents, 1, len, f);
  fclose(f);
  return w == len ? 1 : 0;
}