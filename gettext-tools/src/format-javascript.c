/* JavaScript format strings.
   Copyright (C) 2001-2004, 2006-2009, 2013, 2015 Free Software
   Foundation, Inc.
   Written by Andreas Stricker <andy@knitter.ch>, 2010.
   It's based on python format module from Bruno Haible.

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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "format.h"
#include "c-ctype.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "format-invalid.h"
#include "gettext.h"

#define _(str) gettext (str)

/* Although JavaScript specification itself does not define any format
   strings, many implementations provide printf-like functions.
   We provide a permissive parser which accepts commonly used format
   strings, where:

   A directive
   - starts with '%',
   - is optionally followed by any of the characters '0', '-', ' ',
     or, each of which acts as a flag,
   - is optionally followed by a width specification: a nonempty digit
     sequence,
   - is optionally followed by '.' and a precision specification: a nonempty
     digit sequence,
   - is finished by a specifier
       - 's', that needs a string argument,
       - 'b', 'd', 'u', 'o', 'x', 'X', that need an integer argument,
       - 'f', that need a floating-point argument,
       - 'c', that needs a character argument.
       - 'j', that needs an argument of any type.
   Additionally there is the directive '%%', which takes no argument.  */

enum format_arg_type
{
  FAT_NONE,
  FAT_ANY,
  FAT_CHARACTER,
  FAT_STRING,
  FAT_INTEGER,
  FAT_FLOAT
};

struct spec
{
  unsigned int directives;
  unsigned int format_args_count;
  unsigned int allocated;
  enum format_arg_type *format_args;
};

/* Locale independent test for a decimal digit.
   Argument can be  'char' or 'unsigned char'.  (Whereas the argument of
   <ctype.h> isdigit must be an 'unsigned char'.)  */
#undef isdigit
#define isdigit(c) ((unsigned int) ((c) - '0') < 10)


static void *
format_parse (const char *format, bool translated, char *fdi,
              char **invalid_reason)
{
  const char *const format_start = format;
  struct spec spec;
  struct spec *result;

  spec.directives = 0;
  spec.format_args_count = 0;
  spec.allocated = 0;
  spec.format_args = NULL;

  for (; *format != '\0';)
    if (*format++ == '%')
      {
        /* A directive.  */
        enum format_arg_type type;

        FDI_SET (format - 1, FMTDIR_START);
        spec.directives++;

        while (*format == '-' || *format == '+' || *format == ' '
               || *format == '0' || *format == 'I')
          format++;

        while (isdigit (*format))
          format++;

        if (*format == '.')
          {
            format++;

            while (isdigit (*format))
              format++;
          }

        switch (*format)
          {
          case '%':
            type = FAT_NONE;
            break;
          case 'c':
            type = FAT_CHARACTER;
            break;
          case 's':
            type = FAT_STRING;
            break;
          case 'b': case 'd': case 'o': case 'x': case 'X':
            type = FAT_INTEGER;
            break;
          case 'f':
            type = FAT_FLOAT;
            break;
          case 'j':
            type = FAT_ANY;
            break;
          default:
            if (*format == '\0')
              {
                *invalid_reason = INVALID_UNTERMINATED_DIRECTIVE ();
                FDI_SET (format - 1, FMTDIR_ERROR);
              }
            else
              {
                *invalid_reason =
                  INVALID_CONVERSION_SPECIFIER (spec.directives, *format);
                FDI_SET (format, FMTDIR_ERROR);
              }
            goto bad_format;
          }

        if (*format != '%')
          {
            if (spec.allocated == spec.format_args_count)
              {
                spec.allocated = 2 * spec.allocated + 1;
                spec.format_args = (enum format_arg_type *) xrealloc (spec.format_args, spec.allocated * sizeof (enum format_arg_type));
              }
            spec.format_args[spec.format_args_count] = type;
            spec.format_args_count++;
          }

        FDI_SET (format, FMTDIR_END);

        format++;
      }

  result = XMALLOC (struct spec);
  *result = spec;
  return result;

 bad_format:
  if (spec.format_args != NULL)
    free (spec.format_args);
  return NULL;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec->format_args != NULL)
    free (spec->format_args);
  free (spec);
}

static int
format_get_number_of_directives (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->directives;
}

static bool
format_check (void *msgid_descr, void *msgstr_descr, bool equality,
              formatstring_error_logger_t error_logger,
              const char *pretty_msgid, const char *pretty_msgstr)
{
  struct spec *spec1 = (struct spec *) msgid_descr;
  struct spec *spec2 = (struct spec *) msgstr_descr;
  bool err = false;

  if (spec1->format_args_count + spec2->format_args_count > 0)
    {
      unsigned int i;

      /* Check the argument types are the same.  */
      if (spec1->format_args_count != spec2->format_args_count)
        {
          if (error_logger)
            error_logger (_("number of format specifications in '%s' and '%s' does not match"),
                          pretty_msgid, pretty_msgstr);
          err = true;
        }
      else
        for (i = 0; i < spec2->format_args_count; i++)
          if (!(spec1->format_args[i] == spec2->format_args[i]
                || (!equality
                    && (spec1->format_args[i] == FAT_ANY
                        || spec2->format_args[i] == FAT_ANY))))
            {
              if (error_logger)
                error_logger (_("format specifications in '%s' and '%s' for argument %u are not the same"),
                              pretty_msgid, pretty_msgstr, i + 1);
              err = true;
            }
    }

  return err;
}


struct formatstring_parser formatstring_javascript =
{
  format_parse,
  format_free,
  format_get_number_of_directives,
  NULL,
  format_check
};


#ifdef TEST

/* Test program: Print the argument list specification returned by
   format_parse for strings read from standard input.  */

#include <stdio.h>

static void
format_print (void *descr)
{
  struct spec *spec = (struct spec *) descr;
  unsigned int i;

  if (spec == NULL)
    {
      printf ("INVALID");
      return;
    }

      printf ("(");
      for (i = 0; i < spec->format_args_count; i++)
        {
          if (i > 0)
            printf (" ");
          switch (spec->format_args[i])
            {
            case FAT_ANY:
              printf ("*");
              break;
            case FAT_CHARACTER:
              printf ("c");
              break;
            case FAT_STRING:
              printf ("s");
              break;
            case FAT_INTEGER:
              printf ("i");
              break;
            case FAT_FLOAT:
              printf ("f");
              break;
            default:
              abort ();
            }
        }
      printf (")");
}

int
main ()
{
  for (;;)
    {
      char *line = NULL;
      size_t line_size = 0;
      int line_len;
      char *invalid_reason;
      void *descr;

      line_len = getline (&line, &line_size, stdin);
      if (line_len < 0)
        break;
      if (line_len > 0 && line[line_len - 1] == '\n')
        line[--line_len] = '\0';

      invalid_reason = NULL;
      descr = format_parse (line, false, NULL, &invalid_reason);

      format_print (descr);
      printf ("\n");
      if (descr == NULL)
        printf ("%s\n", invalid_reason);

      free (invalid_reason);
      free (line);
    }

  return 0;
}

/*
 * For Emacs M-x compile
 * Local Variables:
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../intl -DHAVE_CONFIG_H -DTEST format-javascript.c ../gnulib-lib/libgettextlib.la"
 * End:
 */
#endif /* TEST */
