/* Test the Unicode character name functions.
   Copyright (C) 2000-2003, 2005, 2007, 2009-2010 Free Software Foundation,
   Inc.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xalloc.h"
#include "uniname.h"
#include "progname.h"

/* The names according to the UnicodeData.txt file, modified to contain the
   Hangul syllable names, as described in the Unicode 3.0 book.  */
const char * unicode_names [0x110000];

/* Maximum length of a field in the UnicodeData.txt file.  */
#define FIELDLEN 120

/* Reads the next field from STREAM.  The buffer BUFFER has size FIELDLEN.
   Reads up to (but excluding) DELIM.
   Returns 1 when a field was successfully read, otherwise 0.  */
static int
getfield (FILE *stream, char *buffer, int delim)
{
  int count = 0;
  int c;

  for (; (c = getc (stream)), (c != EOF && c != delim); )
    {
      /* Put c into the buffer.  */
      if (++count >= FIELDLEN - 1)
        {
          fprintf (stderr, "field too long\n");
          exit (EXIT_FAILURE);
        }
      *buffer++ = c;
    }

  if (c == EOF)
    return 0;

  *buffer = '\0';
  return 1;
}

/* Stores in unicode_names[] the relevant contents of the UnicodeData.txt
   file.  */
static void
fill_names (const char *unicodedata_filename)
{
  unsigned int i;
  FILE *stream;
  char field0[FIELDLEN];
  char field1[FIELDLEN];
  int lineno = 0;

  for (i = 0; i < 0x110000; i++)
    unicode_names[i] = NULL;

  stream = fopen (unicodedata_filename, "r");
  if (stream == NULL)
    {
      fprintf (stderr, "error during fopen of '%s'\n", unicodedata_filename);
      exit (EXIT_FAILURE);
    }

  for (;;)
    {
      int n;
      int c;

      lineno++;
      n = getfield (stream, field0, ';');
      n += getfield (stream, field1, ';');
      if (n == 0)
        break;
      if (n != 2)
        {
          fprintf (stderr, "short line in '%s':%d\n",
                   unicodedata_filename, lineno);
          exit (EXIT_FAILURE);
        }
      for (; (c = getc (stream)), (c != EOF && c != '\n'); )
        ;
      i = strtoul (field0, NULL, 16);
      if (i >= 0x110000)
        {
          fprintf (stderr, "index too large\n");
          exit (EXIT_FAILURE);
        }
      unicode_names[i] = xstrdup (field1);
    }
  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error reading from '%s'\n", unicodedata_filename);
      exit (1);
    }
}

/* Perform an exhaustive test of the unicode_character_name function.  */
static int
test_name_lookup ()
{
  int error = 0;
  unsigned int i;
  char buf[UNINAME_MAX];

  for (i = 0; i < 0x11000; i++)
    {
      char *result = unicode_character_name (i, buf);

      if (unicode_names[i] != NULL)
        {
          if (result == NULL)
            {
              fprintf (stderr, "\\u%04X name lookup failed!\n", i);
              error = 1;
            }
          else if (strcmp (result, unicode_names[i]) != 0)
            {
              fprintf (stderr, "\\u%04X name lookup returned wrong name: %s\n",
                               i, result);
              error = 1;
            }
        }
      else
        {
          if (result != NULL)
            {
              fprintf (stderr, "\\u%04X name lookup returned wrong name: %s\n",
                               i, result);
              error = 1;
            }
        }
    }

  for (i = 0x110000; i < 0x1000000; i++)
    {
      char *result = unicode_character_name (i, buf);

      if (result != NULL)
        {
          fprintf (stderr, "\\u%04X name lookup returned wrong name: %s\n",
                           i, result);
          error = 1;
        }
    }

  return error;
}

/* Perform a test of the unicode_name_character function.  */
static int
test_inverse_lookup ()
{
  int error = 0;
  unsigned int i;

  /* First, verify all valid character names are recognized.  */
  for (i = 0; i < 0x110000; i++)
    if (unicode_names[i] != NULL)
      {
        unsigned int result = unicode_name_character (unicode_names[i]);
        if (result != i)
          {
            if (result == UNINAME_INVALID)
              fprintf (stderr, "inverse name lookup of \"%s\" failed\n",
                       unicode_names[i]);
            else
              fprintf (stderr,
                       "inverse name lookup of \"%s\" returned 0x%04X\n",
                       unicode_names[i], result);
            error = 1;
          }
      }

  /* Second, generate random but likely names and verify they are not
     recognized unless really valid.  */
  for (i = 0; i < 10000; i++)
    {
      unsigned int i1, i2;
      const char *s1;
      const char *s2;
      unsigned int l1, l2, j1, j2;
      char buf[2*UNINAME_MAX];
      unsigned int result;

      do i1 = ((rand () % 0x11) << 16)
              + ((rand () & 0xff) << 8)
              + (rand () & 0xff);
      while (unicode_names[i1] == NULL);

      do i2 = ((rand () % 0x11) << 16)
              + ((rand () & 0xff) << 8)
              + (rand () & 0xff);
      while (unicode_names[i2] == NULL);

      s1 = unicode_names[i1];
      l1 = strlen (s1);
      s2 = unicode_names[i2];
      l2 = strlen (s2);

      /* Concatenate a starting piece of s1 with an ending piece of s2.  */
      for (j1 = 1; j1 <= l1; j1++)
        if (j1 == l1 || s1[j1] == ' ')
          for (j2 = 0; j2 < l2; j2++)
            if (j2 == 0 || s2[j2-1] == ' ')
              {
                memcpy (buf, s1, j1);
                buf[j1] = ' ';
                memcpy (buf + j1 + 1, s2 + j2, l2 - j2 + 1);

                result = unicode_name_character (buf);
                if (result != UNINAME_INVALID
                    && !(unicode_names[result] != NULL
                         && strcmp (unicode_names[result], buf) == 0))
                  {
                    fprintf (stderr,
                             "inverse name lookup of \"%s\" returned 0x%04X\n",
                             unicode_names[i], result);
                    error = 1;
                  }
              }
    }

  /* Third, some extreme case that used to loop.  */
  if (unicode_name_character ("A A") != UNINAME_INVALID)
    error = 1;

  return error;
}

int
main (int argc, char *argv[])
{
  int error = 0;

  set_program_name (argv[0]);

  fill_names (argv[1]);

  error |= test_name_lookup ();
  error |= test_inverse_lookup ();

  return error;
}
