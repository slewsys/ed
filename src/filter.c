/* filter.c: External filter command for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
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
      int waiting = write_pid == -1 ? 1 : 2;                                  \
                                                                              \
      while (waiting)                                                         \
        if ((any_pid = waitpid (-1, &pid_status, 0)) < 0)                     \
          {                                                                   \
            if (errno != EINTR)                                               \
              {                                                               \
                fprintf (stderr, "%s\n", strerror (errno));                   \
                ed->exec->err = _("Child process error");                     \
                status = ERR;                                                 \
                goto end;                                                     \
              }                                                               \
            errno = 0;                                                        \
          }                                                                   \
        else                                                                  \
          {                                                                   \
            if (any_pid == shell_pid)                                         \
              saved_pid_status = pid_status;                                  \
            --waiting;                                                        \
          }                                                                   \
                                                                              \
      if (WIFEXITED (saved_pid_status) && WEXITSTATUS (saved_pid_status))     \
        {                                                                     \
          snprintf (exit_status, sizeof exit_status, _("Exit status: %#x"),   \
                    WEXITSTATUS (saved_pid_status));                          \
          ed->exec->err = exit_status;                                        \
        }                                                                     \
    end:                                                                      \
      ;                                                                       \
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
      signal (SIGPIPE, SIG_DFL);

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
  if (ed->exec->region->addrs && (status = delete_lines (from, to, ed)) < 0)
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
          || (ed->exec->region->addrs
              && (status = write_stream (ipp, lp, n, &size, ed)) < 0)
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
  if (shell_pid != -1)
    WAITPID ();
  return status;
}
#endif  /* defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER) */
