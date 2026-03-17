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

#include <wx/filename.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#include "stdafx.h"

#include "files.h"
#include "consts.h"
#include "consts_ah.h"
#include "objs.h"
#include "hash.h"

#include "ahapp.h"
#include "ahframe.h"
#include "mapframe.h"
#include "unitframe.h"
#include "unitframefltr.h"
#include "msgframe.h"
#include "editsframe.h"
#include "editpane.h"
#include "mappane.h"
#include "listpane.h"
#include "shaftsframe.h"
#include "unitpane.h"
#include "utildlgs.h"
#include "unitfilterdlg.h"
#include "unitpanefltr.h"
#include "listcoledit.h"
#include "optionsdlg.h"
#include "flagsdlg.h"
#include "routeplanner.h"

//-------------------------------------------------------------------------
// Sets the orders changed flag and updates window title accordingly
//-------------------------------------------------------------------------

void CAhApp::SetOrdersChanged(BOOL Changed)
{
    m_OrdersAreChanged = Changed;
    SetMapFrameTitle();  // Show [modified] indicator in title
}

//-------------------------------------------------------------------------
// Opens file dialog to load orders from a file
// Updates folder preference and loads the selected file
//-------------------------------------------------------------------------

void CAhApp::LoadOrders()
{
    int rc;
    CStr Dir;

    Dir = GetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS);
    if (Dir.IsEmpty())
        Dir = ".";

    wxString CurrentDir = wxGetCwd();
    wxFileDialog dialog(m_Frames[AH_FRAME_MAP],
                        wxT("Load orders"),
                        wxString::FromAscii(Dir.GetData()),
                        wxT(""),
                        wxT(SZ_ORD_FILES),
                        wxFD_OPEN );
    rc = dialog.ShowModal();
    wxSetWorkingDirectory(CurrentDir);

    if (wxID_OK==rc)
    {
        CStr S;
        S = dialog.GetPath().mb_str();
        MakePathRelative(CurrentDir.mb_str(), S);
        
        // Save folder for next time
        GetDirFromPath(S.GetData(), Dir);
        SetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS, Dir.GetData() );

        // Clear unit list before loading (will be repopulated after parsing)
        CUnitPane * pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];
        if (pUnitPane)
            pUnitPane->Update(NULL);

        LoadOrders(S.GetData());
        SetOrdersChanged(FALSE);  // Newly loaded orders are unchanged
    }
}

//-------------------------------------------------------------------------
// Internal method to load orders from a specific file
// Updates the orders configuration with the loaded file
//-------------------------------------------------------------------------

int  CAhApp::LoadOrders  (const char * FNameIn)
{
    int           err;
    CStr          S(32), FName, Sect;
    int           factid;

    FName = FNameIn;  // Make a copy since FNameIn might come from config
    err = m_pAtlantis->LoadOrders(FName.GetData(), factid);
    if (ERR_OK==err)
    {
        // Remember this orders file for this turn and faction
        S.Empty();
        S << (long)m_pAtlantis->m_YearMon;
        ComposeConfigOrdersSection(Sect, factid);
        SetConfig(Sect.GetData(), S.GetData(), FName.GetData());

        // Update UI
        OnMapSelectionChange();
        RedrawTracks();
    }
    return err;
}

//-------------------------------------------------------------------------
// Saves orders for all our factions
// If UsingExistingName is TRUE, uses previously saved filenames
// Otherwise prompts for filenames
//-------------------------------------------------------------------------

int CAhApp::SaveOrders(BOOL UsingExistingName)
{
    CStr S, FName, Section;
    int  i, id, err=ERR_OK;

    // Save orders for each faction we control
    for (i=0; i<m_pAtlantis->m_OurFactions.Count(); i++)
    {
        id = static_cast<long>(reinterpret_cast<intptr_t>(m_pAtlantis->m_OurFactions.At(i)));
        if (UsingExistingName)
        {
            // Get previously used filename for this faction/turn
            ComposeConfigOrdersSection(Section, id);
            S.Empty();
            S << (long)m_pAtlantis->m_YearMon;
            FName = GetConfig(Section.GetData(), S.GetData());
            FName.TrimRight(TRIM_ALL);
        }
        err = SaveOrders(FName.GetData(), id);
        if (ERR_OK!=err)
            break;
    }

    if (ERR_OK==err)
        SetOrdersChanged(FALSE);  // Saved successfully, no changes pending

    return err;
}

//-------------------------------------------------------------------------
// Saves orders for a specific faction to a file
// If FNameOut is empty, prompts user for filename
//-------------------------------------------------------------------------

int  CAhApp::SaveOrders(const char * FNameOut, int FactionId)
{
    int         err;
    char        buf[64];
    CStr        FName;
    CStr        Dir;
    CStr        S, Section, Prompt, Key;
    CFaction  * pFaction;

    FName = FNameOut;
    FName.TrimRight(TRIM_ALL);

    ComposeConfigOrdersSection(Section, FactionId);
    if (FName.IsEmpty())
    {
        // Generate default filename based on turn and faction
        S.Format("%d", m_pAtlantis->m_YearMon);
        FName = GetConfig(Section.GetData(), S.GetData());
        FName.TrimRight(TRIM_ALL);

        if (FName.IsEmpty())
        {
            GetShortFactName(S, FactionId);
            if (S.IsEmpty())
                S << (long)FactionId;
            FName.Format("%s%04d.ord", S.GetData(), m_pAtlantis->m_YearMon);
        }
        pFaction = m_pAtlantis->GetFaction(FactionId);

        Prompt = "Save orders for ";
        if (pFaction)
            Prompt << pFaction->Name.GetData() << " ";
        else
            Prompt << "Faction ";
        Prompt << (long)FactionId;

        Dir = GetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS);
        Dir.TrimRight(TRIM_ALL);
        if (Dir.IsEmpty())
            Dir = ".";

        CStr File;
        wxString CurrentDir = wxGetCwd();
        GetFileFromPath(FName.GetData(), File);

        MakePathFull(CurrentDir.mb_str(), Dir);
        wxFileDialog dialog((CMapFrame*)m_Frames[AH_FRAME_MAP],
                            wxString::FromAscii(Prompt.GetData()),
                            wxString::FromAscii(Dir.GetData()),
                            wxString::FromAscii(File.GetData()),
                            wxT(SZ_ORD_FILES),
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
        err = dialog.ShowModal();
        wxSetWorkingDirectory(CurrentDir);

        if (wxID_OK == err)
        {
            FName = dialog.GetPath().mb_str();
            MakePathRelative(CurrentDir.mb_str(), FName);
            
            // Save folder preference
            GetDirFromPath(FName.GetData(), Dir);
            SetConfig(SZ_SECT_FOLDERS, SZ_KEY_FOLDER_ORDERS, Dir.GetData() );
        }
        else
            return ERR_CANCEL;

        FName.TrimRight(TRIM_ALL);
    }
    
    if (FName.IsEmpty())
        return ERR_FNAME;

    // Get password for this faction
    Key.Empty();
    Key << (long)FactionId;

    // Save orders using Atlantis parser
    err = m_pAtlantis->SaveOrders(FName.GetData(),
                                  GetConfig(SZ_SECT_PASSWORDS, Key.GetData()),
                                  (BOOL)atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_DECORATE_ORDERS)),
                                  FactionId
                                 );
    if (ERR_OK==err)
    {
        // Record that we saved orders for this turn
        sprintf(buf, "%ld", m_pAtlantis->m_YearMon);
        SetConfig(Section.GetData(), buf, FName.GetData());
    }

    // Save configuration files (including updated orders list)
    m_Config[CONFIG_FILE_CONFIG].Save(SZ_CONFIG_FILE);
    m_Config[CONFIG_FILE_STATE ].Save(SZ_CONFIG_STATE_FILE);

    // Also save history if parsing was successful
    if (ERR_OK==m_pAtlantis->m_ParseErr)
        SaveHistory(SZ_HISTORY_FILE);

    return err;
}

//-------------------------------------------------------------------------
// Returns whether orders have unsaved changes
//-------------------------------------------------------------------------

BOOL CAhApp::GetOrdersChanged()
{
    return m_OrdersAreChanged;
}

//-------------------------------------------------------------------------
// Re-runs orders through the parser to update unit states
// Useful after manually editing orders
//-------------------------------------------------------------------------

void CAhApp::RerunOrders()
{
    m_pAtlantis->RunOrders(NULL);  // Run orders for all lands
    
    // Refresh unit list display
    CUnitPane * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
    if (pUnitPane)
        pUnitPane->Update(pUnitPane->m_pCurLand);
}