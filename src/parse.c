/* parse.c: Editor command-parsing routines for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2013-08-06 <alm@slewsys.org>

   This file is part of ed. */

#include "ed.h"
#include <sys/stat.h>

/* Static function declarations. */
static char *character_class __P ((const char *, ed_state_t *));
static int check_address_bounds __P ((off_t, ed_state_t *));
static glob_t *expand_glob __P ((char *, int, glob_t *, ed_state_t *));
static char *is_valid_name __P ((const char *, ed_state_t *));
static int line_address __P ((off_t *, ed_state_t *));
static char *strtok_with_delimiters __P ((char *, const char *));


#define IS_DELIMITER(c) ((c) == '%' || (c) == ',' || (c) == ';')


/* address_range: Get line addresses from the command buffer until an
   illegal address is seen; return address count. */
int
address_range (ed)
     ed_state_t *ed;
{
  /* As per SUSv3, 2004, intermediate addresses may exceed
     ed->buf[0].addr_last or be negative, so use signed types. */
  off_t addr;
  off_t first, second, dot;
  int have_dc;                  /* If set, have address delimiter char. */
  int status;

  ed->region.addrs = 0;
  first = second = dot = ed->buf[0].dot;
  SKIP_WHITESPACE (ed);
  have_dc = IS_DELIMITER (*ed->stdin);
  do
    {
      if (have_dc)
        {
          if (ed->region.addrs)
            first = *ed->stdin == ';' ? (dot = second) : second;
          else
            {
              first = *ed->stdin == ';' ? dot : 1;
              second = ed->buf[0].addr_last;
              ed->region.addrs = 2;
            }
        }
      ed->stdin += have_dc;
      addr = dot;
      if ((status = next_address (&addr, ed)) < 0)
        return status;
      if (status > 0)
        {
          second = addr;
          ++ed->region.addrs;
        }
    }
  while ((have_dc = IS_DELIMITER (*ed->stdin)));

  if ((ed->region.addrs = min (2, ed->region.addrs)) < 2)
    first = second;
  if ((status = check_address_bounds (first, ed)) < 0
      || (status = check_address_bounds (second, ed)) < 0
      || (status = check_address_bounds (dot, ed)) < 0)
    return status;
  ed->region.start = first;
  ed->region.end = second;
  ed->buf[0].dot = dot;
  return ed->region.addrs;
}


/* next_address: Get line address and address offset from command
   buffer. Return status == 1 if found, status == 0 if not found, and
   status < 0 if non-address character or error. */
int
next_address (addr, ed)
     off_t *addr;
     ed_state_t *ed;
{
  char *stdin_prev;
  int status;

  SKIP_WHITESPACE (ed);
  stdin_prev = ed->stdin;
  return (((status = line_address (addr, ed)) < 0
           || (status = address_offset (addr, ed)) < 0)
          ? status : ed->stdin != stdin_prev);
}


/*  line_address: Get line address from command buffer; return
    status. */
static int
line_address (addr, ed)
     off_t *addr;
     ed_state_t *ed;
{
  int c;
  int status = 0;

  switch (c = *ed->stdin)
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
      STRTOLL_THROW (*addr, ed->stdin, &ed->stdin, ERR);
      break;
    case '$':
      *addr = ed->buf[0].addr_last;
      /* FALLTHROUGH */
    case '.':
      ++ed->stdin;
      break;
    case '/':
    case '?':
      if ((status = check_address_bounds (*addr, ed)) < 0)
        return status;
      ed->buf[0].dot = *addr;
      spl1 ();
      if ((status =
           get_matching_node_address (get_compiled_regex (c, RE_SEARCH, ed),
                                      c == '/', addr, ed)) < 0)
        {
          spl0 ();
          return status;
        }
      spl0 ();
      if (*ed->stdin == c)
        ++ed->stdin;
      break;
    case '\'':
      ++ed->stdin;
      if ((status = get_marked_node_address (*ed->stdin++, addr, ed)) < 0)
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
     ed_state_t *ed;
{
  off_t n = 1;

  for (;; n = 1)
    switch ((unsigned char) *ed->stdin)
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
        STRTOLL_THROW (n, ed->stdin, &ed->stdin, ERR);
        *addr += n;
        break;
      case '+':
        if (isdigit ((unsigned char) *++ed->stdin))
          STRTOLL_THROW (n, ed->stdin, &ed->stdin, ERR);
        *addr += n;
        break;
      case '-':
      case '^':
        if (isdigit ((unsigned char) *++ed->stdin))
          STRTOLL_THROW (n, ed->stdin, &ed->stdin, ERR);
        *addr -= n;
        break;
      case ' ':
      case '\t':
        SKIP_WHITESPACE (ed);
        if (isdigit ((unsigned char) *ed->stdin))
          {
            STRTOLL_THROW (n, ed->stdin, &ed->stdin, ERR);
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
     ed_state_t *ed;
{
  if (addr < 0 || ed->buf[0].addr_last < addr)
    {
      ed->exec.err = _("Address out of range");
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
     ed_state_t *ed;
{
  char *fn = NULL;              /* Glob expansion of non-existent pathname. */
  size_t fn_size = 0;
  static glob_t one_time_glob;  /* one-time file glob */

  glob_t *gp = NULL;
  size_t offs = 0;
  char *pattern;
  char *xl;
  char *s;
  char *sep;                    /* Handle glob of no-existent pathnames. */

  if (!(xl = get_extended_line (len, 1, ed)))

    /* Propagate stream status. */
    return NULL;

  /* XXX - Too restrictive, e.g., "./foo" should be permissible... */
  if (ed->exec.opt & RESTRICTED && (!strcmp (xl, "..") || strchr (xl, '/')))
    {
      ed->exec.err = _("Access restricted to working directory");
      return NULL;
    }
  ed->stdin += *len + 1;

  /* Process xl for zero or more whitespace-delimited file globs.
     Whitespace within a file glob must be back slash-escaped (\). */
  if ((pattern = strtok_with_delimiters (xl, WHITE_SPACE)))
    {
      /* Preserve file_glob by using one_time_glob instead. */
      if (ed->file.list.gl_pathv && !replace)
        {
          if (one_time_glob.gl_pathv)
            {
              globfree (&one_time_glob);
              one_time_glob.gl_pathc = 0;
              one_time_glob.gl_offs = 0;
            }
          gp = &one_time_glob;
        }

      /* First time, gl_pathv == argv, so re-initialize file_glob. */
      else if (ed->file.list.gl_offs > 0)
        {
          ed->file.list.gl_offs = 0;
          ed->file.list.gl_pathc = 0;
          ed->file.list.gl_pathv = NULL;
          gp = &ed->file.list;
        }
      else
        {
          /* globfree(3) wants the original pathv allocated by glob(3). */
          ed->file.list = ed->file.glob;
          if (*ed->file.list.gl_pathv)
            globfree (&ed->file.list);
          ed->file.list.gl_offs = 0;
          ed->file.list.gl_pathc = 0;
          ed->file.list.gl_pathv = NULL;
          gp = &ed->file.list;
        }

      for (s = pattern; pattern;
           pattern = strtok_with_delimiters (NULL, WHITE_SPACE))
        {
          if (!expand_glob (pattern, s == pattern ? 0 : GLOB_APPEND, gp, ed))
            return NULL;
        }
      if (replace && gp && *gp->gl_pathv)
        ed->file.glob = *gp;
    }
  else if (cm == 'n' && *ed->file.list.gl_pathv && ed->file.list.gl_pathc > 1)
    {
      --ed->file.list.gl_pathc;
      ++ed->file.list.gl_pathv;
      /* gp = &ed->file.list; */
      offs = ed->file.glob.gl_pathc - ed->file.list.gl_pathc;
      gp = &ed->file.glob;
    }
  else if (cm == 'p' && *ed->file.list.gl_pathv
           && ed->file.list.gl_pathc < ed->file.glob.gl_pathc)
    {
      ++ed->file.list.gl_pathc;
      --ed->file.list.gl_pathv;
      /* gp = &ed->file.list; */
      offs = ed->file.glob.gl_pathc - ed->file.list.gl_pathc;
      gp = &ed->file.glob;
    }
  /* ed-0.271-b3: don't rewind file list if no argument given. */
  /*
  else if (ed->file.is_glob && *ed->file.glob.gl_pathv)
    {
      if (replace)
        ed->file.list = ed->file.glob;
      gp = &ed->file.glob;
    }
  */
  else
    {
      offs = ed->file.glob.gl_pathc - ed->file.list.gl_pathc;
      gp = &ed->file.glob;
    }
  if (!gp || !gp->gl_pathv || !*(gp->gl_pathv + offs))
    {
      ed->exec.err = _("No more files");
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
     ed_state_t *ed;
{
  struct stat sb;
  char **pathv = NULL;
  char **pv = NULL;
  char *sep = NULL;
  char *pathname = NULL;
  size_t len;
  int gloff = gp->gl_pathc;
  int offs;

  /* glob(3) may not properly handle excessively long patterns. */
  if (strlen (pattern) >= get_path_max (pattern))
    {
      ed->exec.err = _("File glob too long");
      return NULL;
    }

#ifdef GLOB_TILDE
  if (glob (pattern, GLOB_TILDE | append, NULL, gp) < 0)
#else
  if (glob (pattern, append, NULL, gp) < 0)
#endif  /* !GLOB_TILDE */
    {

      /* Filename expansion failed, so try dirname expansion. */
      if ((sep = strrchr (pattern, '/')) != NULL)
        {
          errno = 0;
          *sep = '\0';
        }

#ifdef GLOB_TILDE
      if (glob (pattern, GLOB_TILDE | append, NULL, gp) < 0
          || !(sep && gp && gp->gl_pathv))
#else
      if (glob (pattern, append, NULL, gp) < 0
          || !(sep && gp && gp->gl_pathv))

#endif  /* !GLOB_TILDE */
        {
          fprintf (stderr, "%s: %s\n", pattern, strerror (errno));
          ed->exec.err = _("Pathname expansion error");
          return NULL;
        }

      /* Assert: dirname expansion succceeded, so amend gl_pathv. */

      for (pathv = gp->gl_pathv; *pathv; ++pathv)
        {

          /* Skip prior expansions in gl_pathv. */
          if (pathv - gp->gl_pathv < gloff)
            continue;
          else if (stat (*pathv, &sb) == -1)
            {
              fprintf (stderr, "%s: %s\n", *pathv, strerror (errno));
              ed->exec.err = _("File stat error");
              return NULL;
            }

          /* Append filename part of pattern to new directory expansions. */
          else if (S_ISDIR(sb.st_mode))
            {
 
             /* dirname + '/' + basename */
              len = strlen (*pathv) + strlen (sep + 1) + 1;

              spl1 ();

              if ((pathname = (char *) malloc (len + 1)) == NULL)
                {
                  fprintf (stderr, "%s\n", strerror (errno));
                  ed->exec.err = _("Memory exhausted");
                  spl0 ();
                  return NULL;
                }
              snprintf (pathname, len + 1, "%s/%s", *pathv, sep + 1);
              free (*pathv);
              *pathv = pathname;

              spl0 ();
            }

          /* Remove non-directory entries. */
          else
            {
              spl1 ();

              /* Move remaining elements of pathv forward. */
              free (*(pv = pathv--));
              do
                {
                  *pv = *(pv + 1);
                }
              while (*++pv);
              --gp->gl_pathc;
              
              spl0 ();
            }
        }
    }
  return gp;
}


/* is_valid_name: Return status of file name check. */
static char *
is_valid_name (name, ed)
     const char *name;
     ed_state_t *ed;
{
  static char *fn;
  static size_t fn_size;

  char *s;
  size_t len;

  /* Push file name onto command buffer. Append newline if missing. */
  len = strlen (ed->stdin = (char *) name);
  if (name[len - 1] != '\n')
    {
      REALLOC_THROW (fn, fn_size, len + 2, NULL, ed);
      strcpy (ed->stdin = fn, name);
      strcpy (fn + len, "\n");
    }
  if ((s = file_name (&len, ed)) && strcmp (name, s))
    {
      ed->exec.err = _("Invalid file name");
      return NULL;
    }
  return s;
}


/* file_name: Get file name from command buffer; return pointer to
   copy, which may be empty ("") or have embedded newlines ('\n'). */
char *
file_name (len, ed)
     size_t *len;
     ed_state_t *ed;
{
  static char *fn = NULL;       /* File name buffer */
  static size_t fn_size = 0;    /* Buffer size */

  char *xl;

  if (!(xl = get_extended_line (len, 1, ed)))

    /* Propagate stream status. */
    return NULL;

  /* XXX - Too restrictive, e.g., "./foo" should be permissible... */
  if (ed->exec.opt & RESTRICTED && (!strcmp (xl, "..") || strchr (xl, '/')))
    {
      ed->exec.err = _("Access restricted to working directory");
      return NULL;
    }
  ed->stdin += *len + 1;
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
     ed_state_t *ed;
{
  static char *lhs = NULL;      /* substitution template buffer */
  static size_t lhs_size = 0;   /* buffer size */

  char *s = ed->stdin;

  for (; (unsigned char) *ed->stdin != dc && *ed->stdin != '\n'; ++ed->stdin)
    switch (*ed->stdin)
      {
      default:
        break;
      case '[':
        if (!(ed->stdin = character_class (++ed->stdin, ed)))
          {
            ed->exec.err = _("Brackets ([]) unbalanced");
            return NULL;
          }
        break;
      case '\\':
        if (*++ed->stdin == '\n')
          {
            ed->exec.err = _("Backslash (\\) unexpected");
            return NULL;
          }
        break;
      }

  *len = ed->stdin - s;
  REALLOC_THROW (lhs, lhs_size, *len + 1, NULL, ed);
  memcpy (lhs, s, *len);
  *(lhs + *len) = '\0';
#if !defined REG_PEND && !defined HAVE_REG_SYNTAX_T
  if (ed->buf[0].is_binary)
    NUL_TO_NEWLINE (lhs, *len);
#endif
  return lhs;
}


/* character_class: Expand a POSIX character class. */
static char *
character_class (s, ed)
     const char *s;
     ed_state_t *ed;
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
     ed_state_t *ed;
{
  static char *sc = NULL;         /* Shell command buffer */
  static char *sc_curr = NULL;    /* Current command buffer */
  static char *sc_prev = NULL;    /* Previous command buffer */
  static size_t sc_size = 0;      /* Buffer size */
  static size_t sc_prev_size = 0; /* Buffer size */
  static size_t sc_curr_size = 0; /* Buffer size */

  char *xl, *fn = NULL;
  size_t m, n;

  if (ed->exec.opt & RESTRICTED)
    {
      ed->exec.err = _("Shell access restricted");
      return NULL;
    }
  if (!(xl = get_extended_line (&n, 1, ed)))

    /* Propagate stream status */
    return NULL;

  /* Preserve unexpanded command so that `%' gets expanded to current
     file name when repeated via `!!'. */
  REALLOC_THROW (sc_curr, sc_curr_size, n + 1, NULL, ed);
  strcpy (sc_curr, xl);

  ed->stdin += n + 1;
  for (*subs = *len = 0; *xl != '\0'; ++xl)
    switch (*xl)
      {
      default:
        REALLOC_THROW (sc, sc_size, *len + 1, NULL, ed);

        /* Substitute '\%' with `%'. */
        *(sc + (*len)++) = *xl == '\\' && *(xl + 1) == '%' ? *++xl : *xl;
        break;
      case '%':

        /*  Substitute '%%' with ed->exec.file_script. */
        if (*(xl + 1) == '%')
          {
            fn = ed->exec.file_script ? ed->exec.file_script : "(stdin)";
            ++xl;
          }
        /*  Substitute '%' with ed->file_name. */
        else if (!(fn = ed->file.name))
          {
            /* Preserve current (unexpanded) shell command. */
            REALLOC_THROW (sc_prev, sc_prev_size, sc_curr_size, NULL, ed);
            memcpy (sc_prev, sc_curr, sc_curr_size);

            ed->exec.err = _("File name not set");
            return NULL;
          }
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
        else if (!sc_prev || (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL)
                              && !*(sc_prev + 1)))
          {
            ed->exec.err = _("No previous shell command");
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

/*
 * Local variables:
 * mode: c
 * eval: (add-hook 'write-file-functions 'time-stamp)
 * time-stamp-start: "Last modified: "
 * time-stamp-format: "%:y-%02m-%02d <%u@%h>"
 * time-stamp-end: "$"
 * End:
 */
