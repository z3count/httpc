#ifndef __UTIL_H__
#define __UTIL_H__

#define N_ELEMS(array) (sizeof array / sizeof array[0])
#define PTRDIFF(hi, lo) ((size_t) (ptrdiff_t) ((char *) (hi) - (char *) (lo)))

#define CRLF "\r\n"
#define CRLF_LEN (sizeof CRLF -1)

#endif // __UTIL_H__
