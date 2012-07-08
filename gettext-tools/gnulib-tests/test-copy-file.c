/* Test of copying of files.
   Copyright (C) 2008-2010 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2008.  */

#include <config.h>

#include "copy-file.h"

#include "progname.h"
#include "macros.h"

int
main (int argc, char *argv[])
{
  const char *file1;
  const char *file2;

  set_program_name (argv[0]);

  ASSERT (argc == 3);

  file1 = argv[1];
  file2 = argv[2];

  copy_file_preserving (file1, file2);

  return 0;
}
