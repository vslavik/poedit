/* Unicode CLDR plural rule parser and converter
   Copyright (C) 2015 Free Software Foundation, Inc.

   This file was written by Daiki Ueno <ueno@gnu.org>, 2015.

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

#include "basename.h"
#include "cldr-plural-exp.h"
#include "c-ctype.h"
#include <errno.h>
#include <error.h>
#include <getopt.h>
#include "gettext.h"
#include "libexpat-compat.h"
#include <locale.h>
#include "progname.h"
#include "propername.h"
#include "relocatable.h"
#include <stdlib.h>
#include <string.h>
#include "xalloc.h"

#define _(s) gettext(s)

#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT
/* Locale name to extract.  */
static char *extract_locale;

/* CLDR plural rules extracted from XML.  */
static char *extracted_rules;

/* XML parser.  */
static XML_Parser parser;

/* Logical filename, used to label the extracted messages.  */
static char *logical_file_name;

struct element_state
{
  bool extract_rules;
  bool extract_string;
  char *count;
  int lineno;
  char *buffer;
  size_t bufmax;
  size_t buflen;
};
static struct element_state *stack;
static size_t stack_size;
static size_t stack_depth;

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

/* Callback called when <element> is seen.  */
static void
start_element_handler (void *userData, const char *name,
                       const char **attributes)
{
  struct element_state *p;

  if (!stack_depth && strcmp (name, "supplementalData") != 0)
    {
      error_at_line (0, 0,
                     logical_file_name,
                     XML_GetCurrentLineNumber (parser),
                     _("\
The root element <%s> is not allowed in a valid CLDR file"),
                     name);
    }

  /* Increase stack depth.  */
  stack_depth++;
  ensure_stack_size (stack_depth + 1);

  p = &stack[stack_depth];
  p->count = NULL;
  p->extract_rules = false;
  p->extract_string = false;
  p->lineno = XML_GetCurrentLineNumber (parser);
  p->buffer = NULL;
  p->bufmax = 0;
  p->buflen = 0;

  if (strcmp (name, "pluralRules") == 0)
    {
      const char *locales = NULL;
      const char **attp = attributes;
      while (*attp != NULL)
        {
          if (strcmp (attp[0], "locales") == 0)
            locales = attp[1];
          attp += 2;
        }
      if (locales)
        {
          const char *cp = locales;
          size_t length = strlen (extract_locale);
          while (*cp)
            {
              while (c_isspace (*cp))
                cp++;
              if (strncmp (cp, extract_locale, length) == 0
                  && (*(cp + length) == ' '
                      || *(cp + length) == '\n'
                      || *(cp + length) == '\0'))
                {
                  p->extract_rules = true;
                  break;
                }
              while (*cp && !c_isspace (*cp))
                cp++;
            }
        }
    }
  else if (stack_depth > 1 && strcmp (name, "pluralRule") == 0)
    {
      struct element_state *parent = &stack[stack_depth - 1];

      p->extract_string = parent->extract_rules;
      if (p->extract_string)
        {
          const char *count = NULL;
          const char **attp = attributes;
          while (*attp != NULL)
            {
              if (strcmp (attp[0], "count") == 0)
                count = attp[1];
              attp += 2;
            }
          p->count = xstrdup (count);
        }
    }
}

/* Callback called when </element> is seen.  */
static void
end_element_handler (void *userData, const char *name)
{
  struct element_state *p = &stack[stack_depth];

  if (p->extract_string && strcmp (name, "pluralRule") == 0)
    {
      struct element_state *parent = &stack[stack_depth - 1];
      size_t length;

      /* NUL terminate the buffer.  */
      if (p->buflen > 0)
        {
          if (p->buflen == p->bufmax)
            p->buffer = (char *) xrealloc (p->buffer, p->buflen + 1);
          p->buffer[p->buflen] = '\0';
        }

      length = strlen (p->count) + strlen (": ")
        + p->buflen + strlen ("; ");
      if (parent->buflen + length + 1 > parent->bufmax)
        {
          parent->bufmax = 2 * parent->bufmax;
          if (parent->bufmax < parent->buflen + length + 1)
            parent->bufmax = parent->buflen + length + 1;
          parent->buffer = (char *) xrealloc (parent->buffer, parent->bufmax);
        }
      sprintf (parent->buffer + parent->buflen,
               "%s: %s; ",
               p->count, p->buffer == NULL ? "" : p->buffer);
      parent->buflen += length;
      parent->buffer[parent->buflen] = '\0';
    }
  else if (p->extract_rules && strcmp (name, "pluralRules") == 0)
    {
      char *cp;

      /* NUL terminate the buffer.  */
      if (p->buflen > 0)
        {
          if (p->buflen == p->bufmax)
            p->buffer = (char *) xrealloc (p->buffer, p->buflen + 1);
          p->buffer[p->buflen] = '\0';
        }

      /* Scrub the last semicolon, if any.  */
      cp = strrchr (p->buffer, ';');
      if (cp)
        *cp = '\0';
      extracted_rules = xstrdup (p->buffer);
    }

  /* Free memory for this stack level.  */
  if (p->count != NULL)
    free (p->count);
  if (p->buffer != NULL)
    free (p->buffer);

  /* Decrease stack depth.  */
  stack_depth--;
}

/* Callback called when some text is seen.  */
static void
character_data_handler (void *userData, const char *s, int len)
{
  struct element_state *p = &stack[stack_depth];

  /* Accumulate character data.  */
  if (p->extract_string && len > 0)
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

static void
extract_rule (FILE *fp,
              const char *real_filename, const char *logical_filename,
              const char *locale)
{
  logical_file_name = xstrdup (logical_filename);
  extract_locale = xstrdup (locale);

  parser = XML_ParserCreate (NULL);
  if (parser == NULL)
    error (EXIT_FAILURE, 0, _("memory exhausted"));

  XML_SetElementHandler (parser, start_element_handler, end_element_handler);
  XML_SetCharacterDataHandler (parser, character_data_handler);

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

  /* Close scanner.  */
  free (logical_file_name);
  logical_file_name = NULL;

  free (extract_locale);
  extract_locale = NULL;

  XML_ParserFree (parser);
  parser = NULL;
}

#endif

/* Display usage information and exit.  */
static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try '%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION...] [LOCALE RULES]...\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Extract or convert Unicode CLDR plural rules.\n\
\n\
If both LOCALE and RULES are specified, it reads CLDR plural rules for\n\
LOCALE from RULES and print them in a form suitable for gettext use.\n\
If no argument is given, it reads CLDR plural rules from the standard input.\n\
"));
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
Similarly for optional arguments.\n\
"));
      printf ("\n");
      printf (_("\
  -c, --cldr                  print plural rules in the CLDR format\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf ("\n");
      /* TRANSLATORS: The placeholder indicates the bug-reporting address
         for this package.  Please add _another line_ saying
         "Report translation bugs to <...>\n" with the address for translation
         bugs (typically your translation team's web or email address).  */
      fputs (_("Report bugs to <bug-gnu-gettext@gnu.org>.\n"),
             stdout);
    }
  exit (status);
}

/* Long options.  */
static const struct option long_options[] =
{
  { "cldr", no_argument, NULL, 'c' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'V' },
  { NULL, 0, NULL, 0 }
};

int
main (int argc, char **argv)
{
  bool opt_cldr_format = false;
  bool do_help = false;
  bool do_version = false;
  int optchar;

  /* Set program name for messages.  */
  set_program_name (argv[0]);

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  bindtextdomain ("bison-runtime", relocate (BISON_LOCALEDIR));
  textdomain (PACKAGE);

  while ((optchar = getopt_long (argc, argv, "chV", long_options, NULL)) != EOF)
    switch (optchar)
      {
      case '\0':                /* Long option.  */
        break;

      case 'c':
        opt_cldr_format = true;
        break;

      case 'h':
        do_help = true;
        break;

      case 'V':
        do_version = true;
        break;

      default:
        usage (EXIT_FAILURE);
        /* NOTREACHED */
      }

  /* Version information requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", basename (program_name), PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
"),
              "2015");
      printf (_("Written by %s.\n"), proper_name ("Daiki Ueno"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  if (argc == optind + 2)
    {
      /* Two arguments: Read CLDR rules from a file.  */
#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT
      if (LIBEXPAT_AVAILABLE ())
        {
          const char *locale = argv[optind];
          const char *logical_filename = argv[optind + 1];
          FILE *fp;

          fp = fopen (logical_filename, "r");
          if (fp == NULL)
            error (1, 0, _("%s cannot be read"), logical_filename);

          extract_rule (fp, logical_filename, logical_filename, locale);
          if (extracted_rules == NULL)
            error (1, 0, _("cannot extract rules for %s"), locale);

          if (opt_cldr_format)
            printf ("%s\n", extracted_rules);
          else
            {
              struct cldr_plural_rule_list_ty *result;

              result = cldr_plural_parse (extracted_rules);
              if (result == NULL)
                error (1, 0, _("cannot parse CLDR rule"));

              cldr_plural_rule_list_print (result, stdout);
              cldr_plural_rule_list_free (result);
            }
          free (extracted_rules);
        }
      else
#endif
        {
          error (1, 0, _("extraction is not supported"));
        }
    }
  else if (argc == optind)
    {
      /* No argument: Read CLDR rules from standard input.  */
      char *line = NULL;
      size_t line_size = 0;
      for (;;)
        {
          int line_len;
          struct cldr_plural_rule_list_ty *result;

          line_len = getline (&line, &line_size, stdin);
          if (line_len < 0)
            break;
          if (line_len > 0 && line[line_len - 1] == '\n')
            line[--line_len] = '\0';

          result = cldr_plural_parse (line);
          if (result)
            {
              cldr_plural_rule_list_print (result, stdout);
              cldr_plural_rule_list_free (result);
            }
        }

      free (line);
    }
  else
    {
      error (1, 0, _("extra operand %s"), argv[optind]);
    }

  return 0;
}
