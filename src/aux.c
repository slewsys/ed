/* aux.c: Auxiliary editor commands for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2012-12-11 <alm@buttercup.local>

   This file is part of ed. */

#include "ed.h"


#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifndef WIFEXITED
# define WIFEXITED(status) (((status) & 0xff) == 0)
#endif

#ifndef WEXITSTATUS
# define WEXITSTATUS(status) ((unsigned)(status) >> 8)
#endif


/* Static function declarations. */
static ed_line_node_t *append_register_node __P ((size_t, off_t, int,
                                               ed_state_t *));


#if defined HAVE_FORK && defined WANT_EXTERNAL_FILTER
# define WAITPID(pid)                                                         \
  do                                                                          \
    {                                                                         \
      waitpid (pid, &status, 0);                                              \
      if (!status && WIFEXITED (status) && WEXITSTATUS (status) == 127)       \
        {                                                                     \
          ed->exec.err = _("Child process error");                            \
          status = ERR;                                                       \
        }                                                                     \
    }                                                                         \
  while (0)


/* filter_lines: Filter a range of lines through a shell command;
   return status. */
int
filter_lines (from, to, sc, ed)
     off_t from;
     off_t to;
     const char *sc;
     ed_state_t *ed;
{
  FILE *ipp, *opp;
  ed_line_node_t *lp = get_line_node (from, ed->buf);
  off_t n = from ? to - from + 1 : 0;
  pid_t shell_pid, write_pid;
  off_t size = 0;
  int ip[2], op[2];
  int status = 0;

  /* Flush any buffered I/O. */
  fflush (NULL);

  /* Create two pipes: one for writing to the shell command and
     another for reading from it.  */
  if (pipe (ip) < 0 || pipe (op) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Pipe open error");
      return ERR;
    }

  /* Create first child process for shell command. */
  switch (shell_pid = fork ())
    {
    case -1:
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Fork error");
      close (ip[0]), close (ip[1]);
      close (op[0]), close (op[1]);
      status = ERR;
      goto err;
    case 0:
      /* Reset signals for shell. */
      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);

      /* Redirect read/write pipes to command's standard I/O.  Close
         write end of pipe ip, ip[1], so that EOF is seen.  Execute
         shell command. */
      if (dup2 (ip[0], 0) < 0 || dup2 (op[1], 1) < 0 || dup2 (op[1], 2) < 0
          || close (ip[0]) < 0 || close (ip[1]) < 0
          || close (op[0]) < 0 || close (op[1]) < 0
          || execl ("/bin/sh", "sh", "-c", sc, NULL))
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
  if (ed->buf[0].addr_last == 0)
    {
      ed->buf[0].is_binary = 0;
      ed->buf[0].is_empty = 1;
    }
  spl0 ();

  /* Create second child process for writing to shell command. */
  switch (write_pid = fork ())
    {
    case -1:
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Fork error");
      (void) close (ip[1]);
      (void) close (op[0]);
      status = ERR;
      goto err;
    case 0:

      /* Close unused pipe ends, then open and write to shell
         process's standard input. */
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
      ed->exec.err = _("Pipe open error");
      status = ERR;
      goto err;
    }

  /* Read standard output of shell process via reentrant read_stream. */
  if ((status = read_stream_r (opp, ed->buf[0].dot, &size, ed)) < 0)
    goto err;
 
  printf (ed->opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n", size);

  if (fclose (opp) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Pipe close error");
      status = ERR;
      goto err;
    }
  WAITPID (write_pid);

 err:
  WAITPID (shell_pid);
  return status;
}
#endif  /* defined HAVE_FORK && defined WANT_EXTERNAL_FILTER */


/* Global declarations */
ed_line_node_t *register_head[REG_MAX]; /* list of register queues */
ed_line_node_t *register_p[REG_MAX];    /* register queue pointers */


/* append_register_node: Append node to end of given register.
   Return node pointer. */
static ed_line_node_t *
append_register_node (len, offset, qno, ed)
     size_t len;
     off_t offset;
     int qno;
     ed_state_t *ed;
{
  ed_line_node_t *lp;

  if (!(lp = (ed_line_node_t *) malloc (ED_LINE_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Memory exhausted");
      return NULL;
    }
  lp->len = len;
  lp->seek = offset;
  APPEND_NODE (lp, register_head[qno]->q_back);
  return lp;
}


/* copy_register: Write lines from one register to another. If
   `overwrite' is non-zero, any previous contents of target register
   are lost. */
int
copy_register (from, to,  overwrite, ed)
     int from;                  /* source register */
     int to;                    /* destination register */
     int overwrite;
     ed_state_t *ed;
{
  ed_line_node_t *lp, *ep;

  if (!register_head[from]
      && init_register_queue (register_head, from, ed) < 0)
    return ERR;
  if (!register_head[to] && init_register_queue (register_head, to, ed) < 0)
    return ERR;
  if (overwrite && from != to && reset_register_queue (to, ed) < 0)
    return ERR;

  /* Handle case of register appended to itself. */
  if (!(overwrite && from == to))
    for (ep = register_head[from], lp = ep->q_forw; lp != ep; lp = lp->q_forw)
      if (!append_register_node (lp->len, lp->seek, to, ed))
        return ERR;
  return 0;
}
  

/* move_register: Move lines from one register to another. If
   `overwrite' is non-zero, any previous contents of target register
   are lost. */
int
move_register (from, to,  overwrite, ed)
     int from;                  /* source register */
     int to;                    /* destination register */
     int overwrite;
     ed_state_t *ed;
{
  if (!register_head[from]
      && init_register_queue (register_head, from, ed) < 0)
    return ERR;
  if (!register_head[to] && init_register_queue (register_head, to, ed) < 0)
    return ERR;
  if (overwrite && from != to && reset_register_queue (to, ed) < 0)
    return ERR;

  if (from != to && register_head[from]->q_forw != register_head[from])
    {
      spl1 ();
      LINK_NODES (register_head[to]->q_back, register_head[from]->q_forw);
      LINK_NODES (register_head[from]->q_back, register_head[to]);
      LINK_NODES (register_head[from], register_head[from]);
      spl0 ();
    }
  return 0;
}


/* read_register: Append lines from register to buffer after given
   address. */
int
read_register (qno, addr, ed)
     int qno;                    /* source register */
     off_t addr;                /* destination address */
     ed_state_t *ed;
{
  ed_undo_node_t *up = NULL;
  ed_line_node_t *lp, *np, *ep;

  if (!register_head[qno] && init_register_queue (register_head, qno, ed) < 0)
    return ERR;

  ed->buf[0].dot = addr;
  for (ep = register_head[qno], lp = ep->q_forw; lp != ep; lp = lp->q_forw)
    {
      spl1 ();
      if (!(np = append_line_node (lp->len, lp->seek, addr, ed)))
        {
          spl0 ();
          return ERR;
        }
      APPEND_UNDO_NODE (np, up, ed->buf[0].dot);
      ed->buf[0].is_modified = 1;
      spl0 ();
    }
  return 0;
}
  

/* reset_register_queue: Release nodes of given register. */
int
reset_register_queue (qno, ed)
     int qno;
     ed_state_t *ed;
{
  ed_line_node_t *lp, *np, *ep;

  if (!register_head[qno] && init_register_queue (register_head, qno, ed) < 0)
    return ERR;

  spl1 ();
  for (ep = register_head[qno], lp = ep->q_forw; lp != ep; lp = np)
    {
      np = lp->q_forw;
      LINK_NODES (lp->q_back, np);
      free (lp);
    }
  spl0 ();
  return 0;
}


/* write_register: Write addressed lines to given register. If
   `overwrite' is non-zero, any previous register contents are
   lost. */
int
write_register (qno, from, to, overwrite, ed)
     int qno;                   /* destination register */
     off_t from;                /* from address */
     off_t to;                  /* to address */
     int overwrite;
     ed_state_t *ed;
{

  ed_line_node_t *lp = get_line_node (from, ed->buf);
  off_t n = from ? to - from + 1 : 0;

  if (!register_head[qno] && init_register_queue (register_head, qno, ed) < 0)
    return ERR;
  if (overwrite && reset_register_queue (qno, ed) < 0)
    return ERR;

  for (; n; --n, lp = lp->q_forw)
    if (!append_register_node (lp->len, lp->seek, qno, ed))
      return ERR;
  return 0;
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
