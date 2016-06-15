/* io.c: I/O routines for the ed line editor.
 *
 *  Copyright © 1993-2016 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#include "ed.h"

#ifdef WANT_FILE_LOCK
#  ifdef HAVE_FLOCK
#    ifdef HAVE_SYS_FILE_H
#      include <sys/file.h>
#      ifndef LOCK_SH
#        define LOCK_SH 1            /* shared lock */
#        define LOCK_EX 2            /* exclusive lock */
#        define LOCK_NB 4            /* don't block */
#        define LOCK_UN 8            /* unlock */
#      endif /* !LOCK_SH */
#    endif  /* HAVE_SYS_FILE_H */
#  endif   /* !HAVE_FLOCK */
#endif    /* WANT_FILE_LOCK */

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif  /* HAVE_FCNTL_H */

/* Static function declarations. */
static int put_stream_line __P ((FILE *, const char *, int,
                                 ed_buffer_t *));
static int read_stream __P ((FILE *, off_t, off_t *, ed_buffer_t *));
static int get_inode __P ((const char *, INO_T *, ed_buffer_t *));


/* read_file: Read file to buffer; return line count. */
int
read_file (fn, after, addr, size, is_default, ed)
     const char *fn;
     off_t after;
     off_t *addr;
     off_t *size;
     int is_default;            /* If set, fn is default file name. */
     ed_buffer_t *ed;
{
  FILE *fp;
  INO_T inode;
  int status;

  *addr = 0;
#ifdef WANT_FILE_LOCK
  if (get_inode (fn, &inode, ed) < 0)
    return ERR;

  /* File already open. */
  if (inode && inode == ed->file->inode)
    {
      if (FSEEK (fp = ed->file->handle, 0, SEEK_SET) == -1)
        {
          fprintf (stderr, "%s: %s\n", fn, strerror (errno));
          ed->exec->err = _("File seek error");
          return ERR;
        }
    }
  else
#endif  /* WANT_FILE_LOCK */

    /* Open for writing so that exclusive lock can be set by F_SETLK.  */
    if (!(fp = fopen (fn, "r+")))
      {
        fprintf (stderr, "%s: %s\n", fn, strerror (errno));
        ed->exec->err = _("File open error");
        return ERR;
      }
#ifdef WANT_FILE_LOCK
    else if (is_default)
      {
        spl1 ();
        ed->file->inode = inode;
        ed->file->handle = fp;
        ed->file->is_writable = 0;
        spl0 ();
      }
  /* Assert: File lock released on file close. */
  if (set_file_lock (fp, 1) != 0 && isatty (0))
    fprintf (stderr, (ed->exec->opt & VERBOSE
                      ? _("WARNING: File already locked\n") : ""));
#endif  /* HAVE_FILE_LOCK */
  if ((status = read_stream (fp, after, size, ed)) < 0)
    return status;
  *addr = ed->state->dot - after;

#ifdef WANT_FILE_LOCK
  if (!is_default)
#endif
    if (fclose (fp) < 0)
      {
        fprintf (stderr, "%s: %s\n", fn, strerror (errno));
        ed->exec->err = _("File close error");
        return ERR;
      }
  return 0;
}


/* read_pipe: Read output of shell command to buffer; return line
   count. */
int
read_pipe (fn, after, addr, size, ed)
     const char *fn;
     off_t after;
     off_t *addr;
     off_t *size;
     ed_buffer_t *ed;
{
  FILE *fp;
  int status;

  *addr = 0;

  if (!(fp = popen (fn + 1, "r")))
    {
      fprintf (stderr, "%s: %s\n", fn, strerror (errno));
      ed->exec->err = _("File open error");
      return ERR;
    }
  if ((status = read_stream (fp, after, size, ed)) < 0)
    return status;
  *addr = ed->state->dot - after;

  /* Ignore "no child" error. */
  pclose (fp);
  printf (ed->exec->opt & SCRIPTED ? "" : "!\n");
  return 0;
}

/* PUT_BUFFER_LINE: Add a line of text to the editor buffer. */
#define PUT_BUFFER_LINE(lp, tb, len, up, addr)                                \
  do                                                                          \
    {                                                                         \
      spl1 ();                                                                \
      if (!put_buffer_line ((tb), (len), ed))                                 \
        {                                                                     \
          spl0 ();                                                            \
          return ERR;                                                         \
        }                                                                     \
      (lp) = (lp)->q_forw;                                                    \
      APPEND_UNDO_NODE ((lp), (up), (addr), ed);                              \
      spl0 ();                                                                \
    }                                                                         \
  while (0)


/* read_stream: Read a stream into the editor buffer after the given
   address; return bytes read. */
static int
read_stream (fp, after, size, ed)
     FILE *fp;
     off_t after;
     off_t *size;
     ed_buffer_t *ed;
{
  ed_line_node_t *lp = get_line_node (after, ed);
  ed_undo_node_t *up = NULL;
  char *tb;
  size_t len;
  int newline_inserted = 0;
  int newline_appended_already = ed->state->newline_appended;
  int stream_appended = after == ed->state->lines;

  ed->state->dot = after;
  ed->state->input_is_binary = 0;
  ed->state->input_wants_newline = 0;
  for (*size = 0; (tb = get_stream_line (fp, &len, ed)); *size += len)
    PUT_BUFFER_LINE (lp, tb, len, up, ed->state->dot);
  if (feof (fp))
    {
      fflush (fp);
      clearerr (fp);

      /* Empty file into empty buffer. */
      if (!*size && !ed->state->lines)
        {
          PUT_BUFFER_LINE (lp, "\n", 1, up, ed->state->dot);

          /* First time only! */
          if (!newline_appended_already)
            {
              *size = 1;
              ed->state->input_wants_newline = 1;
            }
        }
    }
  else if (ferror (fp))
    {
      clearerr (fp);
      return ERR;
    }
  /* Out of memory */
  else if (!tb)
    return ERR;

  ed->state->is_empty = (ed->state->is_empty
                         ? *size == ed->state->input_wants_newline : 0);

  /* A newline is appended to an empty file read into an empty buffer,
     but the buffer is still considered `empty' in the sense that a
     subsequent read or append will overwrite the newline. This is
     what is expected when concatenating files. */
  newline_inserted = (stream_appended
                      ? ed->state->newline_appended && ed->state->is_binary
                      : ed->state->input_wants_newline);
  ed->state->newline_appended = (stream_appended
                                 ? ed->state->input_wants_newline
                                 : ed->state->newline_appended);

  /* Assume that user knows what they're doing ...
  if (ed->state->is_binary)
    puts (_("Binary data"));
  */

  /* If binary, adjust size since appended newlines are not written. */
  if ((ed->state->is_binary |= ed->state->input_is_binary)
      && stream_appended && ed->state->newline_appended)
    --*size;

  /* When either appending to a binary file which has no trailing
     newline or inserting a binary file with no trailing newline into
     another file, the lines separated by the inserted newline should
     really be joined. This is what is expected when concatenating
     files. For now, just warn that a newline has been inserted and
     leave joining the lines up to the user. */
  if (newline_inserted && *size > 0)
    puts (_("Newline inserted"));
  else if (!ed->state->is_binary && ed->state->newline_appended
           && (*size || (ed->state->is_empty && !newline_appended_already)))
    puts (_("Newline appended"));

  return 0;
}


#ifdef WANT_EXTERNAL_FILTER

#  define APPEND_TEXT_NODE(th, s, len, ed)                                    \
  do                                                                          \
    {                                                                         \
      char *_t;                                                               \
      if (!(_t = strdup (s)) || !append_text_node ((th), _t, (len)))          \
        {                                                                     \
          fprintf (stderr, "%s\n", strerror (errno));                         \
          (ed)->exec->err = _("Memory exhausted");                            \
          return ERR;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)

/* read_stream_r: Read a stream into memory and then into editor
   buffer after the given address; return bytes read. */
int
read_stream_r (fp, after, size, ed)
     FILE *fp;
     off_t after;
     off_t *size;
     ed_buffer_t *ed;
{
  static ed_text_node_t th;     /* text deque head */

  ed_line_node_t *lp;
  ed_undo_node_t *up = NULL;
  char *s;
  char *t;
  size_t len = 0;
  int newline_inserted = 0;
  int newline_appended_already = ed->state->newline_appended;
  int stream_appended = after == ed->state->lines;

  ed->state->input_is_binary = 0;
  ed->state->input_wants_newline = 0;

  init_text_deque (&th);

  /* Read stream into memory to avoid reentrant calls to buffer file I/O. */
  for (*size = 0; (s = get_stream_line (fp, &len, ed)); *size += len)
    APPEND_TEXT_NODE (&th, s, len, ed);
  if (feof (fp))
    {
      fflush (fp);
      clearerr (fp);

      /* Empty file into empty buffer. */
      if (!*size && !ed->state->lines)
        {
          APPEND_TEXT_NODE (&th, "\n", 1, ed);

          /* First time only! */
          if (!newline_appended_already)
            {
              *size = 1;
              ed->state->input_wants_newline = 1;
            }
        }
    }
  else if (ferror (fp) || !s)
    {
      clearerr (fp);
      return ERR;
    }

  lp = get_line_node (after, ed);
  ed->state->dot = after;

  /* Write stream input to buffer file. */
  while ((t = shift_text_node (&th, &len)))
    {
      PUT_BUFFER_LINE (lp, t, len, up, ed->state->dot);
      free (t);
    }

  /* A newline is appended to an empty file read into an empty buffer,
     but the buffer is still considered `empty' in the sense that a
     subsequent read or append will overwrite the newline. This is
     what is expected when concatenating files. */
  ed->state->is_empty = (ed->state->is_empty
                         ? *size == ed->state->input_wants_newline : 0);

  /* If stream_appended, then nl_prev == 0  ==> nl_prev && binary_prev == 0. */
  newline_inserted = (stream_appended
                      ? ed->state->newline_appended && ed->state->is_binary
                      : ed->state->input_wants_newline);
  ed->state->newline_appended = (stream_appended
                                 ? ed->state->input_wants_newline
                                 : ed->state->newline_appended);

  /* Assume that user knows what they're doing ...
  if (ed->state->is_binary)
    puts (_("Binary data"));
  */

  /* If binary, adjust size since appended newlines are not written. */
  if ((ed->state->is_binary |= ed->state->input_is_binary)
      && stream_appended && ed->state->newline_appended)
    --*size;

  /* When either appending to a binary file which has no trailing
     newline or inserting a binary file with no trailing newline into
     another file, the lines separated by the inserted newline should
     really be joined. This is what is expected, e.g., when concatenating
     files. The problem is that ed has historically appended a missing
     newline, and any other behavior would likely be a source of
     confusion. So for now, just warn that a newline has been inserted
     and leave joining the lines up to the user. */
  if (newline_inserted && *size > 0)
    puts (_("Newline inserted"));
  else if (!ed->state->is_binary && ed->state->newline_appended
           && (*size || ed->state->is_empty))
    puts (_("Newline appended"));

  return 0;
}

#endif  /* WANT_EXTERNAL_FILTER */


/* get_extended_line: Get an extended line from stdin;
   return pointer to static buffer. */
char *
get_extended_line (len, nonl, ed)
     size_t *len;               /* extended line length */
     int nonl;                  /* If set, remove trailing newline. */
     ed_buffer_t *ed;
{
  static char *xl = NULL;       /* extended line buffer */
  static size_t xl_size = 0;    /* buffer size */

  size_t n;

  for (*len = 0; *(ed->input + (*len)++) != '\n';)
    ;
  REALLOC_THROW (xl, xl_size, *len + 1, NULL, ed);

  /* NB: Don't assume that ed->input is NUL-terminated. */
  memcpy (xl, ed->input, *len);

  /* Shell escapes set nonl, so we are only interested in a trailing
     escape in this case. */
  while (nonl ? *len > 1 && *(xl + *len - 2) == '\\'
         : has_trailing_escape (xl, xl + *len - 1))
    {
      *(xl + --*len - 1) = '\n'; /* strip trailing backslash */
      if (!(ed->input = get_stdin_line (&n, ed)))

        /* Propagate stream status - don't call clearerr(3). */
        return NULL;
      if (*(ed->input + n - 1) != '\n')
        {
          ed->exec->err = _("End-of-file unexpected");
          return NULL;
        }
      REALLOC_THROW (xl, xl_size, *len + n + 1, NULL, ed);
      memcpy (xl + *len, ed->input, n);
      *len += n;
      ++ed->exec->line_no;
    }
  *len -= nonl;                 /* strip newline, if nonl */
  *(xl + *len) = '\0';
  return xl;
}


/* get_stream_line: Read a line of text from a stream; return line length */
char *
get_stream_line (fp, len, ed)
     FILE *fp;
     size_t *len;               /* line length */
     ed_buffer_t *ed;
{
  static char *tb = NULL;       /* text buffer */
  static size_t tb_size = 0;    /* buffer size */

  int c;
  char fb[PATH_MAX];
  char *fn;

  /* NB: stdin is not buffered to avoid I/O contention (see buf.c),
     but other file I/O is buffered. */

  *len = 0;

 top:
  for (; (c = getc (fp)) != EOF && c != '\n'; ++*len)
    {
      REALLOC_THROW (tb, tb_size, *len + 1, NULL, ed);
      ed->state->input_is_binary |= !(*(tb + *len) = c);
    }
  if (feof (fp))
    {
      if (!*len)

        /* Propagate stream status - don't call clearerr(3). */
        return NULL;
    }
  else if (ferror (fp))
    switch (errno)
      {
      case EINTR:
        clearerr (fp);
        goto top;
      default:
#ifdef F_GETPATH

        /* Recover file name from pointer. */
        fcntl (fileno (fp), F_GETPATH, fb);
        fn = (fn = strrchr (fb, '/')) ? fn + 1 : fb;
        fprintf (stderr, "%s: %s\n", fn, strerror (errno));
#else
        fprintf (stderr, "%s\n", strerror (errno));
#endif
        ed->exec->err = _("File read error");

        /* Propagate stream status - don't call clearerr(3). */
        return NULL;
      }

  REALLOC_THROW (tb, tb_size, *len + 2, NULL, ed);
  if (fp == stdin)
    {
      *(tb + *len) = '\n';
      *len += c == '\n';
      *(tb + *len) = '\0';
    }
  else
    {
      ed->state->input_wants_newline = c != '\n';
      *(tb + *len) = '\n';
      *(tb + ++*len) = '\0';
    }
  return tb;
}


/* write_file: Write buffer range to file; return line count. */
int
write_file (fn, is_default, from, to, addr, size, mode, ed)
     const char *fn;
     int is_default;
     off_t from;
     off_t to;
     off_t *addr;
     off_t *size;
     const char *mode;
     ed_buffer_t *ed;
{
  FILE *fp;
  ed_line_node_t *lp = get_line_node (from, ed);
  off_t n = from ? to - from + 1 : 0;
  INO_T inode;
  int file_already_open = 0;
  int status;

#ifdef WANT_FILE_LOCK
  if (get_inode (fn, &inode, ed) < 0)
    return ERR;

  /* File already open and writable. */
  if ((file_already_open = (inode && inode == ed->file->inode))
      && ed->file->is_writable)
    {
      if (FSEEK (fp = ed->file->handle, 0,
                 *mode == 'a' ? SEEK_END : SEEK_SET) == -1)
        {
          fprintf (stderr, "%s: %s\n", ed->file->name, strerror (errno));
          ed->exec->err = _("File seek error");
          return ERR;
        }
      if (*mode == 'w' && ftruncate (fileno (fp), 0) == -1)
        {
          fprintf (stderr, "%s: %s\n", ed->file->name, strerror (errno));
          ed->exec->err = _("File truncate error");
          return ERR;
        }
    }

  /* XXX Potential race: Reopening file for writing may lose lock. */
  else if (file_already_open && fclose (ed->file->handle) < 0)
    {
      fprintf (stderr, "%s: %s\n", ed->file->name, strerror (errno));
      ed->exec->err = _("File close error");
      return ERR;
    }
  else
    {
#endif  /* WANT_FILE_LOCK */
      if (!(fp = fopen (fn, mode)))
        {
          fprintf (stderr, "%s: %s\n", fn, strerror (errno));
          ed->exec->err = _("File open error");
          return ERR;
        }
#ifdef WANT_FILE_LOCK
      if (is_default)
        {
          spl1 ();
          ed->file->inode = inode;
          ed->file->handle = fp;
          ed->file->is_writable = 1;
          spl0 ();
        }
      if (set_file_lock (fp, 1) != 0 && isatty (0))
        fprintf (stderr, (ed->exec->opt & VERBOSE
                          ? _("WARNING: File already locked\n") : ""));
    }
#endif  /* WANT_FILE_LOCK */
  if ((status = write_stream (fp, lp, n, size, ed)) < 0)
    return status;

  /* Allow writing an empty buffer. */
  *addr = n;

#ifdef WANT_FILE_LOCK
  if (!is_default)
#endif
    if (fclose (fp) < 0)
      {
        fprintf (stderr, "%s: %s\n", fn, strerror (errno));
        ed->exec->err = _("File close error");
        return ERR;
      }
  return 0;
}


/* write_pipe: Write buffer range to pipe; return status. */
int
write_pipe (fn, from, to, addr, size, ed)
     const char *fn;
     off_t from;
     off_t to;
     off_t *addr;
     off_t *size;
     ed_buffer_t *ed;
{
  FILE *fp;
  ed_line_node_t *lp = get_line_node (from, ed);
  off_t n = from ? to - from + 1 : 0;
  int status;

  if (!(fp = popen (fn + 1, "w")))
    {
      fprintf (stderr, "%s: %s\n", fn, strerror (errno));
      ed->exec->err = _("File open error");
      return ERR;
    }
  if ((status = write_stream (fp, lp, n, size, ed)) < 0)
    return status;

  /* Allow writing an empty buffer. */
  *addr = n;

  /* Ignore "no child" error. */
  pclose (fp);
  printf (ed->exec->opt & SCRIPTED ? "" : "!\n");
  return 0;
}


/* write_stream: Write a range of lines to a stream; return status. */
int
write_stream (fp, lp, n, size, ed)
     FILE *fp;
     ed_line_node_t *lp;
     off_t n;
     off_t *size;
     ed_buffer_t *ed;
{
  size_t len;
  char *s;
  int append_newline = 0;
  int status = 0;

  /* Per SUSv4, permit writing an empty buffer. */
  for (*size = 0; n; --n, lp = lp->q_forw)
    {
      if (!(s = get_buffer_line (lp, ed)))
        return ERR;
      len = lp->len;
      append_newline = !(n == 1 && ed->state->is_binary
                         && ed->state->newline_appended);
      if (*size >= OFF_T_MAX - len - append_newline)
        {
          ed->exec->err = _("File too big");
          return ERR;
        }
      if (append_newline)
        *(s + len++) = '\n';
      if ((status = put_stream_line (fp, s, len, ed)) < 0)
        {
          clearerr (fp);
          return status;
        }
      *size += len;
    }
  fflush (fp);
  return 0;
}


/* put_stream_line: Write a line of text to a stream; return status. */
static int
put_stream_line (fp, s, len, ed)
     FILE *fp;
     const char *s;
     int len;
     ed_buffer_t *ed;
{
 top:
  for (; len && putc ((unsigned) *s, fp) != EOF; --len, ++s)
    ;
  if (ferror (fp))
    switch (errno)
      {
      case EINTR:
        clearerr (fp);
        goto top;
      default:
        fprintf (stderr, "%s\n", strerror (errno));
        ed->exec->err = _("File write error");

        /* Propagate stream status - don't call clearerr(3). */
        return ERR;
      }
  return 0;
}


/* get_inode: Get file inode, return status. */
static int
get_inode (fn, inode, ed)
     const char *fn;
     INO_T *inode;
     ed_buffer_t *ed;
{
  STAT_T sb;

  /* Assert: stat returns error if NAME_MAX or PATH_MAX is exceeded. */
  if (STAT (fn, &sb) == -1)
    {
      if (errno == ENOENT)
        {
          /* Assert: i-node 0 is not assigned to any file. */
          *inode = 0;
          return 0;
        }
      fprintf (stderr, "%s: %s\n", fn, strerror (errno));
      ed->exec->err = _("File status error");
      return ERR;
    }

  /* XXX: Potential race condition -- status may change before open(2). */
  if (ed->exec->opt & RESTRICTED && !S_ISREG (sb.st_mode))
    {
      ed->exec->err = _("Access restricted to regular files");
      return ERR;
    }
  *inode = sb.st_ino;
  return 0;
}


#ifdef WANT_FILE_LOCK

/* set_file_lock: Set (advisory) file lock, return status. */
int
set_file_lock (fp, exclusive)
     FILE *fp;
     int exclusive;             /* If set, get exclusive lock. */
{
  int flock_status = 0;
  int fcntl_status = 0;
  int fd = fileno (fp);

#  ifdef F_SETLK
  struct flock l;
#  endif  /* F_SETLK */

#  ifdef HAVE_FLOCK
  if (flock (fd, (exclusive ? LOCK_EX : LOCK_SH) | LOCK_NB) == -1)
    {
      /* fprintf (stderr, "flock: %s\n", strerror (errno)); */
      flock_status = errno;
      errno = 0;
    }
#  endif  /* HAVE_FLOCK */

#  ifdef F_SETLK
  l.l_start = 0;
  l.l_len = 0;
  l.l_type = exclusive ? F_WRLCK : F_RDLCK;
  l.l_whence = 0;
  if (fcntl (fd, F_SETLK, &l) == -1)
    {

      /* In case flock(2) and fcntl(2) share locks. */
#    ifdef HAVE_FLOCK

      /* flock(8) already succeeded...  */
      if (errno == EAGAIN && !flock_status)
        {
          errno = 0;
          return 0;
        }
#    endif  /* HAVE_FLOCK */
      /* fprintf (stderr, "F_SETLK: %s\n", strerror (errno)); */
      fcntl_status = errno;
      errno = 0;
    }
#  endif  /* F_SETLK */

  return flock_status || fcntl_status ? -1 : 0;
}

#endif  /* WANT_FILE_LOCK */
