/* signal.c: Signal handlers for the ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "ed.h"

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif


/* Signal handlers, indexed by signal number (less one), initialized
   to default signal, SIG_DFL == (signal_t) 0. */
signal_t sighandler[NSIG];


/* Static function declarations. */
static void handle_hup (int);
static void handle_int (int);
static void handle_sigpipe (int);
static void handle_winch (int);
static signal_t reliable_signal (int, signal_t, void *);


/* Global declarations */
int _sigactive = 0;
int _sigflags = 0;


void
activate_signals (void)
{
  _sigactive = 1;
}


static void
handle_hup (int signo)
{
  extern ed_buffer_t *ed;

  char template[] = "ed.hup";
  char *hup = NULL;             /* hup file name */
  char *s;
  off_t addr = 0;
  off_t size;
  size_t len;
  int m = 0;

  if (!_sigactive)
    quit (1, ed);
  _sigflags &= ~(1 << (signo - 1));
  if (ed->state->lines
      && write_file ("ed.hup", 0, 1, ed->state->lines,
                     &addr, &size, "w", ed) < 0
      && (s = getenv ("HOME")))
    {
      m = *(s + (len = strlen (s)) - 1) != '/';
      if (!(hup = (char *) malloc ((size_t) (len + m + sizeof template))))
        quit (2, ed);
      memcpy (hup, s, len);
      memcpy (hup + len, m ? "/" : "", 1);
      memcpy (hup + len + m, template, sizeof template);
      write_file (hup, 0, 1, ed->state->lines, &addr, &size, "w", ed);
    }
  quit (2, ed);
}


static void
handle_int (int signo)
{
  extern ed_buffer_t *ed;

  if (!_sigactive)
    quit (1, ed);
  _sigflags &= ~(1 << (signo - 1));
  LONGJMP (env, -1);
}


static void
handle_sigpipe (int signo)
{
  extern ed_buffer_t *ed;

  if (!_sigactive)
    quit (1, ed);
  _sigflags &= ~(1 << (signo - 1));

  /* Interrupted syscall should report broken pipe. */
}


static void
handle_winch (int signo)
{
  extern ed_buffer_t *ed;

  int saved_errno = errno;

#ifdef TIOCGWINSZ
  struct winsize ws;            /* window size structure */
#endif

  if (_sigactive)
    _sigflags &= ~(1 << (signo - 1));
#ifdef TIOCGWINSZ
  if (ioctl (0, TIOCGWINSZ, &ws) >= 0)
    {
      /* Sanity check window size values.  */
      if (1 < ws.ws_row && ws.ws_row < ROWS_MAX)
        ed->display->ws_row = ws.ws_row;
      if (TAB_WIDTH < ws.ws_col && ws.ws_col < COLUMNS_MAX)
        ed->display->ws_col = ws.ws_col;
    }
#endif

  errno = saved_errno;
}


int
init_signal_handler (ed_buffer_t *ed)
{
#ifdef HAVE_SIGACTION
  sigset_t mask;

  sigemptyset (&mask);
  sigaddset (&mask, SIGINT);
  sigaddset (&mask, SIGQUIT);
#else
    int mask = 0;
#endif  /* !HAVE_SIGACTION */

  /* Override signo-indexed LUT for handlers of interest. */
  sighandler[SIGHUP - 1] = handle_hup;
  sighandler[SIGPIPE - 1] = handle_sigpipe;
  sighandler[SIGINT - 1] = handle_int;
#ifdef SIGWINCH
  sighandler[SIGWINCH - 1] = handle_winch;
#endif

  /* Register handlers of interest. */
  if (reliable_signal(SIGCHLD, SIG_DFL, &mask) == SIG_ERR ||
      reliable_signal(SIGHUP, signal_handler, &mask) == SIG_ERR ||
      reliable_signal(SIGPIPE, signal_handler, &mask) == SIG_ERR
#ifdef SIGWINCH
      || (isatty(0) && reliable_signal(SIGWINCH, signal_handler, &mask) == SIG_ERR)
#endif
     )
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Signal error");
      return ERR;
    }

#ifdef HAVE_SIGACTION
  sigdelset (&mask, SIGINT);
#endif
  if (reliable_signal (SIGINT, signal_handler, &mask) == SIG_ERR)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Signal error");
      return ERR;
    }

#ifdef HAVE_SIGACTION
  sigdelset (&mask, SIGQUIT);
  sigaddset (&mask, SIGINT);
#endif
  if (reliable_signal (SIGQUIT, SIG_IGN, &mask) == SIG_ERR)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Signal error");
      return ERR;
    }

  return 0;
}

/*
 * init_parent_signal_handler: Mask SIGCHLD; ignore SIGINT and
 *     SIGQUIT.
 */
int
init_parent_signal_handler (ed_buffer_t *ed)
{
#ifdef HAVE_SIGACTION
  sigset_t mask;

  sigemptyset (&mask);
  sigaddset (&mask, SIGCHLD);
#else
    int mask = 0;
#endif  /* !HAVE_SIGACTION */

  if (reliable_signal (SIGINT, SIG_IGN, &mask) == SIG_ERR
      || reliable_signal (SIGQUIT, SIG_IGN, &mask) == SIG_ERR)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Signal error");
      return ERR;
    }
  return 0;
}


/*
 * init_child_signal_handler: Restore signal defaults.
 */
int
init_child_signal_handler (ed_buffer_t *ed)
{
#ifdef HAVE_SIGACTION
  sigset_t mask;

  sigemptyset (&mask);
#else
  int mask = 0;
#endif  /* !HAVE_SIGACTION */

  /* Restore defaults in child process. */
  if (reliable_signal (SIGCHLD, SIG_DFL, &mask) == SIG_ERR
      || reliable_signal (SIGHUP, SIG_DFL, &mask) == SIG_ERR
      || reliable_signal (SIGINT, SIG_DFL, &mask) == SIG_ERR
      || reliable_signal (SIGPIPE, SIG_DFL, &mask) == SIG_ERR
      || reliable_signal (SIGQUIT, SIG_DFL, &mask) == SIG_ERR)
    {
      fprintf (stderr, "%s\n", strerror (errno));
      ed->exec->err = _("Signal error");
      return ERR;
    }
  return 0;
}


static signal_t
reliable_signal (int signo, signal_t handler, void *mask)
{
#ifndef HAVE_SIGACTION
  return signal (signo, handler);
#else
  struct sigaction sa, osa;

  sa.sa_handler = handler;
  sa.sa_mask = *(sigset_t *) mask;
  sa.sa_flags = 0;
# ifdef SA_RESTART
  sa.sa_flags |= SA_RESTART;
# endif  /* SA_RESTART */
  return (sigaction (signo, &sa, &osa) < 0 ? SIG_ERR : osa.sa_handler);
#endif  /* HAVE_SIGACTION */
}


/* Global declarations */
int _mutex = 0;


/* signal_handler: If signaling blocked (see spl1), register for
   handling when signaling resumed. Otherwise, call signal-indexed
   handler. */
void
signal_handler (int signo)
{
  /* Assert: _sigactive == 1 */
  if (_mutex)
    _sigflags |= (1 << (signo - 1));
  else
    (sighandler[signo - 1]) (signo);
}


/* sp11: Raise priority level and disable signals. */
void
spl1 (void)
{
  /* Assert: _sigactive == 1 */
  ++_mutex;
}


/* spl0: Lower priority level and enable signals if level is nominal. */
void
spl0 (void)
{
  /* Assert: _sigactive == 1 */
  if (!--_mutex)
    {
      if ((_sigflags & (1 << (SIGHUP - 1)))) handle_hup (SIGHUP);
      if ((_sigflags & (1 << (SIGINT - 1)))) handle_int (SIGINT);
#ifdef SIGWINCH
      if ((_sigflags & (1 << (SIGWINCH - 1)))) handle_winch (SIGWINCH);
#endif
    }
}
