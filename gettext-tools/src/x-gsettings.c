/* xgettext GSettings schema file backend.
   Copyright (C) 2002-2003, 2005-2009, 2013, 2015 Free Software
   Foundation, Inc.

   This file was written by Daiki Ueno <ueno@gnu.org>, 2013.

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
# include "config.h"
#endif

/* Specification.  */
#include "x-gsettings.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "xgettext.h"
#include "error.h"
#include "xerror.h"
#include "xvasprintf.h"
#include "basename.h"
#include "progname.h"
#include "xalloc.h"
#include "hash.h"
#include "po-charset.h"
#include "gettext.h"
#include "libexpat-compat.h"

#define _(s) gettext(s)


/* GSettings schema file is an XML based format.
   The syntax is defined in glib/gio/gschema.dtd and:
   https://developer.gnome.org/gio/unstable/GSettings.html  */


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;


/* ============================= XML parsing.  ============================= */

#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT

/* Accumulator for the extracted messages.  */
static message_list_ty *mlp;

/* Logical filename, used to label the extracted messages.  */
static char *logical_file_name;

/* XML parser.  */
static XML_Parser parser;

enum whitespace_type_ty
{
  none,
  normalize,
  strip
};
typedef enum whitespace_type_ty whitespace_type_ty;

struct element_state
{
  bool extract_string;
  whitespace_type_ty whitespace;
  char *extracted_context;
  int lineno;
  char *buffer;
  size_t bufmax;
  size_t buflen;
};
static struct element_state *stack;
static size_t stack_size;

static char *
normalize_whitespace (const char *text, whitespace_type_ty whitespace)
{
  if (whitespace == none)
    return xstrdup (text);
  else
    {
      char *result, *p;

      /* Strip whitespaces at the beginning/end of the text.  */
      result = xstrdup (text + strspn (text, " \t\n"));
      for (p = result + strlen (result);
           p > result && (*p == '\0' || *p == ' ' || *p == '\t' || *p == '\n');
           p--)
        ;
      if (p > result)
        *++p = '\0';

      /* Normalize whitespaces within the text.  */
      if (whitespace == normalize)
        {
          char *end = result + strlen (result);
          for (p = result; *p != '\0';)
            {
              size_t len = strspn (p, " \t\n");
              if (len > 0)
                {
                  *p = ' ';
                  memmove (p + 1, p + len, end - (p + len));
                  end -= len - 1;
                  *end = '\0';
                  p++;
                }
              p += strcspn (p, " \t\n");
            }
        }
      return result;
    }
}

/* Ensures stack_size >= size.  */
static void
ensure_stack_size (size_t size)
{
  if (size > stack_size)
    {
      stack_size = 2 * stack_size;
      if (stack_size < size)
        stack_size = size;
      stack =
        (struct element_state *)
        xrealloc (stack, stack_size * sizeof (struct element_state));
    }
}

static size_t stack_depth;

/* Callback called when <element> is seen.  */
static void
start_element_handler (void *userData, const char *name,
                       const char **attributes)
{
  struct element_state *p;

  /* Increase stack depth.  */
  stack_depth++;
  ensure_stack_size (stack_depth + 1);

  /* Don't extract a string for the containing element.  */
  stack[stack_depth - 1].extract_string = false;

  p = &stack[stack_depth];
  p->extract_string = extract_all;
  p->extracted_context = NULL;

  if (!p->extract_string)
    {
      bool has_translatable = false;
      whitespace_type_ty whitespace = none;
      const char *extracted_context = NULL;
      if (strcmp (name, "summary") == 0 || strcmp (name, "description") == 0)
        {
          has_translatable = true;
          whitespace = normalize;
        }
      else if (strcmp (name, "default") == 0)
        {
          const char *extracted_l10n = NULL;
          const char **attp = attributes;
          while (*attp != NULL)
            {
              if (strcmp (attp[0], "context") == 0)
                extracted_context = attp[1];
              else if (strcmp (attp[0], "l10n") == 0)
                extracted_l10n = attp[1];
              attp += 2;
            }
          if (extracted_l10n != NULL)
            {
              has_translatable = true;
              whitespace = strip;
            }
        }
      p->extract_string = has_translatable;
      p->whitespace = whitespace;
      p->extracted_context =
        (has_translatable && extracted_context != NULL
         ? xstrdup (extracted_context)
         : NULL); 
   }

  p->lineno = XML_GetCurrentLineNumber (parser);
  p->buffer = NULL;
  p->bufmax = 0;
  p->buflen = 0;
  if (!p->extract_string)
    savable_comment_reset ();
}

/* Callback called when </element> is seen.  */
static void
end_element_handler (void *userData, const char *name)
{
  struct element_state *p = &stack[stack_depth];

  /* Actually extract string.  */
  if (p->extract_string)
    {
      /* Don't extract the empty string.  */
      if (p->buflen > 0)
        {
          lex_pos_ty pos;

          if (p->buflen == p->bufmax)
            p->buffer = (char *) xrealloc (p->buffer, p->buflen + 1);
          p->buffer[p->buflen] = '\0';

          pos.file_name = logical_file_name;
          pos.line_number = p->lineno;

          if (p->buffer != NULL)
            {
              remember_a_message (mlp, p->extracted_context,
                                  normalize_whitespace (p->buffer,
                                                        p->whitespace),
                                  null_context, &pos,
                                  NULL, savable_comment);
              p->extracted_context = NULL;
            }
        }
    }

  /* Free memory for this stack level.  */
  if (p->extracted_context != NULL)
    free (p->extracted_context);
  if (p->buffer != NULL)
    free (p->buffer);

  /* Decrease stack depth.  */
  stack_depth--;

  savable_comment_reset ();
}

/* Callback called when some text is seen.  */
static void
character_data_handler (void *userData, const char *s, int len)
{
  struct element_state *p = &stack[stack_depth];

  /* Accumulate character data.  */
  if (len > 0)
    {
      if (p->buflen + len > p->bufmax)
        {
          p->bufmax = 2 * p->bufmax;
          if (p->bufmax < p->buflen + len)
            p->bufmax = p->buflen + len;
          p->buffer = (char *) xrealloc (p->buffer, p->bufmax);
        }
      memcpy (p->buffer + p->buflen, s, len);
      p->buflen += len;
    }
}

/* Callback called when some comment text is seen.  */
static void
comment_handler (void *userData, const char *data)
{
  /* Split multiline comment into lines, and remove leading and trailing
     whitespace.  */
  char *copy = xstrdup (data);
  char *p;
  char *q;

  for (p = copy; (q = strchr (p, '\n')) != NULL; p = q + 1)
    {
      while (p[0] == ' ' || p[0] == '\t')
        p++;
      while (q > p && (q[-1] == ' ' || q[-1] == '\t'))
        q--;
      *q = '\0';
      savable_comment_add (p);
    }
  q = p + strlen (p);
  while (p[0] == ' ' || p[0] == '\t')
    p++;
  while (q > p && (q[-1] == ' ' || q[-1] == '\t'))
    q--;
  *q = '\0';
  savable_comment_add (p);
  free (copy);
}


static void
do_extract_gsettings (FILE *fp,
                      const char *real_filename, const char *logical_filename,
                      msgdomain_list_ty *mdlp)
{
  mlp = mdlp->item[0]->messages;

  /* expat feeds us strings in UTF-8 encoding.  */
  xgettext_current_source_encoding = po_charset_utf8;

  logical_file_name = xstrdup (logical_filename);

  parser = XML_ParserCreate (NULL);
  if (parser == NULL)
    error (EXIT_FAILURE, 0, _("memory exhausted"));

  XML_SetElementHandler (parser, start_element_handler, end_element_handler);
  XML_SetCharacterDataHandler (parser, character_data_handler);
  XML_SetCommentHandler (parser, comment_handler);

  stack_depth = 0;

  while (!feof (fp))
    {
      char buf[4096];
      int count = fread (buf, 1, sizeof buf, fp);

      if (count == 0)
        {
          if (ferror (fp))
            error (EXIT_FAILURE, errno, _("\
error while reading \"%s\""), real_filename);
          /* EOF reached.  */
          break;
        }

      if (XML_Parse (parser, buf, count, 0) == 0)
        error (EXIT_FAILURE, 0, _("%s:%lu:%lu: %s"), logical_filename,
               (unsigned long) XML_GetCurrentLineNumber (parser),
               (unsigned long) XML_GetCurrentColumnNumber (parser) + 1,
               XML_ErrorString (XML_GetErrorCode (parser)));
    }

  if (XML_Parse (parser, NULL, 0, 1) == 0)
    error (EXIT_FAILURE, 0, _("%s:%lu:%lu: %s"), logical_filename,
           (unsigned long) XML_GetCurrentLineNumber (parser),
           (unsigned long) XML_GetCurrentColumnNumber (parser) + 1,
           XML_ErrorString (XML_GetErrorCode (parser)));

  XML_ParserFree (parser);

  /* Close scanner.  */
  logical_file_name = NULL;
  parser = NULL;
}

#endif

void
extract_gsettings (FILE *fp,
                   const char *real_filename, const char *logical_filename,
                   flag_context_list_table_ty *flag_table,
                   msgdomain_list_ty *mdlp)
{
#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT
  if (LIBEXPAT_AVAILABLE ())
    do_extract_gsettings (fp, real_filename, logical_filename, mdlp);
  else
#endif
    {
      multiline_error (xstrdup (""),
                       xasprintf (_("\
Language \"gsettings\" is not supported. %s relies on expat.\n\
This version was built without expat.\n"),
                                  basename (program_name)));
      exit (EXIT_FAILURE);
    }
}
