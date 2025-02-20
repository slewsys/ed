/* exec.c: Command switch for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"

/* COMMAND_SUFFIX: Get command suffix from the command buffer. */
#define COMMAND_SUFFIX(io_f, ed)                                              \
  do                                                                          \
    {                                                                         \
      switch (*(ed)->input)                                                   \
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
          if (*(ed)->input != '\n')                                           \
            {                                                                 \
              (ed)->exec->err = _("Command suffix unexpected");               \
              return ERR;                                                     \
            }                                                                 \
          break;                                                              \
        }                                                                     \
    }                                                                         \
  while (*(ed)->input++ != '\n')


/*
 * FILE_NAME: Get shell command or file name or file glob from the
 *    command buffer.
 */
#define FILE_NAME(fn, len, cm, replace, uniquely, ed)                         \
  do                                                                          \
    {                                                                         \
      int _subs;                                                              \
      if (*(ed)->input == '!')                                                \
        {                                                                     \
          if (!((fn) = expand_shell_command (&(len), &_subs, (ed))))          \
            {                                                                 \
              /* EOF should always flag an error here. */                     \
              clearerr (stdin);                                               \
              return ERR;                                                     \
            }                                                                 \
          if (_subs)                                                          \
            puts ((fn) + 1);                                                  \
        }                                                                     \
      else if (!((fn) =                                                       \
                 ((ed)->file->is_glob                                         \
                  ? file_glob (&(len), (cm), (replace), (uniquely), (ed))     \
                  : file_name (&(len), (ed)))))                               \
        {                                                                     \
          /* EOF should always flag an error here. */                         \
          clearerr (stdin);                                                   \
          return ERR;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)

#ifdef WANT_ED_REGISTER

/* DESTINATION_ADDRESS: Get destination address from command buffer. */
# define DESTINATION_ADDRESS(addr, ed)                                        \
  do                                                                          \
    {                                                                         \
      off_t _start = (ed)->exec->region->start;                               \
      off_t _end = (ed)->exec->region->end;                                   \
      int _status;                                                            \
                                                                              \
      /* Parse "write register" command variation. */                         \
      if (!((ed)->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))                \
          && *ed->input == '>')                                               \
        {                                                                     \
          ed->core->regbuf->rio_f |= REGISTER_WRITE;                          \
          if ((append = *++ed->input == '>'))                                 \
            ++ed->input;                                                      \
          ed->core->regbuf->write_idx =                                       \
            isdigit (*ed->input) ? *ed->input++ - '0' : REGBUF_MAX - 1;       \
        }                                                                     \
      else                                                                    \
        {                                                                     \
          if ((_status = address_range (ed)) < 0)                             \
            return _status;                                                   \
          else if ((ed)->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)          \
              && !(ed)->exec->region->addrs)                                  \
            {                                                                 \
              (ed)->exec->err = _("Destination address required");            \
              return ERR;                                                     \
            }                                                                 \
          else if ((ed)->state->lines < (ed)->exec->region->end)              \
            {                                                                 \
              (ed)->exec->err = _("Address out of range");                    \
              return ERR;                                                     \
            }                                                                 \
          (addr) = (ed)->exec->region->end;                                   \
          (ed)->exec->region->end = _end;                                     \
          (ed)->exec->region->start = _start;                                 \
        }                                                                     \
    }                                                                         \
  while (0)
#else

/* DESTINATION_ADDRESS: Get destination address from command buffer. */
# define DESTINATION_ADDRESS(addr, ed)                                        \
  do                                                                          \
    {                                                                         \
      off_t _start = (ed)->exec->region->start;                               \
      off_t _end = (ed)->exec->region->end;                                   \
      int _status;                                                            \
                                                                              \
      if ((_status = address_range (ed)) < 0)                                 \
        return _status;                                                       \
      else if ((ed)->exec->opt & (POSIXLY_CORRECT | TRADITIONAL)              \
          && !(ed)->exec->region->addrs)                                      \
        {                                                                     \
          (ed)->exec->err = _("Destination address required");                \
          return ERR;                                                         \
        }                                                                     \
      else if ((ed)->state->lines < (ed)->exec->region->end)                  \
        {                                                                     \
          (ed)->exec->err = _("Address out of range");                        \
          return ERR;                                                         \
        }                                                                     \
      (addr) = (ed)->exec->region->end;                                       \
      (ed)->exec->region->end = _end;                                         \
      (ed)->exec->region->start = _start;                                     \
    }                                                                         \
  while (0)
#endif  /* !WANT_ED_REGISTER */

#ifdef WANT_ED_REGISTER

/* GET_INPUT_REGISTER:  Get index of register for reading. */
# define GET_INPUT_REGISTER(ed)                                               \
  do                                                                          \
    {                                                                         \
      (ed)->core->regbuf->rio_f |= REGISTER_READ;                             \
      (ed)->core->regbuf->read_idx =                                          \
        isdigit (*(ed)->input) ? *(ed)->input++ - '0' : REGBUF_MAX - 1;       \
      SKIP_WHITESPACE (ed);                                                   \
    }                                                                         \
  while (0)
#endif

/* Static function declarations. */
static int address_cmd (ed_buffer_t *);
static int comment_cmd (ed_buffer_t *);
static int E_cmd (ed_buffer_t *);
static int H_cmd (ed_buffer_t *);
static int P_cmd (ed_buffer_t *);
static int Z_cmd (ed_buffer_t *);
static int a_cmd (ed_buffer_t *);
static int c_cmd (ed_buffer_t *);
static int d_cmd (ed_buffer_t *);
static int e_cmd (ed_buffer_t *);
static int f_cmd (ed_buffer_t *);
static int global_cmd (ed_buffer_t *);
static int h_cmd (ed_buffer_t *);
static int i_cmd (ed_buffer_t *);
static int invalid_cmd (ed_buffer_t *);
static int is_valid_range (off_t, off_t, ed_buffer_t *);
static int j_cmd (ed_buffer_t *);
static int k_cmd (ed_buffer_t *);
static int l_cmd (ed_buffer_t *);
static int m_cmd (ed_buffer_t *);

#ifdef WANT_ED_MACRO
static int macro_cmd (ed_buffer_t *);
#endif

static int n_cmd (ed_buffer_t *);
static int newline_cmd (ed_buffer_t *);
static int normalize_frame_buffer (ed_buffer_t *);
static int p_cmd (ed_buffer_t *);
static int q_cmd (ed_buffer_t *);
static int q_cmd (ed_buffer_t *);
static int r_cmd (ed_buffer_t *);
static int s_cmd (ed_buffer_t *);
static int scroll_forward_half (ed_buffer_t *);
static int scroll_backward_half (ed_buffer_t *);
static int shell_cmd (ed_buffer_t *);
static int t_cmd (ed_buffer_t *);
static int u_cmd (ed_buffer_t *);
static int w_cmd (ed_buffer_t *);
static int w_cmd (ed_buffer_t *);

#ifdef WANT_ED_ENCRYPTION
static int x_cmd (ed_buffer_t *);
#endif

static int z_cmd (ed_buffer_t *);

#define ED_KEY_FIRST 0x3c
#define ED_KEY_MASK  0x3f

/* Ed command key map for ASCII characters in the range  [0x3c-0x7b]. */
static const ed_command_t ed_cmd[] =
  {
   invalid_cmd,
   address_cmd,                 /* Bound to key `='. */
   invalid_cmd,
   invalid_cmd,

#ifdef WANT_ED_MACRO
   macro_cmd,                   /* Bound to key `@'. */
#else
   invalid_cmd,
#endif

   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   E_cmd,
   invalid_cmd,
   global_cmd,                  /* Bound to key `G'. */
   H_cmd,
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   P_cmd,
   q_cmd,                       /* Bound to key `Q'. */
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   global_cmd,                  /* Bound to key `V'. */
   w_cmd,                       /* Bound to key `W'. */
   invalid_cmd,
   invalid_cmd,
   Z_cmd,
   scroll_backward_half,        /* Bound to key `['. */
   invalid_cmd,
   scroll_forward_half,          /* Bound to key `]'. */
   invalid_cmd,
   invalid_cmd,
   invalid_cmd,
   a_cmd,
   invalid_cmd,
   c_cmd,
   d_cmd,
   e_cmd,
   f_cmd,
   global_cmd,                  /* Bound to key `g'. */
   h_cmd,
   i_cmd,
   j_cmd,
   k_cmd,
   l_cmd,
   m_cmd,
   n_cmd,
   invalid_cmd,
   p_cmd,
   q_cmd,
   r_cmd,
   s_cmd,
   t_cmd,
   u_cmd,
   global_cmd,                  /* Bound to key `v'. */
   w_cmd,

#ifdef WANT_ED_ENCRYPTION
   x_cmd,
#else
   invalid_cmd,
#endif

   invalid_cmd,
   z_cmd,
   invalid_cmd,
  };

/*
 * exec_command: Execute the next command in command buffer; return
 *    print request, if any.
 */
int
exec_command (ed_buffer_t *ed)
{
  int c = 0;

  ed->file->is_glob = 0;
#ifdef WANT_ED_REGISTER
  ed->core->regbuf->rio_f = 0;
#endif
  ed->display->is_paging = ed->display->paging;
  ed->display->paging = 0;
  ed->display->dio_f = 0;

  /* Get command prefix, if any. */
  SKIP_WHITESPACE (ed);
  switch (*ed->input)
    {
#ifdef WANT_ED_REGISTER
    case '<':
      if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
        return invalid_cmd (ed);
      ++ed->input;
      GET_INPUT_REGISTER (ed);
      break;
#endif  /* WANT_ED_REGISTER */
#ifdef WANT_FILE_GLOB
    case '~':
      if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
        return invalid_cmd (ed);
      ++ed->file->is_glob;
      ++ed->input;
      break;
#endif  /* WANT_FILE_GLOB */
    default:
      break;
    }

  /* Validate command prefix, if any. */
  switch (c = *ed->input++)
    {
    case 'm':
    case 't':
#ifdef WANT_ED_MACRO
    case '@':
#endif
      if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
        return invalid_cmd (ed);
      else if (ed->file->is_glob)
        {
          ed->exec->err = _("Command prefix unexpected");
          return ERR;
        }
      break;
#if WANT_ED_REGISTER
    case 'E':
    case 'e':
    case 'f':
    case 'r':
    case 'w':
    case 'W':
      if (ed->core->regbuf->rio_f)
        {
          ed->exec->err = _("Command prefix unexpected");
          return ERR;
        }
      break;
#endif
    default:
#ifdef WANT_ED_REGISTER
      if (ed->core->regbuf->rio_f || ed->file->is_glob)
#else
      if (ed->file->is_glob)
#endif  /* !WANT_ED_REGISTER */
        {
          ed->exec->err = _("Command prefix unexpected");
          return ERR;
        }
      break;
    }

  /* Execute command. */
  switch (c)
    {
    case '\n':
      return newline_cmd (ed);
    case '!':
      return shell_cmd (ed);
    case '#':
      return comment_cmd (ed);
    default:
      return (c >= ED_KEY_FIRST ? ed_cmd[(c - ED_KEY_FIRST) & ED_KEY_MASK](ed)
              : invalid_cmd (ed));
    }
}

static int
a_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /*
   * Buffer not empty or not `logically' empty - i.e., contains
   * more than an empty file.
   */
  if (!ed->state->is_empty || !ed->state->lines)
    {
      COMMAND_SUFFIX (ed->display->dio_f, ed);
      if (!(ed->exec->global || ed->core->sp))
        reset_undo_queue (ed);
      if ((status = append_lines (ed->exec->region->end, ed)) < 0)
        return status;
      return ed->display->dio_f;
    }
  return c_cmd (ed);
}

static int
c_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /* Per SUSv4, 2013, 0c => 1c, so 0,0c => 1,1c. */
  ed->exec->region->start += !ed->exec->region->start;
  ed->exec->region->end += !ed->exec->region->end;
  if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
    return status;
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);
  spl1 ();
  if ((status = delete_lines (ed->exec->region->start,
                              ed->exec->region->end, ed)) < 0)
    {
      spl0 ();
      return status;
    }
  spl0 ();
  if ((status = append_lines (ed->state->dot, ed)) < 0)
    return status;

  /* Per SUSv4, 2013, empty change sets dot to address after deleted lines. */
  if (ed->state->dot < ed->exec->region->start)
    ed->state->dot = min (ed->state->lines, ed->exec->region->start);
  return ed->display->dio_f;
}

static int
d_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  int status = 0;               /* Return status */

  if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
    return status;

#ifdef WANT_ED_REGISTER_DELETE
  /* Save deleted lines in default register. */
  ed->core->regbuf->rio_f |= REGISTER_WRITE;
  ed->core->regbuf->write_idx = REGBUF_MAX - 1;
#endif
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);
#ifdef WANT_ED_REGISTER_DELETE
  if ((status = append_to_register (ed->exec->region->start,
                                   ed->exec->region->end, 0, ed)) < 0)
    return status;
#endif
  spl1 ();
  if ((status = delete_lines (ed->exec->region->start,
                              ed->exec->region->end, ed)) < 0)
    {
      spl0 ();
      return status;
    }
  if ((addr = INC_MOD (ed->state->dot, ed->state->lines)) != 0)
    ed->state->dot = addr;
  if ((ed->state->is_empty = !ed->state->lines))
    ed->state->is_binary = 0;
  spl0 ();
  return ed->display->dio_f;
}

static int
e_cmd (ed_buffer_t *ed)
{
  if (ed->state->is_modified)
    return EMOD;
  return E_cmd (ed);
}

static int
E_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  off_t size = 0;
  size_t len = 0;
  char *fn = NULL;
  int cx = 0;
  int cy = 0;
  int is_default = 0;
  int status = 0;               /* Return status */

  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
#ifdef WANT_FILE_GLOB
  if (ed->file->is_glob && ((cy = *ed->input) == 'n' || cy == 'p'))
    ++ed->input;
#endif
  if (!isspace ((unsigned char) *ed->input))
    {
      ed->exec->err = _("Command suffix unexpected");
      return ERR;
    }
  SKIP_WHITESPACE (ed);
  cx = *ed->input;
  FILE_NAME (fn, len, cy, 1, 0, ed);

  spl1 ();
  if (ed->state->lines > 0
      && (status = delete_lines (1, ed->state->lines, ed)) < 0)
    {
      spl0 ();
      return status;
    }
  reset_undo_queue (ed);
  reset_global_buffer (ed);
  if (reopen_ed_buffer (ed) < 0)
    {
      spl0 ();
      return status;
    }

  init_ed_command (cx != '\n', ed);
  init_ed_state (0, &ed->state[0]);
  init_ed_state (-1, &ed->state[1]);
  spl0 ();

  if (*fn == '!' || (*fn == '\0' && ed->file->name && *ed->file->name == '!'))
    {
      if ((status = read_pipe (*fn == '!' ? fn : ed->file->name,
                               0, &addr, &size, ed)) < 0)
        return status;
    }
  else
    {
      /* Per SUSv4, file name changes unconditionally. */
      if (*fn != '\0')
        {
          REALLOC_THROW (ed->file->name, ed->file->name_size, len + 1, ERR, ed);
          strcpy (ed->file->name, fn);
          ++is_default;
        }
      else if (ed->file->name && *ed->file->name != '\0')
        ++is_default;

      /*
       * Per SUSv4, file argument is optional. Though not mandated
       * by SUSv4, if default file name is not set, trivially
       * succeed by reading from `/dev/null'.
       */
      if ((status = read_file (is_default ? ed->file->name : "/dev/null",
                               0, &addr, &size, is_default, ed)) < 0)
        return status;
      if (ed->file->is_glob && ed->exec->opt & VERBOSE)
        puts (ed->file->name);
    }
  printf (ed->exec->opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n", size);

  /* Per SUSv4, exit_status cannot be reset. We'll do it anyhow ... */
  if (!(ed->exec->opt
        & (POSIXLY_CORRECT | TRADITIONAL | SCRIPTED | EXIT_ON_ERROR)))
    ed->exec->status = 0;
  return 0;
}

static int
f_cmd (ed_buffer_t *ed)
{
  size_t len = 0;
  char *fn = NULL;
  int cx = 0;
  int cy = 0;
  int i;

  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
#ifdef WANT_FILE_GLOB
  if (ed->file->is_glob && ((cy = *ed->input) == 'n' || cy == 'p'))
    ++ed->input;
#endif
  if (!isspace ((unsigned char) *ed->input))
    {
      ed->exec->err = _("Command suffix unexpected");
      return ERR;
    }
  SKIP_WHITESPACE (ed);

  /*
   * Don't set the default file name to a shell command when
   * reading and writing, but why not allow setting it explicitly?
   * NB: To write to a file whose name is generated by a shell command,
   * use one of the following idioms:
   *     w !cat >`filename generator`
   * or
   *     f cat >`prefix generator`
   *     w !%.suffix
   * or
   *     f !cat >`filename generator`
   *     w
   */
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL) && *ed->input == '!')
    {
      ed->exec->err = _("Invalid redirection");
      return ERR;
    }

  if (*ed->input != '\n' && (cy == 'n' || cy == 'p'))
    {
      ed->exec->err = _("Command suffix unexpected");
      return ERR;
    }
  else if ((cx = *ed->input == '\n' && cy != 'n' && cy != 'p'))
    ++ed->input;
  else
    {
      FILE_NAME (fn, len, cy, 1, 0, ed);
      REALLOC_THROW (ed->file->name, ed->file->name_size, len+1, ERR, ed);
      strcpy (ed->file->name, fn);
    }

  /*
   * ~f            - prints the current file list without changing the default
   *                 file name.
   * ~fn           - changes the default file name to the "next" in the
   *                 current file list and prints the new default file name.
   * ~fp           - changes the default file name to the "previous" in the
   *                 current file list and prints the new default file name.
   * ~f file-glob  - sets the file list and default file name to the first
   *                 in the list, then prints the default file name.
   * f             - prints the default file name.
   * f file        - sets the default file name and prints it.
   */
  if (cx && ed->file->is_glob && ed->file->glob->gl_pathc > 0)
    for (i = 0; i < ed->file->glob->gl_pathc; ++i)
      puts (ed->file->glob->gl_pathv[i]);
  else if (ed->file->name)
    puts (ed->file->name);
  else if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    {
      ed->exec->err = _("File name not set");
      return ERR;
    }
  return 0;
}

static int
global_cmd (ed_buffer_t *ed)
{
  int c = *(ed->input - 1);
  int status = 0;               /* Return status */

  if (ed->exec->global)
    {
      ed->exec->err = _("Recursive global command");
      return ERR;
    }
  if ((status = is_valid_range (1, ed->state->lines, ed)) < 0
      || (status = set_global_lines (c == 'g' || c == 'G', ed)) < 0)
    return status;
  if (c == 'G' || c == 'V')
    {
      ed->exec->global = GLBI;
      COMMAND_SUFFIX (ed->display->dio_f, ed);
    }
  ed->exec->global |= GLBL;
  if ((status = exec_global (ed)) < 0)
    {
      ed->exec->global = 0;
      return status;
    }
  ed->exec->global = 0;
  return c == 'G' || c == 'V' ? (ed->display->dio_f = status) : 0;
}

static int
H_cmd (ed_buffer_t *ed)
{

  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);

  /* Toggle VERBOSE setting. */
  ed->exec->opt ^= VERBOSE;
  if (ed->exec->opt & VERBOSE && ed->exec->err)
    puts (ed->exec->err);
  return ed->display->dio_f;
}

static int
h_cmd (ed_buffer_t *ed)
{
  /*
   * Of all commands, at least `h' should be forgiving, but SUSv4,
   * 2013 says otherwise...
   */
  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    {
      if (ed->exec->err)
        puts (ed->exec->err);
    }
  else
    puts (ed->exec->err ? ed->exec->err : _("No previous error"));
  return ed->display->dio_f;
}

static int
i_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /* Per SUSv4, 2013, 0i => 1i. */
  ed->exec->region->end += !ed->exec->region->end;
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);
  if ((status = append_lines (ed->exec->region->end - 1, ed)) < 0)
    return status;

  /* Per SUSv4, 2013, empty insert sets dot to addressed line. */
  if (ed->state->dot == ed->exec->region->end - 1)
    ed->state->dot = ed->exec->region->end;
  return ed->display->dio_f;
}

static int
j_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /*
   * Allow joining an empty buffer, provided no addresses are
   * specified.
   */
  if (!(ed->exec->region->addrs || ed->state->lines))
    {
      ed->exec->region->start = 0;
      ed->exec->region->end = 0;
    }
  else if ((status =
            is_valid_range (ed->state->dot, ed->state->dot + 1, ed)) < 0)
    return status;
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);
  if (ed->exec->region->start != ed->exec->region->end
      && (status = join_lines (ed->exec->region->start,
                               ed->exec->region->end, ed)) < 0)
    return status;
  return ed->display->dio_f;
}

static int
k_cmd (ed_buffer_t *ed)
{
  int cx = 0;
  int status = 0;               /* Return status */

  cx = *ed->input++;

  /*
   * if (ed->exec->region->end == 0)
   *   {
   *     ed->exec->err = _("Address out of range");
   *     return ERR;
   *   }
   */

  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if ((status =
       mark_line_node (get_line_node (ed->exec->region->end, ed), cx, ed)) < 0)
    return status;
  return ed->display->dio_f;
}

static int
l_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /*
   * Allow printing an empty buffer, provided no addresses are
   * specified.
   */
  if (!(ed->exec->region->addrs || ed->state->lines))
    {
      ed->exec->region->start = 0;
      ed->exec->region->end = 0;
    }
  else if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
    return status;
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    ed->display->dio_f = LIST;
  else
    ed->display->dio_f |= LIST;
  return display_lines (ed->exec->region->start,
                        ed->exec->region->end, ed);
}

static int
m_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  int append = 0;               /* Append to register? */
  int status = 0;               /* Return status */

  if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
    return status;
  DESTINATION_ADDRESS (addr, ed);
  if (ed->exec->region->start <= addr && addr < ed->exec->region->end)
    {
      ed->exec->err = _("Invalid destination address");
      return ERR;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);
#ifdef WANT_ED_REGISTER
  switch (ed->core->regbuf->rio_f)
    {
    case 0:
#endif
      if ((status = move_lines (ed->exec->region->start,
                                ed->exec->region->end, addr, ed)) < 0)
        return status;
#ifdef WANT_ED_REGISTER
      break;
    case REGISTER_READ:
      if ((status = append_from_register (addr, ed)) < 0
          || (status =
              reset_register_buffer (ed->core->regbuf->read_idx, ed)) < 0)
        return status;
      break;
    case REGISTER_WRITE:
      if ((status =
           append_to_register (ed->exec->region->start,
                               ed->exec->region->end, append, ed)) < 0)
        return status;
      spl1 ();
      if ((status = delete_lines (ed->exec->region->start,
                                  ed->exec->region->end, ed)) < 0)
        {
          spl0 ();
          return status;
        }
      if ((addr = INC_MOD (ed->state->dot, ed->state->lines)) != 0)
        ed->state->dot = addr;
      else if ((ed->state->is_empty = !ed->state->lines))
        ed->state->is_binary = 0;
      spl0 ();
      break;
    case REGISTER_READ | REGISTER_WRITE:
      if ((status = inter_register_move (append, ed)) < 0)
        return status;
      break;
    default:
      ed->exec->err = _("Invalid register");
      return ERR;
    }
#endif
  return ed->display->dio_f;
}

#ifdef WANT_ED_MACRO
static int
macro_cmd (ed_buffer_t *ed)
{
  static char *saved_input = NULL;
  static size_t saved_input_size = 0;

  int status = 0;               /* Return status */

  /* case '@': */
  if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
    return status;
  if (!ed->core->regbuf->rio_f)
    GET_INPUT_REGISTER (ed);
  else
    SKIP_WHITESPACE (ed);
  if (*ed->input != '\n')
    {
      ed->exec->err = _("Command suffix unexpected");
      return ERR;
    }
  else if (ed->exec->global && *(++ed->input) != '\0')
    {
      REALLOC_THROW (saved_input, saved_input_size,
                     strlen (ed->input) + 1, ERR, ed);
      strcpy (saved_input, ed->input);
    }
  if ((status = exec_macro (ed)) < 0)
    return status;
  else if (ed->exec->global)
    ed->input = saved_input;

  return status;
}
#endif  /* WANT_ED_MACRO */

static int
n_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /*
   * Allow printing an empty buffer, provided no addresses are
   * specified.
   */
  if (!(ed->exec->region->addrs || ed->state->lines))
    {
      ed->exec->region->start = 0;
      ed->exec->region->end = 0;
    }
  else if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed))
           < 0)
    return status;
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    ed->display->dio_f = NMBR;
  else
    ed->display->dio_f |= NMBR;
  return display_lines (ed->exec->region->start,
                        ed->exec->region->end, ed);
}

static int
P_cmd (ed_buffer_t *ed)
{
  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);

  /* Toggle PROMPT settings. */
  ed->exec->opt ^= PROMPT;
  return ed->display->dio_f;
}

static int
p_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /*
   * Allow printing an empty buffer, provided no addresses are
   * specified.
   */
  if (!(ed->exec->region->addrs || ed->state->lines))
    {
      ed->exec->region->start = 0;
      ed->exec->region->end = 0;
    }
  else if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
    return status;
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    ed->display->dio_f = PRNT;
  else
    ed->display->dio_f |= PRNT;
  return display_lines (ed->exec->region->start,
                        ed->exec->region->end, ed);
}

static int
q_cmd (ed_buffer_t *ed)
{
  int c = *(ed->input - 1);

  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  return (c == 'q' && ed->state->is_modified
          && !(ed->exec->opt & SCRIPTED) ? EMOD : EOF);
}

static int
r_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  off_t size = 0;
  size_t len = 0;
  char *fn = NULL;
  int cx = 0;
  int is_default = 0;
  int status = 0;               /* Return status */

  if (!isspace ((unsigned char) *ed->input))
    {
      ed->exec->err = _("Command suffix unexpected");
      return ERR;
    }
  SKIP_WHITESPACE (ed);

  /* If file glob empty, use current file, not first in list!. */
  if (ed->file->is_glob && *ed->input == '\n')
    ed->file->is_glob = 0;

  FILE_NAME (fn, len, 0, !ed->file->name, 1, ed);

  spl1 ();
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);

  /*
   * Reading a sequence of files is similar to concatenation,
   * except that missing newlines are appended. An empty file, on
   * the other hand, should not append a newline unless the buffer
   * is empty, in which case the buffer is still (logically)
   * empty. Concatenating an empty file and any other file yields
   * the other file.
   */
  cx = ed->state->newline_appended;
  if (ed->state->is_empty && ed->state->lines
      && (status = delete_lines (ed->state->dot, ed->state->dot, ed)) < 0)
    {
      spl0 ();
      return status;
    }
  ed->state->newline_appended = cx;
  if (!(ed->exec->region->addrs && ed->state->lines))
    ed->exec->region->end = ed->state->lines;
  spl0 ();

  if (*fn == '!' || (*fn == '\0' && ed->file->name && *ed->file->name == '!'))
    {
      if ((status = read_pipe (*fn == '!' ? fn : ed->file->name,
                               ed->exec->region->end, &addr, &size, ed)) < 0)
        return status;
    }
  else
    {
      /* Save filename as default iff ed->file->name not set. */
      if (!ed->file->name && *fn != '\0')
        {
          REALLOC_THROW (ed->file->name, ed->file->name_size, len + 1, ERR, ed);
          strcpy (ed->file->name, fn);
          ++is_default;
        }
      else if (*fn == '\0' && ed->file->name && *ed->file->name != '\0')
        ++is_default;

      /*
       * As per SUSv4, file argument is optional. Though not
       * mandated by SUSv5, if default file name is not set,
       * trivially succeed by reading from `/dev/null'.
       */
      if ((status = read_file (is_default ? ed->file->name
                               : (*fn != '\0' ? fn : "/dev/null"),
                               ed->exec->region->end, &addr, &size,
                               is_default, ed)) < 0)
        return status;

      if (ed->file->is_glob && ed->exec->opt & VERBOSE)
        puts (fn);

      /*
       * Reading `/dev/null' to the end of a binary file is a
       * special case for removing any trailing newline.
       */
      if (ed->state->is_binary
          && !strcmp ((is_default ? ed->file->name
                       : (*fn != '\0' ? fn : "/dev/null")), "/dev/null"))
        ed->state->newline_appended = 1;
    }

  /* If multiple file args on command line, print first one. */
  if (ed->exec->opt & PRINT_FIRST_FILE && ed->file->list->gl_pathc > 1)
    puts (ed->file->name);

  printf (ed->exec->opt & (SCRIPTED | WRITE_ONLY) ? ""
          : "%" OFF_T_FORMAT_STRING "\n", size);
  if (addr && addr != ed->state->lines)
    ed->state->is_modified = 1;
  return 0;
}

static int
s_cmd (ed_buffer_t *ed)
{
  regex_t *lhs    = NULL;       /* Left-hand substitution pattern buffer */
  off_t s_nth     = 0;          /* Substitution match offset */
  off_t s_mod     = 0;          /* Substitution match modulus */
  int status = 0;               /* Return status */
  unsigned s_f    = 0;          /* Saved substitution modifier flags */
  unsigned sgpr_f = 0;          /* Short-form substitution modifier flags */
  unsigned sio_f  = 0;          /* Saved substitution I/O flags */

  init_substitute (&lhs, &s_f, &s_nth, &s_mod, &sio_f, ed->exec->subst);
  if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0
      || (status = resubstitute (&s_nth, &s_mod, &s_f, &sgpr_f, ed)) < 0
      || (status = substitution_lhs (&lhs, &sgpr_f, ed)) < 0)
    return status;

  /* If repeated substitution, toggle s_f per modifiers. */
  if (sgpr_f)
    {
      /* Ignore after first iteration of non-interactive global. */
      if (!ed->exec->global || ed->exec->first_pass
          || (ed->exec->global & GLBI))
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

  /*
   * Get right-hand side (rhs) of substitution command, i.e.,
   * substitution template.
   */
  else if ((status = substitution_rhs (&s_nth, &s_mod, &s_f, &sio_f, ed)) < 0)
    return status;
  COMMAND_SUFFIX (sio_f, ed);
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);
  save_substitute (lhs, s_f, s_nth, s_mod, sio_f, ed->exec->subst);
  if ((status = substitute_lines (ed->exec->region->start,
                                  ed->exec->region->end,
                                  lhs, s_nth, s_mod, s_f, ed)) < 0)
    return status;
  return ((ed->display->dio_f = sio_f) && !(s_f & PRSW) ?
          display_lines (ed->state->dot, ed->state->dot, ed) : 0);
}

static int
scroll_backward_half (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  /*
   * (.)[n - Displays page of `n' lines centered around addressed
   * line, where `n' defaults to current window size. The current
   * address is set to the last line displayed. A subsequent `['
   * command displays 'n' lines centered around first line in page.
   */
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    {
      ed->exec->err = _("Unknown command");
      return ERR;
    }

  /* Allow `[' command to succeed immediately after opening file. */
  if (!ed->display->is_paging && ed->display->page_addr == 0)
    ed->display->page_addr = ed->state->dot;

  if ((status =
       is_valid_range (ed->display->page_addr, ed->display->page_addr, ed)) < 0)
    return status;

  /* Scrolling into bottom half of first page...   */
  if (ed->display->is_paging
      && ed->display->ws_row / 2 - 1 < ed->display->page_addr
      && ed->display->page_addr < ed->display->ws_row)
    {
      /* Calculation assumes one buffer line per terminal line. */
      ed->display->page_addr += ed->display->ws_row / 2 - 1;
      ed->display->page_addr = min (ed->state->lines, ed->display->page_addr);
      ed->exec->region->start = ed->display->page_addr;
      ed->exec->region->end = ed->display->page_addr;

      ed->display->dio_f |= ZHBW;
      return Z_cmd (ed);
    }

  /*
   * Half-page scrolling implemenation requires "normalized" frame
   * buffer, so load page preceding current address in background
   * (display->hidden = 1), then scroll from there.
   */
  if ((status = normalize_frame_buffer (ed)) < 0)
    return status;

  /* At start or end of buffer, so display it. */
  /*
   * if ((!ed->display->is_paging && ed->exec->region->end < ed->state->dot)
   */
  if ((!ed->display->is_paging
       && ed->exec->region->end <= ed->display->ws_row / 2 + 1)
      || ed->state->dot == ed->state->lines)
    {
      ed->display->dio_f |= ZHBW;
      return Z_cmd (ed);
    }

  return z_cmd (ed);
}

static int
scroll_forward_half (ed_buffer_t *ed)
{
  int status;

  /*
   * (.)]n - Displays page of `n' lines centered around addressed
   * line, where `n' defaults to current window size. The current
   * address is set to the last line printed. A subsequent `]'
   * command displays `n' lines centered around last line in page.
   */
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    {
      ed->exec->err = _("Unknown command");
      return ERR;
    }

  if (!ed->display->is_paging || ed->exec->region->addrs)
    {

      /* Allow `]' command to succeed immediately after opening file. */
      if (!ed->display->is_paging && ed->display->page_addr == 0)
        ed->display->page_addr = ed->state->dot;

      if ((status = is_valid_range (ed->display->page_addr,
                                    ed->display->page_addr, ed)) < 0)
        return status;

      /* Targeting region in bottom half of first page...   */
      if (ed->exec->region->addrs
          && ed->display->ws_row / 2 - 1 < ed->exec->region->end
          && ed->exec->region->end < ed->display->ws_row)
        {
          /* Calculation assumes one buffer line per terminal line. */
          ed->state->dot = ed->exec->region->end + ed->display->ws_row / 2 - 1;
          ed->state->dot = min (ed->state->lines, ed->state->dot);
          ed->exec->region->start = ed->state->dot;
          ed->exec->region->end = ed->state->dot;

          ed->display->dio_f |= ZHBW;
          return Z_cmd (ed);
        }

      /*
       * Half-page scrolling implemenation requires "normalized" frame
       * buffer, so load page preceding current address in background
       * (display->hidden = 1), then scroll from there.
       */
      if ((status = normalize_frame_buffer (ed)) < 0)
        return status;

      if ((!ed->display->is_paging
           && ed->exec->region->end <= ed->display->ws_row / 2 + 1)
          || ed->state->dot == ed->state->lines)
        {
          ed->display->dio_f |= ZHBW;
          return Z_cmd (ed);
        }

      /*
       * ed->display->dio_f |= ZHFW;
       * return scroll_backward_half (ed);
       */
    }

  return z_cmd (ed);
}

static int
t_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  int append = 0;               /* Append to register? */
  int status = 0;               /* Return status */

  if ((status = is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
    return status;
  DESTINATION_ADDRESS (addr, ed);
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if (!(ed->exec->global || ed->core->sp))
    reset_undo_queue (ed);
#ifdef WANT_ED_REGISTER
  switch (ed->core->regbuf->rio_f)
    {
    case 0:
#endif
      if ((status = copy_lines (ed->exec->region->start,
                                ed->exec->region->end, addr, ed)) < 0)
        return status;
#ifdef WANT_ED_REGISTER
      break;
    case REGISTER_READ:
      if ((status = append_from_register (addr, ed)) < 0)
        return status;
      break;
    case REGISTER_WRITE:
      if ((status = append_to_register (ed->exec->region->start,
                                        ed->exec->region->end, append, ed)) < 0)
        return status;
      break;
    case REGISTER_READ | REGISTER_WRITE:
      if ((status = inter_register_copy (append, ed)) < 0)
        return status;
      break;
    default:
      ed->exec->err = _("Invalid register");
      return ERR;
    }
#endif
  return ed->display->dio_f;
}

static int
u_cmd (ed_buffer_t *ed)
{
  int status = 0;               /* Return status */

  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  if ((status = undo_last_command (ed)) < 0)
    return status;
  return ed->display->dio_f;
}

static int
w_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  off_t size = 0;
  size_t len = 0;
  char *fn = NULL;
  int status = 0;               /* Return status */
  int is_default = 0;
  int c = *(ed->input - 1);
  int cx = 0;
  int cy = 0;

  /*
   * Allow writing an empty buffer provided no addresses are
   * specified.
   */
  if (!(ed->exec->region->addrs || ed->state->lines))
    {
      ed->exec->region->start = 0;
      ed->exec->region->end = 0;
    }
  else if ((status = is_valid_range (1, ed->state->lines, ed)) < 0)
    return status;
  if (ed->exec->opt & POSIXLY_CORRECT
      && !isspace ((unsigned char) *ed->input))
    {
      ed->exec->err = _("Command suffix unexpected");
      return ERR;
    }
#ifdef WANT_FILE_GLOB
  else if (ed->file->is_glob && ((cy = *ed->input) == 'n' || cy == 'p'))
    ++ed->input;
#endif
  else if ((cx = *ed->input) == 'q')
    {
#ifdef WANT_SAFE_WRITE
      if (ed->file->is_glob)
        {
          ed->exec->err = _("Command suffix unexpected");
          return ERR;
        }
#endif
      ++ed->input;
    }
  if (!isspace ((unsigned char) *ed->input))
    {
      ed->exec->err = _("Command suffix unexpected");
      return ERR;
    }
  SKIP_WHITESPACE (ed);

  /* If file glob empty, use current file, not first in list!. */
  if (ed->file->is_glob && *ed->input == '\n')
    ed->file->is_glob = 0;

#ifdef WANT_SAFE_WRITE
  FILE_NAME (fn, len, ed->file->is_glob,
             ed->file->is_glob ? *ed->input != '\n' : !ed->file->name, 1, ed);
#else
  FILE_NAME (fn, len, ed->file->is_glob, !ed->file->name, 1, ed);
#endif  /* !WANT_SAFE_WRITE */

  if (*fn == '!' || (*fn == '\0' && ed->file->name && *ed->file->name == '!'))
    {
      if ((status =
           write_pipe (*fn == '!' ? fn : ed->file->name,
                       ed->exec->region->start,
                       ed->exec->region->end, &addr, &size, ed)) < 0)
        return status;
    }
  else
    {
#ifdef WANT_SAFE_WRITE
      if (ed->file->is_glob)
        {
          if (ed->file->name)
            ++is_default;
          else
            fn = "";
        }
      else
#endif
        /* Save filename as default iff ed->file->name not set. */
        if (!ed->file->name && *fn != '\0')
          {
            REALLOC_THROW (ed->file->name, ed->file->name_size,
                           len + 1, ERR, ed);
            strcpy (ed->file->name, fn);
            ++is_default;
          }
        else if (*fn == '\0' && ed->file->name && *ed->file->name != '\0')
          ++is_default;

      /*
       * Per SUSv4, file argument is optional. Though not mandated
       * by SUSv4, if default file name is not set, trivially
       * succeed by writing to `/dev/null'.
       */
      if ((status =
           write_file (is_default ? ed->file->name
                       : (*fn != '\0' ? fn : "/dev/null"),
                       is_default, ed->exec->region->start,
                       ed->exec->region->end,
                       &addr, &size, (c == 'W' ? "a" : "w"), ed)) < 0)
        return status;

      /*
       * Per SUSv4, whole buffer must be written to file (not `!')
       * to reset buffer modifed state.
       */
      if (addr == ed->state->lines && (is_default || *fn != '\0'))
        {
          ed->state->is_modified = 0;

          /*
           * Per SUSv4, exit_status cannot be reset. We'll do it
           * anyhow ...
           */
          if (!(ed->exec->opt &
                (POSIXLY_CORRECT | TRADITIONAL | SCRIPTED | EXIT_ON_ERROR)))
            ed->exec->status = 0;
        }
      if (ed->file->is_glob && ed->exec->opt & VERBOSE)
        puts (is_default ? ed->file->name : (*fn != '\0' ? fn : "/dev/null"));
    }
  printf (ed->exec->opt & SCRIPTED ? "" : "%" OFF_T_FORMAT_STRING "\n", size);

  /*
   *                        State Vars
   * Write command          cx    cy
   * =============          ==========
   *   w                    '\n'  '\n'
   *   w filename           ' '   ' '
   *   w !shell-cmd         '\n'  '\n'
   *   wn                         'n'
   *   wn filename                'n'
   *   wn !shell-cmd              'n'
   *   wp                         'p'
   *   wp filename                'p'
   *   wp !shell-cmd              'p'
   *   wq                   'q'   'q'
   *   wq filename          'q'   'q'
   *   wq !shell-cmd        'q'   'q'
   *   ~w                   '\n'  '\n'
   *   ~w fileglob          ' '   ' '
   *   ~w !shell-cmd        ' '   ' '
   *   ~wq                  'q'   'q'
   *   ~wq fileglob         'q'   'q'
   *   ~wq !shell-cmd       'q'   'q'
   */

#ifdef WANT_FILE_GLOB
  return (
# ifdef WANT_SAFE_WRITE
          (ed->file->is_glob || cy == 'n' || cy == 'p' || cx == 'q')
          && ed->state->is_modified && !(ed->exec->opt & SCRIPTED) ? EMOD
          : ed->file->is_glob ? EOF_GLOB
# else
          (cy == 'n' || cy == 'p' || cx == 'q')
          && ed->state->is_modified && !(ed->exec->opt & SCRIPTED) ? EMOD
# endif  /* !WANT_SAFE_WRITE */
          : cy == 'n' ? EOF_NEXT
          : cy == 'p' ? EOF_PREV
          : cx == 'q' ? EOF
          : 0);
#else
  return (cx == 'q' ? (ed->state->is_modified && !(ed->exec->opt & SCRIPTED)
                       ? EMOD : EOF) : 0);
#endif  /* !WANT_FILE_GLOB */
}

#ifdef WANT_ED_ENCRYPTION
static int
x_cmd (ed_buffer_t *ed)
{
  int status = 0;

  /*
   * Of all commands, at least `h' should be forgiving, but SUSv4,
   * 2013 says otherwise...
   */
  if (ed->exec->region->addrs)
    {
      ed->exec->err = _("Address unexpected");
      return ERR;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  return (status = get_des_keyword (ed)) < 0 ? status : ed->display->dio_f;
}
#endif  /* WANT_ED_ENCRYPTION */

static int
Z_cmd (ed_buffer_t *ed)
{
  size_t len = 0;
  off_t addr = 0;
  int c = *(ed->input - 1);
  int status = 0;               /* Return status */

  /* scroll backward */
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    {
      ed->exec->err = _("Unknown command");
      return ERR;
    }

  /* Determine address of last line to print. */
  if (ed->exec->region->addrs)
    {
      addr = ed->state->dot - !ed->exec->global;
      ed->display->underflow = 0;
      ed->display->overflow = 0;
    }
  else
    {
      addr = (ed->display->is_paging ? ed->display->page_addr : ed->state->dot);

      /* XXX: Compensate for half-page scroll addressed line loss. */
      addr -= !(ed->exec->global || ed->display->overflow
                || (ed->display->dio_f & (ZHBW | ZHFW)
                    && (ed->state->dot == ed->state->lines)));

      /* Allow `Z' command to succeed immediately after opening file. */
      if (!ed->display->is_paging && c == 'Z' && addr < ed->state->lines)
        ++addr;

    }
  if ((status = is_valid_range (ed->exec->region->start = 1, addr, ed)) < 0)
    return status;
  if (isdigit ((unsigned char) *ed->input))
    {
      STRTOUL_THROW (len, ed->input, &ed->input, ERR);

      /* Sanity check window size. */
      if (1 < len && len < ROWS_MAX)
        ed->display->ws_row = len;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  ++ed->display->paging;

  /*
   * Display page preceding either given line or already displayed
   * page. Otherwise, display page preceding current address.
   */
  ed->display->dio_f |= ZBWD;
  return display_lines (max (1, (off_t) ed->exec->region->end -
                             ed->display->ws_row + 2),
                        ed->exec->region->end, ed);
}

static int
z_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  size_t len = 0;
  int c = *(ed->input - 1);
  int status = 0;               /* Return status */

  /* scroll forward */
  if (ed->exec->opt & POSIXLY_CORRECT)
    {
      ed->exec->err = _("Unknown command");
      return ERR;
    }

  /* If address provided, ignore frame buffer status. */
  if (ed->exec->region->addrs)
    {
      ed->display->underflow = 0;
      ed->display->overflow = 0;
    }

  addr = c == 'z' ? ed->state->dot : ed->exec->region->end;
  addr += !(ed->exec->global || ed->display->underflow);

  /* Allow `z' command to succeed immediately after opening file. */
  if (!ed->display->is_paging && c == 'z' && addr > 1)
    --addr;

  /*
   * If no address specified (i.e., ed->exec->region->addrs == 0),
   * then is_valid_range() assigns: ed->exec->region->end = addr
   */
  if ((status = is_valid_range (ed->exec->region->start = 1, addr, ed)) < 0)
    return status;
  if (isdigit ((unsigned char) *ed->input))
    {
      STRTOUL_THROW (len, ed->input, &ed->input, ERR);

      /* Sanity check window size. */
      if (1 < len && len < ROWS_MAX)
        ed->display->ws_row = len;
    }
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  ++ed->display->paging;

  switch (c)
    {
    case 'z':
      /*
       * When scrolling full page, calculate last line to display,
       * addr, where first line is region->end >= 1, and total
       * number of lines in a full page is ws_row - 1, since last
       * screen line is reserved for command prompt:
       *
       *    total_lines == ws_row - 1 == addr - region->end + 1
       *
       * or
       *
       *    addr = region->end + (ws_row - 1) - 1
       */
      addr = min (ed->state->lines, (ed->exec->region->end +
                                     ed->display->ws_row - 2));
      ed->display->dio_f |= ZFWD;
      break;
    case '[':
    case ']':

      /*
       * When scrolling half a page, calculate last line to display,
       * addr, relative to  region->end >= 1 by adding half the total
       * number of lines in a full page, i.e.,  (ws_row - 1) / 2:
       *
       *    total_lines / 2 == (ws_row - 1) / 2 == addr - region->end + 1
       *
       * or
       *
       *    addr = region->end + (ws_row - 1) / 2 - 1
       *
       * NB: In this case, region->end is displayed in the middle of
       *     the screen.
       */
      addr = min (ed->state->lines, (ed->exec->region->end +
                                     ((ed->display->ws_row - 1) >> 1) - 1));
      ed->display->dio_f |= ZFWD | ZHFW;
      break;
    }
  return display_lines (ed->exec->region->end, addr, ed);
}

static int
address_cmd (ed_buffer_t *ed)
{

  /* case '=': */
  COMMAND_SUFFIX (ed->display->dio_f, ed);
  printf ("%" OFF_T_FORMAT_STRING "\n",
          (ed->exec->region->addrs ? ed->exec->region->end : ed->state->lines));
  return ed->display->dio_f;
}

static int
shell_cmd (ed_buffer_t *ed)
{
  /*
   * static char exit_status[BUFSIZ];
   */

  off_t addr = 0;
  size_t len = 0;
  char *fn = NULL;
  int status = 0;               /* Return status */

  /* case '!': */
  --ed->input;
  FILE_NAME (fn, len, 0, 0, 0, ed);
  ed->exec->err = NULL;
  if (!ed->exec->region->addrs)
    {
#ifdef HAVE_VFORK
      status = system_shell (++fn, ed);
      printf (ed->exec->opt & SCRIPTED ? "" : "!\n");
#else
      fprintf (stderr, "%s\n", _("Cannot fork shell"));
      ed->exec->err = _("Fork error");
      status = ERR;
#endif
    }
  else
#if defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER)
    if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
#endif
      {
        ed->exec->err = _("Address unexpected");
        return ERR;
      }
#if defined (HAVE_FORK) && defined (WANT_EXTERNAL_FILTER)
    else
      {
        /*
         * Reset region->addrs to 0 when buffer empty to avoid
         * subsequent calls to delete_lines/write_stream.
         * When buffer not empty, then:
         *   0! shell-cmd => 1! shell-cmd
         *   0,0! shell-cmd => 1,1! shell-cmd
         * per c_cmd, q.v..
         */
        if (ed->state->lines
            || (ed->exec->region->addrs = !(ed->exec->region->start == 0
                                            && ed->exec->region->end == 0)))
          {
            ed->exec->region->start += !ed->exec->region->start;
            ed->exec->region->end += !ed->exec->region->end;
            if ((status =
                 is_valid_range (ed->state->dot, ed->state->dot, ed)) < 0)
              return status;
          }

        if (!(ed->exec->global || ed->core->sp))
          reset_undo_queue (ed);
        addr = ed->state->lines;
        if ((status = filter_lines (ed->exec->region->start,
                                    ed->exec->region->end, ++fn, ed)) < 0)
          return status;
        ed->state->dot =
          ed->exec->region->end + ed->state->lines - addr;
      }
#endif  /* defined HAVE_FORK && defined WANT_EXTERNAL_FILTER */
  return status;
}

static int
newline_cmd (ed_buffer_t *ed)
{
  off_t addr = 0;
  int status = 0;               /* Return status */

  /* case '\n': */
  addr = ed->state->dot + !ed->exec->global;
  if ((status = is_valid_range (ed->exec->region->start = 1, addr, ed)) < 0)
    return status;

  /* Per SUSv4, newline in global command is null command.  */
  if ((!ed->exec->global || ed->exec->region->addrs)
      && (status = display_lines (ed->exec->region->end,
                                  ed->exec->region->end, ed)) < 0)
    return status;
  return 0;
}

static int
comment_cmd (ed_buffer_t *ed)
{
  /* case '#': */
  if (ed->exec->opt & (POSIXLY_CORRECT | TRADITIONAL))
    {
      ed->exec->err = _("Unknown command");
      return ERR;
    }
  while (*ed->input++ != '\n')
    ;
  return 0;
}

static int
invalid_cmd (ed_buffer_t *ed)
{
  off_t dot = ed->state->dot;

  /* Check for out-of-sequence address. */
  ed->exec->err =
    (next_address (&dot, ed) > 0 ? _("Invalid address") : _("Unknown command"));
  return ERR;
}

/* normalize_frame_buffer: Exec hidden `Z' command with currrent address. */
static int
normalize_frame_buffer (ed_buffer_t *ed)
{
  static char *buf = NULL;
  static size_t buf_size = 0;

  char *modifier = ed->input;
  size_t len;
  int status;

  /* Allocate for `start,end' + `Z' + modifier. */
  if ((len = 2 * OFF_T_LEN + strlen (modifier) + 3) >= INT_MAX)
    {
      ed->exec->err = _("Command too long");
      return ERR;
    }
  REALLOC_THROW (buf, buf_size, len, ERR, ed);

  /* Always use default address range. */
  snprintf (buf, len,
            "%" OFF_T_FORMAT_STRING ",%" OFF_T_FORMAT_STRING "%c%s",
            ed->exec->region->start,
            ed->exec->region->end, 'Z', modifier);
  ed->input = buf;
  ed->display->hidden = 1;
  if ((status = address_range (ed)) < 0
      || (status = exec_command (ed)) < 0)
    return status;
  ed->display->hidden = 0;
  ed->input = modifier;

  /* Forget any given address - necessary for consistent behavior of
     half-forward and half-backward scrolling. */
  ed->exec->region->addrs = 0;
  return 0;
}

/* is_valid_range: Return status of address range check. */
static int
is_valid_range (off_t from, off_t to, ed_buffer_t *ed)
{
  if (!ed->exec->region->addrs)
    {
      ed->exec->region->start = from;
      ed->exec->region->end = to;
    }
  if ((ed->exec->region->start < 1
       || ed->state->lines < ed->exec->region->start
       || ed->exec->region->end < 1
       || ed->state->lines < ed->exec->region->end)
#ifdef WANT_ED_REGISTER
      && !(ed->core->regbuf->rio_f & REGISTER_READ)
#endif
      )
    {
      ed->exec->err = _("Address out of range");
      return ERR;
    }
  if (ed->exec->region->start > ed->exec->region->end)
    {
      ed->exec->err = _("Invalid address range");
      return ERR;
    }
  return 0;
}


/*
 * get_path_max: Return PATH_MAX for given file name, which may be
 *   filesystem dependent.
 */
size_t
get_path_max (const char *ps)
{
  return PATH_MAX;
}
