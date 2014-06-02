/* xgettext libexpat compatibility.
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

#include <stdlib.h>
#include <string.h>

#if DYNLOAD_LIBEXPAT
# include <dlfcn.h>
#else
# if HAVE_LIBEXPAT
#  include <expat.h>
# endif
#endif

/* Keep the references to XML_GetCurrent{Line,Column}Number symbols
   before loading libexpat-compat.h, since they are redefined to
   rpl_XML_GetCurrent{Line,Column}Number .  */
#if !DYNLOAD_LIBEXPAT && XML_MAJOR_VERSION >= 2
static void *p_XML_GetCurrentLineNumber = (void *) &XML_GetCurrentLineNumber;
static void *p_XML_GetCurrentColumnNumber = (void *) &XML_GetCurrentColumnNumber;
#endif

#include "libexpat-compat.h"

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

#if !DYNLOAD_LIBEXPAT && XML_MAJOR_VERSION >= 2

/* expat >= 2.0 -> Return type is 'int64_t' worst-case.  */

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

int64_t
rpl_XML_GetCurrentLineNumber (XML_Parser parser)
{
  if (is_XML_LARGE_SIZE_ABI ())
    return ((int64_t (*) (XML_Parser)) p_XML_GetCurrentLineNumber) (parser);
  else
    return ((long (*) (XML_Parser)) p_XML_GetCurrentLineNumber) (parser);
}

int64_t
rpl_XML_GetCurrentColumnNumber (XML_Parser parser)
{
  if (is_XML_LARGE_SIZE_ABI ())
    return ((int64_t (*) (XML_Parser)) p_XML_GetCurrentColumnNumber) (parser);
  else
    return ((long (*) (XML_Parser)) p_XML_GetCurrentColumnNumber) (parser);
}
#endif


/* ===================== Dynamic loading of libexpat.  ===================== */

#if DYNLOAD_LIBEXPAT

static XML_Expat_Version (*p_XML_ExpatVersionInfo) (void);

XML_Expat_Version
XML_ExpatVersionInfo (void)
{
  return (*p_XML_ExpatVersionInfo) ();
}

static const XML_Feature * (*p_XML_GetFeatureList) (void);

const XML_Feature *
XML_GetFeatureList (void)
{
  return (*p_XML_GetFeatureList) ();
}

enum XML_Size_ABI
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

static XML_Parser (*p_XML_ParserCreate) (const XML_Char *encoding);

XML_Parser
XML_ParserCreate (const XML_Char *encoding)
{
  return (*p_XML_ParserCreate) (encoding);
}

static void (*p_XML_SetElementHandler) (XML_Parser parser,
                                        XML_StartElementHandler start,
                                        XML_EndElementHandler end);

void
XML_SetElementHandler (XML_Parser parser,
                       XML_StartElementHandler start,
                       XML_EndElementHandler end)
{
  (*p_XML_SetElementHandler) (parser, start, end);
}


static void (*p_XML_SetCharacterDataHandler) (XML_Parser parser,
                                              XML_CharacterDataHandler handler);

void
XML_SetCharacterDataHandler (XML_Parser parser,
                             XML_CharacterDataHandler handler)
{
  (*p_XML_SetCharacterDataHandler) (parser, handler);
}


static void (*p_XML_SetCommentHandler) (XML_Parser parser,
                                        XML_CommentHandler handler);

void
XML_SetCommentHandler (XML_Parser parser, XML_CommentHandler handler)
{
  (*p_XML_SetCommentHandler) (parser, handler);
}


static int (*p_XML_Parse) (XML_Parser parser, const char *s,
                           int len, int isFinal);

int
XML_Parse (XML_Parser parser, const char *s, int len, int isFinal)
{
  return (*p_XML_Parse) (parser, s, len, isFinal);
}


static enum XML_Error (*p_XML_GetErrorCode) (XML_Parser parser);

enum XML_Error
XML_GetErrorCode (XML_Parser parser)
{
  return (*p_XML_GetErrorCode) (parser);
}


static void *p_XML_GetCurrentLineNumber;

int64_t
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

static void *p_XML_GetCurrentColumnNumber;

int64_t
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


static const XML_LChar * (*p_XML_ErrorString) (int code);

const XML_LChar *
XML_ErrorString (int code)
{
  return (*p_XML_ErrorString) (code);
}

static void (*p_XML_ParserFree) (XML_Parser parser);

void
XML_ParserFree (XML_Parser parser)
{
  return (*p_XML_ParserFree) (parser);
}

static int libexpat_loaded = 0;

bool
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

#endif
