/* buf.c: Buffer routines for the ed line editor.
 *
 *  Copyright © 1993-2016 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#include "ed.h"

#include <sys/file.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifndef _PATH_TMP
# define _PATH_TMP "/tmp/"
#endif


/* Static function declarations. */
static int init_stdio __P ((ed_buffer_t *));

/* 
 * one_time_init: Open ed buffer file; initialize queues, I/O buffers
 *   and translation table; read environment variables.
 */
int
one_time_init (argc, argv, ed)
     int argc;
     char *argv[];
     ed_buffer_t *ed;
{
  char *cs, *ls;
  long l;
  int status;

  if ((status = init_stdio (ed)) < 0
      || (status = create_disk_buffer (&ed->core->fp,
                                       &ed->core->pathname, ed)) < 0)
    return status;

  /* 
   * If ed is invoked with file args, any file globbing is done at the
   * shell level. The results are stored in the file_glob structure
   * even if file globbing support is not enabled.
   */
  ed->file->list->gl_pathc = argc;
  ed->file->list->gl_pathv = argv;

  /* 
   * Reset file_glob->gl_offs after first glob(3) -- see file_glob().
   */
  ed->file->list->gl_offs = 1;

  init_ed_command (1, ed);
  init_ed_state (0, &ed->state[0]);
  init_ed_state (-1, &ed->state[1]);

  /* Initialize buffer meta data deques. */
  INIT_DEQUE (ed->core->line_head);
  INIT_DEQUE (ed->core->global_head);
  INIT_DEQUE (ed->core->undo_head);

  /* Sanity check values of environment vars */
  if ((ls = getenv ("LINES")))
    {
      STRTOL_THROW (l, ls, NULL, ERR);
      if (1 < l  && l < ROWS_MAX)
        ed->display->ws_row = l;
    }

  /* !SIGWINCH */
  else if (!ed->display->ws_row)
    ed->display->ws_row = WS_ROW;

  if ((cs = getenv ("COLUMNS")))
    {
      STRTOL_THROW (l, cs, NULL, ERR);
      if (TAB_WIDTH < l && l < COLUMNS_MAX)
        ed->display->ws_col = l;
    }

  /* !SIGWINCH */
  else if (!ed->display->ws_col)
    ed->display->ws_col = WS_COL;

  /* Assign a default command prompt. */
  if (!ed->exec->prompt)
    ed->exec->prompt = DEFAULT_PROMPT;

  /* Check environment for POSIXLY_CORRECT */
  if (getenv ("POSIXLY_CORRECT"))
    ed->exec->opt |= POSIXLY_CORRECT;

#ifdef HAVE_REG_SYNTAX_T
  re_syntax_options =  (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)
                        ? (ed->exec->opt & REGEX_EXTENDED
                           ? RE_SYNTAX_POSIX_EXTENDED : RE_SYNTAX_POSIX_BASIC)
                        : (ed->exec->opt & REGEX_EXTENDED
                           ? RE_SYNTAX_ED_EXTENDED : RE_SYNTAX_ED_BASIC));
#endif

  return 0;
}


/* init_stdio: Initialize standard I/O. */
static int
init_stdio (ed)
     ed_buffer_t *ed;
{
  /* Redirect command script to stdin. */
  if (ed->exec->pathname && !freopen (ed->exec->pathname, "r", stdin))
    {
      fprintf (stderr, "%s: %s\n", ed->exec->pathname, strerror (errno));
      ed->exec->err = _("Buffer open error");
      return FATAL;
    }

  /* 
   * Read stdin one character at a time to avoid I/O contention with
   * shell escapes invoked by nonterminal input, e.g.,
   *
   *    ed - <<EOF
   *    ! cat
   *    hello, world
   *    EOF
   * 
   * See "Unix Programming Environment"
   * Line buffer stdout so that `ssh user@remote ed file' works as
   * expected.
   */
#ifdef HAVE_SETBUFFER
  setbuffer (stdin, NULL, 0);
  setlinebuf (stdout);
#else
  SETVBUF (stdin, NULL, _IONBF, 0);
  SETVBUF (stdout, NULL, _IOLBF, 0);
#endif   /* !HAVE_SETBUFFER */

  /* Don't exit on errors if stdin is non-seekable, e.g., piped or tty. */
  if (lseek (0, 0, SEEK_CUR) != -1
      && (ed->exec->opt & SCRIPTED || !isatty (0)))
    ed->exec->opt |= EXIT_ON_ERROR;
  return 0;
}


/* init_ed_command: Initialize ed_command parameters. */
void
init_ed_command (init_glob, ed)
     int init_glob;             /* If set, initialize file list. */
     ed_buffer_t *ed;
{
  /* Assert: spl1 () */

  /* Display state */
  ed->display->is_paging = 0;
  ed->display->page_addr = 0;
  ed->display->underflow = 0;
  ed->display->overflow = 0;
  ed->display->off = 0;

  /* Global command state */
  ed->exec->first_pass = 0;
  ed->exec->global = 0;

  /* Set default file list */
  if (init_glob)
    *ed->file->glob = *ed->file->list;

  /* File info */
#ifdef WANT_FILE_LOCK

  /* GNU/Linux double-frees on fclose() here... */
#if __linux__
  if (ed->file->handle)

    /* Assert: No writes since fopen(3). */
    (void) fclose (ed->file->handle);
#endif  /* __linux__ */

  ed->file->handle = NULL;
  ed->file->inode = 0;
  ed->file->is_writable = 0;
#endif  /* WANT_FILE_LOCK */

}


/* init_ed_state: Initialize ed_state structure. */
void
init_ed_state (addr, state)
     off_t addr;
     struct ed_state *state;
{
  /* Editor buffer state */
  state->lines = addr;          /* -1 disables undo */
  state->dot = addr;            /* -1 disables undo */
  state->is_binary = 0;
  state->is_empty = 1;
  state->is_modified = 0;
  state->newline_appended = 0;
  state->input_wants_newline = 0;
  state->input_is_binary = 0;
}


/* reopen_ed_buffer: Close on-disk buffer and create new one. */
int
reopen_ed_buffer (ed)
     ed_buffer_t *ed;
{
  int status;

  if ((status = close_ed_buffer (ed)) < 0
      || (status = create_disk_buffer (&ed->core->fp,
                                       &ed->core->pathname, ed)) < 0)
    return status;
  return 0;
}


/* create_disk_buffer: Open an on-disk buffer. */
int
create_disk_buffer (fp, buf, ed)
     FILE **fp;                 /* buffer file pointer */
     char **buf;                /* buffer pathname */
     ed_buffer_t *ed;
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
      ed->exec->err = _("Invalid buffer name");
      return ERR;
    }
  if (!(*buf = (char *) realloc (*buf, len + sizeof template + 1)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      return ERR;
    }
  strcpy (*buf, s);
  strcpy (*buf + len, m ? "/" : "");
  strcpy (*buf + len + m, template);

  /* NB: Don't unlink(2) buffer upon opening in case it resides on NFS. */
  if (!mktemp (*buf)
      || (fd = open (*buf, O_CREAT | O_EXCL | O_RDWR, 0600)) < 0
      || FSTAT (fd, &sb) < 0
      || !(*fp = fdopen (fd, "w+")))
    {
      fprintf (stderr, "%s: %s\n", *buf, strerror (errno));
      ed->exec->err = _("Buffer open error");
      return ERR;
    }

  /* 
   * fstat(3) is supposed to fail if any subpath of `buf' is not a
   * directory. Even on systems for which this does not hold, if the
   * default `/tmp' folder is used, it's reasonable to assume that all
   * is well. So it only remains to verify that `buf' itself is not a
   * symlink on systems where `open (buf, O_EXCL)' is not considered
   * an error.
   */
  if (!S_ISREG (sb.st_mode))
    {
      fprintf (stderr, "%s: Not a regular file\n", *buf);
      ed->exec->err = _("Buffer open error");
      return ERR;
    }
  return 0;
}


/* close_ed_buffer: Close on-disk buffer file. */
int
close_ed_buffer (ed)
     ed_buffer_t *ed;
{
  int status = 0;

  if (ed->core->fp && fclose (ed->core->fp) < 0)
    {
      fprintf (stderr, "%s: %s\n", ed->core->pathname, strerror (errno));
      ed->exec->err = _("Buffer close error");
      status = ERR;
    }
  ed->core->fp = NULL;
  ed->core->offset = 0;
  ed->core->seek_on_write = 0;
  if (ed->core->pathname)
    unlink (ed->core->pathname);
  return status;
}


/* get_buffer_line: Return pointer to copy of line from buffer file. */
char *
get_buffer_line (lp, ed)
     const ed_line_node_t *lp;
     ed_buffer_t *ed;
{
  static char *tb = NULL;       /* Buffer for line from ed buffer file. */
  static size_t tb_size = 0;    /* Buffer size. */

  /* Permit writing the "contents" ('\n') of an empty buffer. */
  if (lp == ed->core->line_head)
    {
      REALLOC_THROW (tb, tb_size, 1, NULL, ed);
      tb[0] = '\0';
      return tb;
    }
  ed->core->seek_on_write = 1;         /* Force seek on write. */

  /* out of position */
  if (ed->core->offset != lp->seek)
    {
      ed->core->offset = lp->seek;
      if (FSEEK (ed->core->fp, ed->core->offset, SEEK_SET) == -1)
        {
          fprintf (stderr, "%s: %s\n", ed->core->pathname, strerror (errno));
          ed->exec->err = _("Buffer seek error");
          return NULL;
        }
    }

  /* Allocate lp->len + '\0' (or '\n', as per write_stream ()). */
  REALLOC_THROW (tb, tb_size, lp->len + 1, NULL, ed);
  if (fread (tb, sizeof (char), lp->len, ed->core->fp) != lp->len)
    {
      fprintf (stderr, "%s: %s\n", ed->core->pathname, strerror (errno));
      ed->exec->err = _("Buffer read error");
      return NULL;
    }
  ed->core->offset += lp->len;  /* Update current offset in buffer file. */
  *(tb + lp->len) = '\0';
  return tb;
}


/* 
 * put_buffer_line: Append given text to end of ed buffer file, and
 *   reference it from in-core buffer. Return pointer to end of text.
 */
char *
put_buffer_line (t, len, ed)
     const char *t;
     size_t len;
     ed_buffer_t *ed;
{
  /* 
   * If ed buffer file was read since last write, seek to end of file
   * before writing.
   */
  if (ed->core->seek_on_write)
    {
      if (FSEEK (ed->core->fp, 0L, SEEK_END) == -1
          || (ed->core->offset = FTELL (ed->core->fp)) == -1)
        {
          fprintf (stderr, "%s: %s\n", ed->core->pathname, strerror (errno));
          ed->exec->err = _("Buffer seek error");
          return NULL;
        }
      ed->core->seek_on_write = 0;
    }

  /* Assert: spl1 () */

  /* Assert: t has trailing '\n'. */
  --len;                        /* Don't write `\n'. */

  /* NB: We are testing buffer file here, not file it represents. */
  if (ed->core->offset > OFF_T_MAX - len)
    {
      ed->exec->err = _("Buffer full");
      return NULL;
    }
  if (fwrite (t, sizeof (char), len, ed->core->fp) != len)
    {
      ed->core->offset = -1;
      fprintf (stderr, "%s: %s\n", ed->core->pathname, strerror (errno));
      ed->exec->err = _("Buffer write error");
      return NULL;
    }
  if (!append_line_node (len, ed->core->offset, ed->state->dot, ed))
    return NULL;

  ++ed->state->dot;
  ed->core->offset += len;      /* Update current offset in buffer file. */
  return (char *) t + ++len;
}


/* 
 * append_line_node: Append line in buffer after given address. Return
 *   node pointer.
 */
ed_line_node_t *
append_line_node (len, offset, addr, ed)
     size_t len;
     off_t offset;
     off_t addr;
     ed_buffer_t *ed;
{
  ed_line_node_t *lp, *np;

  if (ed->state->lines >= OFF_T_MAX)
    {
      ed->exec->err = _("Buffer full");
      return NULL;
    }

  /* Assert: spl1 () */

  if (!(lp = (ed_line_node_t *) malloc (ED_LINE_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      return NULL;
    }

  lp->len = len;
  lp->seek = offset;

  /* This get_line_node last! */
  np = get_line_node (addr, ed);
  APPEND_NODE (lp, np);
  ++ed->state->lines;
  return lp;
}


/* get_line_node: Return pointer to line node in the ed buffer. */
ed_line_node_t *
get_line_node (n, ed)
     off_t n;
     ed_buffer_t *ed;
{
  static ed_line_node_t *lp = NULL;
  static off_t n_prev = 0;

  if (!lp)
    lp = ed->core->line_head;

  if (n > n_prev)
    {
      if (n <= (n_prev + ed->state->lines) >> 1)
        for (; n_prev < n; ++n_prev)
          lp = lp->q_forw;
      else
        {
          lp = ed->core->line_head->q_back;
          for (n_prev = ed->state->lines; n_prev > n; --n_prev)
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
          lp = ed->core->line_head;
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
     ed_buffer_t *ed;
{
  ed_line_node_t *cp = ed->core->line_head;

  *addr = 0;
  while (cp != lp && (cp = cp->q_forw) != ed->core->line_head)
    ++*addr;
  if (*addr && cp == ed->core->line_head)
    {
      ed->exec->err = _("Address out of range");
      return ERR;
    }
  return 0;
}


/* quit: Unlink buffer file and exit. */
void
quit (n, ed)
     int n;
     ed_buffer_t *ed;
{
  if (ed->core->fp)
    {
      (void) fclose (ed->core->fp);
      unlink (ed->core->pathname);
    }
  _exit (n);
}


/* realloc_buffer: Increase size, *n, of buffer, *b, to at least i. */
void *
realloc_buffer (b, n, i, ed)
     void **b;
     size_t *n;
     size_t i;
     ed_buffer_t *ed;
{
  char *_ts;
  size_t _ti;

  /* 
   * Assert: i >= 0.
   * NB: Allocate memory if i == *n == 0.
   */
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
      ed->exec->err = _("Memory request too big");
      return NULL;
    }
  spl1 ();
  if (!(_ts = (char *) realloc (*b, _ti)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      spl0 ();
      return NULL;
    }
  *n = _ti;
  *b = _ts;
  spl0 ();
  return *b;
}


/* init_global_queue: Initialize ed_core global queue. */
void
init_global_queue (aq, lq, ed)
     ed_global_node_t **aq;
     ed_line_node_t **lq;
     ed_buffer_t *ed;
{
  *aq = ed->core->global_head;
  *lq = ed->core->line_head;
}


/* init_register_queue: Initialize given ed_core register queue. */
int
init_register_queue (qno, ed)
     int qno;                   /* register queue number */
     ed_buffer_t *ed;
{
  ed_line_node_t *rq;

  spl1 ();
  if (!(rq = (ed_line_node_t *) malloc (sizeof (ed_line_node_t))))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      spl0 ();
      return ERR;
    }

  LINK_NODES (rq, rq);
  ed->core->reg[qno] = rq;
  spl0 ();
  return 0;
}


/* init_undo_queue: Initialize ed_core undo queue. */
void
init_undo_queue (uq, ed)
     ed_undo_node_t **uq;
     ed_buffer_t *ed;
{
  *uq = ed->core->undo_head;
}


/* init_text_deque: Initialize text deque and free any prior elements. */
void init_text_deque (th)
     ed_text_node_t *th;
{
  char *t;
  size_t len;

  while ((t = shift_text_node (th, &len)))
    free (t);
  INIT_DEQUE (th);
}


/* 
 * append_text_node: Append text to end of text deque. Return pointer
 *   to new node, or NULL is out of memory.
 */
ed_text_node_t *
append_text_node (th, tb, len)
     ed_text_node_t *th;        /* head of text deque */
     const char *tb;
     const size_t len;
{
  ed_text_node_t *tp;
  ed_text_node_t *tq = th->q_back;
  size_t text_size = 0;

  spl1 ();

  /* Allocate node to push onto text queue */
  if (!(tp = (ed_text_node_t *) malloc (ED_TEXT_NODE_T_SIZE)))
    return NULL;
  tp->text = (char *) tb;
  tp->text_i = len;

  /* APPEND_NODE is macro, so tq is mandatory! */
  APPEND_NODE (tp, tq);
  spl0 ();
  return tp;
}


/* 
 * pop_text_node: Remove last node of text queue and return pointer to
 *   copy of its text, otherwise NULL.
 */
char *
pop_text_node (th, len)
     ed_text_node_t *th;        /* head of text deque */
     size_t *len;
{
  static char *s;

  ed_text_node_t *tq = th->q_back;

  /* Uninitialized or no more elements. */
  if (!tq || tq == th)
      return NULL;

  spl1 ();
  UNLINK_NODE (tq);
  *len = tq->text_i;
  s = tq->text;
  free (tq);
  spl0 ();
  return s;
}


/* 
 * shift_text_node: Remove first node of text queue and return pointer
 *   to copy of its text, otherwise NULL.
 */
char *
shift_text_node (th, len)
     ed_text_node_t *th;        /* head of text deque */
     size_t *len;
{
  static char *s;

  ed_text_node_t *tp = th->q_forw;

  /* Uninitialized or no more elements. */
  if (!tp || tp == th)
      return NULL;

  spl1 ();
  UNLINK_NODE (tp);
  *len = tp->text_i;
  s = tp->text;
  free (tp);
  spl0 ();
  return s;
}

#define ALLOC_ED_TYPE(ptr, n, type)                                           \
  do                                                                          \
    {                                                                         \
      if ((ptr = (type *) calloc (n, sizeof (type))) == NULL)                 \
        {                                                                     \
          fprintf (stderr, "%s\n", strerror (errno));                         \
          return NULL;                                                        \
        }                                                                     \
    }                                                                         \
  while (0)

/* alloc_ed_buffer: Allocate memory for ed_buffer_t struct. */
ed_buffer_t *
alloc_ed_buffer ()
{
  ed_buffer_t *ed_buffer;

  ALLOC_ED_TYPE (ed_buffer, 1, ed_buffer_t);
  ALLOC_ED_TYPE (ed_buffer->state, 2, struct ed_state);
  ALLOC_ED_TYPE (ed_buffer->core, 1, struct ed_core);
  ALLOC_ED_TYPE (ed_buffer->core->line_head, 1, ed_line_node_t);
  ALLOC_ED_TYPE (ed_buffer->core->global_head, 1, ed_global_node_t);
  ALLOC_ED_TYPE (ed_buffer->core->undo_head, 1, ed_undo_node_t);
  ALLOC_ED_TYPE (ed_buffer->display, 1, struct ed_display);
  ALLOC_ED_TYPE (ed_buffer->exec, 1, struct ed_execute);
  ALLOC_ED_TYPE (ed_buffer->exec->subst, 1, struct ed_substitute);
  ALLOC_ED_TYPE (ed_buffer->exec->region, 1, struct ed_region);
  ALLOC_ED_TYPE (ed_buffer->file, 1, struct ed_file);
  ALLOC_ED_TYPE (ed_buffer->file->list, 1, glob_t);
  ALLOC_ED_TYPE (ed_buffer->file->glob, 1, glob_t);
  return ed_buffer;
}
