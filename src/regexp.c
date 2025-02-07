/* regexp.c: Regular expression interface for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"

/* get_compiled_regex: Return pointer to compiled regex from command
   buffer. */
regex_t *
get_compiled_regex (unsigned dc, int re_type, ed_buffer_t *ed)
{
  static const char *compile_err = NULL;
  static regex_t *re_search = NULL; /* search regex */
  static regex_t *re_subst = NULL;  /* substitution regex */
  regex_t *re = NULL;

#ifndef HAVE_REG_SYNTAX_T
  static char re_err[BUFSIZ];   /* regex error message buffer */
#endif

  char *pattern = NULL;
  size_t len = 0;

#ifndef HAVE_REG_SYNTAX_T
  int status;
#endif

  if (isspace (dc) && dc != '\n')
    {
      ed->exec->err = _("Invalid pattern delimiter");
      return NULL;
    }

  /* Assert: spl1 () */

  /* Use previous pattern. */
  if (dc == '\n' || *++ed->input == '\n'
      || (dc == '\\' ?  *ed->input == dc &&  *(ed->input + 1) != dc
          : *ed->input == dc))
    {
      /*
       *  For a substitution command, there may be two patterns
       *  available: one from previous search and one from previous
       *  substitution. In the case of an empty pattern (e.g., ed
       *  command `s///'), the previous substitution pattern is used
       *  only if there was no previous search. In the case of a
       *  repeated substitution, (e.g., `s' => dc == '\n'), the
       *  previous search pattern is used only if explicitly requested
       *  via `r' modifier (i.e., `sr' => r_f).
       *
       *  Some cases to consider...
       *
       *  I. Sequences beginning:        Effect:
       *           s/abc/
       *      1)
       *        s                        s/abc/ - by definition.
       *      2)
       *        s//                      s/abc/ - no previous search.
       *      3)
       *        sr                       s/abc/ - no previous search. (?)
       *      4)
       *        //                       /abc/ - no previous search.
       *      5)
       *        //s                      /abc/s/abc/ - by (I.4) and (I.1).
       *      6)
       *        //s//                    /abc/s/abc/ - by (I.4) and definition
       *                                 of `s//'.
       *      7)
       *        //sr                     /abc/s/abc/ - by (I.4) and defintion
       *                                 of `sr'.
       *
       *  II. Sequences beginning:
       *            /xyz/
       *      1)
       *        s                        s/xyz/ - no previous substitution.
       *      2)
       *        s//                      s/xyz/ - by definition.
       *      3)
       *        sr                       s/xyz/ - by definition.
       *      4)
       *        //                       /xyz/ - by definition.
       *      5)
       *        //s                      /xyz/s/xyz/ - by (II.4) and (II.1).
       *      6)
       *        //s//                    /xyz/s/xyz/ - by (II.4) and definition
       *                                 of `s//'.
       *      7)
       *        //sr                     /xyz/s/xyz/ - by (II.4) and definition
       *                                 of `sr'.
       *
       *  III. Sequences beginning:
       *            s/abc/
       *            /xyz/
       *      1)
       *        s                        s/abc/ - by (I.1).
       *      2)
       *        s//                      s/xyz/ - by (II.2).
       *      3)
       *        sr                       s/xyz/ - by (II.3).
       *      4)
       *        //                       /xyz/ - by (II.4).
       *      5)
       *        //s                      /xyz/s/abc/ - by (II.4) and (I.1).
       *      6)
       *        //s//                    /xyz/s/xyz/ - by (II.4) and (II.2).
       *      7)
       *        //sr                     /xyz/s/xyz/ - by (II.4) and (II.3).
       *
       *  IV. Sequences beginning:
       *            /xyz/
       *            s/abc/
       *      1)
       *        s                        s/abc/ - by (I.1).
       *      2)
       *        s//                      s/xyz/ - by (II.2).
       *      3)
       *        sr                       s/xyz/ - by (II.3).
       *      4)
       *        //                       /xyz/ - by (II.4).
       *      5)
       *        //s                      /xyz/s/abc/ - by (II.4) and (I.1).
       *      6)
       *        //s//                    /xyz/s/xyz/ - by (II.4) and (II.2).
       *      7)
       *        //sr                     /xyz/s/xyz/ - by (II.4) and (II.3).
       *
       */

      switch (re_type)
        {
        case RE_SUBST:
          re = ((dc != '\n' && re_search) || ed->exec->subst->r_f ? re_search
                : re_subst);
          if (!re_subst)
            re_subst = re;
          break;
        case RE_SEARCH:
          re = re_search ? re_search : re_subst;
          if (!re_search)
            re_search = re;
          break;
        }
      if (!re)
        ed->exec->err = _("No previous pattern");
      return re;
    }

  if (!(pattern = regular_expression (dc, &len, ed)))
    return NULL;

  if (!(re = (regex_t *) malloc (sizeof (regex_t))))
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Memory exhausted");
      return NULL;
    }

#ifdef HAVE_REG_SYNTAX_T

  /* GNU regcomp () has no hooks for setting re_syntax_options, and
     pattern cannot contain NUL chars, so use re_compile_pattern (). */
  re->translate = NULL;
  re->fastmap = NULL;
  re->buffer = NULL;
  re->allocated = 0;

  if ((compile_err = re_compile_pattern (pattern, len, re)))
    {
      regfree (re);
      free (re);
      ed->exec->err = compile_err;
      return NULL;
    }

#else
# ifdef REG_ENHANCED

  /* Darwin regcomp () supports enhanced basic and extended regular
     expressions. */
  re->re_endp = pattern + len;
  if ((status =
      regcomp (re, pattern, (REG_PEND |
                             (ed->exec->opt & REGEX_EXTENDED
                              ? REG_EXTENDED |
                              (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)
                               ? 0 : REG_ENHANCED)
                              : 0)))))
# else
#  ifdef REG_PEND

  /* BSD regcomp () accepts pattern with NUL chars via REG_PEND, but
     has no equivalent of GNU's re_syntax_options. */
  re->re_endp = pattern + len;
  if ((status =
      regcomp (re, pattern, (REG_PEND | (ed->exec->opt & REGEX_EXTENDED
                                         ? REG_EXTENDED : 0)))))
#  else

  /* Use generic POSIX regular expression library. */
  if ((status = regcomp (re, pattern, (ed->exec->opt & REGEX_EXTENDED
                                      ? REG_EXTENDED : 0))))
#  endif /* !defined (REG_PEND) */
# endif   /* !defined (REG_ENHANCED) */
    {
      regerror (status, re, re_err, sizeof re_err);
      ed->exec->err = re_err;
      free (re);
      return NULL;
    }
#endif  /* !defined (HAVE_REG_SYNTAX_T) */

  switch (re_type)
    {
    case RE_SUBST:
      if (re_subst && re_subst != re_search)
        {
          regfree (re_subst);
          free (re_subst);
        }
      re_subst = re;
      break;
    case RE_SEARCH:
      if (re_search && re_search != re_subst)
        {
          regfree (re_search);
          free (re_search);
        }
      re_search = re;
      break;
    }
  return re;
}


/* get_matching_node_addr: Return the address of the next line
   matching a pattern in a given direction. Wrap around begin/end of
   editor buffer if necessary. */
int
get_matching_node_address (const regex_t *re, int dir, off_t *addr,
                           ed_buffer_t *ed)
{
  regmatch_t rm[1];
  ed_line_node_t *lp;
  char *s;

  *addr = ed->state->dot;
  if (!re)
    return ERR;
  do
    {
      spl1 ();
      if ((*addr = (dir ? INC_MOD (*addr, ed->state->lines)
                    : DEC_MOD (*addr, ed->state->lines))))
        {
          lp = get_line_node (*addr, ed);
          if (!(s = get_buffer_line (lp, ed)))
            {
              spl0 ();
              return ERR;
            }
#ifdef REG_STARTEND
          rm->rm_so = 0;
          rm->rm_eo = lp->len;
          if (!regexec (re, s, 0, rm, REG_STARTEND))
#else
          if (ed->state->is_binary)
            NUL_TO_NEWLINE (s, lp->len);
          if (!regexec (re, s, 0, NULL, 0))
#endif  /* !defined (REG_STARTEND) */
            {
              spl0 ();
              return 0;
            }
        }
      spl0 ();
    }
  while (*addr != ed->state->dot);
  ed->exec->err = _("No match");
  return ERR;
}
