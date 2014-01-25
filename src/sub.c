/* sub.c: Substitution routines for the ed line editor.

   Copyright © 1993-2014 Andrew L. Moore, SlewSys Research

   Last modified: 2014-01-25 <alm@slewsys.org>

   This file is part of ed. */

#include "ed.h"


/* Static function declarations. */
static int apply_subst_template __P ((const char *, const regmatch_t *,
                                      int, size_t *, ed_buffer_t *));
static size_t count_matches __P ((const regex_t *, const char *, int len,
                                  struct ed_state *));
static int substitute_matching __P ((const regex_t *, const ed_line_node_t *,
                                     size_t *, off_t, off_t, unsigned,
                                     ed_buffer_t *));
static int substitution_modifiers __P ((off_t *, off_t *, unsigned *,
                                        ed_buffer_t *));
static int substitution_template __P ((unsigned, ed_buffer_t *));


#define SGPR_CHARS "\n$+-^gpr0123456789"


/* resubstitute: Get repeated substitution modifiers
   from command buffer. Return status. */
int
resubstitute (s_nth, s_mod, s_f, sgpr_f, ed)
     off_t *s_nth;
     off_t *s_mod;
     unsigned *s_f;
     unsigned *sgpr_f;
     ed_buffer_t *ed;
{
  int status;
  int g_f = 0;                  /* Set if global toggled - e.g. `sg' */

  /* Return unless ed->input contains short-form substitution. */
  if (!strchr (SGPR_CHARS, (unsigned) *ed->input))
    return 0;

  *sgpr_f = SGPR;               /* Flag short-form substitution. */

  for (;;)
    switch ((unsigned char) *ed->input)
      {
      case '\n':
        return 0;
      case '$':
        ++ed->input;
        if (g_f)
          {
            *s_f |= SMLR;       /* Select frequency: (match_count - s_mod). */
            if ((status = address_offset (s_mod, ed)) < 0)
              return status;
          }
        else
          {
            *s_f |= SNLR;       /* Select (match_count - s_nth) match. */
            if ((status = address_offset (s_nth, ed)) < 0)
              return status;
          }
        break;
      case '+':
      case '-':
      case '^':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if (g_f)
          {
            /* If the last substitution is, say, `s;x;y;gp', then a
               subsequent `sg' is equivalent to  `s;x;y;p', and
               `sg' a second time expands to     `s;x;y;gp' again, 
               i.e., `sg' always toggles the global modifier of the
               last substitution.

               `sg2', on the other hand, always overrides the global
               modifier, if any, of the last substitution. So again,
               if the last substitution is `s;x;y;gp' then a
               subsequent `sg2' is interpreted as neither `s;x;y;2p'
               nor `s;x;y;p'. Instead, it is interpreted as
               `s;x;y;g2p'. */
            *s_f &= ~(GSUB | SMLR);
            *s_mod = 0;
            if ((status = address_offset (s_mod, ed)) < 0)
              return status;

            if (*s_mod <= 0)
              *s_f |= SMLR;     /* Select frequency: (match_count - s_mod) */
            else
              *s_f &= ~SMLR;    /* Select frequency: s_mod. */
          }
        else 
          {
            *s_nth = 0;
            if ((status = address_offset (s_nth, ed)) < 0)
              return status;
            if (*s_nth <= 0)
              *s_f |= SNLR;     /* Select (match_count - s_nth) match. */
            else
              *s_f &= ~SNLR;    /* Select s_nth match. */
          }  
        break;
      case 'g':
        g_f = 1;
        *sgpr_f ^= TGSG;
        ++ed->input;
        break;
      case 'p':
        *sgpr_f ^= TGSP;
        ++ed->input;
        break;
      case 'r':
        *sgpr_f ^= TGSR;
        ++ed->input;
        break;
      default:
        if (*sgpr_f)
          {
            ed->exec->err = _("Unknown command suffix");
            return ERR;
          }
        return 0;
      }
  /* NOTREACHED */
}


/* substitution_lhs: Get substitution lhs from command buffer; return
   status. */
int
substitution_lhs (lhs_p, sgpr_f, ed)
     regex_t **lhs_p;
     unsigned *sgpr_f;
     ed_buffer_t *ed;
{
  regex_t *re;

  /* Short form of substitution with unknown modifier, e.g., `sw'. */
  if (*ed->input != '\n' && *(ed->input + 1) == '\n')
    {
      ed->exec->err = _("Unknown command suffix");
      return ERR;
    }

  re = *lhs_p;
  spl1 ();

  /* If r_f, use regex from last search. */
  if (((ed->exec->subst->r_f = (*sgpr_f & TGSR)) || !*sgpr_f)
      && !(re = get_compiled_regex (*ed->input, RE_SUBST, ed)))
    {
      spl0 ();
      return ERR;
    }
  *lhs_p = re;
  spl0 ();

  if (*sgpr_f && !*lhs_p)
    {
      ed->exec->err = _("No previous substitution");
      return ERR;
    }
  return 0;
}


/* Global declarations */
char *rhs;                      /* Substitution template buffer */
size_t rhs_size;                /* Substitution template buffer size */
size_t rhs_i;                   /* Substitution template buffer index */


/* substitution_rhs: Get substitution rhs (template) from command
   buffer; return status. */
int
substitution_rhs (s_nth, s_mod, s_f, sio_f, ed)
     off_t *s_nth;
     off_t *s_mod;
     unsigned *s_f;
     unsigned *sio_f;
     ed_buffer_t *ed;
{
  size_t len;
  unsigned dc;                    /* Pattern delimiting char */
  int status;

  *s_f = *sio_f = 0;              /* Reset modifiers and I/O flags */
  *s_nth = *s_mod = 0;            /* Reset match offset and modulus */
  ed->state[0].input_is_binary = 0; /* Value used in substitute_lines(). */


  /* Don't clobber command buffer if any ed->exec->global set. */
  if (!ed->exec->global && !(ed->input = get_extended_line (&len, 0, ed)))
    {
      /* EOF here always flags error. */
      status = ERR;
      clearerr (stdin);
      return status;
    }

  /* Newline-delimited pattern. */
  if ((dc = *ed->input) == '\n')
    {
      rhs_i = 0;
      *sio_f = PRNT;
      return 0;
    }
  if ((status = substitution_template (dc, ed)) < 0)
    return status;

  /* Newline-delimited pattern. */
  if (*ed->input == '\n')
    {
      *sio_f = PRNT;
      return 0;
    }
  if (*ed->input == dc)
    ++ed->input;
  return substitution_modifiers (s_nth, s_mod, s_f, ed);
}


/* substitution_template: Copy substitution template from command
   buffer; return status. */
static int
substitution_template (dc, ed)
     unsigned dc;               /* Pattern delimiting char */
     ed_buffer_t *ed;
{
  if (*++ed->input == '%'
      && (*(ed->input + 1) == dc || *(ed->input + 1) == '\n'))
    {
      ++ed->input;
      if (!rhs)
        {
          ed->exec->err = _("No previous substitution");
          return ERR;
        }
      return 0;
    }

  if (!ed->exec->global)
    {
      rhs = NULL;
      rhs_size = 0;
    }

  for (rhs_i = 0; *ed->input != dc; ++rhs_i, ++ed->input)
    {
      REALLOC_THROW (rhs, rhs_size, rhs_i + 2, ERR, ed);

      /* Only process escape sequences of the form `\dc' or `\\' here. */
      if ((*(rhs + rhs_i) = *ed->input) == '\n' &&  *(ed->input + 1) == '\0')
        break;
      if (*ed->input != '\\')
        ;
      else if (*(ed->input + 1) == dc)
        *(rhs + rhs_i) = *++ed->input;
      else if (*(ed->input + 1) == '\\')
        *(rhs + ++rhs_i) = *++ed->input;
    }
  REALLOC_THROW (rhs, rhs_size, rhs_i + 1, ERR, ed);
  *(rhs + rhs_i) = '\0';
  return 0;
}


/* substitution_modifiers: Get substitution modifiers from command buffer. */
static int
substitution_modifiers (s_nth, s_mod, s_f, ed)
     off_t *s_nth;
     off_t *s_mod;
     unsigned *s_f;
     ed_buffer_t *ed;
{
  int status;

  for (;;)
    switch ((unsigned char) *ed->input)
      {
      case '$':
        ++ed->input;
        if (*s_f & GSUB)
          {
            *s_f |= SMLR;       /* Select frequency: (match_count - s_mod). */
            if ((status = address_offset (s_mod, ed)) < 0)
              return status;
          }
        else
          {
            *s_f |= SNLR;       /* Select (match_count - s_nth) match. */
            if ((status = address_offset (s_nth, ed)) < 0)
              return status;
          }
        break;
      case '+':
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if (*s_f & GSUB)
          {
            if ((status = address_offset (s_mod, ed)) < 0)
              return status;
            if (*s_mod <= 0)
              *s_f |= SMLR;     /* Select frequency: (last match) - s_mod. */
            else
              *s_f &= ~SMLR;    /* Select frequency: s_mod. */
          }
        else
          {
            if ((status = address_offset (s_nth, ed)) < 0)
              return status;
            *s_f &= ~SMLR;      /* Override any previous substitution. */
            if (*s_nth <= 0)
              *s_f |= SNLR;     /* Select (match_count - s_nth) match. */
            else
              *s_f &= ~SNLR;    /* Select s_nth match. */
          }
        break;
      case 'g':
        *s_f |= GSUB;
        ++ed->input;
        break;
      default:
        return 0;
      }
  /* NOTREACHED */
}


/* init_substitute: Initialize substitution parameters from ed state
   buffer. */
void
init_substitute (lhs, s_f, s_nth, s_mod, sio_f, es)
     regex_t **lhs;
     unsigned *s_f;
     off_t *s_nth;
     off_t *s_mod;
     unsigned *sio_f;
     struct ed_substitute *es;
{
  *lhs = es->lhs;
  rhs = es->rhs;
  rhs_size = es->rhs_size;
  rhs_i = es->rhs_i;
  *s_nth = es->s_nth;
  *s_mod = es->s_mod;
  *s_f = es->s_f;
  *sio_f = es->sio_f;
}


/* save_substitute: Save substitution parameters in ed state buffer. */
void
save_substitute (lhs, s_f, s_nth, s_mod, sio_f, es)
     regex_t *lhs;
     unsigned s_f;
     off_t s_nth;
     off_t s_mod;
     unsigned sio_f;
     struct ed_substitute *es;
{
  es->lhs = lhs;
  es->rhs = rhs;
  es->rhs_size = rhs_size;
  es->rhs_i = rhs_i;
  es->s_nth = s_nth;
  es->s_mod = s_mod;
  es->s_f = s_f;
  es->sio_f = sio_f;
}


/* Global declarations */
char *rb;                       /* substitute_matching_text buffer */
size_t rb_size;                 /* substitute_matching_text buffer size */

/* substitute_lines: For each line in a range, replace text matching a
   pattern per substitution template; return status. */
int
substitute_lines (from, to, re, s_nth, s_mod, s_f, ed)
     off_t from;
     off_t to;
     regex_t *re;
     off_t s_nth;
     off_t s_mod;
     unsigned s_f;
     ed_buffer_t *ed;
{
  ed_line_node_t *lp;
  ed_undo_node_t *up;
  char *s;
  char *txt;
  char *eot;
  off_t lc;
  off_t dot = ed->state[0].dot;
  size_t len = 0;
  int nsubs = 0;
  int status = 0;
  int nl = ed->state[0].newline_appended;

  ed->state[0].dot = from - 1;
  for (lc = 0; lc <= to - from; ++lc)
    {
      /* Can't use lp->q_forw because replacement may be multiple lines. */
      lp = get_line_node (++ed->state[0].dot, ed);
      if ((status =
           substitute_matching (re, lp, &len, s_nth, s_mod, s_f, ed)) < 0)
        return status;
      if (len)
        {
          spl1 ();
          up = NULL;
          if (delete_lines (ed->state[0].dot, ed->state[0].dot, ed) < 0)
            {
              spl0 ();
              return ERR;
            }
          txt = rb;
          eot = rb + len;
          do
            {
              for (s = txt; *s++ != '\n';)
                ;
              if (!(txt = put_buffer_line (txt, s - txt, ed)))
                {
                  spl0 ();
                  return ERR;
                }
              lp = get_line_node (ed->state[0].dot, ed);
              APPEND_UNDO_NODE (lp, up, ed->state[0].dot, ed);
            }
          while (txt != eot);
          ++nsubs;
          dot = ed->state[0].dot;
          ed->state[0].is_binary |= ed->state[0].input_is_binary;
          spl0 ();
        }
    }
  ed->state[0].dot = dot;

  /* Preserve flag newline_appended, inrespective of substitution, to
     prevent gratuitous newline in binary output. */
  ed->state[0].newline_appended = nl;
  if (nsubs == 0 && !ed->exec->global)
    {
      ed->exec->err = _("No match");
      return ERR;
    }
  return 0;
}


/* substitute_matching: Replace text matched by a pattern according to
   a substitution template; return length of modified text. */
static int
substitute_matching (re, lp, len, s_nth, s_mod,  s_f, ed)
     const regex_t *re;
     const ed_line_node_t *lp;
     size_t *len;
     off_t s_nth;
     off_t s_mod;
     unsigned s_f;
     ed_buffer_t *ed;
{
  regmatch_t rm[SE_MAX];
  char *txt;
  char *eot;
  size_t i, j, k = 0;
  size_t n = 0;
  size_t changed = 0;
  int matched_prev = 0;
  int nil_prev;
  int nil_next = 1;
  int status;

  if (!(txt = get_buffer_line (lp, ed)))
    return ERR;

  /* If match-relative and requested match (s_nth) > available (n),
     then nothing to do. */
  if ((s_f & SNLR || s_f & SMLR)
      && (n = count_matches (re, txt, lp->len, ed->state)) <= labs(s_nth))
    return *len = 0;

  /* Last match-relative s_nth?  */
  if (s_f & SNLR)
    s_nth += n;
  else if (!s_nth)
    s_nth = 1;

  /* Last match-relative s_mod? */
  if (s_f & SMLR)
    s_mod += n;
  else if (!s_mod)
    s_mod = 1;

#ifdef REG_STARTEND
  for (*len = 0, eot = txt + lp->len, rm[0].rm_so = 0, rm[0].rm_eo = lp->len;
       (!changed || (s_f & GSUB))
         && !regexec (re, txt, SE_MAX, rm, REG_STARTEND);
       txt += j, rm[0].rm_so = 0, rm[0].rm_eo = eot - txt)
#else
  if (ed->state[0].is_binary)
    NUL_TO_NEWLINE (txt, lp->len);
  for (*len = 0, eot = txt + lp->len;
       (!changed || (s_f & GSUB)) && !regexec (re, txt, SE_MAX, rm, 0);
       txt += j)
#endif  /* !REG_STARTEND */
    {
      i = rm[0].rm_so, j = rm[0].rm_eo;
      nil_prev = nil_next, nil_next = !j;

      /* Infinite loop!! */
      if (matched_prev && nil_next)
        {
          if (!*txt)
            break;
          j = 1;
          REALLOC_THROW (rb, rb_size, *len + j, ERR, ed);
#ifndef REG_STARTEND
          if (ed->state[0].is_binary)
            NEWLINE_TO_NUL (txt, j);
#endif
          memcpy (rb + (*len)++, txt, j);
          matched_prev = 0;
        }

      /* NIL (i.e., empty) match counts towards s_nth iff previous
         match was NIL. */
      else if ((!nil_next || nil_prev) && ++k >= s_nth 
               && !((k - s_nth)  % s_mod))
        {
          REALLOC_THROW (rb, rb_size, *len + i, ERR, ed);
#ifndef REG_STARTEND
          if (ed->state[0].is_binary)
            NEWLINE_TO_NUL (txt, j);
#endif
          memcpy (rb + *len, txt, i);
          *len += i;
          if ((status =
               apply_subst_template (txt, rm, re->re_nsub, len, ed)) < 0)
            return status;
          matched_prev = 1;
          ++changed;
        }

      /* Not yet ... */
      else
        {
          if (nil_next && !*txt)
            break;
          j = max (1, j);
          REALLOC_THROW (rb, rb_size, *len + j, ERR, ed);
#ifndef REG_STARTEND
          if (ed->state[0].is_binary)
            NEWLINE_TO_NUL (txt, j);
#endif
          memcpy (rb + *len, txt, j);
          *len += j;
          matched_prev = 0;
        }
    }
  if (!changed)
    return *len = 0;
  i = max (0, eot - txt);
  REALLOC_THROW (rb, rb_size, *len + i + 2, ERR, ed);
#ifndef REG_STARTEND
  if (ed->state[0].is_binary)
    NEWLINE_TO_NUL (txt, i);
#endif
  memcpy (rb + *len, txt, i);

  /* put_buffer_line() requires newline. For binary data, setting flag
     ed->state[0].newline_appended suppresses its output.
     (See also read_stream() comments.) */
  strcpy (rb + *len + i, "\n");
  *len += i + 1;                /* `\n' => +1. */
  return 0;
}


/* count_matches: Return count of matches in given line. */
static size_t
count_matches (re, txt, len, buf)
     const regex_t *re;
     const char *txt;
     int len;
     struct ed_state *buf;
{
  regmatch_t rm[SE_MAX];
  const char *eot;
  size_t off = 0;
  size_t nmatches = 0;
  int matched_prev = 0;
  int nul_prev;
  int nul_next = 1;

#ifdef REG_STARTEND
  for (eot = txt + len, rm[0].rm_so = 0, rm[0].rm_eo = len;
       !regexec (re, txt, SE_MAX, rm, REG_STARTEND);
       txt += off, rm[0].rm_so = 0, rm[0].rm_eo = eot - txt)
#else
  if (buf->is_binary)
    NUL_TO_NEWLINE (txt, len);
  for (eot = txt + len;
       !regexec (re, txt, SE_MAX, rm, 0);
       txt += off)
#endif  /* !REG_STARTEND */
    {
      off = rm[0].rm_eo;
      nul_prev = nul_next;
      nul_next = !off;

      /* Infinite loop!! */
      if (matched_prev && nul_next)
        {
          if (!*txt)
            break;
          off = 1;
          matched_prev = 0;
        }

      /* NIL (i.e., empty) match counts towards s_nth iff previous
         match was NIL. */
      else if ((!nul_next || nul_prev))
        {
          matched_prev = 1;
          ++nmatches;
        }

      /* Not yet ... */
      else
        {
          if (nul_next && !*txt)
            break;
          off = max (1, off);
          matched_prev = 0;
        }
    }
  return nmatches;
}


/* apply_subst_template: Modify text according to a
   substitution template; return offset to end of modified text. */
static int
apply_subst_template (boln, rm, re_nsub, len, ed)
     const char *boln;
     const regmatch_t *rm;
     int re_nsub;
     size_t *len;
     ed_buffer_t *ed;
{
  char *sub = rhs;
  size_t j = 0;
  size_t k = 0;
  int n;

  for (; sub < rhs + rhs_i; ++sub)
    if (*sub == '&')
      {
        j = rm[0].rm_so;
        k = rm[0].rm_eo;
        REALLOC_THROW (rb, rb_size, *len + k - j, ERR, ed);
        while (j < k)
          *(rb + (*len)++) = *(boln + j++);
      }
    else if (*sub == '\\' && isdigit ((unsigned char) *++sub)
             && (n = *sub - '0') <= re_nsub)
      {
        j = rm[n].rm_so;
        k = rm[n].rm_eo;
        REALLOC_THROW (rb, rb_size, *len + k - j, ERR, ed);
        while (j < k)
          *(rb + (*len)++) = *(boln + j++);
      }
    else
      {
        REALLOC_THROW (rb, rb_size, *len + 1, ERR, ed);
        *(rb + (*len)++) = *sub;
      }
  REALLOC_THROW (rb, rb_size, *len + 1, ERR, ed);
  *(rb + *len) = '\0';
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
