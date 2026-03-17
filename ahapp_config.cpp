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

#include "stdhdr.h"
#include "ahapp.h"
#include "unitfilterdlg.h"

//-------------------------------------------------------------------------
// Configuration Upgrade Methods
//-------------------------------------------------------------------------

/**
 * @brief Upgrades configuration files from older versions to current format
 * 
 * Handles migration of configuration data when new versions add or change
 * configuration sections. Performs:
 * - Moving sections between config files (main vs state)
 * - Updating column list configurations
 * - Migrating filter settings
 * - Renaming color keys
 * - Cleaning up obsolete entries
 */
void CAhApp::UpgradeConfigFiles()
{
    CStr         Section;
    const char * szNextSection;
    const char * szName;
    const char * szValue;
    BOOL         Ok = TRUE;
    CStr         ConfigKey;

    // Move sections between configuration files (main config vs state config)
    for (int currentConfigFile = 0; currentConfigFile < 2; ++currentConfigFile)
    {
        Ok = m_Config[currentConfigFile].GetNextSection("", szNextSection);
        while (Ok)
        {
            Section = szNextSection;
            const int fileno = GetConfigFileNo(Section.GetData());

            if (currentConfigFile != fileno)
            {
                // Move section to the appropriate file
                int idx = m_Config[currentConfigFile].GetFirstInSection(Section.GetData(), szName, szValue);
                while (idx >= 0)
                {
                    m_Config[fileno].SetByName(Section.GetData(), szName, szValue);
                    idx = m_Config[currentConfigFile].GetNextInSection(idx, Section.GetData(), szName, szValue);
                }
                m_Config[currentConfigFile].RemoveSection(Section.GetData());

                // Moving sections implies land flags might need upgrade
                m_UpgradeLandFlags = true;
            }
            Ok = m_Config[currentConfigFile].GetNextSection(Section.GetData(), szNextSection);
        }
    }

    // Upgrade unit list columns configuration
    szValue = m_Config[CONFIG_FILE_CONFIG].GetByName(SZ_SECT_LIST_COL_CURRENT, SZ_KEY_LIS_COL_UNITS_HEX);
    if (!szValue || !*szValue)
    {
        // Move old unit list headers to new column configuration sections
        MoveSectionEntries(CONFIG_FILE_CONFIG, SZ_SECT_UNITLIST_HDR     , SZ_SECT_LIST_COL_UNIT_DEF     );
        MoveSectionEntries(CONFIG_FILE_CONFIG, SZ_SECT_UNITLIST_HDR_FLTR, SZ_SECT_LIST_COL_UNIT_FLTR_DEF);

        SetConfig(SZ_SECT_LIST_COL_CURRENT  , SZ_KEY_LIS_COL_UNITS_HEX  ,     SZ_SECT_LIST_COL_UNIT_DEF);
        SetConfig(SZ_SECT_LIST_COL_CURRENT  , SZ_KEY_LIS_COL_UNITS_FILTER,    SZ_SECT_LIST_COL_UNIT_FLTR_DEF);
    }

    // Remove obsolete aliases
    m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_ALIAS, "MEN", NULL);
    m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_ALIAS, "MAN", NULL);

    // Migrate unit filter settings to named filter sets
    Section.Empty();
    Section  << SZ_SECT_UNIT_FILTER << "Default";
    Ok = FALSE;
    for (int i=0; i<UNIT_SIMPLE_FLTR_COUNT; i++)
    {
        ConfigKey.Format("%s%d", SZ_KEY_UNIT_FLTR_PROPERTY, i);
        szValue = SkipSpaces(gpApp->GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.GetData()));
        if (szValue && *szValue)
        {
            gpApp->SetConfig(Section.GetData(),      ConfigKey.GetData(), szValue);
            gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.GetData(), "");
            Ok = TRUE;
        }

        ConfigKey.Format("%s%d", SZ_KEY_UNIT_FLTR_COMPARE , i);
        szValue = gpApp->GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.GetData());
        if (szValue && *szValue)
        {
            gpApp->SetConfig(Section.GetData(),      ConfigKey.GetData(), szValue);
            gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.GetData(), "");
            Ok = TRUE;
        }

        ConfigKey.Format("%s%d", SZ_KEY_UNIT_FLTR_VALUE   , i);
        szValue = gpApp->GetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.GetData());
        if (szValue && *szValue)
        {
            gpApp->SetConfig(Section.GetData(),      ConfigKey.GetData(), szValue);
            gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, ConfigKey.GetData(), "");
            Ok = TRUE;
        }
    }
    if (Ok)
        gpApp->SetConfig(SZ_SECT_WND_UNITS_FLTR, SZ_KEY_FLTR_SET, Section.GetData());

    // Upgrade road color keys (Arcadia III)
    szValue = m_Config[CONFIG_FILE_CONFIG].GetByName(SZ_SECT_COLORS,  SZ_KEY_MAP_ROAD_OLD);
    if (szValue && *szValue)
    {
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD    , szValue);
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD_OLD, "");
    }
    szValue = m_Config[CONFIG_FILE_CONFIG].GetByName(SZ_SECT_COLORS,  SZ_KEY_MAP_ROAD_BAD_OLD);
    if (szValue && *szValue)
    {
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD_BAD    , szValue);
        m_Config[CONFIG_FILE_CONFIG].SetByName(SZ_SECT_COLORS, SZ_KEY_MAP_ROAD_BAD_OLD, "");
    }
}

//-------------------------------------------------------------------------

/**
 * @brief Moves all entries from one configuration section to another
 * @param fileno Configuration file index
 * @param src Source section name
 * @param dest Destination section name
 * 
 * Copies all key-value pairs from source section to destination,
 * then removes the source section.
 */
void CAhApp::MoveSectionEntries(int fileno, const char * src, const char * dest)
{
    const char * szName;
    const char * szValue;
    CBufColl     Names, Values;
    int          idx;

    // Collect all entries from source section
    idx = m_Config[fileno].GetFirstInSection(src, szName, szValue);
    while (idx>=0)
    {
        Names.Insert(strdup(szName));
        Values.Insert(strdup(szValue));
        idx = m_Config[fileno].GetNextInSection(idx, src, szName, szValue);
    }
    m_Config[fileno].RemoveSection(src);

    // Insert them into destination section
    for (idx=0; idx<Values.Count(); idx++)
    {
        szName = (const char *)Names.At(idx);
        szValue= (const char *)Values.At(idx);
        m_Config[fileno].SetByName(dest, szName?szName:"", szValue?szValue:"");
    }

    Names.FreeAll();
    Values.FreeAll();
}

//-------------------------------------------------------------------------
// Faction-Specific Configuration
//-------------------------------------------------------------------------

/**
 * @brief Upgrades configuration to use faction IDs as keys
 * 
 * Migrates order files and passwords from generic sections to
 * faction-specific sections when a current faction is set.
 */
void CAhApp::UpgradeConfigByFactionId()
{
    int          fileno, idx;
    CStr         S, Section, Key;
    const char * szName;
    const char * szValue;

    if (m_pAtlantis->m_CrntFactionId > 0)
    {
        // Upgrade order files to faction-specific section
        ComposeConfigOrdersSection(Section, m_pAtlantis->m_CrntFactionId);
        fileno  = GetConfigFileNo(SZ_SECT_ORDERS);
        idx     = m_Config[fileno].GetFirstInSection(SZ_SECT_ORDERS, szName, szValue);
        while (idx>=0)
        {
            m_Config[fileno].SetByName(Section.GetData(), szName, szValue);
            idx = m_Config[fileno].GetNextInSection(idx, SZ_SECT_ORDERS, szName, szValue);
        }
        m_Config[fileno].RemoveSection(SZ_SECT_ORDERS);

        // Upgrade passwords to faction-specific storage
        S = GetConfig(SZ_SECT_COMMON, SZ_KEY_PWD_OLD);
        S.TrimRight(TRIM_ALL);
        if (!S.IsEmpty())
        {
            Key.Empty();
            Key << (long)m_pAtlantis->m_CrntFactionId;
            SetConfig(SZ_SECT_PASSWORDS, Key.GetData() , S.GetData() );
            SetConfig(SZ_SECT_COMMON   , SZ_KEY_PWD_OLD, (const char *)NULL);
        }
    }
}

//-------------------------------------------------------------------------

/**
 * @brief Composes orders section name for a specific faction
 * @param Sect Output string for section name
 * @param FactionId Faction ID
 * 
 * Creates section name like "ORDER_FILES_123" for faction 123.
 */
void CAhApp::ComposeConfigOrdersSection(CStr & Sect, int FactionId)
{
    Sect = SZ_SECT_ORDERS;
    Sect << "_" << (long)FactionId;
}

//-------------------------------------------------------------------------
// Configuration File Management
//-------------------------------------------------------------------------

/**
 * @brief Determines which configuration file a section belongs to
 * @param szSection Section name
 * @return Configuration file index (CONFIG_FILE_CONFIG or CONFIG_FILE_STATE)
 * 
 * State sections (like orders, flags, filters) go to state file,
 * other sections go to main config file.
 */
int CAhApp::GetConfigFileNo(const char * szSection)
{
    int x;

    if (m_ConfigSectionsState.Search((void*)szSection, x) ||
        0==strnicmp(SZ_SECT_ORDERS, szSection, sizeof(SZ_SECT_ORDERS)-1) ) // orders section is composite starting from 2.1.6
        return CONFIG_FILE_STATE;
    else
        return CONFIG_FILE_CONFIG;
}

//-------------------------------------------------------------------------
// Configuration Access Methods
//-------------------------------------------------------------------------

/**
 * @brief Gets a configuration value
 * @param szSection Configuration section
 * @param szName Parameter name
 * @return Value string (never NULL, empty string if not found)
 * 
 * If value not found, tries to load default from DefaultConfig array.
 * If default exists, it's saved to config file.
 */
const char * CAhApp::GetConfig(const char * szSection, const char * szName)
{
    const char * p;
    int          i;
    int          fileno = GetConfigFileNo(szSection);

    p = m_Config[fileno].GetByName(szSection, szName);
    if (NULL==p)
    {
        // Try to find default value
        for (i=0; i<DefaultConfigSize; i++)
            if ( (0==stricmp(szSection, DefaultConfig[i].szSection)) &&
                 (0==stricmp(szName,    DefaultConfig[i].szName))  )
            {
                p = DefaultConfig[i].szValue;
                break;
            }
        // Save default to config for future use
        m_Config[fileno].SetByName(szSection, szName, p?p:" ");
    }
    if (NULL==p)
        p = "";
    return p;
}

//-------------------------------------------------------------------------

/**
 * @brief Sets a string configuration value
 * @param szSection Configuration section
 * @param szName Parameter name
 * @param szNewValue New value (NULL to delete)
 */
void CAhApp::SetConfig(const char * szSection, const char * szName, const char * szNewValue)
{
    int  fileno = GetConfigFileNo(szSection);
    m_Config[fileno].SetByName(szSection, szName, szNewValue);
}

//-------------------------------------------------------------------------

/**
 * @brief Sets a numeric configuration value
 * @param szSection Configuration section
 * @param szName Parameter name
 * @param lNewValue New value
 */
void CAhApp::SetConfig(const char * szSection, const char * szName, long lNewValue)
{
    char   buf[64];
    int    fileno = GetConfigFileNo(szSection);

    sprintf(buf, "%ld", lNewValue);
    m_Config[fileno].SetByName(szSection, szName, buf);
}

//-------------------------------------------------------------------------
// Section Iteration Methods
//-------------------------------------------------------------------------

/**
 * @brief Gets first entry in a configuration section
 * @param szSection Section name
 * @param szName Output parameter name
 * @param szValue Output parameter value
 * @return Index for subsequent calls, or -1 if section empty
 * 
 * If section doesn't exist, tries to create it from defaults.
 */
int  CAhApp::GetSectionFirst(const char * szSection, const char *& szName, const char *& szValue)
{
    int idx;
    int i;
    int fileno = GetConfigFileNo(szSection);

    idx = m_Config[fileno].GetFirstInSection(szSection, szName, szValue);
    if (idx < 0)
    {
        // Section doesn't exist - try to create from defaults
        for (i=0; i<DefaultConfigSize; i++)
            if (0==stricmp(szSection, DefaultConfig[i].szSection))
                m_Config[fileno].SetByName(szSection, DefaultConfig[i].szName, DefaultConfig[i].szValue);

        idx = m_Config[fileno].GetFirstInSection(szSection, szName, szValue);
    }

    return idx;
}

//-------------------------------------------------------------------------

/**
 * @brief Gets next entry in a configuration section
 * @param idx Previous index from GetSectionFirst/GetSectionNext
 * @param szSection Section name
 * @param szName Output parameter name
 * @param szValue Output parameter value
 * @return Next index or -1 if no more
 */
int  CAhApp::GetSectionNext (int idx, const char * szSection, const char *& szName, const char *& szValue)
{
    int   fileno = GetConfigFileNo(szSection);
    return m_Config[fileno].GetNextInSection (idx, szSection, szName, szValue);
}

//-------------------------------------------------------------------------

/**
 * @brief Removes an entire configuration section
 * @param szSection Section name to remove
 */
void  CAhApp::RemoveSection(const char * szSection)
{
    int fileno = GetConfigFileNo(szSection);
    m_Config[fileno].RemoveSection(szSection);
}

//-------------------------------------------------------------------------

/**
 * @brief Gets the next section name in a configuration file
 * @param fileno Configuration file index
 * @param szStart Previous section name (NULL for first)
 * @return Next section name, or NULL if no more
 */
const char * CAhApp::GetNextSectionName(int fileno, const char * szStart)
{
    const char * szNextSection = NULL;

    if (fileno!=CONFIG_FILE_STATE && fileno!=CONFIG_FILE_CONFIG)
        return NULL;

    m_Config[fileno].GetNextSection(szStart, szNextSection);

    return szNextSection;
}