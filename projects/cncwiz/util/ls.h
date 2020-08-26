#pragma once

#include <rawkit/string.h>

#include <dirent.h>

StringList list_directory(const char *path) {
  StringList ret;
  DIR *dir = opendir(path);
  if (dir == NULL) {
    return ret;
  }

  struct dirent *ent;
  while ((ent = readdir (dir)) != NULL) {
    switch (ent->d_type) {
    case DT_REG:
      if (ent->d_name[0] == '.') {
        break;
      }

      printf ("%s\n", ent->d_name);
      break;

    case DT_DIR:
      printf ("%s/\n", ent->d_name);
      break;

    case DT_LNK:
      printf ("%s@\n", ent->d_name);
      break;

    default:
      printf ("%s*\n", ent->d_name);
    }
  }

  closedir (dir);
  return ret;
}