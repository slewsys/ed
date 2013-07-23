/* main.c: Entry point for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2013-07-12 <alm@slewsys.org>

   This file is part of ed. */

#ifndef lint
char *copyright =
  "@(#) Copyright © 1993-2013 Andrew L. Moore, SlewSys Research.\n";
#endif  /* not lint */

#include <pwd.h>

#include "argmax.h"
#include "getopt.h"
#include "ed.h"

#define STDIN "/dev/stdin"

#ifdef HAVE_SIGLONGJMP
sigjmp_buf env;
#else
jmp_buf env;
#endif  /* !HAVE_SIGLONGJMP */

/* Global declarations */
ed_state_t _ed;          /* command parameters */

/* Static function declarations. */
#ifdef WANT_ED_ENVAR
static char **getenv_init_argv __P ((const char *, int *, ed_state_t *));
#endif
static int next_edit __P ((int, ed_state_t *));
static void script_die __P ((int, ed_state_t *));
static void ed_usage __P ((int, ed_state_t *));

/* ed: line editor */
int
main (argc, argv)
     volatile int argc;
     char **volatile argv;
{
  struct option long_options[15] =
    {
      {"ansi-color", no_argument, NULL, 'R'},
      {"expression", required_argument, NULL, 'e'},
      {"file", required_argument, NULL, 'f'},
      {"help", no_argument, NULL, 'h'},
      {"info", no_argument, NULL, 'i'},
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
  ed_state_t *ed = &_ed;
  char **argv_new;
  char **argv_prev = NULL;
  size_t buf_size = 0;
  size_t len;
  int argc_new;
  int argc_prev = 0;
  int c;
  int status = 0;
  int signal_status = 0;

  if ((len = strlen (*argv)) > 2 && *(*argv + len - 3) == 'r')
    ed->exec.opt |= RESTRICTED;

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
  while ((c = getopt_long (argc, argv, "Ee:f:Ghip:RrsVv",
                           long_options, NULL)) != -1)
    switch (c)
      {
      default:
        ed_usage (2, ed);
      case 0:
        break;
      case 'E':                 /* Enable extended regular expressions. */
        ed->exec.opt |=  REGEX_EXTENDED;
        break;
      case 'e':
        ed->exec.opt |=  SCRIPTED;
        if (append_script_expression (optarg, ed) < 0)
              script_die (3, ed);
        break;
      case 'f':                 /* Read commands from file arg. */
        ed->exec.opt |= SCRIPTED;
        if (append_script_file (optarg, ed) < 0)
          script_die (3, ed);
        break;
      case 'G':                 /* Compatibility mode. */
        ed->exec.opt |= TRADITIONAL;
        break;
      case 'h':                 /* Display help, then exit. */
        ed->exec.opt |= PRINT_HELP;
        ed_usage (0, ed);
        break;
      case 'i':                 /* Display configuration info, then exit. */
        ed->exec.opt |= PRINT_CONFIG;
        ed_usage (0, ed);
        break;
      case 'p':                 /* Set ed command prompt. */
        ed->exec.opt |= PROMPT;

        /* Protect optarg from longjmp(3) if volatile insufficient. */
        if ((len = strlen (optarg)) >= 0)
          {
            if (!(ed->exec.prompt = realloc (ed->exec.prompt, len + 1)))
              script_die (3, ed);
            strcpy (ed->exec.prompt, optarg);
          }
        break;
      case 'R':                 /* Filter ANSI SGR (color) codes. */
        ed->exec.opt |= ANSI_COLOR;
        break;
      case 'r':                 /* Enable extended regular expressions. */
        ed->exec.opt |= REGEX_EXTENDED;
        break;
      case 's':                 /* Suppress interactive diagnostics. */
        ed->exec.opt |= SCRIPTED;
        break;
      case 'V':                 /* Print version, then exit. */
        ed->exec.opt |= PRINT_VERSION;
        ed_usage (0, ed);
        break;
      case 'v':                 /* Verbose mode. */
        ed->exec.opt |= VERBOSE;
        break;
      }
  argv += optind;
  argc -= optind;
  if (argc && **argv == '-')
    {
      ed->exec.opt |= SCRIPTED;
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

#ifdef HAVE_LOCALE_H
  /* Get native locale. */
  if (!setlocale (LC_ALL, ""))
    fprintf (stderr, "Warning: Locale unknown\n");
#endif

#ifdef ENABLE_NLS
  if (!bindtextdomain (PACKAGE, LOCALEDIR) ||
      !textdomain (PACKAGE))
    fprintf (stderr, "%s\n", strerror(errno));
#endif

  if (!(signal_status = init_signal_handler (ed)))
    {
#ifdef SIGWINCH

      /* Signals are fatal before final initialization, i.e., before
         calling one_time_init (), but SIGWINCH only sets current
         window dimensions. */
      signal_handler (SIGWINCH);
#endif
    }

  if ((status = SETJMP (env)))
    {
      ed->exec.err = _("Interrupted");
      status = ERR;

      /* Re-enable stdout. */
      ed->display.off = 0;
      goto error;
    }

  /* Initialize editor buffer and globals. */
  if ((status = one_time_init (argc, argv, ed)) < 0)
    goto error;

#ifdef WANT_FILE_GLOB

  /* If file globbing is enabled and ed is given multiple file args,
     then print name of each as it becomes current, i.e., opened for
     editing. For SUSv3 compatibility, suppress printing the name if
     only one file arg is given. */
  if (argc > 0)
    ed->exec.opt |= PRINT_FIRST_FILE;
#endif

  /* Enable signal handlers.  */
  activate_signals ();

 next:

  /* If more files in list, edit next per status. */
  if (ed->file.list.gl_pathc)
    {
      /* Open file. */
      if ((status = next_edit (status, ed)) < 0
          || (status = exec_command (ed)) < 0)
        goto error;
      ed->exec.opt &= ~PRINT_FIRST_FILE;

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
      if (ed->exec.opt & PROMPT && !(ed->exec.opt & SCRIPTED))
        {
          fputs (ed->exec.prompt, stdout);
          fflush (stdout);
        }

      /* End of input. */
      if (!(ed->stdin = get_stdin_line (&len, ed)))
        {
          status = (!feof (stdin)
                    ? ERR : (ed->buf[0].is_modified
                             && !(ed->exec.opt & SCRIPTED)
                             ? EMOD : EOF));
          clearerr (stdin);
          goto error;
        }

      /* Input not newline ('\n') terminated. */
      else if (*(ed->stdin + len - 1) != '\n')
        {
          ed->exec.err = _("End-of-file unexpected");
          status = ERR;
          clearerr (stdin);
          goto error;
        }
      ++ed->exec.line_no;
      ed->exec.global = 0;

      /* Execute one command. */
      if ((status = address_range (ed)) >= 0
          && (status = exec_command (ed)) >= 0)

        /* ... */
        if (!status
            || (status = display_lines (ed->buf[0].dot,
                                        ed->buf[0].dot, status, ed)) >= 0)
          continue;

    error:
      switch (status)
        {
#ifdef WANT_SAFE_WRITE
        case EOF_GLB:
#endif
        case EOF_NXT:
        case EOF_PRV:
#ifdef WANT_FILE_GLOB
          if (!(ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL)))
            goto next;
#endif
          /* FALLTHROUGH */
        case EOF:
          quit (ed->exec.status, ed);
        case EMOD:
          ed->exec.err = _("WARNING: Buffer modified since last write");
          ed->buf[0].is_modified = 0;
          puts ("?");
          if (!(ed->exec.opt & EXIT_ON_ERROR))
            printf (ed->exec.opt & VERBOSE ? "%s\n" : "", ed->exec.err);
          else
            script_die (2, ed);
          break;
        case FATAL:
          script_die (3, ed);
        default:
          puts ("?");
          if (!(ed->exec.opt & EXIT_ON_ERROR))
            printf (ed->exec.opt & VERBOSE ? "%s\n" : "", ed->exec.err);
          else
            script_die (4, ed);
          ed->display.off = 0;
          break;
        }

      /* Only reset exit_status explicitly. */
      if (status)
        ed->exec.status = -status;
    }
  /* NOTREACHED */
}


/* next_edit: Construct ed command per status, pointed to by ed->stdin.
   Return status. */
static int
next_edit (status, ed)
     int status;
     ed_state_t *ed;
{
  static char *buf = NULL;
  static size_t buf_size = 0;

  size_t len;

  if ((len = strlen (*ed->file.list.gl_pathv)) > SIZE_T_MAX - 4)
    {
      ed->exec.err = _("File name too long");
      return ERR;
    }
  REALLOC_THROW (buf, buf_size, len + 4, ERR, ed);

#ifdef WANT_FILE_GLOB
  sprintf (ed->stdin = buf,
# ifdef WANT_SAFE_WRITE
           status == EOF_GLB ? "~E\n" :
# endif           
           status == EOF_NXT ? "En\n" :
           status == EOF_PRV ? "Ep\n" :
           "r %s\n",
           *ed->file.list.gl_pathv);
#else
  sprintf (ed->stdin = buf, "r %s\n", *ed->file.list.gl_pathv);
#endif  /* !WANT_FILE_GLOB */

  ed->region.addrs = 0;
  ed->region.start = 0;
  ed->region.end = 0;
  return 0;
}


#ifdef WANT_ED_ENVAR
/* getenv_init_argv: Return argv constructed from value of given
   environment variable; argv[0] points to the given variable name. */
static char **
getenv_init_argv (s, argc, ed)
     const char *s;
     int *argc;
     ed_state_t *ed;
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
      /* ARG_MAX is POSIX limit on execve(2). */
      if (len >= ARG_MAX - 2048)
        {
          ed->exec.err = _("Argument list too long");
          return NULL;
        }
      
      REALLOC_THROW (env, env_size, len + 1, NULL, ed);
      strcpy (env, u);
      for (v = strtok (env, sep); v; v = strtok (NULL, sep))
        {
          /* In addition to max length (ARG_MAX), check max number of
             arguments, approximated as (ARG_MAX / 4). */
          if (*argc >= len >> 2)
            {
              ed->exec.err = _("Argument list full");
              return NULL;
            }
          REALLOC_THROW (argv, argv_size,
                         (*argc + 2) * sizeof (char **), NULL, ed);
          *((char **) argv + ++*argc) = v;
        }
      *((char **) argv + ++*argc) = NULL;
      *((char **) argv) = (char *) s;
    }
  return (char **) (*argc ? argv : 0);
}
#endif /* WANT_ED_ENVAR */


/* append_script_expression: Append expression to script. */
int
append_script_expression (s, ed)
     const char *s;
     ed_state_t *ed;
{
  size_t n;
  int status;

  if (!ed->exec.fp
      && (status = create_disk_buffer (&ed->exec.fp,
                                       &ed->exec.pathname, ed)) < 0)
    return status;
  if (fseek (ed->exec.fp, 0L, SEEK_END) == -1)
    {
      fprintf (stderr, "%s: %s\n", ed->exec.pathname, strerror (errno));
      ed->exec.err = _("File seek error");
      return ERR;
    }
  n = strlen (s);
  if (fwrite (s, 1, n, ed->exec.fp) != n
      || s[n - 1] != '\n' && fwrite ("\n", 1, 1, ed->exec.fp) != 1)
    {  
      fprintf (stderr, "%s: %s\n", ed->exec.pathname, strerror (errno));
      ed->exec.err = _("File write error");
      return ERR;
    }
  if (fseek (ed->exec.fp, 0L, SEEK_SET) == -1)
    {
      fprintf (stderr, "%s: %s\n", ed->exec.pathname, strerror (errno));
      ed->exec.err = _("File seek error");
      return ERR;
    }
  return 0;
}


/* append_script_file: Append file contents to script. */
int
append_script_file (fn, ed)
     char *fn;
     ed_state_t *ed;
{
  FILE *fp;
  char *filename;
  char buf[BUFSIZ];
  size_t len;
  size_t n, m;
  int status;

  /* Save filename. */
  filename = ((len = strlen (fn)) == 1 && *fn == '-' ? STDIN : fn);
  if (len && !ed->exec.file_script)
      {
        if (!(ed->exec.file_script = (char *) malloc (len + 1)))
          {
            fprintf (stderr, "%s\n", strerror (errno));
            ed->exec.err = _("Memory exhausted");
            return ERR;
          }
        strcpy (ed->exec.file_script, filename);
      }
  if (!(fp = fopen (filename, "r")))
    {
      fprintf (stderr, "%s: %s\n", filename, strerror (errno));
      ed->exec.err = _("File open error");
      return ERR;
    }
  if (!ed->exec.fp
      && (status = create_disk_buffer (&ed->exec.fp,
                                       &ed->exec.pathname, ed)) < 0)
    return status;
  if (fseek (ed->exec.fp, 0L, SEEK_END) == -1)
    {
      fprintf (stderr, "%s: %s\n", ed->exec.pathname, strerror (errno));
      ed->exec.err = _("File seek error");
      return ERR;
    }
  while ((n = fread (buf, 1, BUFSIZ, fp)) > 0
         && (m = fwrite (buf, 1, n, ed->exec.fp)) == n)
    ;
  if (n < BUFSIZ)
    {
      if (feof (fp) && buf[m - 1] != '\n')
          m = fwrite ("\n", 1, n = 1, ed->exec.fp);
      else if (!feof (fp))
        {
          fprintf (stderr, "%s: %s\n", filename, strerror (errno));
          ed->exec.err = _("File read error");
          return ERR;
        }
    }
  if (n > 0 && m != n)
    {  
      fprintf (stderr, "%s: %s\n", filename, strerror (errno));
      ed->exec.err = _("File write error");
      return ERR;
    }
  fclose (fp);
  if (fseek (ed->exec.fp, 0L, SEEK_SET) == -1)
    {
      fprintf (stderr, "%s: %s\n", ed->exec.pathname, strerror (errno));
      ed->exec.err = _("File seek error");
      return ERR;
    }
  return 0;
}


/* ed_usage: Print ed usage. */
static void
ed_usage (status, ed)
     int status;
     ed_state_t *ed;
{
  extern char version_string[]; /* From version.c */

  if (status)
    printf (_("Usage: %s [-] [-EGhirsVv] [-f SCRIPT] [-p PROMPT] [FILE]\n"),
            ed->exec.opt & RESTRICTED ? "red" : "ed");
  else if (ed->exec.opt & (PRINT_VERSION | PRINT_CONFIG))
    printf ("ed %s\n%s", version_string, ed->exec.opt & PRINT_CONFIG
            ? CONFIGURE_ARGS : "");
  else if (ed->exec.opt & PRINT_HELP)
    {
      printf (_("Usage: %s [OPTION...] [FILE]\n"), (ed->exec.opt & RESTRICTED
                                                    ? "red" : "ed"));
      printf (_("Options:\n\
  -E, --regexp-extended     Enable extended regular expression syntax.\n\
  -e, --expression=COMMAND  Add COMMAND to scripted input; implies `-s'.\n\
  -f, --file=SCRIPT         Read commands from file SCRIPT.\n\
  -G, --traditional         Enable backward compatibility.\n\
  -h, --help                Dispaly (this) help, then exit.\n\
  -i, --info                Dispaly configuration info, then exit.\n\
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
Report bugs to: <bug-ed@gnu.org>.\n"));
    }
  exit (status);
}


/* script_die: Print an error message to stderr, then exit. */
static void
script_die (status, ed)
     int status;
     ed_state_t *ed;
{
  if (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL) || !ed->exec.file_script)
    fprintf (stderr, (ed->exec.opt & VERBOSE
                      ? _("script, line %" OFF_T_FORMAT_STRING ": %s\n") : ""),
             ed->exec.line_no, ed->exec.err);
  else
    fprintf (stderr, (ed->exec.opt & VERBOSE
                      ? "%s:%" OFF_T_FORMAT_STRING ": %s\n" : ""),
             ed->exec.file_script, ed->exec.line_no, ed->exec.err);
  quit (status, ed);
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
