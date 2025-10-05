static int cli_parse_header_utest(void)
{
  int         n_successes = 0;
  int         n_failures = 0;
  struct utest {
    char *buf;
    char *exp_key;
    char *exp_value;
    int   exp_retval;
  } utests[] = {
    {
      .buf = NULL,
      .exp_retval = -1,
    },
    {
      .buf = "foo",
      .exp_retval = -1,
    },
    {
      .buf = "",
      .exp_retval = -1,
    },
    {
      .buf = ":",
      .exp_retval = -1,
    },
    {
      .buf = "   :",
      .exp_retval = -1,
    },
    {
      .buf = ":   ",
      .exp_retval = -1,
    },
    {
      .buf = "  :  ",
      .exp_retval = -1,
    },
    {
      .buf = "key:",
      .exp_retval = -1,
    },
    {
      .buf = "key  : ",
      .exp_retval = -1,
    },
    {
      .buf = "  key:  ",
      .exp_retval = -1,
    },
    {
      .buf = ": value",
      .exp_retval = -1,
    },
    {
      .buf = "key:value",
      .exp_retval = 0,
      .exp_key = "key",
      .exp_value = "value",
    },
    {
      .buf = "  key :    value  ",
      .exp_retval = 0,
      .exp_key = "key",
      .exp_value = "value",
    },
    {
      .buf = "  key : 'value1 value2'  ",
      .exp_retval = 0,
      .exp_key = "key",
      .exp_value = "'value1 value2'",
    },
  };

  for (size_t i = 0; i < N_ELEMS(utests); i++) {
    struct utest *u = utests + i;
    int           retval = -1;
    char         *key = NULL;
    char         *value = NULL;

    retval = cli_parse_header(u->buf, &key, &value);
    if (retval != u->exp_retval) {
      logger("input '%s', expected retval %d, got %d", u->buf, u->exp_retval, retval);
      n_failures++;
      continue;
    } else if (retval == 0) {
      if (strcmp(key, u->exp_key)) {
        logger("input '%s', expected key %s, got %s", u->buf, u->exp_key, key);
        n_failures++;
      }
      if (strcmp(value, u->exp_value)) {
        logger("input '%s', expected value %s, got %s", u->buf, u->exp_value, value);
        n_failures++;
      }
      free(key);
      free(value);
      n_successes++;
    }
  }

  logger("Successes: %d, Failures: %d", n_successes, n_failures);
  return n_failures;
}

int cli_utest(void)
{
  int n_errors = 0;

  int (*funcs[])(void) = {
    cli_parse_header_utest,
  };

  for (size_t i = 0; i < N_ELEMS(funcs); i++) {
    n_errors += funcs[i]();
  }

  return n_errors;
}

