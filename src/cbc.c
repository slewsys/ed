/* cbc.c: This file contains the encryption routines for the ed line editor */
/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *  Copyright © 1993 The Regents of the University of California.
 *
 *  Copyright © 2013, 2018 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#include <pwd.h>
#include <signal.h>
#include <termios.h>

#include "ed.h"

#ifdef WANT_DES_ENCRYPTION
# include <openssl/des.h>

/* Provide arc4random_buf(3) on GNU/Linux. */
# ifdef HAVE_LIB_BSD
#  include <bsd/stdlib.h>
# endif

# define _(String) gettext (String)

/*
 * BSD and System V systems offer special library calls that do
 * block move_liness and fills, so if possible we take advantage of them
 */
# define MEMCPY(dest,src,len)	memcpy((dest),(src),(len))
# define MEMZERO(dest,len)	memset((dest), 0, (len))

/* read/write without error checking. */
# define READ(buf, n, fp)	fread(buf, sizeof(char), n, fp)
# define WRITE(buf, n, fp)	fwrite(buf, sizeof(char), n, fp)

/* Static function declarations. */
static int expand_des_key (unsigned char *, char *, ed_buffer_t *);
static void set_des_key (DES_cblock *);
static int cbc_encode (unsigned char *, int, FILE *);
static int cbc_decode (unsigned char *, FILE *, ed_buffer_t *);
static int char_to_int (int, int);

/* Global variables. */
static DES_cblock ivec;		/* initialization vector */
static DES_cblock pvec;		/* padding vector */

/* used to extract bits from a char */
static char bits[] =
  {
   '\200',
   '\100',
   '\040',
   '\020',
   '\010',
   '\004',
   '\002',
   '\001'
  };

static int pflag;		/* 1 to preserve parity bits */

static DES_key_schedule schedule; /* expanded DES key */

static unsigned char des_buf[8];  /* Buffer for get_des_char/put_des_char. */
static int des_ct = 0;          /* Count for get_des_char/put_des_char. */
static int des_n = 0;           /* Index for put_des_char/get_des_char. */

/* init_des_cipher: Initialize DES. */
void
init_des_cipher (void)
{
  des_ct = des_n = 0;

  /* Initialize initialization vector. */
  MEMZERO (ivec, 8);

  /* Initialize padding vector. */
  arc4random_buf (pvec, sizeof (pvec));
}


/* get_des_char: Return next char in encrypted file. */
int
get_des_char (FILE *fp, ed_buffer_t *ed)
{
  if (des_n >= des_ct)
    {
      des_n = 0;
      des_ct = cbc_decode (des_buf, fp, ed);
    }
  return (des_ct > 0) ? des_buf[des_n++] : EOF;
}


/* put_des_char: Write char to encrypted file, and return char written. */
int
put_des_char (int c, FILE *fp)
{
  if (des_n == sizeof des_buf)
    {
      des_ct = cbc_encode (des_buf, des_n, fp);
      des_n = 0;
    }
  return (des_ct >= 0) ? (des_buf[des_n++] = c) : EOF;
}


/* flush_des_file: Flush encrypted file's output and return status. */
int
flush_des_file (FILE *fp)
{
  if (des_n == sizeof des_buf)
    {
      des_ct = cbc_encode (des_buf, des_n, fp);
      des_n = 0;
    }
  return (des_ct >= 0 && cbc_encode (des_buf, des_n, fp) >= 0) ? 0 : EOF;
}

/* get des_keyword: Read key from tty. */
int
get_des_keyword (ed_buffer_t *ed)
{
  DES_cblock msgbuf;		/* I/O buffer */
  char *p;			/* Pointer to key read from tty. */
  int status = 0;

  /* Get password. */
  if ((p = getpass ("Enter key: ")) == NULL)
    {
      ed->exec->err = _("Invalid key.");
      return ERR;
    }

  /* If empty password, disable encryption/decryption. */
  else if (*p == '\0')
    {
      ed->exec->have_key = 0;
      return 0;
    }

  /* Copy key in key area */
  if ((status = expand_des_key (msgbuf, p, ed)) < 0)
    return status;
  MEMZERO (p, strlen (p));
  set_des_key (&msgbuf);
  MEMZERO (msgbuf, sizeof msgbuf);
  ed->exec->have_key = 1;
  return 0;
}


/* char_to_int: Convert character to integer per radix. */
static int
char_to_int (int c, int radix)
{
# ifdef HAVE_STRTOL
  static int s[2] = { 0 };

  long n;

  s[0] = c;
  errno = 0;
  return (n = strtol (s, NULL, radix)) == 0 && errno == EINVAL ? -1 : n;
# else  /* !HAVE_STRTOL */
  switch (c)
    {
    case '0':
      return (0x0);
    case '1':
      return (0x1);
    case '2':
      return (radix > 2 ? 0x2 : -1);
    case '3':
      return (radix > 3 ? 0x3 : -1);
    case '4':
      return (radix > 4 ? 0x4 : -1);
    case '5':
      return (radix > 5 ? 0x5 : -1);
    case '6':
      return (radix > 6 ? 0x6 : -1);
    case '7':
      return (radix > 7 ? 0x7 : -1);
    case '8':
      return (radix > 8 ? 0x8 : -1);
    case '9':
      return (radix > 9 ? 0x9 : -1);
    case 'A':
    case 'a':
      return (radix > 10 ? 0xa : -1);
    case 'B':
    case 'b':
      return (radix > 11 ? 0xb : -1);
    case 'C':
    case 'c':
      return (radix > 12 ? 0xc : -1);
    case 'D':
    case 'd':
      return (radix > 13 ? 0xd : -1);
    case 'E':
    case 'e':
      return (radix > 14 ? 0xe : -1);
    case 'F':
    case 'f':
      return (radix > 15 ? 0xf : -1);
    }

  /* Not a digit. */
  return -1;
# endif  /* !HAVE_STRTOL */
}

/* expand_des_key: Convert 64-bit key to bit pattern. */
static int
expand_des_key (unsigned char *obuf, char *kbuf, ed_buffer_t *ed)
{
  int i, j;
  int nbuf[64];			/* Buffer for hex/binary key conversion. */

  /* 64-digit binary key. */
  if (kbuf[0] == '0' && (kbuf[1] == 'b' || kbuf[1] == 'B'))
    {
      kbuf = &kbuf[2];

      /* Convert binary characters to ints. */
      for (i = 0; i < 64 && kbuf[i]; i++)
        if ((nbuf[i] = char_to_int ((int) kbuf[i], 2)) == -1)
          {
            ed->exec->err = _("Invalid key");
            return ERR;
          }
      if (i != 64)
        {
          ed->exec->err = _("Invalid key");
          return ERR;
        }
      for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
          obuf[i] = (obuf[i] << 1) | nbuf[8 * i + j];

      /* Preserve parity bits. */
      pflag = 1;
      return 0;
    }

  /* 16-digit hexidecimal key. */
  if (kbuf[0] == '0' && (kbuf[1] == 'x' || kbuf[1] == 'X'))
    {
      kbuf = &kbuf[2];

      /* Convert hexadecimal characters to ints. */
      for (i = 0; i < 16 && kbuf[i]; i++)
        if ((nbuf[i] = char_to_int ((int) kbuf[i], 16)) == -1)
          {
            ed->exec->err = _("Invalid key");
            return ERR;
          }
      if (i != 16)
        {
          ed->exec->err = _("Invalid key");
          return ERR;
        }
      for (i = 0; i < 8; i++)
        obuf[i] = ((nbuf[2 * i] & 0xf) << 4) | (nbuf[2 * i + 1] & 0xf);

      /* Preserve parity bits. */
      pflag = 1;
      return 0;
    }

  /* 8-byte ASCII key. */
  (void) strncpy ((char *) obuf, kbuf, 8);
  return 0;
}

/*****************
 * DES FUNCTIONS *
 *****************/
/*
 * This sets the DES key and (if you're using the deszip version)
 * the direction of the transformation.  This uses the Sun
 * to map the 64-bit key onto the 56 bits that the key schedule
 * generation routines use: the old way, which just uses the user-
 * supplied 64 bits as is, and the new way, which resets the parity
 * bit to be the same as the low-order bit in each character.  The
 * new way generates a greater variety of key schedules, since many
 * systems set the parity (high) bit of each character to 0, and the
 * DES ignores the low order bit of each character.
 */
static void
set_des_key (DES_cblock * buf)
{
  int i, j;
  int par;			/* Parity counter. */

  /* If parity not preserved, flip it. */
  if (!pflag)
    {
      for (i = 0; i < 8; i++)
        {
          par = 0;
          for (j = 1; j < 8; j++)
            if ((bits[j] & (*buf)[i]) != 0)
              par++;
          if ((par & 0x01) == 0x01)
            (*buf)[i] &= 0x7f;
          else
            (*buf)[i] = ((*buf)[i] & 0x7f) | 0x80;
        }
    }

  DES_set_odd_parity (buf);
  DES_set_key (buf, &schedule);
}


/* cbc_encode: Encrypt a block using DES Cipher Block Chaining mode. */
static int
cbc_encode (unsigned char *msgbuf, int n, FILE *fp)
{
  /* Don't encode empty file. */
  if (n == 0 && des_ct == 0)
    return 0;
  else if (n == 8)
    {
      for (n = 0; n < 8; n++)
        msgbuf[n] ^= ivec[n];
      DES_ecb_encrypt((DES_cblock *) msgbuf, (DES_cblock *) msgbuf,
                      &schedule, DES_ENCRYPT);
      MEMCPY (ivec, msgbuf, 8);
      return WRITE (msgbuf, 8, fp);
    }

  /*
   * At EOF or last block -- in either case, the last byte contains
   * the character representation of the number of bytes in it
   */
  /*
    MEMZERO(msgbuf +  n, 8 - n);
  */
  /* Pad the last block randomly */
  (void) MEMCPY (msgbuf + n, pvec, 8 - n);
  msgbuf[7] = n;
  for (n = 0; n < 8; n++)
    msgbuf[n] ^= ivec[n];
  DES_ecb_encrypt((DES_cblock *) msgbuf, (DES_cblock *) msgbuf,
                  &schedule, DES_ENCRYPT);
  return WRITE (msgbuf, 8, fp);
}

/* cbc_decode: Decrypt using DES Cipher Block Chaining mode. */
static int
cbc_decode (unsigned char *msgbuf, FILE *fp, ed_buffer_t *ed)
{
  DES_cblock tbuf;		/* Initialization vector. */
  int n, c;

  if ((n = READ (msgbuf, 8, fp)) == 8)
    {
      MEMCPY (tbuf, msgbuf, 8);
      DES_ecb_encrypt((DES_cblock *) msgbuf, (DES_cblock *) msgbuf,
                      &schedule, DES_DECRYPT);
      for (c = 0; c < 8; c++)
        msgbuf[c] ^= ivec[c];
      MEMCPY (ivec, tbuf, 8);

      /* If the last one, handle it specially. */
      if ((c = fgetc (fp)) == EOF)
        {
          n = msgbuf[7];
          if (n < 0 || n > 7)
            {
              ed->exec->err = _("Decryption failed (block corrupted).");
              return EOF;
            }
        }
      else
        (void) ungetc (c, fp);
      return n;
    }
  if (n > 0)
    ed->exec->err = _("Decryption failed (incomplete block).");
  else if (n < 0)
    ed->exec->err = _("File read error");
  return EOF;
}
#endif /* WANT_DES_ENCRYPTION */
