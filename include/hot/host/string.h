#pragma once

#include <hot/jitjob.h>

const void *string_host_memchr(const void *a, int b, size_t c)
{
  return memchr(a, b, c);
}

const char *string_host_strrchr(const char *a, int b)
{
  return strrchr(a, b);
}

const char *string_host_strpbrk(const char *a, const char *b)
{
  return strpbrk(a, b);
}

const char *string_host_strstr(const char *a, const char *b)
{
  return strstr(a, b);
}

void host_init_string(JitJob *job) {
  job->addExport("memcpy", (void *)&memcpy);
  job->addExport("memmove", (void *)&memmove);
  job->addExport("memchr", (void *)&string_host_memchr);
  job->addExport("memcmp", (void *)&memcmp);
  job->addExport("memset", (void *)&memset);
  job->addExport("strcat", (void *)&strcat);
  job->addExport("strncat", (void *)&strncat);
  job->addExport("strchr", (void *)(char *(*)(char *, int))strchr);
  job->addExport("strrchr", (void *)&string_host_strrchr);
  job->addExport("strcmp", (void *)&strcmp);
  job->addExport("strncmp", (void *)&strncmp);
  job->addExport("strcoll", (void *)&strcoll);
  job->addExport("strcpy", (void *)&strcpy);
  job->addExport("strncpy", (void *)&strncpy);
  job->addExport("strerror", (void *)&strerror);
  job->addExport("strlen", (void *)&strlen);
  job->addExport("strspn", (void *)&strspn);
  job->addExport("strcspn", (void *)&strcspn);
  job->addExport("strpbrk", (void *)&string_host_strpbrk);
  job->addExport("strstr", (void *)&string_host_strstr);
  job->addExport("strtok", (void *)&strtok);
  job->addExport("strxfrm", (void *)&strxfrm);
}