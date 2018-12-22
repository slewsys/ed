/* ed.h: Header for the ed line editor.
 *
 *  Copyright © 1993-2018 Andrew L. Moore, SlewSys Research
 *
 *  This file is part of ed.
 */

# include "config.h"

/*
 * Work around conflicting off_t typedefs (e.g., if _LARGE_FILE
 * support enabled). Defined per configure, below.
 */
#ifndef __sun
# define off_t INTERNAL_OFF_T
#endif

#include <ctype.h>
#include <errno.h>

#ifndef errno
extern int errno;
#endif

#ifdef HAVE_GLOB_H
# include <glob.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

# include "gettext.h"

#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#include "pathmax.h"
#include <setjmp.h>

#ifdef HAVE_SIGLONGJMP
extern sigjmp_buf env;
# define JMP_BUF   sigjmp_buf
# define SETJMP(x) sigsetjmp ((x), 1)
# define LONGJMP(x, y) siglongjmp ((x), (y))
#else
extern jmp_buf env;
# define JMP_BUF   jmp_buf
# define SETJMP(x) setjmp (x)
# define LONGJMP(x, y) longjmp ((x), (y))
#endif  /* !HAVE_SIGLONGJMP */

#include <stdio.h>
#include <signal.h>

typedef void (*signal_t) (int);

#ifndef HAVE_SIG_ATOMIC_T
typedef int sig_atomic_t;
#endif

#ifndef SIG_ERR
# define SIG_ERR ((signal_t) -1)
#endif

#ifndef NSIG
# define NSIG 32
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
char *getenv ();
void *malloc ();
void *realloc ();
long strtol ();
long labs ();
#endif  /* !STDC_HEADERS */

#ifdef SETVBUF_REVERSED
# define SETVBUF(fp, buf, type, sz) setvbuf ((stdin), (type), (buf), (sz))
#else
# define SETVBUF(fp, buf, type, sz) setvbuf ((stdin), (buf), (type), (sz))
#endif /* !SETVBUF_REVERSED */

#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#if defined (STDC_HEADERS) || defined (HAVE_STRING_H)
# include <string.h>
#else
# include <strings.h>
# define strchr index
# define strrchr rindex
# define memcpy(d, s, n) bcopy ((s), (d), (n))
# define memcmp(s1, s2, n) bcmp ((s1), (s2), (n))
#endif  /* !defined (STDC_HEADERS) && !defined (HAVE_STRING_H) */

#ifndef HAVE_STRERROR
extern const char *sys_errlist[];
extern int sys_nerr;
# define strerror(n)                                                          \
  ((n) > 0 && (n) < sys_nerr ? sys_errlist[n] : _("Unknown error"))
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#else
extern int optind;
extern char *optarg;
char *mktemp ();
int access ();
int dup2 ();
#endif  /* !HAVE_UNISTD_H */

#ifdef HAVE_WCHAR_H
# include <wchar.h>
#endif

#include <sys/types.h>

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#else
int fstat ();
#endif  /* !HAVE_SYS_STAT_H */

#ifndef S_ISREG
# define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#ifndef SEEK_SET
# define SEEK_SET 0
# define SEEK_CUR 1
# define SEEK_END 2
#endif

#ifdef HAVE_FSEEKO
# define FSEEK(fp, off, pos) fseeko ((fp), (off), (pos))
# define FTELL(fp)           ftello (fp)
#else
# define FSEEK(fp, off, pos) fseek ((fp), (off), (pos))
# define FTELL(fp)           ftell (fp)
#endif  /* !HAVE_FSEEKO */

#include <regex.h>

/* Define off_t per storage size determined by configure. */
#ifndef __sun
# undef off_t
typedef OFF_T_SIZE off_t;

/* Define size_t per storage size determined by configure. */
# undef size_t
typedef SIZE_T_SIZE size_t;
#endif  /* !defined (__sun) */

#ifndef INT_MAX
# define INT_MAX ((int) (~(unsigned int) 0 >> (unsigned int) 1))
# define INT_MIN (-INT_MAX - 1)
#endif

#ifndef LONG_MAX
# define LONG_MAX ((long) (~(unsigned long) 0 >> (unsigned long) 1))
# define LONG_MIN (-LONG_MAX - 1)
#endif

#ifndef ULONG_MAX
# define ULONG_MAX ((unsigned long) ~(unsigned long) 0)
#endif

#ifndef SIZE_T_MAX
# define SIZE_T_MAX                                                           \
  ((size_t) (~(size_t) 0))
#endif

#define LINECHARS SIZE_T_MAX    /* Max chars per line, including NULs. */

#ifndef LLONG_MAX
# define LLONG_MAX                                                            \
  ((long long) (~(unsigned long long) 0 >> (unsigned long long) 1))
# define LLONG_MIN (-LLONG_MAX - 1)
#endif

#ifndef OFF_T_MAX
#  define OFF_T_MAX                                                           \
  ((off_t) (~(size_t) 0 >> (size_t) 1))
#  define OFF_T_MIN (-OFF_T_MAX - 1)
#endif

#ifndef max
# define max(a,b) ((a) > (b) ? (a) : (b))
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* UTF-8 character constants. */
enum utf8_char_constant
  {
    BELL = '\a',
    BACKSPACE = '\b',
    CHARACTER_TABULATION = '\t',
    LINE_FEED = '\n',
    LINE_TABULATION = '\v',
    FORM_FEED = '\f',
    CARRIAGE_RETURN = '\r',
    ESCAPE = '\033',
    SPACE = ' ',
    EXCLAMATION_MARK = '!',
    QUOTATION_MARK = '\"',
    DOLLAR_SIGN = '$',
    PERCENT_SIGN = '%',
    PLUS_SIGN = '+',
    COMMA = ',',
    APOSTROPHE = '\'',
    FULL_STOP = '.',
    SOLIDUS = '/',
    COLON = ':',
    SEMICOLON = ';',
    EQUALS_SIGN = '=',
    QUESTION_MARK = '\?',
    LEFT_SQUARE_BRACKET = '[',
    REVERSE_SOLIDUS = '\\',
    RIGHT_SQUARE_BRACKET = ']',
    CIRCUMFLEX_ACCENT = '^'
  };

/*
 * Approximate upper bound for strlen (OFF_T_MAX) from relations:
 *     2 ^ (8 * sizeof (off_t)) < 10 ^ strlen (OFF_T_MAX)
 *     10 ^ strlen (OFF_T_MAX)   <  2 ^ (10 * sizeof (off_t))
 */
#define OFF_T_LEN (3 * sizeof (off_t))

#if defined (HAVE_STAT64) && !__DARWIN_64_BIT_INO_T && !defined (__linux__)
# define INO_T       ino64_t
# define STAT_T      struct stat64
# define FSTAT(x, y) fstat64 ((x), (y))
# define STAT(x, y)  stat64 ((x), (y))
#else
# define INO_T       ino_t
# define STAT_T      struct stat
# define FSTAT(x, y) fstat ((x), (y))
# define STAT(x, y)  stat ((x), (y))
#endif  /* !HAVE_STAT64 */

/* In-core and hold node. */
typedef struct ed_line_node
{
  struct ed_line_node *q_forw;
  struct ed_line_node *q_back;
  size_t len;                   /* Line length. */
  off_t seek;                   /* Line offset in on-disk buffer. */
} ed_line_node_t;

#define ED_LINE_NODE_T_SIZE (sizeof (ed_line_node_t))

/* Console frame node. */
typedef struct ed_frame_node
{
  ed_line_node_t *lp;           /* Line pointer. */
  char *text;                   /* Text buffer. */
  size_t text_size;             /* Text buffer size. */
  size_t text_i;                /* Current text index in row. */
  size_t offset;                /* Offset into source text of row. */
  off_t addr;                   /* Address of current row's line. */
} ed_frame_node_t;

#define ED_FRAME_NODE_T_SIZE (sizeof (ed_frame_node_t))

/* Console frame parameters. */
typedef struct ed_frame_buffer
{
  ed_frame_node_t *prev_first;  /* Pointer to first row in previous frame. */
  ed_frame_node_t *prev_last;   /* Pointer to last row of previous frame. */
  ed_frame_node_t **row;        /* Array of pointers to frame nodes. */
  int row_i;                    /* Index of current row in frame buffer. */
  int first_i;                  /* Index of first frame buffer row to print. */
  int last_i;                   /* Index of last frame buffer row to print. */
  int fill_i;                   /* Index to fill to when half-scrolling. */

  size_t ff_offset;             /* Offset of first row in frame. */
  size_t rem_chars;             /* Remaining chars in source text. */
  int columns;                  /* Number of columns in frame. */
  int rows;                     /* Number of rows in frame. */
  int is_full;                  /* If set, frame buffer full. */
  int wraps;                    /* If set, text wraps around top of frame. */
} ed_frame_buffer_t;

/* Global mark node. */
typedef struct ed_global_node
{
  struct ed_global_node *q_forw;
  struct ed_global_node *q_back;
  ed_line_node_t *lp;           /* Marked node pointer. */
} ed_global_node_t;

#define ED_GLOBAL_NODE_T_SIZE (sizeof (ed_global_node_t))

/* In-core text node. */
typedef struct ed_text_node
{
  struct ed_text_node *q_forw;
  struct ed_text_node *q_back;
  char *text;
  size_t text_i;
} ed_text_node_t;

#define ED_TEXT_NODE_T_SIZE (sizeof (ed_text_node_t))

/* Undo nodes types - inverse derived by toggling least-significant bit. */
typedef enum ed_undo_op
  {
    UADD = 0x00,
    UDEL = 0x01,
    UMOV = 0x10,
    URET = 0x11
  } ed_undo_op_t;

/* Undo node. */
typedef struct ed_undo_node
{
  struct ed_undo_node *q_forw;
  struct ed_undo_node *q_back;
  ed_line_node_t *h;            /* Address of first modified line. */
  ed_line_node_t *t;            /* Address of last modified line. */
  ed_undo_op_t type;            /* Node type. */
} ed_undo_node_t;

#define ED_UNDO_NODE_T_SIZE (sizeof (ed_undo_node_t))

/* Ed buffer state parameters. */
struct ed_state
{
  off_t dot;                    /* Current line number. */
  off_t lines;                  /* Number of lines. */

  int is_binary;                /* If set, buffer contains binary data.  */
  int is_utf8;                  /* If set, buffer contains UTF-8 data.  */
  int is_empty;                 /* If set, buffer is "logically" empty. */
  int is_modified;              /* If set, buffer contains unsaved data. */
  int newline_appended;         /* if set, newline appended to buffer. */
  int newline_missing;      /* If set, newline needed on input.  */
  int input_is_binary;          /* If set, binary data on input. */
};

#ifdef WANT_ED_REGISTER
/* Register I/O flags. */
enum register_io_flags
  {
    REGISTER_READ  = 0x01,
    REGISTER_WRITE = 0x02
  };

#define REGBUF_MAX  11          /* Max registers, including default. */

/* Register buffers. */
struct ed_register
{
  ed_line_node_t *lp[REGBUF_MAX];  /* Register buffers. */
  int read_idx;                 /* Input register index. */
  int write_idx;                /* Output register index. */
  int io_f;                     /* Register I/O flags. */
};
#endif  /* WANT_ED_REGISTER */

#ifdef WANT_ED_MACRO
/* Script buffer stack frame. */
typedef struct ed_stack_frame
{
  FILE *fp;                     /* Nominal standard input. */
  off_t size;                   /* Nominal script buffer size. */
  off_t addr;                   /* Nominal script buffer address. */
} ed_stack_frame_t;

# define ED_STACK_FRAME_T_SIZE (sizeof (ed_stack_frame_t))

# define STACK_FRAMES_MAX 50     /* Max stack frames in script buffer. */
#endif /* WANT_ED_MACRO */

#define MARK_MAX 26             /* Max line markers. */

/* Ed buffer meta data and storage parameters. */
struct ed_core
{

#ifdef WANT_ED_REGISTER
  /* Register buffers and line markers. */
  struct ed_register *regbuf;
#endif

#ifdef WANT_ED_MACRO
  /* Script buffer stack frame. */
  struct ed_stack_frame *stack_frame[STACK_FRAMES_MAX];
  int sp;                       /* Script buffer stack pointer. */
#endif

  ed_line_node_t *mark[MARK_MAX];
  int marks;

  /* Edit-processing buffers. */
  ed_line_node_t *line_head;     /* Head of line buffer. */
  ed_global_node_t *global_head; /* Head of global line buffer. */
  ed_undo_node_t *undo_head;     /* Head of undo buffer. */

  /* Buffer storage */
  FILE *fp;                     /* Pointer to on-disk buffer. */
  char *pathname;               /* Pathname of on-disk buffer. */
  size_t pathname_size;         /* Size of on-disk buffer pathname. */
  off_t offset;                 /* Current offset of on-disk buffer. */
  int seek_on_write;            /* If set, seek before writing. */
};

/* Default display size. */
enum default_window_size
  {
    WS_ROW = 24,                /* Height in characters. */
    WS_COL = 80                 /* Width in characters. */
  };

/* Display parameters */
struct ed_display
{
  volatile sig_atomic_t ws_row; /* Display height. */
  volatile sig_atomic_t ws_col; /* Display width. */
  off_t page_addr;              /* Address of first displayed line in page. */
  int paging;                   /* Set when displaying a page of text. */
  int is_paging;                /* If set, continuing paging of text. */
  int underflow;                /* If set, line truncated at top page. */
  int overflow;                 /* If set, line truncated at bottom of page. */
  int hidden;                   /* If set, do not print frame buffer. */
  int io_f;                     /* Print and scroll flags. */
};

/* Address range parameters. */
struct ed_region
{
  off_t start;                  /* Region start address. */
  off_t end;                    /* Region end address. */
  int addrs;                    /* Number of region addresses. */
};

/* Substitution parameters. */
struct ed_substitute
{
  regex_t *lhs;                 /* Substitution pattern buffer. */
  char *rhs;                    /* Substitution replacement buffer. */
  size_t rhs_size;              /* Substitution replacement buffer size. */
  size_t rhs_i;                 /* Substitution replacement buffer index. */
  off_t s_mod;                  /* Substitution match modulus. */
  off_t s_nth;                  /* Substitution match offset. */
  unsigned r_f;                 /* Substitution TGSR flag. */
  unsigned s_f;                 /* Substitution modifier flags. */
  unsigned sio_f;               /* Substitution I/O flags. */
};

#define DEFAULT_PROMPT "*"

/* Ed command parameters. */
struct ed_execute
{
  struct ed_substitute *subst;  /* Substitution parameters. */
  struct ed_region *region;     /* Address range parameters. */

  FILE *fp;                     /* Command script file pointer. */
  char *file_script;            /* File argument of `-f script' option. */
  char *pathname;               /* Concatenation of scripts. */
  const char *err;              /* Error message. */
  char *prompt;                 /* Interactive command prompt. */
  off_t line_no;                /* Script line number. */
  int have_key;                 /* If set, encrypt I/O. */
  int first_pass;               /* If set, first global command iteration. */
  int global;                   /* If set, global command (GLBL [| GLBI]). */
  int opt;                      /* Command-line options. */
  int status;                   /* Command status. */
};

struct ed_file
{
#ifdef WANT_FILE_LOCK
  FILE *handle;                 /* File handle. */
  INO_T inode;                  /* File inode. */
#endif  /* WANT_FILE_LOCK */

  glob_t *glob;                 /* Glob of file names. */
  glob_t *list;                 /* List of files to edit (copy of glob). */
  char *name;                   /* Name of current file. */
  size_t name_size;             /* Size of name buffer. */
  char *suffix;                 /* Filename suffix for option `-i'.  */
  int is_glob;                  /* Glob filename? */
  int is_writable;              /* If set, file open read-write. */
};

/* Ed command-line flags. */
enum ed_command_flags
{
  ANSI_COLOR        = 0x0001,   /* If set, filter ANSI color codes. */
  EXIT_ON_ERROR     = 0x0002,   /* If set, exit on errors. */
  IN_PLACE          = 0x0004,   /* If set, write file on EOF. */
  POSIXLY_CORRECT   = 0x0008,   /* If set, adhere to SUSv4, 2013 standards. */
  PRINT_CONFIG      = 0x0010,   /* If set, display configuration info. */
  PRINT_FIRST_FILE  = 0x0020,   /* If set, print first filename in a list. */
  PRINT_HELP        = 0x0040,   /* If set, display help. */
  PRINT_VERSION     = 0x0080,   /* If set, print ed version. */
  PROMPT            = 0x0100,   /* If set, print command prompt. */
  REGEX_EXTENDED    = 0x0200,   /* If set, use extended regexps. */
  RESTRICTED        = 0x0400,   /* If set, restricted mode enabled. */
  FSCRIPT           = 0x0800,   /* If set, script via option `-f'.. */
  SCRIPTED          = 0x1000,   /* If set, script mode enabled. */
  TRADITIONAL       = 0x2000,   /* If set, be backward compatible. */
  VERBOSE           = 0x4000    /* If set, print error diagnostics. */
};

/* Ed state parameters. */
typedef struct ed_buffer
{
  struct ed_state *state;       /* Array of buffer states. */
  struct ed_core *core;         /* Meta data and storage parameters. */
  struct ed_display *display;   /* Display parameters. */
  struct ed_execute *exec;      /* Command execution parameters. */
  struct ed_file *file;         /* File parameters. */

  char *input;                  /* Standard input pointer. */
} ed_buffer_t;

typedef int (*ed_command_t) (ed_buffer_t *);

/* Ed command-modifier flags. */
enum ed_modifier_flags
  {
    LIST = 0x00001,             /* Print chars graphically, e.g.,
                                   `jl'. */
    NMBR = 0x00002,             /* Print with line numbers, e.g.,
                                   `jn'. */
    PRNT = 0x00004,             /* Print resulting dot, e.g.,
                                   `jp'. */
    PRSW = 0x00008,             /* Printing switch. */
    SMLR = 0x00010,             /* Last match relative, e.g.,
                                   s/old/new/g$-3. */
    SNLR = 0x00020,             /* Last match relative, e.g.,
                                   s/old/new/$-1. */
    GSUB = 0x00040,             /* Substitute all, e.g.,
                                   `s/old/new/g'. */
    GSFQ = 0x00080,             /* Substitution frequency, e.g.,
                                   `s/old/new/g3'. */
    TGSG = 0x00100,             /* Toggle global substitution, e.g.,
                                   `sg'. */
    TGSF = 0x00200,             /* Toggle substitution frequency, e.g.,
                                   `sg3'. */
    TGSP = 0x00400,             /* Toggle print substitution, e.g.,
                                   `sp'. */
    TGSR = 0x00800,             /* Use pattern from last search, e.g.,
                                   `sr'. */
    SGPR = 0x01000,             /* Repeat last substitution, e.g.,
                                   `s'. */
    GLBL = 0x02000,             /* Global command, i.e.,
                                   'G', 'g', 'V' or 'v'. */
    GLBI = 0x04000,             /* Global interactive command, e.g.,
                                   'G' or 'V'. */
    ZFWD = 0x08000,             /* Scroll forward one screen, i.e.,
                                   `z'. */
    ZBWD = 0x10000,             /* Scroll backward one screen, i.e.,
                                   `Z'. */
    ZHFW = 0x20000,             /* Scroll forward one half screen, i.e.,
                                   `]'. */
    ZHBW = 0x40000,             /* Scroll backward one half screen, i.e.,
                                   `['. */
    OFFB = 0x80000              /* Frame buffer text offset. */
  };

/* Ed error flags. */
enum ed_command_error
  {
#ifndef EOF
    EOF      = -001,
#endif
    ERR      = -002,
    EMOD     = -003,
    FATAL    = -004,
    EOF_NEXT = -005,
    EOF_PREV = -006,
#ifdef WANT_SAFE_WRITE
    EOF_GLOB = -007
#endif
  };

#ifdef HAVE_REG_SYNTAX_T
  /*
   * Flags for GNU regular expressions.
   *
   * Historically, ed regular expressions are what POSIX refers to as
   * "basic". POSIX "extended" regular expressions can be enabled with
   * command-line option `-E' or `-r'.
   *
   * GNU extensions to "basic" regular expression syntax are disabled
   * for POSIX compatibility. Use extended regular expressions
   * instead.
   */
# define _RE_SYNTAX_ED_COMMON                                                 \
   (RE_CHAR_CLASSES |  RE_HAT_LISTS_NOT_NEWLINE | RE_INTERVALS |              \
    RE_NO_EMPTY_RANGES | RE_UNMATCHED_RIGHT_PAREN_ORD |                       \
    RE_NO_GNU_OPS)

# define RE_SYNTAX_ED_BASIC                                                   \
  (_RE_SYNTAX_ED_COMMON | RE_LIMITED_OPS)

# undef RE_SYNTAX_POSIX_BASIC
# define RE_SYNTAX_POSIX_BASIC                                                \
  (RE_SYNTAX_ED_BASIC | RE_DOT_NOT_NULL)

# define RE_SYNTAX_ED_EXTENDED                                                \
  (_RE_SYNTAX_ED_COMMON | RE_NO_BK_BRACES | RE_NO_BK_PARENS |                 \
   RE_NO_BK_VBAR)

# undef RE_SYNTAX_POSIX_EXTENDED
# define RE_SYNTAX_POSIX_EXTENDED                                             \
  (RE_SYNTAX_ED_EXTENDED | RE_DOT_NOT_NULL)
#endif  /* HAVE_REG_SYNTAX_T */

#define SE_MAX 30               /* max subexpressions in regexp */

/* Types of regexes. */
enum search_type
  {
    RE_SEARCH = 0,
    RE_SUBST = 1
  };

#define TAB_WIDTH 8             /* Console tab width. */

#if defined (HAVE_SIG_ATOMIC_T) && defined (HAVE_STDINT_H)
# define COLUMNS_MAX SIG_ATOMIC_MAX
# define ROWS_MAX SIG_ATOMIC_MAX
#else
# define COLUMNS_MAX 127        /* Lower limit of SIG_ATOMIC_MAX per C99. */
# define ROWS_MAX 127           /* Lower limit of SIG_ATOMIC_MAX per C99. */
#endif  /* !defined (HAVE_SIG_ATOMIC_T) || !defined (HAVE_STDINT_H) */

/* Assert: 256 <= BUFSIZ <= 512KB and that 2 ^ BUFSIZ_LOG2 == BUFSIZ */
#define BUFSIZ_LOG2                                                           \
  ((BUFSIZ & 0x100)   ?  8 :                                                  \
   (BUFSIZ & 0x200)   ?  9 :                                                  \
   (BUFSIZ & 0x400)   ? 10 :                                                  \
   (BUFSIZ & 0x800)   ? 11 :                                                  \
   (BUFSIZ & 0x1000)  ? 12 :                                                  \
   (BUFSIZ & 0x2000)  ? 13 :                                                  \
   (BUFSIZ & 0x4000)  ? 14 :                                                  \
   (BUFSIZ & 0x8000)  ? 15 :                                                  \
   (BUFSIZ & 0x10000) ? 16 :                                                  \
   (BUFSIZ & 0x20000) ? 17 :                                                  \
   (BUFSIZ & 0x40000) ? 18 :                                                  \
   (BUFSIZ & 0x80000) ? 19 : -1)

/* SUBSTITUTE_IN_STRING: Overwrite char `from' with `to' in s. */
#define SUBSTITUTE_IN_STRING(s, len, from, to)                                \
  do                                                                          \
    {                                                                         \
      char *t = (s);                                                          \
      size_t l = (len);                                                       \
      for (; l-- > 0; ++t)                                                    \
        if (*t == (from))                                                     \
          *t = (to);                                                          \
    }                                                                         \
  while (0)

/* NUL_TO_NEWLINE: Overwrite ASCII NULs with newlines. */
#define NUL_TO_NEWLINE(s, len)                                                \
  SUBSTITUTE_IN_STRING (s, len, '\0', '\n')

/* NEWLINE_TO_NUL: Overwrite newlines with ASCII NULs. */
#define NEWLINE_TO_NUL(s, len)                                                \
  SUBSTITUTE_IN_STRING (s, len, '\n', '\0')

/* INC_MOD: Increment l modulus k. */
#define INC_MOD(l, k)   ((l) == (k) ? 0 : (l) + 1)

/* DEC_MOD: Decrement l modulus k. */
#define DEC_MOD(l, k)   ((l) == 0 ? (k) : (l) - 1)

/* APPEND_UNDO_NODE: Push added line onto undo queue.  */
#define APPEND_UNDO_NODE(lp, up, addr, ed)                                    \
  do                                                                          \
    {                                                                         \
      if (up)                                                                 \
        (up)->t = (lp);                                                       \
      else if (!((up) = append_undo_node (UADD, (addr), (addr), (ed))))       \
        {                                                                     \
          spl0 ();                                                            \
          return ERR;                                                         \
        }                                                                     \
    }                                                                         \
  while (0)


/* LINK_NODES: Link prev and next nodes. */
#define LINK_NODES(prev, next)                                                \
  ((prev)->q_forw = (next), (next)->q_back = (prev))

/* INIT_DEQUE: Link node to itself. */
#define INIT_DEQUE(node) LINK_NODES ((node), (node))

/*
 * APPEND_NODE: Append node after prev.
 * CAVEAT UTILITOR: APPEND_NODE fails when `prev' of the form
 * p->q_back.  The workaround is to assign t = p->q_back and then
 * call APPEND_NODE(n, t).
 */
#define APPEND_NODE(node, prev)                                               \
  do                                                                          \
    {                                                                         \
      LINK_NODES ((node), (prev)->q_forw);                                    \
      LINK_NODES ((prev), (node));                                            \
    }                                                                         \
  while (0)

/* UNLINK_NODE: Remove node from deque. */
#define UNLINK_NODE(node)                                                     \
  LINK_NODES ((node)->q_back, (node)->q_forw)

/* REVERSE_LINKS: Reverse direction node pointers. */
#define REVERSE_LINKS(node, node_type)                                        \
  do                                                                          \
    {                                                                         \
      node_type *_np;                                                         \
      _np = (node)->q_forw;                                                   \
      (node)->q_forw = (node)->q_back;                                        \
      (node)->q_back = _np;                                                   \
    }                                                                         \
  while (0)

/* REALLOC_THROW: Assure at least a minimum size for buffer b. */
#define REALLOC_THROW(b, n, i, err, ed)                                       \
  do                                                                          \
    {                                                                         \
      if (!realloc_buffer ((void **) &(b), &(n), (size_t) (i), (ed)))         \
        return (err);                                                         \
    }                                                                         \
  while (0)

/* SKIP_WHITESPACE: Scan command buffer for next non-space char. */
#define SKIP_WHITESPACE(ed)                                                   \
  do                                                                          \
    {                                                                         \
      while (isspace ((unsigned char) *(ed)->input) && *(ed)->input != '\n')  \
        ++(ed)->input;                                                        \
    }                                                                         \
  while (0)

#ifdef HAVE_STRTOL
  /* STRTOL_THROW: Convert a string to long, or return err. */
# define STRTOL_THROW(i, s, sp, errval)                                       \
  do                                                                          \
    {                                                                         \
      long _n;                                                                \
      errno = 0;                                                              \
      if (((_n = strtol ((s), (sp), 10)) == 0 && errno == EINVAL)             \
          || ((_n == LONG_MIN || _n == LONG_MAX) && errno == ERANGE))         \
        {                                                                     \
          ed->exec->err = _("Numerical result out of range");                 \
          (i) = 0;                                                            \
          return errval;                                                      \
        }                                                                     \
      (i) = _n;                                                               \
    }                                                                         \
  while (0)
#else
  /* STRTOL_THROW: Convert a string to long, or return err. */
# define STRTOL_THROW(i, s, sp, errval)                                       \
  do                                                                          \
    {                                                                         \
      long _n = 0;                                                            \
      char *_s = *(s) == '-' ? (s) + 1 : (s);                                 \
                                                                              \
      /* No error handling! */                                                \
      while (isdigit (*_s))                                                   \
        _n = _n * 10 + *_s++ - '0';                                           \
      if (sp)                                                                 \
        *(sp) = _s;                                                           \
      (i) = *(s) == '-' ? -_n : _n;                                           \
    }                                                                         \
  while (0)
#endif  /* !HAVE_STRTOL */

#ifdef HAVE_STRTOLL
  /* STRTOLL_THROW: Convert a string to long long, or return err. */
# define STRTOLL_THROW(i, s, sp, errval)                                      \
  do                                                                          \
    {                                                                         \
      long long _n;                                                           \
      errno = 0;                                                              \
      if (((_n = strtoll ((s), (sp), 10)) == 0 && errno == EINVAL)            \
          || ((_n == LLONG_MIN || _n == LLONG_MAX) && errno == ERANGE))       \
        {                                                                     \
          ed->exec->err = _("Numerical result out of range");                 \
          (i) = 0;                                                            \
          return errval;                                                      \
        }                                                                     \
      (i) = _n;                                                               \
    }                                                                         \
  while (0)
#else
  /* STRTOLL_THROW: Convert a string to long long, or return err. */
# define STRTOLL_THROW(i, s, sp, errval)                                      \
  do                                                                          \
    {                                                                         \
      long long _n = 0;                                                       \
      char *_s = (s);                                                         \
                                                                              \
      /* No error handling! */                                                \
      while ('0' < *_s && *_s < '9')                                          \
        _n = _n * 10 + *_s++ - '0';                                           \
      if (sp)                                                                 \
        *(sp) = _s;                                                           \
      (i) = _n;                                                               \
    }                                                                         \
  while (0)
#endif  /* !HAVE_STRTOLL */

#ifdef HAVE_STRTOUL
  /* STRTOUL_THROW: Convert a string to unsigned long, or return err. */
# define STRTOUL_THROW(i, s, sp, errval)                                      \
  do                                                                          \
    {                                                                         \
      unsigned long _n;                                                       \
      errno = 0;                                                              \
      if (((_n = strtoul ((s), (sp), 10)) == 0  && errno == EINVAL)           \
          || (_n == ULONG_MAX && errno == ERANGE))                            \
        {                                                                     \
          ed->exec->err = _("Numerical result out of range");                 \
          (i) = 0;                                                            \
          return errval;                                                      \
        }                                                                     \
      (i) = _n;                                                               \
    }                                                                         \
  while (0)
#else
  /* STRTOUL_THROW: Convert a string to unsigned long, or return err. */
# define STRTOUL_THROW(i, s, sp, errval)                                      \
  do                                                                          \
    {                                                                         \
      unsigned long _n = 0;                                                   \
      char *_s = (s);                                                         \
                                                                              \
      /* No error handling! */                                                \
      while ('0' < *_s && *_s < '9')                                          \
        _n = _n * 10 + *_s++ - '0';                                           \
      if (sp)                                                                 \
        *(sp) = _s;                                                           \
      (i) = _n;                                                               \
    }                                                                         \
  while (0)
#endif  /* !HAVE_STRTOUL */

/* Global function declarations. */
void activate_signals (void);
int address_offset (off_t *, ed_buffer_t *);
int address_range (ed_buffer_t *);
ed_buffer_t *alloc_ed_buffer (void);

#ifdef WANT_ED_REGISTER
int append_from_register (off_t, ed_buffer_t *);
#endif

int append_lines (off_t, ed_buffer_t *);

#ifdef WANT_SCRIPT_FLAGS
int append_script_expression (const char *, ed_buffer_t *);
int append_script_file (char *, ed_buffer_t *);
#endif

ed_line_node_t *append_line_node (size_t, off_t, off_t, ed_buffer_t *);
ed_text_node_t *append_text_node (ed_text_node_t *, const char *, size_t);

#ifdef WANT_ED_REGISTER
int append_to_register (off_t, off_t, int, ed_buffer_t *);
#endif

ed_undo_node_t *append_undo_node (int, off_t, off_t, ed_buffer_t *);
int close_ed_buffer (ed_buffer_t *);
int copy_lines (off_t, off_t, off_t, ed_buffer_t *);
int create_disk_buffer (FILE **, char **, ed_buffer_t *);
int decode_utf8_char (unsigned char **, int);
void delete_global_nodes  (const ed_line_node_t *, const ed_line_node_t *,
                           ed_buffer_t *);
int delete_lines (off_t, off_t, ed_buffer_t *);
int display_lines (off_t, off_t, ed_buffer_t *);
int encode_utf8_char (char *, int *, unsigned int);
int exec_command (ed_buffer_t *);
int exec_global (ed_buffer_t *);
char *expand_shell_command (size_t *, int *, ed_buffer_t *);
char *file_glob (size_t *, int, int, int, ed_buffer_t *);
char *file_name (size_t *, ed_buffer_t *);

#ifdef WANT_EXTERNAL_FILTER
int filter_lines (off_t, off_t, const char *, ed_buffer_t *);
#endif

#ifdef WANT_DES_ENCRYPTION
int flush_des_file (FILE *);
#endif

char *get_buffer_line (const ed_line_node_t *, ed_buffer_t *);
regex_t *get_compiled_regex (unsigned, int, ed_buffer_t *);

#ifdef WANT_DES_ENCRYPTION
int get_des_char (FILE *, ed_buffer_t *);
int get_des_keyword (ed_buffer_t *);
#endif

char *get_extended_line (size_t *, int, int, ed_buffer_t *);
ed_line_node_t *get_line_node (off_t, ed_buffer_t *);
int get_line_node_address (const ed_line_node_t *, off_t *, ed_buffer_t *);
int get_marked_node_address (int, off_t *, ed_buffer_t *);
int get_matching_node_address (const regex_t *, int, off_t *, ed_buffer_t *);
size_t get_path_max (const char *);
#define get_stdin_line(len, ed) get_stream_line (stdin, len, ed)
char *get_stream_line (FILE *, size_t *, ed_buffer_t *);

#ifdef WANT_DES_ENCRYPTION
void init_des_cipher (void);
#endif

void init_ed_command (int, ed_buffer_t *);
void init_ed_state (off_t, struct ed_state *);
void init_global_queue (ed_global_node_t **, ed_line_node_t **, ed_buffer_t *);
#ifdef WANT_ED_REGISTER
int init_register_queue (int, ed_buffer_t *);
#endif

int init_signal_handler (ed_buffer_t *);
int init_stdio (ed_buffer_t *);
void init_substitute (regex_t **, unsigned *, off_t *, off_t *,
                      unsigned *, struct ed_substitute *);
void init_text_deque (ed_text_node_t *);
void init_undo_queue (ed_undo_node_t **, ed_buffer_t *);

#ifdef WANT_ED_REGISTER
int inter_register_copy (int, ed_buffer_t *);
int inter_register_move (int, ed_buffer_t *);
#endif

int is_utf8_str (const char *, size_t);
int join_lines (off_t, off_t, ed_buffer_t *);
int mark_global_nodes (int, ed_buffer_t *);
int mark_line_node (const ed_line_node_t *, int, ed_buffer_t *);
int move_lines (off_t, off_t, off_t, ed_buffer_t *);
int next_address (off_t *, ed_buffer_t *);
int one_time_init (int, char **, ed_buffer_t *);

#ifdef WANT_ED_MACRO
int pop_stack_frame (ed_buffer_t *);
#endif

char *pop_text_node (ed_text_node_t *, size_t *);
char *put_buffer_line (const char *, size_t, ed_buffer_t *);

#ifdef WANT_ED_MACRO
int push_stack_frame (ed_buffer_t *);
#endif

#ifdef WANT_DES_ENCRYPTION
int put_des_char (int, FILE *);
#endif

void quit (int, ed_buffer_t *);
int read_file (const char *, off_t, off_t *, off_t *, int, ed_buffer_t *);
int read_pipe (const char *, off_t, off_t *, off_t *, ed_buffer_t *);
int read_stream_r (FILE *, off_t, off_t *, ed_buffer_t *);
void *realloc_buffer (void **, size_t *, size_t, ed_buffer_t *);
char *regular_expression (unsigned, size_t *, ed_buffer_t *);
int reopen_ed_buffer (ed_buffer_t *);
void reset_global_queue (ed_buffer_t *);

#ifdef WANT_ED_REGISTER
int reset_register_queue (int, ed_buffer_t *);
#endif

void reset_undo_queue (ed_buffer_t *);
int resubstitute (off_t *, off_t *, unsigned *, unsigned *, ed_buffer_t *);
void save_substitute (regex_t *, unsigned, off_t, off_t, unsigned,
                      struct ed_substitute *);
#ifdef WANT_ED_MACRO
int script_from_register  (ed_buffer_t *);
#endif

#ifdef WANT_ED_SCROLL
int scroll_lines (off_t, off_t, ed_buffer_t *);
#endif

#ifdef WANT_FILE_LOCK
int set_file_lock (FILE *, int);
#endif

char *shift_text_node (ed_text_node_t *, size_t *);
void signal_handler (int);
void spl0 (void);
void spl1 (void);
int substitute_lines (off_t, off_t, regex_t *, off_t, off_t, unsigned,
                      ed_buffer_t *);
int substitution_lhs (regex_t **, unsigned *, ed_buffer_t *);
int substitution_rhs (off_t *, off_t *, unsigned *, unsigned *, ed_buffer_t *);
int trailing_escapes (const char *, const char *);
void transfer_marks (const ed_line_node_t *, const ed_line_node_t *,
                     ed_buffer_t *);
int undo_last_command (ed_buffer_t *);
int unwind_stack_frame (int, ed_buffer_t *);
void unmark_line_node (const ed_line_node_t *, ed_buffer_t *);
int utf8_char_size (const char *, size_t);
int utf8_char_display_width (const char *, int);
int utf8_strlen (const char *, size_t);
int write_file (const char *, int, off_t, off_t, off_t *, off_t *,
                const char *, ed_buffer_t *);
int write_pipe (const char *, off_t, off_t, off_t *, off_t *, ed_buffer_t *);
int write_stream (FILE *, ed_line_node_t *, off_t, off_t *, ed_buffer_t *);
