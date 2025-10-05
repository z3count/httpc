#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "util.h"
#include "logger.h"
#include "strutil.h"

int trim(char *haystack, char **trimmedp)
{
  char  *trimmed = NULL;
  size_t i = 0;
  char  *endptr = NULL;

  if (! haystack)
    goto err;

  if (! *haystack) { // Empty string
    if (! (trimmed = strdup(""))) {
      logger("strdup: %m");
      goto err;
    }
    goto end;
  }

  while (haystack[i] && isspace((unsigned char) haystack[i]))
    i++;
  
  if (NULL == (trimmed = strdup(haystack + i)))
    goto err;

  i = 0;

  if (! *trimmed) // whitespaces only, once trimmed it's an empty one
    goto end;

  endptr = trimmed + strlen(trimmed) - 1;

  while (endptr > trimmed && isspace((unsigned char) *endptr))
    endptr--;

  endptr[1] = '\0';
  i = strlen(trimmed);

 end:
  if (trimmedp)
    *trimmedp = trimmed;
  else
    free(trimmed);

  return i;
 err:
  return -1;
}

//
// Unit tests
//

#include "../tests/strutil_utest.c"
