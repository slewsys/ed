/* aux.c: Auxiliary editor commands for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2024 Andrew L. Moore, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"

#if defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER)
# define WAITPID()                                                            \
  do                                                                          \
    {                                                                         \
      pid_t any_pid;                                                          \
      int pid_status = 0;                                                     \
      int saved_pid_status = 0;                                               \
      int count = 0;                                                          \
      while (shell_pid != -1 && write_pid != -1 && count < 2)                 \
        {                                                                     \
          if ((any_pid = waitpid (-1, &pid_status, 0)) == shell_pid)          \
            saved_pid_status = pid_status;                                    \
          count += 1;                                                         \
        }                                                                     \
      if (saved_pid_status && WIFEXITED (saved_pid_status)                    \
          && WEXITSTATUS (saved_pid_status) == 127)                           \
        {                                                                     \
          ed->exec->err = _("Child process error");                           \
          status = ERR;                                                       \
        }                                                                     \
      else if (saved_pid_status)                                              \
        {                                                                     \
          snprintf (exit_status, sizeof exit_status, _("Exit status: %#x"),   \
                    WEXITSTATUS (saved_pid_status));                          \
          ed->exec->err = exit_status;                                        \
        }                                                                     \
    }                                                                         \
  while (0)


/*
 * filter_lines: Filter a range of lines through a shell command;
 *    return status.
 */
int
filter_lines (off_t from, off_t to, const char *sc, ed_buffer_t *ed)
{
  static char exit_status[BUFSIZ];

  FILE *ipp, *opp;
  ed_line_node_t *lp = get_line_node (from, ed);
  off_t n = from ? to - from + 1 : 0;
  pid_t shell_pid, write_pid;
  off_t size = 0;
  int ip[2], op[2];
  int status = 0;

  /* Flush any buffered I/O. */
  if (fflush (NULL) < 0) {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Flush error");
      return ERR;
  }

  /*
   * Create two pipes: one for writing to the shell command and
   * another for reading from it.
   */
  if (pipe (ip) < 0 || pipe (op) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Pipe open error");
      return ERR;
    }

  /* Create first child process for shell command. */
  switch (shell_pid = fork ())
    {
    case -1:
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Fork error");
      close (ip[0]), close (ip[1]);
      close (op[0]), close (op[1]);
      status = ERR;
      goto err;
    case 0:
      /* Reset signals for shell. */
      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);

      /*
       * Redirect read/write pipes to command's standard I/O. Close
       * write end of pipe ip, ip[1], so that EOF is seen. Execute
       * shell command.
       */
      if (dup2 (ip[0], 0) < 0 || dup2 (op[1], 1) < 0
          /* || dup2 (op[1], 2)  < 0 */
          || close (ip[0]) < 0 || close (ip[1]) < 0
          || close (op[0]) < 0 || close (op[1]) < 0
          || execl ("/bin/sh", "sh", "-c", sc, NULL) < 0)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          _exit (127 << 8);
        }

      /* NOTREACHED */
    }

  /* Close write end of pipe op, op[1], so that EOF is seen. */
  close (op[1]);

  spl1 ();
  if ((status = delete_lines (from, to, ed)) < 0)
    {
      kill (shell_pid, SIGINT);
      spl0 ();
      goto err;
    }

  /* If filtering entire file, reset state. */
  if (ed->state->lines == 0)
    {
      ed->state->is_binary = 0;
      ed->state->is_empty = 1;
    }
  spl0 ();

  /* Create second child process for writing to shell command. */
  switch (write_pid = fork ())
    {
    case -1:
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Fork error");
      (void) close (ip[1]);
      (void) close (op[0]);
      status = ERR;
      goto err;
    case 0:

      /*
       * Close unused pipe ends, then open and write to shell
       * process's standard input.
       */
      if (close (ip[0]) < 0 || close (op[0]) < 0
          || !(ipp = fdopen (ip[1], "w"))
          || (status = write_stream (ipp, lp, n, &size, ed)) < 0
          || fclose (ipp) < 0)
        {
          fprintf (stderr, "%s\n", strerror (errno));
          status = ERR;
        }

      /* Avoid exit (3). */
      _exit (status < 0 ? 127 << 8 : 0);
    }

  /* Close unused pipe ends, then open shell process's standard output. */
  if (close (ip[0]) < 0 || close (ip[1]) < 0 || !(opp = fdopen (op[0], "r")))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Pipe open error");
      status = ERR;
      goto err;
    }

  /* Read standard output of shell process via reentrant read_stream. */
  if ((status = read_stream_r (opp, ed->state->dot, &size, ed)) < 0)
    goto err;

  printf (ed->exec->opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n", size);

  if (fclose (opp) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Pipe close error");
      status = ERR;
      goto err;
    }

 err:
  WAITPID ();
  return status;
}
#endif  /* defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER) */

#ifdef WANT_ED_MACRO
/*
 * script_from_register: Append lines from register to script buffer and
 *   redirect buffer contents as standard input.
 */
int
script_from_register (ed_buffer_t *ed)
{
  int read_idx = ed->core->regbuf->read_idx;
  FILE *read_fp = ed->core->regbuf->fp[read_idx];
  size_t size = 0;
  off_t offset = 0;
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

  if (!ed->exec->fp)
    {
      if ((status = init_script_buffer (ed)) < 0)
        return status;
    }
  else if (FSEEK (ed->exec->fp, 0L, SEEK_END) == -1
           || (offset = FTELL (ed->exec->fp)) == -1)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (ed->exec->fp);
      return ERR;
    }

  if ((status = append_stream (ed->exec->fp, read_fp, &size, ed)) < 0)
    return status;

  if ((status = init_stdio (ed)) < 0)
    return status;
  return 0;
}


/* push_stack_frame: Push macro frame on script buffer. */
int
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
  if (dup2 (fileno (stdin),
            fileno (ed->core->stack_frame[ed->core->sp]->fp)) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("dup2 error");
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
int
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

#else  /* !WANT_ED_MACRO */
int
unwind_stack_frame (int status, ed_buffer_t *ed)
{
  return 0;
}
#endif /* !WANT_ED_MACRO */
