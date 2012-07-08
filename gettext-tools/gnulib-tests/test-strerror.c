/* Test of strerror() function.
   Copyright (C) 2007-2010 Free Software Foundation, Inc.

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

/* Written by Eric Blake <ebb9@byu.net>, 2007.  */

#include <config.h>

#include <string.h>

#include "signature.h"
SIGNATURE_CHECK (strerror, char *, (int));

#include <errno.h>

#include "macros.h"

int
main (void)
{
  char *str;

  str = strerror (EACCES);
  ASSERT (str);
  ASSERT (*str);

  str = strerror (ETIMEDOUT);
  ASSERT (str);
  ASSERT (*str);

  str = strerror (EOVERFLOW);
  ASSERT (str);
  ASSERT (*str);

  str = strerror (0);
  ASSERT (str);
  ASSERT (*str);

  str = strerror (-3);
  ASSERT (str);
  ASSERT (*str);

  return 0;
}
