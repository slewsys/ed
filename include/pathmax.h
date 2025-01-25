/* pathmax.h: Pathname limits for ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "config.h"

#ifndef _PATHMAX_H
#define _PATHMAX_H

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef HAVE_PATHCONF
# if !defined PATH_MAX && defined _PC_PATH_MAX
#  define PATH_MAX (pathconf ("/", _PC_PATH_MAX) < 1 ? 1024                   \
                    : pathconf ("/", _PC_PATH_MAX))
# endif  /* !defined PATH_MAX && defined _PC_PATH_MAX */
#endif   /* HAVE_PATHCONF */

#if !defined PATH_MAX && !defined MAXPATHLEN && defined HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#if !defined PATH_MAX && defined MAXPATHLEN
# define PATH_MAX MAXPATHLEN
#endif

#if !defined PATH_MAX && defined _XOPEN_PATH_MAX
# define PATH_MAX _XOPEN_PATH_MAX
#endif

#if !defined PATH_MAX && defined _POSIX_PATH_MAX
# define PATH_MAX _POSIX_PATH_MAX
#endif

/* Fall back to _POSIX_PATH_MAX */
#if !defined PATH_MAX
# define PATH_MAX 256
#endif

#endif /* _PATHMAX_H */
