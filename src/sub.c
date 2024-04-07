/* sub.c: Substitution routines for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2024 Andrew L. Moore, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"


/* Static function declarations. */
static int apply_subst_template (const char *, const regmatch_t *,
                                 size_t, size_t *, ed_buffer_t *);
static ssize_t count_matches (const regex_t *, const char *, int len,
                             ed_buffer_t *);
static int substitute_matching (const regex_t *, const ed_line_node_t *,
                                size_t *, off_t, off_t, unsigned,
                                ed_buffer_t *);
static int substitution_modifiers (off_t *, off_t *, unsigned *,
                                   ed_buffer_t *);
static int substitution_template (unsigned, ed_buffer_t *);


#define SGPR_CHARS "\n$+-^gpr0123456789"


/* resubstitute: Get repeated substitution modifiers
   from command buffer. Return status. */
int
resubstitute (off_t *s_nth, off_t *s_mod, unsigned *s_f,
              unsigned *sgpr_f, ed_buffer_t *ed)
{
  int status;
  int g_f = 0;                  /* Set if global toggled - e.g. `sg' */

  /* Return unless ed->input contains short-form substitution. */
  if ((ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
      || !strchr (SGPR_CHARS, (unsigned) *ed->input))
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
            /*
             *  If the last substitution is, say, `s;x;y;gp', then a
             *  subsequent `sg' is equivalent to  `s;x;y;p', and
             *  `sg' a second time expands to     `s;x;y;gp' again,
             *  i.e., `sg' always toggles the global modifier of the
             *  last substitution.
             *
             *  `sg2', on the other hand, always overrides the global
             *  modifier, if any, of the last substitution. So again,
             *  if the last substitution is `s;x;y;gp' then a
             *  subsequent `sg2' is interpreted as neither `s;x;y;2p'
             *  nor `s;x;y;p'. Instead, it is interpreted as
             *  `s;x;y;g2p'.
             */
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
substitution_lhs (regex_t **lhs_p, unsigned *sgpr_f, ed_buffer_t *ed)
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
static char *rhs;                 /* Substitution template buffer */
static size_t rhs_size;           /* Substitution template buffer size */
static size_t rhs_i;              /* Substitution template buffer index */


/* substitution_rhs: Get substitution rhs (template) from command
   buffer; return status. */
int
substitution_rhs (off_t *s_nth, off_t *s_mod, unsigned *s_f,
                  unsigned *sio_f, ed_buffer_t *ed)
{
  size_t len;
  unsigned dc;                    /* Pattern delimiting char */
  int status;

  *s_f = *sio_f = 0;              /* Reset modifiers and I/O flags */
  *s_nth = *s_mod = 0;            /* Reset match offset and modulus */
  ed->state->input_is_binary = 0; /* Value used in substitute_lines(). */


  /* Don't clobber command buffer if any ed->exec->global set. */
  if (!ed->exec->global && !(ed->input = get_extended_line (&len, 0, 0, ed)))
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
substitution_template (unsigned dc, ed_buffer_t *ed)
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
substitution_modifiers (off_t *s_nth, off_t *s_mod, unsigned *s_f,
                        ed_buffer_t *ed)
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
init_substitute (regex_t **lhs_p, unsigned *s_f, off_t *s_nth,
                 off_t *s_mod, unsigned *sio_f, struct ed_substitute *es)
{
  *lhs_p = es->lhs;
  *s_nth = es->s_nth;
  *s_mod = es->s_mod;
  *s_f = es->s_f;
  *sio_f = es->sio_f;
}


/* save_substitute: Save substitution parameters in ed state buffer. */
void
save_substitute (regex_t *lhs, unsigned s_f, off_t s_nth,
                 off_t s_mod, unsigned sio_f, struct ed_substitute *es)
{
  es->lhs = lhs;
  es->s_nth = s_nth;
  es->s_mod = s_mod;
  es->s_f = s_f;
  es->sio_f = sio_f;
}


/* Global declarations */
static char *rb;                       /* Substitution text buffer */
static size_t rb_size;                 /* Substitution text buffer size */

/* substitute_lines: For each line in a range, replace text matching a
   pattern per substitution template; return status. */
int
substitute_lines (off_t from, off_t to, regex_t *re, off_t s_nth,
                  off_t s_mod, unsigned s_f, ed_buffer_t *ed)
{
  ed_line_node_t *lp;
  ed_line_node_t *mp;
  ed_undo_node_t *up;
  char *s;
  char *txt;
  char *eot;
  off_t lc;
  off_t dot = ed->state->dot;
  size_t len = 0;
  int nsubs = 0;
  int status = 0;
  int nl = ed->state->newline_appended;

  ed->state->dot = from - 1;
  for (lc = 0; lc <= to - from; ++lc)
    {
      /* Can't use lp->q_forw because replacement may be multiple lines. */
      lp = get_line_node (++ed->state->dot, ed);
      if ((status =
           substitute_matching (re, lp, &len, s_nth, s_mod, s_f, ed)) < 0)
        return status;
      if (len)
        {
          spl1 ();
          up = NULL;
          if (delete_lines (ed->state->dot, ed->state->dot, ed) < 0)
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
              mp = get_line_node (ed->state->dot, ed);
              transfer_marks (mp, lp, ed);
              APPEND_UNDO_NODE (mp, up, ed->state->dot, ed);
            }
          while (txt != eot);
          ++nsubs;
          dot = ed->state->dot;
          ed->state->is_binary |= ed->state->input_is_binary;
          spl0 ();
        }
    }
  ed->state->dot = dot;

  /* Preserve flag newline_appended, inrespective of substitution, to
     prevent gratuitous newline in binary output. */
  ed->state->newline_appended = nl;
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
substitute_matching (const regex_t *re, const ed_line_node_t *lp,
                     size_t *len, off_t s_nth, off_t s_mod,
                     unsigned s_f, ed_buffer_t *ed)
{
  static regmatch_t *rm = NULL;
  static size_t rm_size = 0;

  char *txt;
  char *eot;
  ssize_t n = 0;
  size_t i, j, k = 0;
  size_t changed = 0;
  int matched_prev = 0;
  int nil_prev;
  int nil_next = 1;
  int status;
#ifdef REG_STARTEND
  int eflag = REG_STARTEND;
#else
  int eflag = 0;
#endif

  if (!(txt = get_buffer_line (lp, ed)))
    return ERR;
  else if (re->re_nsub >= SE_MAX)
    {
      ed->exec->err = _("Too many regex subexpressions");
      return ERR;
    }
  else
    REALLOC_THROW (rm, rm_size,
                   (re->re_nsub + 1) * sizeof(regmatch_t), ERR, ed);

  /* If match-relative and requested match (s_nth) > available (n),
     then nothing to do. */
  if ((n = count_matches (re, txt, lp->len, ed)) < 0)
    return ERR;
  else if ((s_f & SNLR || s_f & SMLR) && n <= labs(s_nth))
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

#ifndef REG_STARTEND
  if (ed->state->is_binary)
    NUL_TO_NEWLINE (txt, lp->len);
#endif
  for (*len = 0, eot = txt + lp->len, rm->rm_so = 0, rm->rm_eo = lp->len;
       (!changed || (s_f & GSUB))
         && !regexec (re, txt, re->re_nsub + 1, rm, eflag);
       txt += j, rm->rm_so = 0, rm->rm_eo = eot - txt, eflag |= REG_NOTBOL)
    {
      i = rm->rm_so, j = rm->rm_eo;
      nil_prev = nil_next, nil_next = !j;

      /* Infinite loop!! */
      if (matched_prev && nil_next)
        {
          if (!*txt)
            break;
          if (!ed->state->is_utf8 || (j = utf8_char_size (txt, eot - txt)) == 0)
            j = 1;
          REALLOC_THROW (rb, rb_size, *len + j, ERR, ed);
#ifndef REG_STARTEND
          if (ed->state->is_binary)
            NEWLINE_TO_NUL (txt, j);
#endif
          memcpy (rb + *len, txt, j);
          *len += j;
          matched_prev = 0;
        }

      /* NIL (i.e., empty) match counts towards s_nth iff previous
         match was NIL. */
      else if ((!nil_next || nil_prev) && ++k >= s_nth
               && !((k - s_nth)  % s_mod))
        {
          REALLOC_THROW (rb, rb_size, *len + i, ERR, ed);
#ifndef REG_STARTEND
          if (ed->state->is_binary)
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
          if (!j && (!ed->state->is_utf8
                     || (j = utf8_char_size (txt, eot - txt)) == 0))
            j = 1;
          REALLOC_THROW (rb, rb_size, *len + j, ERR, ed);
#ifndef REG_STARTEND
          if (ed->state->is_binary)
            NEWLINE_TO_NUL (txt, j);
#endif
          memcpy (rb + *len, txt, j);
          *len += j;
          matched_prev = 0;
        }
    }
  if (!changed)
    return *len = 0;
  i = eot > txt ? eot - txt : 0;
  REALLOC_THROW (rb, rb_size, *len + i + 2, ERR, ed);
#ifndef REG_STARTEND
  if (ed->state->is_binary)
    NEWLINE_TO_NUL (txt, i);
#endif
  memcpy (rb + *len, txt, i);

  /* put_buffer_line() requires newline. For binary data, setting flag
     ed->state->newline_appended suppresses its output.
     (See also read_stream() comments.) */
  strcpy (rb + *len + i, "\n");
  *len += i + 1;                /* `\n' => +1. */
  return 0;
}


/* count_matches: Return count of matches in given line. */
static ssize_t
count_matches (const regex_t *re, const char *txt, int len,
               ed_buffer_t *ed)
{
  static regmatch_t *rm = NULL;
  static size_t rm_size = 0;

  const char *eot;
  size_t off = 0;
  size_t nmatches = 0;

  int matched_prev = 0;
  int nul_prev;
  int nul_next = 1;
#ifdef REG_STARTEND
  int eflag = REG_STARTEND;
#else
  int eflag = 0;
#endif

  if (re->re_nsub >= SE_MAX)
    {
      ed->exec->err = _("Too many regex subexpressions");
      return ERR;
    }
  else
    REALLOC_THROW (rm, rm_size,
                   (re->re_nsub + 1) * sizeof(regmatch_t), ERR, ed);

#ifndef REG_STARTEND
  if (ed->state->is_binary)
    NUL_TO_NEWLINE (txt, len);
#endif
  for (eot = txt + len, rm->rm_so = 0, rm->rm_eo = len;
       !regexec (re, txt, re->re_nsub + 1, rm, eflag);
       txt += off, rm->rm_so = 0, rm->rm_eo = eot - txt, eflag |= REG_NOTBOL)
    {
      off = rm->rm_eo;
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
          if (!off)
            ++off;
          matched_prev = 0;
        }
    }
  return nmatches;
}


/* apply_subst_template: Modify text according to a
   substitution template; return offset to end of modified text. */
static int
apply_subst_template (const char *boln, const regmatch_t *rm,
                      size_t re_nsub, size_t *len, ed_buffer_t *ed)
{
  static int *cc = NULL;        /* Case-change stack. */
  static size_t cc_size = 0;

  char *sub = rhs;
  size_t cc_top = 0;
  size_t j = 0;
  size_t k = 0;
  int n;
  int change_once = 0;

  if (cc_size < rhs_i / 2 + 1)
    REALLOC_THROW (cc, cc_size, rhs_i /  2 + 1, ERR, ed);
  cc[cc_top++] = 0;

  for (; sub < rhs + rhs_i; ++sub)
    if (*sub == '&')
      {
        j = rm->rm_so;
        k = rm->rm_eo;
        REALLOC_THROW (rb, rb_size, *len + k - j, ERR, ed);
        while (j < k)
          {
            *(rb + (*len)++) = ((cc[cc_top - 1] > 0 || change_once > 0)
                                ?  toupper (*(boln + j++))
                                : (cc[cc_top - 1] < 0 || change_once < 0)
                                ? tolower (*(boln + j++))
                                : *(boln + j++));
            change_once = 0;
          }
      }
    else if (*sub == '\\')
      switch ((unsigned char ) *++sub)
        {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          if ((n = *sub - '0') <= re_nsub)
            {
              j = rm[n].rm_so;
              k = rm[n].rm_eo;
              REALLOC_THROW (rb, rb_size, *len + k - j, ERR, ed);
              while (j < k)
                {
                  *(rb + (*len)++) = ((cc[cc_top - 1] > 0 || change_once > 0)
                                      ?  toupper (*(boln + j++))
                                      : (cc[cc_top - 1] < 0 || change_once < 0)
                                      ? tolower (*(boln + j++))
                                      : *(boln + j++));
                  change_once = 0;
                }
            }
          break;
        case 'U':
          cc[cc_top++] = 1;
          break;
        case 'u':
          change_once = 1;
          break;
        case 'L':
          cc[cc_top++] = -1;
          break;
        case 'l':
          change_once = -1;
          break;
        case 'E':
          if (cc_top > 1)
            --cc_top;
          break;
        default:
          REALLOC_THROW (rb, rb_size, *len + 1, ERR, ed);
          *(rb + (*len)++) = ((cc[cc_top - 1] > 0 || change_once > 0)
                              ?  toupper (*sub)
                              : (cc[cc_top - 1] < 0 || change_once < 0)
                              ? tolower (*sub)
                              : *sub);
          change_once = 0;
          break;
        }
    else
      {
        REALLOC_THROW (rb, rb_size, *len + 1, ERR, ed);
        *(rb + (*len)++) = ((cc[cc_top - 1] > 0 || change_once > 0)
                            ?  toupper (*sub)
                            : (cc[cc_top - 1] < 0 || change_once < 0)
                            ? tolower (*sub)
                            : *sub);
        change_once = 0;
      }
  REALLOC_THROW (rb, rb_size, *len + 1, ERR, ed);
  *(rb + *len) = '\0';
  return 0;
}
