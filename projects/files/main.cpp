#include <stdio.h>
#include <rawkit/file.h>

void setup() {}

void loop() {
  const rawkit_file_t *f = rawkit_file("file.txt");
  if (f) {
    if (f->error) {
      printf("File failed to load (%i)\n", f->error);
      return;
    }

    printf("File contents (%llu):\n%s\n", f->len, (const char *)f->data);
  }
}