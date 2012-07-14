/* Test of pipe2.
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>

#include <unistd.h>

#include "signature.h"
SIGNATURE_CHECK (pipe2, int, (int[2], int));

#include <fcntl.h>
#include <stdbool.h>

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
/* Get declarations of the Win32 API functions.  */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include "binary-io.h"
#include "macros.h"

/* Return true if FD is open.  */
static bool
is_open (int fd)
{
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
  /* On Win32, the initial state of unassigned standard file
     descriptors is that they are open but point to an
     INVALID_HANDLE_VALUE, and there is no fcntl.  */
  return (HANDLE) _get_osfhandle (fd) != INVALID_HANDLE_VALUE;
#else
# ifndef F_GETFL
#  error Please port fcntl to your platform
# endif
  return 0 <= fcntl (fd, F_GETFL);
#endif
}

/* Return true if FD is not inherited to child processes.  */
static bool
is_cloexec (int fd)
{
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
  HANDLE h = (HANDLE) _get_osfhandle (fd);
  DWORD flags;
  ASSERT (GetHandleInformation (h, &flags));
  return (flags & HANDLE_FLAG_INHERIT) == 0;
#else
  int flags;
  ASSERT ((flags = fcntl (fd, F_GETFD)) >= 0);
  return (flags & FD_CLOEXEC) != 0;
#endif
}

/* Return true if FD is in non-blocking mode.  */
static bool
is_nonblocking (int fd)
{
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
  /* We don't use the non-blocking mode for sockets here.  */
  return 0;
#else
  int flags;
  ASSERT ((flags = fcntl (fd, F_GETFL)) >= 0);
  return (flags & O_NONBLOCK) != 0;
#endif
}

int
main ()
{
  int use_nonblocking;
  int use_cloexec;

#if !((defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__)
  for (use_nonblocking = 0; use_nonblocking <= 1; use_nonblocking++)
#else
  use_nonblocking = 0;
#endif
#if defined O_CLOEXEC
    for (use_cloexec = 0; use_cloexec <= 1; use_cloexec++)
#else
    use_cloexec = 0;
#endif
      {
        int o_flags;
        int fd[2];

        o_flags = 0;
#if !((defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__)
        if (use_nonblocking)
          o_flags |= O_NONBLOCK;
#endif
#if defined O_CLOEXEC
        if (use_cloexec)
          o_flags |= O_CLOEXEC;
#endif

        fd[0] = -1;
        fd[1] = -1;
        ASSERT (pipe2 (fd, o_flags) >= 0);
        ASSERT (fd[0] >= 0);
        ASSERT (fd[1] >= 0);
        ASSERT (fd[0] != fd[1]);
        ASSERT (is_open (fd[0]) >= 0);
        ASSERT (is_open (fd[1]) >= 0);
        if (use_cloexec)
          {
            ASSERT (is_cloexec (fd[0]));
            ASSERT (is_cloexec (fd[1]));
          }
        else
          {
            ASSERT (!is_cloexec (fd[0]));
            ASSERT (!is_cloexec (fd[1]));
          }
        if (use_nonblocking)
          {
            ASSERT (is_nonblocking (fd[0]));
            ASSERT (is_nonblocking (fd[1]));
          }
        else
          {
            ASSERT (!is_nonblocking (fd[0]));
            ASSERT (!is_nonblocking (fd[1]));
          }
      }

  return 0;
}
