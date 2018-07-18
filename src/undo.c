/* undo.c: Undo routines for the ed line editor.
 *
 *  Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#include "ed.h"


/* 
 * append_undo_node: Append node to end of undo queue. Return node
 *   pointer.
 */
ed_undo_node_t *
append_undo_node (type, from, to, ed)
     int type;
     off_t from;
     off_t to;
     ed_buffer_t *ed;
{
  ed_undo_node_t *up;

  ed_undo_node_t *tq = ed->core->undo_head->q_back;

  spl1 ();
  if (!(up = (ed_undo_node_t *) malloc (ED_UNDO_NODE_T_SIZE)))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      spl0 ();
      return NULL;
    }
  up->type = type;
  up->h = get_line_node (from, ed);
  up->t = get_line_node (to, ed);

  /* APPEND_NODE is macro, so tq is mandatory! */
  APPEND_NODE (up, tq);
  spl0 ();
  return up;
}


/* undo_last_command: Undo last change to the editor buffer. */
int
undo_last_command (ed)
     ed_buffer_t *ed;
{
  struct ed_state saved_buf = *ed->state;
  ed_undo_node_t *up = ed->core->undo_head;
  ed_undo_node_t *next;

  if (ed->state[1].dot == -1 || ed->state[1].lines == -1)
    {
      ed->exec->err = _("Nothing to undo");
      return ERR;
    }
  spl1 ();
  get_line_node (0, ed);        /* this get_line_node last! */

  while ((up = up->q_back) != ed->core->undo_head)
    {
      switch (up->type)
        {
        case UADD:
          LINK_NODES (up->h->q_back, up->t->q_forw);
          break;
        case UDEL:
          LINK_NODES (up->h->q_back, up->h);
          LINK_NODES (up->t, up->t->q_forw);
          break;
        case UMOV:
        case URET:
          LINK_NODES (up->q_back->h, up->h->q_forw);
          LINK_NODES (up->t->q_back, up->q_back->t);
          LINK_NODES (up->h, up->t);
          up = up->q_back;
          break;
        default:
          /*NOTREACHED */
          ;
        }

      /* Toggle undo type. */
      up->type ^= 1;
    }

  /* reverse undo queue */
  do
    {
      next = up->q_forw;
      REVERSE_LINKS (up, ed_undo_node_t);
    }
  while ((up = next) != ed->core->undo_head);

  if (ed->exec->global)
    reset_global_queue (ed);
  ed->state[0] = ed->state[1], ed->state[1] = saved_buf;
  spl0 ();
  return 0;
}


/* reset_undo_queue: Clear the undo queue. */
void
reset_undo_queue (ed)
     ed_buffer_t *ed;
{
  ed_undo_node_t *up, *up_next;
  ed_line_node_t *lp, *ep, *lp_next;

  spl1 ();
  for (up = ed->core->undo_head->q_forw; up != ed->core->undo_head;
       up = up_next)
    {
      if (up->type == UDEL)
        {
          ep = up->t->q_forw;
          for (lp = up->h; lp != ep; lp = lp_next)
            {
              lp_next = lp->q_forw;
              unmark_line_node (lp, ed);
              free (lp);
            }
        }
      up_next = up->q_forw;
      UNLINK_NODE (up);
      free (up);
    }
  ed->state[1] = ed->state[0];
  spl0 ();
}
