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
#include "unitpane.h"
#include "flagsdlg.h"

//-------------------------------------------------------------------------
// Configuration Line Encoding/Decoding
//-------------------------------------------------------------------------

/**
 * Encodes a string for safe storage in config file
 * Replaces newlines with \n escape sequences, strips carriage returns
 */
void EncodeConfigLine(CStr & dest, const char * src)
{
    dest.Empty();
    while (src && *src)
    {
        switch (*src)
        {
        case '\r':  break;  // Strip carriage returns
        case '\n':  dest.AddStr("\\n", 2);  // Encode newline as \n
                    break;
        default  :  dest.AddCh(*src);
        }
        src++;
    }
}

//-------------------------------------------------------------------------

/**
 * Decodes a string from config file back to original
 * Converts \n escape sequences back to actual newlines
 */
void DecodeConfigLine(CStr & dest, const char * src)
{
    BOOL          Esc;

    dest.Empty();
    Esc = FALSE;
    while (src && *src)
    {
        if ('\\' == *src)
            Esc = TRUE;
        else
        {
            if (Esc)
            {
                switch(*src)
                {
                case 'n':
                    dest << EOL_SCR;  // Convert \n to platform-specific newline
                    break;
                default:
                    dest.AddCh('\\');  // Keep other escapes as-is
                    dest.AddCh(*src);
                }
            }
            else
                dest.AddCh(*src);
            Esc = FALSE;
        }

        src++;
    }
}

//-------------------------------------------------------------------------
// Comments Management
//-------------------------------------------------------------------------

/**
 * Loads default orders (comments) for all units from config
 * Comments are stored per unit ID in DEF_ORDERS section
 */
void CAhApp::LoadComments()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    CStr          S;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        sprintf(buf, "%ld", pUnit->Id);

        // Decode and load default orders
        DecodeConfigLine(pUnit->DefOrders, GetConfig(SZ_SECT_DEF_ORDERS, buf));

        pUnit->DefOrders.TrimRight(TRIM_ALL);
        pUnit->ExtractCommentsFromDefOrders();  // Parse embedded comments
    }
    m_CommentsChanged = FALSE;  // Reset change flag after load
}

//-------------------------------------------------------------------------

/**
 * Saves default orders (comments) for all units to config
 */
void CAhApp::SaveComments()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    CStr          S;
    const char  * p;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        S.Empty();
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        pUnit->DefOrders.TrimRight(TRIM_ALL);
        if (pUnit->DefOrders.GetLength() > 0)
        {
            EncodeConfigLine(S, pUnit->DefOrders.GetData());
            p = S.GetData();
        }
        else
            p = NULL;  // No default orders - remove entry
        sprintf(buf, "%ld", pUnit->Id);
        SetConfig(SZ_SECT_DEF_ORDERS, buf, p);
    }
    m_CommentsChanged = FALSE;  // Reset change flag after save
}

//-------------------------------------------------------------------------
// Unit Flags Management
//-------------------------------------------------------------------------

/**
 * Loads custom unit flags from config
 * Only loads flags in the custom flag range (UNIT_CUSTOM_FLAG_MASK)
 */
void CAhApp::LoadUnitFlags()
{
    int           i, x;
    CUnit       * pUnit;
    char          buf[32];
    CStr          S;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        sprintf(buf, "%ld", pUnit->Id);

        x = atol(GetConfig(SZ_SECT_UNIT_FLAGS, buf));
        if (x & UNIT_CUSTOM_FLAG_MASK)  // Only restore custom flags
        {
            pUnit->Flags    |= (x & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsOrg |= (x & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsLast = ~pUnit->Flags;  // Force update on next check
        }
    }
}

//-------------------------------------------------------------------------

/**
 * Saves custom unit flags to config
 * Only saves flags in the custom flag range
 */
void CAhApp::SaveUnitFlags()
{
    int           i;
    CUnit       * pUnit;
    char          buf[32];
    CStr          S;

    for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
        sprintf(buf, "%ld", pUnit->Id);

        S.Empty();
        if (pUnit->Flags & UNIT_CUSTOM_FLAG_MASK)
            S << (long)(pUnit->Flags & UNIT_CUSTOM_FLAG_MASK);
        SetConfig(SZ_SECT_UNIT_FLAGS, buf, S.GetData());  // NULL removes entry
    }
}

//-------------------------------------------------------------------------

/**
 * Applies flag changes to all lands/units
 * Called when user uses "Set All Flags" dialog
 */
void CAhApp::SetAllLandUnitFlags()
{
    CUnitPane  * pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];
    CPlane     * pPlane;
    CLand      * pLand;
    CUnit      * pUnit;
    int          i, n, f, x;
    int          rc;

    CUnitFlagsDlg dlg(m_Frames[AH_FRAME_MAP], eAll, 0);

    rc = dlg.ShowModal();

    // Handle land flags
    if ((ID_BTN_SET_ALL_LAND==rc || ID_BTN_RMV_ALL_LAND==rc) && dlg.m_LandFlags>0)
    {
        // Iterate through all lands in all planes
        for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
            for (i=0; i<pPlane->Lands.Count(); i++)
            {
                pLand = (CLand*)pPlane->Lands.At(i);
                x     = 1;
                for (f=0; f<LAND_FLAG_COUNT; f++)
                {
                    if (dlg.m_LandFlags & x)  // This flag should be changed
                    {
                        if (ID_BTN_RMV_ALL_LAND==rc)
                        {
                            // Clear flag
                            pLand->FlagText[f].Empty();
                        }
                        else
                        {
                            // Set flag with default label if empty
                            if (pLand->FlagText[f].IsEmpty())
                                pLand->FlagText[f] = LandFlagLabel[f];
                        }
                        pLand->Flags |= LAND_HAS_FLAGS;
                    }
                    x <<= 1;  // Move to next flag bit
                }
            }
        }

        // Refresh map display
        if (m_Panes[AH_PANE_MAP])
            (m_Panes[AH_PANE_MAP])->Refresh(FALSE);
    }

    // Handle unit flags
    if ( (ID_BTN_SET_ALL_UNIT==rc || ID_BTN_RMV_ALL_UNIT==rc) && dlg.m_UnitFlags>0 )
    {
        // Iterate through all units
        for (i=0; i<m_pAtlantis->m_Units.Count(); i++)
        {
            pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);

            if (ID_BTN_SET_ALL_UNIT==rc)
            {
                // Add flags
                pUnit->Flags    |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
                pUnit->FlagsOrg |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            }
            else
            {
                // Remove flags
                pUnit->Flags    &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
                pUnit->FlagsOrg &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            }

            pUnit->FlagsLast = ~pUnit->Flags;  // Force update
        }
        // Refresh unit display
        if (pUnitPane)
            pUnitPane->Update(pUnitPane->m_pCurLand);
    }
}

//-------------------------------------------------------------------------
// Land Flags Management
//-------------------------------------------------------------------------

/**
 * Saves all land flags to configuration
 * Also updates last visited information for current land
 */
void CAhApp::SaveLandFlags()
{
    int          i, n, f;
    CPlane     * pPlane;
    CLand      * pLand;
    CStr         sName;
    CStr         sData;
    long         ym_last;
    long         ym_first;
    const char * p;

    // Iterate through all lands
    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            sData.Empty();
            
            // Build flag data string (format: "0:flag1\n1:flag2")
            for (f=0; f<LAND_FLAG_COUNT; f++)
            {
                pLand->FlagText[f].TrimRight(TRIM_ALL);
                if (!pLand->FlagText[f].IsEmpty())
                {
                    if (!sData.IsEmpty())
                        sData << "\\n";
                    sData << (long)f << ":" << pLand->FlagText[f];
                }
            }
            
            // Get land coordinate string
            m_pAtlantis->ComposeLandStrCoord(pLand, sName);

            // Save flags (empty data removes entry)
            if (!sData.IsEmpty() || (pLand->Flags & LAND_HAS_FLAGS))
                SetConfig(SZ_SECT_LAND_FLAGS, sName.GetData(), sData.GetData());

            // Update last visited information for current land
            if (pLand->Flags&LAND_IS_CURRENT)
            {
                // Parse existing visited data (format: "last,first")
                p        = sData.GetToken(GetConfig(SZ_SECT_LAND_VISITED, sName.GetData()), ',');
                ym_last  = atol(sData.GetData());
                if (sData.IsEmpty())
                    ym_first = m_pAtlantis->m_YearMon;
                else
                {
                    p        = sData.GetToken(SkipSpaces(p), ',');
                    ym_first = atol(sData.GetData());
                }
                
                // Update if this turn is newer
                if (ym_last < m_pAtlantis->m_YearMon)
                {
                    sData.Empty();
                    sData << m_pAtlantis->m_YearMon << "," << ym_first;
                    SetConfig(SZ_SECT_LAND_VISITED, sName.GetData(), sData.GetData());
                }
            }
        }
    }
}

//-------------------------------------------------------------------------

/**
 * Loads all land flags from configuration
 */
void CAhApp::LoadLandFlags()
{
    int               sectidx, n;
    const char      * szName;
    const char      * szValue;
    const char      * p;
    const char      * line;
    CStr              sData, sLine, sN;
    CLand           * pLand;

    // Iterate through all entries in LAND_FLAGS section
    sectidx = GetSectionFirst(SZ_SECT_LAND_FLAGS, szName, szValue);
    while (sectidx >= 0)
    {
        pLand   = m_pAtlantis->GetLand(szName);
        if (pLand)
        {
            DecodeConfigLine(sData, szValue);

            // Parse each flag line (format: "flagIndex:flagText")
            line = sData.GetData();
            while (line && *line)
            {
                line = sLine.GetToken(line, '\n');
                p    = sLine.GetData();
                p    = sN.GetToken(p, ':');
                if (p)
                    n = atoi(sN.GetData());
                else
                {
                    // If no colon, treat entire line as flag 0
                    p = sN.GetData();
                    n = 0;
                }
                if (n<0 || n>=LAND_FLAG_COUNT)
                    n = 0;
                pLand->FlagText[n] = p;
                pLand->Flags |= LAND_HAS_FLAGS;
            }
        }
        sectidx = GetSectionNext(sectidx, SZ_SECT_LAND_FLAGS, szName, szValue);
    }
}