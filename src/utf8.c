/* utf8.c: UTF-8 routines for ed line editor.
 *
 *  Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#include "ed.h"

typedef struct utf8_ea_width {
  int code_point;
  int width;
} utf8_ea_width_t;

/* Encoded UTF-8 byte masks. */
unsigned char utf8_byte_mask[5] =
  {
    0x80,                       /* 10000000 */
    0xC0,                       /* 11000000 */
    0xE0,                       /* 11100000 */
    0xF0,                       /* 11110000 */
    0xF8,                       /* 11111000 */
  };

/* Offsets into (decoded) UTF-8 code point. */
int utf8_cp_offset[4] =
  {
    0,
    8,
    16,
    24
  };


/*
 * decode_utf8_char: Decode UTF-8 character sequence, s, per RFC 3629
 * to code, (up to U+10FFFF) increment s to following char. Return
 * code point if successful, otherwise ERR.
 */
int
decode_utf8_char (s, len)
     unsigned char **s;
     int len;
{
  int cp;
  int bytes = 0;
  unsigned char *t;

  if (len < 1)
    return ERR;

  /* 7-bit ASCII. */
  if (*(t = *s) <= 0x7F)
    {
      *s = t + 1;
      return *t;
    }

  /* UTF-8 continuation byte unexpected. */
  if ((*t & utf8_byte_mask[1]) == utf8_byte_mask[0])
    return ERR;

  /* 2-byte char sequence. */
  if ((*t & utf8_byte_mask[2]) == utf8_byte_mask[1])
    {
      /* UTF-8 encoding over-long. */
      if ((*t & ~utf8_byte_mask[2]) <= 1)
        return ERR;

      bytes = 2;
    }

  /* 3-byte char sequence. */
  else if ((*t & utf8_byte_mask[3]) == utf8_byte_mask[2])
    {
      bytes = 3;
    }

  /* 4-byte char sequence. */
  else if ((*t & utf8_byte_mask[4]) == utf8_byte_mask[3])
    {
      /* UTF-8 code point exceeds 0x10FFFF. */
      if ((*t & ~utf8_byte_mask[4]) > 5)
        return ERR;

      bytes = 4;
    }

  /* UTF-8 leading byte encoding invalid. */
  else
    return ERR;

  /* Insufficient data to decode UTF-8. */
  if (len < bytes)
    return ERR;

  /*  Extract data from leading byte of UTF-8 encoding. */
  cp = (*t++ & ~utf8_byte_mask[bytes]) << utf8_cp_offset[bytes - 1];

  while (--bytes > 0)
    {
      /* UTF-8 continuation byte encoding invalid. */
      if ((*t & utf8_byte_mask[1]) != utf8_byte_mask[0])
        return ERR;

      /*  Extract data from continuation byte of UTF-8 encoding. */
      cp |= (*t++ & ~utf8_byte_mask[1]) << utf8_cp_offset[bytes - 1];
    }

  /*  Not a UTF-8 code point. */
  if (cp == 0xFFFE || cp == 0xFFFF || (0xDC80 <= cp && cp <= 0xDCFF))
    return ERR;

  *s = t;
  return cp;
}


/*
 * encode_utf8_char: Encode UTF-8 code point, CODE, per RFC 3629 (up to
 * U+10FFFF) to character buffer S. Return 0 if successful, otherwise ERR.
 *
 * NB: The buffer S must be long enough to contain the UTF-8 encoding -
 * up to 4 bytes in length per RFC 3629.
 */
int
encode_utf8_char (s, len, code)
     char *s;
     int *len;
     unsigned int code;
{
  unsigned int cp = code;
  char *t = s;

  /* 1-byte UTF-8 encoding (i.e., 7-bit ASCII). */
  if (cp <= 0x7F)
    {
      *t++ = cp;
    }

  /* 2-byte UTF-8 encoding. */
  else if (cp <= 0x7FF)
    {
      /*
       * Map lower 11 bits of code point to UTF-8 encoding.
       * Upper 5 code-point bits to UTF-8 leading byte.
       * Lower 6 code-point bits to UTF-8 continuation byte.
       *
       * Visual representation of code point bit mapping:
       *
       *   Code point: LLLLLXXXXXX
       *               |     \
       *               |      \
       *   Encoded: 110LLLLL 10XXXXXX
       */
      *t++ = utf8_byte_mask[1] | (cp & utf8_byte_mask[4] << 3) >> 6;
      *t++ = utf8_byte_mask[0] | (cp & ~utf8_byte_mask[1]);
    }

  /* 3-byte UTF-8 encoding. */
  else if (cp <= 0xFFFF)
    {
      /* Illegal UTF-8 code points per RFC 3629. */
      if ((0xDC80 <= cp && cp <= 0xDCFF) || cp == 0xFFFE || cp == 0xFFFF)
        return ERR;

      /*
       * Map lower 16 bits of code point to UTF-8 encoding.
       * Upper 4 code-point bits to UTF-8 leading byte.
       * Middle 6 code-point bits to first UTF-8 continuation byte.
       * Lower 6 code-point bits to second UTF-8 continuation byte.
       *
       * Visual representation of code point bit mapping:
       *
       *   Code point:     LLLLXXXXXXYYYYYY
       *                  /    |      \
       *                 /     |       \
       *   Encoded: 1110LLLL 10XXXXXX 10YYYYYY
       */
      *t++ = utf8_byte_mask[1] | (cp & utf8_byte_mask[3] << 8) >> 12;
      *t++ = utf8_byte_mask[0] | (cp & ~utf8_byte_mask[1] << 6) >> 6;
      *t++ = utf8_byte_mask[0] | (cp & ~utf8_byte_mask[1]);
    }

  /* 4-byte UTF-8 encoding */
  else if (cp <= 0x10FFFF)
    {
      /*
       * Map lower 21 bits of code point to UTF-8 encoding.
       * Upper 3 code-point bits to UTF-8 leading byte.
       * Upper middle 6 code-point bits to first UTF-8 continuation byte.
       * Lower middle 6 code-point bits to second UTF-8 continuation byte.
       * Lower 6 code-point bits to third UTF-8 continuation byte.
       *
       * Visual representation of code point bit mapping:
       *
       *   Code point:      LLLXXXXXXYYYYYYZZZZZZ
       *                   /   |      \     `
       *                  /    |       \        `
       *   Encoded: 11110LLL 10XXXXXX 10YYYYYY 10ZZZZZZ
       */
      *t++ = utf8_byte_mask[1] | (cp & utf8_byte_mask[2] << 13) >> 18;
      *t++ = utf8_byte_mask[0] | (cp & ~utf8_byte_mask[1] << 12) >> 12;
      *t++ = utf8_byte_mask[0] | (cp & ~utf8_byte_mask[1] << 6) >> 6;
      *t++ = utf8_byte_mask[0] | (cp & ~utf8_byte_mask[1]);
    }

  /* Not a UTF-8 code point per RFC 3629. */
  else
    return ERR;

  *len = t - s;
  return 0;
}

/*
 * int
 * utf8_char_width (s, len)
 *      char *s;
 *      int *len;
 * {
 *   utf8_ea_width_t *ea_width;
 * }
 */

int
is_utf8 (s, len)
     const char *s;
     size_t len;
{
  unsigned char *t = (unsigned char *) s;
  int code = 0;

  while (*t != '\n' && (code = decode_utf8_char (&t, len)) >= 0)
    ;
  return code >= 0;
}

int
utf8_char_size (s, len)
     const char *s;
     size_t len;
{
  unsigned char *t = (unsigned char *) s;

  return decode_utf8_char (&t, len) >= 0 ? (char *) t - s : 0;
}
