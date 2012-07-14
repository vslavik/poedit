/* Test of snprintf() function.
   Copyright (C) 2007-2010 Free Software Foundation, Inc.

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

#include <stdio.h>

#include "signature.h"
SIGNATURE_CHECK (snprintf, int, (char *, size_t, char const *, ...));

#include <string.h>

#include "macros.h"

int
main (int argc, char *argv[])
{
  char buf[8];
  int size;
  int retval;

  for (size = 0; size <= 8; size++)
    {
      memcpy (buf, "DEADBEEF", 8);
      retval = snprintf (buf, size, "%d", 12345);
      if (size < 6)
        {
#if CHECK_SNPRINTF_POSIX
          ASSERT (retval < 0 || retval >= size);
#endif
          if (size > 0)
            {
              ASSERT (memcmp (buf, "12345", size - 1) == 0);
              ASSERT (buf[size - 1] == '\0' || buf[size - 1] == '0' + size);
            }
#if !CHECK_SNPRINTF_POSIX
          if (size > 0)
#endif
            ASSERT (memcmp (buf + size, "DEADBEEF" + size, 8 - size) == 0);
        }
      else
        {
          ASSERT (retval == 5);
          ASSERT (memcmp (buf, "12345\0EF", 8) == 0);
        }
    }

  return 0;
}
