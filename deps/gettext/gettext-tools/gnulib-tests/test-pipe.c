/* Test of create_pipe_bidi/wait_subprocess.
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

#include "pipe.h"
#include "wait-process.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Depending on arguments, this test intentionally closes stderr or
   starts life with stderr closed.  So, we arrange to have fd 10
   (outside the range of interesting fd's during the test) set up to
   duplicate the original stderr.  */

#define BACKUP_STDERR_FILENO 10
#define ASSERT_STREAM myerr
#include "macros.h"

static FILE *myerr;

/* Code executed by the child process.  argv[1] = "child".  */
static int
child_main (int argc, char *argv[])
{
  char buffer[2] = { 's', 't' };
  int fd;
  int ret;

  ASSERT (argc == 3);

  /* Read one byte from fd 0, and write its value plus one to fd 1.
     fd 2 should be closed iff the argument is 1.  Check that no other file
     descriptors leaked.  */

  ASSERT (read (STDIN_FILENO, buffer, 2) == 1);

  buffer[0]++;
  ASSERT (write (STDOUT_FILENO, buffer, 1) == 1);

  errno = 0;
  ret = dup2 (STDERR_FILENO, STDERR_FILENO);
  switch (atoi (argv[2]))
    {
    case 0:
      /* Expect fd 2 is open.  */
      ASSERT (ret == STDERR_FILENO);
      break;
    case 1:
      /* Expect fd 2 is closed.  */
      ASSERT (ret == -1);
      ASSERT (errno == EBADF);
      break;
    default:
      ASSERT (false);
    }

  for (fd = 3; fd < 7; fd++)
    {
      errno = 0;
      ASSERT (close (fd) == -1);
      ASSERT (errno == EBADF);
    }

  return 0;
}

/* Create a bi-directional pipe to a test child, and validate that the
   child program returns the expected output.  The child is the same
   program as the parent ARGV0, but with different arguments.
   STDERR_CLOSED is true if we have already closed fd 2.  */
static void
test_pipe (const char *argv0, bool stderr_closed)
{
  int fd[2];
  char *argv[4];
  pid_t pid;
  char buffer[2] = { 'a', 't' };

  /* Set up child.  */
  argv[0] = (char *) argv0;
  argv[1] = (char *) "child";
  argv[2] = (char *) (stderr_closed ? "1" : "0");
  argv[3] = NULL;
  pid = create_pipe_bidi (argv0, argv0, argv, false, true, true, fd);
  ASSERT (0 <= pid);
  ASSERT (STDERR_FILENO < fd[0]);
  ASSERT (STDERR_FILENO < fd[1]);

  /* Push child's input.  */
  ASSERT (write (fd[1], buffer, 1) == 1);
  ASSERT (close (fd[1]) == 0);

  /* Get child's output.  */
  ASSERT (read (fd[0], buffer, 2) == 1);

  /* Wait for child.  */
  ASSERT (wait_subprocess (pid, argv0, true, false, true, true, NULL) == 0);
  ASSERT (close (fd[0]) == 0);

  /* Check the result.  */
  ASSERT (buffer[0] == 'b');
  ASSERT (buffer[1] == 't');
}

/* Code executed by the parent process.  */
static int
parent_main (int argc, char *argv[])
{
  int test;
  int fd;

  ASSERT (argc == 2);

  /* Selectively close various standard fds, to verify the child process is
     not impacted by this.  */
  test = atoi (argv[1]);
  switch (test)
    {
    case 0:
      break;
    case 1:
      close (0);
      break;
    case 2:
      close (1);
      break;
    case 3:
      close (0);
      close (1);
      break;
    case 4:
      close (2);
      break;
    case 5:
      close (0);
      close (2);
      break;
    case 6:
      close (1);
      close (2);
      break;
    case 7:
      close (0);
      close (1);
      close (2);
      break;
    default:
      ASSERT (false);
    }

  /* Plug any file descriptor leaks inherited from outside world before
     starting, so that child has a clean slate (at least for the fds that we
     might be manipulating).  */
  for (fd = 3; fd < 7; fd++)
    close (fd);

  test_pipe (argv[0], test >= 4);

  return 0;
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      fprintf (stderr, "%s: need arguments\n", argv[0]);
      return 2;
    }
  if (strcmp (argv[1], "child") == 0)
    {
      /* fd 2 might be closed, but fd BACKUP_STDERR_FILENO is the original
         stderr.  */
      myerr = fdopen (BACKUP_STDERR_FILENO, "w");
      if (!myerr)
        return 2;
      return child_main (argc, argv);
    }
  /* We might close fd 2 later, so save it in fd 10.  */
  if (dup2 (STDERR_FILENO, BACKUP_STDERR_FILENO) != BACKUP_STDERR_FILENO
      || (myerr = fdopen (BACKUP_STDERR_FILENO, "w")) == NULL)
    return 2;
  return parent_main (argc, argv);
}
