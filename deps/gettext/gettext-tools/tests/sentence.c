/* Test of sentence handling.
   Copyright (C) 2015 Free Software Foundation, Inc.
   Written by Daiki Ueno <ueno@gnu.org>, 2015.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "sentence.h"

#include <assert.h>
#include <string.h>

#define PRIMARY "This is a primary sentence"
#define SECONDARY "This is a secondary sentence"

#define SIZEOF(x) (sizeof (x) / sizeof (*x))

struct data
{
  int required_spaces;
  const char *input;

  const char *expected_prefix;
  ucs4_t expected_ending_char;
};

const struct data data[] =
  {
    { 1, PRIMARY, PRIMARY, 0xfffd },
    { 1, PRIMARY ".", PRIMARY, '.' },
    { 1, PRIMARY ".x", PRIMARY ".x", 0xfffd },
    { 2, PRIMARY ".  " SECONDARY, PRIMARY, '.' },
    { 1, PRIMARY ".  " SECONDARY, PRIMARY, '.' },
    { 1, PRIMARY ".' " SECONDARY, PRIMARY, '.' },
    { 3, PRIMARY ".  " SECONDARY, PRIMARY ".  " SECONDARY, 0xfffd },
    { 2, PRIMARY ".'  " SECONDARY, PRIMARY, '.' },
    { 2, PRIMARY ".'x  " SECONDARY, PRIMARY ".'x  " SECONDARY, 0xfffd },
    { 2, PRIMARY ".''x  " SECONDARY, PRIMARY ".''x  " SECONDARY, 0xfffd },
    { 2, PRIMARY ".\n" SECONDARY, PRIMARY, '.' },
    { 2, PRIMARY ". \n" SECONDARY, PRIMARY, '.' },
    { 2, PRIMARY ".\xc2\xa0\n" SECONDARY, PRIMARY, '.' },
    { 2, PRIMARY ".\t" SECONDARY, PRIMARY, '.' },
    { 2, PRIMARY ".'\t" SECONDARY, PRIMARY, '.' },
    { 2, PRIMARY ".'\n" SECONDARY, PRIMARY, '.' }
  };

static void
check_sentence_end (const struct data *d)
{
  int saved_required_spaces = sentence_end_required_spaces;
  const char *result;
  ucs4_t ending_char;

  sentence_end_required_spaces = d->required_spaces;
  result = sentence_end (d->input, &ending_char);
  sentence_end_required_spaces = saved_required_spaces;

  assert (result == d->input + strlen (d->expected_prefix));
  assert (ending_char == d->expected_ending_char);
}

int
main (int argc, char **argv)
{
  int i;

  for (i = 0; i < SIZEOF (data); i++)
    check_sentence_end (&data[i]);

  return 0;
}
