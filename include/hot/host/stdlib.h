#include <hot/jitjob.h>
#include <stdlib.h>

void host_init_stdlib(JitJob *job)
{
  job->addExport("malloc", (void *)&malloc);
  job->addExport("abort", (void *)&abort);
  job->addExport("atexit", (void *)&atexit);
  job->addExport("exit", (void *)&exit);
  // job->addExport("abs", (void *)abs);
  job->addExport("atof", (void *)&atof);
  job->addExport("atoi", (void *)&atoi);
  job->addExport("atol", (void *)&atol);
  job->addExport("bsearch", (void *)&bsearch);
  job->addExport("calloc", (void *)&calloc);
  // job->addExport("div", (void *)&div);
  job->addExport("free", (void *)&free);
  job->addExport("getenv", (void *)&getenv);
  job->addExport("labs", (void *)&labs);
  job->addExport("ldiv", (void *)&ldiv);
  job->addExport("malloc", (void *)&malloc);
  job->addExport("mblen", (void *)&mblen);
  job->addExport("mbstowcs", (void *)&mbstowcs);
  job->addExport("mbtowc", (void *)&mbtowc);
  job->addExport("qsort", (void *)&qsort);
  job->addExport("rand", (void *)&rand);
  job->addExport("realloc", (void *)&realloc);
  job->addExport("srand", (void *)&srand);
  job->addExport("strtod", (void *)&strtod);
  job->addExport("strtol", (void *)&strtol);
  job->addExport("strtoul", (void *)&strtoul);
  job->addExport("system", (void *)&system);
  job->addExport("wcstombs", (void *)&wcstombs);
  job->addExport("wctomb", (void *)&wctomb);
}
