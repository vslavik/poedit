/* Auxiliary code for filtering of data through a subprocess.
   Copyright (C) 2001-2003, 2008-2010 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2009.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif

/* We use a child process, and communicate through a bidirectional pipe.
   To avoid deadlocks, let the child process decide when it wants to read
   or to write, and let the parent behave accordingly.  The parent uses
   select() to know whether it must write or read.  On platforms without
   select(), we use non-blocking I/O.  (This means the parent is busy
   looping while waiting for the child.  Not good.  But hardly any platform
   lacks select() nowadays.)  */

/* On BeOS select() works only on sockets, not on normal file descriptors.  */
#ifdef __BEOS__
# undef HAVE_SELECT
#endif

#ifdef EINTR

/* EINTR handling for close(), read(), write(), select().
   These functions can return -1/EINTR even though we don't have any
   signal handlers set up, namely when we get interrupted via SIGSTOP.  */

static inline int
nonintr_close (int fd)
{
  int retval;

  do
    retval = close (fd);
  while (retval < 0 && errno == EINTR);

  return retval;
}
#undef close /* avoid warning related to gnulib module unistd */
#define close nonintr_close

static inline ssize_t
nonintr_read (int fd, void *buf, size_t count)
{
  ssize_t retval;

  do
    retval = read (fd, buf, count);
  while (retval < 0 && errno == EINTR);

  return retval;
}
#define read nonintr_read

static inline ssize_t
nonintr_write (int fd, const void *buf, size_t count)
{
  ssize_t retval;

  do
    retval = write (fd, buf, count);
  while (retval < 0 && errno == EINTR);

  return retval;
}
#undef write /* avoid warning on VMS */
#define write nonintr_write

# if HAVE_SELECT

static inline int
nonintr_select (int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                struct timeval *timeout)
{
  int retval;

  do
    retval = select (n, readfds, writefds, exceptfds, timeout);
  while (retval < 0 && errno == EINTR);

  return retval;
}
#  undef select /* avoid warning on VMS */
#  define select nonintr_select

# endif

#endif

/* Non-blocking I/O.  */
#ifndef O_NONBLOCK
# define O_NONBLOCK O_NDELAY
#endif
#if HAVE_SELECT
# define IS_EAGAIN(errcode) 0
#else
# ifdef EWOULDBLOCK
#  define IS_EAGAIN(errcode) ((errcode) == EAGAIN || (errcode) == EWOULDBLOCK)
# else
#  define IS_EAGAIN(errcode) ((errcode) == EAGAIN)
# endif
#endif
