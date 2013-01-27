/* page.c: Display routines for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2012-12-11 <alm@buttercup.local>
   
   This file is part of ed. */

#include "ed.h"


/* Static function declarations. */
static int display_frame_buffer __P ((int, int));
static ed_frame_node_t *dup_frame_node __P ((const ed_frame_node_t *,
                                          ed_state_t *));
static int fb_putchar __P ((int, ed_state_t *));
static int init_frame_buffer __P ((struct ed_frame_buffer *,
                                   ed_state_t *));
static void init_frame_node __P ((ed_frame_node_t *));
static int put_frame_buffer_line __P ((ed_line_node_t *, off_t, unsigned,
                                       int, ed_state_t *));
static unsigned int sgr_span __P ((const char *));


/* Global declarations */
struct ed_frame_buffer fb = { 0, 0, 0 };      /* frame buffer */


/* INIT_FB_ROW: Initialize frame buffer row. */
#define INIT_FB_ROW(bp, caddr, off)                                           \
  do                                                                          \
    {                                                                         \
      (fb.row + fb.row_i)->lp = (bp);                                         \
      (fb.row + fb.row_i)->addr = (caddr);                                    \
      (fb.row + fb.row_i)->offset = (off);                                    \
      (fb.row + fb.row_i)->text_i = 0;                                        \
    }                                                                         \
  while (0)

/* INC_FB_ROW: Increment frame buffer row. */
#define INC_FB_ROW(ok_to_wrap)                                                \
  do                                                                          \
    {                                                                         \
      ++fb.row_i;                                                             \
      fb.row_i %= fb.rows - 1;                                                \
      fb.wraps |= !fb.row_i;                                                  \
      fb.is_full = fb.wraps && !(ok_to_wrap);                                 \
    }                                                                         \
  while (0)

#define RESET_FB_PREV()                                                       \
  do                                                                          \
    {                                                                         \
      if (fb.prev_first)                                                      \
        {                                                                     \
          free (fb.prev_first);                                               \
          free (fb.prev_last);                                                \
          fb.prev_first = NULL;                                               \
          fb.prev_last = NULL;                                                \
        }                                                                     \
      ed->display.underflow = 0;                                              \
      ed->display.overflow = 0;                                               \
    }                                                                         \
  while (0)  

/* display_lines: Print a range of lines. */
int
display_lines (from, to, io_f, ed)
     off_t from;
     off_t to;
     unsigned io_f;             /* I/O flags */
     ed_state_t *ed;
{
  ed_line_node_t *bp, *ep;
  off_t lc = 0;                 /* Line counter */
  off_t n;
  size_t rem_chars = 0;         /* Remaining chars of line across pages. */
  int first = 0;                /* Index of first row in page */
  int last = 0;                 /* Index of last row in page */
  int flags = 0;
  int status;
  int width = 1;

  /* Trivially allow printing empty buffer. */
  if (!ed->buf[0].addr_last)
    return 0;

  /* If line no's wanted, then calc line no. length. */
  if (io_f & NMBR)
    for (n = to; n /= 10;)
      ++width;

  /* Ignore frame buffer status. */
  if (!(io_f & (ZFWD | ZBCK)))
    {
      ed->display.underflow = 0;
      ed->display.overflow = 0;
    }

  ep = get_line_node (INC_MOD (to, ed->buf[0].addr_last), ed->buf);
  bp = get_line_node (from, ed->buf);

 top:
  if ((status = init_frame_buffer (&fb, ed)) < 0)
    return status;

  for (fb.is_full = 0; bp != ep && !fb.is_full; bp = bp->q_forw, ++lc)
    {
      INIT_FB_ROW (bp, from + lc, 0);

      /* If paging/scrolling forward and last line of previous page
         was truncated, then print remainder of that line at top of
         current page. */
      if (!(io_f & ZBCK) && ed->display.underflow
          && fb.prev_last && bp == fb.prev_last->lp)
        {
          fb.row->offset = bp->len - fb.rem_chars;
          flags = (io_f | OFFB) & ~NMBR;
        }

      /* If paging/scrolling backward and first line of previous page
         was truncated, then print first part of that line at bottom
         of current page. */
      else if (!(io_f & ZFWD) && ed->display.overflow
               && fb.prev_first && bp == fb.prev_first->lp)
        {
          fb.rem_chars = fb.prev_first->offset;

          /* Preserve length of last (source) line overflow. */
          rem_chars = bp->len - fb.rem_chars;
          fb.ff_offset = 0;
          flags = io_f;
        }
      else
        {
          fb.rem_chars = bp->len;
          fb.ff_offset = 0;
          flags = io_f;
        }
      if ((status =
           put_frame_buffer_line (bp, from + lc, flags, width, ed)) < 0)
        return status;
      if (!fb.is_full)
        INC_FB_ROW ((io_f & ZBCK));
    }

  /* If final forward page not full, then page back from last line. */
  if ((io_f & ZFWD) && to == ed->buf[0].addr_last && !fb.wraps
      && (ed->display.underflow || from != 1))
    {
      /* Ignore frame buffer status. */
      RESET_FB_PREV ();
      return display_lines (max (1, ed->buf[0].addr_last - fb.rows + 2),
                            ed->buf[0].addr_last, (io_f | ZBCK) & ~ZFWD, ed);
    }

  /* If final backward page not full, then page forward from first line. */
  if ((io_f & ZBCK) && from == 1 && !fb.wraps
      && (ed->display.overflow || to != ed->buf[0].addr_last))
    {
      /* Ignore frame buffer status. */
      RESET_FB_PREV ();
      return display_lines (1, min (ed->buf[0].addr_last, fb.rows - 1),
                            (io_f | ZFWD) & ~ZBCK, ed);
    }


  spl1 ();

  /* If the frame buffer is not empty ... */
  if (fb.row_i || fb.wraps)
    {
      RESET_FB_PREV ();

      /* Calculate frame buffer row index of the first and last lines
         to print. If paging forward, then first line is given and row
         index of is trivially 0. If paging backward, then only the
         last line is known. In this case, a starting point is
         estimated and the frame buffer is filled, wrapping around top
         of buffer as necessary, until the end point in the source text
         is reached. */
      first = fb.wraps ? fb.row_i : 0;
      last = (fb.wraps ? (first > 0 ? first - 1 : fb.rows - 2) : fb.row_i - 1);
      if (!(fb.prev_first = dup_frame_node (fb.row + first, ed))
          || !(fb.prev_last = dup_frame_node (fb.row + last, ed)))
        {
          spl0 ();
          return ERR;
        }

      ed->display.page_addr = fb.prev_first->addr;

      if ((io_f & ZBCK))
        fb.rem_chars = rem_chars;

      /* If next command is page forward, then current page becomes "page
         above", and ed->display.underflow is set if last line of
         current page is truncated. */
      ed->display.underflow = !!fb.rem_chars;

      /* If next command is page backward, then current page becomes "page
         below", and ed->display.overflow is set if first line of
         current page has non-zero offset. */
      ed->display.overflow = !!fb.prev_first->offset;

      if ((status = display_frame_buffer (first, last)) < 0)
        {
          spl0 ();
          return status;
        }

    }
  spl0 ();

  if (!(io_f & (ZBCK | ZFWD)))
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


/* init_frame_buffer: Initialize ed_frame_buffer. */
static int
init_frame_buffer (fb, ed)
     struct ed_frame_buffer *fb;
     ed_state_t *ed;
{
  static char *fp = NULL;
  static size_t fp_size = 0;

  int n;

  spl1 ();
  fb->row_i = 0;
  fb->wraps = 0;

  if (fb->rows != window_rows
      || fb->columns != window_columns)
    {
      for (n = 0; n < fb->rows - 1; ++n)
        if ((fb->row + n)->text)
          {
            free ((fb->row + n)->text);
            init_frame_node (fb->row + n);
          }
      REALLOC_THROW (fp, fp_size,
                     (window_rows - 1) * sizeof (ed_frame_node_t), ERR);
      fb->row = (ed_frame_node_t *) fp;
      for (n = max (0, fb->rows - 1); n < window_rows - 1; ++n)
        init_frame_node (fb->row + n);

      fb->rows = window_rows;
      fb->columns = window_columns;
    }
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


#define LEFT_MARGIN 0
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
#define FB_PUTS(s, ed)                                                        \
  do                                                                          \
    {                                                                         \
      int _i = 0;                                                             \
      while (*(s + _i))                                                       \
        if (fb_putchar (*((s) + _i++), (ed)) < 0)                             \
          return ERR;                                                         \
    }                                                                         \
  while (0)


/* COND_WRAP_FB_ROW: Increment fb.row_i and conditionally break. */
#define COND_WRAP_FB_ROW(ok_to_wrap)                                          \
  do                                                                          \
    {                                                                         \
      INC_FB_ROW (ok_to_wrap);                                                \
      if (!fb.is_full)                                                        \
        INIT_FB_ROW (lp, addr, (lp->len - (fb.rem_chars - 1)                  \
                                - ((ok_to_wrap) && fb.prev_first              \
                                   ? fb.prev_first->offset : 0)));            \
    }                                                                         \
  while (0)

/* put_frame_buffer_line: Print text to frame buffer. */
static int
put_frame_buffer_line (lp, addr, io_f, width, ed)
     ed_line_node_t *lp;           /* Line pointer */
     off_t addr;                /* Line address */
     unsigned io_f;             /* I/O flags */
     int width;                 /* strlen (ed->buf[0].addr_last) */
     ed_state_t *ed;
{
  char fmt[OFF_T_LEN + 7];     /* OFF_T_LEN +
                                  strlen ("%" OFF_T_FORMAT_STRING "\t") + 1 */
  char nstr[OFF_T_LEN + TAB_WIDTH + 1];
  char *cp, *s;
  size_t col = 0;
  size_t len = 0;
  unsigned int sgr_len = 0;     /* length of ANSI sgr sequence */
  int form_feed = 0;
  int left_margin = width - 1;

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
      sprintf (fmt, "%%%d" OFF_T_FORMAT_STRING "\t", width);
      len = snprintf (nstr, OFF_T_LEN + TAB_WIDTH + 1, fmt, addr);
      FB_PUTS (nstr, ed);
      col = width +  CHAR_WIDTH ('\t', width);
    }

  if (!(s = get_buffer_line (lp, ed)))
    return ERR;
  if ((io_f & OFFB))
    s += fb.row->offset;

  /* Offset of paged line if form feed (\f) is split across pages. */
  for (; fb.ff_offset; --fb.ff_offset)
    if (fb_putchar (' ', ed) < 0)
      return ERR;

  for (; fb.rem_chars && !fb.is_full; --fb.rem_chars, ++s)
    {
      if (!(io_f & LIST) || (31 < *s && *s < 127 && *s != '\\' && *s != '$'))
        {

          /* If ANSI color support is enabled, exclude SGR sequences
             from column bookkeeping. */
          if (ed->opt & ANSI_COLOR && (sgr_len = sgr_span (s)) > 0)
            {
              for (; --sgr_len; --fb.rem_chars, ++s)
                if (fb_putchar ((unsigned) *s, ed) < 0)
                  return ERR;
              if (fb_putchar ((unsigned) *s, ed) < 0)
                return ERR;
              continue;
            }

          if ((col += CHAR_WIDTH ((unsigned) *s, col)) < left_margin)
            col = left_margin;
          else if (fb_putchar ((unsigned) *s, ed) < 0)
            return ERR;


          /* As with cat(1), form feed (\f) does not force a new page,
             only a new line. Any following text continues on the next
             line -- indented to the current column. */
          if (form_feed = (*s == '\f'))
            COND_WRAP_FB_ROW ((io_f & ZBCK));
        }
      else
        {
          /* If control char has C escape sequence, print literal
             escape sequence - e.g., ASCII bel as `\a'. */
          if (*s && (cp = strchr (control_char, (unsigned) *s)))
            {
              FB_PUTS (escape_sequence[cp - control_char], ed);
              col += 2;
            }

          /* Print other chars as octal escapes - i.e., \ooo. */
          else if (fb_putchar ('\\', ed) < 0
                   || fb_putchar ((((unsigned) *s & 0300) >> 6) + '0', ed) < 0
                   || fb_putchar ((((unsigned) *s & 070) >> 3) + '0', ed) < 0
                   || fb_putchar (((unsigned) *s & 07) + '0', ed) < 0)
            return ERR;
          else
            col += 4;
        }

      /* When listing text -- i.e., (io_f & LIST) -- first try simple
         heuristic to split lines on word boundaries for legibility;
         otherwise, split lines at the right margin, which starts one
         tab stop from the right edge of the window. */
      if ((col >= fb.columns && fb.rem_chars > 1)
          || ((io_f & LIST)
              && ((!isalnum ((unsigned) *s)
                   && col >= fb.columns - (RIGHT_MARGIN << 1)
                   && fb.rem_chars > 2)
                  || (col >= fb.columns - RIGHT_MARGIN
                      && fb.rem_chars > 1))))

        {
          if ((io_f & LIST))
            FB_PUTS ("\\\n", ed);
          COND_WRAP_FB_ROW ((io_f & ZBCK));
          col = 0;
        }
    }

  if (!(io_f & LIST) && col < fb.columns)
    FB_PUTS ("\n", ed);
  else if ((io_f & LIST) && !fb.rem_chars)
    {
      if ((io_f & ZBCK) && fb.prev_first && fb.prev_first->offset)
        FB_PUTS ("\\\n", ed);
      else
        FB_PUTS ("$\n", ed);
    }
  fb.ff_offset = form_feed && (io_f & ZBCK) ? col : 0;
  return 0;
}


/* fb_putchar: Add a character to the frame buffer. */
static int
fb_putchar (c, ed)
     int c;
     ed_state_t *ed;
{
  ed_frame_node_t *rp = fb.row + fb.row_i;

  REALLOC_THROW (rp->text, rp->text_size, rp->text_i + 1, ERR);
  return *((unsigned char *) rp->text + rp->text_i++) = c;
}


/* display_frame_buffer: Write frame buffer to standard output. */
static int
display_frame_buffer (first, last)
     int first;
     int last;
{
  ed_frame_node_t *rp;
  size_t i;
  int c, n;
  int rows = last - first + (first <= last ? 1 : fb.rows);

  for (c = n = 0; n < rows; ++n)
    {
      rp = fb.row + ((first + n) % (fb.rows - 1));
      for (i = 0; i < rp->text_i; ++i)
        c = putchar (*(rp->text + i));
    }
  if (isatty (1) && n && c != '\n')
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
