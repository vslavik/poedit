/* Reading Desktop Entry files.
   Copyright (C) 1995-1998, 2000-2003, 2005-2006, 2008-2009, 2014 Free Software Foundation, Inc.
   This file was written by Daiki Ueno <ueno@gnu.org>.

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
#include "read-desktop.h"

#include "xalloc.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "error-progname.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "c-ctype.h"
#include "po-lex.h"
#include "po-xerror.h"
#include "gettext.h"

#define _(str) gettext (str)

/* The syntax of a Desktop Entry file is defined at
   http://standards.freedesktop.org/desktop-entry-spec/latest/index.html.  */

desktop_reader_ty *
desktop_reader_alloc (desktop_reader_class_ty *method_table)
{
  desktop_reader_ty *reader;

  reader = (desktop_reader_ty *) xmalloc (method_table->size);
  reader->methods = method_table;
  if (method_table->constructor)
    method_table->constructor (reader);
  return reader;
}

void
desktop_reader_free (desktop_reader_ty *reader)
{
  if (reader->methods->destructor)
    reader->methods->destructor (reader);
  free (reader);
}

void
desktop_reader_handle_group (desktop_reader_ty *reader, const char *group)
{
  if (reader->methods->handle_group)
    reader->methods->handle_group (reader, group);
}

void
desktop_reader_handle_pair (desktop_reader_ty *reader,
                            lex_pos_ty *key_pos,
                            const char *key,
                            const char *locale,
                            const char *value)
{
  if (reader->methods->handle_pair)
    reader->methods->handle_pair (reader, key_pos, key, locale, value);
}

void
desktop_reader_handle_comment (desktop_reader_ty *reader, const char *s)
{
  if (reader->methods->handle_comment)
    reader->methods->handle_comment (reader, s);
}

void
desktop_reader_handle_text (desktop_reader_ty *reader, const char *s)
{
  if (reader->methods->handle_text)
    reader->methods->handle_text (reader, s);
}

/* Real filename, used in error messages about the input file.  */
static const char *real_file_name;

/* File name and line number.  */
extern lex_pos_ty gram_pos;

/* The input file stream.  */
static FILE *fp;


static int
phase1_getc ()
{
  int c;

  c = getc (fp);

  if (c == EOF)
    {
      if (ferror (fp))
        {
          const char *errno_description = strerror (errno);
          po_xerror (PO_SEVERITY_FATAL_ERROR, NULL, NULL, 0, 0, false,
                     xasprintf ("%s: %s",
                                xasprintf (_("error while reading \"%s\""),
                                           real_file_name),
                                errno_description));
        }
      return EOF;
    }

  return c;
}

static inline void
phase1_ungetc (int c)
{
  if (c != EOF)
    ungetc (c, fp);
}


static unsigned char phase2_pushback[2];
static int phase2_pushback_length;

static int
phase2_getc ()
{
  int c;

  if (phase2_pushback_length)
    c = phase2_pushback[--phase2_pushback_length];
  else
    {
      c = phase1_getc ();

      if (c == '\r')
        {
          int c2 = phase1_getc ();
          if (c2 == '\n')
            c = c2;
          else
            phase1_ungetc (c2);
        }
    }

  if (c == '\n')
    gram_pos.line_number++;

  return c;
}

static void
phase2_ungetc (int c)
{
  if (c == '\n')
    --gram_pos.line_number;
  if (c != EOF)
    phase2_pushback[phase2_pushback_length++] = c;
}

static char *
read_until_newline (void)
{
  char *buffer = NULL;
  size_t bufmax = 0;
  size_t buflen;

  buflen = 0;
  for (;;)
    {
      int c;

      c = phase2_getc ();

      if (buflen >= bufmax)
        {
          bufmax += 100;
          buffer = xrealloc (buffer, bufmax);
        }

      if (c == EOF || c == '\n')
        break;

      buffer[buflen++] = c;
    }
  buffer[buflen] = '\0';
  return buffer;
}

static char *
read_group_name (void)
{
  char *buffer = NULL;
  size_t bufmax = 0;
  size_t buflen;

  buflen = 0;
  for (;;)
    {
      int c;

      c = phase2_getc ();

      if (buflen >= bufmax)
        {
          bufmax += 100;
          buffer = xrealloc (buffer, bufmax);
        }

      if (c == EOF || c == '\n' || c == ']')
        break;

      buffer[buflen++] = c;
    }
  buffer[buflen] = '\0';
  return buffer;
}

static char *
read_key_name (const char **locale)
{
  char *buffer = NULL;
  size_t bufmax = 0;
  size_t buflen;
  const char *locale_start = NULL;

  buflen = 0;
  for (;;)
    {
      int c;

      c = phase2_getc ();

      if (buflen >= bufmax)
        {
          bufmax += 100;
          buffer = xrealloc (buffer, bufmax);
        }

      if (c == EOF || c == '\n')
        break;

      if (!locale_start)
        {
          if (c == '[')
            {
              buffer[buflen++] = '\0';
              locale_start = &buffer[buflen];
              continue;
            }
          else if (!c_isalnum (c) && c != '-')
            {
              phase2_ungetc (c);
              break;
            }
        }
      else
        {
          if (c == ']')
            {
              buffer[buflen++] = '\0';
              break;
            }
          else if (!c_isascii (c))
            {
              phase2_ungetc (c);
              break;
            }
        }

      buffer[buflen++] = c;
    }
  buffer[buflen] = '\0';

  if (locale_start)
    *locale = locale_start;

  return buffer;
}

void
desktop_parse (desktop_reader_ty *reader, FILE *file,
               const char *real_filename, const char *logical_filename)
{
  fp = file;
  real_file_name = real_filename;
  gram_pos.file_name = xstrdup (real_file_name);
  gram_pos.line_number = 1;

  for (;;)
    {
      int c;

      c = phase2_getc ();

      if (c == EOF)
        break;

      if (c == '[')
        {
          /* A group header.  */
          char *group_name;

          group_name = read_group_name ();

          do
            c = phase2_getc ();
          while (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f');

          if (c == EOF)
            break;

          phase2_ungetc (c);

          desktop_reader_handle_group (reader, group_name);
          free (group_name);
        }
      else if (c == '#')
        {
          /* A comment line.  */
          char *comment;

          comment = read_until_newline ();
          desktop_reader_handle_comment (reader, comment);
          free (comment);
        }
      else if (c_isalnum (c) || c == '-')
        {
          /* A key/value pair.  */
          char *key_name;
          const char *locale;

          phase2_ungetc (c);

          locale = NULL;
          key_name = read_key_name (&locale);
          do
            c = phase2_getc ();
          while (c == ' ' || c == '\t' || c == '\r' || c == '\f');

          if (c == EOF)
            break;

          if (c != '=')
            {
              po_xerror (PO_SEVERITY_FATAL_ERROR, NULL,
                         real_filename, gram_pos.line_number, 0, false,
                         xasprintf (_("missing '=' after \"%s\""), key_name));
            }
          else
            {
              char *value;

              do
                c = phase2_getc ();
              while (c == ' ' || c == '\t' || c == '\r' || c == '\f');

              if (c == EOF)
                break;

              phase2_ungetc (c);

              value = read_until_newline ();
              desktop_reader_handle_pair (reader, &gram_pos,
                                          key_name, locale, value);
              free (value);
            }
          free (key_name);
        }
      else
        {
          char *text;

          phase2_ungetc (c);

          text = read_until_newline ();
          desktop_reader_handle_text (reader, text);
          free (text);
        }
    }

  fp = NULL;
  real_file_name = NULL;
  gram_pos.line_number = 0;
}

char *
desktop_escape_string (const char *s, bool is_list)
{
  char *buffer, *p;

  p = buffer = XNMALLOC (strlen (s) * 2 + 1, char);

  /* The first character must not be a whitespace.  */
  if (*s == ' ')
    {
      p = stpcpy (p, "\\s");
      s++;
    }
  else if (*s == '\t')
    {
      p = stpcpy (p, "\\t");
      s++;
    }

  for (;; s++)
    {
      if (*s == '\0')
        {
          *p = '\0';
          break;
        }

      switch (*s)
        {
        case '\n':
          p = stpcpy (p, "\\n");
          break;
        case '\r':
          p = stpcpy (p, "\\r");
          break;
        case '\\':
          if (is_list && *(s + 1) == ';')
            {
              p = stpcpy (p, "\\;");
              s++;
            }
          else
            p = stpcpy (p, "\\\\");
          break;
        default:
          *p++ = *s;
          break;
        }
    }

  return buffer;
}

char *
desktop_unescape_string (const char *s, bool is_list)
{
  char *buffer, *p;

  p = buffer = XNMALLOC (strlen (s) + 1, char);
  for (;; s++)
    {
      if (*s == '\0')
        {
          *p = '\0';
          break;
        }

      if (*s == '\\')
        {
          s++;

          if (*s == '\0')
            {
              *p = '\0';
              break;
            }

          switch (*s)
            {
            case 's':
              *p++ = ' ';
              break;
            case 'n':
              *p++ = '\n';
              break;
            case 't':
              *p++ = '\t';
              break;
            case 'r':
              *p++ = '\r';
              break;
            case ';':
              p = stpcpy (p, "\\;");
              break;
            default:
              *p++ = *s;
              break;
            }
        }
      else
        *p++ = *s;
    }
  return buffer;
}

void
desktop_add_keyword (hash_table *keywords, const char *name, bool is_list)
{
  hash_insert_entry (keywords, name, strlen (name), (void *) is_list);
}

void
desktop_add_default_keywords (hash_table *keywords)
{
  /* When adding new keywords here, also update the documentation in
     xgettext.texi!  */
  desktop_add_keyword (keywords, "Name", false);
  desktop_add_keyword (keywords, "GenericName", false);
  desktop_add_keyword (keywords, "Comment", false);
  desktop_add_keyword (keywords, "Icon", false);
  desktop_add_keyword (keywords, "Keywords", true);
}
