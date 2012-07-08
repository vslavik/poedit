/* Test of <fcntl.h> substitute.
   Copyright (C) 2007, 2009, 2010 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <fcntl.h>

/* Check that the various O_* macros are defined.  */
int o = O_DIRECT | O_DIRECTORY | O_DSYNC | O_NDELAY | O_NOATIME | O_NONBLOCK
        | O_NOCTTY | O_NOFOLLOW | O_NOLINKS | O_RSYNC | O_SYNC | O_TTY_INIT
        | O_BINARY | O_TEXT;

/* Check that the various SEEK_* macros are defined.  */
int sk[] = { SEEK_CUR, SEEK_END, SEEK_SET };

/* Check that the FD_* macros are defined.  */
int fd = FD_CLOEXEC;

int
main (void)
{
  return 0;
}
