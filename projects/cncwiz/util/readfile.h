#pragma once

#include <stdio.h>
#include <rawkit/string.h>

#ifdef _WIN32

#else
  #include <sys/errno.h>
#endif

StringList readfile_lines(const char *path) {
  StringList ret;
  if (path == NULL) {
    printf("ERROR: readfile_lines was passed a null path");
    return ret;
  }
  FILE *f = fopen(path, "r");
  if (!f) {
    printf("ERROR: can't open %s (%i)\n", path, errno);
    return ret;
  }

  char c = 0;
  String line("");

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
