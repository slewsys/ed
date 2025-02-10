/* main.c: Entry point for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#ifndef lint
char *copyright =
  "@(#) Copyright © 1993-2023 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research.\n";
#endif  /* not lint */

#include "ed.h"
#include "argmax.h"
#include "getopt.h"

#include <pwd.h>

#define STDIN "/dev/stdin"

#ifdef HAVE_SIGLONGJMP
sigjmp_buf env;
#else
jmp_buf env;
#endif  /* !HAVE_SIGLONGJMP */

/* Global declaration - required by interrupt handlers. */
ed_buffer_t *ed;

/* Static function declarations. */
static int append_address_command (const char *, ed_buffer_t *);
static void ed_usage (int, ed_buffer_t *);

#ifdef WANT_ED_ENVAR
static char **getenv_init_argv (const char *, int *, ed_buffer_t *);
#endif

static int next_edit (int, ed_buffer_t *);

#ifdef WANT_ADDRESS_ARGUMENTS
static char **collect_address_args (int *, char **, ed_buffer_t *);
#endif

static int save_edit (int, ed_buffer_t *);
static void script_die (int, ed_buffer_t *);

/* ed: line editor */
int
main (int argc, char **argv)
{
  struct option long_opts[16] =
    {
      {"ansi-color",      no_argument,       NULL, 'R'},

#ifdef WANT_SCRIPT_FLAGS
      {"expression",      required_argument, NULL, 'e'},
      {"file",            required_argument, NULL, 'f'},
#endif

      {"help",            no_argument,       NULL, 'h'},

#ifdef WANT_ED_ENCRYPTION
      {"crypt",           no_argument,       NULL, 'x'},
#endif

      {"in-place",        optional_argument, NULL, 'i'},
      {"prompt",          required_argument, NULL, 'p'},
      {"regexp-extended", no_argument,       NULL, 'E'},
      {"regexp-extended", no_argument,       NULL, 'r'},
      {"scripted",        no_argument,       NULL, 's'},
      {"traditional",     no_argument,       NULL, 'G'},
      {"verbose",         no_argument,       NULL, 'v'},
      {"version",         no_argument,       NULL, 'V'},
      {"write",           no_argument,       NULL, 'w'},
      {0,                 0,                 0,    0}
    };
  const char *short_opts = "E"
#ifdef WANT_SCRIPT_FLAGS
    "e:f:i::"
#endif
    "Ghp:RrsVvw"
#ifdef WANT_ED_ENCRYPTION
    "x"
#endif
    ;

  char **argv_new;
  char **argv_prev = NULL;

  char *cs;
  long l;
  size_t len;
  int argc_new;
  int argc_prev = 0;
  int c;
  int status = 0;
  int signal_status = 0;

  if ((ed = alloc_ed_buffer ()) == NULL)
      exit (1);

  if ((len = strlen (*argv)) > 2 && *(*argv + len - 3) == 'r')
    ed->exec->opt |= RESTRICTED;

#ifdef WANT_ED_ENVAR

  /* Check `ED' environment variable (enabled with configure
     switch `--enable-ed-envar'). */
  if ((argv_new = getenv_init_argv ("ED", &argc_new, ed)))
    {
      argv_prev = argv, argv = argv_new;
      argc_prev = argc, argc = argc_new;
    }
#endif

top:
  while ((c = getopt_long (argc, argv, short_opts, long_opts, NULL)) != -1)
    switch (c)
      {
      default:
        ed_usage (2, ed);
      case 0:
        break;
      case 'E':                 /* Enable extended regular expressions. */
        ed->exec->opt |=  REGEX_EXTENDED;
        break;
#ifdef WANT_SCRIPT_FLAGS
      case 'e':
        ed->exec->opt |= FSCRIPT | SCRIPTED;
        if (append_script_expression (optarg, ed) < 0)
              script_die (3, ed);
        break;
      case 'f':                 /* Read commands from file arg. */
        ed->exec->opt |= FSCRIPT | SCRIPTED;
        if (append_script_file (optarg, ed) < 0)
          script_die (3, ed);
        break;
#endif
      case 'G':                 /* Compatibility mode. */
        ed->exec->opt |= TRADITIONAL;
        break;
      case 'h':                 /* Display help, then exit. */
        ed->exec->opt |= PRINT_HELP;
        ed_usage (0, ed);
        break;
#ifdef WANT_SCRIPT_FLAGS
      case 'i':
        ed->exec->opt |= IN_PLACE;
        if (optarg != NULL)
          {
            if ((len = strlen (optarg)) >= PATH_MAX - 1
                || !(ed->file->suffix = realloc (ed->file->suffix, len + 1)))
              script_die (3, ed);
            strcpy (ed->file->suffix, optarg);
          }
        break;
#endif
      case 'p':                 /* Set ed command prompt. */
        ed->exec->opt |= PROMPT;

        if ((cs = getenv ("COLUMNS")) == NULL || strcmp (cs, "") == 0
            || (l = strtol (cs, NULL, 10)) == 0)
          l = WS_COL;

        if ((len = strlen (optarg)) >= l - 1
            || !(ed->exec->prompt = realloc (ed->exec->prompt, len + 1)))
          script_die (3, ed);

        /* Save prompt from longjmp(3). */
        strcpy (ed->exec->prompt, optarg);
        break;
      case 'R':                 /* Filter ANSI SGR (color) codes. */
        ed->exec->opt |= ANSI_COLOR;
        break;
      case 'r':                 /* Enable extended regular expressions. */
        ed->exec->opt |= REGEX_EXTENDED;
        break;
      case 's':                 /* Suppress interactive diagnostics. */
        ed->exec->opt |= SCRIPTED;
        break;
      case 'V':                 /* Print version, then exit. */
        ed->exec->opt |= PRINT_VERSION;
        ed_usage (0, ed);
        break;
      case 'v':                 /* Verbose mode. */
        ed->exec->opt |= VERBOSE;
        break;
      case 'w':                 /* Open file in write-only mode. */
        ed->exec->opt |=  WRITE_ONLY;
        break;
#ifdef WANT_ED_ENCRYPTION
      case 'x':
        if (get_des_keyword (ed) < 0)
          script_die (3, ed);
        break;
#endif
      }
  argv += optind;
  argc -= optind;
  if (argc && !strcmp (*argv, "-"))
    {
      ed->exec->opt |= SCRIPTED;
      if (argc > 1)
        {
          optind = 0;
          goto top;
        }
      ++argv;
      --argc;
    }
  if (argc_prev)
    {
      argc = argc_prev;
      argv = argv_prev;
      argc_prev = 0;
      optind = 0;
      goto top;
    }

#ifdef WANT_ADDRESS_ARGUMENTS
  argv = collect_address_args (&argc, argv, ed);
#endif

#ifdef WANT_SCRIPT_FLAGS

  /* Rewind internal script, if any. */
  if (ed->exec->fp
      && (fflush (ed->exec->fp) == EOF
          || FSEEK (ed->exec->fp, 0L, SEEK_SET) == -1))
    {
      fprintf (stderr, "%s\n", strerror(errno));
      ed->exec->err = _("File seek error");
      script_die (3, ed);
    }

  if ((ed->exec->opt & IN_PLACE) && ! (ed->exec->opt & FSCRIPT))
    {
      fprintf (stderr, "%s: %s\n",
               ed->exec->opt & RESTRICTED ? "red" : "ed",
               _("Option '-i' requires '-e' or '-f'"));
      ed_usage (1, ed);
    }
#endif  /* WANT_SCRIPT_FLAGS */

#ifdef HAVE_LOCALE_H

  /* Native locale unavailable. */
  if (!setlocale (LC_ALL, ""))
    fprintf (stderr, "Warning: Locale unknown\n");
#endif

#ifdef ENABLE_NLS
  if (!bindtextdomain (PACKAGE, LOCALEDIR) || !textdomain (PACKAGE))
    fprintf (stderr, "%s\n", strerror(errno));
#endif

#ifdef WANT_FILE_GLOB

  /*
   * If file globbing is enabled and ed is given multiple file args,
   * then print name of each as it becomes current, i.e., opened for
   * editing. For SUSv4 compatibility, suppress printing the name if
   * only one file arg is given.
   */
  if (argc > 0 && ed->exec->opt & VERBOSE)
    ed->exec->opt |= PRINT_FIRST_FILE;
#endif

  if (!(signal_status = init_signal_handler (ed)))
    {
#ifdef SIGWINCH

      /*
       * Signals are fatal before final initialization, i.e., before
       * calling one_time_init (), but SIGWINCH only sets current
       * window dimensions.
       *
       * Initial window dimensions are overriden if environment
       * variables `LINES'/`COLUMNS' set, per one_time_init () below.
       */
      signal_handler (SIGWINCH);
#endif
    }

  /* Initialize editor buffer and globals. */
  if ((status = one_time_init (argc, argv, ed)) < 0)
    goto error;

  /* Destination of LONGJMP () on signal SIGINT. */
  if ((status = SETJMP (env)))
    {
/*
 *  XXX: #ifdef here disables ed signal handlers. In particular,
 *       SIGINT causes ed to exit. Workaround by defining dummy
 *       unwind_stack_frame () in register.c.
 */
/*
 * #ifdef WANT_ED_MACRO
 */
      /* Unwind stack frame on interrupt. */
      status = unwind_stack_frame (ERR, ed);
/*
 * #endif
 */
      ed->exec->err = _("Interrupted");
      goto error;
    }

  /* Enable signal handlers.  */
  activate_signals ();

 next:

  /* If more files in list, edit next per status. */
  if (ed->file->list->gl_pathc)
    {
      /* Open file. */
      if ((status = next_edit (status, ed)) < 0
          || (status = exec_command (ed)) < 0)
        goto error;
      ed->exec->opt &= ~PRINT_FIRST_FILE;

#ifdef WANT_SCRIPT_FLAGS
      if (ed->exec->opt & IN_PLACE && ed->file->suffix
          && (status = save_edit (0, ed)) < 0)
        goto error;
#endif

      if (signal_status < 0)
        {
          status = signal_status;
          signal_status = 0;
          goto error;
        }
    }

  /*  Main command loop. */
  for (;;)
    {
      if (ed->exec->opt & PROMPT /* && !(ed->exec->opt & SCRIPTED) */)
        {
          fputs (ed->exec->prompt, stdout);
          fflush (stdout);
        }

      if (ed->exec->address)
        {
          ed->input = ed->exec->address;
          ed->exec->address = strchr (ed->exec->address, '\n') + 1;
          if (*ed->exec->address == '\0')
            ed->exec->address = NULL;
        }

      /* End of input. */
      else if (!(ed->input = get_stdin_line (&len, ed)))
        {
          status = (!feof (stdin)
                    ? ERR : (ed->state->is_modified
                             && !(ed->exec->opt & SCRIPTED)
                             ? EMOD : EOF));
          clearerr (stdin);
          goto error;
        }

      /* Input not newline ('\n') terminated. */
      else if (*(ed->input + len - 1) != '\n')
        {
          ed->exec->err = _("End-of-file unexpected");
          status = ERR;
          clearerr (stdin);
          goto error;
        }
      ++ed->exec->line_no;

      /* Execute one command. */
      if ((status = address_range (ed)) >= 0
          && (status = exec_command (ed)) >= 0)

        /* ... */
        if (!(ed->display->dio_f = status)
            || (status = display_lines (ed->state->dot,
                                        ed->state->dot, ed)) >= 0)
          continue;

    error:
      switch (status)
        {
#ifdef WANT_SAFE_WRITE
        case EOF_GLOB:
#endif
        case EOF_NEXT:
        case EOF_PREV:
#ifdef WANT_FILE_GLOB
          if (!(ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)))
            {
# ifdef WANT_SCRIPT_FLAGS
              if (ed->exec->opt & IN_PLACE
                  && ((status = save_edit (status, ed)) < 0))
                goto error;
# endif

              goto next;
            }
#endif  /* WANT_FILE_GLOB */
          /* FALLTHROUGH */
        case EOF:
#ifdef WANT_SCRIPT_FLAGS
          if (!(ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
              && ed->exec->opt & IN_PLACE
              && (status = save_edit (status, ed)) < 0)
            goto error;
#endif

          /* Command-line files still available and stdin seekable... */
          if (ed->file->list->gl_offs && ed->file->list->gl_pathc > 1
              && FSEEK(stdin, 0L, SEEK_SET) != -1)
            {
              status = EOF_NEXT;
              goto next;
            }
          quit (ed->exec->status, ed);
        case EMOD:
          ed->exec->err = _("WARNING: Buffer modified since last write");
          ed->state->is_modified = 0;
          puts ("?");
          if (!(ed->exec->opt & EXIT_ON_ERROR))
            printf (ed->exec->opt & VERBOSE ? "%s\n" : "", ed->exec->err);
          else
            script_die (2, ed);
          break;
        case FATAL:
          script_die (3, ed);
        default:
          puts ("?");
          if (!(ed->exec->opt & EXIT_ON_ERROR))
            printf (ed->exec->opt & VERBOSE ? "%s\n" : "", ed->exec->err);
          else
            script_die (4, ed);

          /* Re-enable stdout after interrupt or error. */
          ed->display->hidden = 0;
          break;
        }

      /* Only reset exit_status explicitly. */
      if (status)
        ed->exec->status = -status;
    }
  /* NOTREACHED */
}


#ifdef WANT_ADDRESS_ARGUMENTS
/*
 * collect_address_args: Handle command-line arguments of the form `+n',
 *   `+/' and `+?'.
 */
static char **
collect_address_args (int *argc_p, char **argv, ed_buffer_t *ed)
{
  static char *argv_new = NULL;    /* argument vector buffer */
  static size_t argv_new_size = 0; /* argv_new buffer size */

  int argc_new = 0;

  if (*argc_p == 0)
    return argv;

  /* Scan for arguments of the form `+n', `+/' or `+?'. */
  for (int i = 0; i < *argc_p; ++i)
    {
      if (argv[i][0] != '+')
        {
          REALLOC_THROW (argv_new, argv_new_size,
                         (++argc_new + 1) * sizeof (char *), NULL, ed);
          *((char **) argv_new + argc_new - 1) = argv[i];
          continue;
        }

      size_t address = 0;
      char *endp = argv[i] + 1;
      char *regexp = NULL;
      char *regexp_parsed = NULL;
      size_t regexp_size = 0;
      size_t regexp_len = 0;
      size_t argv_len = strlen (argv[i] + 2);
      int delim = argv[i][1];

      switch (argv[i][1])
        {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          STRTOUL_THROW(address, endp, &endp, NULL);

          /* Reject, e.g., `+33a'. */
          if (*endp != '\0')
            {
              REALLOC_THROW (argv_new, argv_new_size,
                             (++argc_new + 1) * sizeof (char *), NULL, ed);
              *((char **) argv_new + argc_new - 1) = argv[i];
              continue;
            }
          else if (append_address_command (argv[i] + 1, ed) < 0)
            script_die (3, ed);
          break;
        case '/':
        case '?':
          REALLOC_THROW (regexp, regexp_size, argv_len + 2, NULL, ed);
          strncpy (regexp, argv[i] + 2, argv_len);
          regexp[argv_len] = '\n';
          regexp[argv_len + 1] = '\0';
          ed->input = regexp;
          if ((regexp_parsed =
               regular_expression (delim, &regexp_len, ed)) == NULL)
            script_die (3, ed);
          free (regexp);
          if (regexp_len != argv_len)
            {
              REALLOC_THROW (argv_new, argv_new_size,
                             (++argc_new + 1) * sizeof (char *), NULL, ed);
              *((char **) argv_new + argc_new - 1) = argv[i];
              continue;
            }
          else if (append_address_command (argv[i] + 1, ed) < 0)
            script_die (3, ed);
          break;
        default:
          REALLOC_THROW (argv_new, argv_new_size,
                         (++argc_new + 1) * sizeof (char *), NULL, ed);
          *((char **) argv_new + argc_new - 1) = argv[i];
          break;
        }
    }

  if (!argv_new)
    REALLOC_THROW (argv_new, argv_new_size,
                   sizeof (char *), NULL, ed);

  *((char **) argv_new + argc_new) = NULL;
  *argc_p = argc_new;
  return (char **) argv_new;
}
#endif  /* WANT_ADDRESS_ARGUMENTS */

/*
 * next_edit: Construct ed command per status, pointed to by
 *   ed->input. Return status.
 */
static int
next_edit (int status, ed_buffer_t *ed)
{
  static char *buf = NULL;
  static size_t buf_size = 0;

  size_t len;

  if ((len = strlen (*ed->file->list->gl_pathv))
      >= get_path_max (*ed->file->list->gl_pathv))
    {
      ed->exec->err = _("File name too long");
      return ERR;
    }

  /* Allocate for string `r <filename>\n\0' */
  REALLOC_THROW (buf, buf_size, len + 4, ERR, ed);

#ifdef WANT_FILE_GLOB
  sprintf (ed->input = buf,
# ifdef WANT_SAFE_WRITE
           status == EOF_GLOB ? "~E\n" :
# endif  /* WANT_SAFE_WRITE */
           status == EOF_NEXT ? "~En\n" :
           status == EOF_PREV ? "~Ep\n" :
           "r %s\n",
           *ed->file->list->gl_pathv);
#else
  sprintf (ed->input = buf, "r %s\n", *ed->file->list->gl_pathv);
#endif  /* !WANT_FILE_GLOB */

  ed->exec->region->addrs = 0;
  ed->exec->region->start = 0;
  ed->exec->region->end = 0;
  return 0;
}


/*
 * save_edit: Construct ed command per status, pointed to by
 *   ed->input. Return status.
 */
static int
save_edit (int status, ed_buffer_t *ed)
{
  static char *buf = NULL;
  static size_t buf_size = 0;

  char *name;
  char *suffix;
  size_t len;

  switch (status)
    {
    case EOF:
#ifdef WANT_SAFE_WRITE
    case EOF_GLOB:
#endif
    case EOF_NEXT:
    case EOF_PREV:
      suffix = "";
      break;
    default:
      suffix = ed->file->suffix;
      break;
    }

  name = (*ed->file->list->gl_pathv != NULL
          && **ed->file->list->gl_pathv != '\0'
          && **ed->file->list->gl_pathv != '!'
          ? *ed->file->list->gl_pathv
          : (ed->file->name != NULL
             && *ed->file->name != '\0'
             && *ed->file->name != '!' ? ed->file->name
             : NULL));

  if (!name)
    {
      ed->exec->err = _("File name not set");
      return FATAL;
    }
  else if ((len = strlen (name) + strlen (suffix)) >= get_path_max (name))
    {
      ed->exec->err = _("File name too long");
      return FATAL;
    }

  /* Allocate for string `w <filename>\n\0' */
  REALLOC_THROW (buf, buf_size, len + 4, ERR, ed);

  sprintf (ed->input = buf, "w %s%s\n", name, suffix);

  ed->exec->region->addrs = 0;
  ed->exec->region->start = 0;
  ed->exec->region->end = 0;

  /*
   * if (status == EOF)
   *   ed->exec->opt &= ~IN_PLACE;
   */

  return exec_command (ed);
 }


#ifdef WANT_ED_ENVAR
/*
 * getenv_init_argv: Return argv constructed from value of given
 *   environment variable; argv[0] points to the given variable name.
 */
static char **
getenv_init_argv (const char *s, int *argc, ed_buffer_t *ed)
{
  static char *argv;            /* argument vector buffer */
  static char *env;             /* copy of environment variable */
  static size_t argv_size = 0;  /* argv buffer size */
  static size_t env_size = 0;   /* env buffer size */

  size_t len;
  char sep[] = " \t\n";
  char *u, *v;

  *argc = 0;
  if ((u = getenv (s)) && (len = strlen (u)) > 0)
    {
      REALLOC_THROW (env, env_size, len + 1, NULL, ed);
      strcpy (env, u);
      for (v = strtok (env, sep); v; v = strtok (NULL, sep))
        {
          /* ARG_MAX is POSIX limit on execve(2). */
          if (*argc >= ARG_MAX)
            {
              ed->exec->err = _("Argument list full");
              return NULL;
            }
          REALLOC_THROW (argv, argv_size,
                         (*argc + 2) * sizeof (char *), NULL, ed);
          *((char **) argv + ++*argc) = v;
        }
      *((char **) argv + ++*argc) = NULL;
      *((char **) argv) = (char *) s;
    }
  return (char **) (*argc ? argv : 0);
}
#endif /* WANT_ED_ENVAR */


#ifdef WANT_ADDRESS_ARGUMENTS
/* append_address_command: Append address command to address buffer. */
static int
append_address_command (const char *s, ed_buffer_t *ed)
{
  static size_t address_size = 0;
  static size_t previous_len = 0;

  size_t len = strlen(s);

  REALLOC_THROW (ed->exec->address, address_size,
                        previous_len + len + 2, ERR, ed);
  strncpy (ed->exec->address + previous_len, s, len);
  previous_len += len + 1;
  ed->exec->address[previous_len - 1] = '\n';
  ed->exec->address[previous_len] = '\0';
  return 0;
}
#endif  /* WANT_ADDRESS_ARGUMENTS */

#ifdef WANT_SCRIPT_FLAGS
/* append_script_expression: Append expression to script. */
int
append_script_expression (const char *s, ed_buffer_t *ed)
{
  size_t fp_size = 0;
  size_t n;
  int status;

  if (!ed->exec->fp)
    {
      if ((status = init_script_buffer (ed)) < 0)
        return status;
      else if (!isatty (0)
               && (status = append_stream (ed->exec->fp,
                                           stdin, &fp_size, ed)) < 0)
        return status;
    }
  else if (FSEEK (ed->exec->fp, 0L, SEEK_END) == -1)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (ed->exec->fp);
      return ERR;
    }

  if ((n = strlen (s)) > 0)
    {
      if (fwrite (s, 1, n, ed->exec->fp) != n
          || (s[n - 1] != '\n' && fwrite ("\n", 1, 1, ed->exec->fp) != 1))
        {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("File write error");
          clearerr (ed->exec->fp);
          return ERR;
        }
    }
  else if (fwrite ("\n", 1, 1, ed->exec->fp) != 1)
    {
          fprintf (stderr, "%s\n", strerror (errno));
          ed->exec->err = _("File write error");
          clearerr (ed->exec->fp);
          return ERR;
    }
  return 0;
}


/* append_script_file: Append file contents to script. */
int
append_script_file (char *fn, ed_buffer_t *ed)
{
  FILE *fp;
  char *filename;
  size_t fp_size = 0;
  size_t len;
  int status;

  filename = strlen (fn) == 1 && *fn == '-' ? STDIN : fn;
  len = strlen (filename);

  /* Conditionally save filename of script. */
  if (len && !ed->exec->script_pathname)
      {
        REALLOC_THROW (ed->exec->script_pathname,
                       ed->exec->script_pathname_size, len + 1, ERR, ed);
        strcpy (ed->exec->script_pathname, filename);
      }

  if (!(fp = fopen (filename, "r")))
    {
      fprintf (stderr, "%s: %s\n", filename, strerror (errno));
      ed->exec->err = _("File open error");
      return ERR;
    }

  if (!ed->exec->fp)
    {
      if ((status = init_script_buffer (ed)) < 0)
        return status;
      else if (!isatty (0)
               && (status = append_stream (ed->exec->fp,
                                           stdin, &fp_size, ed)) < 0)
        return status;
    }
  else if (FSEEK (ed->exec->fp, 0L, SEEK_END) == -1)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (ed->exec->fp);
      return ERR;
    }

  if ((status = append_stream (ed->exec->fp, fp, &fp_size, ed)) < 0)
    return status;
  fclose (fp);
  return 0;
}
#endif  /* WANT_SCRIPT_FLAGS */


/* ed_usage: Print ed usage. */
static void
ed_usage (int status, ed_buffer_t *ed)
{
  extern char version_string[]; /* From version.c */
  char *err_usage =
#ifdef WANT_SCRIPT_FLAGS
# ifdef WANT_FILE_GLOB
    _("Usage: %s [-] [-EGhiRrsVvwx] [-e COMMAND] [-f SCRIPT] [-p PROMPT] [FILE...]\n");
# else
    _("Usage: %s [-] [-EGhiRrsVvwx] [-e COMMAND] [-f SCRIPT] [-p PROMPT] [FILE]\n");
# endif /* !WANT_FILE_GLOB */
#else
    _("Usage: %s [-] [-EGhRrsVvwx] [-p PROMPT] [FILE]\n");
#endif  /* !WANT_SCRIPT_FLAGS */

  if (status)
    printf (err_usage, ed->exec->opt & RESTRICTED ? "red" : "ed");
  else if (ed->exec->opt & PRINT_VERSION)
    printf ("ed %s\n%s", version_string, CONFIGURE_ARGS);
  else if (ed->exec->opt & PRINT_HELP)
    {
#ifdef WANT_FILE_GLOB
      printf (_("Usage: %s [OPTION...] [FILE...]\n"),
              (ed->exec->opt & RESTRICTED ? "red" : "ed"));
#else
      printf (_("Usage: %s [OPTION...] [FILE]\n"),
              (ed->exec->opt & RESTRICTED ? "red" : "ed"));
#endif  /* !WANT_FILE_GLOB */
  /* TODO: Implement configured-feature-based help. Take all-or-none
     approach for now. */
#ifdef WANT_SCRIPT_FLAGS
      printf (_("Options:\n\
  -E, --regexp-extended     Enable extended regular expression syntax.\n\
  -e, --expression=COMMAND  Add COMMAND to scripted input - implies `-s'.\n\
  -f, --file=SCRIPT         Read commands from file SCRIPT - implies `-s'.\n\
  -G, --traditional         Enable backward compatibility.\n\
  -h, --help                Display (this) help, then exit.\n\
  -i, --in-place[=SUFFIX]   Write file before closing and optionally backup.\n\
  -p, --prompt=STRING       Prompt for commands with STRING.\n\
  -R, --ansi-color          Enable support for ANSI color codes.\n\
  -r, --regexp-extended     Enable extended regular expression syntax.\n\
  -s, --script              Suppress interactive diagnostics.\n\
  -v, --verbose             Enable verbose diagnostics.\n\
  -V, --version             Display version information, then exit.\n\
  -w, --write               Enable writing to process substitution.\n\
  -x, --crypt               Enable I/O encryption.\n\
\n\
If FILE is given, read it for editing.  From within ed, run:\n\
  !info ed-intro RET\n\
for a tutorial introduction to ed.\n\
\n\
Please submit issues or pull requests to: <https://github.com/slewsys/ed>\n"));
#else
      printf (_("Options:\n\
  -E, --regexp-extended     Enable extended regular expression syntax.\n\
  -G, --traditional         Enable backward compatibility.\n\
  -h, --help                Dispaly (this) help, then exit.\n\
  -p, --prompt=STRING       Prompt for commands with STRING.\n\
  -R, --ansi-color          Enable support for ANSI color codes.\n\
  -r, --regexp-extended     Enable extended regular expression syntax.\n\
  -s, --script              Suppress interactive diagnostics.\n\
  -v, --verbose             Enable verbose diagnostics.\n\
  -V, --version             Display version information, then exit.\n\
  -w, --write               Enable writing to process substitution.\n\
  -x, --crypt               Enable I/O encryption.\n\
\n\
If FILE is given, read it for editing.  From within ed, run:\n\
  !info ed-intro RET\n\
for a tutorial introduction to ed.\n\
\n\
Please submit issues or pull requests to: <https://github.com/slewsys/ed>\n"));
#endif  /* !WANT_SCRIPT_FLAGS */
    }
  exit (status);
}


/* script_die: Print an error message to stderr, then exit. */
static void
script_die (int status, ed_buffer_t *ed)
{
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)
      || !ed->exec->script_pathname)
    fprintf (stderr, (ed->exec->opt & VERBOSE
                      ? _("script, line %" OFF_T_FORMAT_STRING ": %s\n") : ""),
             ed->exec->line_no, ed->exec->err);
  else
    fprintf (stderr, (ed->exec->opt & VERBOSE
                      ? "%s:%" OFF_T_FORMAT_STRING ": %s\n" : ""),
             ed->exec->script_pathname, ed->exec->line_no, ed->exec->err);
  quit (status, ed);
}
