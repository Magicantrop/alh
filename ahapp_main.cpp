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

#ifdef __WXMAC_OSX__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#endif

CAhApp * gpApp = NULL; // Global pointer to the application instance

wxIMPLEMENT_APP(CAhApp);

CGameDataHelper ThisGameDataHelper;

//=========================================================================
// Constructor - initializes all member variables
//=========================================================================

CAhApp::CAhApp() : m_HexDescrSrc    (128),
                   m_UnitDescrSrc   (128),
                   m_ItemWeights    ( 32),
                   m_OrderHash      (  3),
                   m_TradeItemsHash (  2),
                   m_MenHash        (  2),
                   m_MaxSkillHash   (  6),
                   m_MagicSkillsHash(  6)
{
    m_FirstLoad         = TRUE;
    m_OrdersAreChanged  = FALSE;
    m_CommentsChanged   = FALSE;
    m_UpgradeLandFlags  = FALSE;
    m_DiscardChanges    = FALSE;
    m_SelUnitIdx        = -1;
    m_layout            = 0;
    m_DisableErrs       = FALSE;
    m_pAccel            = NULL;

    memset(m_Frames, 0, sizeof(m_Frames));
    memset(m_Panes , 0, sizeof(m_Panes ));
    memset(m_Fonts , 0, sizeof(m_Fonts ));

    // Descriptive names for fonts (used in options dialog)
    m_FontDescr[FONT_EDIT_DESCR]  = "Descriptions";
    m_FontDescr[FONT_EDIT_ORDER]  = "Orders & comments";
    m_FontDescr[FONT_MAP_COORD ]  = "Map coordinates";
    m_FontDescr[FONT_MAP_TEXT  ]  = "Map text";
    m_FontDescr[FONT_UNIT_LIST ]  = "Unit list";
    m_FontDescr[FONT_EDIT_HDR  ]  = "Edit pane header";
    m_FontDescr[FONT_VIEW_DLG  ]  = "View dialogs";
    m_FontDescr[FONT_ERR_DLG   ]  = "Messages and Errors";
}

CAhApp::~CAhApp()
{
}

//-------------------------------------------------------------------------
// Application initialization - called by wxWidgets framework
//-------------------------------------------------------------------------

bool CAhApp::OnInit()
{
    int               i;
    const char      * p;
    const char      * szName;
    const char      * szValue;
    CStrStr         * pSS;
    CStr              S(32), S2;
    int               sectidx;
    CStrStrColl2      Coll;

    gpApp = this;  // Set global pointer

    // Define which configuration sections belong to the state file (ah.st.cfg)
    // All other sections will go to the main config file (ah.cfg)
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_DEF_ORDERS       ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_ORDERS           ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_REPORTS          ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_LAND_FLAGS       ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_LAND_VISITED     ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_SKILLS           ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_ITEMS            ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_OBJECTS          ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_PASSWORDS        ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_UNIT_TRACKING    ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_FOLDERS          ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_DO_NOT_SHOW_THESE));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_TROPIC_ZONE      ));
    m_ConfigSectionsState.Insert(strdup(SZ_SECT_UNIT_FLAGS       ));

    // Load configuration files
    m_Config[CONFIG_FILE_CONFIG].Load(SZ_CONFIG_FILE);
    m_Config[CONFIG_FILE_STATE ].Load(SZ_CONFIG_STATE_FILE);

    // Upgrade config if needed (from older versions)
    UpgradeConfigFiles();

    // Determine window layout
    m_layout = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_LAYOUT));
    if (m_layout<0)
        m_layout = 0;
    if (m_layout>=AH_LAYOUT_COUNT)
        m_layout = AH_LAYOUT_COUNT-1;

    m_Brightness_Delta = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_BRIGHT_DELTA));

    // Load fonts from configuration
    for (i=0; i<FONT_COUNT; i++)
    {
        S.Empty();
        S << (long)i;
        szValue = GetConfig(SZ_SECT_FONTS_2, S.GetData());
        m_Fonts[i] = NewFontFromStr(szValue);
    }

    // Set end-of-line style based on configuration
    if (0==stricmp(SZ_EOL_MS, GetConfig(SZ_SECT_COMMON, SZ_KEY_EOL)))
        EOL_FILE = EOL_MS;
    else
        EOL_FILE = EOL_UNIX;

    // Load unit property groups (used for categorizing unit properties in lists)
    m_UnitPropertyGroups.m_bDuplicates=TRUE;
    sectidx = GetSectionFirst(SZ_SECT_UNITPROP_GROUPS, szName, szValue);
    while (sectidx >= 0)
    {
        while (szValue && *szValue)
        {
            szValue = S.GetToken(szValue, ',');
            pSS     = new CStrStr(szName, S.GetData());
            if (Coll.Insert(pSS))
                m_UnitPropertyGroups.Insert(pSS);
            else
                delete pSS;
        }
        sectidx = GetSectionNext(sectidx, SZ_SECT_UNITPROP_GROUPS, szName, szValue);
    }
    CUnit::m_PropertyGroupsColl = &m_UnitPropertyGroups;

    // Validate that property group names are not aliases (would cause confusion)
    szName = "";
    for (i=0; i<Coll.Count(); i++)
    {
        pSS = (CStrStr*)Coll.At(i);
        if (0!=stricmp(szName, pSS->m_key))
        {
            szName = pSS->m_key;
            p = ResolveAlias(szName);
            if (0!=stricmp(szName, p))
            {
                S = "Group name \"";
                S << szName << "\" can be resolved as alias for \"" << p << "\"!\r\n";
                ShowError(S.GetData(), S.GetLength(), TRUE);
            }
        }
    }
    Coll.DeleteAll();

    // Initialize movement-related data
    InitMoveModes();
    InitMovementSpeed();

    // Set default attitude for unknown factions
    SetAttitudeForFaction(0, ATT_NEUTRAL);

    // Load water terrain types (used for coastline detection)
    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_WATER_TERRAINS));
    int idx;
    while (p && *p)
    {
        p = SkipSpaces(S.GetToken(p, ','));
        if (!S.IsEmpty() && !m_WaterTerrainNames.Search((void *)S.ToLower(), idx))
            m_WaterTerrainNames.Insert(strdup(S.ToLower()));
    }

    // Load order hash - maps order names to internal IDs
    m_OrderHash.Insert("advance"    ,     (void*)O_ADVANCE    );
    m_OrderHash.Insert("assassinate",     (void*)O_ASSASSINATE);
    m_OrderHash.Insert("attack"     ,     (void*)O_ATTACK     );
    m_OrderHash.Insert("autotax"    ,     (void*)O_AUTOTAX    );
    m_OrderHash.Insert("build"      ,     (void*)O_BUILD      );
    m_OrderHash.Insert("buy"        ,     (void*)O_BUY        );
    m_OrderHash.Insert("claim"      ,     (void*)O_CLAIM      );
    m_OrderHash.Insert("end"        ,     (void*)O_ENDFORM    );
    m_OrderHash.Insert("endturn"    ,     (void*)O_ENDTURN    );
    m_OrderHash.Insert("enter"      ,     (void*)O_ENTER      );
    m_OrderHash.Insert("form"       ,     (void*)O_FORM       );
    m_OrderHash.Insert("give"       ,     (void*)O_GIVE       );
    m_OrderHash.Insert("giveif"     ,     (void*)O_GIVEIF     );
    m_OrderHash.Insert("take"       ,     (void*)O_TAKE       );
    m_OrderHash.Insert("send"       ,     (void*)O_SEND       );
    m_OrderHash.Insert("withdraw"   ,     (void*)O_WITHDRAW   );
    m_OrderHash.Insert("leave"      ,     (void*)O_LEAVE      );
    m_OrderHash.Insert("move"       ,     (void*)O_MOVE       );
    m_OrderHash.Insert("produce"    ,     (void*)O_PRODUCE    );
    m_OrderHash.Insert("promote"    ,     (void*)O_PROMOTE    );
    m_OrderHash.Insert("sail"       ,     (void*)O_SAIL       );
    m_OrderHash.Insert("sell"       ,     (void*)O_SELL       );
    m_OrderHash.Insert("steal"      ,     (void*)O_STEAL      );
    m_OrderHash.Insert("study"      ,     (void*)O_STUDY      );
    m_OrderHash.Insert("teach"      ,     (void*)O_TEACH      );
    m_OrderHash.Insert("turn"       ,     (void*)O_TURN       );

    m_OrderHash.Insert("pillage"    ,     (void*)O_PILLAGE    );
    m_OrderHash.Insert("tax"        ,     (void*)O_TAX        );
    m_OrderHash.Insert("entertain"  ,     (void*)O_ENTERTAIN  );
    m_OrderHash.Insert("work"       ,     (void*)O_WORK       );

    m_OrderHash.Insert("guard"      ,     (void*)O_GUARD      );
    m_OrderHash.Insert("avoid"      ,     (void*)O_AVOID      );
    m_OrderHash.Insert("behind"     ,     (void*)O_BEHIND     );
    m_OrderHash.Insert("reveal"     ,     (void*)O_REVEAL     );
    m_OrderHash.Insert("hold"       ,     (void*)O_HOLD       );
    m_OrderHash.Insert("noaid"      ,     (void*)O_NOAID      );
    m_OrderHash.Insert("consume"    ,     (void*)O_CONSUME    );
    m_OrderHash.Insert("nocross"    ,     (void*)O_NOCROSS    );
    m_OrderHash.Insert("spoils"     ,     (void*)O_SPOILS     );

    m_OrderHash.Insert("recruit"    ,     (void*)O_RECRUIT    );
    m_OrderHash.Insert("share"      ,     (void*)O_SHARE      );

    m_OrderHash.Insert("template"   ,     (void*)O_TEMPLATE   );
    m_OrderHash.Insert("endtemplate",     (void*)O_ENDTEMPLATE);
    m_OrderHash.Insert("all"        ,     (void*)O_ALL        );
    m_OrderHash.Insert("endall"     ,     (void*)O_ENDALL     );

    m_OrderHash.Insert("type"       ,     (void*)O_TYPE       );
    m_OrderHash.Insert("label"      ,     (void*)O_LABEL      );
    m_OrderHash.Insert("name"       ,     (void*)O_NAME       );

    // Add custom valid orders from configuration
    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_VALID_ORDERS));
    while (p && *p)
    {
        const void * data;
        p = SkipSpaces(S.GetToken(p, ','));
        if (!S.IsEmpty() && !m_OrderHash.Locate(S.GetData(), data))
            m_OrderHash.Insert(S.GetData(), (void*)-1);
    }

    // Load trade items hash
    p = SkipSpaces(GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_TRADE_ITEMS));
    while (p && *p)
    {
        const void * data;
        p = SkipSpaces(S.GetToken(p, ','));
        if (!S.IsEmpty() && !m_TradeItemsHash.Locate(S.GetData(), data))
            m_TradeItemsHash.Insert(S.GetData(), (void*)-1);
    }

    // Load men items hash (soldier types)
    p = SkipSpaces(GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_MEN));
    while (p && *p)
    {
        const void * data;
        p = SkipSpaces(S.GetToken(p, ','));
        if (!S.IsEmpty() && !m_MenHash.Locate(S.GetData(), data))
            m_MenHash.Insert(S.GetData(), (void*)-1);
    }

    // Load magic skills hash
    p = SkipSpaces(GetConfig(SZ_SECT_UNITPROP_GROUPS,  PRP_MAG_SKILLS));
    while (p && *p)
    {
        const void * data;
        int x;
        p = SkipSpaces(S.GetToken(p, ','));
        // Remove skill postfix (e.g., "_s" for study, "_d" for days)
        x = S.FindSubStrR(PRP_SKILL_POSTFIX);
        if (x>=0)
            S.DelSubStr(x, S.GetLength()-x+1);

        if (!S.IsEmpty() && !m_MagicSkillsHash.Locate(S.GetData(), data))
            m_MagicSkillsHash.Insert(S.GetData(), (void*)-1);
    }

    // Create initial Atlantis parser and add to reports collection
    m_pAtlantis = new CAtlaParser(&ThisGameDataHelper);
    m_Reports.Insert(m_pAtlantis);

    // Read list of available report dates
    i = GetSectionFirst(SZ_SECT_REPORTS, szName, szValue);
    while (i>=0)
    {
        m_ReportDates.Insert(reinterpret_cast<void*>(static_cast<intptr_t>(atol(szName))));
        i = GetSectionNext (i, SZ_SECT_REPORTS, szName, szValue);
    }

    // Load terrain movement cost configuration
    LoadTerrainCostConfig();

    // Initialize stdout/stderr redirection (for Python integration)
    StdRedirectInit();

    // Create keyboard accelerators
    CreateAccelerator();

    // Open main map window
    OpenMapFrame();

    // Open units window if configured for this layout
    if ((AH_LAYOUT_3_WIN==m_layout || AH_LAYOUT_2_WIN==m_layout) &&
        atol(GetConfig(CUnitFrame::GetConfigSection(m_layout), SZ_KEY_OPEN)) )
        OpenUnitFrame();

    // Open editors window if configured for 3-window layout
    if ((AH_LAYOUT_3_WIN==m_layout) &&
        (atol(GetConfig(CEditsFrame::GetConfigSection(m_layout), SZ_KEY_OPEN))) )
        OpenEditsFrame();

    SetTopWindow(m_Frames[AH_FRAME_MAP]);
    m_Frames[AH_FRAME_MAP]->SetFocus();

    // Load reports from command line arguments if provided
    if (argc>1)
        for (i=1; i<argc; i++)
            LoadReport(wxString(argv[i]).mb_str(), i>1);
    else
        // Otherwise load last report if configured
        if (atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_LOAD_REP)) && (m_ReportDates.Count() > 0) )
        {
            S.Empty();
            S << static_cast<long>(reinterpret_cast<intptr_t>(m_ReportDates.At(m_ReportDates.Count() - 1)));
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

    // Open filtered units window if configured
    if (atol(GetConfig(CUnitFrameFltr::GetConfigSection(m_layout), SZ_KEY_OPEN)) )
        OpenUnitFrameFltr(FALSE);

    return TRUE;
}

//-------------------------------------------------------------------------
// Application exit - saves configuration and cleans up
//-------------------------------------------------------------------------

int CAhApp::OnExit()
{
    int  i;
    CStr S;
    CStr Name;

    CUnit::ResetCustomFlagNames();

    // Save font settings
    for (i = 0; i < FONT_COUNT; i++)
    {
        FontToStr(m_Fonts[i], S);
        Name.Empty();
        Name << (long)i;
        SetConfig(SZ_SECT_FONTS_2, Name.GetData(), S.GetData());
    }

    // Save configuration files (unless user chose to discard changes)
    if (!m_DiscardChanges)
    {
        m_Config[CONFIG_FILE_CONFIG].Save(SZ_CONFIG_FILE);
        m_Config[CONFIG_FILE_STATE].Save(SZ_CONFIG_STATE_FILE);

        // Save history if parsing was successful
        if (ERR_OK == m_pAtlantis->m_ParseErr)
            SaveHistory(SZ_HISTORY_FILE);
    }

    // Clean up hash tables
    m_TradeItemsHash.FreeAll();
    m_MenHash.FreeAll();
    m_MaxSkillHash.FreeAll();
    m_MagicSkillsHash.FreeAll();

    // Free reports collection (including main parser)
    m_Reports.FreeAll();

    // Free fonts
    for (i = 0; i < FONT_COUNT; i++)
        delete m_Fonts[i];

    if (m_pAccel)
        delete m_pAccel;

    // Free various collections
    m_MoveModes.FreeAll();
    m_ItemWeights.FreeAll();
    m_ConfigSectionsState.FreeAll();
    m_OrderHash.FreeAll();
    m_Attitudes.FreeAll();
    m_WaterTerrainNames.FreeAll();

    // Clean up stdout/stderr redirection
    StdRedirectDone();

    gpApp = NULL;
    return 0;
}

//-------------------------------------------------------------------------
// Called when a frame is closing
//-------------------------------------------------------------------------

void CAhApp::FrameClosing(CAhFrame * pFrame)
{
    int  no;

    if (pFrame)
        for (no=0; no<AH_FRAME_COUNT; no++)
            if (m_Frames[no] == pFrame)
            {
                if (AH_FRAME_MAP==no)
                {
                    // Map frame closing - initiate shutdown
                    for (no=0; no<AH_FRAME_COUNT; no++)
                        ForgetFrame(no, FALSE);
                }
                else
                    ForgetFrame(no, TRUE);
                break;
            }
}

//-------------------------------------------------------------------------
// Removes a frame from internal tracking
//-------------------------------------------------------------------------

void CAhApp::ForgetFrame(int no, BOOL frameclosed)
{
    int i;

    if (m_Frames[no])
    {
        m_Frames[no]->Done(frameclosed);

        // Clear references to panes from this frame
        for (i=0; i<AH_PANE_COUNT; i++)
            if (m_Frames[no]->m_Panes[i])
                m_Panes[i] = NULL;

        m_Frames[no] = NULL;
    }
}

//-------------------------------------------------------------------------
// Updates the map frame title with current location and modification status
//-------------------------------------------------------------------------

void CAhApp::SetMapFrameTitle()
{
    CMapFrame   * pMapFrame  = (CMapFrame *)m_Frames[AH_FRAME_MAP];
    CMapPane    * pMapPane   = (CMapPane  * )m_Panes[AH_PANE_MAP];
    CPlane      * pPlane     = NULL;

    CStr          S;

    S = m_sTitle;

    if (pMapPane)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);

        S << " (" << pMapPane->m_SelHexX << "," << pMapPane->m_SelHexY;
        if (pPlane && 0!=stricmp(DEFAULT_PLANE, pPlane->Name.GetData()))
        {
            S << "," << pPlane->Name;
        }
        S << ")";
    }

    // Show [modified] indicator if orders have unsaved changes
    if (m_OrdersAreChanged)
        S << " [modified]";
    if (pMapFrame)
        pMapFrame->SetTitle(wxString::FromAscii(S.GetData()));
}

//-------------------------------------------------------------------------
// Temporarily selects a unit (without updating the unit list selection)
// Used for showing unit descriptions without changing current unit
//-------------------------------------------------------------------------

void CAhApp::SelectTempUnit(CUnit * pUnit)
{
    CEditPane   * pDescription = (CEditPane*)m_Panes[AH_PANE_UNIT_DESCR   ];
    CEditPane   * pOrders      = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMANDS];
    CEditPane   * pComments    = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMENTS];

    OnUnitHexSelectionChange(-1); // Unselect current unit
    m_UnitDescrSrc.Empty();

    if (pUnit)
        m_UnitDescrSrc = pUnit->Description;

    // Update description pane
    if (pDescription)
        pDescription->SetSource(&m_UnitDescrSrc, NULL);
    
    // Orders pane becomes read-only for temporary selection
    if (pOrders)
    {
        pOrders->SetSource(NULL, NULL);
        pOrders->SetReadOnly ( TRUE );
        pOrders->ApplyFonts();
    }
    if (pComments)
    {
        pComments->SetSource(NULL, NULL);
    }
}

//-------------------------------------------------------------------------
// Selects a unit, ensuring its hex is visible and updating all panes
//-------------------------------------------------------------------------

void CAhApp::SelectUnit(CUnit * pUnit)
{
    CMapPane    * pMapPane  = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
    CUnitPane   * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
    CLand       * pLand;
    CPlane      * pPlane;
    int           nx, ny, nz;
    BOOL          refresh;
    BOOL          NeedSetUnit;

    if (!pUnit || !pMapPane)
        return;
    pLand = m_pAtlantis->GetLand(pUnit->LandId);
    if (!pLand)
        return;

    pLand->guiUnit = pUnit->Id;  // Store for UI reference

    LandIdToCoord(pLand->Id, nx, ny, nz);
    pPlane   = (CPlane*)m_pAtlantis->m_Planes.At(nz);

    // Ensure the hex is visible, scrolling if needed
    refresh = pMapPane->EnsureLandVisible(nx, ny, nz, FALSE);
    if (refresh)
        pMapPane->Refresh(FALSE);

    NeedSetUnit = (pUnitPane && (pLand==pUnitPane->m_pCurLand));

    pMapPane->SetSelection(nx, ny, pUnit, pPlane, TRUE);

    // Handle temporary units differently
    if (pUnit->Flags & UNIT_FLAG_TEMP)
    {
        pUnitPane->SelectUnit(-1);
        SelectTempUnit(pUnit);  // Just show description
    }
    else
        if (NeedSetUnit)
            pUnitPane->SelectUnit(pUnit->Id); // Select in unit list
}

//-------------------------------------------------------------------------
// Selects a land (hex) on the map
//-------------------------------------------------------------------------

void CAhApp::SelectLand(CLand * pLand)
{
    CMapPane    * pMapPane  = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
    CUnitPane   * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
    CPlane      * pPlane;
    int           nx, ny, nz;
    BOOL          refresh;

    if (pLand)
    {
        LandIdToCoord(pLand->Id, nx, ny, nz);
        pPlane   = (CPlane*)gpApp->m_pAtlantis->m_Planes.At(nz);

        refresh = pMapPane->EnsureLandVisible(nx, ny, nz, TRUE);
        if (refresh)
            pMapPane->Refresh(FALSE);

        // Only set selection if not already showing this land's units
        if (!pUnitPane || pLand != pUnitPane->m_pCurLand)
            pMapPane->SetSelection(nx, ny, NULL, pPlane, TRUE);
    }
}

//-------------------------------------------------------------------------
// Selects a land by coordinate string (e.g., "48,52")
//-------------------------------------------------------------------------

BOOL CAhApp::SelectLand(const char * landcoords)
{
    CLand * pLand = m_pAtlantis->GetLand(landcoords);

    if (pLand)
    {
        SelectLand(pLand);
        return TRUE;
    }
    else
        return FALSE;
}

//-------------------------------------------------------------------------
// Called when map selection changes - updates all dependent panes
//-------------------------------------------------------------------------

void CAhApp::OnMapSelectionChange()
{
    CLand       * pLand    = NULL;
    CMapPane    * pMapPane = (CMapPane* )m_Panes[AH_PANE_MAP];

    if (pMapPane)
        pLand   = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, TRUE);

    UpdateHexEditPane(pLand);  // Update hex description
    UpdateHexUnitList(pLand);  // Update unit list
    SetMapFrameTitle();        // Update window title
}

//-------------------------------------------------------------------------
// Called when unit selection in hex changes - updates description, orders, comments
//-------------------------------------------------------------------------

void CAhApp::OnUnitHexSelectionChange(long idx)
{
    BOOL          ReadOnly = TRUE;
    CEditPane   * pDescription;
    CEditPane   * pOrders;
    CEditPane   * pComments;
    CUnit       * pUnit;

    m_SelUnitIdx = idx;
    pUnit        = GetSelectedUnit(); // Get unit based on index

    pDescription = (CEditPane*)m_Panes[AH_PANE_UNIT_DESCR   ];
    pOrders      = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMANDS];
    pComments    = (CEditPane*)m_Panes[AH_PANE_UNIT_COMMENTS];

    m_UnitDescrSrc.Empty();

    if (pUnit)
    {
        // Build description with errors and events
        m_UnitDescrSrc = pUnit->Description;
        if (!pUnit->Errors.IsEmpty())
            m_UnitDescrSrc << " ***** Errors:\r\n" << pUnit->Errors;
        if (!pUnit->Events.IsEmpty())
            m_UnitDescrSrc << " ----- Events:\r\n" << pUnit->Events;
        if (pUnit->IsOurs && !pUnit->m_EndTurnDescription.IsEmpty())
            m_UnitDescrSrc << "... At the end of turn:\r\n" << pUnit->m_EndTurnDescription;

        // Determine if orders should be read-only
        ReadOnly = (!pUnit->IsOurs || pUnit->Id<=0);
    }

    // Additional read-only condition: if not showing latest turn
    if (!ReadOnly)
        ReadOnly = (m_pAtlantis->m_YearMon != static_cast<long>(reinterpret_cast<intptr_t>(m_ReportDates.At(gpApp->m_ReportDates.Count()-1))) );

    // Update description pane
    if (pDescription)
        pDescription->SetSource(&m_UnitDescrSrc, NULL);
    
    // Update orders pane
    if (pOrders)
    {
        if (pOrders->m_pEditor->IsModified())
        {
            pOrders->OnKillFocus();  // Save changes if modified
        }

        pOrders->SetSource(pUnit?&pUnit->Orders:NULL,      &m_OrdersAreChanged);
        pOrders->SetReadOnly ( ReadOnly );
        pOrders->ApplyFonts();
    }
    
    // Update comments pane
    if (pComments)
    {
        if (pComments->m_pEditor->IsModified())
        {
            pComments->OnKillFocus();  // Save changes if modified
        }
        pComments->SetSource(pUnit?&pUnit->DefOrders:NULL, &m_CommentsChanged);
    }

    RedrawTracks();  // Update movement tracks on map
}

//-------------------------------------------------------------------------
// Called when an edit pane content changes
//-------------------------------------------------------------------------

void CAhApp::EditPaneChanged(CEditPane * pPane)
{
    CMapPane  * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];
    CLand     * pLand;
    CUnit     * pUnit;

    if (pPane && pMapPane)
    {
        pLand = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, TRUE);
        if (!pLand) return;

        if (pPane == m_Panes[AH_PANE_UNIT_COMMANDS])
        {
            // Unit orders have been changed
            if (pLand->guiUnit)
                m_pAtlantis->RunOrders(pLand);  // Re-run orders to update state
            UpdateHexUnitList(pLand);
            UpdateHexEditPane(pLand);
            SetOrdersChanged(m_OrdersAreChanged); // Update change flag
        }
        else if (pPane == m_Panes[AH_PANE_UNIT_COMMENTS])
        {
            // Unit comments/default orders changed
            pUnit = GetSelectedUnit();
            if (pUnit)
            {
                pUnit->ExtractCommentsFromDefOrders(); // Parse embedded comments
                UpdateHexUnitList(pLand);
            }
        }
    }
}

//-------------------------------------------------------------------------
// Called when an edit pane is double-clicked
// Attempts to parse the clicked text and navigate to referenced unit/land
//-------------------------------------------------------------------------

void CAhApp::EditPaneDClicked(CEditPane * pPane)
{
    const char  * p;
    CStr          src, S;
    char          ch;
    CUnit       * pUnit;
    CBaseObject   Dummy;
    int           idx;
    long          position;

    if (pPane == m_Panes[AH_PANE_MSG])
    {
        pPane->GetValue(src);
        position = pPane->m_pEditor->GetInsertionPoint();

#ifdef __WXMSW__
        // Adjust position for Windows line endings (CRLF)
        long x = 0;
        p = src.GetData();
        while (x<position)
        {
            if ('\n' == p[x])
                position--;
            x++;
        }
#endif
        if (position > src.GetLength())
            position = src.GetLength();

        // Find start of line containing click
        p = src.GetData();
        while (position > 0)
        {
            if (p[position-1]=='\n')
                break;
            position--;
        }

        // Try to parse as order error report: "UNIT 123 ..."
        p = &src.GetData()[position];
        p = SkipSpaces(S.GetToken(p, " \t", ch, TRIM_ALL));
        if (0==stricmp("UNIT", S.GetData()))
        {
            S.GetToken(p, " \t", ch, TRIM_ALL);
            Dummy.Id = atol(S.GetData());
            if (m_pAtlantis->m_Units.Search(&Dummy, idx))
            {
                pUnit = (CUnit*)m_pAtlantis->m_Units.At(idx);
                SelectUnit(pUnit);
                return;
            }
        }

        // Try to parse as report error: "something (123) ..."
        p = &src.GetData()[position];
        p = SkipSpaces(S.GetToken(p, "(\n", ch, TRIM_ALL));
        if ('('==ch)
        {
            S.GetToken(p, ",)\n", ch, TRIM_ALL);
            Dummy.Id = atol(S.GetData());
            if (')'==ch && m_pAtlantis->m_Units.Search(&Dummy, idx))
            {
                pUnit = (CUnit*)m_pAtlantis->m_Units.At(idx);
                SelectUnit(pUnit);
                return;
            }
        }

        // Try to parse as land reference: (5,1,2)
        p = &src.GetData()[position];
        p = SkipSpaces(S.GetToken(p, "(\n", ch, TRIM_ALL));
        if ('('==ch)
        {
            p = SkipSpaces(S.GetToken(p, ")\n", ch, TRIM_ALL));
            if (')' == ch)
            {
                // Try to parse unit number as well: (5,7) NEW 1 (2883585)
                CStr U;
                CLand * pLand = m_pAtlantis->GetLand(S.GetData());
                p = SkipSpaces(U.GetToken(p, "(\n", ch, TRIM_ALL));
                if ('('==ch)
                {
                    U.GetToken(p, ",)\n", ch, TRIM_ALL);
                    if (')' == ch)
                    {
                        Dummy.Id = atol(U.GetData());
                        if (pLand->Units.Search(&Dummy, idx))
                        {
                            pUnit = (CUnit*)pLand->Units.At(idx);
                            SelectUnit(pUnit);
                            return;
                        }
                    }
                }
                if (SelectLand(S.GetData()))
                    return;
            }
        }
    }
}

//---------------------------------------------------------------------------------
// Updates the unit description pane when unit data changes
// Only updates if the unit is currently selected
//---------------------------------------------------------------------------------

void CAhApp::UpdateUnitDescriptionPane(CUnit* pUnit)
{
    if (!gpApp || !pUnit)
        return;

    CEditPane* pDescription = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_DESCR];
    if (!pDescription)
        return;

    CUnit* pSelectedUnit = gpApp->GetSelectedUnit();

    // Only update if this is the currently selected unit
    if (pSelectedUnit && pSelectedUnit->Id == pUnit->Id)
    {
        CStr updatedDescription;
        updatedDescription = pUnit->Description;

        if (!pUnit->Errors.IsEmpty())
            updatedDescription << " ***** Errors:\r\n" << pUnit->Errors;
        if (!pUnit->Events.IsEmpty())
            updatedDescription << " ----- Events:\r\n" << pUnit->Events;
        if (pUnit->IsOurs && !pUnit->m_EndTurnDescription.IsEmpty())
            updatedDescription << "... At the end of turn:\r\n" << pUnit->m_EndTurnDescription;

        gpApp->m_UnitDescrSrc = updatedDescription;
        pDescription->SetSource(&gpApp->m_UnitDescrSrc, NULL);
    }
}