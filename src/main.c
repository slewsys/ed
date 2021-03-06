/* main.c: Entry point for the ed line editor.
 *
 *  Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

#ifndef lint
char *copyright =
  "@(#) Copyright © 1993-2018 Andrew L. Moore, SlewSys Research.\n";
#endif  /* not lint */

#include <pwd.h>

#include "ed.h"
#include "argmax.h"
#include "getopt.h"

#define STDIN "/dev/stdin"

#ifdef HAVE_SIGLONGJMP
sigjmp_buf env;
#else
jmp_buf env;
#endif  /* !HAVE_SIGLONGJMP */

/* Global declaration - required by interrupt handlers. */
ed_buffer_t *ed;

/* Static function declarations. */
static void ed_usage __P ((int, ed_buffer_t *));

#ifdef WANT_ED_ENVAR
static char **getenv_init_argv __P ((const char *, int *, ed_buffer_t *));
#endif

static int next_edit __P ((int, ed_buffer_t *));
static int save_edit __P ((int, ed_buffer_t *));
static void script_die __P ((int, ed_buffer_t *));

/* ed: line editor */
int
main (int argc, char **argv)
{
  struct option long_options[15] =
    {
      {"ansi-color", no_argument, NULL, 'R'},

#ifdef WANT_SCRIPT_FLAGS
      {"expression", required_argument, NULL, 'e'},
      {"file", required_argument, NULL, 'f'},
#endif

      {"help", no_argument, NULL, 'h'},

#ifdef WANT_ED_ENCRYPTION
      {"crypt", no_argument, NULL, 'x'},
#endif

      {"in-place", optional_argument, NULL, 'i'},
      {"prompt", required_argument, NULL, 'p'},
      {"quiet", no_argument, NULL, 's'},           /* Deprecated long option */
      {"regexp-extended", no_argument, NULL, 'E'}, /* BSD extended regexp */
      {"regexp-extended", no_argument, NULL, 'r'}, /* GNU extended regexp */
      {"scripted", no_argument, NULL, 's'},
      {"traditional", no_argument, NULL, 'G'},
      {"verbose", no_argument, NULL, 'v'},
      {"version", no_argument, NULL, 'V'},
      {0, 0, 0, 0},
    };
  char **argv_new;
  char **argv_prev = NULL;
  char wbuf[3];
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
  while ((c = getopt_long (argc, argv, "E"
#ifdef WANT_SCRIPT_FLAGS
                           "e:f:"
#endif
                           "Ghi::p:RrsVv"
#ifdef WANT_ED_ENCRYPTION
                           "x"
#endif
                           , long_options, NULL)) != -1)
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

  /* In case of option `-f' or `-e', rewind internal script file. */
  if (ed->exec->fp
      && (fflush (ed->exec->fp) == EOF
          || FSEEK (ed->exec->fp, 0L, SEEK_SET) == -1))
    {
      fprintf (stderr, "%s\n", strerror(errno));
      ed->exec->err = _("File seek error");
      script_die (3, ed);
    }

#ifdef HAVE_LOCALE_H

  /* Native locale unavailable. */
  if (!setlocale (LC_ALL, ""))
    fprintf (stderr, "Warning: Locale unknown\n");
#endif

#ifdef ENABLE_NLS
  if (!bindtextdomain (PACKAGE, LOCALEDIR) ||
      !textdomain (PACKAGE))
    fprintf (stderr, "%s\n", strerror(errno));
#endif

#ifdef WANT_FILE_GLOB

  /*
   * If file globbing is enabled and ed is given multiple file args,
   * then print name of each as it becomes current, i.e., opened for
   * editing. For SUSv4 compatibility, suppress printing the name if
   * only one file arg is given.
   */
  if (argc > 0)
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
 *       unwind_stack_frame ().
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

      if (ed->exec->opt & IN_PLACE && ed->file->suffix
          && (status = save_edit (0, ed)) < 0)
        goto error;

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

      /* End of input. */
      if (!(ed->input = get_stdin_line (&len, ed)))
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
      ed->exec->global = 0;

      /* Execute one command. */
      if ((status = address_range (ed)) >= 0
          && (status = exec_command (ed)) >= 0)

        /* ... */
        if (!(ed->display->io_f = status)
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
              if (ed->exec->opt & IN_PLACE
                  && ((status = save_edit (status, ed)) < 0))
                goto error;

              goto next;
            }
#endif
          /* FALLTHROUGH */
        case EOF:
          if (!(ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
              && ed->exec->opt & IN_PLACE
              && (status = save_edit (status, ed)) < 0)
            goto error;

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
           status == EOF_NEXT ? "En\n" :
           status == EOF_PREV ? "Ep\n" :
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

  if ((len = strlen (*ed->file->list->gl_pathv) + strlen (suffix))
      >= get_path_max (*ed->file->list->gl_pathv))
    {
      ed->exec->err = _("File name too long");
      return ERR;
    }

  /* Allocate for string `w <filename>\n\0' */
  REALLOC_THROW (buf, buf_size, len + 4, ERR, ed);

  sprintf (ed->input = buf, "w %s%s\n", *ed->file->list->gl_pathv, suffix);

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


#ifdef WANT_SCRIPT_FLAGS
/* append_script_expression: Append expression to script. */
int
append_script_expression (const char *s, ed_buffer_t *ed)
{
  size_t n;
  int status;

  if (!ed->exec->fp
      && (status = create_disk_buffer (&ed->exec->fp,
                                       &ed->exec->pathname, ed)) < 0)
    return status;

  /* Append string `s' to end of file `ed->exec->pathname'. */
  if (FSEEK (ed->exec->fp, 0L, SEEK_END) == -1)
    {
      fprintf (stderr, "%s: %s\n", ed->exec->pathname, strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (ed->exec->fp);
      return ERR;
    }
  if (((n = strlen (s)) > 0
      && (fwrite (s, 1, n, ed->exec->fp) != n
          || (s[n - 1] != '\n' && fwrite ("\n", 1, 1, ed->exec->fp) != 1)))
      || (n == 0 && fwrite ("\n", 1, 1, ed->exec->fp) != 1))
    {
          fprintf (stderr, "%s: %s\n", ed->exec->pathname, strerror (errno));
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
  char buf[BUFSIZ];
  size_t len;
  size_t n;
  size_t m = 0;
  int status;

  filename = ((len = strlen (fn)) == 1 && *fn == '-' ? STDIN : fn);

  /* Conditionally save filename of script. */
  if (len && !ed->exec->file_script)
      {
        if (!(ed->exec->file_script = (char *) malloc (len + 1)))
          {
            fprintf (stderr, "%s\n", strerror (errno));
            ed->exec->err = _("Memory exhausted");
            return ERR;
          }
        strcpy (ed->exec->file_script, filename);
      }

  /* Open script and create internal script file `ed->exec->pathname'. */
  if (!(fp = fopen (filename, "r")))
    {
      fprintf (stderr, "%s: %s\n", filename, strerror (errno));
      ed->exec->err = _("File open error");
      return ERR;
    }
  if (!ed->exec->fp
      && (status = create_disk_buffer (&ed->exec->fp,
                                       &ed->exec->pathname, ed)) < 0)
    return status;

  /* Append contents of script to end of internal script. */
  if (FSEEK (ed->exec->fp, 0L, SEEK_END) == -1)
    {
      fprintf (stderr, "%s: %s\n", ed->exec->pathname, strerror (errno));
      ed->exec->err = _("File seek error");
      clearerr (ed->exec->fp);
      return ERR;
    }
  while ((n = fread (buf, 1, BUFSIZ, fp)) > 0
         && (m = fwrite (buf, 1, n, ed->exec->fp)) == n)
    ;
  if (feof (fp))
    {
      if (!ferror (ed->exec->fp) && m && buf[m - 1] != '\n')
        m = fwrite ("\n", 1, n = 1, ed->exec->fp);
    }
  else if (ferror (fp))
    {
      fprintf (stderr, "%s: %s\n", filename, strerror (errno));
      ed->exec->err = _("File read error");
      clearerr (fp);
      fclose (fp);
      return ERR;
    }
  fclose (fp);
  if (ferror (ed->exec->fp))
    {
      fprintf (stderr, "%s: %s\n", ed->exec->pathname, strerror (errno));
      ed->exec->err = _("File write error");
      clearerr (ed->exec->fp);
      return ERR;
    }

  return 0;
}
#endif  /* WANT_SCRIPT_FLAGS */


/* ed_usage: Print ed usage. */
static void
ed_usage (int status, ed_buffer_t *ed)
{
  extern char version_string[]; /* From version.c */


  if (status)
    printf (_("Usage: %s [-] [-EGhirsVv] [-e COMMAND] [-f SCRIPT] [-p PROMPT] [FILE]\n"),
            ed->exec->opt & RESTRICTED ? "red" : "ed");
  else if (ed->exec->opt & PRINT_VERSION)
    printf ("ed %s\n%s", version_string, CONFIGURE_ARGS);
  else if (ed->exec->opt & PRINT_HELP)
    {
      printf (_("Usage: %s [OPTION...] [FILE]\n"), (ed->exec->opt & RESTRICTED
                                                    ? "red" : "ed"));
  /* TODO: Implement configured-feature-based help. Take all-or-none
     approach for now. */
#ifdef WANT_SCRIPT_FLAGS
      printf (_("Options:\n\
  -E, --regexp-extended     Enable extended regular expression syntax.\n\
  -e, --expression=COMMAND  Add COMMAND to scripted input - implies `-s'.\n\
  -f, --file=SCRIPT         Read commands from file SCRIPT - implies `-s'.\n\
  -G, --traditional         Enable backward compatibility.\n\
  -h, --help                Dispaly (this) help, then exit.\n\
  -i, --in-place[=SUFFIX]   Write file before closing, with optional backup.\n\
  -p, --prompt=STRING       Prompt for commands with STRING.\n\
  -R, --ansi-color          Enable support for ANSI color codes.\n\
  -r, --regexp-extended     Enable extended regular expression syntax.\n\
  -s, --script              Suppress interactive diagnostics.\n\
  -v, --verbose             Enable verbose error diagnostics.\n\
  -V, --version             Print version information, then exit.\n\
  -x, --crypt               Prompt for encryption key used for subsequent I/O.\n\
\n\
If FILE is given, read it for editing.  From within ed, run:\n\
  !info ed RET m switches RET\n\
to see full documentation of these options.\n\
\n\
Please submit issues or pull requests to: <https://github.com/slewsys/ed>\n"));
#else
      printf (_("Options:\n\
  -E, --regexp-extended     Enable extended regular expression syntax.\n\
  -G, --traditional         Enable backward compatibility.\n\
  -h, --help                Dispaly (this) help, then exit.\n\
  -i, --in-place[=SUFFIX]   Write file before closing, with optional backup.\n\
  -p, --prompt=STRING       Prompt for commands with STRING.\n\
  -R, --ansi-color          Enable support for ANSI color codes.\n\
  -r, --regexp-extended     Enable extended regular expression syntax.\n\
  -s, --script              Suppress interactive diagnostics.\n\
  -v, --verbose             Enable verbose error diagnostics.\n\
  -V, --version             Print version information, then exit.\n\
\n\
If FILE is given, read it for editing.  From within ed, run:\n\
  !info ed RET m switches RET\n\
to see full documentation of these options.\n\
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
      || !ed->exec->file_script)
    fprintf (stderr, (ed->exec->opt & VERBOSE
                      ? _("script, line %" OFF_T_FORMAT_STRING ": %s\n") : ""),
             ed->exec->line_no, ed->exec->err);
  else
    fprintf (stderr, (ed->exec->opt & VERBOSE
                      ? "%s:%" OFF_T_FORMAT_STRING ": %s\n" : ""),
             ed->exec->file_script, ed->exec->line_no, ed->exec->err);
  quit (status, ed);
}
