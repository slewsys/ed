/* register.c: Register and macro interface for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"

#ifdef WANT_ED_REGISTER

/* Static function declarations. */
static int script_from_register (ed_buffer_t *);
static int push_stack_frame (ed_buffer_t *);
static int pop_stack_frame (ed_buffer_t *);


/*
 * append_from_register: Append lines from register to buffer after given
 *   address.
 */
int
append_from_register (off_t addr, ed_buffer_t *ed)
{
  int read_idx = ed->core->regbuf->read_idx;
  FILE *read_fp = ed->core->regbuf->fp[read_idx];
  off_t size = 0;
  int status = 0;

  /* Nothing to read. */
  if (!read_fp)
    return 0;
  else if (FSEEK (read_fp, 0L, SEEK_SET) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (read_fp);
      return ERR;
    }

  if ((status = read_stream (read_fp, addr, &size, ed)) < 0)
    return status;
  if (size)
    ed->state->is_modified = 1;
  return 0;
}


/*
 * append_to_register: Write addressed lines to given register. If
 *   `append' is zero, any previous register contents are lost.
 */
int
append_to_register (off_t from, off_t to, int append, ed_buffer_t *ed)
{
  int write_idx = ed->core->regbuf->write_idx;
  FILE *write_fp = ed->core->regbuf->fp[write_idx];
  ed_line_node_t *lp = get_line_node (from, ed);
  off_t size = 0;
  off_t n = from ? to - from + 1 : 0;
  int status = 0;

  /* Nothing to write. */
  if (!n)
    {
      if (!append)
        return reset_register_buffer (write_idx, ed);
      return 0;

    }
  else if (!write_fp)
    {
      if ((status = init_register_buffer (write_idx, ed)) < 0)
        return status;
      write_fp = ed->core->regbuf->fp[write_idx];
    }

  if (append)
    {
      if (FSEEK (write_fp, 0L, SEEK_END) != 0)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("File seek error");
          clearerr (write_fp);
          return ERR;
        }
    }
  else if ((status = reset_register_buffer (write_idx, ed) < 0))
    return status;

  if ((status = write_stream (write_fp, lp, n, &size, ed)) < 0)
    return status;
  return 0;
}


/*
 * inter_register_copy: Write lines from one register to another. If
 *   `append' is zero, any previous contents of target register are
 *   lost.
 */
int
inter_register_copy (int append, ed_buffer_t *ed)
{
  int read_idx = ed->core->regbuf->read_idx;
  int write_idx = ed->core->regbuf->write_idx;
  FILE *read_fp = ed->core->regbuf->fp[read_idx];
  FILE *write_fp = ed->core->regbuf->fp[write_idx];
  size_t size;
  int status = 0;

  /* Copying register to itself. */
  if (read_idx == write_idx)
    {
      if (read_fp && append)
        {
          ed->exec->err = _("Cannot append register to itself");
          return ERR;
        }
      return 0;
    }

  /* Nothing to write. */
  else if (!read_fp)
    {
      if (!append)
        return reset_register_buffer (write_idx, ed);
      return 0;

    }
  else if (!write_fp)
    {
      if ((status = init_register_buffer (write_idx, ed)) < 0)
        return status;
      write_fp = ed->core->regbuf->fp[write_idx];
    }


  if (FSEEK (read_fp, 0L, SEEK_SET) != 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (read_fp);
      return ERR;
    }

  if (append)
    {
      if (FSEEK (write_fp, 0L, SEEK_END) != 0)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("File seek error");
          clearerr (write_fp);
          return ERR;
        }
    }
  else if ((status = reset_register_buffer (write_idx, ed) < 0))
    return status;

  if ((status = append_stream (write_fp, read_fp, &size, ed)) < 0)
    return status;
  return 0;
}


/*
 * inter_register_move: Move lines from one register to another. If `append'
 *   is zero, any previous contents of target register are lost.
 */
int
inter_register_move (int append, ed_buffer_t *ed)
{
  int read_idx = ed->core->regbuf->read_idx;
  int write_idx = ed->core->regbuf->write_idx;
  FILE *read_fp = ed->core->regbuf->fp[read_idx];
  FILE *write_fp = ed->core->regbuf->fp[write_idx];
  size_t size = 0;
  int status = 0;

  /* Overwriting register with itself. */
  if (read_idx == write_idx)
    return 0;

  /* Nothing to write. */
  else if (!read_fp)
    {
      if (!append)
        return reset_register_buffer (write_idx, ed);
      return 0;
    }
  else if (!write_fp)
    {
      /* Swap registers. */
      spl1 ();
      ed->core->regbuf->fp[write_idx] = read_fp;
      ed->core->regbuf->path[write_idx].name =
        ed->core->regbuf->path[read_idx].name;
      ed->core->regbuf->path[write_idx].size =
        ed->core->regbuf->path[read_idx].size;

      ed->core->regbuf->fp[read_idx] = NULL;
      ed->core->regbuf->path[read_idx].name = NULL;
      ed->core->regbuf->path[read_idx].size = 0;
      spl0 ();
      return 0;
    }

  if (FSEEK (read_fp, 0L, SEEK_SET) != 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (read_fp);
      return ERR;
    }
  if (append)
    {
      if (FSEEK (write_fp, 0L, SEEK_END) != 0)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("File seek error");
          clearerr (write_fp);
          return ERR;
        }
    }
  else if ((status = reset_register_buffer (write_idx, ed)) < 0)
    return status;

  if ((status = append_stream (write_fp, read_fp, &size, ed)) < 0)
    return status;
  else if ((status = reset_register_buffer (read_idx, ed)) < 0)
    return status;
  return 0;
}


/* reset_register_buffer: Release nodes of given register. */
int
reset_register_buffer (int idx, ed_buffer_t *ed)
{
  FILE *fp = ed->core->regbuf->fp[idx];

  if (!fp)
    return 0;
  else if (FSEEK (fp, 0L, SEEK_SET) != 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (fp);
      return ERR;
    }
  else if (fflush (fp) == EOF || ftruncate (fileno (fp), 0) == -1)
    {
      fprintf (stderr, "%s: %s\n", ed->file->name, strerror (errno));
      ed->exec->err = _("File truncate error");
      return ERR;
    }
  return 0;
}

#ifdef WANT_ED_MACRO
int
exec_macro (ed_buffer_t *ed)
{
  off_t start = ed->exec->region->start;
  off_t end = ed->exec->region->end;
  off_t addr;
  size_t len;
  int status = 0;

  if (!ed->core->sp && !ed->exec->global)
    reset_undo_queue (ed);
  if ((status = script_from_register (ed)) < 0 || !ed->exec->fp)
    goto err;

  /* Execute macro over region. */
  for (addr = start; addr <= end && addr <= ed->state->lines; ++addr)
    {
      /* Position stdin to beginning of macro. */
      if (FSEEK (stdin, ed->core->stack_frame[ed->core->sp - 1]->size,
                 SEEK_SET) < 0)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("File seek error");
          clearerr (stdin);
          status = ERR;
          break;
        }

      for (ed->state->dot = addr;;)
        {
          if (!(ed->input = get_stdin_line (&len, ed)))
            {
              status = feof (stdin) ? EOF : ERR;
              clearerr (stdin);
              break;
            }
          else if (*(ed->input + len - 1) != '\n')
            {
              ed->exec->err = _("End-of-file unexpected");
              status = ERR;
              clearerr (stdin);
              break;
            }

          if ((status = address_range (ed)) < 0
              || (status = exec_command (ed)) < 0
              || ((ed->display->dio_f = status) > 0
                  && (status = display_lines (ed->state->dot,
                                              ed->state->dot, ed)) < 0))
            break;
        }
    }
 err:

  /* Pop stack frame, or unwind on error. */
  return (status < 0 && status != EOF ? unwind_stack_frame (status, ed)
          : pop_stack_frame (ed));
}

/*
 * script_from_register: Append lines from register to script buffer and
 *   redirect buffer contents as standard input.
 */
static int
script_from_register (ed_buffer_t *ed)
{
  int read_idx = ed->core->regbuf->read_idx;
  FILE *read_fp = ed->core->regbuf->fp[read_idx];
  size_t size = 0;
  int status;

  if (ed->core->sp >= STACK_FRAMES_MAX)
    {
      ed->exec->err = _("Exceeded maximum stack frame depth");
      return ERR;
    }
  else if ((status = push_stack_frame (ed)) < 0)
    return status;

  if (!read_fp)
    {
      /* Can't return until stdin initialized. */
      if ((status = init_register_buffer (read_idx, ed)) < 0)
        return status;
      read_fp = ed->core->regbuf->fp[read_idx];
    }
  else if (FSEEK (read_fp, 0L, SEEK_SET) == -1)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (read_fp);
      return ERR;
    }

  return ((!ed->exec->fp && (status = init_script_buffer (ed)) < 0)
          || (status = append_stream (ed->exec->fp, read_fp, &size, ed)) < 0
          || (status = init_stdio (ed)) < 0) ? status : 0;
}


/* push_stack_frame: Push macro frame on script buffer. */
static int
push_stack_frame (ed_buffer_t *ed)
{
  spl1 ();

  /* Create new frame. */
  if (!(ed->core->stack_frame[ed->core->sp] =
        (struct ed_stack_frame *) malloc (ED_STACK_FRAME_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      spl0 ();
      return ERR;
    }

  /* Push file pointer, input size and return address. */
#if defined(__sun) || defined(__NetBSD__) || defined(__OpenBSD__)
  int fd;

  if ((fd = dup(fileno(stdin))) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("dup error");
      spl0 ();
      return ERR;
    }
  else if ((ed->core->stack_frame[ed->core->sp]->fp =
            fdopen (fd, "r")) == NULL)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("fdopen error");
      spl0 ();
      return ERR;
    }
#else
  ed->core->stack_frame[ed->core->sp]->fp = stdin;
#endif
  ed->core->stack_frame[ed->core->sp]->addr = 0;
  ed->core->stack_frame[ed->core->sp]->size = 0;
  if (ed->core->sp || ed->exec->opt & FSCRIPT)
    {
      if ((ed->core->stack_frame[ed->core->sp]->addr =
              FTELL (ed->exec->fp)) == -1
          || FSEEK (ed->exec->fp, 0, SEEK_END) == -1
          || (ed->core->stack_frame[ed->core->sp]->size =
              FTELL (ed->exec->fp)) == -1)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("File seek error");
          spl0 ();
          return ERR;
        }
    }

  ++ed->core->sp;
  spl0 ();
  return 0;
}


/* pop_stack_frame: Pop macro frame from script buffer. */
static int
pop_stack_frame (ed_buffer_t *ed)
{
  spl1 ();
  --ed->core->sp;

  /* Restore file pointer, input size and return address. */
#if defined(__sun) || defined(__NetBSD__) || defined(__OpenBSD__)
  if (dup2 (fileno (ed->core->stack_frame[ed->core->sp]->fp),
            fileno (stdin)) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("dup2 error");
      spl0 ();
      return ERR;
    }
#else
  stdin = ed->core->stack_frame[ed->core->sp]->fp;
#endif
  if (FSEEK (ed->exec->fp, 0, SEEK_CUR) != -1
      && (ftruncate (fileno (ed->exec->fp),
                    ed->core->stack_frame[ed->core->sp]->size) == -1
          || fflush (ed->exec->fp) == EOF
          || FSEEK (ed->exec->fp,
                    ed->core->stack_frame[ed->core->sp]->addr, SEEK_SET) == -1))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      spl0 ();
      return ERR;
    }
  free (ed->core->stack_frame[ed->core->sp]);
  spl0 ();

  return 0;
}


int
unwind_stack_frame (int status, ed_buffer_t *ed)
{
  if (ed->core->sp)
    {
      spl1 ();
      --ed->core->sp;

      /* Restore file pointer and input size. */
#if defined(__sun) || defined(__NetBSD__) || defined(__OpenBSD__)
      if (dup2 (fileno (ed->core->stack_frame[0]->fp), fileno (stdin)) < 0)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("dup2 error");
          spl0 ();
          return ERR;
        }
#else
      stdin = ed->core->stack_frame[0]->fp;
#endif
      if (FSEEK (ed->exec->fp, 0, SEEK_CUR) != -1
          && (ftruncate (fileno (ed->exec->fp),
                         ed->core->stack_frame[0]->size) == -1
              || fflush (ed->exec->fp) == EOF
              || FSEEK (ed->exec->fp,
                    ed->core->stack_frame[0]->addr, SEEK_SET) == -1))
        {
          fprintf (stderr, "stdin: %s\n", strerror (errno));
          ed->exec->err = _("File seek error");
          spl0 ();
          return ERR;
        }
      while (ed->core->sp > 0)
        free (ed->core->stack_frame[ed->core->sp--]);
      free (ed->core->stack_frame[ed->core->sp]);
      spl0 ();
    }
  return status;
}

#endif /* !WANT_ED_MACRO */
#else  /* !WANT_ED_REGISTER */
int
unwind_stack_frame (int status, ed_buffer_t *ed)
{
  return 0;
}
#endif  /* !WANT_ED_REGISTER */
