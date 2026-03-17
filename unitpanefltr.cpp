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
#include "unitfilterdlg.h"
#include "unitpanefltr.h"
#include "editpane.h"
#include "mappane.h"
#include "extend.h"
#include "utildlgs.h"


BEGIN_EVENT_TABLE(CUnitPaneFltr, wxListCtrl)
    EVT_LIST_ITEM_SELECTED   (list_units_hex_fltr, CUnitPaneFltr::OnSelected)
    EVT_LIST_COL_CLICK       (list_units_hex_fltr, CUnitPaneFltr::OnColClicked)
    EVT_LIST_ITEM_RIGHT_CLICK(list_units_hex_fltr, CUnitPaneFltr::OnRClick)
    EVT_IDLE                 (CUnitPaneFltr::OnIdle)

    EVT_MENU             (menu_Popup_Filter ,  CUnitPaneFltr::OnPopupMenuFilter   )
    EVT_MENU             (menu_Popup_SetSort,  CUnitPaneFltr::OnPopupMenuSetSort   )

    EVT_MENU             (menu_Popup_AddToTracking , CUnitPane::OnPopupMenuAddUnitToTracking)
    EVT_MENU             (menu_Popup_UnitFlags     , CUnitPane::OnPopupMenuUnitFlags      )
    EVT_MENU             (menu_Popup_IssueOrders   , CUnitPaneFltr::OnPopupMenuIssueOrders    )


END_EVENT_TABLE()


//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
CUnitPaneFltr::CUnitPaneFltr(wxWindow *parent, wxWindowID id)
              :CUnitPane(parent, id),
               m_NewUnits(32)
{
    m_ColClickedFltr = -1;
    m_IsUpdating     = FALSE;
}

//--------------------------------------------------------------------------
// Done: Clean up and clear map flags
//--------------------------------------------------------------------------
void CUnitPaneFltr::Done()
{
    CMapPane       * pMapPane = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];

    CUnitPane::Done();
    m_NewUnits.FreeAll();

    if (pMapPane)
    {
        ClearLandFlags();
        pMapPane->Refresh(FALSE);
    }
}

//--------------------------------------------------------------------------
// Update: Apply filter and display matching units
//--------------------------------------------------------------------------
void CUnitPaneFltr::Update(CUnitFilterDlg * pFilter)
{
    CStr             Property[UNIT_SIMPLE_FLTR_COUNT];
    CStr             Compare [UNIT_SIMPLE_FLTR_COUNT];
    CStr             sValue  [UNIT_SIMPLE_FLTR_COUNT];
    long             lValue  [UNIT_SIMPLE_FLTR_COUNT];
    int              i,k;
    CStr             ConfigKey;
    eCompareOp       CompareOp[UNIT_SIMPLE_FLTR_COUNT];
    CUnit          * pUnit, * pPrevUnit;
    BOOL             ok;
    CStr             TrackingGroup;
    CLongSortColl    Tracking(32);
    const char     * p;
    CStr             S;
    BOOL             bUsePython = FALSE;
    CPythonEmbedder  Python(gpApp->m_pAtlantis);
    eEErr            rcPy = E_OK;
    CStr             sPythonText, sRealPythonText;
    CStr             sConfSect;
    CBaseColl        Hexes(64);
    CLand          * pLand, * pPrevLand;
    int              unitidx;
    BOOL             Selected = FALSE;
    BOOL             ShowOnMap= FALSE;
    CMapPane       * pMapPane = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
    int              nl;
    BOOL             ShowGoneUnits = FALSE;
    CAtlaParser    * pPrevTurn = NULL;

    sConfSect = gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_FLTR_SET);
    if (pFilter)
    {
        ShowOnMap     = pFilter->m_chDisplayOnMap->IsChecked();
        ShowGoneUnits = pFilter->m_bFindGoneUnits;
        Selected      = pFilter->m_chUseSelectedHexes->IsChecked();
    }

    // Load filter criteria from dialog or config
    for (i=0; i<UNIT_SIMPLE_FLTR_COUNT; i++)
    {
        if (pFilter)
        {
            Property[i] = pFilter->m_cbProperty[i]->GetValue().mb_str();
            Compare [i] = pFilter->m_cbCompare [i]->GetValue().mb_str();
            sValue  [i] = pFilter->m_tcValue   [i]->GetValue().mb_str();
        }
        else
        {
            ConfigKey.Format("%s%d", SZ_KEY_UNIT_FLTR_PROPERTY, i);  Property[i] = gpApp->GetConfig(sConfSect.GetData(), ConfigKey.GetData());
            ConfigKey.Format("%s%d", SZ_KEY_UNIT_FLTR_COMPARE , i);  Compare [i] = gpApp->GetConfig(sConfSect.GetData(), ConfigKey.GetData());
            ConfigKey.Format("%s%d", SZ_KEY_UNIT_FLTR_VALUE   , i);  sValue  [i] = gpApp->GetConfig(sConfSect.GetData(), ConfigKey.GetData());
        }

        Property[i].TrimRight(TRIM_ALL);    Property[i].TrimLeft(TRIM_ALL);
        Compare [i].TrimRight(TRIM_ALL);    Compare [i].TrimLeft(TRIM_ALL);
        sValue  [i].TrimRight(TRIM_ALL);    sValue  [i].TrimLeft(TRIM_ALL);

        // Convert operator string to enum
        CompareOp[i] = NOP;
        for (k=GT; k<NOP; k++)
            if (0==stricmp(UNIT_FILTER_OPERATION[k], Compare[i].GetData()))
            {
                CompareOp[i] = (eCompareOp)k;
                break;
            }
        lValue[i] = atol(sValue[i].GetData());
    }

    // Load tracking group if specified
    if (pFilter)
    {
        TrackingGroup = pFilter->m_TrackingGroup;
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_UNIT_FLTR_TRACKING, TrackingGroup.GetData());
    }
    else
        TrackingGroup = gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_UNIT_FLTR_TRACKING);

    TrackingGroup.TrimRight(TRIM_ALL);
    if (!TrackingGroup.IsEmpty())
    {
        p = gpApp->GetConfig(SZ_SECT_UNIT_TRACKING, TrackingGroup.GetData());
        while (p && *p)
        {
            p = S.GetToken(p, ',');
            Tracking.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(atol(S.GetData()))));
        }
    }

    // Check if Python filter should be used
    if (!ShowGoneUnits && TrackingGroup.IsEmpty())
        if (pFilter)
        {
            bUsePython = TrackingGroup.IsEmpty() && pFilter->m_rbUsePython->GetValue();
            sPythonText= pFilter->m_tcFilterText->GetValue().mb_str();
        }
        else
        {
            S = gpApp->GetConfig(sConfSect.GetData(), SZ_KEY_UNIT_FLTR_SOURCE);
            bUsePython = (0==stricmp(S.GetData(), SZ_KEY_UNIT_FLTR_SOURCE_PYTHON));
            sPythonText= gpApp->GetConfig(sConfSect.GetData(), SZ_KEY_UNIT_FLTR_PYTHON_CODE);
        }

    if (bUsePython)
    {
        rcPy = Python.InitUnitFilter(sPythonText.GetData(), sRealPythonText);
        if (E_OK != rcPy)
            goto Failed;
    }

    InsertUnitInit();

    // Mode 1: Find disappeared units (compare with previous turn)
    if (ShowGoneUnits)
    {
        if (!gpApp->GetPrevTurnReport(pPrevTurn))
            goto Failed;

        CUnitPane * pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
        if (!pUnitPane)
            goto Failed;

        if (Selected)
            pMapPane->GetSelectedOrAllHexes(Hexes, Selected);
        else
        {
            pLand = pUnitPane->m_pCurLand;
            if (!pLand)
                goto Failed;
            Hexes.Insert(pLand);
        }

        for (nl=0; nl<Hexes.Count(); nl++)
        {
            pLand = (CLand*)Hexes.At(nl);
            pPrevLand = pPrevTurn->GetLand(pLand->Id);
            if (!pPrevLand)
                goto Failed;

            for (unitidx=0; unitidx<pPrevLand->Units.Count(); unitidx++)
            {
                ok    = FALSE;
                pUnit = NULL;
                pPrevUnit = (CUnit*)pPrevLand->Units.At(unitidx);
                
                // Check if unit exists in current turn
                if (pLand->Units.Search(pPrevUnit, i))
                    pUnit = (CUnit*)pLand->Units.At(i);
                    
                if (!pUnit) // Unit disappeared
                {
                    if (gpApp->m_pAtlantis->m_Units.Search(pPrevUnit, i))
                        pUnit = (CUnit*)gpApp->m_pAtlantis->m_Units.At(i);
                    ok = TRUE;
                }
                
                if (!pUnit)
                {
                    // Create temporary unit for disappeared one
                    pUnit = pPrevUnit->AllocSimpleCopy();
                    pUnit->Name.InsStr("-=DISAPPEARED=- ", 0);
                    pUnit->Description.InsStr("-=DISAPPEARED=- ", 0);
                    pUnit->Flags |= UNIT_FLAG_TEMP;
                }

                if (ok)
                {
                    InsertUnit(pUnit);
                    if (ShowOnMap)
                    {
                        CLand * pUnitLand = gpApp->m_pAtlantis->GetLand(pUnit->LandId);
                        if (pUnitLand)
                            pUnitLand->Flags |=  LAND_LOCATED_UNITS;
                    }
                }
            }
        }
    }
    // Mode 2: Normal filtering
    else
    {
        pMapPane->GetSelectedOrAllHexes(Hexes, Selected);
        for (nl=0; nl<Hexes.Count(); nl++)
        {
            pLand = (CLand*)Hexes.At(nl);
            for (unitidx=0; unitidx<pLand->Units.Count(); unitidx++)
            {
                pUnit = (CUnit*)pLand->Units.At(unitidx);
                ok    = TRUE;

                // Apply filter: tracking group, Python, or simple boxes
                if (!TrackingGroup.IsEmpty())
                    ok = Tracking.Search(reinterpret_cast<void*>(static_cast<uintptr_t>(pUnit->Id)), i);
                else if (bUsePython)
                {
                    rcPy = Python.RunUnitFilter(pUnit, ok);
                    if (E_OK != rcPy)
                        break;
                }
                else
                    ok = EvaluateBaseObjectByBoxes(pUnit, Property, CompareOp, sValue, lValue, UNIT_SIMPLE_FLTR_COUNT);

                if (ok)
                {
                    InsertUnit(pUnit);
                    if (ShowOnMap)
                        pLand->Flags |=  LAND_LOCATED_UNITS;
                }
            }
        }
    }

    Hexes.DeleteAll();
    InsertUnitDone();
    pMapPane->RemoveRectangle();
    pMapPane->Refresh(FALSE);


Failed:

    if (bUsePython)
    {
        CStr sOut, sErr;

        Python.DoneUnitFilter();
        gpApp->StdRedirectReadMore(FALSE, sErr);
        gpApp->StdRedirectReadMore(TRUE, sOut);
        if (E_OK != rcPy)
        {
            // Display Python error
            S.Empty();
            S << EOL_SCR << "----------- Python code -------------" << EOL_SCR;
            gpApp->ShowError(S.GetData(), S.GetLength(), TRUE);
            gpApp->ShowError(sRealPythonText.GetData(), sRealPythonText.GetLength(), TRUE);
            S.Empty();
            S << EOL_SCR << "------------------------" << EOL_SCR << EOL_SCR;
            gpApp->ShowError(S.GetData(), S.GetLength(), TRUE);

            gpApp->ShowError(sErr.GetData(), sErr.GetLength(), TRUE);
            gpApp->ShowError(sOut.GetData(), sOut.GetLength(), TRUE);
        }
    }
}

//--------------------------------------------------------------------------
// ClearLandFlags: Reset map highlighting flags
//--------------------------------------------------------------------------
void CUnitPaneFltr::ClearLandFlags()
{
    int      np,nl;
    CPlane * pPlane;
    CLand  * pLand;

    for (np=0; np<gpApp->m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)gpApp->m_pAtlantis->m_Planes.At(np);
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand    = (CLand*)pPlane->Lands.At(nl);
            pLand->Flags = pLand->Flags & (~(unsigned long)LAND_LOCATED_UNITS);
        }
    }
}

//--------------------------------------------------------------------------
// InsertUnitInit: Prepare for inserting filtered units
//--------------------------------------------------------------------------
void CUnitPaneFltr::InsertUnitInit()
{
    m_IsUpdating = TRUE;
    m_pUnits->DeleteAll();
    m_NewUnits.FreeAll();

    ClearLandFlags();
}

//--------------------------------------------------------------------------
// InsertUnit: Add unit to filtered list
//--------------------------------------------------------------------------
void CUnitPaneFltr::InsertUnit(CUnit * pUnit)
{
    if (pUnit->Flags & UNIT_FLAG_TEMP)
        m_NewUnits.Insert(pUnit); // Temporary unit that needs cleanup later

    m_pUnits->AtInsert(m_pUnits->Count(), pUnit);
}

//--------------------------------------------------------------------------
// InsertUnitDone: Finish inserting and update display
//--------------------------------------------------------------------------
void CUnitPaneFltr::InsertUnitDone()
{
    m_pUnits->SetSortMode(m_SortKey, 3);
    SetData(sel_by_no, 0, TRUE);
    m_IsUpdating = FALSE;
}

//--------------------------------------------------------------------------
// OnSelected: Handle unit selection
//--------------------------------------------------------------------------
void CUnitPaneFltr::OnSelected(wxListEvent& event)
{
    if (m_IsUpdating)
        return;
    CUnit       * pUnit     = GetUnit(event.m_itemIndex);

    gpApp->SelectUnit(pUnit);
}

//--------------------------------------------------------------------------
// OnColClicked: Handle column header click
//--------------------------------------------------------------------------
void CUnitPaneFltr::OnColClicked(wxListEvent& event)
{
    wxMenu  menu;

    m_ColClickedFltr = event.m_col;
    menu.Append(menu_Popup_SetSort  , wxT("Set sort order"));
    menu.Append(menu_Popup_Filter   , wxT("Filter"        ));

    PopupMenu( &menu, event.GetPoint().x, event.GetPoint().y);
}

//--------------------------------------------------------------------------
// OnIdle: Idle processing
//--------------------------------------------------------------------------
void CUnitPaneFltr::OnIdle(wxIdleEvent& event)
{
    CUnitPane::OnIdle(event);
}

//--------------------------------------------------------------------------
// OnPopupMenuSetSort: Set sort column from context menu
//--------------------------------------------------------------------------
void CUnitPaneFltr::OnPopupMenuSetSort  (wxCommandEvent& event)
{
    m_ColClicked = m_ColClickedFltr;
}

//--------------------------------------------------------------------------
// OnRClick: Handle right-click context menu
//--------------------------------------------------------------------------
void CUnitPaneFltr::OnRClick(wxListEvent& event)
{
    wxMenu  menu;

    menu.Append(menu_Popup_Filter  , wxT("Filter")        );

    if (GetSelectedItemCount()>1)
    {
        menu.Append(menu_Popup_IssueOrders     , wxT("Issue orders"));
        menu.Append(menu_Popup_UnitFlags       , wxT("Set custom flags")    );
        menu.Append(menu_Popup_AddToTracking   , wxT("Add to a tracking group"));
    }

    PopupMenu( &menu, event.GetPoint().x, event.GetPoint().y);
}

//--------------------------------------------------------------------------
// OnPopupMenuFilter: Show filter dialog
//--------------------------------------------------------------------------
void CUnitPaneFltr::OnPopupMenuFilter  (wxCommandEvent& event)
{
    CUnitFilterDlg dlg(m_pParent, m_sConfigSection.GetData());

    if (wxID_OK == dlg.ShowModal())
        Update(&dlg);
    else
    {
        CMapPane * pMapPane = (CMapPane* )gpApp->m_Panes[AH_PANE_MAP];
        pMapPane->RemoveRectangle();
        pMapPane->Refresh(FALSE);
    }
}

//--------------------------------------------------------------------------
// OnPopupMenuIssueOrders: Issue same orders to multiple units
//--------------------------------------------------------------------------
void CUnitPaneFltr::OnPopupMenuIssueOrders(wxCommandEvent& event)
{
    long           idx;
    CUnit        * pUnit;
    CEditPane    * pOrders;
    CGetTextDlg    dlg(this, "Order", "Orders for the selected units");
    CLongSortColl  LandIds;
    int            i;
    CLand        * pLand;
    CUnitPane    * pUnitPane;

    if (wxID_OK != dlg.ShowModal())
        return;
    dlg.m_Text.TrimRight(TRIM_ALL);
    if (dlg.m_Text.IsEmpty())
        return;

    pOrders = (CEditPane*)gpApp->m_Panes[AH_PANE_UNIT_COMMANDS];
    if (pOrders)
        pOrders->SaveModifications();

    // Add orders to all selected units
    idx   = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    while (idx>=0)
    {
        pUnit = GetUnit(idx);
        if (pUnit->IsOurs)
        {
            pUnit->Orders.TrimRight(TRIM_ALL);
            if (!pUnit->Orders.IsEmpty())
                pUnit->Orders << EOL_SCR;
            pUnit->Orders << dlg.m_Text;
            LandIds.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(pUnit->LandId)));
        }
        idx   = GetNextItem(idx, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    // Re-run orders for all affected lands
    if (LandIds.Count()>0)
    {
        gpApp->SetOrdersChanged(TRUE);
        for (i=0; i<LandIds.Count(); i++)
        {
            pLand = gpApp->m_pAtlantis->GetLand(static_cast<long>(reinterpret_cast<intptr_t>(LandIds.At(i))));
            gpApp->m_pAtlantis->RunOrders(pLand);
        }
        pUnitPane = (CUnitPane*)gpApp->m_Panes[AH_PANE_UNITS_HEX];
        if (pUnitPane)
            pUnitPane->Update(pUnitPane->m_pCurLand);
    }

    LandIds.DeleteAll();
}