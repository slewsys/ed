/* pathmax.h: Pathname limits for ed line editor.

   Copyright © 1993-2014 Andrew L. Moore <slewsys@gmail.com>, SlewSys Research

   This file is part of ed.

   Ed is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Ed is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with ed; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


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
