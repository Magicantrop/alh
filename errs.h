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

#ifndef __AH__ERRS_H_INCLL__
#define __AH__ERRS_H_INCLL__

/**
 * @def ERR_DESIGN
 * @brief Error level for design/logic errors in code
 * 
 * Indicates an internal design or logic error that should never occur
 * in normal operation. These indicate bugs in the program itself.
 */
#define ERR_DESIGN   0

/**
 * @def ERR_PARSE
 * @brief Error level for parsing errors
 * 
 * Indicates errors that occur while parsing game reports or orders.
 * These are typically due to malformed input or unexpected formats.
 */
#define ERR_PARSE    1

/**
 * @def ERR_UNKNOWN
 * @brief Error level for unknown/unclassified errors
 * 
 * Catch-all for errors that don't fit into other categories.
 */
#define ERR_UNKNOWN  2

/**
 * @def LOG_ERR
 * @brief Macro for convenient error logging with file/line information
 * @param level Error level (ERR_* constants)
 * @param msg Error message string
 * 
 * Automatically adds the current file name and line number to the error log.
 * Example: LOG_ERR(ERR_PARSE, "Failed to parse unit");
 */
#define LOG_ERR(level, msg) LogError(__FILE__, __LINE__, level, msg);

/**
 * @brief Logs an error message with file and line information
 * @param fname Source file name where error occurred
 * @param lineno Line number where error occurred
 * @param level Error level (ERR_* constants)
 * @param msg Error message text
 * 
 * Writes error information to the application's error log for debugging
 * and troubleshooting purposes.
 */
void LogError(const char * fname, int lineno, int level, const char * msg);

#endif