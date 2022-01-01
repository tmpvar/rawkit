#pragma once

#include <rawkit/jit.h>
#include <stdlib.h>

typedef double (*rawkit_abs)(double val);

typedef errno_t (*rawkit_wcstombs_s)(size_t *, char *, size_t, const wchar_t *, size_t);
typedef errno_t (*rawkit_mbstowcs_s)(size_t *, wchar_t *, size_t, const char *, size_t);


static void host_init_stdlib(rawkit_jit_t *jit)
{
  rawkit_jit_add_export(jit, "malloc", malloc);
  rawkit_jit_add_export(jit, "abort", abort);
  rawkit_jit_add_export(jit, "atexit", atexit);
  rawkit_jit_add_export(jit, "exit", exit);
  // rawkit_jit_add_export(jit, "abs", abs);
  rawkit_jit_add_export(jit, "atof", atof);
  rawkit_jit_add_export(jit, "atoi", atoi);
  rawkit_jit_add_export(jit, "atol", atol);
  rawkit_jit_add_export(jit, "bsearch", bsearch);
  rawkit_jit_add_export(jit, "calloc", calloc);
  // rawkit_jit_add_export(jit, "div", div);
  rawkit_jit_add_export(jit, "free", free);
  rawkit_jit_add_export(jit, "getenv", getenv);
  rawkit_jit_add_export(jit, "labs", labs);
  rawkit_jit_add_export(jit, "ldiv", ldiv);
  rawkit_jit_add_export(jit, "malloc", malloc);
  rawkit_jit_add_export(jit, "mblen", mblen);
  rawkit_jit_add_export(jit, "mbstowcs", mbstowcs);
  rawkit_jit_add_export(jit, "mbtowc", mbtowc);
  rawkit_jit_add_export(jit, "qsort", qsort);
  rawkit_jit_add_export(jit, "rand", rand);
  rawkit_jit_add_export(jit, "realloc", realloc);
  rawkit_jit_add_export(jit, "srand", srand);
  rawkit_jit_add_export(jit, "strtod", strtod);
  rawkit_jit_add_export(jit, "strtol", strtol);
  rawkit_jit_add_export(jit, "strtoul", strtoul);
  rawkit_jit_add_export(jit, "system", system);
  rawkit_jit_add_export(jit, "wcstombs", wcstombs);

  // #ifdef _WIN32
  //   rawkit_jit_add_export(jit, "wcstombs_s", ((rawkit_wcstombs_s)wcstombs_s));
  //   rawkit_jit_add_export(jit, "mbstowcs_s", ((rawkit_mbstowcs_s)mbstowcs_s));
  // #endif

  rawkit_jit_add_export(jit, "wctomb", wctomb);
}
