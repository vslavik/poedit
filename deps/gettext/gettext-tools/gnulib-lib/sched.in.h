/* Replacement <sched.h> for platforms that lack it.
   Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.

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

#ifndef _GL_SCHED_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#if @HAVE_SCHED_H@
# @INCLUDE_NEXT@ @NEXT_SCHED_H@
#endif

#ifndef _GL_SCHED_H
#define _GL_SCHED_H

#if !@HAVE_STRUCT_SCHED_PARAM@

struct sched_param
{
  int sched_priority;
};

#endif

#if !(defined SCHED_FIFO && defined SCHED_RR && defined SCHED_OTHER)
# define SCHED_FIFO   1
# define SCHED_RR     2
# define SCHED_OTHER  0
#endif

#endif /* _GL_SCHED_H */
#endif /* _GL_SCHED_H */
