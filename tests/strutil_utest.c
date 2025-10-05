static int trim_utest(void)
{
  int         n_successes = 0;
  int         n_failures = 0;
  struct utest {
    char *buf;
    char *exp_result;
    int   exp_retval;
  } utests[] = {
    {
      .buf = NULL,
      .exp_retval = -1,
    },
    {
      .buf = "foo",
      .exp_result = "foo",
    },
    {
      .buf = "",
      .exp_result = "",
    },
    {
      .buf = "  ",
      .exp_result = "",
    },
    {
      .buf = " foo  ",
      .exp_result = "foo",
    },
    {
      .buf = "foo  ",
      .exp_result = "foo",
    },
    {
      .buf = "  foo",
      .exp_result = "foo",
    },
    {
      .buf = " foo bar  ",
      .exp_result = "foo bar",
    },
  };

  for (size_t i = 0; i < N_ELEMS(utests); i++) {
    struct utest *u = utests + i;
    int           retval = -1;
    char         *result = NULL;

    retval = trim(u->buf, &result);
    if (u->exp_result)
      u->exp_retval = (int) strlen(u->exp_result);
    if (retval != u->exp_retval) {
      logger("input '%s', expected retval %d, got %d", u->buf, u->exp_retval, retval);
      n_failures++;
    } else if (retval >= 0) {
      if (strcmp(result, u->exp_result)) {
        logger("input '%s', expected key %s, got %s", u->buf, u->exp_result, result);
        n_failures++;
      }
      free(result);
      n_successes++;
    }
  }

  logger("Successes: %d, Failures: %d", n_successes, n_failures);
  return n_failures;
}

int strutil_utest(void)
{
  int n_errors = 0;

  int (*funcs[])(void) = {
    trim_utest,
  };

  for (size_t i = 0; i < N_ELEMS(funcs); i++) {
    n_errors += funcs[i]();
  }

  return n_errors;
}
