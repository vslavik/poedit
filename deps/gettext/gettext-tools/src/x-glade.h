/* xgettext glade backend.
   Copyright (C) 2002-2003, 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2002.

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


#include <stdio.h>

#include "message.h"
#include "xgettext.h"


#ifdef __cplusplus
extern "C" {
#endif


#define EXTENSIONS_GLADE \
  { "glade",     "glade"    },                                          \
  { "glade2",    "glade"    },                                          \

#define SCANNERS_GLADE \
  { "glade",            extract_glade, NULL, NULL, NULL },              \

/* Scan a glade XML file and add its translatable strings to mdlp.  */
extern void extract_glade (FILE *fp, const char *real_filename,
                           const char *logical_filename,
                           flag_context_list_table_ty *flag_table,
                           msgdomain_list_ty *mdlp);


/* Handling of options specific to this language.  */

extern void x_glade_extract_all (void);
extern void x_glade_keyword (const char *name);


#ifdef __cplusplus
}
#endif
