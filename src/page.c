/* page.c: Display routines for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"


/* Static function declarations. */

#ifdef WANT_ED_SCROLL
static int display_frame_buffer (const ed_frame_buffer_t *, ed_buffer_t *);
static ed_frame_node_t *dup_frame_node (const ed_frame_node_t *, ed_buffer_t *);
static int fb_putc (int, ed_frame_buffer_t *, ed_buffer_t *);
static int fb_putwc (char *, size_t, ed_frame_buffer_t *, ed_buffer_t *);
static int init_frame_buffer (ed_frame_buffer_t *, ed_buffer_t *);
static void init_frame_node (ed_frame_node_t *);
static int put_frame_buffer_line (ed_line_node_t *, off_t,
                                  ed_frame_buffer_t *, ed_buffer_t *);
static void inc_mod_fb_row (ed_line_node_t *, off_t, int,
                            ed_frame_buffer_t *, ed_buffer_t *);

#endif

static unsigned int sgr_span (const char *);

/* GET_CHAR_WIDTH: Assert: Tabstops are of fixed length. */
#define GET_CHAR_WIDTH(i, c)                                                  \
  ((i) == '\b' ? -1                                                           \
   : (i) == '\f' ? 0                                                          \
   : (i) == '\r' ? -(c)                                                       \
   : (i) == '\t' ? TAB_WIDTH - ((c) - ((c) / TAB_WIDTH) * TAB_WIDTH)          \
   : (i) < '\040' ? 0                                                         \
   : 1)

#define RIGHT_MARGIN TAB_WIDTH

#ifndef WANT_ED_SCROLL
static int put_tty_line (ed_line_node_t *, off_t, ed_buffer_t *);

/* display_lines: Print a range of lines. Return status (<= 0). */
int
display_lines (off_t from, off_t to, ed_buffer_t *ed)
{
  static ed_frame_buffer_t frame_buffer = { 0 };

  ed_frame_buffer_t *fb = &frame_buffer;
  ed_line_node_t *bp, *ep;
  off_t lc = 0;                 /* Line counter */
  size_t rem_chars = 0;         /* Remaining chars of line across pages. */
  int flags = 0;
  int status;

  /* Trivially allow printing empty buffer. */
  if (!ed->state->lines)
    return 0;

  ep = get_line_node (INC_MOD (to, ed->state->lines), ed);
  bp = get_line_node (from, ed);

  for (; bp != ep; bp = bp->q_forw, ++lc)
    if ((status = put_tty_line (bp, from + lc, ed)) < 0)
      return status;

  /* Don't update current address until printing completed -- per bwk. */
  ed->state->dot = from + lc - 1;
  return 0;
}

/* put_frame_buffer_line: Print text to frame buffer. */
static int
put_tty_line (ed_line_node_t *lp, off_t addr, ed_buffer_t *ed)
{
  static off_t lines = 0;
  static int lines_len = 0;      /* strlen (lines) */
  static char fmt[27] = { '\0' }; /* Line no. format, e.g., "% 3lld\t" */

  off_t n;
  ed_frame_node_t *rp;
  char line_no[OFF_T_LEN + TAB_WIDTH + 1];
  char *cp, *es, *s;
  size_t col = 0;
  size_t i = 0;
  int len = 0;
  unsigned int sgr_len = 0;     /* Length of ANSI SGR sequence. */
  int form_feed = 0;

  /*
   * Per SUSv4, 2013, the `$' (dollar sign) character is output by the
   * `l' (literal) command as the escape sequence `\$'
   */
  char control_char[10] =
    {
      BELL,
      BACKSPACE,
      CHARACTER_TABULATION,
      LINE_FEED,
      LINE_TABULATION,
      FORM_FEED,
      CARRIAGE_RETURN,
      DOLLAR_SIGN,
      REVERSE_SOLIDUS,
      '\0'
    };

  char *escape_sequence[] =
    {
      "\\a",                    /* BELL */
      "\\b",                    /* BACKSPACE */
      "\\t",                    /* CHARACTER_TABULATION */
      "\\n",                    /* LINE_FEED */
      "\\v",                    /* LINE_TABULATION */
      "\\f",                    /* FORM_FEED */
      "\\r",                    /* CARRIAGE_RETURN */
      "\\$",                    /* DOLLAR_SIGN */
      "\\\\"                    /* REVERSE_SOLIDUS */
    };

  /* Numbered output. */
  if (ed->display->dio_f & NMBR)
    {
      /* If lines has changed, update format string. */
      if (lines != ed->state->lines)
        {
          n = lines = ed->state->lines;
          for (lines_len = 0; n /= 10; ++lines_len)
            ;
          ++lines_len;

          /* Set line numbering format string. */
          sprintf (fmt, "%%%d" OFF_T_FORMAT_STRING "\t", lines_len);
        }

      /* Evaluate format string, fmt, with address, addr, of dot. */
      snprintf (line_no, OFF_T_LEN + TAB_WIDTH + 1, fmt, addr);
      while (line_no[i])
        if (putchar (line_no[i++]) < 0)
          return ERR;
      col = lines_len;
      col += GET_CHAR_WIDTH ('\t', lines_len);
    }

  if (!(s = get_buffer_line (lp, ed)))
    return ERR;
  len = strlen (s);

  for (; *s; ++s)
    {
      if (!(ed->display->dio_f & LIST)
          || (31 < *s && *s < 127 && *s != '\\' && *s != '$'))
        {

          /*
           * If ANSI color support is enabled, exclude SGR sequences
           * from column bookkeeping.
           */
          if (ed->exec->opt & ANSI_COLOR && (sgr_len = sgr_span (s)) > 0)
            {
              for (; sgr_len--; ++s)
                if (putchar ((unsigned) *s) < 0)
                  return ERR;
              continue;
            }

          if (putchar ((unsigned) *s) < 0)
            return ERR;
          col += GET_CHAR_WIDTH ((unsigned) *s, col);

        }
      else
        {
          /*
           * If control char has C escape sequence, print literal
           * escape sequence - e.g., ASCII bel as `\a'.
           */
          if (*s && (cp = strchr (control_char, (unsigned) *s)))
            {
              es = escape_sequence[cp - control_char];
              if (putchar (es[0]) < 0 || putchar (es[1]) < 0)
                return ERR;
              col += 2;
            }

          /* Print other chars as octal escapes - i.e., \ooo. */
          else if (putchar ('\\') < 0
                   || putchar ((((unsigned) *s & 0300) >> 6) + '0') < 0
                   || putchar ((((unsigned) *s & 070) >> 3) + '0') < 0
                   || putchar (((unsigned) *s & 07) + '0') < 0)
            return ERR;
          else
            col += 4;
        }

      /*
       * When listing text -- i.e., (ed->display->dio_f & LIST) --
       * first try simple heuristic to split lines on word boundaries
       * for legibility; otherwise, split lines at the right margin,
       * which starts one tab stop from the right edge of the window.
       */
      if (!(ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
            && (ed->display->dio_f & LIST)
              && ((!isalnum ((unsigned) *s)
                   && col >= ed->display->ws_col - (RIGHT_MARGIN << 1)
                   && strlen (s) > 2)
                  || (col >= ed->display->ws_col - RIGHT_MARGIN
                      && strlen (s) > 1)))

        {
          if (putchar ('\\') < 0 || putchar ('\n') < 0)
            return ERR;
          col = 0;
        }
    }

  if ((ed->display->dio_f & LIST) && putchar ('$') < 0)
    return ERR;
  else if (putchar ('\n') < 0)
    return ERR;
  return 0;
}


#else /* WANT_ED_SCROLL */

/* INIT_FB_ROW: Initialize frame buffer row. */
# define INIT_FB_ROW(bp, caddr, off, fb)                                      \
  do                                                                          \
    {                                                                         \
      (fb)->row[(fb)->row_i]->lp = (bp);                                      \
      (fb)->row[(fb)->row_i]->addr = (caddr);                                 \
      (fb)->row[(fb)->row_i]->offset = (off);                                 \
      (fb)->row[(fb)->row_i]->text_i = 0;                                     \
    }                                                                         \
  while (0)

/* INC_FB_ROW: Increment frame buffer row. */
# define INC_FB_ROW(ok_to_wrap, fb)                                           \
  do                                                                          \
    {                                                                         \
      (fb)->row_i = ((fb)->row_i + 1) % ((fb)->rows - 1);                     \
      (fb)->wraps |= !(fb)->row_i;                                            \
      (fb)->is_full =                                                         \
        (ed->display->dio_f & ZHFW                                            \
         ? (fb)->row_i == ((fb)->fill_i + 1) % ((fb)->rows - 1)               \
         :  ((fb)->wraps && !(ok_to_wrap)));                                  \
    }                                                                         \
  while (0)

# define RESET_FB_PREV(fb, ed)                                                \
  do                                                                          \
    {                                                                         \
      if ((fb)->prev_first)                                                   \
        {                                                                     \
          free ((fb)->prev_first);                                            \
          (fb)->prev_first = NULL;                                            \
        }                                                                     \
      if ((fb)->prev_last)                                                    \
        {                                                                     \
          free ((fb)->prev_last);                                             \
          (fb)->prev_last = NULL;                                             \
        }                                                                     \
      (ed)->display->underflow = 0;                                           \
      (ed)->display->overflow = 0;                                            \
    }                                                                         \
  while (0)



/* display_lines: Print a range of lines. Return status (<= 0). */
int
display_lines (off_t from, off_t to, ed_buffer_t *ed)
{
  static ed_frame_buffer_t frame_buffer = { 0 };

  ed_frame_buffer_t *fb = &frame_buffer;
  ed_line_node_t *bp, *ep;
  off_t lc = 0;                 /* Line counter */
  size_t rem_chars = 0;         /* Remaining chars of line across pages. */
  int status;
  int saved_dio_f = ed->display->dio_f;

  /* Trivially allow printing empty buffer. */
  if (!ed->state->lines)
    return 0;

  /* XXX: Half-page scolling addresses are lost but need to be
     retained for this condition. */
  if (ed->exec->region->addrs)
    RESET_FB_PREV (fb, ed);

  /*
   * Prevent scrolling beyond limits of editor buffer.
   * XXX: Shouldn't be exceptions for half-page scolling.
   */
  if (ed->display->dio_f & ZBWD && !(ed->display->dio_f & (ZHBW | ZHFW))
      && !ed->exec->region->addrs && fb->prev_first
           && fb->prev_first->addr == 1 && fb->prev_first->offset == 0)
    {
      ed->exec->err = _("Address out of range");
      return ERR;
    }

  /* If not scrolling, ignore frame buffer status. */
  if (!(ed->display->dio_f & (ZFWD | ZBWD)))
    {
      ed->display->underflow = 0;
      ed->display->overflow = 0;
    }

  ep = get_line_node (INC_MOD (to, ed->state->lines), ed);
  bp = get_line_node (from, ed);

 top:
  if ((status = init_frame_buffer (fb, ed)) < 0)
    return status;

  for (fb->is_full = 0; bp != ep && !fb->is_full; bp = bp->q_forw, ++lc)
    {
      INIT_FB_ROW (bp, from + lc, 0, fb);

      /*
       * If last line of previous page was truncated while scrolling
       * forward, then resume with truncated part at top of page.
       */
      if (!(ed->display->dio_f & ZBWD) &&  ed->display->underflow
          && fb->prev_last && bp == fb->prev_last->lp)
        {
          fb->row[fb->row_i]->offset = bp->len - fb->rem_chars;
          ed->display->dio_f |= OFFB;
          ed->display->dio_f &= ~NMBR;
        }

      /*
       * If first line of previous page was truncated while scrolling
       * backward, then resume with truncated part at bottom of page.
       */
      else if (!(ed->display->dio_f & ZFWD) && ed->display->overflow
               && fb->prev_first && bp == fb->prev_first->lp)
        {
          fb->rem_chars = fb->prev_first->offset;

          /* Preserve length of last (source) line overflow. */
          rem_chars = bp->len - fb->rem_chars;
          fb->ff_offset = 0;
        }
      else
        {
          fb->rem_chars = bp->len;
          fb->ff_offset = 0;
        }
      if ((status = put_frame_buffer_line (bp, from + lc, fb, ed)) < 0)
        return status;
      if (!fb->is_full)
        INC_FB_ROW (ed->display->dio_f & ZBWD, fb);
      ed->display->dio_f = saved_dio_f;
    }

  /* If final forward page not full, then scroll back from last line. */
  if (ed->display->dio_f & ZFWD && to == ed->state->lines && !fb->wraps
      && (ed->display->underflow || from != 1))
    {
      /* Ignore frame buffer status. */
      RESET_FB_PREV (fb, ed);
      ed->display->dio_f |= ZBWD;
      ed->display->dio_f &= ~(ZFWD | ZHFW);
      return display_lines (max (1, ed->state->lines - fb->rows + 2),
                            ed->state->lines, ed);
    }

  /*
   * If final backward page not full, then scroll forward from first
   * line.
   */
  if (ed->display->dio_f & ZBWD && from == 1 && !fb->wraps
      && (ed->display->overflow || to != ed->state->lines))
    {
      /* Ignore frame buffer status. */
      RESET_FB_PREV (fb, ed);
      ed->display->dio_f |= ZFWD;
      ed->display->dio_f &= ~(ZBWD | ZHBW);
      return display_lines (1, min (ed->state->lines, fb->rows - 1), ed);

    }

  spl1 ();

  /* frame buffer not empty ... */
  if (lc)
    {
      RESET_FB_PREV (fb, ed);

      /*
       * Calculate frame buffer row index of the first and last lines
       * to print. If paging forward, then first line is given and its
       * row index is trivially 0. If paging backward, then only the
       * last line is known. In this case, a starting point is
       * estimated and the frame buffer is filled, wrapping around top
       * of buffer as necessary, until the end point in the source
       * text is reached.
       */
      if (ed->display->dio_f & ZHFW)
        {
          fb->first_i = fb->row_i;
          fb->last_i = fb->row_i ? fb->row_i - 1 : fb->rows - 2;
        }
      else
        {
          fb->first_i = fb->wraps ? fb->row_i : 0;
          fb->last_i = (fb->wraps
                        ? (fb->first_i ? fb->first_i - 1 : fb->rows - 2)
                        : fb->row_i - 1);
        }
      if (!(fb->prev_first = dup_frame_node (fb->row[fb->first_i], ed))
          || !(fb->prev_last = dup_frame_node (fb->row[fb->last_i], ed)))
        {
          spl0 ();
          return ERR;
        }

      /* Line address of first row. */
      ed->display->page_addr = fb->prev_first->addr;

      /*
       * Assign old prev_first `len - offset' as rem_chars to last
       * line of current page.
       */
      if (ed->display->dio_f & ZBWD)
        fb->rem_chars = rem_chars;

      /*
       * If next command is `scroll forward', then current page
       * becomes `page above', and ed->display->underflow is set if
       * last line of current page is truncated.
       */
      ed->display->underflow = !!fb->rem_chars;

      /*
       * If next command is `scroll backward', then current page
       * becomes `page below', and ed->display->overflow is set if
       * first line of current page has non-zero offset.
       */
      ed->display->overflow = !!fb->prev_first->offset;

      if (!ed->display->hidden  && (status = display_frame_buffer (fb, ed)) < 0)
        {
          spl0 ();
          return status;
        }
    }
  spl0 ();

  if (!(ed->display->dio_f & (ZBWD | ZFWD)))
    {
      /* More lines to print. */
      if (from + lc - 1 != to && !ed->display->underflow)
        goto top;

      /* More of current line to print. */
      if (ed->display->underflow)
        {
          bp = bp->q_back;
          --lc;
          goto top;
        }
    }

  /* Don't update current address until printing completed -- per bwk. */
  ed->state->dot = from + lc - 1;
  return 0;
}


/* GET_CHAR_WIDTH: Assert: Tabstops are of fixed length. */
#define GET_WCHAR_WIDTH(s, len, c)                                            \
  ((*s) == '\b' ? -1                                                          \
   : (*s) == '\f' ? 0                                                         \
   : (*s) == '\r' ? -(c)                                                      \
   : (*s) == '\t' ? TAB_WIDTH - ((c) - ((c) / TAB_WIDTH) * TAB_WIDTH)         \
   : ((unsigned char) *s) < '\040' ? 0                                        \
   : ed->state->is_utf8 ? utf8_char_display_width (s, len)                    \
   : 1)

/* FB_PUTS: Add string, s, to frame buffer. */
#define FB_PUTS(s, fb, ed)                                                    \
  do                                                                          \
    {                                                                         \
      int _i = 0;                                                             \
      while (*((s) + _i))                                                     \
        if (fb_putc (*((s) + _i++), (fb), (ed)) < 0)                          \
          return ERR;                                                         \
    }                                                                         \
  while (0)


/* put_frame_buffer_line: Print text to frame buffer. */
static int
put_frame_buffer_line (ed_line_node_t *lp, off_t addr,
                       ed_frame_buffer_t *fb, ed_buffer_t *ed)
{
  static off_t lines = 0;
  static int lines_len = 0;      /* strlen (lines) */
  static char fmt[27] = { '\0' }; /* Line no. format, e.g., "% 3lld\t" */

  off_t n;
  char line_no[OFF_T_LEN + TAB_WIDTH + 1];
  char *cp, *s;
  size_t col = 0;
  size_t len;
  unsigned int sgr_len = 0;     /* Length of ANSI SGR sequence. */
  int form_feed = 0;
  int csize = 1;                /* UTF-8 byte width. */

  /*
   * Per SUSv4, 2013, the `$' (dollar sign) character is output by the
   * `l' (literal) command as the escape sequence `\$'
   */
  char control_char[10] =
    {
      BELL,
      BACKSPACE,
      CHARACTER_TABULATION,
      LINE_FEED,
      LINE_TABULATION,
      FORM_FEED,
      CARRIAGE_RETURN,
      DOLLAR_SIGN,
      REVERSE_SOLIDUS,
      '\0'
    };

  char *escape_sequence[] =
    {
      "\\a",                    /* BELL */
      "\\b",                    /* BACKSPACE */
      "\\t",                    /* CHARACTER_TABULATION */
      "\\n",                    /* LINE_FEED */
      "\\v",                    /* LINE_TABULATION */
      "\\f",                    /* FORM_FEED */
      "\\r",                    /* CARRIAGE_RETURN */
      "\\$",                    /* DOLLAR_SIGN */
      "\\\\"                    /* REVERSE_SOLIDUS */
    };

  /* Numbered output. */
  if (ed->display->dio_f & NMBR)
    {
      /* If lines has changed, update format string. */
      if (lines != ed->state->lines)
        {
          n = lines = ed->state->lines;
          for (lines_len = 0; n /= 10; ++lines_len)
            ;
          ++lines_len;

          /* Set line numbering format string. */
          sprintf (fmt, "%%%d" OFF_T_FORMAT_STRING "\t", lines_len);
        }

      /* Evaluate format string, fmt, with address, addr, of dot. */
      snprintf (line_no, OFF_T_LEN + TAB_WIDTH + 1, fmt, addr);
      FB_PUTS (line_no, fb, ed);
      col = lines_len;
      col += GET_CHAR_WIDTH ('\t', lines_len);
    }

  if (!(s = get_buffer_line (lp, ed)))
    return ERR;

  if (ed->display->dio_f & OFFB)
    s += fb->row[fb->row_i]->offset;

  if (ed->display->overflow && fb->prev_first
      && fb->prev_first->lp == lp && ed->state->is_utf8)

    /*
     * XXX: For multibyte streams, adjust fb->rem_chars upward.
     * See test zb7.tso.
     */
    while (utf8_strlen (s, fb->rem_chars) == ERR)
      ++fb->rem_chars;

  /* Offset of paged line if form feed (\f) is split across pages. */
  for (; fb->ff_offset; --fb->ff_offset)
    if (fb_putc (' ', fb, ed) < 0)
      return ERR;

  for (; fb->rem_chars && !fb->is_full; fb->rem_chars -= csize, s += csize)
    {
      if (!(ed->display->dio_f & LIST)
          || (31 < *s && *s < 127 && *s != '\\' && *s != '$'))
        {

          /*
           * If ANSI color support is enabled, exclude SGR sequences
           * from column bookkeeping.
           */
          if (ed->exec->opt & ANSI_COLOR && (sgr_len = sgr_span (s)) > 0)
            {
              for (; sgr_len--; --fb->rem_chars, ++s)
                if (fb_putc ((unsigned) *s, fb, ed) < 0)
                  return ERR;
              csize = 0;
              continue;
            }

          if (!ed->state->is_utf8) {
            if (fb_putc (*s, fb, ed) < 0)
              return ERR;
          } else if (fb_putwc (s, fb->rem_chars, fb, ed) < 0)
            return ERR;
          col += GET_WCHAR_WIDTH (s, fb->rem_chars, col);

          /*
           * As with cat(1), form feed (\f) does not force a new page,
           * only a new line. Any following text continues on the next
           * line -- indented to the current column.
           */
          if ((form_feed = (*s == '\f')))
            inc_mod_fb_row (lp, addr, ed->display->dio_f & ZBWD, fb, ed);
        }
      else
        {
          /*
           * If control char has C escape sequence, print literal
           * escape sequence - e.g., ASCII bel as `\a'.
           */
          if (*s && (cp = strchr (control_char, (unsigned) *s)))
            {
              FB_PUTS (escape_sequence[cp - control_char], fb, ed);
              col += 2;
            }

          /* Print other chars as octal escapes - i.e., \ooo. */
          else if (fb_putc ('\\', fb, ed) < 0
                   || fb_putc ((((unsigned) *s & 0300) >> 6) + '0', fb, ed) < 0
                   || fb_putc ((((unsigned) *s & 070) >> 3) + '0', fb, ed) < 0
                   || fb_putc (((unsigned) *s & 07) + '0', fb, ed) < 0)
            return ERR;
          else
            col += 4;
        }

      /* Convert fb->rem_chars to units of wide chars. */
      len = ed->state->is_utf8 ? utf8_strlen (s, fb->rem_chars) : fb->rem_chars;

      /*
       * When listing text -- i.e., ed->display->dio_f & LIST --
       * first try simple heuristic to split lines on word boundaries
       * for legibility; otherwise, split lines at the right margin,
       * which starts one tab stop from the right edge of the window.
       */
      if ((col >= fb->columns && len > 1)
          || (ed->display->dio_f & LIST
              && ((!isalnum ((unsigned) *s)
                   && col >= fb->columns - (RIGHT_MARGIN << 1)
                   && len > 2)
                  || (col >= fb->columns - RIGHT_MARGIN
                      && len > 1))))

        {
          if (ed->display->dio_f & LIST)
            FB_PUTS (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)
                     ? "" : "\\\n", fb, ed);
          inc_mod_fb_row (lp, addr, ed->display->dio_f & ZBWD, fb, ed);
          col = 0;
        }

      csize = ed->state->is_utf8 ? utf8_char_size (s, fb->rem_chars) : 1;
    }

  if (!(ed->display->dio_f & LIST) && !fb->rem_chars)
    FB_PUTS ("\n", fb, ed);
  else if (ed->display->dio_f & LIST && !fb->rem_chars)
    {
      if (ed->display->dio_f & (ZBWD | ZHBW)
          && fb->prev_first && fb->prev_first->offset)
        FB_PUTS (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)
                 ? "\n" : "\\\n", fb, ed);
      else
        FB_PUTS ("$\n", fb, ed);
    }
  fb->ff_offset = form_feed && ed->display->dio_f & ZBWD ? col : 0;
  return 0;
}


/* fb_putc: Add a character to the frame buffer. */
static int
fb_putc (int c, ed_frame_buffer_t *fb, ed_buffer_t *ed)
{
  ed_frame_node_t *rp = fb->row[fb->row_i];

  REALLOC_THROW (rp->text, rp->text_size, rp->text_i + 1, ERR, ed);

  /* Don't return c, which might be signed. */
  rp->text[rp->text_i++] = c;
  return 0;
}


/* fb_putwc: Add a wide character to the frame buffer. */
static int
fb_putwc (char *s, size_t len, ed_frame_buffer_t *fb, ed_buffer_t *ed)
{
  ed_frame_node_t *rp = fb->row[fb->row_i];
  size_t i;

  if ((i = utf8_char_size (s, len)) == 0)
    return ERR;

  REALLOC_THROW (rp->text, rp->text_size, rp->text_i + i, ERR, ed);

  /* Don't return char, which might be signed. */
  while (i--)
    rp->text[rp->text_i++] = *s++;
  return 0;
}


/* display_frame_buffer: Write frame buffer to standard output. */
static int
display_frame_buffer (const ed_frame_buffer_t *fb, ed_buffer_t *ed)
{
  ed_frame_node_t *rp;
  size_t m;
  int n;
  int row_i;
  int row_count;

  row_count = (fb->last_i >= fb->first_i
               ? fb->last_i - fb->first_i + 1
               : fb->rows - (fb->first_i - fb->last_i));
  for (n = 0; n < row_count; ++n)
    {
      row_i = (fb->first_i + n) % (fb->rows - 1);
      rp = fb->row[row_i];
      for (m = 0; m < rp->text_i; ++m)
        putchar (rp->text[m]);
    }
  if ((ed->display->dio_f & (ZBWD | ZFWD))
      && n && rp->text[rp->text_i - 1] != '\n')
    putchar ('\n');
  return 0;
}


/* inc_mod_fb_row: Increment fb->row_i and calculate text offset. */
static void
inc_mod_fb_row (ed_line_node_t *lp, off_t addr, int ok_to_wrap,
                ed_frame_buffer_t *fb, ed_buffer_t *ed)
{
  size_t offset = 0;

  INC_FB_ROW (ok_to_wrap, fb);

  if (!fb->is_full)
    {
      if (fb->prev_first && fb->prev_first->lp == lp)
        {
          /* XXX: See tests zb[12].tso. */
          offset = (ok_to_wrap ? fb->prev_first->offset : lp->len);
          if (offset < fb->rem_chars - 1)
            offset = lp->len - fb->rem_chars + 1;
          else
            offset -= fb->rem_chars - 1;
        }
      else
        {
          /* XXX: See tests zb[34].tso. */
          offset = (ok_to_wrap && fb->prev_first ? fb->prev_first->offset
                    : lp->len - fb->rem_chars + 1);
          if (offset < fb->rem_chars - 1)
            offset = lp->len - fb->rem_chars + 1;
        }

      INIT_FB_ROW (lp, addr, offset, fb);
    }
}


/* Init_frame_buffer: Initialize ed_frame_buffer. */
static int
init_frame_buffer (ed_frame_buffer_t *fb, ed_buffer_t *ed)
{
  static char *fp = NULL;
  static size_t fp_size = 0;

  size_t size;
  int n;

  spl1 ();

  if (fb->rows != ed->display->ws_row
      || fb->columns != ed->display->ws_col)
    {
      fb->row_i = 0;
      for (n = 0; n < fb->rows - 1; ++n)
        if (fb->row[n]->text)
          {
            free (fb->row[n]->text);
            init_frame_node (fb->row[n]);
          }
      REALLOC_THROW (fp, fp_size,
                     (ed->display->ws_row - 1) * sizeof (ed_frame_node_t *),
                     ERR, ed);
      fb->row = (ed_frame_node_t **) fp;
      for (n = max (0, fb->rows - 1); n < ed->display->ws_row - 1; ++n)
        {
          size = 0;
          fb->row[n] = NULL;
          REALLOC_THROW (fb->row[n], size, sizeof (ed_frame_node_t), ERR, ed);
          init_frame_node (fb->row[n]);
        }

      fb->rows = ed->display->ws_row;
      fb->columns = ed->display->ws_col;
    }

  /*
   * Don't reset row_i if scrolling forward in half-pages. Append to
   * end of frame buffer instead.
   */
  if (!(ed->display->dio_f & ZHFW))
    fb->row_i = 0;
  fb->fill_i = (fb->row_i + (fb->rows >> 1)) % (fb->rows - 1);
  fb->wraps = !!fb->row_i;
  spl0 ();
  return 0;
}


/* init_frame_node: Initialize ed_frame_node_t structure. */
static void
init_frame_node (ed_frame_node_t *rp)
{
  rp->addr = 0;
  rp->lp = NULL;
  rp->offset = 0;
  rp->text = NULL;
  rp->text_size = 0;
  rp->text_i = 0;
}


/* dup_frame_node: Return a pointer to a copy of a row node. */
static ed_frame_node_t *
dup_frame_node (const ed_frame_node_t *rp, ed_buffer_t *ed)
{
  ed_frame_node_t *np;

  /* Assert: spl1 () */

  if (!(np = (ed_frame_node_t *) malloc (ED_FRAME_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      return NULL;
    }
  np->addr = rp->addr;
  np->lp = rp->lp;
  np->offset = rp->offset;
  np->text = rp->text;
  np->text_size = rp->text_size;
  np->text_i = rp->text_i;
  return np;
}
#endif  /* WANT_ED_SCROLL */

/* sgr_span: Return length of ANSI SGR sequence, otherwise 0.
 *           Cf. <https://github.com/termstandard/colors>
 */
static unsigned int
sgr_span (const char *s)
{
  const char *t = s;

  if (*s != ESCAPE || *++s != '[')
    return 0;
  while (isdigit (*++s) || *s == ';' || *s == ':')
    ;
  return *s == 'm' ? s - t + 1 : 0;
}
