#pragma once

#include <rawkit/jit.h>

static const void *string_host_memchr(const void *a, int b, size_t c)
{
  return memchr(a, b, c);
}

static const char *string_host_strrchr(const char *a, int b)
{
  return strrchr(a, b);
}

static const char *string_host_strpbrk(const char *a, const char *b)
{
  return strpbrk(a, b);
}

static const char *string_host_strstr(const char *a, const char *b)
{
  return strstr(a, b);
}

static void host_init_string(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "memcpy", memcpy);
  rawkit_jit_add_export(jit, "memmove", memmove);
  rawkit_jit_add_export(jit, "memchr", string_host_memchr);
  rawkit_jit_add_export(jit, "memcmp", memcmp);
  rawkit_jit_add_export(jit, "memset", memset);
  rawkit_jit_add_export(jit, "strcat", strcat);
  rawkit_jit_add_export(jit, "strncat", strncat);
  rawkit_jit_add_export(jit, "strchr", string_host_strstr);
  rawkit_jit_add_export(jit, "strrchr", string_host_strrchr);
  rawkit_jit_add_export(jit, "strcmp", strcmp);
  rawkit_jit_add_export(jit, "strncmp", strncmp);
  rawkit_jit_add_export(jit, "strcoll", strcoll);
  rawkit_jit_add_export(jit, "strcpy", strcpy);
  rawkit_jit_add_export(jit, "strncpy", strncpy);
  rawkit_jit_add_export(jit, "strerror", strerror);
  rawkit_jit_add_export(jit, "strlen", strlen);
  rawkit_jit_add_export(jit, "strspn", strspn);
  rawkit_jit_add_export(jit, "strcspn", strcspn);
  rawkit_jit_add_export(jit, "strpbrk", string_host_strpbrk);
  rawkit_jit_add_export(jit, "strstr", string_host_strstr);
  rawkit_jit_add_export(jit, "strtok", strtok);
  rawkit_jit_add_export(jit, "strxfrm", strxfrm);
  rawkit_jit_add_export(jit, "strdup", strdup);
}