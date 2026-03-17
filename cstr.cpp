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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "stdafx.h"
#include "cstr.h"
#include "compat.h"

#define MIN_DELTA   8  // Minimum growth increment for string buffer

//--------------------------------------------------------------------------
// Platform-specific string functions for non-MSVC compilers
//--------------------------------------------------------------------------

#if !defined(_MSC_VER)

/**
 * Converts string to uppercase in-place (POSIX version)
 */
char* _strupr(char* p)
{
    char* ret = p;

    while (*p)
    {
        *p = toupper(*p);
        p++;
    }
    return ret;
}

/**
 * Converts string to lowercase in-place (POSIX version)
 */
char* _strlwr(char* p)
{
    char* ret = p;

    while (*p)
    {
        *p = tolower(*p);
        p++;
    }
    return ret;
}

#endif

//--------------------------------------------------------------------------
// Case-insensitive substring search
// Returns pointer to first occurrence, or NULL if not found
//--------------------------------------------------------------------------

const char* stristr(const char* string, const char* image)
{
    char   first[3];
    int    imagelen;

    if ((NULL == image) || (0 == *image))
        return string;

    imagelen = static_cast<int>(strlen(image));
    
    // Prepare both cases of first character for quick search
    first[0] = tolower(image[0]);
    first[1] = toupper(image[0]);
    first[2] = 0;

    while (string)
    {
        // Find any occurrence of first character (case-insensitive)
        string = strpbrk(string, first);
        if (string)
        {
            // Check if rest of string matches
            if (0 == strnicmp(string, image, imagelen))
                return string;
            else
                string++;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------
// Safe string comparison that handles NULL pointers
// Returns: 0 if equal, -1 if s1 < s2, 1 if s1 > s2
//--------------------------------------------------------------------------

int SafeCmp(const char* s1, const char* s2)
{
    if (NULL == s1)
        if (NULL == s2)
            return 0;
        else
            return -1;
    else
        if (NULL == s2)
            return 1;
        else
            return stricmp(s1, s2);
}

//--------------------------------------------------------------------------
// Safe string comparison ignoring spaces
//--------------------------------------------------------------------------

int SafeCmpNoSpaces(const char* s1, const char* s2)
{
    if (NULL == s1)
        if (NULL == s2)
            return 0;
        else
            return -1;
    else
        if (NULL == s2)
            return 1;
        else
        {
            // Skip spaces in both strings while comparing
            while (*s1 && *s2)
            {
                while (*s1 && *s1 <= ' ')
                    s1++;
                while (*s2 && *s2 <= ' ')
                    s2++;
                if (*s1 < *s2)
                    return -1;
                else if (*s1 > *s2)
                    return 1;
                s1++;
                s2++;
            }
            return 0;
        }
}

//--------------------------------------------------------------------------
// Skip whitespace characters in string
// Returns pointer to first non-whitespace character
//--------------------------------------------------------------------------

const char* SkipSpaces(const char* p)
{
    while (p && *p && (*p <= ' '))
        p++;
    return (char*)p;
}

//==========================================================================
// CStr Implementation - Dynamic String Class
//==========================================================================

/**
 * Default constructor
 */
CStr::CStr()
{
    m_pData = NULL;
    m_nDelta = MIN_DELTA * 2;  // Default growth increment: 16
    m_nStrLen = 0;
    m_nDataSize = 0;
}

/**
 * Constructor from C-style string
 */
CStr::CStr(const char* src)
{
    m_pData = NULL;
    m_nDelta = MIN_DELTA * 2;
    m_nStrLen = 0;
    m_nDataSize = 0;

    if (src && *src)
        AddStr(src);
}

/**
 * Constructor with custom growth increment
 */
CStr::CStr(short int nDelta)
{
    m_pData = NULL;
    m_nDelta = nDelta;
    m_nStrLen = 0;
    m_nDataSize = 0;

    if (m_nDelta < MIN_DELTA)
        m_nDelta = MIN_DELTA;
}

/**
 * Copy constructor - deep copy
 */
CStr::CStr(const CStr& src)
{
    m_pData = NULL;
    m_nDelta = src.m_nDelta;
    m_nStrLen = 0;
    m_nDataSize = 0;

    // Safe copy of source data
    if (src.m_pData && src.m_nStrLen > 0)
        AddBuf(src.m_pData, src.m_nStrLen);
}

//--------------------------------------------------------------------------

/**
 * Destructor - frees allocated memory
 */
CStr::~CStr()
{
    if (m_pData)
    {
        free(m_pData);
        m_pData = NULL;
    }
}

//--------------------------------------------------------------------------

/**
 * Reallocates internal buffer to new size
 * Uses m_nDelta increments for growth
 */
void CStr::ReAllocate(int nNewSize)
{
    if (nNewSize < 0) nNewSize = 0;

    int nTheSize = 0;
    if (nNewSize > 0)
    {
        // Round up to next multiple of delta
        nTheSize = ((nNewSize - 1) / m_nDelta + 1) * m_nDelta;
    }

    if (nTheSize != m_nDataSize)
    {
        if (nTheSize == 0)
        {
            // Free memory completely
            if (m_pData)
            {
                free(m_pData);
                m_pData = NULL;
            }
            m_nDataSize = 0;
            m_nStrLen = 0;
        }
        else
        {
            // Reallocate to new size
            char* newData = (char*)realloc(m_pData, nTheSize);
            if (newData)
            {
                m_pData = newData;
                m_nDataSize = nTheSize;
                // Ensure we don't exceed new buffer size
                if (m_nStrLen >= nTheSize)
                {
                    m_nStrLen = nTheSize - 1; // Leave room for null terminator
                    if (m_nStrLen < 0) m_nStrLen = 0;
                }
            }
            else
            {
                // Memory allocation failed - keep old buffer
                return;
            }
        }
    }
}

/**
 * Adds a single character to the end of the string
 */
void CStr::AddCh(char ch)
{
    if (m_nStrLen + 1 > m_nDataSize)
        ReAllocate(m_nStrLen + 1);

    if (m_pData)
    {
        m_pData[m_nStrLen] = ch;
        m_nStrLen++;
    }
}

//--------------------------------------------------------------------------

/**
 * Deletes character at specified position
 */
void CStr::DelCh(int nPos)
{
    if ((nPos < 0) || (nPos >= m_nStrLen) || !m_pData)
        return;

    // Shift remaining characters left
    memmove(&m_pData[nPos], &m_pData[nPos + 1], m_nStrLen - (nPos + 1));
    m_nStrLen--;
}

//--------------------------------------------------------------------------

/**
 * Sets character at specified position
 */
void CStr::SetCh(int nPos, char ch)
{
    if ((nPos < 0) || (nPos >= m_nStrLen) || !m_pData)
        return;
    m_pData[nPos] = ch;
}

//--------------------------------------------------------------------------

/**
 * Adds formatted long integer to the end
 */
void CStr::AddLong(long lNum)
{
    char* p;
    ReAllocate(m_nStrLen + 32); // 32 chars is enough for any long
    if (!m_pData) return;
    p = &(m_pData[m_nStrLen]);
    sprintf(p, "%ld", lNum);
    m_nStrLen += static_cast<int>(strlen(p));
}

//--------------------------------------------------------------------------

/**
 * Adds formatted unsigned long integer to the end
 */
void CStr::AddULong(unsigned long ulNum)
{
    char* p;
    ReAllocate(m_nStrLen + 32);
    if (!m_pData) return;
    p = &(m_pData[m_nStrLen]);
    sprintf(p, "%lu", ulNum);
    m_nStrLen += static_cast<int>(strlen(p));
}

//--------------------------------------------------------------------------

/**
 * Adds formatted double to the end with specified width and precision
 */
void CStr::AddDouble(double dNum, int width, int precision)
{
    char mask[64];
    sprintf(mask, "%s%d.%df", "%", width, precision);

    // Ensure buffer is large enough
    if (width < 312 + precision)
        width = 312 + precision;
        
    char* p;
    ReAllocate(m_nStrLen + width);
    if (!m_pData) return;
    p = &(m_pData[m_nStrLen]);
    sprintf(p, mask, dNum);
    m_nStrLen += static_cast<int>(strlen(p));
}

//--------------------------------------------------------------------------

/**
 * Adds binary data buffer to the end
 */
void CStr::AddBuf(const void* szData, int iDataLen)
{
    if (NULL == szData || iDataLen <= 0)
        return;

    if (m_nStrLen + iDataLen > m_nDataSize)
        ReAllocate(m_nStrLen + iDataLen);

    if (m_pData)
    {
        memmove(&m_pData[m_nStrLen], szData, iDataLen);
        m_nStrLen += iDataLen;
    }
}

//--------------------------------------------------------------------------

/**
 * Inserts binary data buffer at specified position
 */
void CStr::InsBuf(const void* szData, int nPos, int iDataLen)
{
    if (NULL == szData || iDataLen <= 0)
        return;

    if (nPos > m_nStrLen)
        nPos = m_nStrLen;

    if (m_nStrLen + iDataLen > m_nDataSize)
        ReAllocate(m_nStrLen + iDataLen);

    if (!m_pData)
        return;

    // Shift existing data right to make room
    if (nPos < m_nStrLen)
        memmove(&m_pData[nPos + iDataLen], &m_pData[nPos], m_nStrLen - nPos);
    memmove(&m_pData[nPos], szData, iDataLen);
    m_nStrLen += iDataLen;
}

//--------------------------------------------------------------------------

/**
 * Adds string to the end
 */
void CStr::AddStr(const char* szS, int iSLen)
{
    if (NULL == szS)
        return;

    if (iSLen == 0)
        iSLen = static_cast<int>(strlen(szS));

    if (iSLen <= 0)
        return;

    AddBuf(szS, iSLen);
}

//--------------------------------------------------------------------------

/**
 * Sets string to new content
 */
void CStr::SetStr(const char* szS, int iSLen)
{
    Empty();
    AddStr(szS, iSLen);
}

//--------------------------------------------------------------------------

/**
 * Inserts string at specified position
 */
void CStr::InsStr(const char* szS, int nPos, int iSLen)
{
    if ((NULL == szS) || (0 == szS[0]))
        return;

    if (0 == iSLen)
        iSLen = static_cast<int>(strlen(szS));

    InsBuf(szS, nPos, iSLen);
}

//--------------------------------------------------------------------------

/**
 * Returns null-terminated string data
 * Temporarily adds terminator for this call
 */
const char* CStr::GetData()
{
    AddCh(0);
    m_nStrLen--;  // Remove the added null from length

    return m_pData ? m_pData : "";
}

/**
 * Returns safe C string (never NULL)
 * Ensures null termination without modifying object
 */
const char* CStr::GetSafeCStr() const
{
    if (!m_pData || m_nStrLen == 0)
        return "";

    if (m_nStrLen < m_nDataSize)
    {
        // We have room to add terminator
        const_cast<CStr*>(this)->m_pData[m_nStrLen] = 0;
        return m_pData;
    }

    // Need temporary buffer
    static thread_local CStr tempBuffer;
    tempBuffer.Empty();
    tempBuffer.AddBuf(m_pData, m_nStrLen);
    return tempBuffer.GetData();
}

/**
 * Empties the string
 * Keeps memory if not too large, otherwise frees
 */
void CStr::Empty()
{
    if (m_nDataSize > m_nDelta * 2)
    {
        ReAllocate(0);  // Free memory if too large
    }
    else if (m_pData)
    {
        m_pData[0] = 0;
        m_nStrLen = 0;
    }
}

//--------------------------------------------------------------------------

/**
 * Allocates extra buffer at the end for external writer
 * Returns pointer to buffer
 */
char* CStr::AllocExtraBuf(int size)
{
    if (m_nStrLen + size > m_nDataSize)
        ReAllocate(m_nStrLen + size);

    return m_pData ? &m_pData[m_nStrLen] : NULL;
}

//--------------------------------------------------------------------------

/**
 * Updates string length after using AllocExtraBuf
 */
void CStr::UseExtraBuf(int size)
{
    if (m_nStrLen + size > m_nDataSize)
        ReAllocate(m_nStrLen + size);

    m_nStrLen += size;
}

//--------------------------------------------------------------------------

/**
 * Converts string to uppercase in-place
 */
const char* CStr::ToUpper()
{
    GetData();
    if (!m_pData) return "";
    return _strupr(m_pData);
}

//--------------------------------------------------------------------------

/**
 * Converts string to lowercase in-place
 */
const char* CStr::ToLower()
{
    GetData();
    if (!m_pData) return "";
    return _strlwr(m_pData);
}

//--------------------------------------------------------------------------

/**
 * Extracts integer token from source string
 * Returns pointer to next character after the integer
 */
char* CStr::GetInteger(const char* Src, BOOL& Valid)
{
    Empty();

    if (!Src)
    {
        Valid = FALSE;
        return NULL;
    }

    // Read digits, allowing leading minus sign
    while (Src && *Src)
    {
        if ((*Src >= '0') && (*Src <= '9'))
            AddCh(*Src);
        else if (('-' == *Src) && (0 == GetLength()))
            AddCh(*Src);
        else
            break;
        Src++;
    }

    // Valid if we have at least one digit (or minus sign not alone)
    Valid = m_nStrLen > 1 || (m_nStrLen == 1 && *m_pData != '-');

    return (char*)Src;
}

//--------------------------------------------------------------------------

/**
 * Extracts double token from source string (format: 123.45)
 */
char* CStr::GetDouble(const char* Src, BOOL& Valid)
{
    Empty();

    if (!Src)
    {
        Valid = FALSE;
        return NULL;
    }

    // Read digits and decimal point, allowing leading minus
    while (Src && *Src)
    {
        if ((*Src >= '0') && (*Src <= '9') || (*Src == '.'))
            AddCh(*Src);
        else if (('-' == *Src) && (0 == GetLength()))
            AddCh(*Src);
        else
            break;
        Src++;
    }

    // Valid if we have at least one digit (not just '-' or '.')
    Valid = m_nStrLen > 1 || (m_nStrLen == 1 && *m_pData != '-' && *m_pData != '.');

    return (char*)Src;
}

//--------------------------------------------------------------------------

/**
 * Checks if string represents a valid integer
 */
BOOL CStr::IsInteger()
{
    if (!m_pData || m_nStrLen == 0)
        return FALSE;

    BOOL   Ok = FALSE;
    int    pos = 0;
    const char* p = GetData();

    while (*p)
    {
        if ((*p >= '0' && *p <= '9') || (0 == pos && '-' == *p))
            Ok = TRUE;
        else
            return FALSE;
        p++;
        pos++;
    }
    return Ok;
}

//--------------------------------------------------------------------------

/**
 * Extracts token delimited by single character
 * Optionally strips quotes and trims whitespace
 */
char* CStr::GetToken(const char* Src, char Limit, TrimMode Mode, BOOL StripQuotes)
{
    char* p;

    Empty();

    if (NULL == Src)
        return NULL;

    // Handle quoted strings
    if (StripQuotes && '\"' == *Src)
    {
        p = (char*)strchr(Src + 1, '\"');
        Src++;
    }
    else
        p = (char*)strchr(Src, Limit);

    if (NULL == p)
        AddStr(Src);
    else
    {
        if (p != Src)
            AddStr(Src, static_cast<int>(p - Src));
        p++;
    }

    TrimLeft(Mode);
    TrimRight(Mode);

    return p;
}

//--------------------------------------------------------------------------

/**
 * Extracts token delimited by any of multiple characters
 * Returns delimiter used via LimitUsed parameter
 */
char* CStr::GetToken(const char* Src, const char* Limit, char& LimitUsed, TrimMode Mode, BOOL StripQuotes)
{
    char* p;

    Empty();
    LimitUsed = 0;

    if (NULL == Src)
        return NULL;

    // Handle quoted strings
    if (StripQuotes && '\"' == *Src)
    {
        p = (char*)strchr(Src + 1, '\"');
        Src++;
    }
    else
        p = (char*)strpbrk(Src, Limit);

    if (NULL == p)
        AddStr(Src);
    else
    {
        if (p != Src)
            AddStr(Src, static_cast<int>(p - Src));
        LimitUsed = *p;
        p++;
    }

    TrimLeft(Mode);
    TrimRight(Mode);

    return p;
}

//--------------------------------------------------------------------------

/**
 * Trims leading characters
 */
void CStr::TrimLeft(TrimMode Mode)
{
    if (TRIM_NONE == Mode || !m_pData || m_nStrLen == 0)
        return;

    char* p = m_pData;
    int    i = 0;

    if (TRIM_SPACES == Mode)
        while ((i < m_nStrLen) && ((' ' == *p) || ('\t' == *p)))
        {
            p++;
            i++;
        }
    else
        while ((i < m_nStrLen) && (*p <= ' '))
        {
            p++;
            i++;
        }
        
    if (i > 0)
    {
        m_nStrLen -= i;
        memmove(m_pData, p, m_nStrLen);
    }
}

//--------------------------------------------------------------------------

/**
 * Trims trailing characters
 */
void CStr::TrimRight(TrimMode Mode)
{
    if (TRIM_NONE == Mode || !m_pData || m_nStrLen == 0)
        return;
        
    if (TRIM_SPACES == Mode)
        while ((m_nStrLen > 0) && ((' ' == m_pData[m_nStrLen - 1]) || ('\t' == m_pData[m_nStrLen - 1])))
            m_nStrLen--;
    else
        while ((m_nStrLen > 0) && (m_pData[m_nStrLen - 1] <= ' '))
            m_nStrLen--;
}

//--------------------------------------------------------------------------

/**
 * Formats string using printf-style format (va_list version)
 */
void CStr::Format(const char* lpszFormat, va_list argList)
{
    int     nMaxLen = 0x0100;  // Start with 256 bytes
    va_list argListSave;
    int     err;

#if defined(_MSC_VER)
    argListSave = argList;
#else
    va_copy(argListSave, argList);
#endif

    Empty();

    // Try increasing buffer sizes until successful
    while (nMaxLen < 0x80000)  // Don't exceed half megabyte
    {
        ReAllocate(nMaxLen);
        if (!m_pData) break;

        err = _vsnprintf(m_pData, nMaxLen, lpszFormat, argListSave);

        if (err >= 0 && err < nMaxLen)
        {
            m_nStrLen = err;
            break;
        }
        else if (err < 0)
        {
            // Old _vsnprintf returns -1 on overflow
            nMaxLen <<= 1;    // Double buffer size
        }
        else
        {
            // glibc returns required size
            nMaxLen = err + 1;
        }
    }

#if !defined(_MSC_VER)
    va_end(argListSave);
#endif
}

//--------------------------------------------------------------------------

/**
 * Formats string using printf-style format (variable arguments)
 */
void CStr::Format(const char* lpszFormat, ...)
{
    va_list argList;

    va_start(argList, lpszFormat);
    Format(lpszFormat, argList);
    va_end(argList);
}

//--------------------------------------------------------------------------

/**
 * Finds first occurrence of substring (case-insensitive)
 */
int CStr::FindSubStr(const char* szS)
{
    if (!szS || !m_pData || m_nStrLen == 0)
        return -1;

    const char* pFirst = stristr(GetData(), szS);

    if (pFirst)
        return (static_cast<int>(pFirst - m_pData));

    return -1;
}

//--------------------------------------------------------------------------

/**
 * Finds last occurrence of substring (case-insensitive)
 */
int CStr::FindSubStrR(const char* szS)
{
    if (!szS || !m_pData || m_nStrLen == 0)
        return -1;

    const char* p1 = NULL;
    const char* p2 = GetData();
    int    n;

    n = static_cast<int>(strlen(szS));
    p2 = stristr(p2, szS);
    
    // Find last occurrence by scanning repeatedly
    while (p2)
    {
        p1 = p2;
        p2 += n;
        if (p2 - m_pData >= GetLength())
            break;
        p2 = stristr(p2, szS);
    }

    if (p1)
        return static_cast<int>(p1 - m_pData);

    return -1;
}

//--------------------------------------------------------------------------

/**
 * Deletes substring at specified position
 */
void CStr::DelSubStr(int nPos, int nCount)
{
    if ((nPos < 0) || (nCount <= 0) || ((nPos + 1) > GetLength()) || !m_pData)
        return;

    if ((nPos + nCount) > GetLength())
        nCount = GetLength() - nPos;

    // Shift remaining characters left
    memmove(&m_pData[nPos], &m_pData[nPos + nCount], m_nStrLen - (nPos + nCount));
    m_nStrLen = m_nStrLen - nCount;
}

//--------------------------------------------------------------------------

/**
 * Normalizes string by removing extra spaces
 * Used for string comparison
 */
void CStr::Normalize()
{
    if (!m_pData || m_nStrLen == 0)
        return;

    int i;

    TrimRight(TRIM_ALL);
    TrimLeft(TRIM_ALL);

    // Replace multiple whitespace with single spaces
    for (i = m_nStrLen - 1; i > 0; i--)
        if (m_pData[i] <= ' ')
            if (m_pData[i - 1] <= ' ')
                DelCh(i);  // Remove duplicate whitespace
            else
                m_pData[i] = ' ';  // Convert to space
}

//--------------------------------------------------------------------------

/**
 * Removes line breaks (CR and LF) from string
 */
void CStr::RemoveLineBreaks()
{
    if (!m_pData || m_nStrLen == 0)
        return;

    int i;

    for (i = m_nStrLen - 1; i >= 0; i--)
        if ('\r' == m_pData[i] || '\n' == m_pData[i])
            DelCh(i);
}

//--------------------------------------------------------------------------

/**
 * Replaces all occurrences of a character
 */
void CStr::Replace(char search, char replace_with)
{
    if (!m_pData || m_nStrLen == 0)
        return;

    int i;

    for (i = 0; i < m_nStrLen; i++)
        if (search == m_pData[i])
            m_pData[i] = replace_with;
}

//--------------------------------------------------------------------------