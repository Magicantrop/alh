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

#ifndef _INCL_COLL_H_
#define _INCL_COLL_H_

#include "bool.h"
#include "compat.h"
#include "stdafx.h"

/**
 * @class CCollection
 * @brief Base dynamic array collection class
 * 
 * Provides a resizable array that stores void pointers. The collection
 * automatically grows when needed using a delta increment strategy.
 * This is an abstract class that requires derived classes to implement
 * FreeItem() for proper memory management.
 */
class CCollection
{
public:
    /**
     * @brief Constructor with default delta (10)
     */
    CCollection();
    
    /**
     * @brief Constructor with custom delta
     * @param nDelta Growth increment when array needs resizing
     */
    CCollection(int nDelta);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~CCollection();

    /**
     * @brief Gets item at specified index
     * @param nIndex Index (0-based)
     * @return Pointer to item, or NULL if index out of range
     */
    void * At(int nIndex) const;
    
    /**
     * @brief Inserts item at the end of collection
     * @param pItem Pointer to item to insert
     * @return TRUE if successful, FALSE on memory allocation failure
     */
    virtual BOOL Insert(void * pItem);
    
    // Note: Delete and Free methods are intentionally removed because they are slow
    // Use AtDelete/AtFree with index instead
    
    /**
     * @brief Inserts item at specific position
     * @param nIndex Insertion position (0-based)
     * @param pItem Pointer to item to insert
     */
    void AtInsert(int nIndex, void * pItem);
    
    /**
     * @brief Sets item at specific position
     * @param nIndex Position (0-based)
     * @param pItem New item pointer
     * @param bFreeOld If TRUE, frees the old item using FreeItem()
     */
    void AtSet(int nIndex, void * pItem, BOOL bFreeOld);
    
    /**
     * @brief Deletes item at index without freeing it
     * @param nIndex Index to delete (0-based)
     */
    void AtDelete(int nIndex);
    
    /**
     * @brief Deletes and frees item at index
     * @param nIndex Index to delete (0-based)
     */
    void AtFree(int nIndex);
    
    /**
     * @brief Deletes all items without freeing them
     */
    void DeleteAll();
    
    /**
     * @brief Deletes and frees all items
     */
    void FreeAll();
    
    /**
     * @brief Gets current number of items
     * @return Item count
     */
    inline int  Count() const {return m_nCount;}
    
    /**
     * @brief Gets direct access to internal array
     * @return Pointer to array of void pointers
     */
    inline void ** GetItems() {return m_pItems;};

protected:
    /**
     * @brief Pure virtual method to free an item
     * @param pItem Pointer to item to free
     * 
     * Derived classes must implement this to properly free their items.
     */
    virtual void FreeItem(void * pItem)=0;
    
    /**
     * @brief Reallocates internal array when more space is needed
     */
    void Reallocate();

    void ** m_pItems;     /**< Dynamic array of item pointers */
    int     m_nCount;      /**< Current number of items */
    int     m_nSize;       /**< Current allocated size (in items) */
    int     m_nDelta;      /**< Growth increment when resizing */
};

//====================================================

/**
 * @class CLongColl
 * @brief Collection for storing long integers directly as pointers
 * 
 * Stores long values by casting them directly to void pointers.
 * This is efficient for small integers but careful with memory
 * deallocation - only frees values that are actually allocated.
 */
class CLongColl : public CCollection
{
public:
    CLongColl() : CCollection() {};
    CLongColl(int nDelta) : CCollection(nDelta) {};

protected:
    /**
     * @brief Frees a long value if it was dynamically allocated
     * @param pItem Pointer to free
     * 
     * Uses a heuristic to determine if the pointer is actually a
     * dynamically allocated value (based on address range).
     */
    virtual void FreeItem(void* pItem)
    {
        if (!pItem) return;
        uintptr_t ptrValue = reinterpret_cast<uintptr_t>(pItem);
        const uintptr_t MAX_SMALL_NUMBER = 0x00000000FFFFFFFFULL; // 4GB threshold
        if (ptrValue > MAX_SMALL_NUMBER)
        {
            free(pItem);
        }
    }
};

//====================================================

/**
 * @class CBufColl
 * @brief Collection for storing dynamically allocated buffers
 * 
 * Stores pointers to memory allocated with malloc(). Automatically
 * frees them using free() when items are removed.
 */
class CBufColl : public CCollection
{
public:
    CBufColl() : CCollection() {};
    CBufColl(int nDelta) : CCollection(nDelta) {};
    
protected:
    /**
     * @brief Frees a buffer using free()
     * @param pItem Pointer to buffer to free
     */
    virtual void FreeItem(void * pItem) { if (pItem) free(pItem); };
};

//====================================================

/**
 * @class CSortedCollection
 * @brief Base class for sorted collections
 * 
 * Extends CCollection to maintain items in sorted order.
 * Items are inserted at the correct position based on comparison.
 * Can optionally allow duplicates.
 */
class CSortedCollection : public CCollection
{
public:
    CSortedCollection();
    CSortedCollection(int nDelta);
    
    /**
     * @brief Inserts item in sorted position
     * @param pItem Pointer to item to insert
     * @return TRUE if successful
     */
    virtual BOOL Insert(void * pItem);
    
    /**
     * @brief Searches for an item in the collection
     * @param pItem Item to search for (used for comparison)
     * @param nIndex Output found index
     * @return TRUE if found, FALSE otherwise
     */
    BOOL Search(void * pItem, int & nIndex) const;
    
    // Note: FindFirst is removed since Search always finds first matching item when duplicates allowed

    BOOL m_bDuplicates;  /**< Whether duplicate items are allowed */

protected:
    /**
     * @brief Pure virtual comparison function
     * @param pItem1 First item
     * @param pItem2 Second item
     * @return Negative if pItem1 < pItem2, zero if equal, positive if greater
     */
    virtual int Compare(void * pItem1, void * pItem2) const = 0;
};

//====================================================

/**
 * @class CLongSortColl
 * @brief Sorted collection for long integers
 * 
 * Stores long values directly as pointers and maintains them
 * in sorted order. No memory management needed as values are
 * stored directly in the pointer.
 */
class CLongSortColl : public CSortedCollection
{
public:
    CLongSortColl() : CSortedCollection() {};
    CLongSortColl(int nDelta) : CSortedCollection(nDelta) {};
    
protected:
    /**
     * @brief No-op free since values are stored directly
     */
    virtual void FreeItem(void *) {};
    
    /**
     * @brief Compares two long values
     * @param pItem1 First value (cast from pointer)
     * @param pItem2 Second value (cast from pointer)
     * @return Comparison result (-1, 0, 1)
     */
    virtual int Compare(void * pItem1, void * pItem2) const
    {
        intptr_t val1 = reinterpret_cast<intptr_t>(pItem1);
        intptr_t val2 = reinterpret_cast<intptr_t>(pItem2);

        if (val1 > val2)
            return 1;
        else if (val1 < val2)
            return -1;
        else
            return 0;
    };
};

//====================================================

/**
 * @class CStringSortColl
 * @brief Sorted collection for C-style strings
 * 
 * Stores dynamically allocated strings and maintains them
 * in case-insensitive sorted order.
 */
class CStringSortColl : public CSortedCollection
{
public:
    CStringSortColl()           : CSortedCollection() {};
    CStringSortColl(int nDelta) : CSortedCollection(nDelta) {};
    
protected:
    /**
     * @brief Frees a string using free()
     * @param pItem Pointer to string to free
     */
    virtual void FreeItem(void * pItem) { if (pItem) free(pItem); };
    
    /**
     * @brief Case-insensitive string comparison
     * @param pItem1 First string
     * @param pItem2 Second string
     * @return Comparison result from stricmp
     */
    virtual int Compare(void * pItem1, void * pItem2) const {return stricmp((const char*)pItem1, (const char*)pItem2);}
};

//====================================================

/**
 * @class CResortableCollection
 * @brief Sorted collection that can change sort order dynamically
 * 
 * Extends CSortedCollection to allow changing the sort mode
 * and resorting the collection on demand. Derived classes must
 * implement Compare() to check m_SortMode.
 * 
 * @note Duplicates handling becomes complex with resorting -
 *       caller must manage this appropriately.
 */
class CResortableCollection : public CSortedCollection
{
public:
    CResortableCollection()           : CSortedCollection()       {m_SortMode=0;};
    CResortableCollection(int nDelta) : CSortedCollection(nDelta) {m_SortMode=0;};

    /**
     * @brief Changes sort mode and resorts collection
     * @param SortMode New sort mode identifier
     */
    void SetSortMode(int SortMode);

protected:
    /**
     * @brief Internal quicksort implementation
     * @param l Left bound
     * @param r Right bound
     */
    void DoSort(int l, int r);

    int m_SortMode;  /**< Current sort mode */
};

#endif