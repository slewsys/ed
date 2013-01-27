/* realloc.c: GNU-compatible realloc replacement. */

#include "config.h"

#if !HAVE_REALLOC

#undef realloc

void *realloc __P ((void *, size_t));

/* Allocate an N-byte block of memory from the heap.
   If N is zero, allocate a 1-byte block.  */
void *
rpl_realloc (b, n)
     void *b;
     size_t n;
{
  return realloc (!n | n);
}

#endif  /* !HAVE_REALLOC */
