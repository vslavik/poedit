/* xgettext glade backend.
   Copyright (C) 2002-2003, 2005-2009, 2013 Free Software Foundation, Inc.

   This file was written by Bruno Haible <haible@clisp.cons.org>, 2002.

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
#include "x-glade.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if DYNLOAD_LIBEXPAT
# include <dlfcn.h>
#else
# if HAVE_LIBEXPAT
#  include <expat.h>
# endif
#endif

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

#define _(s) gettext(s)


/* Glade is an XML based format with three variants.  The syntax for
   each format is defined as follows.

   - Glade 1
     Some example files are contained in libglade-0.16.

   - Glade 2
     See http://library.gnome.org/devel/libglade/unstable/libglade-dtd.html

   - GtkBuilder
     See https://developer.gnome.org/gtk3/stable/GtkBuilder.html#BUILDER-UI  */


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

/* The keywords correspond to the translatable elements in Glade 1.
   For Glade 2 and GtkBuilder, translatable content is determined by
   the translatable="..." attribute, thus those keywords are not used.  */
static hash_table keywords;
static bool default_keywords = true;


void
x_glade_extract_all ()
{
  extract_all = true;
}


void
x_glade_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      if (keywords.table == NULL)
        hash_init (&keywords, 100);

      hash_insert_entry (&keywords, name, strlen (name), NULL);
    }
}

/* Finish initializing the keywords hash table.
   Called after argument processing, before each file is processed.  */
static void
init_keywords ()
{
  if (default_keywords)
    {
      /* When adding new keywords here, also update the documentation in
         xgettext.texi!  */
      x_glade_keyword ("label");
      x_glade_keyword ("title");
      x_glade_keyword ("text");
      x_glade_keyword ("format");
      x_glade_keyword ("copyright");
      x_glade_keyword ("comments");
      x_glade_keyword ("preview_text");
      x_glade_keyword ("tooltip");
      default_keywords = false;
    }
}


/* ======================= Different libexpat ABIs.  ======================= */

/* There are three different ABIs of libexpat, regarding the functions
   XML_GetCurrentLineNumber and XML_GetCurrentColumnNumber.
   In expat < 2.0, they return an 'int'.
   In expat >= 2.0, they return
     - a 'long' if expat was compiled with the default flags, or
     - a 'long long' if expat was compiled with -DXML_LARGE_SIZE.
   But the <expat.h> include file does not contain the information whether
   expat was compiled with -DXML_LARGE_SIZE; so the include file is lying!
   For this information, we need to call XML_GetFeatureList(), for
   expat >= 2.0.1; for expat = 2.0.0, we have to assume the default flags.  */

#if !DYNLOAD_LIBEXPAT

# if XML_MAJOR_VERSION >= 2

/* expat >= 2.0 -> Return type is 'int64_t' worst-case.  */

/* Put the function pointers into variables, because some GCC 4 versions
   generate an abort when we convert symbol address to different function
   pointer types.  */
static void *p_XML_GetCurrentLineNumber = (void *) &XML_GetCurrentLineNumber;
static void *p_XML_GetCurrentColumnNumber = (void *) &XML_GetCurrentColumnNumber;

/* Return true if libexpat was compiled with -DXML_LARGE_SIZE.  */
static bool
is_XML_LARGE_SIZE_ABI (void)
{
  static bool tested;
  static bool is_large;

  if (!tested)
    {
      const XML_Feature *features;

      is_large = false;
      for (features = XML_GetFeatureList (); features->name != NULL; features++)
        if (strcmp (features->name, "XML_LARGE_SIZE") == 0)
          {
            is_large = true;
            break;
          }

      tested = true;
    }
  return is_large;
}

static int64_t
GetCurrentLineNumber (XML_Parser parser)
{
  if (is_XML_LARGE_SIZE_ABI ())
    return ((int64_t (*) (XML_Parser)) p_XML_GetCurrentLineNumber) (parser);
  else
    return ((long (*) (XML_Parser)) p_XML_GetCurrentLineNumber) (parser);
}
#  define XML_GetCurrentLineNumber GetCurrentLineNumber

static int64_t
GetCurrentColumnNumber (XML_Parser parser)
{
  if (is_XML_LARGE_SIZE_ABI ())
    return ((int64_t (*) (XML_Parser)) p_XML_GetCurrentColumnNumber) (parser);
  else
    return ((long (*) (XML_Parser)) p_XML_GetCurrentColumnNumber) (parser);
}
#  define XML_GetCurrentColumnNumber GetCurrentColumnNumber

# else

/* expat < 2.0 -> Return type is 'int'.  */

# endif

#endif


/* ===================== Dynamic loading of libexpat.  ===================== */

#if DYNLOAD_LIBEXPAT

typedef struct
  {
    int major;
    int minor;
    int micro;
  }
  XML_Expat_Version;
enum XML_FeatureEnum { XML_FEATURE_END = 0 };
typedef struct
  {
    enum XML_FeatureEnum feature;
    const char *name;
    long int value;
  }
  XML_Feature;
typedef void *XML_Parser;
typedef char XML_Char;
typedef char XML_LChar;
enum XML_Error { XML_ERROR_NONE };
typedef void (*XML_StartElementHandler) (void *userData, const XML_Char *name, const XML_Char **atts);
typedef void (*XML_EndElementHandler) (void *userData, const XML_Char *name);
typedef void (*XML_CharacterDataHandler) (void *userData, const XML_Char *s, int len);
typedef void (*XML_CommentHandler) (void *userData, const XML_Char *data);

static XML_Expat_Version (*p_XML_ExpatVersionInfo) (void);
static const XML_Feature * (*p_XML_GetFeatureList) (void);
static XML_Parser (*p_XML_ParserCreate) (const XML_Char *encoding);
static void (*p_XML_SetElementHandler) (XML_Parser parser, XML_StartElementHandler start, XML_EndElementHandler end);
static void (*p_XML_SetCharacterDataHandler) (XML_Parser parser, XML_CharacterDataHandler handler);
static void (*p_XML_SetCommentHandler) (XML_Parser parser, XML_CommentHandler handler);
static int (*p_XML_Parse) (XML_Parser parser, const char *s, int len, int isFinal);
static enum XML_Error (*p_XML_GetErrorCode) (XML_Parser parser);
static void *p_XML_GetCurrentLineNumber;
static void *p_XML_GetCurrentColumnNumber;
static void (*p_XML_ParserFree) (XML_Parser parser);
static const XML_LChar * (*p_XML_ErrorString) (int code);

#define XML_ExpatVersionInfo (*p_XML_ExpatVersionInfo)
#define XML_GetFeatureList (*p_XML_GetFeatureList)

enum XML_Size_ABI { is_int, is_long, is_int64_t };

static enum XML_Size_ABI
get_XML_Size_ABI (void)
{
  static bool tested;
  static enum XML_Size_ABI abi;

  if (!tested)
    {
      if (XML_ExpatVersionInfo () .major >= 2)
        /* expat >= 2.0 -> XML_Size is 'int64_t' or 'long'.  */
        {
          const XML_Feature *features;

          abi = is_long;
          for (features = XML_GetFeatureList ();
               features->name != NULL;
               features++)
            if (strcmp (features->name, "XML_LARGE_SIZE") == 0)
              {
                abi = is_int64_t;
                break;
              }
        }
      else
        /* expat < 2.0 -> XML_Size is 'int'.  */
        abi = is_int;
      tested = true;
    }
  return abi;
}

#define XML_ParserCreate (*p_XML_ParserCreate)
#define XML_SetElementHandler (*p_XML_SetElementHandler)
#define XML_SetCharacterDataHandler (*p_XML_SetCharacterDataHandler)
#define XML_SetCommentHandler (*p_XML_SetCommentHandler)
#define XML_Parse (*p_XML_Parse)
#define XML_GetErrorCode (*p_XML_GetErrorCode)

static int64_t
XML_GetCurrentLineNumber (XML_Parser parser)
{
  switch (get_XML_Size_ABI ())
    {
    case is_int:
      return ((int (*) (XML_Parser)) p_XML_GetCurrentLineNumber) (parser);
    case is_long:
      return ((long (*) (XML_Parser)) p_XML_GetCurrentLineNumber) (parser);
    case is_int64_t:
      return ((int64_t (*) (XML_Parser)) p_XML_GetCurrentLineNumber) (parser);
    default:
      abort ();
    }
}

static int64_t
XML_GetCurrentColumnNumber (XML_Parser parser)
{
  switch (get_XML_Size_ABI ())
    {
    case is_int:
      return ((int (*) (XML_Parser)) p_XML_GetCurrentColumnNumber) (parser);
    case is_long:
      return ((long (*) (XML_Parser)) p_XML_GetCurrentColumnNumber) (parser);
    case is_int64_t:
      return ((int64_t (*) (XML_Parser)) p_XML_GetCurrentColumnNumber) (parser);
    default:
      abort ();
    }
}

#define XML_ParserFree (*p_XML_ParserFree)
#define XML_ErrorString (*p_XML_ErrorString)

static int libexpat_loaded = 0;

static bool
load_libexpat ()
{
  if (libexpat_loaded == 0)
    {
      void *handle;

      /* Try to load libexpat-2.x.  */
      handle = dlopen ("libexpat.so.1", RTLD_LAZY);
      if (handle == NULL)
        /* Try to load libexpat-1.x.  */
        handle = dlopen ("libexpat.so.0", RTLD_LAZY);
      if (handle != NULL
          && (p_XML_ExpatVersionInfo =
                (XML_Expat_Version (*) (void))
                dlsym (handle, "XML_ExpatVersionInfo")) != NULL
          && (p_XML_GetFeatureList =
                (const XML_Feature * (*) (void))
                dlsym (handle, "XML_GetFeatureList")) != NULL
          && (p_XML_ParserCreate =
                (XML_Parser (*) (const XML_Char *))
                dlsym (handle, "XML_ParserCreate")) != NULL
          && (p_XML_SetElementHandler =
                (void (*) (XML_Parser, XML_StartElementHandler, XML_EndElementHandler))
                dlsym (handle, "XML_SetElementHandler")) != NULL
          && (p_XML_SetCharacterDataHandler =
                (void (*) (XML_Parser, XML_CharacterDataHandler))
                dlsym (handle, "XML_SetCharacterDataHandler")) != NULL
          && (p_XML_SetCommentHandler =
                (void (*) (XML_Parser, XML_CommentHandler))
                dlsym (handle, "XML_SetCommentHandler")) != NULL
          && (p_XML_Parse =
                (int (*) (XML_Parser, const char *, int, int))
                dlsym (handle, "XML_Parse")) != NULL
          && (p_XML_GetErrorCode =
                (enum XML_Error (*) (XML_Parser))
                dlsym (handle, "XML_GetErrorCode")) != NULL
          && (p_XML_GetCurrentLineNumber =
                dlsym (handle, "XML_GetCurrentLineNumber")) != NULL
          && (p_XML_GetCurrentColumnNumber =
                dlsym (handle, "XML_GetCurrentColumnNumber")) != NULL
          && (p_XML_ParserFree =
                (void (*) (XML_Parser))
                dlsym (handle, "XML_ParserFree")) != NULL
          && (p_XML_ErrorString =
                (const XML_LChar * (*) (int))
                dlsym (handle, "XML_ErrorString")) != NULL)
        libexpat_loaded = 1;
      else
        libexpat_loaded = -1;
    }
  return libexpat_loaded >= 0;
}

#define LIBEXPAT_AVAILABLE() (load_libexpat ())

#elif HAVE_LIBEXPAT

#define LIBEXPAT_AVAILABLE() true

#endif

/* ============================= XML parsing.  ============================= */

#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT

/* Accumulator for the extracted messages.  */
static message_list_ty *mlp;

/* Logical filename, used to label the extracted messages.  */
static char *logical_file_name;

/* XML parser.  */
static XML_Parser parser;

struct element_state
{
  bool extract_string;
  bool extract_context;         /* used by Glade 2 */
  char *extracted_comment;      /* used by Glade 2 or GtkBuilder */
  char *extracted_context;      /* used by GtkBuilder */
  int lineno;
  char *buffer;
  size_t bufmax;
  size_t buflen;
};
static struct element_state *stack;
static size_t stack_size;

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

/* Parser logic for each Glade compatible file format.  */
struct element_parser
{
  void (*start_element) (struct element_state *p, const char *name,
                         const char **attributes);
  void (*end_element) (struct element_state *p, const char *name);
};
static struct element_parser *element_parser;

static void
start_element_null (struct element_state *p, const char *name,
                    const char **attributes)
{
}

static void
end_element_null (struct element_state *p, const char *name)
{
}

static void
start_element_glade1 (struct element_state *p, const char *name,
                      const char **attributes)
{
  void *hash_result;

  /* In Glade 1, a few specific elements are translatable.  */
  if (!p->extract_string)
    p->extract_string =
      (hash_find_entry (&keywords, name, strlen (name), &hash_result) == 0);
}

static void
end_element_glade1 (struct element_state *p, const char *name)
{
  lex_pos_ty pos;

  pos.file_name = logical_file_name;
  pos.line_number = p->lineno;

  if (p->buffer != NULL)
    {
      remember_a_message (mlp, NULL, p->buffer,
                          null_context, &pos,
                          p->extracted_comment, savable_comment);
      p->buffer = NULL;
    }
}

static void
start_element_glade2 (struct element_state *p, const char *name,
                      const char **attributes)
{
  /* In Glade 2, all <property> and <atkproperty> elements are translatable
     that have the attribute translatable="yes".
     See <http://library.gnome.org/devel/libglade/unstable/libglade-dtd.html>.
     The translator comment is found in the attribute comments="...".
     See <http://live.gnome.org/TranslationProject/DevGuidelines/Use comments>.
     If the element has the attribute context="yes", the content of
     the element is in the form "msgctxt|msgid".  */
  if (!p->extract_string
      && (strcmp (name, "property") == 0 || strcmp (name, "atkproperty") == 0))
    {
      bool has_translatable = false;
      bool has_context = false;
      const char *extracted_comment = NULL;
      const char **attp = attributes;
      while (*attp != NULL)
        {
          if (strcmp (attp[0], "translatable") == 0)
            has_translatable = (strcmp (attp[1], "yes") == 0);
          else if (strcmp (attp[0], "comments") == 0)
            extracted_comment = attp[1];
          else if (strcmp (attp[0], "context") == 0)
            has_context = (strcmp (attp[1], "yes") == 0);
          attp += 2;
        }
      p->extract_string = has_translatable;
      p->extract_context = has_context;
      p->extracted_comment =
        (has_translatable && extracted_comment != NULL
         ? xstrdup (extracted_comment)
         : NULL);
    }

  /* In Glade 2, the attribute description="..." of <atkaction>
     element is also translatable.  */
  if (!p->extract_string
      && strcmp (name, "atkaction") == 0)
    {
      const char **attp = attributes;
      while (*attp != NULL)
        {
          if (strcmp (attp[0], "description") == 0)
            {
              if (strcmp (attp[1], "") != 0)
                {
                  lex_pos_ty pos;

                  pos.file_name = logical_file_name;
                  pos.line_number = XML_GetCurrentLineNumber (parser);

                  remember_a_message (mlp, NULL, xstrdup (attp[1]),
                                      null_context, &pos,
                                      NULL, savable_comment);
                }
              break;
            }
          attp += 2;
        }
    }
}

static void
end_element_glade2 (struct element_state *p, const char *name)
{
  lex_pos_ty pos;
  char *msgid = NULL;
  char *msgctxt = NULL;

  pos.file_name = logical_file_name;
  pos.line_number = p->lineno;

  if (p->extract_context)
    {
      char *separator = strchr (p->buffer, '|');

      if (separator == NULL)
        {
          error_with_progname = false;
          error_at_line (0, 0,
                         pos.file_name,
                         pos.line_number,
                         _("\
Missing context for the string extracted from '%s' element"),
                         name);
          error_with_progname = true;
        }
      else
        {
          *separator = '\0';
          msgid = xstrdup (separator + 1);
          msgctxt = xstrdup (p->buffer);
        }
    }
  else
    {
      msgid = p->buffer;
      p->buffer = NULL;
    }

  if (msgid != NULL)
    remember_a_message (mlp, msgctxt, msgid,
                        null_context, &pos,
                        p->extracted_comment, savable_comment);
}

static void
start_element_gtkbuilder (struct element_state *p, const char *name,
                          const char **attributes)
{
  /* In GtkBuilder (used by Glade 3), all elements are translatable
     that have the attribute translatable="yes".
     See <https://developer.gnome.org/gtk3/stable/GtkBuilder.html#BUILDER-UI>.
     The translator comment is found in the attribute comments="..."
     and context is found in the attribute context="...".  */
  if (!p->extract_string)
    {
      bool has_translatable = false;
      const char *extracted_comment = NULL;
      const char *extracted_context = NULL;
      const char **attp = attributes;
      while (*attp != NULL)
        {
          if (strcmp (attp[0], "translatable") == 0)
            has_translatable = (strcmp (attp[1], "yes") == 0);
          else if (strcmp (attp[0], "comments") == 0)
            extracted_comment = attp[1];
          else if (strcmp (attp[0], "context") == 0)
            extracted_context = attp[1];
          attp += 2;
        }
      p->extract_string = has_translatable;
      p->extracted_comment =
        (has_translatable && extracted_comment != NULL
         ? xstrdup (extracted_comment)
         : NULL);
      p->extracted_context =
        (has_translatable && extracted_context != NULL
         ? xstrdup (extracted_context)
         : NULL); 
   }
}

static void
end_element_gtkbuilder (struct element_state *p, const char *name)
{
  lex_pos_ty pos;

  pos.file_name = logical_file_name;
  pos.line_number = p->lineno;

  if (p->buffer != NULL)
    {
      remember_a_message (mlp, p->extracted_context, p->buffer,
                          null_context, &pos,
                          p->extracted_comment, savable_comment);
      p->buffer = NULL;
      p->extracted_context = NULL;
    }
}

static struct element_parser element_parser_null =
{
  start_element_null,
  end_element_null
};

static struct element_parser element_parser_glade1 =
{
  start_element_glade1,
  end_element_glade1
};

static struct element_parser element_parser_glade2 =
{
  start_element_glade2,
  end_element_glade2
};

static struct element_parser element_parser_gtkbuilder =
{
  start_element_gtkbuilder,
  end_element_gtkbuilder
};

/* Callback called when <element> is seen.  */
static void
start_element_handler (void *userData, const char *name,
                       const char **attributes)
{
  struct element_state *p;

  if (!stack_depth)
    {
      if (strcmp (name, "GTK-Interface") == 0)
        element_parser = &element_parser_glade1;
      else if (strcmp (name, "glade-interface") == 0)
        element_parser = &element_parser_glade2;
      else if (strcmp (name, "interface") == 0)
        element_parser = &element_parser_gtkbuilder;
      else
        {
          element_parser = &element_parser_null;
          error_with_progname = false;
          error_at_line (0, 0,
                         logical_file_name,
                         XML_GetCurrentLineNumber (parser),
                         _("\
The root element <%s> is not allowed in a valid Glade file"),
                         name);
          error_with_progname = true;
        }
    }

  /* Increase stack depth.  */
  stack_depth++;
  ensure_stack_size (stack_depth + 1);

  /* Don't extract a string for the containing element.  */
  stack[stack_depth - 1].extract_string = false;

  p = &stack[stack_depth];
  p->extract_string = extract_all;
  p->extract_context = false;
  p->extracted_comment = NULL;
  p->extracted_context = NULL;

  element_parser->start_element (p, name, attributes);

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
          if (p->buflen == p->bufmax)
            p->buffer = (char *) xrealloc (p->buffer, p->buflen + 1);
          p->buffer[p->buflen] = '\0';

          element_parser->end_element (p, name);
        }
    }

  /* Free memory for this stack level.  */
  if (p->extracted_comment != NULL)
    free (p->extracted_comment);
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
do_extract_glade (FILE *fp,
                  const char *real_filename, const char *logical_filename,
                  msgdomain_list_ty *mdlp)
{
  mlp = mdlp->item[0]->messages;

  /* expat feeds us strings in UTF-8 encoding.  */
  xgettext_current_source_encoding = po_charset_utf8;

  logical_file_name = xstrdup (logical_filename);

  init_keywords ();

  parser = XML_ParserCreate (NULL);
  if (parser == NULL)
    error (EXIT_FAILURE, 0, _("memory exhausted"));

  XML_SetElementHandler (parser, start_element_handler, end_element_handler);
  XML_SetCharacterDataHandler (parser, character_data_handler);
  XML_SetCommentHandler (parser, comment_handler);

  stack_depth = 0;
  element_parser = &element_parser_null;

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
extract_glade (FILE *fp,
               const char *real_filename, const char *logical_filename,
               flag_context_list_table_ty *flag_table,
               msgdomain_list_ty *mdlp)
{
#if DYNLOAD_LIBEXPAT || HAVE_LIBEXPAT
  if (LIBEXPAT_AVAILABLE ())
    do_extract_glade (fp, real_filename, logical_filename, mdlp);
  else
#endif
    {
      multiline_error (xstrdup (""),
                       xasprintf (_("\
Language \"glade\" is not supported. %s relies on expat.\n\
This version was built without expat.\n"),
                                  basename (program_name)));
      exit (EXIT_FAILURE);
    }
}
