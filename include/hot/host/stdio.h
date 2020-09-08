#include <hot/jitjob.h>
#include <stdio.h>

#ifdef __APPLE__
#include <dirent.h>
#endif

void host_init_stdio(JitJob *job) {
  job->addExport("fclose", (void *)&fclose);
  job->addExport("clearerr", (void *)&clearerr);
  job->addExport("feof", (void *)&feof);
  job->addExport("ferror", (void *)&ferror);
  job->addExport("fflush", (void *)&fflush);
  job->addExport("fgetpos", (void *)&fgetpos);
  job->addExport("fopen", (void *)&fopen);
  #ifdef WIN32
  job->addExport("fopen_s", (void *)&fopen_s);
  #endif
  job->addExport("fread", (void *)&fread);
  job->addExport("freopen", (void *)&freopen);
  job->addExport("fseek", (void *)&fseek);
  job->addExport("fsetpos", (void *)&fsetpos);
  job->addExport("ftell", (void *)&ftell);
  job->addExport("fwrite", (void *)&fwrite);
  // job->addExport("remove", (void *)&remove);
  job->addExport("rename", (void *)&rename);
  job->addExport("rewind", (void *)&rewind);
  job->addExport("setbuf", (void *)&setbuf);
  job->addExport("setvbuf", (void *)&setvbuf);
  job->addExport("tmpfile", (void *)&tmpfile);
  job->addExport("tmpnam", (void *)&tmpnam);

  #ifdef WIN32
  job->addExport("__stdio_common_vfprintf", (void *)&__stdio_common_vfprintf);
  job->addExport("__stdio_common_vsprintf", (void *)&__stdio_common_vsprintf);
  job->addExport("__stdio_common_vsprintf_s", (void *)&__stdio_common_vsprintf_s);
  #endif

  job->addExport("fprintf", (void *)&fprintf);
  job->addExport("printf", (void *)&printf);
  job->addExport("sprintf", (void *)&sprintf);

  // #ifdef __APPLE__

  // job->addExport("_readdir$INODE64", (void *)&readdir);
  // job->addExport("_opendir$INODE64", (void *)&opendir);
  // job->addExport("_sprintf", (void *)&sprintf);
  // // job->addExport("___bzero", (void *)&___bzero);
  // job->addExport("_vsnprintf", (void *)&vsnprintf);
  // job->addExport("_snprintf", (void *)&snprintf);
  // job->addExport("_closedir", (void *)&closedir);

  // #endif

  job->addExport("vfprintf", (void *)&vfprintf);
  job->addExport("vprintf", (void *)&vprintf);
  job->addExport("vsprintf", (void *)&vsprintf);
  job->addExport("fscanf", (void *)&fscanf);
  job->addExport("scanf", (void *)&scanf);
  job->addExport("sscanf", (void *)&sscanf);
  job->addExport("fgetc", (void *)&fgetc);
  job->addExport("fgets", (void *)&fgets);
  job->addExport("fputc", (void *)&fputc);
  job->addExport("fputs", (void *)&fputs);
  job->addExport("getc", (void *)&getc);
  job->addExport("getchar", (void *)&getchar);
  // job->addExport("gets", (void *)&gets);
  job->addExport("putc", (void *)&putc);
  job->addExport("putchar", (void *)&putchar);
  job->addExport("puts", (void *)&puts);
  job->addExport("ungetc", (void *)&ungetc);
  job->addExport("perror", (void *)&perror);
}
