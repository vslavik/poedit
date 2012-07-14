/* A substitute for ISO C99 <wchar.h>, for platforms that have issues.

   Copyright (C) 2007-2010 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Eric Blake.  */

/*
 * ISO C 99 <wchar.h> for platforms that have issues.
 * <http://www.opengroup.org/susv3xbd/wchar.h.html>
 *
 * For now, this just ensures proper prerequisite inclusion order and
 * the declaration of wcwidth().
 */

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if defined __need_mbstate_t || defined __need_wint_t || (defined __hpux && ((defined _INTTYPES_INCLUDED && !defined strtoimax) || defined _GL_JUST_INCLUDE_SYSTEM_WCHAR_H)) || defined _GL_ALREADY_INCLUDING_WCHAR_H
/* Special invocation convention:
   - Inside glibc and uClibc header files.
   - On HP-UX 11.00 we have a sequence of nested includes
     <wchar.h> -> <stdlib.h> -> <stdint.h>, and the latter includes <wchar.h>,
     once indirectly <stdint.h> -> <sys/types.h> -> <inttypes.h> -> <wchar.h>
     and once directly.  In both situations 'wint_t' is not yet defined,
     therefore we cannot provide the function overrides; instead include only
     the system's <wchar.h>.
   - On IRIX 6.5, similarly, we have an include <wchar.h> -> <wctype.h>, and
     the latter includes <wchar.h>.  But here, we have no way to detect whether
     <wctype.h> is completely included or is still being included.  */

#@INCLUDE_NEXT@ @NEXT_WCHAR_H@

#else
/* Normal invocation convention.  */

#ifndef _GL_WCHAR_H

#define _GL_ALREADY_INCLUDING_WCHAR_H

/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.
   But avoid namespace pollution on glibc systems.  */
#ifndef __GLIBC__
# include <stddef.h>
# include <stdio.h>
# include <time.h>
#endif

/* Include the original <wchar.h> if it exists.
   Some builds of uClibc lack it.  */
/* The include_next requires a split double-inclusion guard.  */
#if @HAVE_WCHAR_H@
# @INCLUDE_NEXT@ @NEXT_WCHAR_H@
#endif

#undef _GL_ALREADY_INCLUDING_WCHAR_H

#ifndef _GL_WCHAR_H
#define _GL_WCHAR_H

/* The definitions of _GL_FUNCDECL_RPL etc. are copied here.  */

/* The definition of _GL_ARG_NONNULL is copied here.  */

/* The definition of _GL_WARN_ON_USE is copied here.  */


/* Define wint_t and WEOF.  (Also done in wctype.in.h.)  */
#if !@HAVE_WINT_T@ && !defined wint_t
# define wint_t int
# ifndef WEOF
#  define WEOF -1
# endif
#else
# ifndef WEOF
#  define WEOF ((wint_t) -1)
# endif
#endif


/* Override mbstate_t if it is too small.
   On IRIX 6.5, sizeof (mbstate_t) == 1, which is not sufficient for
   implementing mbrtowc for encodings like UTF-8.  */
#if !(@HAVE_MBSINIT@ && @HAVE_MBRTOWC@) || @REPLACE_MBSTATE_T@
typedef int rpl_mbstate_t;
# undef mbstate_t
# define mbstate_t rpl_mbstate_t
# define GNULIB_defined_mbstate_t 1
#endif


/* Convert a single-byte character to a wide character.  */
#if @GNULIB_BTOWC@
# if @REPLACE_BTOWC@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef btowc
#   define btowc rpl_btowc
#  endif
_GL_FUNCDECL_RPL (btowc, wint_t, (int c));
_GL_CXXALIAS_RPL (btowc, wint_t, (int c));
# else
#  if !@HAVE_BTOWC@
_GL_FUNCDECL_SYS (btowc, wint_t, (int c));
#  endif
_GL_CXXALIAS_SYS (btowc, wint_t, (int c));
# endif
_GL_CXXALIASWARN (btowc);
#elif defined GNULIB_POSIXCHECK
# undef btowc
# if HAVE_RAW_DECL_BTOWC
_GL_WARN_ON_USE (btowc, "btowc is unportable - "
                 "use gnulib module btowc for portability");
# endif
#endif


/* Convert a wide character to a single-byte character.  */
#if @GNULIB_WCTOB@
# if @REPLACE_WCTOB@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wctob
#   define wctob rpl_wctob
#  endif
_GL_FUNCDECL_RPL (wctob, int, (wint_t wc));
_GL_CXXALIAS_RPL (wctob, int, (wint_t wc));
# else
#  if !defined wctob && !@HAVE_DECL_WCTOB@
/* wctob is provided by gnulib, or wctob exists but is not declared.  */
_GL_FUNCDECL_SYS (wctob, int, (wint_t wc));
#  endif
_GL_CXXALIAS_SYS (wctob, int, (wint_t wc));
# endif
_GL_CXXALIASWARN (wctob);
#elif defined GNULIB_POSIXCHECK
# undef wctob
# if HAVE_RAW_DECL_WCTOB
_GL_WARN_ON_USE (wctob, "wctob is unportable - "
                 "use gnulib module wctob for portability");
# endif
#endif


/* Test whether *PS is in the initial state.  */
#if @GNULIB_MBSINIT@
# if @REPLACE_MBSINIT@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbsinit
#   define mbsinit rpl_mbsinit
#  endif
_GL_FUNCDECL_RPL (mbsinit, int, (const mbstate_t *ps));
_GL_CXXALIAS_RPL (mbsinit, int, (const mbstate_t *ps));
# else
#  if !@HAVE_MBSINIT@
_GL_FUNCDECL_SYS (mbsinit, int, (const mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (mbsinit, int, (const mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbsinit);
#elif defined GNULIB_POSIXCHECK
# undef mbsinit
# if HAVE_RAW_DECL_MBSINIT
_GL_WARN_ON_USE (mbsinit, "mbsinit is unportable - "
                 "use gnulib module mbsinit for portability");
# endif
#endif


/* Convert a multibyte character to a wide character.  */
#if @GNULIB_MBRTOWC@
# if @REPLACE_MBRTOWC@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbrtowc
#   define mbrtowc rpl_mbrtowc
#  endif
_GL_FUNCDECL_RPL (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
_GL_CXXALIAS_RPL (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
# else
#  if !@HAVE_MBRTOWC@
_GL_FUNCDECL_SYS (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (mbrtowc, size_t,
                  (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbrtowc);
#elif defined GNULIB_POSIXCHECK
# undef mbrtowc
# if HAVE_RAW_DECL_MBRTOWC
_GL_WARN_ON_USE (mbrtowc, "mbrtowc is unportable - "
                 "use gnulib module mbrtowc for portability");
# endif
#endif


/* Recognize a multibyte character.  */
#if @GNULIB_MBRLEN@
# if @REPLACE_MBRLEN@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbrlen
#   define mbrlen rpl_mbrlen
#  endif
_GL_FUNCDECL_RPL (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
_GL_CXXALIAS_RPL (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
# else
#  if !@HAVE_MBRLEN@
_GL_FUNCDECL_SYS (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (mbrlen, size_t, (const char *s, size_t n, mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbrlen);
#elif defined GNULIB_POSIXCHECK
# undef mbrlen
# if HAVE_RAW_DECL_MBRLEN
_GL_WARN_ON_USE (mbrlen, "mbrlen is unportable - "
                 "use gnulib module mbrlen for portability");
# endif
#endif


/* Convert a string to a wide string.  */
#if @GNULIB_MBSRTOWCS@
# if @REPLACE_MBSRTOWCS@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbsrtowcs
#   define mbsrtowcs rpl_mbsrtowcs
#  endif
_GL_FUNCDECL_RPL (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len,
                   mbstate_t *ps));
# else
#  if !@HAVE_MBSRTOWCS@
_GL_FUNCDECL_SYS (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (mbsrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbsrtowcs);
#elif defined GNULIB_POSIXCHECK
# undef mbsrtowcs
# if HAVE_RAW_DECL_MBSRTOWCS
_GL_WARN_ON_USE (mbsrtowcs, "mbsrtowcs is unportable - "
                 "use gnulib module mbsrtowcs for portability");
# endif
#endif


/* Convert a string to a wide string.  */
#if @GNULIB_MBSNRTOWCS@
# if @REPLACE_MBSNRTOWCS@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef mbsnrtowcs
#   define mbsnrtowcs rpl_mbsnrtowcs
#  endif
_GL_FUNCDECL_RPL (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# else
#  if !@HAVE_MBSNRTOWCS@
_GL_FUNCDECL_SYS (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (mbsnrtowcs, size_t,
                  (wchar_t *dest, const char **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (mbsnrtowcs);
#elif defined GNULIB_POSIXCHECK
# undef mbsnrtowcs
# if HAVE_RAW_DECL_MBSNRTOWCS
_GL_WARN_ON_USE (mbsnrtowcs, "mbsnrtowcs is unportable - "
                 "use gnulib module mbsnrtowcs for portability");
# endif
#endif


/* Convert a wide character to a multibyte character.  */
#if @GNULIB_WCRTOMB@
# if @REPLACE_WCRTOMB@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcrtomb
#   define wcrtomb rpl_wcrtomb
#  endif
_GL_FUNCDECL_RPL (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
_GL_CXXALIAS_RPL (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
# else
#  if !@HAVE_WCRTOMB@
_GL_FUNCDECL_SYS (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
#  endif
_GL_CXXALIAS_SYS (wcrtomb, size_t, (char *s, wchar_t wc, mbstate_t *ps));
# endif
_GL_CXXALIASWARN (wcrtomb);
#elif defined GNULIB_POSIXCHECK
# undef wcrtomb
# if HAVE_RAW_DECL_WCRTOMB
_GL_WARN_ON_USE (wcrtomb, "wcrtomb is unportable - "
                 "use gnulib module wcrtomb for portability");
# endif
#endif


/* Convert a wide string to a string.  */
#if @GNULIB_WCSRTOMBS@
# if @REPLACE_WCSRTOMBS@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcsrtombs
#   define wcsrtombs rpl_wcsrtombs
#  endif
_GL_FUNCDECL_RPL (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len,
                   mbstate_t *ps));
# else
#  if !@HAVE_WCSRTOMBS@
_GL_FUNCDECL_SYS (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len, mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (wcsrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (wcsrtombs);
#elif defined GNULIB_POSIXCHECK
# undef wcsrtombs
# if HAVE_RAW_DECL_WCSRTOMBS
_GL_WARN_ON_USE (wcsrtombs, "wcsrtombs is unportable - "
                 "use gnulib module wcsrtombs for portability");
# endif
#endif


/* Convert a wide string to a string.  */
#if @GNULIB_WCSNRTOMBS@
# if @REPLACE_WCSNRTOMBS@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcsnrtombs
#   define wcsnrtombs rpl_wcsnrtombs
#  endif
_GL_FUNCDECL_RPL (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
_GL_CXXALIAS_RPL (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# else
#  if !@HAVE_WCSNRTOMBS@
_GL_FUNCDECL_SYS (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps)
                  _GL_ARG_NONNULL ((2)));
#  endif
_GL_CXXALIAS_SYS (wcsnrtombs, size_t,
                  (char *dest, const wchar_t **srcp, size_t srclen, size_t len,
                   mbstate_t *ps));
# endif
_GL_CXXALIASWARN (wcsnrtombs);
#elif defined GNULIB_POSIXCHECK
# undef wcsnrtombs
# if HAVE_RAW_DECL_WCSNRTOMBS
_GL_WARN_ON_USE (wcsnrtombs, "wcsnrtombs is unportable - "
                 "use gnulib module wcsnrtombs for portability");
# endif
#endif


/* Return the number of screen columns needed for WC.  */
#if @GNULIB_WCWIDTH@
# if @REPLACE_WCWIDTH@
#  if !(defined __cplusplus && defined GNULIB_NAMESPACE)
#   undef wcwidth
#   define wcwidth rpl_wcwidth
#  endif
_GL_FUNCDECL_RPL (wcwidth, int, (wchar_t));
_GL_CXXALIAS_RPL (wcwidth, int, (wchar_t));
# else
#  if !@HAVE_DECL_WCWIDTH@
/* wcwidth exists but is not declared.  */
_GL_FUNCDECL_SYS (wcwidth, int, (wchar_t));
#  endif
_GL_CXXALIAS_SYS (wcwidth, int, (wchar_t));
# endif
_GL_CXXALIASWARN (wcwidth);
#elif defined GNULIB_POSIXCHECK
# undef wcwidth
# if HAVE_RAW_DECL_WCWIDTH
_GL_WARN_ON_USE (wcwidth, "wcwidth is unportable - "
                 "use gnulib module wcwidth for portability");
# endif
#endif


#endif /* _GL_WCHAR_H */
#endif /* _GL_WCHAR_H */
#endif
