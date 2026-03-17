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

#ifndef __CONFIG_FILE_H_INCL__
#define __CONFIG_FILE_H_INCL__

#include "collection.h"

/**
 * @struct _CONFIG_PARAM
 * @brief Structure representing a single configuration parameter
 * 
 * Contains all information about a configuration entry including its
 * section, name, value, and optional comment. String lengths are stored
 * separately for efficient access without recalculating.
 */
typedef struct _CONFIG_PARAM
{
public:
    const char * szSection;    /**< Section name (e.g., "[Options]") */
    const char * szName;        /**< Parameter name */
    const char * szValue;       /**< Parameter value */
    const char * szComment;     /**< Optional comment (NULL if none) */
    int          SectionLen;     /**< Length of section string (excluding null terminator) */
    int          NameLen;        /**< Length of name string (excluding null terminator) */
    int          ValueLen;       /**< Length of value string (excluding null terminator) */
    int          CommentLen;     /**< Length of comment string (excluding null terminator) */
} CONFIG_PARAM;

/**
 * @class CConfigFile
 * @brief Configuration file handler for INI-style format
 * 
 * This class manages reading, writing, and manipulating configuration files
 * in the standard INI format with sections, parameters, and comments.
 * It inherits from CSortedCollection to maintain sorted parameters.
 */
class CConfigFile : public CSortedCollection
{
public:
    CConfigFile();
    ~CConfigFile();

    /**
     * @brief Loads configuration from a file
     * @param szFName Path to configuration file
     * @return TRUE if file was loaded successfully, FALSE otherwise
     */
    BOOL Load(const char * szFName);
    
    /**
     * @brief Saves configuration to a file
     * @param szFName Path to output file
     * @return TRUE if file was saved successfully, FALSE otherwise
     */
    BOOL Save(const char * szFName);
    
    /**
     * @brief Retrieves a parameter value by section and name
     * @param szSection Section name
     * @param szName Parameter name
     * @return Parameter value string, or NULL if not found
     */
    const char * GetByName(const char * szSection, const char * szName);
    
    /**
     * @brief Sets or updates a parameter value
     * @param szSection Section name
     * @param szName Parameter name
     * @param szNewValue New value to set
     * @param szComment Optional comment (NULL to keep existing, "" to remove)
     */
    void         SetByName(const char * szSection, const char * szName, const char * szNewValue, const char * szComment=NULL);

    /**
     * @brief Gets the first parameter in a section
     * @param szSection Section name
     * @param szName Output parameter name
     * @param szValue Output parameter value
     * @return Index for subsequent calls to GetNextInSection, or -1 if section empty
     */
    int          GetFirstInSection(const char * szSection, const char *& szName, const char *& szValue);
    
    /**
     * @brief Gets the next parameter in a section
     * @param idx Previous index from GetFirstInSection/GetNextInSection
     * @param szSection Section name
     * @param szName Output parameter name
     * @param szValue Output parameter value
     * @return Next index or -1 if no more parameters
     */
    int          GetNextInSection (int idx, const char * szSection, const char *& szName, const char *& szValue);

    /**
     * @brief Removes an entire section and all its parameters
     * @param szSection Section name to remove
     */
    void         RemoveSection(const char * szSection);
    
    /**
     * @brief Gets the next section name in the file
     * @param szPrevSection Previous section name (NULL for first)
     * @param szNextSection Output next section name
     * @return TRUE if next section found, FALSE if no more sections
     */
    BOOL         GetNextSection(const char * szPrevSection, const char *& szNextSection);

protected:
    /**
     * @brief Frees a CONFIG_PARAM item from the collection
     * @param pItem Pointer to the item to free
     */
    virtual void FreeItem(void * pItem);
    
    /**
     * @brief Compares two CONFIG_PARAM items for sorting
     * @param pItem1 First item to compare
     * @param pItem2 Second item to compare
     * @return Comparison result (negative, zero, positive)
     */
    virtual int  Compare (void * pItem1, void * pItem2) const;
};

#endif