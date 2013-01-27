/* malloc.c: GNU-compatible malloc replacement. */

#include "config.h"

#if !HAVE_MALLOC

#undef malloc

void *malloc __P (size_t);

/* Allocate an N-byte block of memory from the heap.
   If N is zero, allocate a 1-byte block.  */
void *
rpl_malloc (size_t n)
{
  return malloc (!n | n);
}

#endif  /* !HAVE_MALLOC */
