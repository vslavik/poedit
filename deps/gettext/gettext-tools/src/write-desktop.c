/* Writing Desktop Entry files.
   Copyright (C) 1995-1998, 2000-2003, 2005-2006, 2008-2009, 2014-2015
   Free Software Foundation, Inc.
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
#include "write-desktop.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "msgl-iconv.h"
#include "po-charset.h"
#include "read-catalog.h"
#include "read-po.h"
#include "read-desktop.h"
#include "fwriteerror.h"
#include "xalloc.h"
#include "gettext.h"

#define _(str) gettext (str)

typedef struct msgfmt_desktop_reader_ty msgfmt_desktop_reader_ty;
struct msgfmt_desktop_reader_ty
{
  DESKTOP_READER_TY
  string_list_ty *languages;
  message_list_ty **messages;
  hash_table *keywords;
  FILE *output_file;
};

static void
msgfmt_desktop_handle_group (struct desktop_reader_ty *reader,
                             const char *group)
{
  msgfmt_desktop_reader_ty *msgfmt_reader = (msgfmt_desktop_reader_ty *) reader;

  fprintf (msgfmt_reader->output_file, "[%s]\n", group);
}

static void
msgfmt_desktop_handle_pair (desktop_reader_ty *reader,
                            lex_pos_ty *key_pos,
                            const char *key,
                            const char *locale,
                            const char *value)
{
  msgfmt_desktop_reader_ty *msgfmt_reader = (msgfmt_desktop_reader_ty *) reader;
  void *keyword_value;

  if (!locale)
    {
      /* Write translated pair, if any.  */
      if (hash_find_entry (msgfmt_reader->keywords, key, strlen (key),
                           &keyword_value) == 0)
        {
          bool is_list = (bool) keyword_value;
          char *unescaped = desktop_unescape_string (value, is_list);
          size_t i;

          for (i = 0; i < msgfmt_reader->languages->nitems; i++)
            {
              const char *language = msgfmt_reader->languages->item[i];
              message_list_ty *mlp = msgfmt_reader->messages[i];
              message_ty *mp;

              mp = message_list_search (mlp, NULL, unescaped);
              if (mp && *mp->msgstr != '\0')
                {
                  char *escaped;

                  escaped = desktop_escape_string (mp->msgstr, is_list);
                  fprintf (msgfmt_reader->output_file,
                           "%s[%s]=%s\n",
                           key, language, escaped);
                  free (escaped);
                }
            }
          free (unescaped);
        }

      /* Write untranslated pair.  */
      fprintf (msgfmt_reader->output_file, "%s=%s\n", key, value);
    }
  else
    /* Preserve already translated pair.  */
    fprintf (msgfmt_reader->output_file, "%s[%s]=%s\n", key, locale, value);
}

static void
msgfmt_desktop_handle_comment (struct desktop_reader_ty *reader, const char *s)
{
  msgfmt_desktop_reader_ty *msgfmt_reader = (msgfmt_desktop_reader_ty *) reader;

  fputc ('#', msgfmt_reader->output_file);
  fputs (s, msgfmt_reader->output_file);
  fputc ('\n', msgfmt_reader->output_file);
}

static void
msgfmt_desktop_handle_blank (struct desktop_reader_ty *reader, const char *s)
{
  msgfmt_desktop_reader_ty *msgfmt_reader = (msgfmt_desktop_reader_ty *) reader;

  fputs (s, msgfmt_reader->output_file);
  fputc ('\n', msgfmt_reader->output_file);
}

desktop_reader_class_ty msgfmt_methods =
  {
    sizeof (msgfmt_desktop_reader_ty),
    NULL,
    NULL,
    msgfmt_desktop_handle_group,
    msgfmt_desktop_handle_pair,
    msgfmt_desktop_handle_comment,
    msgfmt_desktop_handle_blank
  };

int
msgdomain_write_desktop_bulk (string_list_ty *languages,
                              message_list_ty **messages,
                              const char *template_file_name,
                              hash_table *keywords,
                              const char *file_name)
{
  desktop_reader_ty *reader;
  msgfmt_desktop_reader_ty *msgfmt_reader;
  FILE *template_file;

  reader = desktop_reader_alloc (&msgfmt_methods);
  msgfmt_reader = (msgfmt_desktop_reader_ty *) reader;

  msgfmt_reader->languages = languages;
  msgfmt_reader->messages = messages;
  msgfmt_reader->keywords = keywords;

  if (strcmp (file_name, "-") == 0)
    msgfmt_reader->output_file = stdout;
  else
    {
      msgfmt_reader->output_file = fopen (file_name, "w");
      if (msgfmt_reader->output_file == NULL)
        {
          desktop_reader_free (reader);
          error (EXIT_SUCCESS,
                 errno, _("error while opening \"%s\" for writing"),
                 file_name);
          return 1;
        }
    }

  template_file = fopen (template_file_name, "r");
  if (template_file == NULL)
    {
      desktop_reader_free (reader);
      error (EXIT_SUCCESS,
             errno, _("error while opening \"%s\" for reading"),
             template_file_name);
      return 1;
    }

  desktop_parse (reader, template_file, template_file_name, template_file_name);

  /* Make sure nothing went wrong.  */
  if (fwriteerror (msgfmt_reader->output_file))
    error (EXIT_FAILURE, errno, _("error while writing \"%s\" file"),
           file_name);

  desktop_reader_free (reader);

  return 0;
}

int
msgdomain_write_desktop (message_list_ty *mlp,
                         const char *canon_encoding,
                         const char *locale_name,
                         const char *template_file_name,
                         hash_table *keywords,
                         const char *file_name)
{
  string_list_ty *languages;
  message_list_ty **messages;
  int retval;

  /* Convert the messages to Unicode.  */
  iconv_message_list (mlp, canon_encoding, po_charset_utf8, NULL);

  languages = string_list_alloc ();
  string_list_append (languages, locale_name);

  messages = XNMALLOC (1, message_list_ty *);
  messages[0] = mlp;

  retval = msgdomain_write_desktop_bulk (languages,
                                         messages,
                                         template_file_name,
                                         keywords,
                                         file_name);

  string_list_free (languages);
  free (messages);

  return retval;
}
