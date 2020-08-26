#include <hot/jitjob.h>
#include <stdio.h>

void host_init_stdio(JitJob *job) {
  job->addExport("fclose", (void *)&fclose);
  job->addExport("clearerr", (void *)&clearerr);
  job->addExport("feof", (void *)&feof);
  job->addExport("ferror", (void *)&ferror);
  job->addExport("fflush", (void *)&fflush);
  job->addExport("fgetpos", (void *)&fgetpos);
  job->addExport("fopen", (void *)&fopen);
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

  job->addExport("__stdio_common_vfprintf", (void *)&__stdio_common_vfprintf);
  job->addExport("__stdio_common_vsprintf", (void *)&__stdio_common_vsprintf);
  job->addExport("__stdio_common_vsprintf_s", (void *)&__stdio_common_vsprintf_s);

  job->addExport("fprintf", (void *)&fprintf);
  job->addExport("printf", (void *)&printf);
  job->addExport("sprintf", (void *)&sprintf);
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
