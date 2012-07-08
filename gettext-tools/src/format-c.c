/* C format strings.
   Copyright (C) 2001-2004, 2006-2007, 2009 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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

#include "format.h"
#include "c-ctype.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "format-invalid.h"
#include "gettext.h"

#define _(str) gettext (str)

/* C format strings are described in POSIX (IEEE P1003.1 2001), section
   XSH 3 fprintf().  See also Linux fprintf(3) manual page.
   A directive
   - starts with '%' or '%m$' where m is a positive integer,
   - is optionally followed by any of the characters '#', '0', '-', ' ', '+',
     "'", or - only in msgstr strings - the string "I", each of which acts as
     a flag,
   - is optionally followed by a width specification: '*' (reads an argument)
     or '*m$' or a nonempty digit sequence,
   - is optionally followed by '.' and a precision specification: '*' (reads
     an argument) or '*m$' or a nonempty digit sequence,
   - is either continued like this:
       - is optionally followed by a size specifier, one of 'hh' 'h' 'l' 'll'
         'L' 'q' 'j' 'z' 't',
       - is finished by a specifier
           - '%', that needs no argument,
           - 'c', 'C', that need a character argument,
           - 's', 'S', that need a string argument,
           - 'i', 'd', that need a signed integer argument,
           - 'o', 'u', 'x', 'X', that need an unsigned integer argument,
           - 'e', 'E', 'f', 'F', 'g', 'G', 'a', 'A', that need a floating-point
             argument,
           - 'p', that needs a 'void *' argument,
           - 'n', that needs a pointer to integer.
     or is finished by a specifier '<' inttypes-macro '>' where inttypes-macro
     is an ISO C 99 section 7.8.1 format directive.
   Numbered ('%m$' or '*m$') and unnumbered argument specifications cannot
   be used in the same string.  When numbered argument specifications are
   used, specifying the Nth argument requires that all the leading arguments,
   from the first to the (N-1)th, are specified in the format string.
 */

enum format_arg_type
{
  FAT_NONE              = 0,
  /* Basic types */
  FAT_INTEGER           = 1,
  FAT_DOUBLE            = 2,
  FAT_CHAR              = 3,
  FAT_STRING            = 4,
  FAT_OBJC_OBJECT       = 5,
  FAT_POINTER           = 6,
  FAT_COUNT_POINTER     = 7,
  /* Flags */
  FAT_UNSIGNED          = 1 << 3,
  FAT_SIZE_SHORT        = 1 << 4,
  FAT_SIZE_CHAR         = 2 << 4,
  FAT_SIZE_LONG         = 1 << 6,
  FAT_SIZE_LONGLONG     = 2 << 6,
  FAT_SIZE_8_T          = 1 << 8,
  FAT_SIZE_16_T         = 1 << 9,
  FAT_SIZE_32_T         = 1 << 10,
  FAT_SIZE_64_T         = 1 << 11,
  FAT_SIZE_LEAST8_T     = 1 << 12,
  FAT_SIZE_LEAST16_T    = 1 << 13,
  FAT_SIZE_LEAST32_T    = 1 << 14,
  FAT_SIZE_LEAST64_T    = 1 << 15,
  FAT_SIZE_FAST8_T      = 1 << 16,
  FAT_SIZE_FAST16_T     = 1 << 17,
  FAT_SIZE_FAST32_T     = 1 << 18,
  FAT_SIZE_FAST64_T     = 1 << 19,
  FAT_SIZE_INTMAX_T     = 1 << 20,
  FAT_SIZE_INTPTR_T     = 1 << 21,
  FAT_SIZE_SIZE_T       = 1 << 22,
  FAT_SIZE_PTRDIFF_T    = 1 << 23,
  FAT_WIDE              = FAT_SIZE_LONG,
  /* Meaningful combinations of basic types and flags:
  'signed char'                 = FAT_INTEGER | FAT_SIZE_CHAR,
  'unsigned char'               = FAT_INTEGER | FAT_SIZE_CHAR | FAT_UNSIGNED,
  'short'                       = FAT_INTEGER | FAT_SIZE_SHORT,
  'unsigned short'              = FAT_INTEGER | FAT_SIZE_SHORT | FAT_UNSIGNED,
  'int'                         = FAT_INTEGER,
  'unsigned int'                = FAT_INTEGER | FAT_UNSIGNED,
  'long int'                    = FAT_INTEGER | FAT_SIZE_LONG,
  'unsigned long int'           = FAT_INTEGER | FAT_SIZE_LONG | FAT_UNSIGNED,
  'long long int'               = FAT_INTEGER | FAT_SIZE_LONGLONG,
  'unsigned long long int'      = FAT_INTEGER | FAT_SIZE_LONGLONG | FAT_UNSIGNED,
  'double'                      = FAT_DOUBLE,
  'long double'                 = FAT_DOUBLE | FAT_SIZE_LONGLONG,
  'char'/'int'                  = FAT_CHAR,
  'wchar_t'/'wint_t'            = FAT_CHAR | FAT_SIZE_LONG,
  'const char *'                = FAT_STRING,
  'const wchar_t *'             = FAT_STRING | FAT_SIZE_LONG,
  'void *'                      = FAT_POINTER,
  FAT_COUNT_SCHAR_POINTER       = FAT_COUNT_POINTER | FAT_SIZE_CHAR,
  FAT_COUNT_SHORT_POINTER       = FAT_COUNT_POINTER | FAT_SIZE_SHORT,
  FAT_COUNT_INT_POINTER         = FAT_COUNT_POINTER,
  FAT_COUNT_LONGINT_POINTER     = FAT_COUNT_POINTER | FAT_SIZE_LONG,
  FAT_COUNT_LONGLONGINT_POINTER = FAT_COUNT_POINTER | FAT_SIZE_LONGLONG,
  */
  /* Bitmasks */
  FAT_SIZE_MASK         = (FAT_SIZE_SHORT | FAT_SIZE_CHAR
                           | FAT_SIZE_LONG | FAT_SIZE_LONGLONG
                           | FAT_SIZE_8_T | FAT_SIZE_16_T
                           | FAT_SIZE_32_T | FAT_SIZE_64_T
                           | FAT_SIZE_LEAST8_T | FAT_SIZE_LEAST16_T
                           | FAT_SIZE_LEAST32_T | FAT_SIZE_LEAST64_T
                           | FAT_SIZE_FAST8_T | FAT_SIZE_FAST16_T
                           | FAT_SIZE_FAST32_T | FAT_SIZE_FAST64_T
                           | FAT_SIZE_INTMAX_T | FAT_SIZE_INTPTR_T
                           | FAT_SIZE_SIZE_T | FAT_SIZE_PTRDIFF_T)
};
#ifdef __cplusplus
typedef int format_arg_type_t;
#else
typedef enum format_arg_type format_arg_type_t;
#endif

struct numbered_arg
{
  unsigned int number;
  format_arg_type_t type;
};

struct unnumbered_arg
{
  format_arg_type_t type;
};

struct spec
{
  unsigned int directives;
  unsigned int unnumbered_arg_count;
  unsigned int allocated;
  struct unnumbered_arg *unnumbered;
  bool unlikely_intentional;
  unsigned int sysdep_directives_count;
  const char **sysdep_directives;
};

/* Locale independent test for a decimal digit.
   Argument can be  'char' or 'unsigned char'.  (Whereas the argument of
   <ctype.h> isdigit must be an 'unsigned char'.)  */
#undef isdigit
#define isdigit(c) ((unsigned int) ((c) - '0') < 10)


static int
numbered_arg_compare (const void *p1, const void *p2)
{
  unsigned int n1 = ((const struct numbered_arg *) p1)->number;
  unsigned int n2 = ((const struct numbered_arg *) p2)->number;

  return (n1 > n2 ? 1 : n1 < n2 ? -1 : 0);
}

#define INVALID_C99_MACRO(directive_number) \
  xasprintf (_("In the directive number %u, the token after '<' is not the name of a format specifier macro. The valid macro names are listed in ISO C 99 section 7.8.1."), directive_number)

static void *
format_parse (const char *format, bool translated, bool objc_extensions,
              char *fdi, char **invalid_reason)
{
  const char *const format_start = format;
  struct spec spec;
  unsigned int numbered_arg_count;
  struct numbered_arg *numbered;
  struct spec *result;

  spec.directives = 0;
  numbered_arg_count = 0;
  spec.unnumbered_arg_count = 0;
  spec.allocated = 0;
  numbered = NULL;
  spec.unnumbered = NULL;
  spec.unlikely_intentional = false;
  spec.sysdep_directives_count = 0;
  spec.sysdep_directives = NULL;

  for (; *format != '\0';)
    if (*format++ == '%')
      {
        /* A directive.  */
        unsigned int number = 0;
        format_arg_type_t type;
        format_arg_type_t size;

        FDI_SET (format - 1, FMTDIR_START);
        spec.directives++;

        if (isdigit (*format))
          {
            const char *f = format;
            unsigned int m = 0;

            do
              {
                m = 10 * m + (*f - '0');
                f++;
              }
            while (isdigit (*f));

            if (*f == '$')
              {
                if (m == 0)
                  {
                    *invalid_reason = INVALID_ARGNO_0 (spec.directives);
                    FDI_SET (f, FMTDIR_ERROR);
                    goto bad_format;
                  }
                number = m;
                format = ++f;
              }
          }

        /* Parse flags.  */
        for (;;)
          {
            if (*format == ' ' || *format == '+' || *format == '-'
                || *format == '#' || *format == '0' || *format == '\'')
              format++;
            else if (translated && *format == 'I')
              {
                spec.sysdep_directives =
                  (const char **)
                  xrealloc (spec.sysdep_directives,
                            2 * (spec.sysdep_directives_count + 1)
                            * sizeof (const char *));
                spec.sysdep_directives[2 * spec.sysdep_directives_count] = format;
                spec.sysdep_directives[2 * spec.sysdep_directives_count + 1] = format + 1;
                spec.sysdep_directives_count++;
                format++;
              }
            else
              break;
          }

        /* Parse width.  */
        if (*format == '*')
          {
            unsigned int width_number = 0;

            format++;

            if (isdigit (*format))
              {
                const char *f = format;
                unsigned int m = 0;

                do
                  {
                    m = 10 * m + (*f - '0');
                    f++;
                  }
                while (isdigit (*f));

                if (*f == '$')
                  {
                    if (m == 0)
                      {
                        *invalid_reason =
                          INVALID_WIDTH_ARGNO_0 (spec.directives);
                        FDI_SET (f, FMTDIR_ERROR);
                        goto bad_format;
                      }
                    width_number = m;
                    format = ++f;
                  }
              }

            if (width_number)
              {
                /* Numbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (spec.unnumbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format - 1, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (spec.allocated == numbered_arg_count)
                  {
                    spec.allocated = 2 * spec.allocated + 1;
                    numbered = (struct numbered_arg *) xrealloc (numbered, spec.allocated * sizeof (struct numbered_arg));
                  }
                numbered[numbered_arg_count].number = width_number;
                numbered[numbered_arg_count].type = FAT_INTEGER;
                numbered_arg_count++;
              }
            else
              {
                /* Unnumbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (numbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format - 1, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (spec.allocated == spec.unnumbered_arg_count)
                  {
                    spec.allocated = 2 * spec.allocated + 1;
                    spec.unnumbered = (struct unnumbered_arg *) xrealloc (spec.unnumbered, spec.allocated * sizeof (struct unnumbered_arg));
                  }
                spec.unnumbered[spec.unnumbered_arg_count].type = FAT_INTEGER;
                spec.unnumbered_arg_count++;
              }
          }
        else if (isdigit (*format))
          {
            do format++; while (isdigit (*format));
          }

        /* Parse precision.  */
        if (*format == '.')
          {
            format++;

            if (*format == '*')
              {
                unsigned int precision_number = 0;

                format++;

                if (isdigit (*format))
                  {
                    const char *f = format;
                    unsigned int m = 0;

                    do
                      {
                        m = 10 * m + (*f - '0');
                        f++;
                      }
                    while (isdigit (*f));

                    if (*f == '$')
                      {
                        if (m == 0)
                          {
                            *invalid_reason =
                              INVALID_PRECISION_ARGNO_0 (spec.directives);
                            FDI_SET (f, FMTDIR_ERROR);
                            goto bad_format;
                          }
                        precision_number = m;
                        format = ++f;
                      }
                  }

                if (precision_number)
                  {
                    /* Numbered argument.  */

                    /* Numbered and unnumbered specifications are exclusive.  */
                    if (spec.unnumbered_arg_count > 0)
                      {
                        *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                        FDI_SET (format - 1, FMTDIR_ERROR);
                        goto bad_format;
                      }

                    if (spec.allocated == numbered_arg_count)
                      {
                        spec.allocated = 2 * spec.allocated + 1;
                        numbered = (struct numbered_arg *) xrealloc (numbered, spec.allocated * sizeof (struct numbered_arg));
                      }
                    numbered[numbered_arg_count].number = precision_number;
                    numbered[numbered_arg_count].type = FAT_INTEGER;
                    numbered_arg_count++;
                  }
                else
                  {
                    /* Unnumbered argument.  */

                    /* Numbered and unnumbered specifications are exclusive.  */
                    if (numbered_arg_count > 0)
                      {
                        *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                        FDI_SET (format - 1, FMTDIR_ERROR);
                        goto bad_format;
                      }

                    if (spec.allocated == spec.unnumbered_arg_count)
                      {
                        spec.allocated = 2 * spec.allocated + 1;
                        spec.unnumbered = (struct unnumbered_arg *) xrealloc (spec.unnumbered, spec.allocated * sizeof (struct unnumbered_arg));
                      }
                    spec.unnumbered[spec.unnumbered_arg_count].type = FAT_INTEGER;
                    spec.unnumbered_arg_count++;
                  }
              }
            else if (isdigit (*format))
              {
                do format++; while (isdigit (*format));
              }
          }

        if (*format == '<')
          {
            spec.sysdep_directives =
              (const char **)
              xrealloc (spec.sysdep_directives,
                        2 * (spec.sysdep_directives_count + 1)
                        * sizeof (const char *));
            spec.sysdep_directives[2 * spec.sysdep_directives_count] = format;

            format++;
            /* Parse ISO C 99 section 7.8.1 format string directive.
               Syntax:
               P R I { d | i | o | u | x | X }
               { { | LEAST | FAST } { 8 | 16 | 32 | 64 } | MAX | PTR }  */
            if (*format != 'P')
              {
                *invalid_reason = INVALID_C99_MACRO (spec.directives);
                FDI_SET (*format == '\0' ? format - 1 : format, FMTDIR_ERROR);
                goto bad_format;
              }
            format++;
            if (*format != 'R')
              {
                *invalid_reason = INVALID_C99_MACRO (spec.directives);
                FDI_SET (*format == '\0' ? format - 1 : format, FMTDIR_ERROR);
                goto bad_format;
              }
            format++;
            if (*format != 'I')
              {
                *invalid_reason = INVALID_C99_MACRO (spec.directives);
                FDI_SET (*format == '\0' ? format - 1 : format, FMTDIR_ERROR);
                goto bad_format;
              }
            format++;

            switch (*format)
              {
              case 'i': case 'd':
                type = FAT_INTEGER;
                break;
              case 'u': case 'o': case 'x': case 'X':
                type = FAT_INTEGER | FAT_UNSIGNED;
                break;
              default:
                *invalid_reason = INVALID_C99_MACRO (spec.directives);
                FDI_SET (*format == '\0' ? format - 1 : format, FMTDIR_ERROR);
                goto bad_format;
              }
            format++;

            if (format[0] == 'M' && format[1] == 'A' && format[2] == 'X')
              {
                type |= FAT_SIZE_INTMAX_T;
                format += 3;
              }
            else if (format[0] == 'P' && format[1] == 'T' && format[2] == 'R')
              {
                type |= FAT_SIZE_INTPTR_T;
                format += 3;
              }
            else
              {
                if (format[0] == 'L' && format[1] == 'E' && format[2] == 'A'
                    && format[3] == 'S' && format[4] == 'T')
                  {
                    format += 5;
                    if (format[0] == '8')
                      {
                        type |= FAT_SIZE_LEAST8_T;
                        format++;
                      }
                    else if (format[0] == '1' && format[1] == '6')
                      {
                        type |= FAT_SIZE_LEAST16_T;
                        format += 2;
                      }
                    else if (format[0] == '3' && format[1] == '2')
                      {
                        type |= FAT_SIZE_LEAST32_T;
                        format += 2;
                      }
                    else if (format[0] == '6' && format[1] == '4')
                      {
                        type |= FAT_SIZE_LEAST64_T;
                        format += 2;
                      }
                    else
                      {
                        *invalid_reason = INVALID_C99_MACRO (spec.directives);
                        FDI_SET (*format == '\0' ? format - 1 : format,
                                 FMTDIR_ERROR);
                        goto bad_format;
                      }
                  }
                else if (format[0] == 'F' && format[1] == 'A'
                         && format[2] == 'S' && format[3] == 'T')
                  {
                    format += 4;
                    if (format[0] == '8')
                      {
                        type |= FAT_SIZE_FAST8_T;
                        format++;
                      }
                    else if (format[0] == '1' && format[1] == '6')
                      {
                        type |= FAT_SIZE_FAST16_T;
                        format += 2;
                      }
                    else if (format[0] == '3' && format[1] == '2')
                      {
                        type |= FAT_SIZE_FAST32_T;
                        format += 2;
                      }
                    else if (format[0] == '6' && format[1] == '4')
                      {
                        type |= FAT_SIZE_FAST64_T;
                        format += 2;
                      }
                    else
                      {
                        *invalid_reason = INVALID_C99_MACRO (spec.directives);
                        FDI_SET (*format == '\0' ? format - 1 : format,
                                 FMTDIR_ERROR);
                        goto bad_format;
                      }
                  }
                else
                  {
                    if (format[0] == '8')
                      {
                        type |= FAT_SIZE_8_T;
                        format++;
                      }
                    else if (format[0] == '1' && format[1] == '6')
                      {
                        type |= FAT_SIZE_16_T;
                        format += 2;
                      }
                    else if (format[0] == '3' && format[1] == '2')
                      {
                        type |= FAT_SIZE_32_T;
                        format += 2;
                      }
                    else if (format[0] == '6' && format[1] == '4')
                      {
                        type |= FAT_SIZE_64_T;
                        format += 2;
                      }
                    else
                      {
                        *invalid_reason = INVALID_C99_MACRO (spec.directives);
                        FDI_SET (*format == '\0' ? format - 1 : format,
                                 FMTDIR_ERROR);
                        goto bad_format;
                      }
                  }
              }

            if (*format != '>')
              {
                *invalid_reason =
                  xasprintf (_("In the directive number %u, the token after '<' is not followed by '>'."), spec.directives);
                FDI_SET (*format == '\0' ? format - 1 : format, FMTDIR_ERROR);
                goto bad_format;
              }

            spec.sysdep_directives[2 * spec.sysdep_directives_count + 1] = format + 1;
            spec.sysdep_directives_count++;
          }
        else
          {
            /* Parse size.  */
            size = 0;
            for (;; format++)
              {
                if (*format == 'h')
                  {
                    if (size & (FAT_SIZE_SHORT | FAT_SIZE_CHAR))
                      size = FAT_SIZE_CHAR;
                    else
                      size = FAT_SIZE_SHORT;
                  }
                else if (*format == 'l')
                  {
                    if (size & (FAT_SIZE_LONG | FAT_SIZE_LONGLONG))
                      size = FAT_SIZE_LONGLONG;
                    else
                      size = FAT_SIZE_LONG;
                  }
                else if (*format == 'L')
                  size = FAT_SIZE_LONGLONG;
                else if (*format == 'q')
                  /* Old BSD 4.4 convention.  */
                  size = FAT_SIZE_LONGLONG;
                else if (*format == 'j')
                  size = FAT_SIZE_INTMAX_T;
                else if (*format == 'z' || *format == 'Z')
                  /* 'z' is standardized in ISO C 99, but glibc uses 'Z'
                     because the warning facility in gcc-2.95.2 understands
                     only 'Z' (see gcc-2.95.2/gcc/c-common.c:1784).  */
                  size = FAT_SIZE_SIZE_T;
                else if (*format == 't')
                  size = FAT_SIZE_PTRDIFF_T;
                else
                  break;
              }

            switch (*format)
              {
              case '%':
                /* Programmers writing _("%2%") most often will not want to
                   use this string as a c-format string, but rather as a
                   literal or as a different kind of format string.  */
                if (format[-1] != '%')
                  spec.unlikely_intentional = true;
                type = FAT_NONE;
                break;
              case 'm': /* glibc extension */
                type = FAT_NONE;
                break;
              case 'c':
                type = FAT_CHAR;
                type |= (size & (FAT_SIZE_LONG | FAT_SIZE_LONGLONG)
                         ? FAT_WIDE : 0);
                break;
              case 'C': /* obsolete */
                type = FAT_CHAR | FAT_WIDE;
                break;
              case 's':
                type = FAT_STRING;
                type |= (size & (FAT_SIZE_LONG | FAT_SIZE_LONGLONG)
                         ? FAT_WIDE : 0);
                break;
              case 'S': /* obsolete */
                type = FAT_STRING | FAT_WIDE;
                break;
              case 'i': case 'd':
                type = FAT_INTEGER;
                type |= (size & FAT_SIZE_MASK);
                break;
              case 'u': case 'o': case 'x': case 'X':
                type = FAT_INTEGER | FAT_UNSIGNED;
                type |= (size & FAT_SIZE_MASK);
                break;
              case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
              case 'a': case 'A':
                type = FAT_DOUBLE;
                type |= (size & FAT_SIZE_LONGLONG);
                break;
              case '@':
                if (objc_extensions)
                  {
                    type = FAT_OBJC_OBJECT;
                    break;
                  }
                goto other;
              case 'p':
                type = FAT_POINTER;
                break;
              case 'n':
                type = FAT_COUNT_POINTER;
                type |= (size & FAT_SIZE_MASK);
                break;
              other:
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
          }

        if (type != FAT_NONE)
          {
            if (number)
              {
                /* Numbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (spec.unnumbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (spec.allocated == numbered_arg_count)
                  {
                    spec.allocated = 2 * spec.allocated + 1;
                    numbered = (struct numbered_arg *) xrealloc (numbered, spec.allocated * sizeof (struct numbered_arg));
                  }
                numbered[numbered_arg_count].number = number;
                numbered[numbered_arg_count].type = type;
                numbered_arg_count++;
              }
            else
              {
                /* Unnumbered argument.  */

                /* Numbered and unnumbered specifications are exclusive.  */
                if (numbered_arg_count > 0)
                  {
                    *invalid_reason = INVALID_MIXES_NUMBERED_UNNUMBERED ();
                    FDI_SET (format, FMTDIR_ERROR);
                    goto bad_format;
                  }

                if (spec.allocated == spec.unnumbered_arg_count)
                  {
                    spec.allocated = 2 * spec.allocated + 1;
                    spec.unnumbered = (struct unnumbered_arg *) xrealloc (spec.unnumbered, spec.allocated * sizeof (struct unnumbered_arg));
                  }
                spec.unnumbered[spec.unnumbered_arg_count].type = type;
                spec.unnumbered_arg_count++;
              }
          }

        FDI_SET (format, FMTDIR_END);

        format++;
      }

  /* Sort the numbered argument array, and eliminate duplicates.  */
  if (numbered_arg_count > 1)
    {
      unsigned int i, j;
      bool err;

      qsort (numbered, numbered_arg_count,
             sizeof (struct numbered_arg), numbered_arg_compare);

      /* Remove duplicates: Copy from i to j, keeping 0 <= j <= i.  */
      err = false;
      for (i = j = 0; i < numbered_arg_count; i++)
        if (j > 0 && numbered[i].number == numbered[j-1].number)
          {
            format_arg_type_t type1 = numbered[i].type;
            format_arg_type_t type2 = numbered[j-1].type;
            format_arg_type_t type_both;

            if (type1 == type2)
              type_both = type1;
            else
              {
                /* Incompatible types.  */
                type_both = FAT_NONE;
                if (!err)
                  *invalid_reason =
                    INVALID_INCOMPATIBLE_ARG_TYPES (numbered[i].number);
                err = true;
              }

            numbered[j-1].type = type_both;
          }
        else
          {
            if (j < i)
              {
                numbered[j].number = numbered[i].number;
                numbered[j].type = numbered[i].type;
              }
            j++;
          }
      numbered_arg_count = j;
      if (err)
        /* *invalid_reason has already been set above.  */
        goto bad_format;
    }

  /* Verify that the format strings uses all arguments up to the highest
     numbered one.  */
  if (numbered_arg_count > 0)
    {
      unsigned int i;

      for (i = 0; i < numbered_arg_count; i++)
        if (numbered[i].number != i + 1)
          {
            *invalid_reason =
              xasprintf (_("The string refers to argument number %u but ignores argument number %u."), numbered[i].number, i + 1);
            goto bad_format;
          }

      /* So now the numbered arguments array is equivalent to a sequence
         of unnumbered arguments.  */
      spec.unnumbered_arg_count = numbered_arg_count;
      spec.allocated = spec.unnumbered_arg_count;
      spec.unnumbered = XNMALLOC (spec.allocated, struct unnumbered_arg);
      for (i = 0; i < spec.unnumbered_arg_count; i++)
        spec.unnumbered[i].type = numbered[i].type;
      free (numbered);
      numbered_arg_count = 0;
    }

  result = XMALLOC (struct spec);
  *result = spec;
  return result;

 bad_format:
  if (numbered != NULL)
    free (numbered);
  if (spec.unnumbered != NULL)
    free (spec.unnumbered);
  if (spec.sysdep_directives != NULL)
    free (spec.sysdep_directives);
  return NULL;
}

static void *
format_c_parse (const char *format, bool translated, char *fdi,
                char **invalid_reason)
{
  return format_parse (format, translated, false, fdi, invalid_reason);
}

static void *
format_objc_parse (const char *format, bool translated, char *fdi,
                   char **invalid_reason)
{
  return format_parse (format, translated, true, fdi, invalid_reason);
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec->unnumbered != NULL)
    free (spec->unnumbered);
  if (spec->sysdep_directives != NULL)
    free (spec->sysdep_directives);
  free (spec);
}

static bool
format_is_unlikely_intentional (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->unlikely_intentional;
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
  unsigned int i;

  /* Check the argument types are the same.  */
  if (equality
      ? spec1->unnumbered_arg_count != spec2->unnumbered_arg_count
      : spec1->unnumbered_arg_count < spec2->unnumbered_arg_count)
    {
      if (error_logger)
        error_logger (_("number of format specifications in '%s' and '%s' does not match"),
                      pretty_msgid, pretty_msgstr);
      err = true;
    }
  else
    for (i = 0; i < spec2->unnumbered_arg_count; i++)
      if (spec1->unnumbered[i].type != spec2->unnumbered[i].type)
        {
          if (error_logger)
            error_logger (_("format specifications in '%s' and '%s' for argument %u are not the same"),
                          pretty_msgid, pretty_msgstr, i + 1);
          err = true;
        }

  return err;
}


struct formatstring_parser formatstring_c =
{
  format_c_parse,
  format_free,
  format_get_number_of_directives,
  format_is_unlikely_intentional,
  format_check
};


struct formatstring_parser formatstring_objc =
{
  format_objc_parse,
  format_free,
  format_get_number_of_directives,
  format_is_unlikely_intentional,
  format_check
};


void
get_sysdep_c_format_directives (const char *string, bool translated,
                                struct interval **intervalsp, size_t *lengthp)
{
  /* Parse the format string with all possible extensions turned on.  (The
     caller has already verified that the format string is valid for the
     particular language.)  */
  char *invalid_reason = NULL;
  struct spec *descr =
    (struct spec *)
    format_parse (string, translated, true, NULL, &invalid_reason);

  if (descr != NULL && descr->sysdep_directives_count > 0)
    {
      unsigned int n = descr->sysdep_directives_count;
      struct interval *intervals = XNMALLOC (n, struct interval);
      unsigned int i;

      for (i = 0; i < n; i++)
        {
          intervals[i].startpos = descr->sysdep_directives[2 * i] - string;
          intervals[i].endpos = descr->sysdep_directives[2 * i + 1] - string;
        }
      *intervalsp = intervals;
      *lengthp = n;
    }
  else
    {
      *intervalsp = NULL;
      *lengthp = 0;
    }

  if (descr != NULL)
    format_free (descr);
  else
    free (invalid_reason);
}


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
  for (i = 0; i < spec->unnumbered_arg_count; i++)
    {
      if (i > 0)
        printf (" ");
      if (spec->unnumbered[i].type & FAT_UNSIGNED)
        printf ("[unsigned]");
      switch (spec->unnumbered[i].type & FAT_SIZE_MASK)
        {
        case 0:
          break;
        case FAT_SIZE_SHORT:
          printf ("[short]");
          break;
        case FAT_SIZE_CHAR:
          printf ("[char]");
          break;
        case FAT_SIZE_LONG:
          printf ("[long]");
          break;
        case FAT_SIZE_LONGLONG:
          printf ("[long long]");
          break;
        case FAT_SIZE_8_T:
          printf ("[int8_t]");
          break;
        case FAT_SIZE_16_T:
          printf ("[int16_t]");
          break;
        case FAT_SIZE_32_T:
          printf ("[int32_t]");
          break;
        case FAT_SIZE_64_T:
          printf ("[int64_t]");
          break;
        case FAT_SIZE_LEAST8_T:
          printf ("[int_least8_t]");
          break;
        case FAT_SIZE_LEAST16_T:
          printf ("[int_least16_t]");
          break;
        case FAT_SIZE_LEAST32_T:
          printf ("[int_least32_t]");
          break;
        case FAT_SIZE_LEAST64_T:
          printf ("[int_least64_t]");
          break;
        case FAT_SIZE_FAST8_T:
          printf ("[int_fast8_t]");
          break;
        case FAT_SIZE_FAST16_T:
          printf ("[int_fast16_t]");
          break;
        case FAT_SIZE_FAST32_T:
          printf ("[int_fast32_t]");
          break;
        case FAT_SIZE_FAST64_T:
          printf ("[int_fast64_t]");
          break;
        case FAT_SIZE_INTMAX_T:
          printf ("[intmax_t]");
          break;
        case FAT_SIZE_INTPTR_T:
          printf ("[intptr_t]");
          break;
        case FAT_SIZE_SIZE_T:
          printf ("[size_t]");
          break;
        case FAT_SIZE_PTRDIFF_T:
          printf ("[ptrdiff_t]");
          break;
        default:
          abort ();
        }
      switch (spec->unnumbered[i].type & ~(FAT_UNSIGNED | FAT_SIZE_MASK))
        {
        case FAT_INTEGER:
          printf ("i");
          break;
        case FAT_DOUBLE:
          printf ("f");
          break;
        case FAT_CHAR:
          printf ("c");
          break;
        case FAT_STRING:
          printf ("s");
          break;
        case FAT_OBJC_OBJECT:
          printf ("@");
          break;
        case FAT_POINTER:
          printf ("p");
          break;
        case FAT_COUNT_POINTER:
          printf ("n");
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
      descr = format_c_parse (line, false, NULL, &invalid_reason);

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
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../intl -DHAVE_CONFIG_H -DTEST format-c.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
