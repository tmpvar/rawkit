#pragma once

#include <stdio.h>
#include <rawkit/string.h>

StringList readfile_lines(const char *path) {
  StringList ret;
  if (path == NULL) {
    printf("ERROR: readfile_lines was passed a null path");
    return ret;
  }
  FILE *f = NULL;
  errno_t err;
  if ((err = fopen_s(&f, path, "r")) != 0) {
    printf("ERROR: can't open %s\n", path);
    return ret;
  }

  char c = 0;
  String line;

  while (!feof(f) && fread(&c, sizeof(char), 1, f)) {
    if (c != '\n') {
      line.append_char(c);
      continue;
    }

    ret.push(&line);
    line.clear();
  }
  fclose(f);
  return ret;
}
