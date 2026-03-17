/*
 * This source file is part of the Atlantis Little Helper program.
 * Copyright (C) 2001 Maxim Shariy.
 *
 * Atlantis Little Helper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atlantis Little Helper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atlantis Little Helper; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if !defined(__COMPAT_H_INCL__)
#define __COMPAT_H_INCL__

/**
 * @file compat.h
 * @brief Cross-platform compatibility header
 * 
 * Provides function name mappings for non-MSVC compilers to ensure
 * consistent API across different platforms. Maps Microsoft-specific
 * function names to their POSIX equivalents.
 */

#if !defined(_MSC_VER)

    /**
     * @def strnicmp
     * @brief Maps to POSIX strncasecmp for non-MSVC compilers
     * 
     * Case-insensitive string comparison with length limit.
     * Microsoft uses strnicmp, POSIX uses strncasecmp.
     */
    #define strnicmp   strncasecmp
    
    /**
     * @def stricmp
     * @brief Maps to POSIX strcasecmp for non-MSVC compilers
     * 
     * Case-insensitive string comparison.
     * Microsoft uses stricmp, POSIX uses strcasecmp.
     */
    #define stricmp    strcasecmp
    
    /**
     * @def _vsnprintf
     * @brief Maps to POSIX vsnprintf for non-MSVC compilers
     * 
     * Variable argument formatted string output with buffer size limit.
     * Microsoft uses _vsnprintf, POSIX uses vsnprintf.
     */
    #define _vsnprintf vsnprintf

#endif

#endif