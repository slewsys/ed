/* glbl.c: Global command implentation for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2013-08-09 <alm@slewsys.org>

   This file is part of ed. */

#include "ed.h"


/* Static function declarations. */
static ed_global_node_t *append_global_node __P ((const ed_line_node_t *,
                                               ed_state_t *));
static ed_line_node_t *next_global_node __P ((ed_state_t *));


/* mark_global_nodes: Add lines matching a pattern to global queue. */
int
mark_global_nodes (want_match, ed)
     int want_match;
     ed_state_t *ed;
{
  regmatch_t rm[1];
  regex_t *re;
  ed_line_node_t *lp;
  off_t from = ed->region.start;
  off_t to = ed->region.end;
  off_t n = from ? to - from + 1 : 0;
  char dc = *ed->input;              /* pattern delimiting char */
  char *s;

  spl1 ();
  if (!(re = get_compiled_regex (dc, RE_SEARCH, ed)))
    {
      spl0 ();
      return ERR;
    }
  if (*ed->input == dc && *ed->input != '\n')
    ++ed->input;
  reset_global_queue (ed);
  spl0 ();
  lp = get_line_node (from, ed);
  for (; n; --n, lp = lp->q_forw)
    {
      if (!(s = get_buffer_line (lp, ed)))
        return ERR;
#ifdef REG_STARTEND
      rm[0].rm_so = 0;
      rm[0].rm_eo = lp->len;
      if (!regexec (re, s, 0, rm, REG_STARTEND) == want_match
          && !append_global_node (lp, ed))
#else
      if (ed->buf[0].is_binary)
        NUL_TO_NEWLINE (s, lp->len);
      if (!regexec (re, s, 0, NULL, 0) == want_match
          && !append_global_node (lp, ed))
#endif  /* !REG_STARTEND */
        return ERR;
    }
  return 0;
}


/* exec_global: Apply command list to lines in a range in global queue;
   return command status. */
int
exec_global (io_f, ed)
     unsigned io_f;             /* I/O flags */
     ed_state_t *ed;
{
  static char *gcb = NULL;      /* global command buffer */
  static size_t gcb_size = 0;   /* buffer size */

  ed_line_node_t *lp = NULL;
  size_t len;
  int first_time = 1;
  int interactive = ed->exec.global & GLBI;
  int status;

  reset_undo_queue (ed);

  /* If non-interactive, read command before entering loop in the
     event of empty global queue. */
  if (!interactive && !(ed->input = get_extended_line (&len, 0, ed)))
    {
      status = ERR;
      clearerr (stdin);
      return status;
    }
  for (ed->exec.first_pass = 1; (lp = next_global_node (ed));
       ed->input = gcb, ed->exec.first_pass = 0)
    {
      if ((status = get_line_node_address (lp, &ed->buf[0].dot, ed)) < 0
          || (interactive
              && (status = display_lines (ed->buf[0].dot,
                                          ed->buf[0].dot, io_f, ed)) < 0))
        return status;

      /* If `G/V' command, then read from stdin. */
      if (interactive
          && (!(ed->input = get_stdin_line (&len, ed))
              || !(ed->input = get_extended_line (&len, 0, ed))))
        {
          /* For an interactive global command, permit EOF to cancel. */
          status = feof (stdin) ? 0 : ERR;
          clearerr (stdin);
          return status;
        }

      /* Global non-interactive command already set. */
      if (ed->input == gcb)
        ;

      /* Skip to next line */
      else if (len == 1 && interactive)
        continue;

      /* Repeat previous command. */
      else if (len == 2 && *ed->input == '&' && interactive)
        {
          if (first_time)
            {
              ed->exec.err = _("No previous command");
              return ERR;
            }
        }
      else
        {
          /* get_extended_line () isn't reentrant, so save ed->input. */
          REALLOC_THROW (gcb, gcb_size, len + 1, ERR, ed);

          /* Assert: ed->input is NUL-terminated! */
          strcpy (gcb, ed->input);
          first_time = 0;
        }
      for (ed->input = gcb; *ed->input;)
        if ((status =
             address_range (ed)) < 0
            || (status = exec_command (ed)) < 0)
          return status;
    }
  return 0;
}


/* append_global_node: Append node to end of global queue. Return node
   pointer. */
static ed_global_node_t *
append_global_node (lp, ed)
     const ed_line_node_t *lp;
     ed_state_t *ed;
{
  ed_global_node_t *ap;
  ed_global_node_t *global_last = ed->core.global_head.q_back;

  spl1 ();
  if (!(ap = (ed_global_node_t *) malloc (ED_GLOBAL_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec.err = _("Memory exhausted");
      spl0 ();
      return NULL;
    }
  ap->lp = (ed_line_node_t *) lp;
  APPEND_NODE (ap, global_last);
  spl0 ();
  return ap;
}


/* next_global_node: Pop node from top of global queue; return line
   node pointer. */
static ed_line_node_t *
next_global_node (ed)
     ed_state_t *ed;
{
  ed_global_node_t *ap = ed->core.global_head.q_forw;
  ed_line_node_t *lp = ap->lp;

  if (ap != &ed->core.global_head)
    {
      spl1 ();
      UNLINK_NODE (ap);
      free (ap);
      spl0 ();
    }
  return lp;
}


/* reset_global_queue: Initialize and reset global queue. */
void
reset_global_queue (ed)
     ed_state_t *ed;
{
  /* Assert: spl1 () */

  delete_global_nodes (ed->core.global_head.q_forw->lp,
                       ed->core.global_head.lp, ed);
}


/* SEEK_GLOBAL_LINE: Seek global line node between min and max. */
#define SEEK_GLOBAL_LINE(global, head, min, max)                              \
  do                                                                          \
    {                                                                         \
      global = head;                                                          \
      while (global->lp != min                                                \
             && (global = global->q_forw) != &ed->core.global_head)           \
        ;                                                                     \
      if (global->lp == min || min == max)                                    \
        break;                                                                \
      min = min->q_forw;                                                      \
    }                                                                         \
  while (1)


/* delete_global_nodes: Delete range of global nodes, excluding end. */
void
delete_global_nodes (begin, end, ed)
     const ed_line_node_t *begin;
     const ed_line_node_t *end;
     ed_state_t *ed;
{
  ed_line_node_t *first = (ed_line_node_t *) begin;
  ed_line_node_t *last = (ed_line_node_t *) end;
  ed_global_node_t *from, *to, *next;

  /* Assert: get_line_node_address (first) <= get_line_node_address (last) */

  SEEK_GLOBAL_LINE (from, &ed->core.global_head, first, last);

  if (first != last)
    SEEK_GLOBAL_LINE (to, from->q_forw, last, &ed->core.buffer_head);

  /* Assert: spl1 () */

  for (; from != &ed->core.global_head && from->lp != last; from = next)
    {
      next = from->q_forw;
      UNLINK_NODE (from);
      free (from);
    }
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
