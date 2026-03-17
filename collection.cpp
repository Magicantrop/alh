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

#include <stdlib.h>
#include <string.h>

#include "stdafx.h"

#include "collection.h"

#if defined(_MSC_VER)
    #ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
    static char THIS_FILE[] = __FILE__;
    #endif
#endif

#define KEEP_MEM  // Keep memory allocated even after deletions (performance optimization)

//--------------------------------------------------------------------
// CCollection - Dynamic array base class
//--------------------------------------------------------------------

CCollection::CCollection()
{
    m_nCount = 0;   // No items initially
    m_nSize  = 0;   // No memory allocated
    m_nDelta = 8;   // Default growth increment
    m_pItems = NULL;
}

//--------------------------------------------------------------------

CCollection::CCollection(int nDelta)
{
    m_nCount = 0;
    m_nSize  = 0;
    m_nDelta = nDelta;
    m_pItems = NULL;

    // Validate and clamp delta to reasonable range
    if (m_nDelta <= 4)
        m_nDelta = 4;
    else if (m_nDelta > 0x0400)  // 1024
        m_nDelta = 0x0400;
}

//--------------------------------------------------------------------

CCollection::~CCollection()
{
    // Note: FreeAll() is not called here deliberately.
    // The application must call FreeAll() explicitly before destruction
    // if it wants to free the items. This destructor only frees the array itself.
    if (m_pItems)
        free(m_pItems);
}

//--------------------------------------------------------------------

/**
 * Returns item at specified index (0-based)
 * Returns NULL if index out of range
 */
void * CCollection::At(int nIndex) const
{
    if ( (nIndex>=0) && (nIndex<m_nCount) )
        return m_pItems[nIndex];
    else
        return NULL;
}

//--------------------------------------------------------------------

/**
 * Inserts item at the end of the collection
 */
BOOL CCollection::Insert(void * pItem)
{
    AtInsert(m_nCount, pItem);
    return TRUE;
}

//--------------------------------------------------------------------

/**
 * Deletes item at index without freeing it
 * Shifts all subsequent items left
 */
void CCollection::AtDelete(int nIndex)
{
    if ( (nIndex>=0) && (nIndex<m_nCount) )
    {
        // Shift remaining items left
        if (nIndex < m_nCount-1)
            memmove(&m_pItems[nIndex], &m_pItems[nIndex+1], sizeof(void*)*(m_nCount-1-nIndex));
        m_nCount--;
        
#ifndef KEEP_MEM
        // Optionally shrink memory if we've grown too much
        Reallocate();
#endif
    }
}

//--------------------------------------------------------------------

/**
 * Reallocates the internal array to fit current item count
 * Expands when full, optionally shrinks when too empty
 */
void CCollection::Reallocate() 
{
    int nNewSize;
    
    // Check if we need to expand or (optionally) shrink
    if (m_nCount >= m_nSize || (m_nSize - m_nCount) > 2 * m_nDelta) 
    {
        // Calculate new size: round up to next multiple of delta
        nNewSize = (m_nCount / m_nDelta + 1) * m_nDelta;
        void ** pNewItems = (void**)malloc(nNewSize * sizeof(void*));

        if (!pNewItems) 
        {
            // TODO: handle memory allocation error
            return;
        }

        // Copy existing items to new array
        if (m_nCount > 0)
            memcpy(pNewItems, m_pItems, sizeof(void*) * m_nCount);

        // Free old array
        if (m_pItems)
            free(m_pItems);

        m_pItems = (void**)pNewItems;
        m_nSize  = nNewSize;
    }
}

//--------------------------------------------------------------------

/**
 * Deletes item at index and frees it using FreeItem()
 */
void CCollection::AtFree(int nIndex)
{
    void * pItem;

    pItem = At(nIndex);
    if (pItem)
        FreeItem(pItem);
    AtDelete(nIndex);
}

//--------------------------------------------------------------------

/**
 * Frees all items and clears the collection
 */
void CCollection::FreeAll()
{
    void * pItem;
    int    i;

    // Free each item
    for (i=0; i<m_nCount; i++)
    {
        pItem = At(i);
        if (pItem)
            FreeItem(pItem);
    }
    
    // Clear the array (without freeing memory if KEEP_MEM is defined)
    DeleteAll();
}

//--------------------------------------------------------------------

/**
 * Clears the collection without freeing items
 */
void CCollection::DeleteAll()
{
    m_nCount = 0;
    
#ifndef KEEP_MEM
    // Free the array if we're not keeping memory
    if (m_pItems)
    {
        free(m_pItems);
        m_pItems = NULL;
    }
#endif
}

//--------------------------------------------------------------------

/**
 * Inserts item at specified position, shifting subsequent items right
 */
void CCollection::AtInsert(int nIndex, void * pItem)
{
    // Validate index
    if (nIndex < 0 || nIndex > m_nCount)
    {
        // Deliberate crash to indicate programming error
        int xx = 2;
        xx = 2/(xx-2);  // Division by zero
    }
    
    // Ensure we have space
    Reallocate();

    // Shift items right to make room
    if (nIndex < m_nCount)
        memmove(&m_pItems[nIndex+1], &m_pItems[nIndex], sizeof(void*)*(m_nCount-nIndex));
    
    m_pItems[nIndex] = pItem;
    m_nCount++;
}

//--------------------------------------------------------------------

/**
 * Sets item at specified position
 * If index equals count, appends; otherwise replaces existing item
 */
void CCollection::AtSet(int nIndex, void * pItem, BOOL bFreeOld)
{
    // Validate index
    if (nIndex < 0 || nIndex > m_nCount)
    {
        // Deliberate crash to indicate programming error
        int xx = 2;
        xx = 2/(xx-2);  // Division by zero
    }

    // Ensure we have space
    Reallocate();

    if (nIndex == m_nCount)
    {
        // Appending
        m_nCount++;
    }
    else
    {
        // Replacing existing item
        if (bFreeOld)
            FreeItem(m_pItems[nIndex]);
    }

    m_pItems[nIndex] = pItem;
}

//============================================================================
// CSortedCollection - Sorted collection with binary search
//============================================================================

CSortedCollection::CSortedCollection() : CCollection()
{
    m_bDuplicates = FALSE;  // Default: no duplicates allowed
}

CSortedCollection::CSortedCollection(int nDelta) : CCollection(nDelta)
{
    m_bDuplicates = FALSE;
}

//--------------------------------------------------------------------

/**
 * Inserts item in sorted order
 * Uses binary search to find insertion position
 * Returns FALSE if item already exists and duplicates not allowed
 */
BOOL CSortedCollection::Insert(void * pItem)
{
    int I;

    // Search for insertion position
    if ( (!Search(pItem, I)) || m_bDuplicates)
    {
        AtInsert(I, pItem);
        return TRUE;
    }
    else
        return FALSE;  // Duplicate and duplicates not allowed
}

//--------------------------------------------------------------------

/**
 * Binary search for an item
 * Returns TRUE if found, and nIndex points to the item or insertion position
 */
BOOL CSortedCollection::Search(void * pItem, int & nIndex) const
{
    int   L, H, I, C;
    BOOL  Ok;

    Ok = FALSE;
    L  = 0;
    H  = Count() - 1;
    
    // Binary search
    while (L <= H)
    {
        I = (L + H) >> 1;  // Midpoint
        C = Compare(m_pItems[I], pItem);
        
        if (C < 0)
            L = I + 1;  // Search right half
        else
        {
            H = I - 1;  // Search left half
            if (C == 0)
            {
                Ok = TRUE;  // Found exact match
                if ( !m_bDuplicates)
                    L = I;   // For duplicates, we want first occurrence
            }
        }
    }
    
    nIndex = L;  // Insertion position (or found index)
    return Ok;
}

//--------------------------------------------------------------------
// CResortableCollection - Collection that can be resorted
//--------------------------------------------------------------------

/**
 * Changes sort mode and resorts the collection if needed
 */
void CResortableCollection::SetSortMode(int SortMode)
{
    if ( (SortMode != m_SortMode) && (Count()>0) )
    {
        m_SortMode = SortMode;
        DoSort(0, Count()-1);  // Quicksort the entire collection
    }
}

//--------------------------------------------------------------------

/**
 * Quicksort implementation for resorting
 * @param l Left bound (inclusive)
 * @param r Right bound (inclusive)
 */
void CResortableCollection::DoSort(int l, int r)
{
    int     i, j;
    void *  x, * y;

    i = l;
    j = r;
    x = m_pItems[(l+r) >> 1];  // Pivot element

    do
    {
        // Find elements that need swapping
        while (Compare(m_pItems[i], x) < 0)
            i++;
        while (Compare(x, m_pItems[j]) < 0)
            j--;
            
        if (i <= j)
        {
            // Swap elements
            if (i != j)
            {
                y           = m_pItems[i];
                m_pItems[i] = m_pItems[j];
                m_pItems[j] = y;
            }
            j--;
            i++;
        }
    }
    while (i <= j);

    // Recursively sort the partitions
    if (l < j)
        DoSort(l, j);
    if (i < r)
        DoSort(i, r);
}