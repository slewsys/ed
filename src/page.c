/* page.c: Display routines for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2013-07-20 <alm@buttercup.local>

   This file is part of ed. */

#include "ed.h"


/* Static function declarations. */
static int display_frame_buffer __P ((const ed_frame_buffer_t *));
static ed_frame_node_t *dup_frame_node __P ((const ed_frame_node_t *,
                                             ed_state_t *));
static int fb_putc __P ((int, ed_frame_buffer_t *, ed_state_t *));
static int init_frame_buffer __P ((ed_frame_buffer_t *, int, ed_state_t *));
static void init_frame_node __P ((ed_frame_node_t *));
static int put_frame_buffer_line __P ((ed_line_node_t *, off_t, unsigned,
                                       ed_frame_buffer_t *, ed_state_t *));
static unsigned int sgr_span __P ((const char *));


/* INIT_FB_ROW: Initialize frame buffer row. */
#define INIT_FB_ROW(bp, caddr, off, fb)                                       \
  do                                                                          \
    {                                                                         \
      (fb->row + fb->row_i)->lp = (bp);                                       \
      (fb->row + fb->row_i)->addr = (caddr);                                  \
      (fb->row + fb->row_i)->offset = (off);                                  \
      (fb->row + fb->row_i)->text_i = 0;                                      \
    }                                                                         \
  while (0)

/* INC_FB_ROW: Increment frame buffer row. */
#define INC_FB_ROW(ok_to_wrap, fb)                                            \
  do                                                                          \
    {                                                                         \
      (fb)->row_i = ((fb)->row_i + 1) % ((fb)->rows - 1);                     \
      (fb)->wraps |= !(fb)->row_i;                                            \
      (fb)->is_full =                                                         \
        (io_f & ZHFW                                                          \
         ? (fb)->row_i == ((fb)->fill_i + 1) % ((fb)->rows - 1)               \
         :  ((fb)->wraps && !(ok_to_wrap)));                                  \
    }                                                                         \
  while (0)

#define RESET_FB_PREV(fb, ed)                                                 \
  do                                                                          \
    {                                                                         \
      if ((fb)->prev_first)                                                   \
        {                                                                     \
          free ((fb)->prev_first);                                            \
          free ((fb)->prev_last);                                             \
          (fb)->prev_first = NULL;                                            \
          (fb)->prev_last = NULL;                                             \
        }                                                                     \
      (ed)->display.underflow = 0;                                            \
      (ed)->display.overflow = 0;                                             \
    }                                                                         \
  while (0)


/* display_lines: Print a range of lines. Return status (<= 0). */
int
display_lines (from, to, io_f, ed)
     off_t from;
     off_t to;
     unsigned int io_f;         /* I/O flags */
     ed_state_t *ed;
{
  static ed_frame_buffer_t frame_buffer = { 0 };

  ed_frame_buffer_t *fb = &frame_buffer;
  ed_line_node_t *bp, *ep;
  off_t lc = 0;                 /* Line counter */
  size_t rem_chars = 0;         /* Remaining chars of line across pages. */
  int flags = 0;
  int status;

  /* Trivially allow printing empty buffer. */
  if (!ed->buf[0].addr_last)
    return 0;

  /* If not scrolling, ignore frame buffer status. */
  if (!(io_f & (ZFWD | ZBWD)))
    {
      ed->display.underflow = 0;
      ed->display.overflow = 0;
    }

  ep = get_line_node (INC_MOD (to, ed->buf[0].addr_last), ed);
  bp = get_line_node (from, ed);

 top:
  if ((status = init_frame_buffer (fb, io_f, ed)) < 0)
    return status;

  for (fb->is_full = 0; bp != ep && !fb->is_full; bp = bp->q_forw, ++lc)
    {
      INIT_FB_ROW (bp, from + lc, 0, fb);

      /* If scrolling forward and last line of previous page was
         truncated, then print remainder of that line after first
         part. */
      if ((io_f & ZFWD) && ed->display.underflow
          && fb->prev_last && bp == fb->prev_last->lp)
        {
          fb->row->offset = bp->len - fb->rem_chars;
          flags = (io_f | OFFB) & ~NMBR;
        }

      /* If scrolling backward and first line of previous page was
         truncated, then print beginning of that line before first
         part. */
      else if ((io_f & ZBWD) && ed->display.overflow
               && fb->prev_first && bp == fb->prev_first->lp)
        {
          fb->rem_chars = fb->prev_first->offset;

          /* Preserve length of last (source) line overflow. */
          rem_chars = bp->len - fb->rem_chars;
          fb->ff_offset = 0;
          flags = io_f;
        }
      else
        {
          fb->rem_chars = bp->len;
          fb->ff_offset = 0;
          flags = io_f;
        }
      if ((status =
           put_frame_buffer_line (bp, from + lc, flags, fb, ed)) < 0)
        return status;
      if (!fb->is_full)
        INC_FB_ROW (io_f & ZBWD, fb);
    }

  /* If final forward page not full, then scroll back from last line. */
  if ((io_f & ZFWD) && to == ed->buf[0].addr_last && !fb->wraps
      && (ed->display.underflow || from != 1))
    {
      /* Ignore frame buffer status. */
      RESET_FB_PREV (fb, ed);
      return display_lines (max (1, ed->buf[0].addr_last - fb->rows + 2),
                            ed->buf[0].addr_last, (io_f | ZBWD) & ~ZFWD, ed);
    }

  /* If final backward page not full, then scroll forward from first line. */
  if ((io_f & ZBWD) && from == 1 && !fb->wraps
      && (ed->display.overflow || to != ed->buf[0].addr_last))
    {
      /* Ignore frame buffer status. */
      RESET_FB_PREV (fb, ed);
      return display_lines (1, min (ed->buf[0].addr_last, fb->rows - 1),
                            (io_f | ZFWD) & ~ZBWD, ed);
    }

  spl1 ();

  /* frame buffer not empty ... */
  if (lc)
    {
      RESET_FB_PREV (fb, ed);

      /* Calculate frame buffer row index of the first and last lines
         to print. If paging forward, then first line is given and its
         row index is trivially 0. If paging backward, then only the
         last line is known. In this case, a starting point is
         estimated and the frame buffer is filled, wrapping around top
         of buffer as necessary, until the end point in the source
         text is reached. */
      if (io_f & ZHFW)
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
      if (!(fb->prev_first = dup_frame_node (fb->row + fb->first_i, ed))
          || !(fb->prev_last = dup_frame_node (fb->row + fb->last_i, ed)))
        {
          spl0 ();
          return ERR;
        }

      /* Line address of first row. */
      ed->display.page_addr = fb->prev_first->addr;

      /* Assign old prev_first `len - offset' as rem_chars to last
         line of current page. */
      if ((io_f & ZBWD))
        fb->rem_chars = rem_chars;

      /* If next command is `scroll forward', then current page
         becomes `page above', and ed->display.underflow is set if
         last line of current page is truncated. */
      ed->display.underflow = !!fb->rem_chars;

      /* If next command is `scroll backward', then current page becomes
         `page below', and ed->display.overflow is set if first line
         of current page has non-zero offset. */
      ed->display.overflow = !!fb->prev_first->offset;

      if (!ed->display.off  && (status = display_frame_buffer (fb)) < 0)
        {
          spl0 ();
          return status;
        }
    }
  spl0 ();

  if (!(io_f & (ZBWD | ZFWD)))
    {
      /* More lines to print. */
      if (from + lc - 1 != to && !ed->display.underflow)
        goto top;

      /* More of current line to print. */
      if (ed->display.underflow)
        {
          bp = bp->q_back;
          --lc;
          goto top;
        }
    }

  /* Don't update current address until printing completed -- per bwk. */
  ed->buf[0].dot = from + lc - 1;
  return 0;
}


/* Init_frame_buffer: Initialize ed_frame_buffer. */
static int
init_frame_buffer (fb, io_f, ed)
     ed_frame_buffer_t *fb;
     int io_f;
     ed_state_t *ed;
{
  static char *fp = NULL;
  static size_t fp_size = 0;

  int n;

  spl1 ();

  if (fb->rows != window_rows
      || fb->columns != window_columns)
    {
      fb->row_i = 0;
      for (n = 0; n < fb->rows - 1; ++n)
        if ((fb->row + n)->text)
          {
            free ((fb->row + n)->text);
            init_frame_node (fb->row + n);
          }
      REALLOC_THROW (fp, fp_size,
                     (window_rows - 1) * sizeof (ed_frame_node_t), ERR, ed);
      fb->row = (ed_frame_node_t *) fp;
      for (n = max (0, fb->rows - 1); n < window_rows - 1; ++n)
        init_frame_node (fb->row + n);

      fb->rows = window_rows;
      fb->columns = window_columns;
    }

  /* Don't reset row_i if scrolling forward. Append to end of frame
     buffer instead. */
  if (!(io_f & ZHFW))
    fb->row_i = 0;
  fb->fill_i = (fb->row_i + (fb->rows >> 1)) % (fb->rows - 1);
  fb->wraps = !!fb->row_i;
  spl0 ();
  return 0;
}


/* init_frame_node: Initialize ed_frame_node_t structure. */
static void
init_frame_node (rp)
     ed_frame_node_t *rp;
{
  rp->addr = 0;
  rp->lp = NULL;
  rp->offset = 0;
  rp->text = NULL;
  rp->text_size = 0;
  rp->text_i = 0;
}


#define RIGHT_MARGIN TAB_WIDTH


/* CHAR_WIDTH: Assert: Tabstops are of fixed length. */
#define CHAR_WIDTH(i, c)                                                      \
  ((i) == '\b' ? -1                                                           \
   : (i) == '\f' ? 0                                                          \
   : (i) == '\r' ? -(c)                                                       \
   : (i) == '\t' ? TAB_WIDTH - ((c) - ((c) / TAB_WIDTH) * TAB_WIDTH)          \
   : (i) < '\040' ? 0                                                         \
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


/* INC_MOD_FB_ROW: Increment fb->row_i modulus (fb->rows - 1). */
#define INC_MOD_FB_ROW(lp, addr,  ok_to_wrap, fb)                             \
  do                                                                          \
    {                                                                         \
      INC_FB_ROW ((ok_to_wrap), (fb));                                        \
      if (!(fb)->is_full)                                                     \
        {                                                                     \
          size_t _offset = ((lp)->len - ((fb)->rem_chars - 1)                 \
                            - ((ok_to_wrap) && (fb)->prev_first               \
                               ? (fb)->prev_first->offset : 0));              \
          INIT_FB_ROW ((lp), (addr), _offset, (fb));                          \
        }                                                                     \
    }                                                                         \
  while (0)

/* put_frame_buffer_line: Print text to frame buffer. */
static int
put_frame_buffer_line (lp, addr, io_f, fb, ed)
     ed_line_node_t *lp;        /* Line node pointer */
     off_t addr;                /* Line no. */
     unsigned io_f;             /* I/O flags */
     ed_frame_buffer_t *fb;
     ed_state_t *ed;
{
  static off_t addr_last = 0;
  static int addr_last_len = 0;  /* strlen (addr_last) */
  static char fmt[9] = { '\0' }; /* Line no. format string, e.g., "% 3lld\t" */

  off_t n;
  ed_frame_node_t *rp;
  char line_no[OFF_T_LEN + TAB_WIDTH + 1];
  char *cp, *s;
  size_t col = 0;
  size_t len = 0;
  unsigned int sgr_len = 0;     /* length of ANSI sgr sequence */
  int form_feed = 0;
  /* int left_margin = 0;          /\* addr_last_len - 1 *\/ */

  /* As per SUSv3, 2004, the `$' (dollar sign) character is output by
     the `l' (literal) command as the escape sequence `\$' */
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
  if ((io_f & NMBR))
    {
      /* If addr_last has changed, update format string. */
      if (addr_last != ed->buf[0].addr_last)
        {
          n = addr_last = ed->buf[0].addr_last;
          for (addr_last_len = 0; n /= 10; ++addr_last_len)
            ;
          ++addr_last_len;

          /* Set line numbering format string. */
          sprintf (fmt, "%%%d" OFF_T_FORMAT_STRING "\t", addr_last_len);
        }

      /* Evaluate format string, fmt, with address, addr, of dot. */
      len = snprintf (line_no, OFF_T_LEN + TAB_WIDTH + 1, fmt, addr);
      FB_PUTS (line_no, fb, ed);
      col = addr_last_len;
      col += CHAR_WIDTH ('\t', addr_last_len);
    }

  if (!(s = get_buffer_line (lp, ed)))
    return ERR;
  if ((io_f & OFFB))
    s += fb->row->offset;

  /* Offset of paged line if form feed (\f) is split across pages. */
  for (; fb->ff_offset; --fb->ff_offset)
    if (fb_putc (' ', fb, ed) < 0)
      return ERR;

  for (; fb->rem_chars && !fb->is_full; --fb->rem_chars, ++s)
    {
      if (!(io_f & LIST) || (31 < *s && *s < 127 && *s != '\\' && *s != '$'))
        {

          /* If ANSI color support is enabled, exclude SGR sequences
             from column bookkeeping. */
          if (ed->exec.opt & ANSI_COLOR && (sgr_len = sgr_span (s)) > 0)
            {
              for (; sgr_len--; --fb->rem_chars, ++s)
                if (fb_putc ((unsigned) *s, fb, ed) < 0)
                  return ERR;
              continue;
            }

          if (fb_putc ((unsigned) *s, fb, ed) < 0)
            return ERR;
          col += CHAR_WIDTH ((unsigned) *s, col);

          /* As with cat(1), form feed (\f) does not force a new page,
             only a new line. Any following text continues on the next
             line -- indented to the current column. */
          if (form_feed = (*s == '\f'))
            INC_MOD_FB_ROW (lp, addr, io_f & ZBWD, fb);
        }
      else
        {
          /* If control char has C escape sequence, print literal
             escape sequence - e.g., ASCII bel as `\a'. */
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

      /* When listing text -- i.e., (io_f & LIST) -- first try simple
         heuristic to split lines on word boundaries for legibility;
         otherwise, split lines at the right margin, which starts one
         tab stop from the right edge of the window. */
      if ((col >= fb->columns && fb->rem_chars > 1)
          || ((io_f & LIST)
              && ((!isalnum ((unsigned) *s)
                   && col >= fb->columns - (RIGHT_MARGIN << 1)
                   && fb->rem_chars > 2)
                  || (col >= fb->columns - RIGHT_MARGIN
                      && fb->rem_chars > 1))))

        {
          if ((io_f & LIST))
            FB_PUTS ("\\\n", fb, ed);
          INC_MOD_FB_ROW (lp, addr, io_f & ZBWD, fb);
          col = 0;
        }
    }

  if (!(rp = fb->row + fb->row_i)->text)
    FB_PUTS ("\n", fb, ed);
  if (!(io_f & LIST) && col < fb->columns && rp->text[rp->text_i - 1] != '\n')
    FB_PUTS ("\n", fb, ed);
  else if ((io_f & LIST) && !fb->rem_chars)
    {
      if ((io_f & (ZBWD | ZHBW)) && fb->prev_first && fb->prev_first->offset)
        FB_PUTS ("\\\n", fb, ed);
      else
        FB_PUTS ("$\n", fb, ed);
    }
  fb->ff_offset = form_feed && (io_f & ZBWD) ? col : 0;
  return 0;
}


/* fb_putc: Add a character to the frame buffer. */
static int
fb_putc (c, fb, ed)
     int c;
     ed_frame_buffer_t *fb;
     ed_state_t *ed;
{
  ed_frame_node_t *rp = fb->row + fb->row_i;

  REALLOC_THROW (rp->text, rp->text_size, rp->text_i + 1, ERR, ed);
  return *((unsigned char *) rp->text + rp->text_i++) = c;
}


/* display_frame_buffer: Write frame buffer to standard output. */
static int
display_frame_buffer (fb)
     const ed_frame_buffer_t *fb;
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
      rp = fb->row + row_i;
      for (m = 0; m < rp->text_i; ++m)
        putchar (*(rp->text + m));
    }
  if (isatty (1) && n && *(rp->text + rp->text_i - 1) != '\n')
    putchar ('\n');
  return 0;
}


/* dup_frame_node: Return a pointer to a copy of a row node. */
static ed_frame_node_t *
dup_frame_node (rp, ed)
     const ed_frame_node_t *rp;
     ed_state_t *ed;
{
  ed_frame_node_t *np;

  /* Assert: spl1 () */

  if (!(np = (ed_frame_node_t *) malloc (ED_FRAME_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Memory exhausted");
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


/* sgr_span: Return length of ANSI sgr sequence, otherwise 0. */
static unsigned int
sgr_span (s)
     const char *s;
{
  const char *t = s;

  if (*s != ESCAPE || *++s != '[')
    return 0;
  while (isdigit (*++s) || *s == ';')
    ;
  return *s == 'm' ? s - t + 1 : 0;
}

/*
 * Local variables:
 * mode: c
 * eval: (add-hook 'write-file-functions 'time-stamp)
 * time-stamp-start: "Last modified: "
 * time-stamp-format: "%:y-%02m-%02d <%u@%h>"
 * time-stamp-end: "$"
 * End:
 */
