/* Convert ASCII quotations to Unicode quotations.
   Copyright (C) 2014 Free Software Foundation, Inc.
   Written by Daiki Ueno <ueno@gnu.org>, 2014.

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
# include <config.h>
#endif

/* Specification.  */
#include "filters.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xalloc.h"

#define BOLD_START "\e[1m"
#define BOLD_END "\e[0m"

/* This is a direct translation of po/quot.sed and po/boldquot.sed.  */
static void
convert_ascii_quote_to_unicode (const char *input, size_t input_len,
                                char **output_p, size_t *output_len_p,
                                bool bold)
{
  const char *start, *end, *p;
  char *output, *r;
  bool state;
  size_t quote_count;

  start = input;
  end = &input[input_len - 1];

  /* True if we have seen a character which could be an opening
     quotation mark.  Note that we can't determine if it is really an
     opening quotation mark until we see a closing quotation mark.  */
  state = false;

  /* Count the number of quotation characters.  */
  quote_count = 0;
  for (p = start; p <= end; p++)
    {
      size_t len;

      p = strpbrk (p, "`'\"");
      if (!p)
        break;

      len = strspn (p, "`'\"");
      quote_count += len;
      p += len;
    }

  /* Large enough.  */
  r = output = XNMALLOC (input_len - quote_count
                         + (bold ? 7 : 3) * quote_count + 1,
                         char);

#undef COPY_SEEN
#define COPY_SEEN                             \
  do                                            \
    {                                           \
      memcpy (r, start, p - start);             \
      r += p - start;                           \
      start = p;                                \
    }                                           \
  while (0)

  for (p = start; p <= end; p++)
    {
      switch (*p)
        {
        case '"':
          if (state)
            {
              if (*start == '"')
                {
                  if (p > start + 1)
                    {
                      /* U+201C: LEFT DOUBLE QUOTATION MARK */
                      memcpy (r, "\xe2\x80\x9c", 3);
                      r += 3;
                      if (bold)
                        {
                          memcpy (r, BOLD_START, 4);
                          r += 4;
                        }
                      memcpy (r, start + 1, p - start - 1);
                      r += p - start - 1;
                      if (bold)
                        {
                          memcpy (r, BOLD_END, 4);
                          r += 4;
                        }
                      /* U+201D: RIGHT DOUBLE QUOTATION MARK */
                      memcpy (r, "\xe2\x80\x9d", 3);
                      r += 3;
                    }
                  else
                    {
                      /* Consider "" as "".  */
                      memcpy (r, "\"\"", 2);
                      r += 2;
                    }
                  start = p + 1;
                  state = false;
                }
            }
          else
            {
              COPY_SEEN;
              state = true;
            }
          break;

        case '`':
          if (state)
            {
              if (*start == '`')
                COPY_SEEN;
            }
          else
            {
              COPY_SEEN;
              state = true;
            }
          break;

        case '\'':
          if (state)
            {
              if (/* `...' */
                  *start == '`'
                  /* '...', where:
                     - The left quote is preceded by a space, and the
                       right quote is followed by a space.
                     - The left quote is preceded by a space, and the
                       right quote is at the end of line.
                     - The left quote is at the beginning of the line, and
                       the right quote is followed by a space.
                   */
                  || (*start == '\''
                      && (((start > input && *(start - 1) == ' ')
                           && (p == end || *(p + 1) == '\n' || *(p + 1) == ' '))
                          || ((start == input || *(start - 1) == '\n')
                              && p < end && *(p + 1) == ' '))))
                {
                  /* U+2018: LEFT SINGLE QUOTATION MARK */
                  memcpy (r, "\xe2\x80\x98", 3);
                  r += 3;
                  if (bold)
                    {
                      memcpy (r, BOLD_START, 4);
                      r += 4;
                    }
                  memcpy (r, start + 1, p - start - 1);
                  r += p - start - 1;
                  if (bold)
                    {
                      memcpy (r, BOLD_END, 4);
                      r += 4;
                    }
                  /* U+2019: RIGHT SINGLE QUOTATION MARK */
                  memcpy (r, "\xe2\x80\x99", 3);
                  r += 3;
                  start = p + 1;
                }
              else
                COPY_SEEN;
              state = false;
            }
          else if (p == input || *(p - 1) == '\n' || *(p - 1) == ' ')
            {
              COPY_SEEN;
              state = true;
            }
          break;
        }
    }

#undef COPY_SEEN

  /* Copy the rest to R.  */
  if (p > start)
    {
      memcpy (r, start, p - start);
      r += p - start;
    }
  *r = '\0';

  *output_p = output;
  *output_len_p = r - output;
}

void
ascii_quote_to_unicode (const char *input, size_t input_len,
                        char **output_p, size_t *output_len_p)
{
  convert_ascii_quote_to_unicode (input, input_len,
                                  output_p, output_len_p,
                                  false);
}

void
ascii_quote_to_unicode_bold (const char *input, size_t input_len,
                             char **output_p, size_t *output_len_p)
{
  convert_ascii_quote_to_unicode (input, input_len,
                                  output_p, output_len_p,
                                  true);
}
