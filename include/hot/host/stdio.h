#include <rawkit/jit.h>
#include <stdio.h>

#ifdef __APPLE__
#include <dirent.h>
#endif

#ifdef __cplusplus
  extern "C" {
#endif


void host_init_stdio(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "fclose", fclose);
  rawkit_jit_add_export(jit, "clearerr", clearerr);
  rawkit_jit_add_export(jit, "feof", feof);
  rawkit_jit_add_export(jit, "ferror", ferror);
  rawkit_jit_add_export(jit, "fflush", fflush);
  rawkit_jit_add_export(jit, "fgetpos", fgetpos);
  rawkit_jit_add_export(jit, "fopen", fopen);
  #ifdef _WIN32
    rawkit_jit_add_export(jit, "fopen_s", fopen_s);
    rawkit_jit_add_export(jit, "vsnprintf", vsnprintf);
  #endif
  rawkit_jit_add_export(jit, "fread", fread);
  rawkit_jit_add_export(jit, "freopen", freopen);
  rawkit_jit_add_export(jit, "fseek", fseek);
  rawkit_jit_add_export(jit, "fsetpos", fsetpos);
  rawkit_jit_add_export(jit, "ftell", ftell);
  rawkit_jit_add_export(jit, "fwrite", fwrite);
  // rawkit_jit_add_export(jit, "remove", remove);
  rawkit_jit_add_export(jit, "rename", rename);
  rawkit_jit_add_export(jit, "rewind", rewind);
  rawkit_jit_add_export(jit, "setbuf", setbuf);
  rawkit_jit_add_export(jit, "setvbuf", setvbuf);
  rawkit_jit_add_export(jit, "tmpfile", tmpfile);
  rawkit_jit_add_export(jit, "tmpnam", tmpnam);

  #ifdef WIN32
  rawkit_jit_add_export(jit, "__stdio_common_vfprintf", __stdio_common_vfprintf);
  rawkit_jit_add_export(jit, "__stdio_common_vsprintf", __stdio_common_vsprintf);
  rawkit_jit_add_export(jit, "__stdio_common_vsprintf_s", __stdio_common_vsprintf_s);
  #endif

  rawkit_jit_add_export(jit, "fprintf", fprintf);
  rawkit_jit_add_export(jit, "printf", printf);
  rawkit_jit_add_export(jit, "sprintf", sprintf);

  // #ifdef __APPLE__

  // rawkit_jit_add_export(jit, "_readdir$INODE64", readdir);
  // rawkit_jit_add_export(jit, "_opendir$INODE64", opendir);
  // rawkit_jit_add_export(jit, "_sprintf", sprintf);
  // // rawkit_jit_add_export(jit, "___bzero", ___bzero);
  // rawkit_jit_add_export(jit, "_vsnprintf", vsnprintf);
  // rawkit_jit_add_export(jit, "_snprintf", snprintf);
  // rawkit_jit_add_export(jit, "_closedir", closedir);

  // #endif

  rawkit_jit_add_export(jit, "vfprintf", vfprintf);
  rawkit_jit_add_export(jit, "vprintf", vprintf);
  rawkit_jit_add_export(jit, "vsprintf", vsprintf);
  rawkit_jit_add_export(jit, "fscanf", fscanf);
  rawkit_jit_add_export(jit, "scanf", scanf);
  rawkit_jit_add_export(jit, "sscanf", sscanf);
  rawkit_jit_add_export(jit, "fgetc", fgetc);
  rawkit_jit_add_export(jit, "fgets", fgets);
  rawkit_jit_add_export(jit, "fputc", fputc);
  rawkit_jit_add_export(jit, "fputs", fputs);
  rawkit_jit_add_export(jit, "getc", getc);
  rawkit_jit_add_export(jit, "getchar", getchar);
  // rawkit_jit_add_export(jit, "gets", gets);
  rawkit_jit_add_export(jit, "putc", putc);
  rawkit_jit_add_export(jit, "putchar", putchar);
  rawkit_jit_add_export(jit, "puts", puts);
  rawkit_jit_add_export(jit, "ungetc", ungetc);
  rawkit_jit_add_export(jit, "perror", perror);
}

#ifdef __cplusplus
  }
#endif