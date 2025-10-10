static int cli_parse_target_utest(void)
{
  int         n_successes = 0;
  int         n_failures = 0;
  struct utest {
    char    *buf;
    int      exp_is_tls;
    char    *exp_hostname;
    uint16_t exp_port;
    char    *exp_path;
    int      exp_retval;
  } utests[] = {
    {
      .buf = NULL,
      .exp_retval = -1,
    },
    {
      .buf = "",
      .exp_retval = -1,
    },
    {
      .buf = "http://",
      .exp_retval = -1,
    },
    {
      .buf = "https://",
      .exp_retval = -1,
    },
    {
      .buf = "http:",
      .exp_retval = -1,
    },
    {
      .buf = "http:/",
      .exp_retval = -1,
    },
    {
      .buf = "host:/path",
      .exp_retval = -1,
    },
    {
      .buf = "https://host:/path",
      .exp_retval = -1,
    },
    {
      .buf = "https://:80/path",
      .exp_retval = -1,
    },
    {
      .buf = "http://hostname:port/path",
      .exp_retval = -1,
    },
    {
      // first complete and valid target
      .buf = "https://hostname:80/path",
      .exp_retval = 0,
      .exp_is_tls = 1,
      .exp_hostname = "hostname",
      .exp_port = 80,
      .exp_path = "/path",
    },
    {
      .buf = "https://hostname/path",
      .exp_retval = 0,
      .exp_is_tls = 1,
      .exp_hostname = "hostname",
      .exp_port = 443,
      .exp_path = "/path",
    },
    {
      .buf = "https://hostname:/path",
      .exp_retval = -1,
    },
    {
      .buf = "https://hostname:443/",
      .exp_retval = 0,
      .exp_is_tls = 1,
      .exp_hostname = "hostname",
      .exp_port = 443,
      .exp_path = "/",
    },
    {
      .buf = "hostname:80/path",
      .exp_retval = 0,
      .exp_is_tls = 0,
      .exp_hostname = "hostname",
      .exp_port = 80,
      .exp_path = "/path",
    },
    {
      .buf = "hostname:80",
      .exp_retval = 0,
      .exp_is_tls = 0,
      .exp_hostname = "hostname",
      .exp_port = 80,
      .exp_path = NULL,
    },
    {
      .buf = "hostname",
      .exp_retval = 0,
      .exp_is_tls = 0,
      .exp_hostname = "hostname",
      .exp_port = 80,
      .exp_path = NULL,
    },
    {
      .buf = "hostname/",
      .exp_retval = 0,
      .exp_is_tls = 0,
      .exp_hostname = "hostname",
      .exp_port = 80,
      .exp_path = "/",
    },
  };

  for (size_t i = 0; i < N_ELEMS(utests); i++) {
    struct utest *u = utests + i;
    int           retval = -1;
    int           is_tls = 0;
    char         *hostname = NULL;
    uint16_t      port = 0;
    char         *path = NULL;
    
    retval = cli_parse_target(u->buf, &is_tls, &hostname, &port, &path);
    if (retval != u->exp_retval) {
      logger("input '%s', expected retval %d, got %d", u->buf, u->exp_retval, retval);
      n_failures++;
      free(hostname);
      free(path);
      continue;
    }

    if (u->exp_retval < 0) {
      continue;
    }

    if (is_tls != u->exp_is_tls) {
      logger("input '%s', expected is_tls %d, got %d", u->buf, u->exp_is_tls, is_tls);
      n_failures++;
    } else if (u->exp_hostname && strcmp(hostname, u->exp_hostname)) {
      logger("input '%s', expected hostname %s, got %s", u->buf, u->exp_hostname, hostname);
      n_failures++;
    } else if (u->exp_port != port) {
      logger("input '%s', expected port %"PRIu16", got %"PRIu16, u->buf, u->exp_port, port);
      n_failures++;
    } else if (u->exp_path && strcmp(path, u->exp_path)) {
      logger("input '%s', expected path %s, got %s", u->buf, u->exp_path, path);
      n_failures++;
    } else {
      n_successes++;
    }

    free(hostname);
    free(path);
  }

  logger("Successes: %d, Failures: %d", n_successes, n_failures);
  return n_failures;  
}

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
    cli_parse_target_utest,
  };

  for (size_t i = 0; i < N_ELEMS(funcs); i++) {
    n_errors += funcs[i]();
  }

  return n_errors;
}

