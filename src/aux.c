/* aux.c: Auxiliary editor commands for the ed line editor.

   Copyright © 1993-2014 Andrew L. Moore, SlewSys Research

   Last modified: 2014-03-12 <alm@slewsys.org>

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
static ed_line_node_t *append_node_to_register __P ((size_t, off_t, int,
                                               ed_buffer_t *));


#if defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER)
# define WAITPID(pid)                                                         \
  do                                                                          \
    {                                                                         \
      waitpid (pid, &status, 0);                                              \
      if (!status && WIFEXITED (status) && WEXITSTATUS (status) == 127)       \
        {                                                                     \
          ed->exec->err = _("Child process error");                           \
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
     ed_buffer_t *ed;
{
  FILE *ipp, *opp;
  ed_line_node_t *lp = get_line_node (from, ed);
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
  if (ed->state[0].lines == 0)
    {
      ed->state[0].is_binary = 0;
      ed->state[0].is_empty = 1;
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
      ed->exec->err = _("Pipe open error");
      status = ERR;
      goto err;
    }

  /* Read standard output of shell process via reentrant read_stream. */
  if ((status = read_stream_r (opp, ed->state[0].dot, &size, ed)) < 0)
    goto err;
 
  printf (ed->exec->opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n", size);

  if (fclose (opp) < 0)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Pipe close error");
      status = ERR;
      goto err;
    }
  WAITPID (write_pid);

 err:
  WAITPID (shell_pid);
  return status;
}
#endif  /* defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER) */


/* append_node_to_register: Append node to end of given register.
   Return node pointer. */
static ed_line_node_t *
append_node_to_register (len, offset, qno, ed)
     size_t len;
     off_t offset;
     int qno;
     ed_buffer_t *ed;
{
  ed_line_node_t *lp;
  ed_line_node_t *qt = ed->core->reg[qno]->q_back;

  spl1 ();
  if (!(lp = (ed_line_node_t *) malloc (ED_LINE_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      spl0 ();
      return NULL;
    }
  lp->len = len;
  lp->seek = offset;

  /* APPEND_NODE is macro, so qt is mandatory! */
  APPEND_NODE (lp, qt);
  spl0 ();
  return lp;
}


/* append_from_register: Append lines from register to buffer after given
   address. */
int
read_from_register (qno, addr, ed)
     int qno;                   /* source register */
     off_t addr;                /* destination address */
     ed_buffer_t *ed;
{
  ed_undo_node_t *up = NULL;
  ed_line_node_t *lp, *rp, *rh;

  if (!ed->core->reg[qno] && init_register_queue (qno, ed) < 0)
    return ERR;

  for (rh = ed->core->reg[qno], rp = rh->q_forw; rp != rh; rp = rp->q_forw)
    {
      spl1 ();
      if (!(lp = append_line_node (rp->len, rp->seek, addr, ed)))
        {
          spl0 ();
          return ERR;
        }
      ed->state[0].dot = ++addr;
      APPEND_UNDO_NODE (lp, up, addr, ed);
      ed->state[0].is_modified = 1;
      spl0 ();
    }
  return 0;
}
  

/* register_copy: Write lines from one register to another. If
   `append' is zero, any previous contents of target register
   are lost. */
int
register_copy (from, to,  append, ed)
     int from;                  /* source register */
     int to;                    /* destination register */
     int append;
     ed_buffer_t *ed;
{
  ed_line_node_t *rp, *rh;

  if (!ed->core->reg[from]
      && init_register_queue (from, ed) < 0)
    return ERR;
  if (!ed->core->reg[to] && init_register_queue (to, ed) < 0)
    return ERR;
  if (!append && from != to && reset_register_queue (to, ed) < 0)
    return ERR;

  /* Handle case of register appended to itself. */
  if (append && from != to)
    for (rh = ed->core->reg[from], rp = rh->q_forw; rp != rh; rp = rp->q_forw)
      if (!append_node_to_register (rp->len, rp->seek, to, ed))
        return ERR;
  return 0;
}
  

/* move_register: Move lines from one register to another. If
   `append' is zero, any previous contents of target register
   are lost. */
int
register_move (from, to,  append, ed)
     int from;                  /* source register */
     int to;                    /* destination register */
     int append;
     ed_buffer_t *ed;
{
  if (!ed->core->reg[from]
      && init_register_queue (from, ed) < 0)
    return ERR;
  if (!ed->core->reg[to] && init_register_queue (to, ed) < 0)
    return ERR;
  if (!append && from != to && reset_register_queue (to, ed) < 0)
    return ERR;

  if (from != to && ed->core->reg[from]->q_forw != ed->core->reg[from])
    {
      spl1 ();
      LINK_NODES (ed->core->reg[to]->q_back, ed->core->reg[from]->q_forw);
      LINK_NODES (ed->core->reg[from]->q_back, ed->core->reg[to]);
      LINK_NODES (ed->core->reg[from], ed->core->reg[from]);
      spl0 ();
    }
  return 0;
}


/* reset_register_queue: Release nodes of given register. */
int
reset_register_queue (qno, ed)
     int qno;
     ed_buffer_t *ed;
{
  ed_line_node_t *rp, *rn, *rh;

  if (!ed->core->reg[qno] && init_register_queue (qno, ed) < 0)
    return ERR;

  spl1 ();
  for (rh = ed->core->reg[qno], rp = rh->q_forw; rp != rh; rp = rn)
    {
      rn = rp->q_forw;
      UNLINK_NODE (rp);
      free (rp);
    }
  spl0 ();
  return 0;
}


/* write_to_register: Write addressed lines to given register. If
   `append' is zero, any previous register contents are
   lost. */
int
write_to_register (qno, from, to, append, ed)
     int qno;                   /* destination register */
     off_t from;                /* from address */
     off_t to;                  /* to address */
     int append;
     ed_buffer_t *ed;
{

  ed_line_node_t *lp = get_line_node (from, ed);
  off_t n = from ? to - from + 1 : 0;

  if (!ed->core->reg[qno] && init_register_queue (qno, ed) < 0)
    return ERR;
  if (!append && reset_register_queue (qno, ed) < 0)
    return ERR;

  for (; n; --n, lp = lp->q_forw)
    if (!append_node_to_register (lp->len, lp->seek, qno, ed))
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
