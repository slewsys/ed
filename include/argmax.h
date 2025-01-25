/* argmax.h: ARGV limits for ed line editor.
 *
 * SPDX-FileCopyrightText: 1993-2025 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research
 *
 * SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later OR MIT
 */

#include "config.h"

#ifndef _ARGMAX_H
#define _ARGMAX_H

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_SYSLIMITS_H
# include <sys/syslimits.h>
#endif

#ifdef HAVE_SYSCONF
# if !defined (ARG_MAX) && defined (_SC_ARG_MAX)
#  define ARG_MAX (sysconf (_SC_ARG_MAX) < 1 ? 4096                           \
                   : sysconf (_SC_ARG_MAX))
# endif  /* !defined (ARG_MAX) && defined (_PC_ARG_MAX) */
#endif   /* HAVE_SYSCONF */

#if !defined (ARG_MAX) && !defined (NCARGS) && defined (HAVE_SYS_LIMITS_H)
# include <sys/limits.h>
#endif

#if !defined (ARG_MAX) && !defined (NCARGS) && defined (HAVE_SYS_PARAM_H)
# include <sys/param.h>
#endif

#if !defined (ARG_MAX) && !defined (NCARGS) && defined (HAVE_LINUX_LIMITS_H)
# include <linux/limts.h>
#endif

#if !defined (ARG_MAX) && defined (NCARGS)
# define ARG_MAX NCARGS
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if !defined (ARG_MAX) && defined (_POSIX_ARG_MAX)
# define ARG_MAX _POSIX_ARG_MAX
#endif

/* Fall back to _POSIX_ARG_MAX */
#if !defined (ARG_MAX)
# define ARG_MAX 4096
#endif

#endif /* _ARGMAX_H */
