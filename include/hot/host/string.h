#pragma once

#include <rawkit/jit.h>

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

void host_init_string(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "memcpy", (void *)&memcpy);
  rawkit_jit_add_export(jit, "memmove", (void *)&memmove);
  rawkit_jit_add_export(jit, "memchr", (void *)&string_host_memchr);
  rawkit_jit_add_export(jit, "memcmp", (void *)&memcmp);
  rawkit_jit_add_export(jit, "memset", (void *)&memset);
  rawkit_jit_add_export(jit, "strcat", (void *)&strcat);
  rawkit_jit_add_export(jit, "strncat", (void *)&strncat);
  rawkit_jit_add_export(jit, "strchr", (void *)(char *(*)(char *, int))strchr);
  rawkit_jit_add_export(jit, "strrchr", (void *)&string_host_strrchr);
  rawkit_jit_add_export(jit, "strcmp", (void *)&strcmp);
  rawkit_jit_add_export(jit, "strncmp", (void *)&strncmp);
  rawkit_jit_add_export(jit, "strcoll", (void *)&strcoll);
  rawkit_jit_add_export(jit, "strcpy", (void *)&strcpy);
  rawkit_jit_add_export(jit, "strncpy", (void *)&strncpy);
  rawkit_jit_add_export(jit, "strerror", (void *)&strerror);
  rawkit_jit_add_export(jit, "strlen", (void *)&strlen);
  rawkit_jit_add_export(jit, "strspn", (void *)&strspn);
  rawkit_jit_add_export(jit, "strcspn", (void *)&strcspn);
  rawkit_jit_add_export(jit, "strpbrk", (void *)&string_host_strpbrk);
  rawkit_jit_add_export(jit, "strstr", (void *)&string_host_strstr);
  rawkit_jit_add_export(jit, "strtok", (void *)&strtok);
  rawkit_jit_add_export(jit, "strxfrm", (void *)&strxfrm);
  rawkit_jit_add_export(jit, "strdup", (void *)&strdup);
}