/* cmds.c: Basic editor commands for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2024 Andrew L. Moore, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"


/*
 * append_lines: Insert text from stdin to after given address; stop
 *   when either a single period is read or EOF. Return status.
 */
int
append_lines (off_t after, ed_buffer_t *ed)
{
  char *s;
  size_t len;
  ed_line_node_t *lp;
  ed_undo_node_t *up = NULL;
  int status;

  ed->state->input_is_binary = 0;
  for (ed->state->dot = after;;)
    {
      if (!ed->exec->global)
        {
          if (!(ed->input = get_stdin_line (&len, ed)))
            {
              /* Permit EOF (i.e., key stroke: CTL + D) at beginning of
                 line as alternative to `.' */
              status = feof (stdin) ? 0 : ERR;
              clearerr (stdin);
              return status;
            }
          if (*(ed->input + len - 1) != '\n')
            {
              ed->exec->err = _("End-of-file unexpected");
              clearerr (stdin);
              return ERR;
            }
          ++ed->exec->line_no;
        }
      else if (*ed->input == '\0')
        return 0;
      if (*ed->input == '.' && *(ed->input + 1) == '\n')
        {
          ++ed->input;
          return 0;
        }
      if (ed->exec->global)
        {
          for (s = ed->input; *s++ != '\n';)
            ;
          len = s - ed->input;
        }
      spl1 ();
      ed->state->is_binary |= ed->state->input_is_binary;
      if (after == ed->state->lines)
        ed->state->newline_appended = 0;
      if (!(ed->input = put_buffer_line (ed->input, len, ed)))
        {
          spl0 ();
          return ERR;
        }
      lp = get_line_node (ed->state->dot, ed);
      APPEND_UNDO_NODE (lp, up, ed->state->dot, ed);
      ed->state->is_empty = 0;
      ed->state->is_modified = 1;
      spl0 ();
    }
  /* NOTREACHED */
}


/*
 * copy_lines: Copy a range of lines to after given address. Return status.
 */
int
copy_lines (off_t from, off_t to, off_t after, ed_buffer_t *ed)
{
  ed_line_node_t *lp, *np;
  ed_undo_node_t *up = NULL;
  off_t ci[2];
  int split = (from <= after && after < to);
  int i;

  ci[0] = (split ? after : to) - from + 1;
  ci[1] = (split ? to - after : 0);
  for (i = 0; i < 2; ++i)
    {
      lp = get_line_node (i == 0 ? from : after + 1, ed);
      for (; ci[i]-- > 0; lp = lp->q_forw)
        {
          spl1 ();
          if (!(np = append_line_node (lp->len, lp->seek, after, ed)))
            {
              spl0 ();
              return ERR;
            }
          ed->state->dot = ++after;
          APPEND_UNDO_NODE (np, up, after, ed);
          ed->state->is_modified = 1;
          spl0 ();
        }
    }
  return 0;
}


/* delete_lines: Delete a range of lines. */
int
delete_lines (off_t from, off_t to, ed_buffer_t *ed)
{
  ed_line_node_t *b, *a;

  /* Assert: spl1 () */

  if (!append_undo_node (UDEL, from, to, ed))
    return ERR;
  a = get_line_node (INC_MOD (to, ed->state->lines), ed);

  /* This get_line_node last! */
  b = get_line_node (from - 1, ed);
  if (ed->exec->global)
    delete_global_nodes (b->q_forw, a, ed);
  LINK_NODES (b, a);
  if (to == ed->state->lines)
    ed->state->newline_appended = 0;
  ed->state->lines -= to - from + 1;
  ed->state->dot = from - 1;
  ed->state->is_modified = 1;
  return 0;
}


/*
 * join_lines: Replace a range of lines with the joined text of those lines.
 */
int
join_lines (off_t from, off_t to, ed_buffer_t *ed)
{
  static char *lj = NULL;       /* line join buffer */
  static size_t lj_size;        /* buffer size */

  ed_line_node_t *lp = get_line_node (from, ed);
  off_t n = from ? to - from + 1 : 0;
  size_t len = 0;
  char *s;

  for (; n; --n, lp = lp->q_forw)
    {
      if (!(s = get_buffer_line (lp, ed)))
        return ERR;
      REALLOC_THROW (lj, lj_size, len + lp->len, ERR, ed);
      memcpy (lj + len, s, lp->len);
      len += lp->len;
    }
  REALLOC_THROW (lj, lj_size, len + 2, ERR, ed);
  memcpy (lj + len++, "\n", 2);
  spl1 ();
  if (delete_lines (from, to, ed) < 0)
    {
      spl0 ();
      return ERR;
    }
  ed->state->dot = from - 1;
  if (!put_buffer_line (lj, len, ed)
      || !append_undo_node (UADD, ed->state->dot, ed->state->dot, ed))
    {
      spl0 ();
      return ERR;
    }
  ed->state->is_modified = 1;
  spl0 ();
  return 0;
}


/*
 * move_lines: Move a range of lines to after given address. Return status.
 */
int
move_lines (off_t from, off_t to, off_t after, ed_buffer_t *ed)
{
  ed_line_node_t *b1, *a1, *b2, *a2;
  off_t succ = INC_MOD (to, ed->state->lines);
  off_t prec = from - 1;
  int done = (after == from - 1 || after == to);

  spl1 ();
  if (done)
    {
      a2 = get_line_node (succ, ed);
      b2 = get_line_node (prec, ed);
      ed->state->dot = to;
    }
  else if (!append_undo_node (UMOV, prec, succ, ed)
           || !append_undo_node (UMOV, after,
                                 INC_MOD (after, ed->state->lines), ed))
    {
      spl0 ();
      return ERR;
    }
  else
    {
      a1 = get_line_node (succ, ed);
      if (from > after)
        {
          b1 = get_line_node (prec, ed);

          /* this get_line_node last! */
          b2 = get_line_node (after, ed);
        }
      else
        {
          b2 = get_line_node (after, ed);

          /* this get_line_node last! */
          b1 = get_line_node (prec, ed);
        }
      a2 = b2->q_forw;
      LINK_NODES (b2, b1->q_forw);
      LINK_NODES (a1->q_back, a2);
      LINK_NODES (b1, a1);
      ed->state->dot = after + (from > after ? to - from + 1 : 0);
    }
  if (ed->exec->global)
    delete_global_nodes (b2->q_forw, a2, ed);
  ed->state->is_modified = 1;
  spl0 ();
  return 0;
}


/* mark_line_node: Set a line node mark. */
int
mark_line_node (const ed_line_node_t *lp, int n, ed_buffer_t *ed)
{
  if (!islower (n))
    {
      ed->exec->err = _("Invalid address mark");
      return ERR;
    }
  if (!ed->core->mark[n - 'a'])
    ++ed->core->marks;
  ed->core->mark[n - 'a'] = (ed_line_node_t *) lp;
  return 0;
}


/* get_marked_node_address: Get address of a marked line. Return status. */
int
get_marked_node_address (int n, off_t *addr, ed_buffer_t *ed)
{
  if (!islower (n))
    {
      ed->exec->err = _("Invalid address mark");
      return ERR;
    }
  return get_line_node_address (ed->core->mark[n - 'a'], addr, ed);
}


/* transfer_marks: Transfer marks from line node lp to mp. */
void
transfer_marks (const ed_line_node_t *mp, const ed_line_node_t *lp,
                ed_buffer_t *ed)
{
  int n = 25;

  do
    {
      if (ed->core->mark[n] == lp)
        ed->core->mark[n] = (ed_line_node_t *) mp;
    }
  while (n--);
}


/* unmark_line_node: Clear line node mark. */
void
unmark_line_node (const ed_line_node_t *lp, ed_buffer_t *ed)
{
  int i;

  for (i = 0; ed->core->marks && i < MARK_MAX; ++i)
    if (ed->core->mark[i] == lp)
      {
        ed->core->mark[i] = NULL;
        --ed->core->marks;
      }
}
