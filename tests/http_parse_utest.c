static int http_parse_headers_utest(void)
{
  int         n_successes = 0;
  int         n_failures = 0;
  struct utest {
    char   *buf;
    size_t  buf_size;
    int     exp_code;
    int     exp_retval;
  } utests[] = {

    {
      .buf = NULL,
      .buf_size = 0,
      .exp_retval = -1,
    },
    {
      .buf = "foo",
      .exp_code = -1,
      .exp_retval = -1,
    },
    {
      .buf = "",
      .exp_code = -1,
      .exp_retval = -1,
    },
    {
      .buf = "foo\r\n\r\nbar",
      .exp_code = -1,
      .exp_retval = -1,
    },
    {
      .buf = "HTTP/1.1 200 SUCCESS\r\n",
      .exp_retval = 0,
      .exp_code = 200,
    },
    {
      .buf = "HTTP/1.2 301 SUCCESS\r\n",
      .exp_retval = 0,
      .exp_code = 301,
    },
    {
      .buf = "FOO/1.1 301 SUCCESS\r\n",
      .exp_retval = -1,
      .exp_code = 0,
    },
  };

  for (size_t i = 0; i < N_ELEMS(utests); i++) {
    struct utest *u = utests + i;
    int           retval = -1;
    int           code = -1;

    if (u->buf)
      u->buf_size = strlen(u->buf) + 1;

    retval = http_parse_code((unsigned char *) u->buf, u->buf_size, &code);
    if (retval != u->exp_retval) {
      logger("input '%s', expected retval %d, got %d", u->buf, u->exp_retval, retval);
      n_failures++;
      continue;
    }
    if (retval == 0 && code != u->exp_code) {
      logger("input '%s', expected http code %d, got %d", u->buf, u->exp_code, code);
      n_failures++;
    }
    n_successes++;
  }

  logger("Successes: %d, Failures: %d", n_successes, n_failures);
  return n_failures;
}

static int http_parse_code_utest(void)
{
  int         n_successes = 0;
  int         n_failures = 0;
  struct utest {
    char   *buf;
    size_t  buf_size;
    int     exp_code;
    int     exp_retval;
  } utests[] = {

    {
      .buf = NULL,
      .buf_size = 0,
      .exp_retval = -1,
    },
    {
      .buf = "foo",
      .exp_code = -1,
      .exp_retval = -1,
    },
    {
      .buf = "",
      .exp_code = -1,
      .exp_retval = -1,
    },
    {
      .buf = "foo\r\n\r\nbar",
      .exp_code = -1,
      .exp_retval = -1,
    },
    {
      .buf = "HTTP/1.1 200 SUCCESS\r\n",
      .exp_retval = 0,
      .exp_code = 200,
    },
    {
      .buf = "HTTP/1.2 301 SUCCESS\r\n",
      .exp_retval = 0,
      .exp_code = 301,
    },
    {
      .buf = "FOO/1.1 301 SUCCESS\r\n",
      .exp_retval = -1,
      .exp_code = 0,
    },
  };

  for (size_t i = 0; i < N_ELEMS(utests); i++) {
    struct utest *u = utests + i;
    int           retval = -1;
    int           code = -1;

    if (u->buf)
      u->buf_size = strlen(u->buf) + 1;

    retval = http_parse_code((unsigned char *) u->buf, u->buf_size, &code);
    if (retval != u->exp_retval) {
      logger("input '%s', expected retval %d, got %d", u->buf, u->exp_retval, retval);
      n_failures++;
      continue;
    }
    if (retval == 0 && code != u->exp_code) {
      logger("input '%s', expected http code %d, got %d", u->buf, u->exp_code, code);
      n_failures++;
    }
    n_successes++;
  }

  logger("Successes: %d, Failures: %d", n_successes, n_failures);
  return n_failures;
}


static int http_parse_body_utest(void)
{
  int         n_successes = 0;
  int         n_failures = 0;
  struct utest {
    char   *buf;
    size_t  buf_size;
    int     exp_retval;
    char   *exp_body;
    size_t  exp_body_size;
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
      .buf = "foo\r\n\r\nbar",
      .exp_retval = 0,
      .exp_body = "bar",
      .exp_body_size = sizeof "bar", // Because we allocate enough room for trailing \0
    },
    {
      .buf = "HTTP1/1 200 SUCCESS\r\nHost: foo\r\n\r\nbar",
      .exp_retval = 0,
      .exp_body = "bar",
      .exp_body_size = sizeof "bar", // Because we allocate enough room for trailing \0
    },
    {
      .buf = "HTTP1/1 200 SUCCESS\r\nHost: foo\r\nDate: whatever space\r\n\r\nbar",
      .exp_retval = 0,
      .exp_body = "bar",
      .exp_body_size = sizeof "bar", // Because we allocate enough room for trailing \0
    },
  };

  for (size_t i = 0; i < N_ELEMS(utests); i++) {
    struct utest   *u = utests + i;
    unsigned char *body = NULL;
    size_t         body_len = 0;
    int            retval = -1;

    if (u->buf)
      u->buf_size = strlen(u->buf) + 1;

    retval = http_parse_body((unsigned char *) u->buf, u->buf_size, &body, &body_len);
    if (retval != u->exp_retval) {
      logger("input '%s', expected %d, got %d", u->buf, u->exp_retval, retval);
      n_failures++;
      free(body);
    } else {
      if (body && u->exp_body) {
        if (strcmp((char *) body, u->exp_body)) {
          logger("input '%s', expected body '%s', got '%s'",
                 u->buf, u->exp_body, body);
          n_failures++;
        } else if (body_len != u->exp_body_size) {
          logger("input '%s', expected body_len '%zu', got '%zu'",
                 u->buf, u->exp_body_size, body_len);
          n_failures++;
        }
      }

      n_successes++;
    }

    free(body);
  }


  logger("Successes: %d, Failures: %d", n_successes, n_failures);
  return n_failures;
}

int http_parse_utest(void)
{
  int n_errors = 0;

  int (*funcs[])(void) = {
    http_parse_code_utest,
    http_parse_headers_utest,
    http_parse_body_utest,
  };

  for (size_t i = 0; i < N_ELEMS(funcs); i++) {
    n_errors += funcs[i]();
  }

  return n_errors;
}
