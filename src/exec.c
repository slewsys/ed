/* exec.c: Command switch for the ed line editor.

   Copyright © 1993-2013 Andrew L. Moore, SlewSys Research

   Last modified: 2013-08-07 <alm@slewsys.org>

   This file is part of ed. */

#include "ed.h"


/* COMMAND_SUFFIX: Get command suffix from the command buffer. */
#define COMMAND_SUFFIX(io_f, ed)                                              \
  do                                                                          \
    {                                                                         \
      switch (*(ed)->stdin)                                                   \
        {                                                                     \
        case 'p':                                                             \
          io_f |= PRNT;                                                       \
          break;                                                              \
        case 'l':                                                             \
          io_f |= LIST;                                                       \
          break;                                                              \
        case 'n':                                                             \
          io_f |= NMBR;                                                       \
          break;                                                              \
        default:                                                              \
          if (*(ed)->stdin != '\n')                                           \
            {                                                                 \
              (ed)->exec.err = _("Command suffix unexpected");                \
              return ERR;                                                     \
            }                                                                 \
          break;                                                              \
        }                                                                     \
    }                                                                         \
  while (*(ed)->stdin++ != '\n')


/* FILE_NAME: Get shell command or file name or file glob from the
   command buffer. */
#define FILE_NAME(fn, len, cm, replace, ed)                                   \
  do                                                                          \
    {                                                                         \
      int _subs;                                                              \
      if (*(ed)->stdin == '!')                                                \
        {                                                                     \
          if (!((fn) = shell_command (&(len), &_subs, (ed))))                 \
            {                                                                 \
              /* EOF should always flag an error here. */                     \
              clearerr (stdin);                                               \
              return ERR;                                                     \
            }                                                                 \
          if (_subs && !((ed)->exec.opt & SCRIPTED))                          \
            puts ((fn) + 1);                                                  \
        }                                                                     \
      else if (!((fn) =                                                       \
                 ((ed)->file.is_glob || (cm) == 'n' || (cm) == 'p'            \
                  ? file_glob (&(len), (cm), (replace), (ed))                 \
                  : file_name (&(len), (ed)))))                               \
        {                                                                     \
          /* EOF should always flag an error here. */                         \
          clearerr (stdin);                                                   \
          return ERR;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)


/* DESTINATION_ADDRESS: Get destination address from command buffer. */
#define DESTINATION_ADDRESS(addr, ed)                                         \
  do                                                                          \
    {                                                                         \
      off_t _start = (ed)->region.start;                                      \
      off_t _end = (ed)->region.end;                                          \
      int _status;                                                            \
      if ((_status = address_range (ed)) < 0)                                 \
        return _status;                                                       \
      if ((ed)->exec.opt & (POSIXLY_CORRECT | TRADITIONAL)                    \
          && !(ed)->region.addrs)                                             \
        {                                                                     \
          (ed)->exec.err = _("Destination address required");                 \
          return ERR;                                                         \
        }                                                                     \
      if ((ed)->buf[0].addr_last < (ed)->region.end)                          \
        {                                                                     \
          (ed)->exec.err = _("Address out of range");                         \
          return ERR;                                                         \
        }                                                                     \
      (addr) = (ed)->region.end;                                              \
      (ed)->region.end = _end;                                                \
      (ed)->region.start = _start;                                            \
    }                                                                         \
  while (0)


/* Static function declarations. */
static int exec_one_off __P ((const char *, char *, ed_state_t *ed));
static int is_valid_range __P ((off_t, off_t, ed_state_t *));


/* exec_command: Execute the next command in command buffer; return
   print request, if any. */
int
exec_command (ed)
     ed_state_t *ed;
{
  static int paging = 0;        /* If set, last command was page */
  static char *cmd = NULL;
  static char *suffix = NULL;
  static size_t cmd_size = 0;
  static size_t suffix_size = 0;

  regex_t *lhs  = NULL;         /* Left-hand substitution pattern buffer */
  off_t addr = 0;
  off_t dot = ed->buf[0].dot;
  off_t s_nth = 0;              /* Substitution match offset */
  off_t s_mod = 0;              /* Substitution match modulus */
  off_t size = 0;
  char *fn = NULL;
  size_t len = 0;
  unsigned io_f = 0;            /* I/O flags */
  unsigned s_f = 0;             /* Saved substitution modifier flags */
  unsigned sgpr_f = 0;          /* Short-form substitution modifier flags */
  unsigned  sio_f = 0;          /* Saved substitution I/O flags */
  int c = 0;
  int cx = 0;
  int cy = 0;
  int cz = 0;
  int is_default = 0;
  int status = 0;               /* Return status */

  ed->display.is_paging = paging;
  paging = 0;
  SKIP_WHITESPACE (ed);

  if (ed->file.is_glob = (c = *ed->stdin++) == '~')
    c = *ed->stdin++;
  switch (c)
    {
    case 'a':

      /* Buffer not empty or not `logically' empty - i.e., contains
         more than an empty file. */
      if (!ed->buf[0].is_empty || !ed->buf[0].addr_last)
        {
          COMMAND_SUFFIX (io_f, ed);
          if (!ed->exec.global)
            reset_undo_queue (ed);
          if ((status = append_lines (ed->region.end, ed)) < 0)
            return status;
          break;
        }
      /* FALLTHROUGH */
    case 'c':

      /* As per SUSv3, 2004, 0c => 1c, so 0,0c => 1,1c. */
      ed->region.start += !ed->region.start;
      ed->region.end += !ed->region.end;
      if ((status = is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      COMMAND_SUFFIX (io_f, ed);
      if (!ed->exec.global)
        reset_undo_queue (ed);
      spl1 ();
      if ((status = delete_lines (ed->region.start, ed->region.end, ed)) < 0)
        {
          spl0 ();
          return status;
        }
      spl0 ();
      if ((status = append_lines (ed->buf[0].dot, ed)) < 0)
        return status;
      break;
    case 'd':
      if ((status = is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      COMMAND_SUFFIX (io_f, ed);
      if (!ed->exec.global)
        reset_undo_queue (ed);

      spl1 ();
      if ((status = delete_lines (ed->region.start, ed->region.end, ed)) < 0)
        {
          spl0 ();
          return status;
        }
      if ((addr = INC_MOD (ed->buf[0].dot, ed->buf[0].addr_last)) != 0)
        ed->buf[0].dot = addr;
      if ((ed->buf[0].is_empty = !ed->buf[0].addr_last))
        ed->buf[0].is_binary = 0;
      spl0 ();
      break;
    case 'e':
      if (ed->buf[0].is_modified && !(ed->exec.opt & SCRIPTED))
        return EMOD;
      /* FALLTHROUGH */
    case 'E':
      if (ed->region.addrs)
        {
          ed->exec.err = _("Address unexpected");
          return ERR;
        }
#ifdef WANT_FILE_GLOB
      if ((cy = *ed->stdin) == 'n' || cy == 'p')
        ++ed->stdin;
#endif
      if (!isspace ((unsigned char) *ed->stdin))
        {
          ed->exec.err = _("Command suffix unexpected");
          return ERR;
        }
      SKIP_WHITESPACE (ed);
      cx = *ed->stdin;
      FILE_NAME (fn, len, cy, 1, ed);

      spl1 ();
      if (ed->buf[0].addr_last > 0
          && (status = delete_lines (1, ed->buf[0].addr_last, ed)) < 0)
        {
          spl0 ();
          return status;
        }
      reset_undo_queue (ed);
      reset_global_queue (ed);

      if (reopen_ed_buffer (ed) < 0)
        {
          spl0 ();
          return status;
        }

      init_ed_command (cx != '\n', ed);
      init_ed_buffer (0, &ed->buf[0]);
      init_ed_buffer (-1, &ed->buf[1]);
      spl0 ();

      if (*fn == '!' ||
          (*fn == '\0' && ed->file.name && *ed->file.name == '!'))
        {
          if ((status = read_pipe (*fn == '!' ? fn : ed->file.name,
                                   0, &addr, &size, ed)) < 0)
            return status;
        }
      else
        {
          /* As per SUSv3, file name changes unconditionally. */
          if (*fn != '\0')
            {
              REALLOC_THROW (ed->file.name, ed->file.name_size,
                             len + 1, ERR, ed);
              strcpy (ed->file.name, fn);
              ++is_default;
            }
          else if (ed->file.name && *ed->file.name != '\0')
            ++is_default;

          /* As per SUSv3, file argument is optional. Though not
             mandated by SUSv3, if default file name is not set,
             trivially succeed by reading from `/dev/null'. */
          if ((status = read_file (is_default ? ed->file.name : "/dev/null",
                                   0, &addr, &size, is_default, ed)) < 0)
            return status;
          if (!(ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL | SCRIPTED))
              && access (ed->file.name, W_OK) < 0)
            fprintf (stderr, (ed->exec.opt & VERBOSE
                              ? "%s: Read-only file permission\n" : ""),
                     ed->file.name);
          else if (ed->file.is_glob || cx == '\n')
            printf (ed->exec.opt & SCRIPTED ? "" : "%s\n", fn);
        }
      printf (ed->exec.opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n",
              size);

      /* As per SUSv3, exit_status cannot be reset. We'll do it anyhow ... */
      if (!(ed->exec.opt
            & (POSIXLY_CORRECT|TRADITIONAL|SCRIPTED|EXIT_ON_ERROR)))
        ed->exec.status = 0;
        

      return 0;
    case 'f':
      if (ed->region.addrs)
        {
          ed->exec.err = _("Address unexpected");
          return ERR;
        }
#ifdef WANT_FILE_GLOB
      if ((cy = *ed->stdin) == 'n' || cy == 'p')
        ++ed->stdin;
#endif
      if (!isspace ((unsigned char) *ed->stdin))
        {
          ed->exec.err = _("Command suffix unexpected");
          return ERR;
        }
      SKIP_WHITESPACE (ed);

      /* Don't set the default file name to a shell command when
         reading and writing, but why not allow setting it explicitly?
         NB: To write to a file whose name is generated by a shell command,
         use one of the following idioms:
             w !cat >`filename generator`
         or
             f cat >`prefix generator`
             w !%.suffix
         or
             f !cat >`filename generator`
             w
      */
      if (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL) && *ed->stdin == '!')
        {
          ed->exec.err = _("Invalid redirection");
          return ERR;
        }

      if (*ed->stdin == '\n' && cy != 'n' && cy != 'p')
        ++ed->stdin;
      else
        {
          FILE_NAME (fn, len, cy, 1, ed);
          REALLOC_THROW (ed->file.name, ed->file.name_size, len + 1, ERR, ed);
          strcpy (ed->file.name, fn);
        }
      if (ed->file.is_glob && ed->file.glob.gl_pathc > 0)
        for (cx = 0; cx < ed->file.glob.gl_pathc; ++cx)
          puts (ed->file.glob.gl_pathv[cx]);
      else if (ed->file.name)
        puts (ed->file.name);
      else if (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL))
        {
          ed->exec.err = _("File name not set");
          return ERR;
        }
      return 0;
    case 'G':
    case 'g':
    case 'V':
    case 'v':
      if (ed->exec.global)
        {
          ed->exec.err = _("Recursive global command");
          return ERR;
        }
      if ((status = is_valid_range (1, ed->buf[0].addr_last, ed)) < 0
          || (status = mark_global_nodes (c == 'g' || c == 'G', ed)) < 0)
        return status;
      if (c == 'G' || c == 'V')
        {
          ed->exec.global = GLBI;
          COMMAND_SUFFIX (io_f, ed);
        }
      ed->exec.global |= GLBL;

      if ((status = exec_global (io_f, ed)) < 0)
        return status;
      break;
    case 'H':
      if (ed->region.addrs)
        {
          ed->exec.err = _("Address unexpected");
          return ERR;
        }
      COMMAND_SUFFIX (io_f, ed);

      /* Toggle VERBOSE setting. */
      ed->exec.opt ^= ed->exec.opt & VERBOSE;
      if (ed->exec.opt & VERBOSE && ed->exec.err)
        puts (ed->exec.err);
      break;
    case 'h':

      /* Of all commands, at least `h' should be forgiving, but
         SUSv3, 2004 says otherwise... */
      if (ed->region.addrs)
        {
          ed->exec.err = _("Address unexpected");
          return ERR;
        }
      COMMAND_SUFFIX (io_f, ed);
      puts (ed->exec.err ? ed->exec.err : _("No previous error"));
      break;
    case 'i':

      /* As per SUSv3, 2004, 0i => 1i. */
      ed->region.end += !ed->region.end;
      COMMAND_SUFFIX (io_f, ed);
      if (!ed->exec.global)
        reset_undo_queue (ed);
      if ((status = append_lines (ed->region.end - 1, ed)) < 0)
        return status;

      /* Per SUSv3, 2004, empty insert sets dot to addressed line. */
      if (ed->buf[0].dot == ed->region.end - 1)
        ed->buf[0].dot = ed->region.end;
      break;
    case 'j':

      /* Allow joining an empty buffer provided no addresses are
         specified. */
      if (!(ed->region.addrs || ed->buf[0].addr_last))
        {
          ed->region.start = 0;
          ed->region.end = 0;
        }
      else
        if ((status =
             is_valid_range (ed->buf[0].dot, ed->buf[0].dot + 1, ed)) < 0)
          return status;
      COMMAND_SUFFIX (io_f, ed);
      if (!ed->exec.global)
        reset_undo_queue (ed);
      if (ed->region.start != ed->region.end
          && (status = join_lines (ed->region.start, ed->region.end, ed)) < 0)
        return status;
      break;
    case 'k':
      cx = *ed->stdin++;
      if (ed->region.end == 0)
        {
          ed->exec.err = _("Address out of range");
          return ERR;
        }
      COMMAND_SUFFIX (io_f, ed);
      if ((status = mark_line_node (get_line_node (ed->region.end, ed),
                                    cx, ed)) < 0)
        return status;
      break;
    case 'l':

      /* Allow printing an empty buffer provided no addresses are
         specified. */
      if (!(ed->region.addrs || ed->buf[0].addr_last))
        {
          ed->region.start = 0;
          ed->region.end = 0;
        }
      else if ((status =
                is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      COMMAND_SUFFIX (io_f, ed);
      return display_lines (ed->region.start, ed->region.end, io_f | LIST, ed);
    case 'm':
      if ((status = is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      DESTINATION_ADDRESS (addr, ed);
      if (ed->region.start <= addr && addr < ed->region.end)
        {
          ed->exec.err = _("Invalid destination address");
          return ERR;
        }
      COMMAND_SUFFIX (io_f, ed);
      if (!ed->exec.global)
        reset_undo_queue (ed);
      if ((status =
           move_lines (ed->region.start, ed->region.end, addr, ed)) < 0)
        return status;
      break;
    case 'n':

      /* Allow printing an empty buffer provided no addresses are
         specified. */
      if (!(ed->region.addrs || ed->buf[0].addr_last))
        {
          ed->region.start = 0;
          ed->region.end = 0;
        }
      else if ((status =
                is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      COMMAND_SUFFIX (io_f, ed);
      return display_lines (ed->region.start, ed->region.end, io_f | NMBR, ed);
    case 'P':
      if (ed->region.addrs)
        {
          ed->exec.err = _("Address unexpected");
          return ERR;
        }
      COMMAND_SUFFIX (io_f, ed);

      /* Toggle PROMPT settings. */
      ed->exec.opt ^= ed->exec.opt & PROMPT;
      break;
    case 'p':
      /* Allow printing an empty buffer provided no addresses are
         specified. */
      if (!(ed->region.addrs || ed->buf[0].addr_last))
        {
          ed->region.start = 0;
          ed->region.end = 0;
        }
      else if ((status =
                is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      COMMAND_SUFFIX (io_f, ed);
      return display_lines (ed->region.start, ed->region.end, io_f | PRNT, ed);
    case 'Q':
    case 'q':
      if (ed->region.addrs)
        {
          ed->exec.err = _("Address unexpected");
          return ERR;
        }
      COMMAND_SUFFIX (io_f, ed);
      return (c == 'q' && ed->buf[0].is_modified && !(ed->exec.opt & SCRIPTED)
              ? EMOD : EOF);
    case 'r':
      if (!isspace ((unsigned char) *ed->stdin))
        {
          ed->exec.err = _("Command suffix unexpected");
          return ERR;
        }
      SKIP_WHITESPACE (ed);
      FILE_NAME (fn, len, 0, !ed->file.name, ed);

      spl1 ();
      if (!ed->exec.global)
        reset_undo_queue (ed);

      /* Reading a sequence of files is similar to concatenation,
         except that missing newlines are appended. An empty file, on
         the other hand, should not append a newline unless the buffer
         is empty, in which case the buffer is still (logically)
         empty. Concatenating an empty file and any other file yields
         the other file. */
      cx = ed->buf[0].newline_appended;
      if (ed->buf[0].is_empty && ed->buf[0].addr_last
          && (status = delete_lines (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        {
          spl0 ();
          return status;
        }
      ed->buf[0].newline_appended = cx;
      if (!(ed->region.addrs && ed->buf[0].addr_last))
        ed->region.end = ed->buf[0].addr_last;
      spl0 ();

      if (*fn == '!'
          || (*fn == '\0' && ed->file.name && *ed->file.name == '!'))
        {
          if ((status = read_pipe (*fn == '!' ? fn : ed->file.name,
                                   ed->region.end, &addr, &size, ed)) < 0)
            return status;
        }
      else
        {
          /* Save filename as default iff ed->file.name not set. */
          if (!ed->file.name && *fn != '\0')
            {
              REALLOC_THROW (ed->file.name, ed->file.name_size,
                             len + 1, ERR, ed);
              strcpy (ed->file.name, fn);
              ++is_default;
            }
          else if (*fn == '\0' && ed->file.name && *ed->file.name != '\0')
            ++is_default;

          /* As per SUSv3, file argument is optional. Though not mandated
             by SUSv3, if default file name is not set, trivially
             succeed by reading from `/dev/null'. */
          if ((status =
               read_file ((is_default ? ed->file.name
                           : (*fn != '\0' ? fn : "/dev/null")),
                          ed->region.end, &addr, &size, is_default, ed)) < 0)
            return status;

          if (!(ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL | SCRIPTED))
                && access (ed->file.name, W_OK) < 0)
            fprintf (stderr, (ed->exec.opt & VERBOSE
                              ? "%s: Read-only access\n" : ""), fn);
          else if (ed->file.is_glob)
            printf (ed->exec.opt & SCRIPTED ? "" : "%s\n", fn);

          /* Reading `/dev/null' to the end of a binary file is a special
             case for removing any trailing newline. */
          if (ed->buf[0].is_binary
              && !strcmp ((cz ? ed->file.name
                           : (*fn != '\0' ? fn : "/dev/null")), "/dev/null"))
            ed->buf[0].newline_appended = 1;
        }

      /* If multiple file args on command line, print first one. */
      if (ed->exec.opt & PRINT_FIRST_FILE && ed->file.list.gl_pathc > 1)
        printf (ed->exec.opt & SCRIPTED ? "" : "%s\n", ed->file.name);

      printf (ed->exec.opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n",
              size);
      if (addr && addr != ed->buf[0].addr_last)
        ed->buf[0].is_modified = 1;
      return 0;
    case 's':
      init_substitute (&lhs, &s_f, &s_nth, &s_mod, &sio_f, &ed->subst);
      if ((status = is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0
          || (status = resubstitute (&s_nth, &s_mod, &s_f, &sgpr_f, ed)) < 0
          || (status = substitution_lhs (&lhs, &sgpr_f, ed)) < 0)
        return status;

      /* If repeated substitution, toggle s_f per modifiers. */  
      if (sgpr_f)
        {
          /* Ignore after first iteration of non-interactive global. */
          if (!ed->exec.global || ed->exec.first_pass
              || (ed->exec.global & GLBI))
            {
              if (sgpr_f & TGSG)
                s_f ^= GSUB;
              if (sgpr_f & TGSP)
                {
                  if (!sio_f)
                    sio_f = PRNT;
                  else
                    s_f ^= PRSW;
                }
            }
        }

      /* Get right-hand side (rhs) of substitution command, i.e.,
         substitution template. */
      else if ((status =
                substitution_rhs (&s_nth, &s_mod, &s_f, &sio_f, ed)) < 0)
        return status;
      COMMAND_SUFFIX (sio_f, ed);
      if (!ed->exec.global)
        reset_undo_queue (ed);
      save_substitute (lhs, s_f, s_nth, s_mod, sio_f, &ed->subst);
      if ((status = substitute_lines (ed->region.start, ed->region.end,
                                      lhs, s_nth, s_mod, s_f, ed)) < 0)
        return status;
      if (sio_f && !(s_f & PRSW))
        return display_lines (ed->buf[0].dot, ed->buf[0].dot, sio_f, ed);
      break;
    case 't':
      if ((status = is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      DESTINATION_ADDRESS (addr, ed);
      COMMAND_SUFFIX (io_f, ed);
      if (!ed->exec.global)
        reset_undo_queue (ed);
      if ((status =
           copy_lines (ed->region.start, ed->region.end, addr, ed)) < 0)
        return status;
      break;
    case 'u':
      if (ed->region.addrs)
        {
          ed->exec.err = _("Address unexpected");
          return ERR;
        }
      COMMAND_SUFFIX (io_f, ed);
      if ((status = undo_last_command (ed)) < 0)
        return status;
      break;
    case 'W':
    case 'w':

      /* Allow writing an empty buffer provided no addresses are
         specified. */
      if (!(ed->region.addrs || ed->buf[0].addr_last))
        {
          ed->region.start = 0;
          ed->region.end = 0;
        }
      else if ((status = is_valid_range (1, ed->buf[0].addr_last, ed)) < 0)
        return status;
      if (ed->exec.opt & POSIXLY_CORRECT
          && !isspace ((unsigned char) *ed->stdin))
        {
          ed->exec.err = _("Command suffix unexpected");
          return ERR;
        }
#ifdef WANT_FILE_GLOB
      else if ((cy = *ed->stdin) == 'n' || cy == 'p')
        ++ed->stdin;
#endif
      else if ((cx = *ed->stdin) == 'q')
        {
#ifdef WANT_SAFE_WRITE
          if (ed->file.is_glob)
            {
              ed->exec.err = _("Command suffix unexpected");
              return ERR;
            }
#endif
          ++ed->stdin;
        }
      if (!isspace ((unsigned char) *ed->stdin))
        {
          ed->exec.err = _("Command suffix unexpected");
          return ERR;
        }
      SKIP_WHITESPACE (ed);

      cz = *ed->stdin == '!';
#ifdef WANT_SAFE_WRITE
      FILE_NAME (fn, len, ed->file.is_glob,
                 ed->file.is_glob ? *ed->stdin != '\n' : !ed->file.name, ed);
#else
      FILE_NAME (fn, len, ed->file.is_glob, !ed->file.name, ed);
#endif  /* !WANT_SAFE_WRITE */

      if (*fn == '!'
          || (*fn == '\0' && ed->file.name && *ed->file.name == '!'))
        {
          if ((status = write_pipe (*fn == '!' ? fn : ed->file.name,
                                    ed->region.start, ed->region.end,
                                    &addr, &size, ed)) < 0)
            return status;
        }
      else
        {
#ifdef WANT_SAFE_WRITE
          if (ed->file.is_glob)
            {
              if (ed->file.name)
                ++is_default;
              else
                fn = "";
            }
          else
#endif
            /* Save filename as default iff ed->file.name not set. */
            if (!ed->file.name && *fn != '\0')
              {
                REALLOC_THROW (ed->file.name, ed->file.name_size,
                               len + 1, ERR, ed);
                strcpy (ed->file.name, fn);
                ++is_default;
              }
            else if (*fn == '\0' && ed->file.name && *ed->file.name != '\0')
              ++is_default;

          /* As per SUSv3, file argument is optional. Though not
             mandated by SUSv3, if default file name is not set,
             trivially succeed by writing to `/dev/null'. */
          if ((status =
               write_file ((is_default ? ed->file.name
                            : (*fn != '\0' ? fn : "/dev/null")),
                           is_default, ed->region.start, ed->region.end,
                           &addr, &size, (c == 'W' ? "a" : "w"), ed)) < 0)
            return status;

          /* As per SUSv3, whole buffer must be written to file (not `!')
             to reset buffer modifed state. */
          if (addr == ed->buf[0].addr_last && (is_default || *fn != '\0'))
            {
              ed->buf[0].is_modified = 0;

              /* As per SUSv3, exit_status cannot be reset. We'll do
                 it anyhow ... */
              if (!(ed->exec.opt
                    & (POSIXLY_CORRECT|TRADITIONAL|SCRIPTED|EXIT_ON_ERROR)))
                ed->exec.status = 0;
            }
          if (ed->file.is_glob || cy == 'n' || cy == 'p')
            printf (ed->exec.opt & SCRIPTED ? "" : "%s\n",
                    (is_default ? ed->file.name
                     : (*fn != '\0' ? fn : "/dev/null")));
        }
      printf (ed->exec.opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n",
              size);

      /*
       *                        State Variables
       * Write command          cx    cy    cz
       * =============          ===============
       *   w                    '\n'
       *   w filename           ' '
       *   w !shell-cmd         ' '         '!'
       *   wn                   '\n'  'n'
       *   wn filename          ' '   'n'
       *   wn !shell-cmd        ' '   'n'   '!'
       *   wp                   '\n'  'p'
       *   wp filename          ' '   'p'
       *   wp !shell-cmd        ' '   'p'   '!'
       *   wq                   '\n'  'q'
       *   wq filename          ' '   'q'
       *   wq !shell-cmd        ' '   'q'   '!'
       *   ~w                   '\n'
       *   ~w fileglob          ' '
       *   ~w !shell-cmd        ' '         '!'
       *   ~wq                  '\n'  'q'
       *   ~wq fileglob         ' '   'q'
       *   ~wq !shell-cmd       ' '   'q'   '!'
       */

#ifdef WANT_FILE_GLOB
      return (
# ifdef WANT_SAFE_WRITE
              (ed->file.is_glob || cy == 'n' || cy == 'p' || cx == 'q')
              && ed->buf[0].is_modified && !(ed->exec.opt & SCRIPTED) ? EMOD
              : ed->file.is_glob ? EOF_GLB
              
# else
              (cy == 'n' || cy == 'p' || cx == 'q')
              && ed->buf[0].is_modified && !(ed->exec.opt & SCRIPTED) ? EMOD
# endif  /* !WANT_SAFE_WRITE */
              : cy == 'n' ? EOF_NXT
              : cy == 'p' ? EOF_PRV
              : cx == 'q' ? EOF
              : 0);
#else
      return (cx == 'q'
              ? (ed->buf[0].is_modified && !(ed->exec.opt & SCRIPTED)
                 ? EMOD : EOF) : 0);
#endif  /* !WANT_FILE_GLOB */

      break;
    case '[':
      /* (.)[n - Displays page of `n' lines centered around addressed
         line, where `n' defaults to current window size. The current
         address is set to the last line displayed. A subsequent `['
         command, therefore, scrolls backward one-half page. */
      /* ed->buf[0].dot = ed->display.page_addr; */
      if (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL))
        {
          ed->exec.err = _("Unknown command");
          return ERR;
        }
      if ((status = is_valid_range (ed->display.page_addr,
                                    ed->display.page_addr, ed)) < 0)
        return status;

      /* XXX - Required, so explain why */
      ed->region.addrs = 1;

      /* Half-page scrolling implemenation requires "normalized"
         frame buffer, so load page preceding given address in
         background (display.off = 1), then scroll from there. */
      ed->display.off = 1;
      if ((status = exec_one_off ("Z", ed->stdin, ed)) < 0)
        {
          ed->display.off = 0;
          return status;
        }
      ed->display.off = 0;

      /* At start or end of buffer, so display it. */
      if (!ed->display.is_paging && ed->region.end < ed->buf[0].dot
          || ed->buf[0].dot == ed->buf[0].addr_last)
        return exec_one_off ("Z", ed->stdin, ed);

      /* XXX - Required, so explain why */
      ed->region.addrs = 0;

      /* FALLTHROUGH */
    case ']':
      /* (.)]n - Displays page of `n' lines centered around addressed
         line, where `n' defaults to current window size. The current
         address is set to the last line printed. A subsequent `]'
         command, therefore, scrolls forward one-half page. */
      if (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL))
        {
          ed->exec.err = _("Unknown command");
          return ERR;
        }
      if (c == ']' && (ed->region.addrs || !ed->display.is_paging))
          return exec_one_off ("[", ed->stdin, ed);
      io_f |= ZHFW;

      /* FALLTHROUGH */
    case 'z':                   /* scroll forward */
      if (ed->exec.opt & POSIXLY_CORRECT)
        {
          ed->exec.err = _("Unknown command");
          return ERR;
        }

      /* If address provided, ignore frame buffer status. */
      if (ed->region.addrs)
        {
          ed->display.underflow = 0;
          ed->display.overflow = 0;
        }

      /* is_valid_range () assigns addr to ed->region.end by default. */
      addr = ed->buf[0].dot + !(ed->exec.global || ed->display.underflow);
      if ((status = is_valid_range (ed->region.start = 1, addr, ed)) < 0)
        return status;
      if (isdigit ((unsigned char) *ed->stdin))
        {
          STRTOUL_THROW (len, ed->stdin, &ed->stdin, ERR);

          /* Sanity check window size. */
          if (1 < len && len < ROWS_MAX)
            window_rows = len;
        }
      COMMAND_SUFFIX (io_f, ed);
      ++paging;
      if (c != 'z')
        addr = min (ed->buf[0].addr_last,
                    ed->region.end + (window_rows >> 1) - 1);
      else
        addr = min (ed->buf[0].addr_last,
                    ed->region.end + window_rows - 2);
      io_f |= c == '[' ? ZBWD : ZFWD;
      return display_lines (ed->region.end, addr, io_f, ed);
      break;

    case 'Z':                   /* scroll backward */
      if (ed->exec.opt & (TRADITIONAL | POSIXLY_CORRECT))
        {
          ed->exec.err = _("Unknown command");
          return ERR;
        }
      
      /* Determine address of last line to print. */
      if (ed->region.addrs)
        {
          addr = ed->buf[0].dot - !ed->exec.global;
          ed->display.underflow = 0;
          ed->display.overflow = 0;
        }
      else
        {
          addr = ed->display.is_paging ? ed->display.page_addr : ed->buf[0].dot;
          addr -= !(ed->exec.global || ed->display.overflow);
        }
      if ((status = is_valid_range (ed->region.start = 1, addr, ed)) < 0)
        return status;
      if (isdigit ((unsigned char) *ed->stdin))
        {
          STRTOUL_THROW (len, ed->stdin, &ed->stdin, ERR);

          /* Sanity check window size. */
          if (1 < len && len < ROWS_MAX)
            window_rows = len;
        }
      COMMAND_SUFFIX (io_f, ed);
      ++paging;

      /* Display page preceding either given line or already displayed page.
         Otherwise, display page preceding current address. */
      return display_lines (max (1, (off_t) ed->region.end - window_rows + 2),
                            ed->region.end, io_f | ZBWD, ed);
      break;
    case '=':
      COMMAND_SUFFIX (io_f, ed);
      printf ("%" OFF_T_FORMAT_STRING "\n",
              (ed->region.addrs ? ed->region.end : ed->buf[0].addr_last));
      break;
    case '!':
      if (ed->region.addrs
          && (status = is_valid_range (ed->buf[0].dot, ed->buf[0].dot, ed)) < 0)
        return status;
      --ed->stdin;
      FILE_NAME (fn, len, cx, 0, ed);
      if (!ed->region.addrs)
        {
          /* system(3) blocks SIGCHLD. */
          system (++fn);
          printf (ed->exec.opt & SCRIPTED ? "" : "!\n");
        }
      else
#if defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER)
        if (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL))
#endif
          {
            ed->exec.err = _("Address unexpected");
            return ERR;
          }
#if defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER)
        else
          {
            if (!ed->exec.global)
              reset_undo_queue (ed);
            addr = ed->buf[0].addr_last;
            if ((status =
                 filter_lines (ed->region.start, ed->region.end, ++fn, ed)) < 0)
              return status;
            ed->buf[0].dot = ed->region.end + ed->buf[0].addr_last - addr;
          }
#endif  /* defined HAVE_FORK && defined WANT_EXTERNAL_FILTER */
      return 0;
    case '\n':
      addr = (ed->buf[0].dot + (ed->exec.opt & (POSIXLY_CORRECT | TRADITIONAL)
                                ? 1 : !ed->exec.global));
      if ((status = is_valid_range (ed->region.start = 1, addr, ed)) < 0)
        return status;
      return display_lines (ed->region.end, ed->region.end, 0, ed);
    case '#':
      while (*ed->stdin++ != '\n')
        ;
      return 0;
    default:

      /* Check for out-of-sequence address. */
      ed->exec.err = (next_address (&dot, ed) > 0 ? _("Invalid address")
                      : _("Unknown command"));
      return ERR;
    }

  return io_f;
}

/* exec_one_off: Exec single command. */
static int
exec_one_off (cmd, modifier, ed)
     const char *cmd;
     char *modifier;
     ed_state_t *ed;
{
  static char *input = NULL;
  static off_t input_size = 0;

  char *saved_modifier = modifier;
  int input_len;
  int status;

  /* Allocate for `start,end' + cmd + modifier. */
  if (strlen (cmd) != 1
      || (input_len = 2 * OFF_T_LEN + strlen (modifier) + 3) > SIZE_T_MAX)
    {
      ed->exec.err = _("Command too long");
      return ERR;
    }
  REALLOC_THROW (input, input_size, input_len, ERR, ed);
  if (ed->region.addrs)
    snprintf (input, input_len, "%ld,%ld%s%s", ed->region.start,
              ed->region.end, cmd, modifier);
  else
    snprintf (input, input_len, "%s%s", cmd, modifier);
  ed->stdin = input;
  if ((status = address_range (ed)) < 0
      || (status = exec_command (ed)) < 0)
    return status;
  ed->stdin = saved_modifier;
  return 0;
}

/* is_valid_range: Return status of address range check. */
static int
is_valid_range (from, to, ed)
     off_t from;
     off_t to;
     ed_state_t *ed;
{
  if (!ed->region.addrs)
    {
      ed->region.start = from;
      ed->region.end = to;
    }
  if (ed->region.start < 1 || ed->buf[0].addr_last < ed->region.start
      || ed->region.end < 1 || ed->buf[0].addr_last < ed->region.end)
    {
      ed->exec.err = _("Address out of range");
      return ERR;
    }
  if (ed->region.start > ed->region.end)
    {
      ed->exec.err = _("Invalid address range");
      return ERR;
    }
  return 0;
}


/* get_path_max: Return PATH_MAX for given file name, which may be
   filesystem dependent. */
size_t
get_path_max (ps)
const char *ps;
{
  return PATH_MAX;
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
