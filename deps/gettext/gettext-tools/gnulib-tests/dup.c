/* Duplicate an open file descriptor.

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

/* Specification.  */
#include <unistd.h>

#include <errno.h>

#include "msvc-inval.h"

#undef dup

#if HAVE_MSVC_INVALID_PARAMETER_HANDLER
static int
dup_nothrow (int fd)
{
  int result;

  TRY_MSVC_INVAL
    {
      result = dup (fd);
    }
  CATCH_MSVC_INVAL
    {
      result = -1;
      errno = EBADF;
    }
  DONE_MSVC_INVAL;

  return result;
}
#else
# define dup_nothrow dup
#endif

int
rpl_dup (int fd)
{
  int result = dup_nothrow (fd);
#if REPLACE_FCHDIR
  if (result >= 0)
    result = _gl_register_dup (fd, result);
#endif
  return result;
}
