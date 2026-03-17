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
#include "data.h"
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
// Prepares for loading a new report by saving current state
// Saves flags, comments, orders, and history before switching
//-------------------------------------------------------------------------

void CAhApp::PreLoadReport()
{
    CStr S, FName;

    // Save all current state before loading new report
    SaveLandFlags();
    SaveUnitFlags();
    if (m_CommentsChanged)
        SaveComments();
    if (GetOrdersChanged())
        SaveOrders(TRUE);

    // Save history if current report parsed successfully
    if (ERR_OK==m_pAtlantis->m_ParseErr)
        SaveHistory(SZ_HISTORY_FILE);
}

//-------------------------------------------------------------------------
// Performs post-processing after loading a report
// Updates UI, sets up properties, saves descriptions, etc.
//-------------------------------------------------------------------------

void CAhApp::PostLoadReport()
{
    CStr              S;
    CMapFrame* pMapFrame = (CMapFrame*)m_Frames[AH_FRAME_MAP];
    CMapPane* pMapPane = (CMapPane*)m_Panes[AH_PANE_MAP];
    CUnitPaneFltr* pUnitPaneF = (CUnitPaneFltr*)m_Panes[AH_PANE_UNITS_FILTER];
    CUnitPane* pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];
    long              year, mon;
    const char* szName;
    const char* szValue;
    CUnit* pUnit;
    CPlane* pPlane;
    CShortNamedObj* pItem;
    CFaction          DummyFaction;
    CFaction* pFaction;
    int               i, n;

    // Update edge structures (roads, coastlines) based on new data
    UpdateEdgeStructs();

    SaveLandFlags();

    // Count our faction's men in every hex for display
    m_pAtlantis->CountMenForTheFaction(m_pAtlantis->m_CrntFactionId);

    // Update window title with current turn information
    if (pMapFrame)
    {
        m_sTitle.Empty();

        // Add faction names/IDs
        for (i = 0; i < m_pAtlantis->m_OurFactions.Count(); i++)
        {
            pFaction = m_pAtlantis->GetFaction(static_cast<long>(reinterpret_cast<intptr_t>(m_pAtlantis->m_OurFactions.At(i))));
            if (pFaction)
            {
                if (!m_sTitle.IsEmpty())
                    m_sTitle << ", ";
                if (m_pAtlantis->m_OurFactions.Count() < 3)
                    m_sTitle << pFaction->Name << " ";
                m_sTitle << (long)pFaction->Id;
            }
        }
        
        // Add month and year
        year = (long)(m_pAtlantis->m_YearMon / 100);
        mon = m_pAtlantis->m_YearMon % 100 - 1;
        if ((mon >= 0) && (mon < 12))
            m_sTitle << ". " << Monthes[mon] << " year " << year;
        SetMapFrameTitle();
    }

    // If this is the first report loaded, center the map
    if (GetSectionFirst(SZ_SECT_REPORTS, szName, szValue) < 0)
    {
        wxCommandEvent event(wxEVT_COMMAND_TOOL_CLICKED, tool_centerout);

        if (m_Panes[AH_PANE_MAP])
            ((CMapPane*)m_Panes[AH_PANE_MAP])->OnToolbarCmd(event);
    }

    // Register standard unit property names and their types
    // These are used for filtering and display
#define SET_UNIT_PROP_NAME(_name, _type)                                 \
    {                                                                        \
        CStrInt         * pSI, SI;                                           \
        int               k;                                                 \
        if (!m_pAtlantis->m_UnitPropertyNames.Search((void*)_name, k))       \
            m_pAtlantis->m_UnitPropertyNames.Insert(strdup(_name));          \
        SI.m_key = _name;                                                    \
        if (!m_pAtlantis->m_UnitPropertyTypes.Search(&SI, k))                \
        {                                                                    \
            pSI = new CStrInt(_name, _type);                                 \
            m_pAtlantis->m_UnitPropertyTypes.Insert(pSI);                    \
        }                                                                    \
        SI.m_key = NULL;                                                     \
    }

    SET_UNIT_PROP_NAME(PRP_COMMENTS, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_ORDERS, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FACTION_ID, eLong)
    SET_UNIT_PROP_NAME(PRP_FACTION, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_LAND_ID, eLong)
    SET_UNIT_PROP_NAME(PRP_ID, eLong)
    SET_UNIT_PROP_NAME(PRP_NAME, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FULL_TEXT, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_TEACHING, eLong)
    SET_UNIT_PROP_NAME(PRP_SEQUENCE, eLong)
    SET_UNIT_PROP_NAME(PRP_FRIEND_OR_FOE, eLong)
    SET_UNIT_PROP_NAME(PRP_WEIGHT, eLong)
    SET_UNIT_PROP_NAME(PRP_WEIGHT_WALK, eLong)
    SET_UNIT_PROP_NAME(PRP_WEIGHT_RIDE, eLong)
    SET_UNIT_PROP_NAME(PRP_WEIGHT_FLY, eLong)
    SET_UNIT_PROP_NAME(PRP_WEIGHT_SWIM, eLong)
    SET_UNIT_PROP_NAME(PRP_BEST_SKILL, eLong)
    SET_UNIT_PROP_NAME(PRP_BEST_SKILL_DAYS, eLong)
    SET_UNIT_PROP_NAME(PRP_DESCRIPTION, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_COMBAT, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_GUI_COLOR, eLong)
    SET_UNIT_PROP_NAME(PRP_MOVEMENT, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_STANDARD, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_CUSTOM, eCharPtr)
    SET_UNIT_PROP_NAME(PRP_FLAGS_CUSTOM_ABBR, eCharPtr)

    LoadTerrainCostConfig();

    // If no orders loaded, force recalculation of weights and flags
    if (!m_pAtlantis->m_OrdersLoaded)
    {
        for (i = 0; i < m_pAtlantis->m_Units.Count(); i++)
        {
            pUnit = (CUnit*)m_pAtlantis->m_Units.At(i);
            pUnit->ResetNormalProperties();
        }

        for (n = 0; n < m_pAtlantis->m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
            for (i = 0; i < pPlane->Lands.Count(); i++)
            {
                ((CLand*)pPlane->Lands.At(i))->CalcStructsLoad();
                ((CLand*)pPlane->Lands.At(i))->SetFlagsFromUnits();
            }
        }
    }

    // Save skill descriptions to configuration
    for (i = 0; i < m_pAtlantis->m_Skills.Count(); i++)
    {
        pItem = (CShortNamedObj*)m_pAtlantis->m_Skills.At(i);
        EncodeConfigLine(S, pItem->Description.GetData());
        SetConfig(SZ_SECT_SKILLS, pItem->Name.GetData(), S.GetData());
    }

    // Save item descriptions to configuration
    for (i = 0; i < m_pAtlantis->m_Items.Count(); i++)
    {
        pItem = (CShortNamedObj*)m_pAtlantis->m_Items.At(i);
        EncodeConfigLine(S, pItem->Description.GetData());
        SetConfig(SZ_SECT_ITEMS, pItem->Name.GetData(), S.GetData());
    }

    // Save object descriptions to configuration
    for (i = 0; i < m_pAtlantis->m_Objects.Count(); i++)
    {
        pItem = (CShortNamedObj*)m_pAtlantis->m_Objects.At(i);
        EncodeConfigLine(S, pItem->Description.GetData());
        SetConfig(SZ_SECT_OBJECTS, pItem->Name.GetData(), S.GetData());
    }

    // ===== NEW: Update configuration from descriptions using regex rules =====
    if (m_pAtlantis)
    {
        // Load regex rules from ah.regexp.cfg
        m_pAtlantis->LoadRegexRules("ah.regexp.cfg");

        // Update configuration from descriptions if there are changes
        if (m_pAtlantis->UpdateConfigFromDescriptions(&m_Config[CONFIG_FILE_CONFIG]))
        {
            // Mark that config needs to be saved
            m_CommentsChanged = TRUE;
        }
    }
    // ===== END NEW =====

    // Refresh map display
    if (pMapPane)
        pMapPane->Refresh(FALSE, NULL);

    // Force unit pane to do full update
    if (pUnitPane)
        pUnitPane->m_pCurLand = NULL;
    OnMapSelectionChange();

    // Show hex events if any occurred
    if (!m_pAtlantis->m_HexEvents.Description.IsEmpty())
    {
        CBaseColl   Coll;
        Coll.Insert(&m_pAtlantis->m_HexEvents);
        ShowDescriptionList(Coll, "Hex Events");
    }

    // Update filtered units pane
    if (pUnitPaneF)
        pUnitPaneF->Update(NULL);

    // Check for output from Python scripts
    CheckRedirectedOutputFiles();

    if (!m_pAtlantis->m_SecurityEvents.Description.IsEmpty())
        m_pAtlantis->m_SecurityEvents.Description << EOL_SCR << EOL_SCR;
}

//-------------------------------------------------------------------------
// Loads a report from a file
// If Join is TRUE, adds to existing data; otherwise replaces
//-------------------------------------------------------------------------

int  CAhApp::LoadReport  (const char * FNameIn, BOOL Join)
{
    CStr S, Sect, S2;
    CStr FName;
    int  LoadOrd;
    int  i;
    long n;
    int  err = ERR_FOPEN;

    wxBeginBusyCursor();

    m_DisableErrs = TRUE;  // Suppress error messages during load

    if (FNameIn && *FNameIn)
    {
        FName = FNameIn;
        FName.TrimRight(TRIM_ALL);

        PreLoadReport();  // Save current state

        // If not joining and not first load, create new parser
        if (!m_FirstLoad && !Join)
        {
            m_pAtlantis = new CAtlaParser(&ThisGameDataHelper);
            LoadTerrainCostConfig();
        }

        // Load history file first if not joining
        if (!Join)
        {
            m_pAtlantis->Clear();
            m_pAtlantis->ParseRep(SZ_HISTORY_FILE, FALSE, TRUE);
        }

        // Register unit property groups so they're available during parsing
        for (i=0; i<m_UnitPropertyGroups.Count(); i++ )
        {
            CStrStr * pSS = (CStrStr*)m_UnitPropertyGroups.At(i);
            #define SET_UNIT_PROP_NAME(_name, _type)                                 \
            {                                                                        \
                CStrInt         * pSI, SI;                                           \
                int               k;                                                 \
                if (!m_pAtlantis->m_UnitPropertyNames.Search((void*)_name, k))       \
                    m_pAtlantis->m_UnitPropertyNames.Insert(strdup(_name));          \
                SI.m_key = _name;                                                    \
                if (!m_pAtlantis->m_UnitPropertyTypes.Search(&SI, k))                \
                {                                                                    \
                    pSI = new CStrInt(_name, _type);                                 \
                    m_pAtlantis->m_UnitPropertyTypes.Insert(pSI);                    \
                }                                                                    \
                SI.m_key = NULL;                                                     \
            }
            SET_UNIT_PROP_NAME(pSS->m_key, eLong)
        }

        // Parse the report
        err = m_pAtlantis->ParseRep(FName.GetData(), Join, FALSE);
        switch (err)
        {
            case ERR_INV_TURN:
                wxMessageBox(wxT("Wrong turn in the report"), wxT("Error"));
                break;
        }
        
        SetOrdersChanged(FALSE);
        m_CommentsChanged = FALSE;
        
        // If successful, update configuration and possibly load orders
        if ( ERR_OK==err && m_pAtlantis->m_YearMon != 0 && m_pAtlantis->m_CrntFactionId != 0 )
        {
            m_ReportDates.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(m_pAtlantis->m_YearMon)));
            UpgradeConfigByFactionId();

            // Save password if configured to read from report
            if (atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_PWD_READ)) && !m_pAtlantis->m_CrntFactionPwd.IsEmpty())
            {
                S.Empty();
                S << (long)m_pAtlantis->m_CrntFactionId;
                SetConfig(SZ_SECT_PASSWORDS, S.GetData(), m_pAtlantis->m_CrntFactionPwd.GetData() );
            }

            // Load orders if configured
            LoadOrd = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_ORDER));
            if (LoadOrd)
            {
                S.Empty();
                S << (long)m_pAtlantis->m_YearMon;
                ComposeConfigOrdersSection(Sect, m_pAtlantis->m_CrntFactionId);
                LoadOrders(GetConfig(Sect.GetData(), S.GetData()));
            }
        }

        // Load comments, flags, and perform post-processing
        LoadComments();
        LoadLandFlags();
        LoadUnitFlags();
        PostLoadReport();

        // Save report filename in configuration for this turn
        if ( (ERR_OK==err) && (m_pAtlantis->m_YearMon != 0) )
        {
            S.Empty();
            S << (long)m_pAtlantis->m_YearMon;
            if (!Join)
                SetConfig(SZ_SECT_REPORTS, S.GetData(), FName.GetData());
            else
            {
                S2 = GetConfig(SZ_SECT_REPORTS, S.GetData());
                if (!S2.IsEmpty())
                    S2 << ", ";
                S2 << FName;
                SetConfig(SZ_SECT_REPORTS, S.GetData(), S2.GetData());
            }
        }

        // Manage report cache (keep only N most recent reports)
        if (!m_FirstLoad && !Join)
        {
            if (m_Reports.Search(m_pAtlantis, i))
                m_Reports.AtFree(i);
            m_Reports.Insert(m_pAtlantis);

            n = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_REP_CACHE_COUNT));
            if (n<=0)
                n = 1;
            if (m_Reports.Count()>n)
            {
                if (i > n/2)
                    n = 0;
                else
                    n = m_Reports.Count()-1;
                if (m_pAtlantis != m_Reports.At(n))
                    m_Reports.AtFree(n);
            }
        }
        m_FirstLoad = FALSE;
    }

    m_DisableErrs = FALSE;  // Re-enable error messages

    wxEndBusyCursor();

    return err;
}

//-------------------------------------------------------------------------
// Shows file dialog and loads a report
// Join parameter determines whether to replace or add to current data
//-------------------------------------------------------------------------

int  CAhApp::LoadReport(BOOL Join)
{
    int rc;
    CStr Dir;
    const char * key;

    key = Join ? SZ_KEY_FOLDER_REP_JOIN : SZ_KEY_FOLDER_REP_LOAD;
    Dir = GetConfig(SZ_SECT_FOLDERS, key);
    if (Dir.IsEmpty())
        Dir = ".";

    wxString CurrentDir = wxGetCwd();
    wxFileDialog dialog(m_Frames[AH_FRAME_MAP],
                        wxT("Load Report"),
                        wxString::FromAscii(Dir.GetData()),
                        wxT(""),
                        wxT(SZ_REP_FILES),
                        wxFD_OPEN);
    rc = dialog.ShowModal();
    wxSetWorkingDirectory(CurrentDir);

    if (wxID_OK == rc)
    {
        CStr S;
        S = dialog.GetPath().mb_str();
        MakePathRelative(CurrentDir.mb_str(), S);

        // Save folder preference
        GetDirFromPath(S.GetData(), Dir);
        SetConfig(SZ_SECT_FOLDERS, key, Dir.GetData() );

        return LoadReport(S.GetData(), Join);
    }
    else
        return ERR_CANCEL;
}

//-------------------------------------------------------------------------
// Switches to a specific turn (by year/month value)
// Loads the report if not already in cache
//-------------------------------------------------------------------------

void CAhApp::SwitchToYearMon(long YearMon)
{
    char          Dummy[sizeof(CAtlaParser)];
    CAtlaParser * pDummy  = (CAtlaParser *)Dummy;
    int           i;
    CStr          S, S2;

    PreLoadReport();  // Save current state
    
    if (GetOrdersChanged())
        return;  // Don't switch if orders have unsaved changes
        
    pDummy->m_YearMon = YearMon;
    
    // Check if report is already in cache
    if (m_Reports.Search(pDummy, i))
    {
        m_pAtlantis = (CAtlaParser *)m_Reports.At(i);
        PostLoadReport();
    }
    else
    {
        // Load from configuration
        S.Empty();
        S << YearMon;

        S2 = GetConfig(SZ_SECT_REPORTS, S.GetData());
        const char * p = S2.GetData();
        BOOL         join = FALSE;
        while (p && *p)
        {
            p = S.GetToken(p, ',');
            LoadReport(S.GetData(), join);
            join = TRUE;
        }
    }
}

//-------------------------------------------------------------------------
// Switches to a report based on sequence (first, prev, next, last, last visited)
//-------------------------------------------------------------------------

void CAhApp::SwitchToRep(eRepSeq whichrep)
{
    int  i;

    m_DisableErrs = TRUE;

    if (CanSwitchToRep(whichrep, i))
        SwitchToYearMon(static_cast<long>(reinterpret_cast<intptr_t>(m_ReportDates.At(i))));

    m_DisableErrs = FALSE;
}

//-------------------------------------------------------------------------
// Checks if switching to a specific report is possible
// Returns index in m_ReportDates if available
//-------------------------------------------------------------------------

BOOL CAhApp::CanSwitchToRep(eRepSeq whichrep, int & RepIdx)
{
    long       ym;
    CStr       sName, sData;
    CLand    * pLand;
    CMapPane * pMapPane;

    RepIdx=-1;

    switch(whichrep)
    {
    case repFirst:
        RepIdx = 0;
        break;

    case repLast:
        if (m_pAtlantis->m_YearMon == static_cast<long>(reinterpret_cast<intptr_t>(m_ReportDates.At(gpApp->m_ReportDates.Count()-1))) )
            RepIdx = -1;  // Already on last report
        else
            RepIdx = m_ReportDates.Count()-1;
        break;

    case repPrev:
        if (m_ReportDates.Search(reinterpret_cast<void*>(static_cast<uintptr_t>(m_pAtlantis->m_YearMon)), RepIdx) )
            RepIdx--;  // Previous index
        break;

    case repNext:
        if (m_ReportDates.Search(reinterpret_cast<void*>(static_cast<uintptr_t>(m_pAtlantis->m_YearMon)), RepIdx) )
            RepIdx++;  // Next index
        break;

    case repLastVisited:
        // Get the last turn when current hex was visited
        pMapPane = (CMapPane* )m_Panes[AH_PANE_MAP];
        pLand    = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, TRUE);
        m_pAtlantis->ComposeLandStrCoord(pLand, sName);
        sData.GetToken(GetConfig(SZ_SECT_LAND_VISITED, sName.GetData()), ',');
        ym = atol(sData.GetData());

        if (ym==m_pAtlantis->m_YearMon || !m_ReportDates.Search(reinterpret_cast<void*>(static_cast<uintptr_t>(ym)), RepIdx))
            RepIdx = -1;  // No earlier visit or same as current
        break;
    }

    return (RepIdx>=0 && RepIdx<m_ReportDates.Count());
}

//-------------------------------------------------------------------------
// Gets the parser for the previous turn
// Loads it if not already cached
//-------------------------------------------------------------------------

BOOL CAhApp::GetPrevTurnReport(CAtlaParser *& pPrevTurn)
{
    int idx;

    pPrevTurn = NULL;

    if (CanSwitchToRep(repPrev, idx))
    {
        char          Dummy[sizeof(CAtlaParser)];
        CAtlaParser * pDummy  = (CAtlaParser *)Dummy;
        int           i;
        CStr          S, S2;

        long YearMon = static_cast<long>(reinterpret_cast<intptr_t>(m_ReportDates.At(idx)));

        pDummy->m_YearMon = YearMon;
        
        // Check cache first
        if (m_Reports.Search(pDummy, i))
        {
            pPrevTurn = (CAtlaParser *)m_Reports.At(i);
        }
        else
        {
            // Load from files
            S.Empty();
            S << YearMon;

            S2 = GetConfig(SZ_SECT_REPORTS, S.GetData());
            const char * p = S2.GetData();
            BOOL         join = FALSE;
            m_DisableErrs = TRUE;
            wxBeginBusyCursor();
            pPrevTurn = new CAtlaParser(&ThisGameDataHelper);
            pPrevTurn->ParseRep(SZ_HISTORY_FILE, FALSE, TRUE);
            while (p && *p)
            {
                p = S.GetToken(p, ',');
                pPrevTurn->ParseRep(S.GetData(), join, FALSE);
                join = TRUE;
            }
            wxEndBusyCursor();
            m_DisableErrs = FALSE;
            
            // Cache if successfully loaded
            if (pPrevTurn->m_YearMon == YearMon)
                m_Reports.Insert(pPrevTurn);
            else
            {
                delete pPrevTurn;
                pPrevTurn = NULL;
            }
        }
    }

    return (pPrevTurn != NULL);
}