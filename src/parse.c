/* parse.c: Editor command-parsing routines for the ed line editor.
 *
 *  Copyright © 1993-2016 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#include "ed.h"
#include <sys/stat.h>

/* Static function declarations. */
static char *character_class __P ((const char *, ed_buffer_t *));
static int check_address_bounds __P ((off_t, ed_buffer_t *));
static glob_t *expand_glob __P ((char *, int, glob_t *, ed_buffer_t *));
static char *is_valid_name __P ((const char *, ed_buffer_t *));
static int line_address __P ((off_t *, ed_buffer_t *));
static char *strtok_with_delimiters __P ((char *, const char *));


#define IS_DELIMITER(c) ((c) == '%' || (c) == ',' || (c) == ';')


/* address_range: Get line addresses from the command buffer until an
   illegal address is seen; return address count. */
int
address_range (ed)
     ed_buffer_t *ed;
{
  /* Per SUSv4, 2013, intermediate addresses may exceed
     ed->state->lines or be negative, so use signed types. */
  off_t addr;
  off_t first, second, dot;
  int have_dc;                  /* If set, have address delimiter char. */
  int status;

  ed->exec->region->addrs = 0;
  first = second = dot = ed->state->dot;
  SKIP_WHITESPACE (ed);
  have_dc = IS_DELIMITER (*ed->input);
  do
    {
      if (have_dc)
        {
          if (ed->exec->region->addrs)
            first = *ed->input == ';' ? (dot = second) : second;
          else
            {
              first = *ed->input == ';' ? dot : 1;
              second = ed->state->lines;
              ed->exec->region->addrs = 2;
            }
        }
      ed->input += have_dc;
      addr = dot;
      if ((status = next_address (&addr, ed)) < 0)
        return status;
      if (status > 0)
        {
          second = addr;
          ++ed->exec->region->addrs;
        }
    }
  while ((have_dc = IS_DELIMITER (*ed->input)));

  if (ed->exec->region->addrs < 2)
    first = second;
  else
    ed->exec->region->addrs = 2;
  if ((status = check_address_bounds (first, ed)) < 0
      || (status = check_address_bounds (second, ed)) < 0
      || (status = check_address_bounds (dot, ed)) < 0)
    return status;
  ed->exec->region->start = first;
  ed->exec->region->end = second;
  ed->state->dot = dot;
  return ed->exec->region->addrs;
}


/* next_address: Get line address and address offset from command
   buffer. Return status == 1 if found, status == 0 if not found, and
   status < 0 if non-address character or error. */
int
next_address (addr, ed)
     off_t *addr;
     ed_buffer_t *ed;
{
  char *input_prev;
  int status;

  SKIP_WHITESPACE (ed);
  input_prev = ed->input;
  return (((status = line_address (addr, ed)) < 0
           || (status = address_offset (addr, ed)) < 0)
          ? status : ed->input != input_prev);
}


/*  line_address: Get line address from command buffer; return
    status. */
static int
line_address (addr, ed)
     off_t *addr;
     ed_buffer_t *ed;
{
  int c;
  int status = 0;

  switch (c = *ed->input)
    {
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
      STRTOLL_THROW (*addr, ed->input, &ed->input, ERR);
      break;
    case '$':
      *addr = ed->state->lines;
      /* FALLTHROUGH */
    case '.':
      ++ed->input;
      break;
    case '/':
    case '?':
      if ((status = check_address_bounds (*addr, ed)) < 0)
        return status;
      ed->state->dot = *addr;
      spl1 ();
      if ((status =
           get_matching_node_address (get_compiled_regex (c, RE_SEARCH, ed),
                                      c == '/', addr, ed)) < 0)
        {
          spl0 ();
          return status;
        }
      spl0 ();
      if (*ed->input == c)
        ++ed->input;
      break;
    case '\'':
      ++ed->input;
      if ((status = get_marked_node_address (*ed->input++, addr, ed)) < 0)
        return status;
      break;
    default:
      break;
    }
  return 0;
}


/*  address_offset: Apply offset from command buffer to address. */
int
address_offset (addr, ed)
     off_t *addr;
     ed_buffer_t *ed;
{
  off_t n = 1;

  for (;; n = 1)
    switch ((unsigned char) *ed->input)
      {
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
        STRTOLL_THROW (n, ed->input, &ed->input, ERR);
        *addr += n;
        break;
      case '+':
        if (isdigit ((unsigned char) *++ed->input))
          STRTOLL_THROW (n, ed->input, &ed->input, ERR);
        *addr += n;
        break;
      case '-':
      case '^':
        if (isdigit ((unsigned char) *++ed->input))
          STRTOLL_THROW (n, ed->input, &ed->input, ERR);
        *addr -= n;
        break;
      case ' ':
      case '\t':
        SKIP_WHITESPACE (ed);
        if (isdigit ((unsigned char) *ed->input))
          {
            STRTOLL_THROW (n, ed->input, &ed->input, ERR);
            *addr += n;
          }
        break;
      default:
        return 0;
      }
  /* NOTREACHED */
}


/* check_address_bounds: Verify that line address is legal with
   respect to current bounds. */
static int
check_address_bounds (addr, ed)
     off_t addr;
     ed_buffer_t *ed;
{
  if (addr < 0 || ed->state->lines < addr)
    {
      ed->exec->err = _("Address out of range");
      return ERR;
    }
  return 0;
}


#define WHITE_SPACE " \f\n\r\t\v"


/* file_glob: Get file name pattern from command buffer. Return
   pointer to either first file name in expansion of pattern or, if
   pattern is empty, the next name of previous expansion (may be
   NULL). */
char *
file_glob (len, cm, replace, ed)
     size_t *len;
     int cm;
     int replace;
     ed_buffer_t *ed;
{
  static glob_t *one_time_glob = NULL;  /* one-time file glob */

  glob_t *gp = NULL;
  char *pattern;
  char *xl;
  char *s;
  off_t offs = 0;

  if (!(xl = get_extended_line (len, 1, ed)))

    /* Propagate stream status. */
    return NULL;

  /* XXX - Too restrictive, e.g., "./foo" should be permissible... */
  if (ed->exec->opt & RESTRICTED && (!strcmp (xl, "..") || strchr (xl, '/')))
    {
      ed->exec->err = _("Access restricted to working directory");
      return NULL;
    }
  ed->input += *len + 1;

  /* Process xl for zero or more whitespace-delimited file globs.
     Whitespace within a file glob must be back slash-escaped (\). */
  if ((pattern = strtok_with_delimiters (xl, WHITE_SPACE)))
    {
      /* Not updating file->glob, so use  one_time_glob instead. */
      if (ed->file->list->gl_pathv && !replace)
        {
          if (!one_time_glob
              && (one_time_glob =
                  (glob_t *) calloc (1, sizeof (glob_t))) == NULL)
            {
              fprintf (stderr, "%s\n", strerror (errno));
              ed->exec->err = _("Memory exhausted");
              return NULL;
            }
          else if (one_time_glob->gl_pathv)
            {
              globfree (one_time_glob);
              one_time_glob->gl_offs = 0;
              one_time_glob->gl_pathc = 0;
              one_time_glob->gl_pathv = NULL;
            }
          gp = one_time_glob;
        }

      /* First time, gl_pathv == argv, so re-initialize file_glob. */
      else if (ed->file->list->gl_offs > 0)
        {
          ed->file->list->gl_offs = 0;
          ed->file->list->gl_pathc = 0;
          ed->file->list->gl_pathv = NULL;
          gp = ed->file->list;
        }
      else
        {
          /* globfree(3) wants the original pathv allocated by glob(3). */
          *ed->file->list = *ed->file->glob;
          if (ed->file->list->gl_pathv && *ed->file->list->gl_pathv)
            globfree (ed->file->list);
          ed->file->list->gl_offs = 0;
          ed->file->list->gl_pathc = 0;
          ed->file->list->gl_pathv = NULL;
          gp = ed->file->list;
        }

      for (s = pattern; pattern;
           pattern = strtok_with_delimiters (NULL, WHITE_SPACE))
        {
          if (!expand_glob (pattern, s == pattern ? 0 : GLOB_APPEND, gp, ed))
            {
              /* Reinitialize ed->file->glob/list to known state. */
              if (gp == ed->file->list)
                {
                  gp->gl_offs = 0;
                  gp->gl_pathc = 0;
                  gp->gl_pathv = NULL;
                  *ed->file->glob = *ed->file->list = *gp;
                }
              return NULL;
            }
        }
      if (replace && gp && gp->gl_pathv && *gp->gl_pathv)
        *ed->file->glob = *gp;
    }
  /* Select from existing file list. */
  else
    switch (cm)
      {
      case 'n':
        if (!ed->file->list->gl_pathv
            || ed->file->list->gl_pathc <= 1)
          {
            ed->exec->err = _("No more files");
            return NULL;
          }
        --ed->file->list->gl_pathc;
        ++ed->file->list->gl_pathv;
        offs = ed->file->glob->gl_pathc - ed->file->list->gl_pathc;
        gp = ed->file->glob;
        break;

      case 'p':
        if (!ed->file->list->gl_pathv
            || ed->file->list->gl_pathc >= ed->file->glob->gl_pathc)
          {
            ed->exec->err = _("No more files");
            return NULL;
          }
        ++ed->file->list->gl_pathc;
        --ed->file->list->gl_pathv;
        offs = ed->file->glob->gl_pathc - ed->file->list->gl_pathc;
        gp = ed->file->glob;

      default:
        offs = ed->file->glob->gl_pathc - ed->file->list->gl_pathc;
        gp = ed->file->glob;

        /* Command `~e' with no arguments rewinds file list. */
        ed->file->list->gl_pathc += offs;
        ed->file->list->gl_pathv -= offs;
        offs = 0;

        /* Return default file name if file list empty. */
        if (!gp->gl_pathc && ed->file->name)
          return ed->file->name;
        break;
      }

  if (!(gp && gp->gl_pathv && *gp->gl_pathv)
      || !(0 <= offs && offs <= ed->file->glob->gl_pathc - 1))
    {
      ed->exec->err = _("No more files");
      return NULL;
    }
  if (!is_valid_name (*(gp->gl_pathv + offs), ed))
    return NULL;
  *len = strlen (*(gp->gl_pathv + offs));
  return *(gp->gl_pathv + offs);
}


/* strtok_with_delimiters: Parse `s' into `delims'-separated tokens.
   Tokens may themselves contain `delims' chars provided the chars are
   preceded by a backslash (\) escape. In this case, the backslash is
   removed from the returned token.
   NB: Only ASCII `delims' are supported. */
static char *
strtok_with_delimiters (s, delims)
     char *s;
     const char *delims;
{
  static char *t = NULL;

  char *tok;

  if (s)
    t = s;

  /* If t != NULL string, then *t != '\0' char, per test below. */
  else if (!t || *++t == '\0')
    return NULL;

  /* Strip leading `delims' chars. */
  if (*(t += strspn (t, delims)) == '\0')
    return NULL;

  /* Remove escapes preceding `delims'; break on unescaped `delims' char. */
  for (tok = s = t;; ++t, ++s)
    if (*t == '\\' && strchr (delims, (unsigned char) *(t + 1)))
        *s = *++t;
    else if ((*s = *t) == '\0' || strchr (delims, (unsigned char) *s))
      break;

  /* No more tokens. */
  if (*t == '\0')
    t = NULL;
  else
    *s = '\0';
  return tok;
}


/* expand_glob: Attempt to expand given file-glob pattern. If
   expansion does not match any existing files, then try expanding
   just the directory name, if any. If that succeeds, then append the
   filename to each directory in expansion. */
glob_t *
expand_glob (pattern, append, gp, ed)
     char *pattern;
     int append;
     glob_t *gp;
     ed_buffer_t *ed;
{
  struct stat sb;
  char **pathv = NULL;
  char **pv = NULL;
  char *basename = NULL;
  char *pathname = NULL;
  size_t len;
  int gloff = gp->gl_pathc;
  int status = 0;

  /* glob(3) may not properly handle excessively long patterns. */
  if (strlen (pattern) >= get_path_max (pattern))
    {
      ed->exec->err = _("File glob too long");
      return NULL;
    }

#ifndef GLOB_TILDE
# define GLOB_TILDE 0
#endif

  /* If cannot match existing file or directory... */
  if ((status = glob (pattern, GLOB_TILDE | append, NULL, gp)) != 0)
    {

      /* If not a directory pattern, then return unchecked glob. */
      if ((basename = strrchr (pattern, '/')) == NULL)
        {
          if ((status = glob (pattern, GLOB_NOCHECK | GLOB_TILDE | append,
                              NULL, gp)) != 0)
            {
#if __linux__
              /* GNU/Linux does not set errno. */
              fprintf (stderr, "%s: %s\n", pattern,
                       _("Pathname expansion error"));
#else
              fprintf (stderr, "%s: %s\n", pattern, strerror (errno));
#endif
              ed->exec->err = _("Pathname expansion error");
              return NULL;
            }

        }
      /* Otherwise, glob pattern less basename. */
      else
        {
          *basename++ = '\0';
          errno = 0;

          if ((status = glob (pattern, GLOB_TILDE | append, NULL, gp)) != 0)
            {
#if __linux__
              /* GNU/Linux does not set errno. */
              fprintf (stderr, "%s: %s\n", pattern,
                       _("No such file or directory"));
#else
              fprintf (stderr, "%s: %s\n", pattern, strerror (errno));
#endif
              ed->exec->err = _("Pathname expansion error");
              return NULL;
            }

          /* If any directories matched, then append basename to gl_pathv. */
          for (pathv = gp->gl_pathv; *pathv; ++pathv)
            {

              /* Skip prior expansions in gl_pathv. */
              if (pathv - gp->gl_pathv < gloff)
                continue;
              else if (stat (*pathv, &sb) == -1)
                {
                  fprintf (stderr, "%s: %s\n", *pathv, strerror (errno));
                  ed->exec->err = _("File stat error");
                  return NULL;
                }
              else if (S_ISDIR(sb.st_mode))
                {
                  /* dirname + '/' + basename */
                  len = strlen (*pathv) + strlen (basename) + 1;

                  spl1 ();

                  if ((pathname = (char *) malloc (len + 1)) == NULL)
                    {
                      fprintf (stderr, "%s\n", strerror (errno));
                      ed->exec->err = _("Memory exhausted");
                      spl0 ();
                      return NULL;
                    }
                  snprintf (pathname, len + 1, "%s/%s", *pathv, basename);
                  free (*pathv);
                  *pathv = pathname;

                  spl0 ();
                }

              else
                {
                  spl1 ();

                  /* Remove non-directory entry in pathv. */
                  free (*(pv = pathv--));

                  /* Fill void with remaining elements in pathv. */
                  do
                    {
                      *pv = *(pv + 1);
                    }
                  while (*++pv);
                  --gp->gl_pathc;

                  spl0 ();
                }
            }

          /* No directories in expansion. */
          if (gp->gl_pathc <= 0)
            return NULL;
        }
    }
  return gp;
}


/* is_valid_name: Return status of file name check. */
static char *
is_valid_name (name, ed)
     const char *name;
     ed_buffer_t *ed;
{
  static char *fn;
  static size_t fn_size;

  char *s;
  size_t len;

  /* Push file name onto command buffer. Append newline if missing. */
  len = strlen (ed->input = (char *) name);
  if (name[len - 1] != '\n')
    {
      REALLOC_THROW (fn, fn_size, len + 2, NULL, ed);
      strcpy (ed->input = fn, name);
      strcpy (fn + len, "\n");
    }
  if ((s = file_name (&len, ed)) && strcmp (name, s))
    {
      ed->exec->err = _("Invalid file name");
      return NULL;
    }
  return s;
}


/* file_name: Get file name from command buffer; return pointer to
   copy, which may be empty ("") or have embedded newlines ('\n'). */
char *
file_name (len, ed)
     size_t *len;
     ed_buffer_t *ed;
{
  static char *fn = NULL;       /* File name buffer */
  static size_t fn_size = 0;    /* Buffer size */

  char *xl;

  if (!(xl = get_extended_line (len, 1, ed)))

    /* Propagate stream status. */
    return NULL;

  /* XXX - Too restrictive, e.g., "./foo" should be permissible... */
  if (ed->exec->opt & RESTRICTED && (!strcmp (xl, "..") || strchr (xl, '/')))
    {
      ed->exec->err = _("Access restricted to working directory");
      return NULL;
    }
  ed->input += *len + 1;
  REALLOC_THROW (fn, fn_size, *len + 1, NULL, ed);
  strcpy (fn, xl);
  return fn;
}


/* regular_expression: Get delimited regular expression pattern from the
   command buffer; return pointer to copy. */
char *
regular_expression (dc, len, ed)
     unsigned dc;               /* pattern delimiting char */
     size_t *len;
     ed_buffer_t *ed;
{
  static char *lhs = NULL;      /* substitution template buffer */
  static size_t lhs_size = 0;   /* buffer size */

  char *s = ed->input;

  for (; (unsigned char) *ed->input != dc && *ed->input != '\n'; ++ed->input)
    switch (*ed->input)
      {
      default:
        break;
      case '[':
        if (!(ed->input = character_class (++ed->input, ed)))
          {
            ed->exec->err = _("Brackets ([]) unbalanced");
            return NULL;
          }
        break;
      case '\\':
        if (*++ed->input == '\n')
          {
            ed->exec->err = _("Backslash (\\) unexpected");
            return NULL;
          }
        break;
      }

  *len = ed->input - s;
  REALLOC_THROW (lhs, lhs_size, *len + 1, NULL, ed);
  memcpy (lhs, s, *len);
  *(lhs + *len) = '\0';
#if !defined REG_PEND && !defined HAVE_REG_SYNTAX_T
  if (ed->state->is_binary)
    NUL_TO_NEWLINE (lhs, *len);
#endif
  return lhs;
}


/* character_class: Expand a POSIX character class. */
static char *
character_class (s, ed)
     const char *s;
     ed_buffer_t *ed;
{
  unsigned c, d;

  if (*s == '^')
    ++s;
  if (*s == ']')
    ++s;
  for (; *s != ']' && *s != '\n'; ++s)
    if (*s == '[' && ((d = (unsigned char) *(s + 1)) == '.'
                      || d == ':' || d == '='))
      for (++s, c = (unsigned char) *++s; *s != ']' || c != d; ++s)
        if ((c = *s) == '\n')
          return NULL;
  return (*s == ']' ? (char *) s : NULL);
}


/* shell_command: Get shell command from command buffer; return
   substitution status. */
char *
shell_command (len, subs, ed)
     size_t *len;               /* Shell command length */
     int *subs;                 /* Substitution count */
     ed_buffer_t *ed;
{
  static char *sc = NULL;         /* Shell command buffer */
  static char *sc_curr = NULL;    /* Current command buffer */
  static char *sc_prev = NULL;    /* Previous command buffer */
  static size_t sc_size = 0;      /* Buffer size */
  static size_t sc_prev_size = 0; /* Buffer size */
  static size_t sc_curr_size = 0; /* Buffer size */

  char *xl, *fn = NULL;
  size_t m, n;

  if (ed->exec->opt & RESTRICTED)
    {
      ed->exec->err = _("Shell access restricted");
      return NULL;
    }
  if (!(xl = get_extended_line (&n, 1, ed)))

    /* Propagate stream status */
    return NULL;

  /* Preserve unexpanded command so that `%' and `%%' get expanded to
     current file name when repeated via `!!'. */
  REALLOC_THROW (sc_curr, sc_curr_size, n + 1, NULL, ed);
  strcpy (sc_curr, xl);

  ed->input += n + 1;
  for (*subs = *len = 0; *xl != '\0'; ++xl)
    switch (*xl)
      {
      default:
        REALLOC_THROW (sc, sc_size, *len + 1, NULL, ed);

        /* Substitute '\%' with `%'. */
        *(sc + (*len)++) = *xl == '\\' && *(xl + 1) == '%' ? *++xl : *xl;
        break;
      case '%':

        /*  Substitute '%%' with ed->exec->file_script. */
        if (*(xl + 1) == '%')
          {
            fn = ed->exec->file_script ? ed->exec->file_script : "stdin";
            ++xl;
          }
        /*  Substitute '%' with ed->file_name. */
        else
          fn = ed->file->name ? ed->file->name : "";
        m = strlen (fn);
        REALLOC_THROW (sc, sc_size, *len + m + 1, NULL, ed);
        memcpy (sc + *len, fn, m);
        *len += m;
        ++*subs;
        break;
      case '!':

        /* Replace only leading `!!' with previous `!command'. */
        if (*len || *(xl + 1) != '!')
          {
            REALLOC_THROW (sc, sc_size, *len + 1, NULL, ed);
            *(sc + (*len)++) = *xl;
          }
        else if (!sc_prev || (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)
                              && !*(sc_prev + 1)))
          {
            ed->exec->err = _("No previous shell command");
            return NULL;
          }

        else
          {
            m = strlen (sc_prev);

            /* Append sc_curr without `!!' prefix to sc_prev. */
            REALLOC_THROW (sc_curr, sc_curr_size, n + m - 1, NULL, ed);
            memmove (sc_curr + m, sc_curr + 2, n - 1);
            memcpy (sc_curr, sc_prev, m);
            xl = sc_curr;
            n += m - 2;         /* Update (unused) strlen of xl. */

            /* Assert: xl now begins with a single `!'. */
            REALLOC_THROW (sc, sc_size, *len + 1, NULL, ed);
            *(sc + (*len)++) = *xl;
            ++*subs;
          }
        break;
      }

  /* Preserve current (unexpanded) shell command. */
  REALLOC_THROW (sc_prev, sc_prev_size, sc_curr_size, NULL, ed);
  memcpy (sc_prev, sc_curr, sc_curr_size);
  *(sc + *len) = '\0';
  return sc;
}


/* has_trailing_escape: Return the parity of escapes preceding a
   character *t, in a string, s. */
int
has_trailing_escape (s, t)
     const char *s;
     const char *t;
{
  return (s == t || *(t - 1) != '\\' ? 0 : !has_trailing_escape (s, t - 1));
}
