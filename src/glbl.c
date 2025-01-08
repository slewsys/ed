/* glbl.c: Global command implentation for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2024 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"


/* Static function declarations. */
static int append_global_line (const ed_line_node_t *, ed_buffer_t *);
static ed_line_node_t *next_global_line (ed_buffer_t *);


/*
 * set_global_lines: Add lines matching a pattern to global comand
 *   buffer.
 */
int
set_global_lines (int want_match, ed_buffer_t *ed)
{
  regmatch_t rm[1];
  regex_t *re;
  ed_line_node_t *lp;
  off_t from = ed->exec->region->start;
  off_t to = ed->exec->region->end;
  off_t n = from ? to - from + 1 : 0;
  int status = 0;
  char dc = *ed->input;              /* pattern delimiting char */
  char *s;

  spl1 ();
  if (!(re = get_compiled_regex (dc, RE_SEARCH, ed)))
    {
      spl0 ();
      return ERR;
    }
  if ((dc == '\\' ? *ed->input == dc && *(ed->input + 1) != dc
       : *ed->input == dc) && *ed->input != '\n')
    ++ed->input;
  reset_global_buffer (ed);
  spl0 ();
  lp = get_line_node (from, ed);
  for (; n; --n, lp = lp->q_forw)
    {
      if (!(s = get_buffer_line (lp, ed)))
        return ERR;
#ifdef REG_STARTEND
      rm->rm_so = 0;
      rm->rm_eo = lp->len;
      if ((!regexec (re, s, 0, rm, REG_STARTEND)) == want_match
          && (status = append_global_line (lp, ed)) < 0)
#else
      if (ed->state->is_binary)
        NUL_TO_NEWLINE (s, lp->len);
      if (!regexec (re, s, 0, NULL, 0) == want_match
          && (status = append_global_line (lp, ed)) < 0)
#endif  /* !REG_STARTEND */
        return status;
    }
  return 0;
}


/*
 * exec_global: Apply command list to lines in global command buffer.
 *   Return command status.
 */
int
exec_global (ed_buffer_t *ed)
{
  static char *gcb = NULL;      /* global command buffer */
  static size_t gcb_size = 0;   /* buffer size */

  ed_line_node_t *lp = NULL;
  size_t len = 0;
  int first_time = 1;
  int interactive = ed->exec->global & GLBI;
  int saved_io_f = ed->display->io_f;
  int status;

  reset_undo_queue (ed);

  /*
   * If non-interactive, read command before entering loop in the
   * event of empty global queue.
   */
  if (!interactive && !(ed->input = get_extended_line (&len, 0, 1, ed)))
    {
      status = ERR;
      clearerr (stdin);
      return status;
    }

  /* Empty command list equivalent to `p' command per SUSv4. */
  if (strlen (ed->input) == 1 && *ed->input == '\n')
    ed->input = "p\n";

  for (ed->exec->first_pass = 1; (lp = next_global_line (ed));
       ed->input = gcb, ed->exec->first_pass = 0)
    {
      if ((status = get_line_node_address (lp, &ed->state->dot, ed)) < 0)
        return status;

      else if (interactive)
        {
          ed->display->io_f = PRNT;
          if ((status = display_lines (ed->state->dot,
                                       ed->state->dot, ed)) < 0)
            return status;
          else if (!(ed->input = get_stdin_line (&len, ed))
                   || !(ed->input = get_extended_line (&len, 0, 1, ed)))
            {
              /* For an interactive global command, permit EOF to cancel. */
              status = feof (stdin) ? 0 : ERR;
              clearerr (stdin);
              return status;
            }
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
              ed->exec->err = _("No previous command");
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
      for (ed->input = gcb; ed->input && *ed->input;)
        if ((status = address_range (ed)) < 0
            || (status = exec_command (ed)) < 0
            || ((ed->display->io_f = status) > 0
                && (status = display_lines (ed->state->dot,
                                            ed->state->dot, ed)) < 0))
          return status;
    }
  return saved_io_f;
}

/*
 * append_global_line: Append line node to end of global command buffer.
 */
static int
append_global_line (const ed_line_node_t *lp, ed_buffer_t *ed)
{
  ed_global_buffer_t *gb = ed->core->global_buffer;

  REALLOC_THROW (gb->lbuf, gb->size,
                 (gb->last + 1) * sizeof (ed_line_node_t *),
                 ERR, ed);
  gb->lbuf[gb->last++] = (ed_line_node_t *) lp;
  return 0;
}


/*
 * clear_global_lines: Clear a range of lines from global command
 *   buffer
 */
void
clear_global_lines (ed_line_node_t *np, ed_line_node_t *mp,
                    off_t count, ed_buffer_t *ed)
{
  ed_global_buffer_t *gb = ed->core->global_buffer;
  ed_line_node_t *lp;
  size_t i;

  for (lp = np; lp != mp; lp = lp->q_forw)
    for (i = 0; i < gb->last; i++)
      if (gb->lbuf[gb->index] == lp)
        {
          gb->lbuf[gb->index] = NULL;
          gb->index = INC_MOD (gb->index, gb->last - 1);
          break;
        }
      else
        gb->index = INC_MOD (gb->index, gb->last - 1);
}


 /*
  * next_global_line: Return the next line node in the global command
  *   buffer.
  */
ed_line_node_t *
next_global_line (ed_buffer_t *ed)
{
  ed_global_buffer_t *gb = ed->core->global_buffer;

  while (gb->ptr < gb->last && gb->lbuf[gb->ptr] == NULL)
    gb->ptr++;
  return (gb->ptr < gb->last) ? gb->lbuf[gb->ptr++] : NULL;
}


/* reset_global_buffer: Reset the global command buffer. */
void
reset_global_buffer (ed_buffer_t *ed)
{
  ed_global_buffer_t *gb = ed->core->global_buffer;

  spl1 ();
  if (gb->lbuf != NULL)
    free (gb->lbuf);
  gb->lbuf = NULL;
  gb->size = 0;
  gb->last = 0;
  gb->ptr = 0;
  gb->index = 0;
  spl0 ();
}
