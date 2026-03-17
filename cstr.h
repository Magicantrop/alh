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

#ifndef __CSTR_IS_A_SIMPLE_STRING__
#define __CSTR_IS_A_SIMPLE_STRING__

#include <stdarg.h>
#include <string.h>
#include "bool.h"

/**
 * @enum TrimMode
 * @brief Trimming modes for string operations
 */
enum TrimMode { 
    TRIM_NONE = 0,    /**< No trimming */
    TRIM_SPACES,      /**< Trim spaces only */
    TRIM_ALL          /**< Trim all whitespace characters */
};

/**
 * @class CStr
 * @brief Simple dynamic string class with automatic memory management
 * 
 * Provides a convenient and efficient string implementation with
 * automatic buffer resizing, formatting capabilities, and various
 * string manipulation methods. Designed for simplicity and performance.
 */
class CStr
{
public:
    /**
     * @brief Constructor with default delta (16)
     */
    CStr();
    
    /**
     * @brief Constructor with custom growth increment
     * @param nDelta Growth increment when buffer needs resizing
     */
    CStr(short int nDelta);
    
    /**
     * @brief Constructor from C-style string
     * @param src Source string (can be NULL)
     */
    CStr(const char* src);
    
    /**
     * @brief Copy constructor
     * @param src Source CStr object
     */
    CStr(const CStr& src);
    
    /**
     * @brief Destructor
     */
    ~CStr();

    //--------------------------------------------------------------------------
    // Basic String Operations
    //--------------------------------------------------------------------------

    /**
     * @brief Adds a single character to the end of the string
     * @param ch Character to add
     */
    void         AddCh(char ch);
    
    /**
     * @brief Deletes a character at specified position
     * @param nPos Position (0-based)
     */
    void         DelCh(int nPos);
    
    /**
     * @brief Sets character at specified position
     * @param nPos Position (0-based)
     * @param ch New character
     */
    void         SetCh(int nPos, char ch);
    
    /**
     * @brief Adds a string to the end
     * @param szS String to add
     * @param iSLen Length to add (0 = full string)
     */
    void         AddStr(const char* szS, int iSLen = 0);
    
    /**
     * @brief Replaces current string with new content
     * @param szS New string content
     * @param iSLen Length to set (0 = full string)
     */
    void         SetStr(const char* szS, int iSLen = 0);
    
    /**
     * @brief Inserts a string at specified position
     * @param szS String to insert
     * @param nPos Insertion position (0-based)
     * @param iSLen Length to insert (0 = full string)
     */
    void         InsStr(const char* szS, int nPos, int iSLen = 0);

    //--------------------------------------------------------------------------
    // Numeric Conversion
    //--------------------------------------------------------------------------

    /**
     * @brief Adds a formatted long integer
     * @param lNum Number to add
     */
    void         AddLong(long lNum);
    
    /**
     * @brief Adds a formatted unsigned long integer
     * @param ulNum Number to add
     */
    void         AddULong(unsigned long ulNum);
    
    /**
     * @brief Adds a formatted double
     * @param dNum Number to add
     * @param width Field width (0 for default)
     * @param precision Decimal precision
     */
    void         AddDouble(double dNum, int width, int precision);

    //--------------------------------------------------------------------------
    // Binary Data Operations
    //--------------------------------------------------------------------------

    /**
     * @brief Adds binary data to the string
     * @param szData Pointer to data
     * @param iDataLen Data length in bytes
     */
    void         AddBuf(const void* szData, int iDataLen);
    
    /**
     * @brief Inserts binary data at specified position
     * @param szData Pointer to data
     * @param nPos Insertion position (0-based)
     * @param iDataLen Data length in bytes
     */
    void         InsBuf(const void* szData, int nPos, int iDataLen);

    //--------------------------------------------------------------------------
    // Data Access
    //--------------------------------------------------------------------------

    /**
     * @brief Gets the string data with null terminator
     * @return Null-terminated string
     */
    const char* GetData();
    
    /**
     * @brief Gets the string data safely (never returns NULL)
     * @return String data or empty string if empty
     */
    const char* GetSafeCStr() const;

    /**
     * @brief Empties the string (sets length to 0)
     */
    void         Empty();
    
    /**
     * @brief Converts string to uppercase in-place
     * @return Pointer to modified string
     */
    const char* ToUpper();
    
    /**
     * @brief Converts string to lowercase in-place
     * @return Pointer to modified string
     */
    const char* ToLower();

    //--------------------------------------------------------------------------
    // Token Extraction
    //--------------------------------------------------------------------------

    /**
     * @brief Extracts token delimited by a single character
     * @param Src Source string to parse
     * @param Limit Delimiter character
     * @param Mode Trimming mode
     * @param StripQuotes Whether to strip quotes
     * @return Pointer to next position in source
     */
    char* GetToken(const char* Src, char Limit, TrimMode Mode = TRIM_SPACES, BOOL StripQuotes = TRUE);
    
    /**
     * @brief Extracts token delimited by any of multiple characters
     * @param Src Source string to parse
     * @param Limit Set of delimiter characters
     * @param LimitUsed Output actual delimiter used
     * @param Mode Trimming mode
     * @param StripQuotes Whether to strip quotes
     * @return Pointer to next position in source
     */
    char* GetToken(const char* Src, const char* Limit, char& LimitUsed, TrimMode Mode = TRIM_SPACES, BOOL StripQuotes = TRUE);
    
    /**
     * @brief Extracts integer token from source
     * @param Src Source string to parse
     * @param Valid Output validation flag
     * @return Pointer to next position in source
     */
    char* GetInteger(const char* Src, BOOL& Valid);
    
    /**
     * @brief Extracts double token from source (format: 123.45)
     * @param Src Source string to parse
     * @param Valid Output validation flag
     * @return Pointer to next position in source
     */
    char* GetDouble(const char* Src, BOOL& Valid);

    //--------------------------------------------------------------------------
    // String Manipulation
    //--------------------------------------------------------------------------

    /**
     * @brief Trims leading characters
     * @param Mode Trimming mode
     */
    void         TrimLeft(TrimMode Mode = TRIM_SPACES);
    
    /**
     * @brief Trims trailing characters
     * @param Mode Trimming mode
     */
    void         TrimRight(TrimMode Mode = TRIM_SPACES);
    
    /**
     * @brief Formats string using printf-style format
     * @param lpszFormat Format string
     * @param ... Format arguments
     */
    void         Format(const char* lpszFormat, ...);
    
    /**
     * @brief Formats string using va_list
     * @param lpszFormat Format string
     * @param argList Variable argument list
     */
    void         Format(const char* lpszFormat, va_list argList);
    
    /**
     * @brief Finds first occurrence of substring
     * @param szS Substring to find
     * @return Position (0-based) or -1 if not found
     */
    int          FindSubStr(const char* szS);
    
    /**
     * @brief Finds last occurrence of substring (reverse search)
     * @param szS Substring to find
     * @return Position (0-based) or -1 if not found
     */
    int          FindSubStrR(const char* szS);
    
    /**
     * @brief Deletes substring at specified position
     * @param nPos Start position
     * @param nCount Number of characters to delete
     */
    void         DelSubStr(int nPos, int nCount);
    
    /**
     * @brief Allocates extra buffer at the end for external writer
     * @param size Size to allocate
     * @return Pointer to allocated buffer
     */
    char* AllocExtraBuf(int size);
    
    /**
     * @brief Updates string length after AllocExtraBuf
     * @param size New total length
     */
    void         UseExtraBuf(int size);
    
    /**
     * @brief Normalizes string (removes redundant whitespace)
     */
    void         Normalize();
    
    /**
     * @brief Removes line breaks from string
     */
    void         RemoveLineBreaks();
    
    /**
     * @brief Replaces all occurrences of a character
     * @param search Character to search for
     * @param replace_with Replacement character
     */
    void         Replace(char search, char replace_with);
    
    /**
     * @brief Checks if string represents a valid integer
     * @return TRUE if string is a valid integer
     */
    BOOL         IsInteger();

    //--------------------------------------------------------------------------
    // Inline Accessors
    //--------------------------------------------------------------------------

    /**
     * @brief Checks if string is empty
     * @return TRUE if empty
     */
    inline BOOL  IsEmpty() const { return 0 == m_nStrLen; }
    
    /**
     * @brief Gets current string length
     * @return Length in characters
     */
    inline int   GetLength() const { return m_nStrLen; }
    
    /**
     * @brief Direct access to internal buffer (no terminator guarantee)
     * @return Pointer to internal buffer
     */
    inline const char* Data() const { return m_pData; }

    //--------------------------------------------------------------------------
    // Operator Overloads
    //--------------------------------------------------------------------------

    inline CStr& operator<<(const char* psz) { if (psz) AddStr(psz); return *this; }
    inline CStr& operator<<(char ch) { AddCh(ch); return *this; }
    inline CStr& operator<<(long lNum) { AddLong(lNum); return *this; }
    inline CStr& operator<<(unsigned long ulNum) { AddULong(ulNum); return *this; }
    inline CStr& operator<<(double dNum) { AddDouble(dNum, 0, 2); return *this; }

    inline CStr& operator=(const char* psz) { Empty(); if (psz) AddStr(psz); return *this; }
    inline CStr& operator=(const CStr& S) { Empty(); if (!S.IsEmpty()) AddBuf(S.Data(), S.GetLength()); return *this; }
    inline CStr& operator<<(const CStr& S) { if (!S.IsEmpty()) AddBuf(S.Data(), S.GetLength()); return *this; }

private:
    char* m_pData;           /**< Internal string buffer */
    short int m_nDelta;      /**< Growth increment when resizing */
    int       m_nStrLen;     /**< Current string length */
    int       m_nDataSize;    /**< Allocated buffer size */

    /**
     * @brief Reallocates internal buffer to new size
     * @param nNewSize New buffer size
     */
    void ReAllocate(int nNewSize);
};

//--------------------------------------------------------------------------
// Global Utility Functions
//--------------------------------------------------------------------------

/**
 * @brief Safe string comparison (handles NULL pointers)
 * @param s1 First string (can be NULL)
 * @param s2 Second string (can be NULL)
 * @return Comparison result (-1, 0, 1)
 */
int SafeCmp(const char* s1, const char* s2);

/**
 * @brief Safe string comparison ignoring spaces
 * @param s1 First string (can be NULL)
 * @param s2 Second string (can be NULL)
 * @return Comparison result (-1, 0, 1)
 */
int SafeCmpNoSpaces(const char* s1, const char* s2);

/**
 * @brief Skips whitespace characters in string
 * @param p Input pointer
 * @return Pointer to first non-whitespace character
 */
const char* SkipSpaces(const char* p);

#endif