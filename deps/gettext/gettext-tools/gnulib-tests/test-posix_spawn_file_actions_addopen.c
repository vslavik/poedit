/* Test posix_spawn_file_actions_addopen() function.
   Copyright (C) 2011-2013 Free Software Foundation, Inc.

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

#include <config.h>

#include <spawn.h>

#include "signature.h"
SIGNATURE_CHECK (posix_spawn_file_actions_addopen, int,
                 (posix_spawn_file_actions_t *, int,
                  const char *, int, mode_t));

#include <errno.h>
#include <fcntl.h>

#include "macros.h"

int
main (void)
{
  posix_spawn_file_actions_t actions;

  ASSERT (posix_spawn_file_actions_init (&actions) == 0);

  /* Test behaviour for invalid file descriptors.  */
  {
    errno = 0;
    ASSERT (posix_spawn_file_actions_addopen (&actions, -1,
                                              "foo", 0, O_RDONLY)
            == EBADF);
  }
  {
    errno = 0;
    ASSERT (posix_spawn_file_actions_addopen (&actions, 10000000,
                                              "foo", 0, O_RDONLY)
            == EBADF);
  }

  return 0;
}
