#include <config.h>

/* XSIZE_INLINE can be expanded to _GL_UNUSED, which is defined through
   a gnulib-tool magic.  Define it here so not to require Gnulib.  */
#if defined IN_LIBINTL && !defined GL_UNUSED
/* Define as a marker that can be attached to declarations that might not
    be used.  This helps to reduce warnings, such as from
    GCC -Wunused-parameter.  */
# if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#  define _GL_UNUSED __attribute__ ((__unused__))
# else
#  define _GL_UNUSED
# endif
#endif

#define XSIZE_INLINE _GL_EXTERN_INLINE
#include "xsize.h"
