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

#include "errs.h"
#include <stdio.h>
#include "stdafx.h"

/**
 * Logs an error message to stderr with file and line information
 * 
 * This function provides a simple error logging mechanism for debugging
 * purposes. It writes formatted error messages to the standard error stream.
 * 
 * @param fname Source file name where the error occurred
 * @param lineno Line number in the source file
 * @param level Error severity level (ERR_DESIGN, ERR_PARSE, or ERR_UNKNOWN)
 * @param msg Error message text
 * 
 * @note The level parameter is currently ignored but kept for API consistency
 *       and potential future filtering of error messages by severity.
 * @note In a GUI application, stderr output may be redirected or captured
 *       by the parent process or development environment.
 */
void LogError(const char * fname, int lineno, int level, const char * msg)
{
    // Write formatted error to stderr
    // Format: "Error, file <filename>, line <linenumber>: <message>\n"
    fprintf(stderr, "Error, file %s, line %d: %s\n", fname, lineno, msg);
}