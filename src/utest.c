#include "http_parse.h"
#include "cli.h"
#include "strutil.h"

#include "util.h"
#include "utest.h"

int utest_run(void)
{
  int n_errors = 0;

  int (*funcs[])(void) = {
    strutil_utest,
    cli_utest,
    http_parse_utest,
  };

  for (size_t i = 0; i < N_ELEMS(funcs); i++) {
    n_errors += funcs[i]();
  }

  if (n_errors)
    fprintf(stderr, "\nUnit tests failed (%d errors)\n", n_errors);
  else
    printf("\nAll unit tests passed!\n");
  return n_errors;
}
