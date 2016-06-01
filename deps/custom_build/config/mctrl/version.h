/*
 * Copyright (c) 2008-2014 Martin Mitas
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MC_VERSION_H
#define MC_VERSION_H


#define MC_VERSION_MAJOR              0
#define MC_VERSION_MINOR              11
#define MC_VERSION_RELEASE            1


/* Stringize given expression */
#define MC_STRINGIZE_HELPER(a)  #a
#define MC_STRINGIZE(a)         MC_STRINGIZE_HELPER(a)


#ifdef DEBUG
    #define MC_VERSION_STR     \
       MC_STRINGIZE(MC_VERSION_MAJOR.MC_VERSION_MINOR.MC_VERSION_RELEASE-dbg(DEBUG))
#else
    #define MC_VERSION_STR     \
       MC_STRINGIZE(MC_VERSION_MAJOR.MC_VERSION_MINOR.MC_VERSION_RELEASE)
#endif


#endif  /* MC_VERSION_H */
