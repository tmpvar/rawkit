#include <rawkit/jit.h>
#include <stdlib.h>

typedef double (*rawkit_abs)(double val);

typedef errno_t (*rawkit_wcstombs_s)(size_t *, char *, size_t, const wchar_t *, size_t);
typedef errno_t (*rawkit_mbstowcs_s)(size_t *, wchar_t *, size_t, const char *, size_t);


void host_init_stdlib(rawkit_jit_t *jit)
{
  rawkit_jit_add_export(jit, "malloc", (void *)&malloc);
  rawkit_jit_add_export(jit, "abort", (void *)&abort);
  rawkit_jit_add_export(jit, "atexit", (void *)&atexit);
  rawkit_jit_add_export(jit, "exit", (void *)&exit);
  // rawkit_jit_add_export(jit, "abs", (void *)abs);
  rawkit_jit_add_export(jit, "atof", (void *)&atof);
  rawkit_jit_add_export(jit, "atoi", (void *)&atoi);
  rawkit_jit_add_export(jit, "atol", (void *)&atol);
  rawkit_jit_add_export(jit, "bsearch", (void *)&bsearch);
  rawkit_jit_add_export(jit, "calloc", (void *)&calloc);
  // rawkit_jit_add_export(jit, "div", (void *)&div);
  rawkit_jit_add_export(jit, "free", (void *)&free);
  rawkit_jit_add_export(jit, "getenv", (void *)&getenv);
  rawkit_jit_add_export(jit, "labs", (void *)&labs);
  rawkit_jit_add_export(jit, "ldiv", (void *)&ldiv);
  rawkit_jit_add_export(jit, "malloc", (void *)&malloc);
  rawkit_jit_add_export(jit, "mblen", (void *)&mblen);
  rawkit_jit_add_export(jit, "mbstowcs", (void *)&mbstowcs);
  rawkit_jit_add_export(jit, "mbtowc", (void *)&mbtowc);
  rawkit_jit_add_export(jit, "qsort", (void *)&qsort);
  rawkit_jit_add_export(jit, "rand", (void *)&rand);
  rawkit_jit_add_export(jit, "realloc", (void *)&realloc);
  rawkit_jit_add_export(jit, "srand", (void *)&srand);
  rawkit_jit_add_export(jit, "strtod", (void *)&strtod);
  rawkit_jit_add_export(jit, "strtol", (void *)&strtol);
  rawkit_jit_add_export(jit, "strtoul", (void *)&strtoul);
  rawkit_jit_add_export(jit, "system", (void *)&system);
  rawkit_jit_add_export(jit, "wcstombs", (void *)&wcstombs);

  #ifdef _WIN32
    rawkit_jit_add_export(jit, "wcstombs_s", (void *)&((rawkit_wcstombs_s)wcstombs_s));
    rawkit_jit_add_export(jit, "mbstowcs_s", (void *)&((rawkit_mbstowcs_s)mbstowcs_s));
  #endif

  rawkit_jit_add_export(jit, "wctomb", (void *)&wctomb);
}
