#include <rawkit/jit.h>
#include <stdio.h>

#ifdef __APPLE__
#include <dirent.h>
#endif

void host_init_stdio(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "fclose", (void *)&fclose);
  rawkit_jit_add_export(jit, "clearerr", (void *)&clearerr);
  rawkit_jit_add_export(jit, "feof", (void *)&feof);
  rawkit_jit_add_export(jit, "ferror", (void *)&ferror);
  rawkit_jit_add_export(jit, "fflush", (void *)&fflush);
  rawkit_jit_add_export(jit, "fgetpos", (void *)&fgetpos);
  rawkit_jit_add_export(jit, "fopen", (void *)&fopen);
  #ifdef WIN32
  rawkit_jit_add_export(jit, "fopen_s", (void *)&fopen_s);
  #endif
  rawkit_jit_add_export(jit, "fread", (void *)&fread);
  rawkit_jit_add_export(jit, "freopen", (void *)&freopen);
  rawkit_jit_add_export(jit, "fseek", (void *)&fseek);
  rawkit_jit_add_export(jit, "fsetpos", (void *)&fsetpos);
  rawkit_jit_add_export(jit, "ftell", (void *)&ftell);
  rawkit_jit_add_export(jit, "fwrite", (void *)&fwrite);
  // rawkit_jit_add_export(jit, "remove", (void *)&remove);
  rawkit_jit_add_export(jit, "rename", (void *)&rename);
  rawkit_jit_add_export(jit, "rewind", (void *)&rewind);
  rawkit_jit_add_export(jit, "setbuf", (void *)&setbuf);
  rawkit_jit_add_export(jit, "setvbuf", (void *)&setvbuf);
  rawkit_jit_add_export(jit, "tmpfile", (void *)&tmpfile);
  rawkit_jit_add_export(jit, "tmpnam", (void *)&tmpnam);

  #ifdef WIN32
  rawkit_jit_add_export(jit, "__stdio_common_vfprintf", (void *)&__stdio_common_vfprintf);
  rawkit_jit_add_export(jit, "__stdio_common_vsprintf", (void *)&__stdio_common_vsprintf);
  rawkit_jit_add_export(jit, "__stdio_common_vsprintf_s", (void *)&__stdio_common_vsprintf_s);
  #endif

  rawkit_jit_add_export(jit, "fprintf", (void *)&fprintf);
  rawkit_jit_add_export(jit, "printf", (void *)&printf);
  rawkit_jit_add_export(jit, "sprintf", (void *)&sprintf);

  // #ifdef __APPLE__

  // rawkit_jit_add_export(jit, "_readdir$INODE64", (void *)&readdir);
  // rawkit_jit_add_export(jit, "_opendir$INODE64", (void *)&opendir);
  // rawkit_jit_add_export(jit, "_sprintf", (void *)&sprintf);
  // // rawkit_jit_add_export(jit, "___bzero", (void *)&___bzero);
  // rawkit_jit_add_export(jit, "_vsnprintf", (void *)&vsnprintf);
  // rawkit_jit_add_export(jit, "_snprintf", (void *)&snprintf);
  // rawkit_jit_add_export(jit, "_closedir", (void *)&closedir);

  // #endif

  rawkit_jit_add_export(jit, "vfprintf", (void *)&vfprintf);
  rawkit_jit_add_export(jit, "vprintf", (void *)&vprintf);
  rawkit_jit_add_export(jit, "vsprintf", (void *)&vsprintf);
  rawkit_jit_add_export(jit, "fscanf", (void *)&fscanf);
  rawkit_jit_add_export(jit, "scanf", (void *)&scanf);
  rawkit_jit_add_export(jit, "sscanf", (void *)&sscanf);
  rawkit_jit_add_export(jit, "fgetc", (void *)&fgetc);
  rawkit_jit_add_export(jit, "fgets", (void *)&fgets);
  rawkit_jit_add_export(jit, "fputc", (void *)&fputc);
  rawkit_jit_add_export(jit, "fputs", (void *)&fputs);
  rawkit_jit_add_export(jit, "getc", (void *)&getc);
  rawkit_jit_add_export(jit, "getchar", (void *)&getchar);
  // rawkit_jit_add_export(jit, "gets", (void *)&gets);
  rawkit_jit_add_export(jit, "putc", (void *)&putc);
  rawkit_jit_add_export(jit, "putchar", (void *)&putchar);
  rawkit_jit_add_export(jit, "puts", (void *)&puts);
  rawkit_jit_add_export(jit, "ungetc", (void *)&ungetc);
  rawkit_jit_add_export(jit, "perror", (void *)&perror);
}
