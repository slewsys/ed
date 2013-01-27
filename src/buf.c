/* buf.c: Buffer routines for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2012-12-11 <alm@buttercup.local>

   This file is part of ed. */

#include "ed.h"

#include <sys/file.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifndef _PATH_TMP
# define _PATH_TMP "/tmp/"
#endif


/* Global declarations */
volatile sig_atomic_t window_columns = 80; /* default screen width: ws_col  */
volatile sig_atomic_t window_rows = 24;    /* default screen length: ws_row */
struct ed_core ec;

/* Static function declarations. */
static int init_stdio __P ((ed_state_t *));
static void init_text_queue __P ((ed_text_node_t **));


/* one_time_init: Open ed buffer file; initialize queues, I/O buffers
   and translation table; read environment variables. */
int
one_time_init (argc, argv, ed)
     int argc;
     char *argv[];
     ed_state_t *ed;
{
  char *cs, *ls;
  long l;
  int status;

  if ((status = init_stdio(ed)) < 0
      || (status = open_buffer (ed)) < 0)
    return status;

  /* If ed is invoked with file args, any file globbing is done at the
     shell level. The results are stored in the file_glob structure
     even if file globbing support is not enabled. */
  ed->file.list.gl_pathc = argc;
  ed->file.list.gl_pathv = argv;

  /* Reset file_glob.gl_offs after first glob(3) -- see
     file_glob(). */
  ed->file.list.gl_offs = 1;

  init_ed_command (1, ed);
  init_ed_buffer (0, &ed->buf[0]);
  init_ed_buffer (-1, &ed->buf[1]);

  /* Initialize deques -- aka doubly-linked circular queues. */
  LINK_NODES (&ec.in_core_head, &ec.in_core_head);
  LINK_NODES (&ec.global_head, &ec.global_head);
  LINK_NODES (&ec.text_head, &ec.text_head);
  LINK_NODES (&ec.undo_head, &ec.undo_head);

  /* Sanity check values of environment vars */
  if ((ls = getenv ("LINES")))
    {
      STRTOL_THROW (l, ls, NULL, ERR);
      if (1 < l  && l < ROWS_MAX)
        window_rows = l;
    }
  if ((cs = getenv ("COLUMNS")))
    {
      STRTOL_THROW (l, cs, NULL, ERR);
      if (TAB_WIDTH < l && l < COLUMNS_MAX)
        window_columns = l;
    }

  /* Assign a default command prompt. */
  if (!ed->prompt)
    ed->prompt = DEFAULT_PROMPT;

  /* Check environment for POSIXLY_CORRECT */
  if (getenv ("POSIXLY_CORRECT"))
    ed->opt |= POSIXLY_CORRECT;
  
#ifdef HAVE_REG_SYNTAX_T
  re_syntax_options =  (ed->opt & (POSIXLY_CORRECT | TRADITIONAL)
                        ? (ed->opt & REGEX_EXTENDED
                           ? RE_SYNTAX_POSIX_EXTENDED : RE_SYNTAX_POSIX_BASIC)
                        : (ed->opt & REGEX_EXTENDED
                           ? RE_SYNTAX_ED_EXTENDED : RE_SYNTAX_ED_BASIC));
#endif

  return 0;
}


/* init_stdio: Initialize standard I/O. */
static int
init_stdio (ed)
     ed_state_t *ed;
{
  /* Redirect command script to stdin. */
  if (ed->script_name && !freopen (ed->script_name, "r", stdin))
    {
      fprintf (stderr, "%s: %s\n", ed->script_name, strerror (errno));
      ed->exec.err = _("Buffer open error");
      return FATAL;
    }

  /* Read stdin one character at a time to avoid I/O contention
     with shell escapes invoked by nonterminal input, e.g.,
     ed - <<EOF
     ! cat
     hello, world
     EOF

     Line buffer stdout so that `ssh user@remote ed file' works as
     expected. */
#ifdef HAVE_SETBUFFER
  setbuffer (stdin, NULL, 0);
  setlinebuf (stdout);
#else
  SETVBUF (stdin, NULL, _IONBF, 0);
  SETVBUF (stdout, NULL, _IOLBF, 0);
#endif   /* !HAVE_SETBUFFER */

  /* Don't exit on errors if stdin is non-seekable, e.g., piped, or tty. */
  if (lseek (0, 0, SEEK_CUR) != -1 && (ed->opt & SCRIPTED || !isatty (0)))
    ed->opt |= EXIT_ON_ERROR;
  return 0;
}


/* init_ed_command: Initialize ed_command parameters. */
void
init_ed_command (init_glob, ed)
     int init_glob;             /* If set, initialize file list. */
     ed_state_t *ed;
{
  /* Assert: spl1 () */

  /* Display state */
  ed->display.is_paging = 0;
  ed->display.page_addr = 0;
  ed->display.underflow = 0;
  ed->display.overflow = 0;

  /* Global command state */
  ed->exec.first_pass = 0;
  ed->exec.global = 0;

  /* Set default file list */
  if (init_glob)
    ed->file.glob = ed->file.list;

  /* File info */
#ifdef WANT_FILE_LOCK
  if (ed->file.handle)

    /* Assert: No writes since fopen(3). */
    (void) fclose (ed->file.handle);

  ed->file.handle = NULL;
  ed->file.inode = 0;
  ed->file.is_writable = 0;
#endif
}


/* init_ed_buffer: Initialize ed_buffer structure. */
void
init_ed_buffer (addr, buf)
     off_t addr;
     struct ed_buffer *buf;
{
  /* Editor buffer state */
  buf->addr_last = addr;         /* -1 disables undo */
  buf->dot = addr;               /* -1 disables undo */
  buf->is_binary = 0;
  buf->is_empty = 1;
  buf->is_modified = 0;
  buf->newline_appended = 0;
  buf->input_wants_newline = 0;
  buf->input_is_binary = 0;
}


/* init_global_queue: Initialize ed_core global queue. */
void
init_global_queue (aq, lq)
     ed_global_node_t **aq;
     ed_line_node_t **lq;
{
  *aq = &ec.global_head;
  *lq = &ec.in_core_head;
}


/* init_register_queue: Initialize given ed_core register queue. */
int
init_register_queue (rq, qno, ed)
     ed_line_node_t *rq[];
     int qno;                   /* register queue number */
     ed_state_t *ed;
{
  spl1 ();
  if (!(rq[qno] = (ed_line_node_t *) malloc (sizeof (ed_line_node_t))))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Memory exhausted");
      spl0 ();
      return ERR;
    }

  LINK_NODES (rq[qno], rq[qno]);
  ec.reg[qno] = rq[qno];
  spl0 ();
  return 0;
}


/* init_text_queue: Initialize ed_core text queue. */
static void
init_text_queue (tq)
     ed_text_node_t **tq;
{
  *tq = &ec.text_head;
}


/* init_undo_queue: Initialize ed_core undo queue. */
void
init_undo_queue (uq)
     ed_undo_node_t **uq;
{
  *uq = &ec.undo_head;
}


/* open_buffer: Open ed buffer. */
int
open_buffer (ed)
     ed_state_t *ed;
{
  STAT_T sb;
  char template[] = "ed.XXXXXXXXXX";
  char *s;
  size_t len;
  int fd;
  int m = 0;

  if ((!(s = getenv ("TMPDIR")) || *s == '\0')
      && (!(s = _PATH_TMP) || *s == '\0'))
    s = "/tmp/";
  m = *(s + (len = strlen (s)) - 1) != '/';
  if (len > get_path_max (s) - sizeof template - m)
    {
      fprintf (stderr, "%s/%s: %s\n", s, template, _("File name too long"));
      ed->exec.err = _("Invalid buffer name");
      return FATAL;
    }
  REALLOC_THROW (ec.pathname, ec.pathname_size,
                 len + sizeof template + 1, FATAL);
  memcpy (ec.pathname, s, len);
  memcpy (ec.pathname + len, m ? "/" : "", 1);
  memcpy (ec.pathname + len + m, template, sizeof template);

  /* NB: Don't unlink(2) buffer upon opening in case it resides on NFS. */
  if (!mktemp (ec.pathname)
      || (fd = open (ec.pathname, O_CREAT | O_EXCL | O_RDWR, 0600)) < 0
      || FSTAT (fd, &sb) < 0
      || !(ec.fp = fdopen (fd, "w+")))
    {
      fprintf (stderr, "%s: %s\n", ec.pathname, strerror (errno));
      ed->exec.err = _("Buffer open error");
      return FATAL;
    }

  /* fstat(3) is supposed to fail if any subpath of `ec.pathname' is not a
     directory. Even on systems for which this does not hold, if the
     default `/tmp' folder is used, it's reasonable to assume that all
     is well. So it only remains to verify that `ec.pathname' itself is not a
     symlink on systems where `open (ec.pathname, O_EXCL)' is not considered
     an error. */
  if (!S_ISREG (sb.st_mode))
    {
      fprintf (stderr, "%s: Not a regular file\n", ec.pathname);
      ed->exec.err = _("Buffer open error");
      return FATAL;
    }
  return 0;
}


/* close_buffer: Close buffer file. */
int
close_buffer (ed)
     ed_state_t *ed;
{
  int status = 0;
  
  if (ec.fp && fclose (ec.fp) < 0)
    {
      fprintf (stderr, "%s: %s\n", ec.pathname, strerror (errno));
      ed->exec.err = _("Buffer close error");
      status = ERR;
    }
  ec.fp = NULL;
  ec.offset = 0;
  ec.seek_on_write = 0;
  if (ec.pathname)
    unlink (ec.pathname);
  return 0;
}


/* get_buffer_line: Return pointer to copy of line from buffer file. */
char *
get_buffer_line (lp, ed)
     const ed_line_node_t *lp;
     ed_state_t *ed;
{
  static char *tb = NULL;       /* Buffer for line from ed buffer file. */
  static size_t tb_size = 0;    /* Buffer size. */

  /* Permit writing the "contents" ('\n') of an empty buffer. */
  if (lp == &ec.in_core_head)
    {
      REALLOC_THROW (tb, tb_size, 1, NULL);
      tb[0] = '\0';
      return tb;
    }
  ec.seek_on_write = 1;         /* Force seek on write. */

  /* out of position */
  if (ec.offset != lp->seek)
    {
      ec.offset = lp->seek;
      if (FSEEK (ec.fp, ec.offset, SEEK_SET) == -1)
        {
          fprintf (stderr, "%s: %s\n", ec.pathname, strerror (errno));
          ed->exec.err = _("Buffer seek error");
          return NULL;
        }
    }

  /* Allocate lp->len + '\0' (or '\n', as per write_stream ()). */
  REALLOC_THROW (tb, tb_size, lp->len + 1, NULL);
  if (fread (tb, sizeof (char), lp->len, ec.fp) != lp->len)
    {
      fprintf (stderr, "%s: %s\n", ec.pathname, strerror (errno));
      ed->exec.err = _("Buffer read error");
      return NULL;
    }
  ec.offset += lp->len;         /* Update current offset in buffer file. */
  *(tb + lp->len) = '\0';
  return tb;
}


/* put_buffer_line: Append given text to end of ed buffer file, and
   reference it from in-core buffer. Return a pointer to end of
   text. */
char *
put_buffer_line (t, len, ed)
     const char *t;
     size_t len;
     ed_state_t *ed;
{
  /* If ed buffer file was read since last write, seek to end of file
     before writing. */
  if (ec.seek_on_write)
    {
      if (FSEEK (ec.fp, 0L, SEEK_END) == -1
          || (ec.offset = FTELL (ec.fp)) == -1)
        {
          fprintf (stderr, "%s: %s\n", ec.pathname, strerror (errno));
          ed->exec.err = _("Buffer seek error");
          return NULL;
        }
      ec.seek_on_write = 0;
    }

  /* Assert: spl1 () */

  /* Assert: t has trailing '\n'. */
  --len;                        /* Don't write `\n'. */

  /* NB: We are testing buffer file here, not file it represents. */
  if (ec.offset > OFF_T_MAX - len)
    {
      ed->exec.err = _("Buffer full");
      return NULL;
    }
  if (fwrite (t, sizeof (char), len, ec.fp) != len)
    {
      ec.offset = -1;
      fprintf (stderr, "%s: %s\n", ec.pathname, strerror (errno));
      ed->exec.err = _("Buffer write error");
      return NULL;
    }
  if (!append_line_node (len, ec.offset, ed->buf[0].dot, ed))
    return NULL;

  ++ed->buf[0].dot;
  ec.offset += len;             /* Update current offset in buffer file. */
  return (char *) t + ++len;
}


/* append_line_node: Append line in buffer after given address.
   Return node pointer. */
ed_line_node_t *
append_line_node (len, offset, addr, ed)
     size_t len;
     off_t offset;
     off_t addr;
     ed_state_t *ed;
{
  ed_line_node_t *lp, *np;

  if (ed->buf[0].addr_last >= OFF_T_MAX)
    {
      ed->exec.err = _("Buffer full");
      return NULL;
    }
  if (!(lp = (ed_line_node_t *) malloc (ED_LINE_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Memory exhausted");
      return NULL;
    }

  lp->len = len;
  lp->seek = offset;

  /* This get_line_node last! */
  np = get_line_node (addr, ed->buf);
  APPEND_NODE (lp, np);
  ++ed->buf[0].addr_last;
  return lp;
}


/* get_line_node: Return pointer to line node in the ed buffer. */
ed_line_node_t *
get_line_node (n, buf)
     off_t n;
     struct ed_buffer *buf;
{
  static ed_line_node_t *lp = &ec.in_core_head;
  static off_t n_prev = 0;

  if (n > n_prev)
    {
      if (n <= (n_prev + buf->addr_last) >> 1)
        for (; n_prev < n; ++n_prev)
          lp = lp->q_forw;
      else
        {
          lp = ec.in_core_head.q_back;
          for (n_prev = buf->addr_last; n_prev > n; --n_prev)
            lp = lp->q_back;
        }
    }
  else
    {
      if (n >= n_prev >> 1)
        for (; n_prev > n; --n_prev)
          lp = lp->q_back;
      else
        {
          lp = &ec.in_core_head;
          for (n_prev = 0; n_prev < n; ++n_prev)
            lp = lp->q_forw;
        }
    }
  return lp;
}


/* get_line_node_address: Get line number of pointer; return status. */
int
get_line_node_address (lp, addr, ed)
     const ed_line_node_t *lp;
     off_t *addr;
     ed_state_t *ed;
{
  ed_line_node_t *cp = &ec.in_core_head;

  for (*addr = 0; cp != lp && (cp = cp->q_forw) != &ec.in_core_head; ++*addr)
    ;
  if (*addr && cp == &ec.in_core_head)
    {
      ed->exec.err = _("Address out of range");
      return ERR;
    }
  return 0;
}


/* quit: Unlink buffer file and exit. */
void
quit (n)
     int n;
{
  if (ec.fp)
    {
      (void) fclose (ec.fp);
      unlink (ec.pathname);
    }
  _exit (n);
}


/* realloc_buffer: Increase size, *n, of buffer, *b, to at least i. */
void *
realloc_buffer (b, n, i, ed)
     void **b;
     size_t *n;
     off_t i;
     ed_state_t *ed;
{
  char *_ts;
  size_t _ti;

  /* Assert: i >= 0.
     NB: Allocate memory if i == *n == 0. */
  if (i < *n)
    return *b;
  if (i < BUFSIZ)
    _ti = BUFSIZ;
  else if (i >> BUFSIZ_LOG2 <= SIZE_T_MAX >> (BUFSIZ_LOG2 + 1))
    _ti = (size_t) (i >> BUFSIZ_LOG2 << (BUFSIZ_LOG2 + 1));
  else if (i <= SIZE_T_MAX)
    _ti = (size_t) i;
  else
    {
      ed->exec.err = _("Memory request too big");
      return NULL;
    }
  spl1 ();
  if (!(_ts = (char *) realloc (*b, _ti)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Memory exhausted");
      spl0 ();
      return NULL;
    }
  *n = _ti;
  *b = _ts;
  spl0 ();
  return *b;
}


#ifdef WANT_EXTERNAL_FILTER

/* Global declarations */
ed_text_node_t *text_head = NULL; /* head of text queue */
ed_text_node_t *tn_p = NULL;      /* text queue pointer */


/* append_text_node: Append text to end of text queue.
   Return status. */
ed_text_node_t *
append_text_node (tb, len, ed)
     const char *tb;
     size_t len;
     ed_state_t *ed;
{
  ed_text_node_t *tp;
  ed_text_node_t *last = text_head->q_back;
  size_t text_size = 0;

  spl1 ();
  
  /* Allocate node to push onto text queue */
  if (!(tp = (ed_text_node_t *) malloc (ED_TEXT_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Memory exhausted");
      spl0 ();
      return NULL;
    }
  tp->text = NULL;
  REALLOC_THROW (tp->text, text_size, len + 1, NULL);
  memcpy (tp->text, tb, len + 1);
  tp->text_i = len;
  APPEND_NODE (tp, last);
  spl0 ();
  return tp;
}


/* next_text_node: Return next node of text queue, otherwise NULL. */
char *
next_text_node (len)
     size_t *len;
{
  static ed_text_node_t *tp = NULL;

  if ((tp = tn_p = tn_p->q_forw) == text_head)
    return NULL;
  *len = tp->text_i;
  return tp->text;
}


/* clear_text_queue: Release nodes of text queue. */
void
reset_text_queue ()
{
  ed_text_node_t *tp, *next;

  /* Assert: spl1 () */

  if (!text_head)
    init_text_queue (&text_head);

  for (tp = (tn_p = text_head)->q_forw; tp != text_head; tp = next)
    {
      next = tp->q_forw;
      UNLINK_NODE (tp);
      free (tp);
    }
}

#endif  /* WANT_EXTERNAL_FILTER */

/*
 * Local variables:
 * mode: c
 * eval: (add-hook 'write-file-functions 'time-stamp)
 * time-stamp-start: "Last modified: "
 * time-stamp-format: "%:y-%02m-%02d <%u@%h>"
 * time-stamp-end: "$"
 * End:
 */
