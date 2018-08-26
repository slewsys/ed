/* utf8.c: UTF-8 routines for ed line editor.
 *
 *  Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#include "ed.h"

/* Static function declarations. */
static int is_utf8_ea_wide __P ((int));

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
    return EOF;

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

int
is_utf8_str (s, len)
     const char *s;
     size_t len;
{
  unsigned char *t = (unsigned char *) s;
  int status;

  while ((status = decode_utf8_char (&t, len - ((char *) t - s))) >= 0)
    ;
  return status == EOF;
}

int
utf8_char_size (s, len)
     const char *s;
     size_t len;
{
  unsigned char *t = (unsigned char *) s;
  int status;

  return (status = decode_utf8_char (&t, len)) >= EOF ? (char *) t - s : status;
}

int
utf8_strlen (s, len)
     const char *s;
     size_t len;
{
  unsigned char *t = (unsigned char *) s;
  int status;
  int utf8len = 0;

  while ((status = decode_utf8_char (&t, len - ((char *) t - s))) >= 0)
    ++utf8len;
  return status == EOF ? utf8len : status;
}

int
utf8_char_display_width (s, len)
     const char *s;
     int len;
{
  unsigned char *t = (unsigned char *) s;
  int code;

  switch (code = decode_utf8_char (&t, len))
    {
    case EOF:
      return 0;
    case ERR:
      return code;
    default:
      return is_utf8_ea_wide (code) ? 2 : 1;
    }
}

/*
 * Return 1 if Unicode code point, CODE, is East Asian wide as per Unicode
 * Standard Annex #11 (UAX #11).
 */
static int
is_utf8_ea_wide (code)
     int code;
{
  if (code < 0x1100)
    return 0;

  return (0x1100 <= code && code <= 0x115F)
    || (0x231A <= code && code <= 0x231B)
    || 0x2329 == code
    || 0x232A == code
    || (0x23E9 <= code && code <= 0x23EC)
    || 0x23F0 == code
    || 0x23F3 == code
    || (0x25FD <= code && code <= 0x25FE)
    || (0x2614 <= code && code <= 0x2615)
    || (0x2648 <= code && code <= 0x2653)
    || 0x267F == code
    || 0x2693 == code
    || 0x26A1 == code
    || (0x26AA <= code && code <= 0x26AB)
    || (0x26BD <= code && code <= 0x26BE)
    || (0x26C4 <= code && code <= 0x26C5)
    || 0x26CE == code
    || 0x26D4 == code
    || 0x26EA == code
    || (0x26F2 <= code && code <= 0x26F3)
    || 0x26F5 == code
    || 0x26FA == code
    || 0x26FD == code
    || 0x2705 == code
    || (0x270A <= code && code <= 0x270B)
    || 0x2728 == code
    || 0x274C == code
    || 0x274E == code
    || (0x2753 <= code && code <= 0x2755)
    || 0x2757 == code
    || (0x2795 <= code && code <= 0x2797)
    || 0x27B0 == code
    || 0x27BF == code
    || (0x2B1B <= code && code <= 0x2B1C)
    || 0x2B50 == code
    || 0x2B55 == code
    || (0x2E80 <= code && code <= 0x2E99)
    || (0x2E9B <= code && code <= 0x2EF3)
    || (0x2F00 <= code && code <= 0x2FD5)
    || (0x2FF0 <= code && code <= 0x2FFB)
    || (0x3001 <= code && code <= 0x303E)
    || (0x3041 <= code && code <= 0x3096)
    || (0x3099 <= code && code <= 0x30FF)
    || (0x3105 <= code && code <= 0x312F)
    || (0x3131 <= code && code <= 0x318E)
    || (0x3190 <= code && code <= 0x31BA)
    || (0x31C0 <= code && code <= 0x31E3)
    || (0x31F0 <= code && code <= 0x321E)
    || (0x3220 <= code && code <= 0x3247)
    || (0x3250 <= code && code <= 0x32FE)
    || (0x3300 <= code && code <= 0x4DBF)
    || (0x4E00 <= code && code <= 0xA48C)
    || (0xA490 <= code && code <= 0xA4C6)
    || (0xA960 <= code && code <= 0xA97C)
    || (0xAC00 <= code && code <= 0xD7A3)
    || (0xF900 <= code && code <= 0xFAFF)
    || (0xFE10 <= code && code <= 0xFE19)
    || (0xFE30 <= code && code <= 0xFE52)
    || (0xFE54 <= code && code <= 0xFE6B)
    || (0x16FE0 <= code && code <= 0x16FE1)
    || (0x17000 <= code && code <= 0x187F1)
    || (0x18800 <= code && code <= 0x18AF2)
    || (0x1B000 <= code && code <= 0x1B11E)
    || (0x1B170 <= code && code <= 0x1B2FB)
    || 0x1F004 == code
    || 0x1F0CF == code
    || 0x1F18E == code
    || (0x1F191 <= code && code <= 0x1F19A)
    || (0x1F200 <= code && code <= 0x1F202)
    || (0x1F210 <= code && code <= 0x1F23B)
    || (0x1F240 <= code && code <= 0x1F248)
    || (0x1F250 <= code && code <= 0x1F251)
    || (0x1F260 <= code && code <= 0x1F265)
    || (0x1F300 <= code && code <= 0x1F320)
    || (0x1F32D <= code && code <= 0x1F335)
    || (0x1F337 <= code && code <= 0x1F37C)
    || (0x1F37E <= code && code <= 0x1F393)
    || (0x1F3A0 <= code && code <= 0x1F3CA)
    || (0x1F3CF <= code && code <= 0x1F3D3)
    || (0x1F3E0 <= code && code <= 0x1F3F0)
    || 0x1F3F4 == code
    || (0x1F3F8 <= code && code <= 0x1F43E)
    || 0x1F440 == code
    || (0x1F442 <= code && code <= 0x1F4FC)
    || (0x1F4FF <= code && code <= 0x1F53D)
    || (0x1F54B <= code && code <= 0x1F54E)
    || (0x1F550 <= code && code <= 0x1F567)
    || 0x1F57A == code
    || (0x1F595 <= code && code <= 0x1F596)
    || 0x1F5A4 == code
    || (0x1F5FB <= code && code <= 0x1F64F)
    || (0x1F680 <= code && code <= 0x1F6C5)
    || 0x1F6CC == code
    || (0x1F6D0 <= code && code <= 0x1F6D2)
    || (0x1F6EB <= code && code <= 0x1F6EC)
    || (0x1F6F4 <= code && code <= 0x1F6F9)
    || (0x1F910 <= code && code <= 0x1F93E)
    || (0x1F940 <= code && code <= 0x1F970)
    || (0x1F973 <= code && code <= 0x1F976)
    || 0x1F97A == code
    || (0x1F97C <= code && code <= 0x1F9A2)
    || (0x1F9B0 <= code && code <= 0x1F9B9)
    || (0x1F9C0 <= code && code <= 0x1F9C2)
    || (0x1F9D0 <= code && code <= 0x1F9FF)
    || (0x20000 <= code && code <= 0x2FFFD)
    || (0x30000 <= code && code <= 0x3FFFD);
}
