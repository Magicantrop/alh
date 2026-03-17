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

#include "stdafx.h"
#include "stdhdr.h"

#include "wx/listctrl.h"
#include "wx/spinctrl.h"
#include "wx/tokenzr.h"
#include <wx/colour.h>

#include "cstr.h"
#include "collection.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"
#include "hash.h"

#include "ahapp.h"
#include "ahframe.h"
#include "listpane.h"
#include "unitpane.h"
#include "editpane.h"
#include "utildlgs.h"
#include "unitsplitdlg.h"
#include "flagsdlg.h"
#include "takedlg.h"
#include "template.h"
#include "templateselectdlg.h"
#include "errs.h"
#include "transportdlg.h"
#include "producedlg.h"
#include "battledlg.h"


BEGIN_EVENT_TABLE(CUnitPane, wxListCtrl)
    EVT_LIST_ITEM_SELECTED   (list_units_hex, CUnitPane::OnSelected)
    EVT_LIST_COL_CLICK       (list_units_hex, CUnitPane::OnColClicked)
    EVT_LIST_ITEM_RIGHT_CLICK(list_units_hex, CUnitPane::OnRClick)
    EVT_IDLE                 (CUnitPane::OnIdle)

    EVT_MENU             (menu_Popup_ShareSilv     , CUnitPane::OnPopupMenuShareSilv      )
    EVT_MENU             (menu_Popup_Teach         , CUnitPane::OnPopupMenuTeach          )
    EVT_MENU             (menu_Popup_Split         , CUnitPane::OnPopupMenuSplit          )
    EVT_MENU             (menu_Popup_DiscardJunk   , CUnitPane::OnPopupMenuDiscardJunk    )
    EVT_MENU             (menu_Popup_DetectSpies   , CUnitPane::OnPopupMenuDetectSpies    )
    EVT_MENU             (menu_Popup_GiveEverything, CUnitPane::OnPopupMenuGiveEverything )

    EVT_MENU             (menu_Popup_ScoutSimple   , CUnitPane::OnPopupMenuScoutSimple    )
    EVT_MENU             (menu_Popup_ScoutMove     , CUnitPane::OnPopupMenuScoutMove      )
    EVT_MENU             (menu_Popup_ScoutObserver , CUnitPane::OnPopupMenuScoutObserver  )
    EVT_MENU             (menu_Popup_ScoutStealth  , CUnitPane::OnPopupMenuScoutStealth   )
    EVT_MENU             (menu_Popup_ScoutGuard    , CUnitPane::OnPopupMenuScoutGuard     )

    EVT_MENU             (menu_Popup_AddToTracking      , CUnitPane::OnPopupMenuAddUnitToTracking)
    EVT_MENU             (menu_Popup_UnitFlags          , CUnitPane::OnPopupMenuUnitFlags      )
    EVT_MENU             (menu_Popup_IssueOrders        , CUnitPane::OnPopupMenuIssueOrders    )
    EVT_MENU             (menu_Popup_Give               , CUnitPane::OnPopupMenuGive)
    EVT_MENU             (menu_Popup_Take               , CUnitPane::OnPopupMenuTake)
    EVT_MENU             (menu_Popup_MakeTemplate       , CUnitPane::OnPopupMenuMakeTemplate)
    EVT_MENU             (menu_Popup_CreateFromTemplate , CUnitPane::OnPopupMenuCreateFromTemplate)
    EVT_MENU             (menu_Popup_Transport          , CUnitPane::OnPopupMenuTransport)
    EVT_MENU             (menu_Popup_Produce            , CUnitPane::OnPopupMenuProduce)

    EVT_MENU             (menu_Popup_AddToAttackers     , CUnitPane::OnPopupMenuAddToAttackers)
    EVT_MENU             (menu_Popup_AddToDefenders     , CUnitPane::OnPopupMenuAddToDefenders)

END_EVENT_TABLE()


//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
CUnitPane::CUnitPane(wxWindow *parent, wxWindowID id)
          :CListPane(parent, id, wxLC_REPORT | wxLC_SINGLE_SEL)
{
    m_pUnits   = NULL;
    m_pCurLand = NULL;
    m_ColClicked = -1;
    m_nMonthLongColumn = -1;
    ApplyFonts();
}

//--------------------------------------------------------------------------
// Init: Initializes pane with frame and config sections
//--------------------------------------------------------------------------
void CUnitPane::Init(CAhFrame * pParentFrame, const char * szConfigSection, const char * szConfigSectionHdr)
{
    m_pFrame            = pParentFrame;
    m_sConfigSection    = szConfigSection;
    m_sConfigSectionHdr = szConfigSectionHdr;
    m_pLayout           = new CListLayout;
    m_pUnits            = new TPropertyHolderColl(128);
    m_pData             = m_pUnits;
    LoadUnitListHdr();
}

//--------------------------------------------------------------------------
// Done: Cleans up resources
//--------------------------------------------------------------------------
void CUnitPane::Done()
{
    SaveUnitListHdr();
    DeleteAllItems();

    if (m_pLayout)
    {
        m_pLayout->FreeAll();
        delete m_pLayout;
        m_pLayout = NULL;
    }

    if (m_pUnits)
    {
        m_pUnits->DeleteAll();
        delete m_pUnits;
        m_pUnits = NULL;
        m_pData  = NULL;
    }
}

//--------------------------------------------------------------------------
// Update: Refreshes unit list for a given land
//--------------------------------------------------------------------------
void CUnitPane::Update(CLand* pLand)
{
    int               i;
    CUnit* pUnit;
    eSelMode          selmode = sel_by_no;
    long              seldata = 0;
    BOOL              FullUpdate = (pLand != m_pCurLand);
    wxListItem        info;
    CBaseColl         ArrivingUnits;
    long              GuiColor;

    // Clear current units - pointers may be invalid after orders processing
    m_pUnits->DeleteAll();

    if (!FullUpdate)
        for (i = GetItemCount() - 1; i >= 0; i--)
        {
            info.m_itemId = i;
            info.m_col = 0;
            info.m_mask = wxLIST_MASK_DATA;
            GetItem(info);
        }

    if (pLand)
    {
        // Add units currently in the hex
        for (i = 0; i < pLand->Units.Count(); i++)
        {
            pUnit = (CUnit*)pLand->Units.At(i);
            if (pUnit && pUnit->pMovement)
                GuiColor = 1;  // Moving unit
            else if (pUnit && (pUnit->Flags & UNIT_FLAG_GUARDING))
                GuiColor = 3;  // Guarding unit
            else
                GuiColor = 0;  // Normal unit
            pUnit->SetProperty(PRP_GUI_COLOR, eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(GuiColor)), eBoth);
            m_pUnits->AtInsert(m_pUnits->Count(), pUnit);
        }
        m_pUnits->SetSortMode(m_SortKey, NUM_SORTS);

        // Add units moving into this hex
        gpApp->GetUnitsMovingIntoHex(pLand->Id, ArrivingUnits);
        for (i = 0; i < ArrivingUnits.Count(); ++i)
        {
            pUnit = (CUnit*)ArrivingUnits.At(i);
            if (pUnit->LandId != pLand->Id)
            {
                GuiColor = 2;  // Arriving unit
                pUnit->SetProperty(PRP_GUI_COLOR, eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(GuiColor)), eBoth);
                m_pUnits->AtInsert(m_pUnits->Count(), pUnit);
            }
        }

        if (pLand->guiUnit)
        {
            seldata = pLand->guiUnit;
            selmode = sel_by_id;
        }
    }
    m_pCurLand = pLand;

    SetData(selmode, seldata, FullUpdate);

    if ((0 == m_pUnits->Count()) || !FullUpdate)
        gpApp->OnUnitHexSelectionChange(GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED));
}

//--------------------------------------------------------------------------
// SelectUnit: Selects unit by ID
//--------------------------------------------------------------------------
void CUnitPane::SelectUnit(long UnitId)
{
    int               i;
    wxListItem        info;

    for (i=GetItemCount()-1; i>=0; i--)
    {
        info.m_itemId = i;
        info.m_col    = 0;
        info.m_mask   = wxLIST_MASK_DATA;
        GetItem(info);
        if ((int)info.m_data==UnitId)
        {
            SetItemState(i, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
            EnsureVisible(i);
        }
        else
            SetItemState(i, 0, wxLIST_STATE_SELECTED);
    }
}

//--------------------------------------------------------------------------
// Sort: Re-sorts unit list
//--------------------------------------------------------------------------
void CUnitPane::Sort()
{
    long              idx;
    long              data=0;
    wxListItem        info;

    idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx>=0)
    {
        info.SetId(idx);
        info.m_mask  = wxLIST_MASK_DATA;
        GetItem(info);
        data = info.m_data;
    }
    m_pUnits->SetSortMode(m_SortKey, NUM_SORTS);
    SetData(sel_by_id, data, TRUE);
}

//--------------------------------------------------------------------------
// ApplyFonts: Sets font for unit list
//--------------------------------------------------------------------------
void CUnitPane::ApplyFonts()
{
    SetFont(*gpApp->m_Fonts[FONT_UNIT_LIST]);
}

//--------------------------------------------------------------------------
// LoadUnitListHdr: Loads column configuration
//--------------------------------------------------------------------------
void CUnitPane::LoadUnitListHdr()
{
    CListLayoutItem * pLI;
    int               i;
    const char      * szName;
    const char      * szValue;
    CStr              S(32);
    int               width;
    unsigned long     flags;

    m_pLayout->FreeAll();
    i = gpApp->GetSectionFirst(m_sConfigSectionHdr.GetData(), szName, szValue);
    int columnIndex = 0;
    m_nMonthLongColumn = -1;

    while (i >= 0)
    {
        szValue = S.GetToken(szValue, ',');  width = atol(S.GetData());
        szValue = S.GetToken(szValue, ',');  flags = atol(S.GetData());
        szValue = S.GetToken(szValue, ',');
        while (szValue && (*szValue<=' '))
            szValue++;
        if ( width>0 && szValue && strlen(szValue)>0 )
        {
            pLI = new CListLayoutItem(S.GetData(), szValue, width, flags);
            m_pLayout->Insert(pLI);

            // Remember monthlong column index for special handling
            if (stricmp(pLI->m_Name, "monthlong") == 0)
            {
                m_nMonthLongColumn = columnIndex;
            }

            columnIndex++;
        }

        i = gpApp->GetSectionNext(i, m_sConfigSectionHdr.GetData(), szName, szValue);
    }

    SetLayout();
    SetSortName(0,  gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_SORT1));
    SetSortName(1,  gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_SORT2));
    SetSortName(2,  gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_SORT3));
    Sort();
}

//--------------------------------------------------------------------------
// ReloadHdr: Reloads column headers
//--------------------------------------------------------------------------
void CUnitPane::ReloadHdr(const char * szConfigSectionHdr)
{
    eSelMode          selmode = sel_by_no;
    long              seldata = 0;
    int               i, x;

    m_sConfigSectionHdr = szConfigSectionHdr;

    DeleteAllItems();
    x = m_pLayout ? (m_pLayout->Count()+20) : 100;
    for (i=x; i>=0; i--)
        DeleteColumn(i);

    LoadUnitListHdr();

    if (m_pCurLand && m_pCurLand->guiUnit)
    {
        seldata = m_pCurLand->guiUnit;
        selmode = sel_by_id;
    }

    SetData(selmode, seldata, TRUE);
}

//--------------------------------------------------------------------------
// SaveUnitListHdr: Saves column configuration
//--------------------------------------------------------------------------
void CUnitPane::SaveUnitListHdr()
{
    CListLayoutItem * pLI;
    int               i;
    CStr              Key;
    CStr              Val;

    if (m_pLayout)
    {
        gpApp->RemoveSection(m_sConfigSectionHdr.GetData());

        for (i=0; i<m_pLayout->Count(); i++)
        {
            pLI = (CListLayoutItem*)m_pLayout->At(i);

            Key.Format("%03d", i);
            Val.Format("%d, %lu, %s, %s", GetColumnWidth(i), pLI->m_Flags, pLI->m_Name, pLI->m_Caption);
            gpApp->SetConfig(m_sConfigSectionHdr.GetData(), Key.GetData(), Val.GetData());
        }

        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_SORT1, GetSortName(0 ) );
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_SORT2, GetSortName(1 ) );
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_SORT3, GetSortName(2 ) );
    }
}

//--------------------------------------------------------------------------
// OnSelected: Handles unit selection
//--------------------------------------------------------------------------
void CUnitPane::OnSelected(wxListEvent& event)
{
    CEditPane  * pOrders;
    CUnit      * pUnit = GetUnit(event.m_itemIndex);
    bool         changed = false;
    int          idx, i;

    if (pUnit)
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            changed = pOrders->SaveModifications();

        if (changed && m_pCurLand)
        {
            CBaseObject Dummy;
            CUnit * pOldUnit = NULL;

            Dummy.Id = m_pCurLand->guiUnit;
            if (m_pCurLand->Units.Search(&Dummy, idx))
                pOldUnit = (CUnit*)m_pCurLand->Units.At(idx);
            else
            {
                CBaseColl ArrivingUnits;
                gpApp->GetUnitsMovingIntoHex(m_pCurLand->Id, ArrivingUnits);
                for (i=0; i<ArrivingUnits.Count(); ++i)
                {
                    pOldUnit = (CUnit*)ArrivingUnits.At(i);
                    if (pOldUnit->Id == m_pCurLand->guiUnit)
                    {
                        CLand * pEditedLand = gpApp->m_pAtlantis->GetLand(pOldUnit->LandId);
                        if (pEditedLand)
                        {
                            gpApp->m_pAtlantis->RunOrders(pEditedLand);
                            if (m_pCurLand != pEditedLand)
                                gpApp->m_pAtlantis->RunOrders(m_pCurLand);
                        }
                        break;
                    }
                }
            }
        }

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        if (changed)
            gpApp->EditPaneChanged(pOrders);

        gpApp->OnUnitHexSelectionChange(event.m_itemIndex);
    }
}

//--------------------------------------------------------------------------
// GetUnit: Retrieves unit by list index
//--------------------------------------------------------------------------
CUnit * CUnitPane::GetUnit(long index)
{
    wxListItem   info;
    CUnit      * pUnit = NULL;

    info.m_itemId = index;
    info.m_col    = 0;
    info.m_mask   = wxLIST_MASK_DATA;
    if (GetItem(info))
    {
        pUnit = (CUnit*)m_pUnits->At(info.m_itemId);
        wxASSERT(pUnit && (pUnit->Id == (long)info.m_data));
    }

    return pUnit;
}

//--------------------------------------------------------------------------
// SelectNextUnit: Selects next unit in list
//--------------------------------------------------------------------------
void CUnitPane::SelectNextUnit()
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    if (idx < GetItemCount()-1)
    {
        CUnit      * pUnit = GetUnit(idx+1);
        if (pUnit)
            SelectUnit(pUnit->Id);
    }
}

//--------------------------------------------------------------------------
// SelectPrevUnit: Selects previous unit in list
//--------------------------------------------------------------------------
void CUnitPane::SelectPrevUnit()
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

    if (idx>0)
    {
        CUnit      * pUnit = GetUnit(idx-1);
        if (pUnit)
            SelectUnit(pUnit->Id);
    }
}

//--------------------------------------------------------------------------
// OnColClicked: Handles column click for sorting
//--------------------------------------------------------------------------
void CUnitPane::OnColClicked(wxListEvent& event)
{
    m_ColClicked = event.m_col;
}

//--------------------------------------------------------------------------
// OnIdle: Processes delayed sorting
//--------------------------------------------------------------------------
void CUnitPane::OnIdle(wxIdleEvent& event)
{
    if (m_ColClicked>=0)
    {
        CListLayoutItem * p;
        int               col = m_ColClicked;

        m_ColClicked = -1;
        p = (CListLayoutItem*)m_pLayout->At(col);
        if (p)
        {
            wxString choice, message=wxString::FromAscii(p->m_Caption), caption=wxT("Set sort order");
            wxString choices[NUM_SORTS-1];

            choices[0]=wxT("primary");
            choices[1]=wxT("secondary");
            choices[2]=wxT("tertiary");

            choice = wxGetSingleChoice(message, caption, NUM_SORTS-1, choices, m_pParent);

            if (!choice.IsEmpty())
            {
                int key;
                if (0==stricmp(choice.mb_str(), "primary"))
                    key = 0;
                else if (0==stricmp(choice.mb_str(), "secondary"))
                    key = 1;
                else
                    key = 2;
                SetSortName(key, p->m_Name);
                Sort();
            }
        }
    }

    event.Skip();
}

//--------------------------------------------------------------------------
// OnRClick: Handles right-click context menu
//--------------------------------------------------------------------------
void CUnitPane::OnRClick(wxListEvent& event)
{
    wxMenu       menu;
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);

    int nItems = GetSelectedItemCount();

    if (nItems>1)
    {
        menu.Append(menu_Popup_IssueOrders     , wxT("Issue orders"));
        menu.Append(menu_Popup_UnitFlags       , wxT("Set custom flags"));
        menu.Append(menu_Popup_AddToTracking   , wxT("Add to a tracking group"));

        menu.AppendSeparator();
        menu.Append(menu_Popup_AddToAttackers, wxT("Add to Battle as Attackers"));
        menu.Append(menu_Popup_AddToDefenders, wxT("Add to Battle as Defenders"));

        PopupMenu( &menu, event.GetPoint().x, event.GetPoint().y);
    }
    else if (pUnit)
    {
        if (pUnit->IsOurs)
        {
            if (!IS_NEW_UNIT(pUnit))
            {
                menu.Append(menu_Popup_ShareSilv     , wxT("Share SILV"));
                menu.Append(menu_Popup_Split         , wxT("Split (FORM)"));
            }
            menu.Append(menu_Popup_Teach             , wxT("Teach"));
            if (IS_NEW_UNIT(pUnit))
            {
                menu.Append(menu_Popup_DiscardJunk   , wxT("Discard This Unit"));
                menu.Append(menu_Popup_MakeTemplate  , wxT("Make template"));
                menu.Append(menu_Popup_Take          , wxT("Take items"));
            }
            if (!IS_NEW_UNIT(pUnit))
            {
                menu.Append(menu_Popup_DiscardJunk   , wxT("Discard junk items"));
                menu.Append(menu_Popup_Produce       , wxT("Produce"));
                menu.Append(menu_Popup_GiveEverything, wxT("Give everything"));
                menu.Append(menu_Popup_CreateFromTemplate, wxT("Create from template"));
                menu.Append(menu_Popup_Give, wxT("Give items"));
                menu.Append(menu_Popup_Take, wxT("Take items"));
                menu.Append(menu_Popup_Transport, wxT("Transport items"));

                wxMenu * menuScouts = new wxMenu();
                menuScouts->Append(menu_Popup_ScoutSimple , wxT("Scout"));
                menuScouts->Append(menu_Popup_ScoutMove   , wxT("Scout North"));
                menuScouts->Append(menu_Popup_ScoutObserver, wxT("Scout Observer"));
                menuScouts->Append(menu_Popup_ScoutStealth, wxT("Scout Stealth"));
                menuScouts->Append(menu_Popup_ScoutGuard  , wxT("Scout Guard"));

                menu.AppendSubMenu(menuScouts, wxT("Create"), wxT("Create a new unit with a simple task"));
            }
        }
        menu.Append(menu_Popup_UnitFlags       , wxT("Set custom flags"));
        menu.Append(menu_Popup_AddToTracking   , wxT("Add to a tracking group"));

        menu.AppendSeparator();
        menu.Append(menu_Popup_AddToAttackers, wxT("Add as Attackers"));
        menu.Append(menu_Popup_AddToDefenders, wxT("Add as Defenders"));

        PopupMenu( &menu, event.GetPoint().x, event.GetPoint().y);
    }
}

//--------------------------------------------------------------------------
// Teach command handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuTeach (wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit)
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        gpApp->SetOrdersChanged(gpApp->m_pAtlantis->GenOrdersTeach(pUnit)
                               || gpApp->GetOrdersChanged());
        Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// Split command handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuSplit(wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        CUnitSplitDlg dlg(this, pUnit);
        if (wxID_OK == dlg.ShowModal())
            gpApp->SetOrdersChanged(TRUE);

        Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// CreateScout: Helper to create scout units
//--------------------------------------------------------------------------
bool CUnitPane::CreateScout(CUnit * pUnit, ScoutType scoutType)
{
    CStr         race(32), racemarket;
    EValueType   type;
    long         peritem;
    wxString    newUnitOrders;
    int         newUnitId;

    if (!m_pCurLand) return false;

    newUnitOrders << wxT("name unit scout\n");

    gpApp->m_pAtlantis->ReadPropertyName(m_pCurLand->PeasantRace.GetData(), race);

    if (race.IsEmpty())
        return false;

    peritem = 0;
    MakeQualifiedPropertyName(PRP_SALE_PRICE_PREFIX, race.GetData(), racemarket);
    const void* value = NULL;
    m_pCurLand->GetProperty(racemarket.GetData(), type, value, eNormal);
    if (type == eLong && value)
    {
        peritem = static_cast<long>(reinterpret_cast<intptr_t>(value));
    }

    newUnitOrders << wxString::Format("buy 1 %s\n", wxString::FromAscii(race.GetData()).Lower());

    if (   pUnit->FlagsOrg & UNIT_FLAG_TAXING)             newUnitOrders << wxT("autotax 0\n");
    if (! (pUnit->FlagsOrg & UNIT_FLAG_AVOIDING))          newUnitOrders << wxT("avoid 1\n");
    if (! (pUnit->FlagsOrg & UNIT_FLAG_BEHIND))            newUnitOrders << wxT("behind 1\n");
    if (   pUnit->FlagsOrg & UNIT_FLAG_CONSUMING_UNIT)     newUnitOrders << wxT("consume\n");
    if (   pUnit->FlagsOrg & UNIT_FLAG_CONSUMING_FACTION)  newUnitOrders << wxT("consume\n");
    if (   pUnit->FlagsOrg & UNIT_FLAG_GUARDING)           newUnitOrders << wxT("guard 0\n");
    if (   pUnit->FlagsOrg & UNIT_FLAG_HOLDING)            newUnitOrders << wxT("hold 0\n");
    if (   pUnit->FlagsOrg & UNIT_FLAG_RECEIVING_NO_AID)   newUnitOrders << wxT("noaid 0\n");
    if (   pUnit->FlagsOrg & UNIT_FLAG_NO_CROSS_WATER)     newUnitOrders << wxT("nocross 0\n");
    if (scoutType == SCOUT_STEALTH)
    {
        if ((pUnit->FlagsOrg & UNIT_FLAG_REVEALING_FACTION) || (pUnit->FlagsOrg & UNIT_FLAG_REVEALING_UNIT)) newUnitOrders << wxT("reveal\n");
    }
    else if (! (pUnit->FlagsOrg & UNIT_FLAG_REVEALING_FACTION)) newUnitOrders << wxT("reveal faction\n");
    if (   pUnit->FlagsOrg & UNIT_FLAG_SHARING)            newUnitOrders << wxT("share 0\n");
    newUnitOrders << wxT("spoils none\n");

    newUnitId = m_pCurLand->GetNextNewUnitNo();

    switch (scoutType)
    {
        case SCOUT_SIMPLE:
            newUnitOrders << wxT("@work\n");
        break;
        case SCOUT_MOVE:
            peritem += 10;
            newUnitOrders << wxT("MOVE N\n");
            newUnitOrders << wxT("TURN\n@work\nENDTURN\n");
        break;
        case SCOUT_OBSERVER:
            peritem += 60;
            newUnitOrders << wxT("STUDY OBSERVATION\n");
            newUnitOrders << wxT("@;; name unit \"scout observer\"\n");
            newUnitOrders << wxT("TURN\n@work\nENDTURN\n");
        break;
        case SCOUT_STEALTH:
            peritem += 60;
            newUnitOrders << wxT("STUDY STEALTH\n");
            newUnitOrders << wxT("@;; name unit \"scout stealth\"\n");
            newUnitOrders << wxT("TURN\n@work\nENDTURN\n");
        break;
        case SCOUT_GUARD:
            peritem += 20;
            newUnitOrders << wxT("STUDY COMBAT\n");
            newUnitOrders << wxT("@;; name unit guard\n");
            newUnitOrders << wxT("TURN\navoid 0\nhold 1\n@guard 1\n@work\nENDTURN\n");
        default: ;
    }

    CUnit * pUnitNew = gpApp->m_pAtlantis->SplitUnit(pUnit, newUnitId);
    if (pUnitNew)
        pUnitNew->Orders << newUnitOrders.ToUTF8();

    pUnit->Orders.TrimRight(TRIM_ALL);
    if (!pUnit->Orders.IsEmpty())
        pUnit->Orders << EOL_SCR;

    if (peritem > 0)
    {
        CStr giveCmd;
        giveCmd.Format("GIVE NEW %ld %ld SILV%s", newUnitId, peritem, EOL_SCR);
        pUnit->Orders << giveCmd;
    }

    if (m_pCurLand)
        gpApp->m_pAtlantis->RunOrders(m_pCurLand);

    gpApp->SetOrdersChanged(TRUE);

    return true;
}

//--------------------------------------------------------------------------
// Scout command handlers
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuScoutSimple(wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        if (CreateScout(pUnit, SCOUT_SIMPLE))
            Update(m_pCurLand);
    }
}

void CUnitPane::OnPopupMenuScoutMove(wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        if (CreateScout(pUnit, SCOUT_MOVE))
            Update(m_pCurLand);
    }
}

void CUnitPane::OnPopupMenuScoutObserver(wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        if (CreateScout(pUnit, SCOUT_OBSERVER))
            Update(m_pCurLand);
    }
}

void CUnitPane::OnPopupMenuScoutStealth(wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        if (CreateScout(pUnit, SCOUT_STEALTH))
            Update(m_pCurLand);
    }
}

void CUnitPane::OnPopupMenuScoutGuard(wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        if (CreateScout(pUnit, SCOUT_GUARD))
            Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// Share silver handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuShareSilv  (wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit)
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        gpApp->SetOrdersChanged(gpApp->m_pAtlantis->ShareSilver(pUnit)
                               || gpApp->GetOrdersChanged());
        Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// Give everything handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuGiveEverything (wxCommandEvent& event)
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;
    wxString     N;

    if (pUnit)
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        N = wxGetTextFromUser(wxT("Give everything to unit"), wxT("Confirm"));

        gpApp->SetOrdersChanged(gpApp->m_pAtlantis->GenGiveEverything(pUnit, N.mb_str())
                               || gpApp->GetOrdersChanged());
        Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// Discard junk items handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuDiscardJunk(wxCommandEvent& WXUNUSED(event))
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;

    if (pUnit)
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        if (IS_NEW_UNIT(pUnit))
        {
            CLand * pLand = gpApp->m_pAtlantis->GetLand(pUnit->LandId);
            pLand->RemoveUnit(pUnit);
            delete pUnit;
            gpApp->SetOrdersChanged(true);
        }
        else
            gpApp->SetOrdersChanged(gpApp->m_pAtlantis->DiscardJunkItems(pUnit, gpApp->GetConfig(SZ_SECT_UNITPROP_GROUPS, PRP_JUNK_ITEMS))
                               || gpApp->GetOrdersChanged());
        Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// Detect spies handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuDetectSpies(wxCommandEvent& WXUNUSED(event))
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    CEditPane  * pOrders;
    BOOL         DoCheck;

    if (pUnit)
    {
        DoCheck = atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_SPY_DETECT_WARNING));
        if (DoCheck &&
            wxYES != wxMessageBox(wxT("Really generate orders for spy detection?  It might freeze the program on Linux!"), wxT("Confirm"), wxYES_NO, NULL))
            return;

        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        gpApp->SetOrdersChanged(gpApp->m_pAtlantis->DetectSpies(pUnit,
                                                                atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_SPY_DETECT_LO)),
                                                                atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_SPY_DETECT_HI)),
                                                                atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_SPY_DETECT_AMT)))
                               || gpApp->GetOrdersChanged());
        Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// Add unit to tracking group
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuAddUnitToTracking (wxCommandEvent& WXUNUSED(event))
{
    long         idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit      * pUnit = GetUnit(idx);
    int          sectidx;
    CStr         S;
    const char * szName;
    const char * szValue;
    BOOL         found = FALSE;
    BOOL         ManyUnits = (GetSelectedItemCount() > 1);

    sectidx = gpApp->GetSectionFirst(SZ_SECT_UNIT_TRACKING, szName, szValue);
    while (sectidx >= 0)
    {
        if (!S.IsEmpty())
            S << ",";
        S << szName;
        sectidx = gpApp->GetSectionNext(sectidx, SZ_SECT_UNIT_TRACKING, szName, szValue);
    }
    if (S.IsEmpty())
        S = "Default";


    if (pUnit || ManyUnits)
    {
        CComboboxDlg dlg(this, "Add unit to a tracking group", "Select a group to add unit to.\nTo create a new group, just type in it's name.", S.GetData());
        if (wxID_OK == dlg.ShowModal())
        {
            idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            while (idx>=0)
            {
                pUnit = GetUnit(idx);
                found = FALSE;

                szValue = gpApp->GetConfig(SZ_SECT_UNIT_TRACKING, dlg.m_Choice.GetData());
                while (szValue && *szValue)
                {
                    szValue = S.GetToken(szValue, ',');
                    if (atol(S.GetData()) == pUnit->Id)
                    {
                        found = TRUE;
                        break;
                    }
                }
                if (found)
                    wxMessageBox(wxT("The unit is already in the group."));
                else
                {
                    S = gpApp->GetConfig(SZ_SECT_UNIT_TRACKING, dlg.m_Choice.GetData());
                    S.TrimRight(TRIM_ALL);
                    if (!S.IsEmpty())
                        S << ",";
                    S << pUnit->Id;
                    gpApp->SetConfig(SZ_SECT_UNIT_TRACKING, dlg.m_Choice.GetData(), S.GetData());
                }

                idx   = GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            }
        }
    }
}

//--------------------------------------------------------------------------
// Set custom flags
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuUnitFlags (wxCommandEvent& WXUNUSED(event))
{
    long         idx;
    CUnit      * pUnit;
    unsigned int flags = 0;
    BOOL         ManyUnits = (GetSelectedItemCount() > 1);

    if (!ManyUnits)
    {
        idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        pUnit = GetUnit(idx);
        if (pUnit)
            flags = pUnit->Flags;
    }

    CUnitFlagsDlg dlg(this, ManyUnits?eManyUnits:eThisUnit, flags & UNIT_CUSTOM_FLAG_MASK);
    int rc = dlg.ShowModal();
    idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    while (idx>=0)
    {
        pUnit = GetUnit(idx);

        switch (rc)
        {
        case wxID_OK:
            pUnit->Flags    &= ~UNIT_CUSTOM_FLAG_MASK;
            pUnit->FlagsOrg &= ~UNIT_CUSTOM_FLAG_MASK;
            pUnit->Flags    |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsOrg |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsLast = ~pUnit->Flags;
            break;
        case ID_BTN_SET_ALL_UNIT:
            pUnit->Flags    |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsOrg |= (dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsLast = ~pUnit->Flags;
            break;
        case ID_BTN_RMV_ALL_UNIT:
            pUnit->Flags    &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsOrg &= ~(dlg.m_UnitFlags & UNIT_CUSTOM_FLAG_MASK);
            pUnit->FlagsLast = ~pUnit->Flags;
            break;
        }
        idx   = GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }
    SetData(sel_by_no, -1, FALSE);
}

//--------------------------------------------------------------------------
// Issue orders to multiple units
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuIssueOrders(wxCommandEvent& event)
{
    long         idx;
    CUnit      * pUnit;
    CEditPane  * pOrders;
    BOOL         Changed = FALSE;
    CGetTextDlg  dlg(this, "Order", "Orders for the selected units");

    if (wxID_OK != dlg.ShowModal())
        return;
    dlg.m_Text.TrimRight(TRIM_ALL);
    if (dlg.m_Text.IsEmpty())
        return;


    pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
    if (pOrders)
        pOrders->SaveModifications();

    idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    while (idx>=0)
    {
        pUnit = GetUnit(idx);
        if (pUnit->IsOurs)
        {
            Changed = TRUE;
            pUnit->Orders.TrimRight(TRIM_ALL);
            if (!pUnit->Orders.IsEmpty())
                pUnit->Orders << EOL_SCR;
            pUnit->Orders << dlg.m_Text;
        }
        idx   = GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    if (Changed)
    {
        gpApp->SetOrdersChanged(TRUE);
        gpApp->m_pAtlantis->RunOrders(m_pCurLand);
        Update(m_pCurLand);
    }
}

//--------------------------------------------------------------------------
// Give items dialog
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuGive(wxCommandEvent& event)
{
    long idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);
    CEditPane* pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        CGiveItemsDlg dlg(this, pUnit, m_pCurLand);
        if (wxID_OK == dlg.ShowModal())
        {
            gpApp->SetOrdersChanged(TRUE);
            Update(m_pCurLand);
        }
    }
}

//--------------------------------------------------------------------------
// Take items dialog
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuTake(wxCommandEvent& event)
{
    long idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);
    CEditPane* pOrders;

    if (pUnit)
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        CTakeItemsDlg dlg(this, pUnit, m_pCurLand);
        if (wxID_OK == dlg.ShowModal())
        {
            gpApp->SetOrdersChanged(TRUE);
            Update(m_pCurLand);
        }
    }
}

//--------------------------------------------------------------------------
// Make template from new unit
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuMakeTemplate(wxCommandEvent& event)
{
    long         idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);
    CEditPane* pOrders;

    if (pUnit && IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        CTemplateManager tm;

        wxString unitOrders = wxString::FromUTF8(pUnit->Orders.GetData());
        wxString templateName = tm.ExtractTemplateName(unitOrders);

        if (tm.TemplateExists(templateName))
        {
            wxMessageDialog dlg(this,
                wxString::Format(wxT("Template \"%s\" already exists. Overwrite?"), templateName),
                wxT("Confirm Overwrite"),
                wxYES_NO | wxCANCEL | wxICON_QUESTION);

            if (dlg.ShowModal() != wxID_YES)
            {
                templateName = wxGetTextFromUser(
                    wxT("Enter template name:"),
                    wxT("Save Template"),
                    templateName);

                if (templateName.IsEmpty())
                    return;
            }
        }
        else
        {
            templateName = wxGetTextFromUser(
                wxT("Enter template name:"),
                wxT("Save Template"),
                templateName);

            if (templateName.IsEmpty())
                return;
        }

        if (tm.SaveTemplate(templateName, unitOrders))
        {
            wxMessageBox(wxString::Format(wxT("Template \"%s\" saved successfully."), templateName),
                wxT("Success"),
                wxOK | wxICON_INFORMATION);
        }
        else
        {
            wxMessageBox(wxT("Failed to save template."),
                wxT("Error"),
                wxOK | wxICON_ERROR);
        }
    }
}

//--------------------------------------------------------------------------
// Create units from template
//--------------------------------------------------------------------------
bool CUnitPane::CreateFromTemplate(CUnit* pUnit, const wxString& templateName, int count, bool repeatEveryTurn)
{
    if (!m_pCurLand) return false;

    CTemplateManager tm;
    wxString templateOrders;

    if (!tm.LoadTemplate(templateName, templateOrders))
    {
        wxMessageBox(wxString::Format(wxT("Failed to load template \"%s\""), templateName),
            wxT("Error"),
            wxOK | wxICON_ERROR);
        return false;
    }

    CStr race(32);
    gpApp->m_pAtlantis->ReadPropertyName(m_pCurLand->PeasantRace.GetData(), race);

    if (race.IsEmpty())
    {
        wxMessageBox(wxT("Cannot determine peasant race for this hex."),
            wxT("Error"),
            wxOK | wxICON_ERROR);
        return false;
    }

    CStr menGroup = gpApp->GetConfig(SZ_SECT_UNITPROP_GROUPS, "men");
    CStr leadItem = "lead";

    CStr racemarket;
    long pricePerUnit = 0;
    MakeQualifiedPropertyName(PRP_SALE_PRICE_PREFIX, race.GetData(), racemarket);
    const void* value = NULL;
    EValueType type;
    m_pCurLand->GetProperty(racemarket.GetData(), type, value, eNormal);
    if (type == eLong && value)
    {
        pricePerUnit = static_cast<long>(reinterpret_cast<intptr_t>(value));
    }

    bool anyCreated = false;

    for (int i = 0; i < count; i++)
    {
        int newUnitId = m_pCurLand->GetNextNewUnitNo();
        wxString newUnitOrders;

        long totalSilverNeeded = 0;
        wxStringTokenizer tokenizer(templateOrders, "\n", wxTOKEN_STRTOK);

        while (tokenizer.HasMoreTokens())
        {
            wxString line = tokenizer.GetNextToken();
            wxString trimmedLine = line.Trim(false).Trim();

            if (trimmedLine.Lower().StartsWith("buy "))
            {
                wxString originalLine = line;
                wxString lowerLine = trimmedLine.Lower();

                wxStringTokenizer buyTokenizer(trimmedLine, " \t", wxTOKEN_STRTOK);
                wxVector<wxString> tokens;

                while (buyTokenizer.HasMoreTokens())
                {
                    tokens.push_back(buyTokenizer.GetNextToken());
                }

                if (tokens.size() >= 3)
                {
                    wxString countStr = tokens[1];
                    wxString itemCode = tokens[2];

                    long itemBuyCount = 0;
                    if (countStr.ToLong(&itemBuyCount))
                    {
                        bool isMenGroup = false;

                        if (!menGroup.IsEmpty())
                        {
                            CStr groupList = menGroup;
                            const char* p = groupList.GetData();
                            CStr token(32);

                            while (p && *p)
                            {
                                p = token.GetToken(p, ',');
                                if (stricmp(token.GetData(), itemCode.c_str()) == 0)
                                {
                                    isMenGroup = true;
                                    break;
                                }
                            }
                        }

                        if (isMenGroup && stricmp(itemCode.c_str(), leadItem.GetData()) != 0)
                        {
                            totalSilverNeeded += pricePerUnit * itemBuyCount;

                            int pos = line.Lower().Find(itemCode.Lower());
                            if (pos != wxNOT_FOUND)
                            {
                                wxString before = line.Left(pos);
                                wxString after = line.Mid(pos + itemCode.Length());
                                line = before + wxString::FromAscii(race.GetData()).Lower() + after;
                            }
                        }
                    }
                }
            }

            newUnitOrders << line << "\n";
        }

        CUnit* pUnitNew = gpApp->m_pAtlantis->SplitUnit(pUnit, newUnitId);
        if (pUnitNew)
        {
            pUnitNew->Orders << newUnitOrders.ToUTF8();
            anyCreated = true;

            if (totalSilverNeeded > 0)
            {
                CStr giveCmd;
                giveCmd.Format("GIVE NEW %ld %ld SILV%s", newUnitId, totalSilverNeeded, EOL_SCR);

                pUnit->Orders.TrimRight(TRIM_ALL);
                if (!pUnit->Orders.IsEmpty())
                    pUnit->Orders << EOL_SCR;
                pUnit->Orders << giveCmd;
            }
        }
    }

    if (anyCreated)
    {
        if (m_pCurLand)
            gpApp->m_pAtlantis->RunOrders(m_pCurLand);

        gpApp->SetOrdersChanged(TRUE);
    }

    return anyCreated;
}

//--------------------------------------------------------------------------
// Create from template handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuCreateFromTemplate(wxCommandEvent& event)
{
    long         idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);
    CEditPane* pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        CTemplateManager tm;
        wxArrayString templates = tm.GetTemplateNames();

        if (templates.IsEmpty())
        {
            wxMessageBox(wxT("No templates found in templates.cfg"),
                wxT("Information"),
                wxOK | wxICON_INFORMATION);
            return;
        }

        CTemplateSelectDlg dlg(this, templates);
        if (dlg.ShowModal() == wxID_OK)
        {
            wxString selectedTemplate = dlg.GetSelectedTemplate();
            int count = dlg.GetTemplateCount();
            bool repeatEveryTurn = dlg.GetRepeatEveryTurn();

            if (!selectedTemplate.IsEmpty())
            {
                if (CreateFromTemplate(pUnit, selectedTemplate, count, repeatEveryTurn))
                {
                    Update(m_pCurLand);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------
// Get month-long orders for unit
//--------------------------------------------------------------------------
wxString CUnitPane::GetMonthLongOrdersForUnit(CUnit* pUnit, bool& hasMultiple, bool& hasNone, bool& warningAdded)
{
    wxString result;
    hasMultiple = false;
    hasNone = true;
    warningAdded = false;

    if (!pUnit)
        return result;

    if (!pUnit->IsOurs) {
        hasNone = false;
        return result;
    }

    wxString orders = wxString::FromUTF8(pUnit->Orders.GetData());

    CStr monthLongConfig = gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_ORD_MONTH_LONG);
    wxArrayString monthLongOrders;

    wxStringTokenizer tokenizer(wxString::FromUTF8(monthLongConfig.GetData()), ",", wxTOKEN_STRTOK);
    while (tokenizer.HasMoreTokens()) {
        wxString token = tokenizer.GetNextToken().Trim().Trim(false).Upper();
        if (!token.IsEmpty()) {
            monthLongOrders.Add(token);
        }
    }

    if (monthLongOrders.IsEmpty()) {
        monthLongOrders = wxArrayString();
        monthLongOrders.Add("ADVANCE");
        monthLongOrders.Add("AUTOTAX");
        monthLongOrders.Add("BUILD");
        monthLongOrders.Add("ENTERTAIN");
        monthLongOrders.Add("MOVE");
        monthLongOrders.Add("PILLAGE");
        monthLongOrders.Add("PRODUCE");
        monthLongOrders.Add("SAIL");
        monthLongOrders.Add("STUDY");
        monthLongOrders.Add("TAX");
        monthLongOrders.Add("TEACH");
        monthLongOrders.Add("WORK");
    }

    wxStringTokenizer lineTokenizer(orders, "\n", wxTOKEN_STRTOK);
    wxArrayString foundOrders;
    int orderCount = 0;

    bool inFormBlock = false;
    bool inTurnBlock = false;
    int blockDepth = 0;

    while (lineTokenizer.HasMoreTokens()) {
        wxString line = lineTokenizer.GetNextToken();
        wxString trimmedLine = line.Trim(false).Trim();
        wxString upperLine = line.Upper();

        if (trimmedLine.IsEmpty())
            continue;

        if (upperLine.Contains("FORM") && !upperLine.Contains(";")) {
            inFormBlock = true;
            blockDepth++;
            continue;
        }

        if (upperLine.Contains("END") && !upperLine.Contains(";") && inFormBlock) {
            blockDepth--;
            if (blockDepth == 0) {
                inFormBlock = false;
            }
            continue;
        }

        if (upperLine.Contains("TURN") && !upperLine.Contains(";") && !upperLine.Contains("ENDTURN")) {
            inTurnBlock = true;
            continue;
        }

        if (upperLine.Contains("ENDTURN") && !upperLine.Contains(";")) {
            inTurnBlock = false;
            continue;
        }

        if (inFormBlock || inTurnBlock) {
            continue;
        }

        if (trimmedLine.StartsWith(";"))
            continue;

        wxString cleanLine = trimmedLine;
        if (cleanLine.StartsWith("@")) {
            cleanLine = cleanLine.Mid(1).Trim(false);
        }

        wxString firstWord;
        wxStringTokenizer wordTokenizer(cleanLine, " \t", wxTOKEN_STRTOK);
        if (wordTokenizer.HasMoreTokens()) {
            firstWord = wordTokenizer.GetNextToken().Upper();
        }

        if (monthLongOrders.Index(firstWord) != wxNOT_FOUND) {
            int semicolonPos = line.Find(';');
            int commandPos = line.Upper().Find(firstWord);

            if (commandPos == wxNOT_FOUND) {
                wxString lineWithoutAt = line;
                lineWithoutAt.Replace("@", "");
                commandPos = lineWithoutAt.Upper().Find(firstWord);
            }

            if (semicolonPos != wxNOT_FOUND && semicolonPos < commandPos) {
                continue;
            }

            foundOrders.Add(trimmedLine);
            orderCount++;
        }
    }

    for (size_t i = 0; i < foundOrders.size(); i++) {
        if (!result.IsEmpty()) result << " | ";
        result << foundOrders[i];
    }

    hasNone = foundOrders.empty();
    hasMultiple = (foundOrders.size() > 1);

    bool needsOrderUpdate = false;
    wxString newOrders;

    if (hasMultiple && !orders.Contains("; Overwrite")) {
        needsOrderUpdate = true;
        warningAdded = true;

        wxStringTokenizer lineTokenizer2(orders, "\n", wxTOKEN_STRTOK);

        bool inFormBlock2 = false;
        bool inTurnBlock2 = false;
        int blockDepth2 = 0;
        int monthLongCount = 0;

        while (lineTokenizer2.HasMoreTokens()) {
            wxString line = lineTokenizer2.GetNextToken();
            wxString trimmedLine = line.Trim(false).Trim();
            wxString upperLine = line.Upper();

            if (upperLine.Contains("FORM") && !upperLine.Contains(";")) {
                inFormBlock2 = true;
                blockDepth2++;
                newOrders << line << "\n";
                continue;
            }

            if (upperLine.Contains("END") && !upperLine.Contains(";") && inFormBlock2) {
                blockDepth2--;
                if (blockDepth2 == 0) {
                    inFormBlock2 = false;
                }
                newOrders << line << "\n";
                continue;
            }

            if (upperLine.Contains("TURN") && !upperLine.Contains(";") && !upperLine.Contains("ENDTURN")) {
                inTurnBlock2 = true;
                newOrders << line << "\n";
                continue;
            }

            if (upperLine.Contains("ENDTURN") && !upperLine.Contains(";")) {
                inTurnBlock2 = false;
                newOrders << line << "\n";
                continue;
            }

            if (inFormBlock2 || inTurnBlock2) {
                newOrders << line << "\n";
                continue;
            }

            if (trimmedLine.StartsWith(";")) {
                newOrders << line << "\n";
                continue;
            }

            wxString cleanLine = trimmedLine;
            if (cleanLine.StartsWith("@")) {
                cleanLine = cleanLine.Mid(1).Trim(false);
            }

            wxString firstWord;
            wxStringTokenizer wordTokenizer(cleanLine, " \t", wxTOKEN_STRTOK);
            if (wordTokenizer.HasMoreTokens()) {
                firstWord = wordTokenizer.GetNextToken().Upper();
            }

            if (monthLongOrders.Index(firstWord) != wxNOT_FOUND) {
                int semicolonPos = line.Find(';');
                int commandPos = line.Upper().Find(firstWord);

                if (commandPos == wxNOT_FOUND) {
                    wxString lineWithoutAt = line;
                    lineWithoutAt.Replace("@", "");
                    commandPos = lineWithoutAt.Upper().Find(firstWord);
                }

                if (!(semicolonPos != wxNOT_FOUND && semicolonPos < commandPos)) {
                    monthLongCount++;

                    newOrders << line << "\n";

                    if (monthLongCount > 1) {
                        newOrders << "; Overwrite previous monthlong order\n";
                    }
                    continue;
                }
            }

            newOrders << line << "\n";
        }

        LOG_ERR(ERR_DESIGN, "Unit has multiple monthlong orders");

    }
    else if (!hasMultiple && orders.Contains("; Overwrite")) {
        needsOrderUpdate = true;

        wxStringTokenizer lineTokenizer2(orders, "\n", wxTOKEN_STRTOK);

        while (lineTokenizer2.HasMoreTokens()) {
            wxString line = lineTokenizer2.GetNextToken();
            wxString trimmedLine = line.Trim(false).Trim();

            if (trimmedLine.Contains("; Overwrite previous monthlong order")) {
                continue;
            }

            newOrders << line << "\n";
        }
    }

    if (needsOrderUpdate && !newOrders.IsEmpty()) {
        pUnit->Orders = newOrders.Trim().mb_str();
    }

    return result;
}

//--------------------------------------------------------------------------
// SetData: Overridden to handle month-long column coloring
//--------------------------------------------------------------------------
void CUnitPane::SetData(eSelMode selmode, long seldata, BOOL FullUpdate)
{
    CListPane::SetData(selmode, seldata, FullUpdate);
    if (m_nMonthLongColumn == -1)
        return;

    if (m_pData && m_pLayout) {
        for (int row = 0; row < m_pData->Count(); row++) {
            wxListItem info;
            info.m_itemId = row;
            info.m_col = 0;
            info.m_mask = wxLIST_MASK_DATA;

            if (GetItem(info)) {
                CUnit* pUnit = (CUnit*)m_pUnits->At(row);
                if (pUnit) {
                    // Reset colors before applying new ones
                    SetItemBackgroundColour(row, GetBackgroundColour());
                    SetItemTextColour(row, GetTextColour());

                    // Get GUI color from unit property
                    const void* value = NULL;
                    EValueType type;
                    long guiColor = 0;

                    if (pUnit->GetProperty(PRP_GUI_COLOR, type, value) && type == eLong && value) {
                        guiColor = static_cast<long>(reinterpret_cast<intptr_t>(value));
                    }

                    // Check for arriving units FIRST (highest priority)
                    if (guiColor == 2) { // UNIT_ARRIVING
                        wxColour arrivingColor;
                        StrToColor(&arrivingColor, gpApp->GetConfig(SZ_SECT_COLORS, SZ_UNIT_ARRIVING));
                        if (arrivingColor.IsOk()) {
                            SetItemBackgroundColour(row, arrivingColor);
                            SetItemTextColour(row, GetTextColour());
                        }
                        // Still set month-long column if needed
                        if (pUnit->IsOurs) {
                            bool hasMultiple = false, hasNone = false, warningAdded = false;
                            wxString monthLongOrders = GetMonthLongOrdersForUnit(pUnit, hasMultiple, hasNone, warningAdded);
                            SetItem(row, m_nMonthLongColumn, monthLongOrders);
                        }
                        else {
                            SetItem(row, m_nMonthLongColumn, wxT(""));
                        }
                    }
                    // Check for FOREIGN units with monsters
                    else if (!pUnit->IsOurs && HasMonsters(pUnit)) {
                        // Get monster colors from config
                        wxColour monsterBg;
                        wxColour monsterText;

                        StrToColor(&monsterBg, gpApp->GetConfig(SZ_SECT_COLORS, SZ_MONSTER_BG));
                        StrToColor(&monsterText, gpApp->GetConfig(SZ_SECT_COLORS, SZ_MONSTER_TEXT));

                        // Only apply monster highlighting if colors are defined in config
                        if (monsterBg.IsOk() && monsterText.IsOk()) {
                            SetItemBackgroundColour(row, monsterBg);
                            SetItemTextColour(row, monsterText);
                        }

                        // Foreign units don't show month-long orders
                        SetItem(row, m_nMonthLongColumn, wxT(""));
                    }
                    // Check for moving/guarding units
                    else if (guiColor == 1) { // UNIT_MOVING_OUT
                        wxColour movingColor;
                        StrToColor(&movingColor, gpApp->GetConfig(SZ_SECT_COLORS, SZ_UNIT_MOVING_OUT));
                        if (movingColor.IsOk()) {
                            SetItemBackgroundColour(row, movingColor);
                            SetItemTextColour(row, GetTextColour());
                        }
                        // Continue with month-long handling for our units
                        if (pUnit->IsOurs) {
                            bool hasMultiple = false, hasNone = false, warningAdded = false;
                            wxString monthLongOrders = GetMonthLongOrdersForUnit(pUnit, hasMultiple, hasNone, warningAdded);
                            SetItem(row, m_nMonthLongColumn, monthLongOrders);
                        }
                        else {
                            SetItem(row, m_nMonthLongColumn, wxT(""));
                        }
                    }
                    else if (guiColor == 3) { // UNIT_GUARDING
                        wxColour guardingColor;
                        StrToColor(&guardingColor, gpApp->GetConfig(SZ_SECT_COLORS, SZ_UNIT_GUARDING));
                        if (guardingColor.IsOk()) {
                            SetItemBackgroundColour(row, guardingColor);
                            SetItemTextColour(row, GetTextColour());
                        }
                        // Continue with month-long handling for our units
                        if (pUnit->IsOurs) {
                            bool hasMultiple = false, hasNone = false, warningAdded = false;
                            wxString monthLongOrders = GetMonthLongOrdersForUnit(pUnit, hasMultiple, hasNone, warningAdded);
                            SetItem(row, m_nMonthLongColumn, monthLongOrders);
                        }
                        else {
                            SetItem(row, m_nMonthLongColumn, wxT(""));
                        }
                    }
                    // Process OUR units (with or without monsters) - standard month-long logic
                    else if (pUnit->IsOurs) {
                        bool hasMultiple = false;
                        bool hasNone = false;
                        bool warningAdded = false;
                        wxString monthLongOrders = GetMonthLongOrdersForUnit(pUnit, hasMultiple, hasNone, warningAdded);

                        SetItem(row, m_nMonthLongColumn, monthLongOrders);

                        wxColour noMonthLongColor;
                        wxColour multiMonthLongColor;

                        StrToColor(&noMonthLongColor, gpApp->GetConfig(SZ_SECT_COLORS, SZ_UNIT_NO_MONTHLONG));
                        StrToColor(&multiMonthLongColor, gpApp->GetConfig(SZ_SECT_COLORS, SZ_UNIT_MULTI_MONTHLONG));

                        if (hasNone) {
                            SetItemBackgroundColour(row, noMonthLongColor);
                            SetItemTextColour(row, GetTextColour());
                        }
                        else if (hasMultiple) {
                            SetItemBackgroundColour(row, multiMonthLongColor);

                            if (warningAdded) {
                                SetItemTextColour(row, *wxRED);
                            }
                            else {
                                SetItemTextColour(row, GetTextColour());
                            }
                        }
                        // else - normal colors (already reset at the beginning)
                    }
                    // Foreign units WITHOUT monsters - nothing to do (colors already reset)
                    else {
                        SetItem(row, m_nMonthLongColumn, wxT(""));
                        // Colors already reset at the beginning
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------
// Transport dialog handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuTransport(wxCommandEvent& event)
{
    long idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);
    CEditPane* pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        CTransportItemsDlg dlg(this, pUnit, m_pCurLand, gpApp);
        if (wxID_OK == dlg.ShowModal())
        {
            gpApp->SetOrdersChanged(TRUE);
            Update(m_pCurLand);
        }
    }
}

//--------------------------------------------------------------------------
// Produce dialog handler
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuProduce(wxCommandEvent& event)
{
    long idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);
    CEditPane* pOrders;

    if (pUnit && !IS_NEW_UNIT(pUnit))
    {
        pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
        if (pOrders)
            pOrders->SaveModifications();

        if (m_pCurLand)
            m_pCurLand->guiUnit = pUnit->Id;

        CProduceDlg dlg(this, pUnit, m_pCurLand, gpApp);
        if (wxID_OK == dlg.ShowModal())
        {
            gpApp->SetOrdersChanged(TRUE);
            Update(m_pCurLand);
        }
    }
}

//--------------------------------------------------------------------------
// Battle group handlers
//--------------------------------------------------------------------------
void CUnitPane::OnPopupMenuAddToAttackers(wxCommandEvent& event)
{
    long idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);

    if (!pUnit)
        return;

    if (CBattleDlg::AddUnitToAttackers(pUnit))
    {
        wxMessageBox(wxString::Format("Unit %ld added to attackers", pUnit->Id),
            "Success", wxOK | wxICON_INFORMATION);
    }
    else
    {
        wxMessageBox("Failed to add unit to attackers", "Error", wxOK | wxICON_ERROR);
    }
}

void CUnitPane::OnPopupMenuAddToDefenders(wxCommandEvent& event)
{
    long idx = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    CUnit* pUnit = GetUnit(idx);

    if (!pUnit)
        return;

    if (CBattleDlg::AddUnitToDefenders(pUnit))
    {
        wxMessageBox(wxString::Format("Unit %ld added to defenders", pUnit->Id),
            "Success", wxOK | wxICON_INFORMATION);
    }
    else
    {
        wxMessageBox("Failed to add unit to defenders", "Error", wxOK | wxICON_ERROR);
    }
}

//--------------------------------------------------------------------------
// HasMonsters: Checks if unit has any monsters
//--------------------------------------------------------------------------
bool CUnitPane::HasMonsters(CUnit* pUnit)
{
    if (!pUnit)
        return false;

    // Get monsters group from config
    CStr monstersGroup = gpApp->GetConfig(SZ_SECT_UNITPROP_GROUPS, "monsters");
    if (monstersGroup.IsEmpty())
        return false;

    // Parse monsters group
    CStr token(32);
    const char* p = monstersGroup.GetData();

    while (p && *p)
    {
        p = token.GetToken(p, ',');
        if (token.IsEmpty())
            continue;

        // Get property value
        const void* value = NULL;
        EValueType type;

        if (pUnit->GetProperty(token.GetData(), type, value))
        {
            if (type == eLong && value)
            {
                // Safe casting using intptr_t
                long count = static_cast<long>(reinterpret_cast<intptr_t>(value));
                if (count > 0)
                    return true;
            }
        }
    }

    return false;
}
