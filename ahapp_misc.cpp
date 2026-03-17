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
// Displays an error message in the message frame
// Can be disabled via m_DisableErrs flag
//-------------------------------------------------------------------------

void CAhApp::ShowError(const char * msg, int msglen, BOOL ignore_disabled)
{
    CEditPane * p;

    // Check if errors are disabled and this message doesn't ignore that
    if (m_DisableErrs && !ignore_disabled)
       return;

    OpenMsgFrame();  // Ensure message frame is open
    m_MsgSrc.AddStr(msg, msglen);

    p = (CEditPane*)m_Panes[AH_PANE_MSG];
    if (p)
        p->SetSource(&m_MsgSrc, NULL);
}

//-------------------------------------------------------------------------
// Returns the currently selected unit based on m_SelUnitIdx
//-------------------------------------------------------------------------

CUnit * CAhApp::GetSelectedUnit()
{
    CUnit       * pUnit = NULL;
    CUnitPane   * pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];

    if (pUnitPane)
        pUnit = (CUnit*)pUnitPane->m_pUnits->At(m_SelUnitIdx);

    return pUnit;
}

//-------------------------------------------------------------------------
// Redraws movement tracks for the selected unit on the map
//-------------------------------------------------------------------------

void CAhApp::RedrawTracks()
{
    CUnit       * pUnit = GetSelectedUnit();
    CPlane      * pPlane;
    CMapPane    * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];

    if (!pMapPane)
        return;

    pPlane   = (CPlane*)m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);
    pMapPane->RedrawTracksForUnit(pPlane, pUnit, NULL, TRUE);
}

//-------------------------------------------------------------------------
// Forces a refresh of all panes
//-------------------------------------------------------------------------

void CAhApp::Redraw()
{
    int i;

    for (i=0; i<AH_PANE_COUNT; i++)
        if (m_Panes[i])
            m_Panes[i]->Refresh(FALSE);
}

//-------------------------------------------------------------------------
// Checks if application can be closed
// Saves flags and comments if needed, checks for unsaved orders
//-------------------------------------------------------------------------

BOOL CAhApp::CanCloseApp()
{
    SaveLandFlags();
    SaveUnitFlags();
    if (m_CommentsChanged)
        SaveComments();

    // Can close if:
    // - Discard changes flag is set, OR
    // - No orders changed, OR
    // - Successfully saved orders
    return ( m_DiscardChanges || !GetOrdersChanged() || ERR_OK==SaveOrders(TRUE));
}

//-------------------------------------------------------------------------
// Saves history of all hexes to file
// Used for backup/archive purposes
//-------------------------------------------------------------------------

int CAhApp::SaveHistory(const char * FNameOut)
{
    CLand            * pLand;
    CFileWriter        Dest;
    int                nl, np;
    CPlane           * pPlane;
    SAVE_HEX_OPTIONS   options;

    memset(&options, 0, sizeof(options));
    options.AlwaysSaveImmobStructs = TRUE;
    options.SaveResources          = TRUE;

    // Only save if we have planes and no parse errors
    if ( (m_pAtlantis->m_Planes.Count()>0) &&
         (0==m_pAtlantis->m_ParseErr)      &&
         Dest.Open(FNameOut)
       )
    {
        for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
            for (nl=0; nl<pPlane->Lands.Count(); nl++)
            {
                pLand    = (CLand*)pPlane->Lands.At(nl);
                m_pAtlantis->SaveOneHex(Dest, pLand, pPlane, &options);
            }
        }
        Dest.Close();
    }
    return ERR_OK;
}

//-------------------------------------------------------------------------
// Initializes redirection of stdout/stderr to files
// On MacOS, also handles working directory setup
//-------------------------------------------------------------------------

void CAhApp::StdRedirectInit()
{
#ifdef __WXMAC_OSX__
    // On MacOS, if started from /Applications, move to user's .alh directory
    char cwd[MAXPATHLEN];
    if((getcwd(cwd, MAXPATHLEN)) != NULL){
        if((strncmp(cwd, "/Applications", strlen("/Applications"))) == 0){
            const char *home = getenv("HOME");
            if(home != NULL){
                if(0 == chdir(home)){
                    mkdir(".alh", 0750);
                    if(0 != chdir(".alh"))
                        chdir("/Applications");
                }
            }
        }
    }
#endif
    // Redirect standard output streams to files
    freopen("ah.stdout", "w", stdout);
    freopen("ah.stderr", "w", stderr);
    m_nStdoutLastPos = 0;
    m_nStderrLastPos = 0;
}

//-------------------------------------------------------------------------
// Cleanup for stdout/stderr redirection (currently empty)
//-------------------------------------------------------------------------

void CAhApp::StdRedirectDone()
{
}

//-------------------------------------------------------------------------
// Reads more data from redirected output files
// Used to capture output from Python scripts
//-------------------------------------------------------------------------

void CAhApp::StdRedirectReadMore(BOOL FromStdout, CStr & sData)
{
    FILE       * f;
    int        * pCurPos;
    char         buf[1024];
    int          n;

    sData.Empty();
    if (FromStdout)
    {
        fflush(stdout);
        pCurPos  =  &m_nStdoutLastPos;
        f        = fopen("ah.stdout", "rb");
    }
    else
    {
        fflush(stderr);
        pCurPos  =  &m_nStderrLastPos;
        f        = fopen("ah.stderr", "rb");
    }

    if (f)
    {
        // Read from last position
        fseek(f, *pCurPos, SEEK_SET);
        do
        {
            n = fread(buf, 1, sizeof(buf), f);
            if (n>0)
                sData.AddBuf(buf, n);
        } while (n>0);
        *pCurPos = ftell(f);
        fclose(f);
    }
}

//-------------------------------------------------------------------------
// Checks both stdout and stderr for new output and displays it
//-------------------------------------------------------------------------

void CAhApp::CheckRedirectedOutputFiles()
{
    CStr S;

    gpApp->StdRedirectReadMore(FALSE, S);
    if (!S.IsEmpty())
        ShowError(S.GetData(), S.GetLength(), TRUE);
    gpApp->StdRedirectReadMore(TRUE, S);
    if (!S.IsEmpty())
        ShowError(S.GetData(), S.GetLength(), TRUE);
}

//-------------------------------------------------------------------------
// Initializes movement modes from configuration
// Also handles upgrade to newer versions with more movement modes
//-------------------------------------------------------------------------

void CAhApp::InitMoveModes()
{
    const char * p;
    CStr         S;
    int          n;
    BOOL         Update = FALSE;

    p     = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMENTS));
    while (p && *p)
    {
        p = SkipSpaces(S.GetToken(p, ','));
        m_MoveModes.Insert(strdup(S.GetData()));
    }

    // Check if we need to add default movement modes (for version upgrade)
    p = SZ_DEFAULT_MOVEMENT_MODE;
    n = 0;
    while (p && *p)
    {
        p = SkipSpaces(S.GetToken(p, ','));
        n++;

        if (n > m_MoveModes.Count())
        {
            m_MoveModes.Insert(strdup(S.GetData()));
            Update = TRUE;
        }
    }
    
    // Save updated modes back to config if changed
    if (Update)
    {
        S.Empty();
        for (n=0; n<m_MoveModes.Count(); n++)
        {
            if (n>0)
                S << ',';
            S << (const char *)m_MoveModes.At(n);
        }
        SetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMENTS, S.GetData());
    }
}

//-------------------------------------------------------------------------
// Initializes movement speeds from configuration
// Sets default values if not configured
//-------------------------------------------------------------------------

void CAhApp::InitMovementSpeed()
{
    int value = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMEMENT_SPEED_WALK));
    if (value == 0)
    {
        value = 2;  // Default walking speed
        SetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMEMENT_SPEED_WALK, value);
    }
    RoutePlanner::SpeedWalk = value;

    value = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMEMENT_SPEED_RIDE));
    if (value == 0)
    {
        value = 4;  // Default riding speed
        SetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMEMENT_SPEED_RIDE, value);
    }
    RoutePlanner::SpeedRide = value;

    value = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMEMENT_SPEED_FLY));
    if (value == 0)
    {
        value = 6;  // Default flying speed
        SetConfig(SZ_SECT_COMMON, SZ_KEY_MOVEMEMENT_SPEED_FLY, value);
    }
    RoutePlanner::SpeedFly = value;
}

//-------------------------------------------------------------------------
// Loads terrain movement cost configuration into the Atlantis parser
//-------------------------------------------------------------------------

void CAhApp::LoadTerrainCostConfig()
{
    const char * szName, * szValue;
    int sectidx = GetSectionFirst(SZ_SECT_TERRAIN_COST, szName, szValue);
    while (sectidx >= 0)
    {
        if (szValue && *szValue)
        {
            m_pAtlantis->TerrainMovementCost[wxString::FromUTF8(szName)] = atoi(szValue);
        }
        sectidx = GetSectionNext(sectidx, SZ_SECT_TERRAIN_COST, szName, szValue);
    }
}

//-------------------------------------------------------------------------
// Selects the next unit in the unit list
//-------------------------------------------------------------------------

void CAhApp::SelectNextUnit()
{
    if (m_Panes[AH_PANE_UNITS_HEX])
        ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX])->SelectNextUnit();
}

//-------------------------------------------------------------------------
// Selects the previous unit in the unit list
//-------------------------------------------------------------------------

void CAhApp::SelectPrevUnit()
{
    if (m_Panes[AH_PANE_UNITS_HEX])
        ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX])->SelectPrevUnit();
}

//-------------------------------------------------------------------------
// Sets focus to the units pane
//-------------------------------------------------------------------------

void CAhApp::SelectUnitsPane()
{
    if (m_Panes[AH_PANE_UNITS_HEX])
        ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX])->SetFocus();
}

//-------------------------------------------------------------------------
// Sets focus to the orders pane
//-------------------------------------------------------------------------

void CAhApp::SelectOrdersPane()
{
    if (m_Panes[AH_PANE_UNIT_COMMANDS])
        ((CUnitPane*)m_Panes[AH_PANE_UNIT_COMMANDS])->SetFocus();
}

//-------------------------------------------------------------------------
// Placeholder for viewing moved units functionality
//-------------------------------------------------------------------------

void CAhApp::ViewMovedUnits()
{
    // Implementation for viewing moved units
}

//-------------------------------------------------------------------------
// Opens dialog for editing list columns
// Saves current layout before editing
//-------------------------------------------------------------------------

void CAhApp::EditListColumns(int command)
{
    CMapFrame   * pMapFrame  = (CMapFrame *)m_Frames[AH_FRAME_MAP];
    CUnitPane   * pUnitPane  = NULL;
    const char  * szConfigSectionHdr;

    const char * szKey = NULL;
    switch (command)
    {
    case menu_ListColUnits:
        szKey = SZ_KEY_LIS_COL_UNITS_HEX;
        pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];
        break;

    case menu_ListColUnitsFltr:
        szKey = SZ_KEY_LIS_COL_UNITS_FILTER;
        pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_FILTER];
        break;

    default:
        return;
    }
    
    // Save current layout before editing
    if (pUnitPane)
        pUnitPane->SaveUnitListHdr();

    CListHeaderEditDlg dlg(pMapFrame, szKey);

    if (wxID_OK == dlg.ShowModal())
    {
        szConfigSectionHdr = GetListColSection(SZ_SECT_LIST_COL_UNIT, szKey);
        if (pUnitPane)
            pUnitPane->ReloadHdr(szConfigSectionHdr);  // Reload new layout
    }
}

//-------------------------------------------------------------------------
// Gets the list column section name for a given key
// Returns current set or first available
//-------------------------------------------------------------------------

const char * CAhApp::GetListColSection(const char * sectprefix, const char * key)
{
    const char * sect;

    sect = GetConfig(SZ_SECT_LIST_COL_CURRENT, key);
    if (!sect || !*sect)
        sect  = GetNextSectionName(CONFIG_FILE_CONFIG, sectprefix);

    return sect;
}

//-------------------------------------------------------------------------
// Checks for duplicate month-long orders and missing month-long orders
// Can output to message box or filtered units pane
//-------------------------------------------------------------------------

void CAhApp::CheckMonthLongOrders()
{
    static const char dup_ord_msg[] = ";--- Duplicate month long orders";
    int                  x;
    CUnit              * pUnit;
    const char         * src;
    const char         * dupord;
    const char         * p;
    char                 ch;
    CStr                 Line;
    CStr                 Ord;
    const char         * order;
    BOOL                 IsNew;
    BOOL                 Found;
    CStr                 Errors(128);
    CStr                 S(64);
    CStr                 FoundOrder;
    CStringSortColl      MonthLongOrders;
    CStringSortColl      MonthLongDup;
    long                 men;
    EValueType           type;
    CUnitPaneFltr      * pUnitPaneF = NULL;
    int                  errcount = 0;
    int                  turnlvl;
    CBaseColl            Hexes(64);
    int                  nl, unitidx;
    CLand              * pLand;
    CMapPane           * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];

    // Load list of month-long orders
    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_ORD_MONTH_LONG));
    while (p && *p)
    {
        p = SkipSpaces(S.GetToken(p, ','));
        if (!S.IsEmpty())
            MonthLongOrders.Insert(strdup(S.GetData()));
    }

    // Load list of duplicatable orders (can appear multiple times)
    p = SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_ORD_DUPLICATABLE));
    while (p && *p)
    {
        p = SkipSpaces(S.GetToken(p, ','));
        if (!S.IsEmpty())
            MonthLongDup.Insert(strdup(S.GetData()));
    }

    // Decide output destination
    if (1==atol(SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_OUTPUT_LIST))))
    {
        // Output will go into the unit filter window
        OpenUnitFrameFltr(FALSE);
        pUnitPaneF = (CUnitPaneFltr*)m_Panes [AH_PANE_UNITS_FILTER];
    }

    if (pUnitPaneF)
        pUnitPaneF->InsertUnitInit();

    // Check all hexes
    pMapPane->GetSelectedOrAllHexes(Hexes, FALSE);
    for (nl=0; nl<Hexes.Count(); nl++)
    {
        pLand = (CLand*)Hexes.At(nl);
        for (unitidx=0; unitidx<pLand->Units.Count(); unitidx++)
        {
            pUnit    = (CUnit*)pLand->Units.At(unitidx);

            if (!pUnit->IsOurs)
                continue;
                
            src   = pUnit->Orders.GetData();
            IsNew = FALSE;
            Found = FALSE;
            turnlvl = 0;
            
            // Parse each order line
            while (src && *src)
            {
                dupord = src;
                src    = Line.GetToken(src, '\n', TRIM_ALL);
                Ord.GetToken(SkipSpaces(Line.GetData()), " \t", ch, TRIM_ALL);
                order = Ord.GetData();
                if ('@'==*order)
                    order++;  // Skip repeat flag
                    
                // Track FORM/END blocks and TURN nesting
                if (0==SafeCmp("FORM", order))
                    IsNew = TRUE;
                else if (0==SafeCmp("END", order))
                    IsNew = FALSE;
                else if (0==SafeCmp("TURN", order))
                    turnlvl++;
                else if (0==SafeCmp("ENDTURN", order))
                    turnlvl--;
                else if (!IsNew && 0==turnlvl && MonthLongOrders.Search((void*)order, x) )
                {
                    if (Found)
                    {
                        // Check if this order can be duplicated
                        if (0==stricmp(order, FoundOrder.GetData()) &&
                            MonthLongDup.Search((void*)order, x))
                            continue; // it is an order which can be duplicated

                        errcount++;
                        if (pUnitPaneF)
                        {
                            int newpos;

                            pUnitPaneF->InsertUnit(pUnit);
                            S = dup_ord_msg;
                            S << EOL_SCR;
                            newpos = dupord - pUnit->Orders.GetData() + S.GetLength();
                            pUnit->Orders.InsBuf(S.GetData(), dupord - pUnit->Orders.GetData(), S.GetLength());
                            src = &pUnit->Orders.GetData()[newpos];
                        }
                        else
                        {
                            S.Format("Unit % 5d Error : Duplicate month long orders - %s", pUnit->Id, Line.GetData());
                            Errors << S << EOL_SCR;
                        }
                        break;
                    }
                    Found      = TRUE;
                    FoundOrder = order;
                }
            }
            
            // Check if unit has any month-long order at all
            if (!Found)
            {
                if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) ||
                    (eLong==type && 0==men))
                    continue; // no men - no orders is ok

                errcount++;
                if (pUnitPaneF)
                {
                    pUnitPaneF->InsertUnit(pUnit);
                }
                else
                {
                    S.Format("Unit % 5d Warning : No month long orders", pUnit->Id);
                    Errors << S << EOL_SCR;
                }
            }
        }
    }

    Hexes.DeleteAll();

    if (pUnitPaneF)
        pUnitPaneF->InsertUnitDone();

    if (!pUnitPaneF && errcount>0)
        ShowError(Errors.GetData(), Errors.GetLength(), TRUE);

    if (0==errcount)
        wxMessageBox(wxT("No problems found."), wxT("Order checking"), wxOK | wxCENTRE, m_Frames[AH_FRAME_MAP]);

    MonthLongOrders.FreeAll();
    MonthLongDup.FreeAll();
}

//-------------------------------------------------------------------------
// Checks production resource availability for all units
//-------------------------------------------------------------------------

void CAhApp::CheckProduction()
{
    int    n, i, x;
    CLand  * pLand;
    CPlane * pPlane;
    CUnit  * pUnit;
    CStr     Error(64), S(32);

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            for (x=0; x<pLand->Units.Count(); x++)
            {
                pUnit = (CUnit*)pLand->Units.At(x);
                if (!m_pAtlantis->CheckResourcesForProduction(pUnit, pLand, S))
                    Error << "Unit " << pUnit->Id << " " << S << EOL_SCR;
            }
        }
    }

    S.Empty();
    if (Error.IsEmpty())
        wxMessageBox(wxT("No problem with resources for production detected"));
    else
    {
        S << "The following problems were detected:" << EOL_SCR << EOL_SCR << Error;
        ShowError(S.GetData(), S.GetLength(), TRUE);
    }
}

//--------------------------------------------------------------------------
// Checks sailing-related problems (overloaded or underpowered ships)
//--------------------------------------------------------------------------

void CAhApp::CheckSailing()
{
    int    n, i, x;
    CLand  * pLand;
    CPlane * pPlane;
    CStruct* pStruct;
    CStr     Error(64), S(32), sCoord(32);

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
            for (x=0; x<pLand->Structs.Count(); x++)
            {
                pStruct = (CStruct*)pLand->Structs.At(x);
                if ((pStruct->Attr & SA_MOBILE) && pStruct->SailingPower > 0)
                {
                    if (pStruct->Load > pStruct->MaxLoad)
                        Error << pLand->TerrainType << " (" << sCoord << ") - Ship " << pStruct->Id << " is overloaded by " << (pStruct->Load - pStruct->MaxLoad) << "." << EOL_SCR;
                    if (pStruct->SailingPower < pStruct->MinSailingPower)
                        Error << pLand->TerrainType << " (" << sCoord << ") - Ship " << pStruct->Id << " is underpowered by " << (pStruct->MinSailingPower - pStruct->SailingPower) << "." << EOL_SCR;
                }
            }
        }
    }

    S.Empty();
    if (Error.IsEmpty())
        wxMessageBox(wxT("No problems with sailing detected"));
    else
    {
        S << "The following problems were detected:" << EOL_SCR << EOL_SCR << Error;
        ShowError(S.GetData(), S.GetLength(), TRUE);
    }
}

//--------------------------------------------------------------------------
// Checks tax details for a single land
// Calculates undertax/ overtax per faction
//--------------------------------------------------------------------------

void CAhApp::CheckTaxDetails  (CLand  * pLand, CTaxProdDetailsCollByFaction & TaxDetails)
{
    CUnit           * pUnit;
    EValueType        type;
    long              men;
    CStr              sCoord;
    CTaxProdDetails * pDetail;
    CTaxProdDetails   Dummy;
    int               idx;
    CTaxProdDetailsCollByFaction Factions;
    wxString          OneLine;

    // Collect taxing units by faction
    for (int x=0; x<pLand->Units.Count(); x++)
    {
        pUnit = (CUnit*)pLand->Units.At(x);
        if (pUnit->Flags & UNIT_FLAG_TAXING)
        {
            Dummy.FactionId = pUnit->FactionId;
            if (TaxDetails.Search(&Dummy, idx))
                pDetail = (CTaxProdDetails*)TaxDetails.At(idx);
            else
            {
                pDetail = new CTaxProdDetails;
                pDetail->FactionId = pUnit->FactionId;
                TaxDetails.Insert(pDetail);
            }
            if (Factions.Insert(pDetail))
            {
                pDetail->amount = pLand->Taxable;
                pDetail->HexCount++;
            }
            if (pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) && eLong==type)
                pDetail->amount -= men*atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_TAX_PER_TAXER));
        }
    }

    // Generate formatted output for each faction
    for (int iFac=0; iFac<Factions.Count(); iFac++)
    {
        pDetail = (CTaxProdDetails*)Factions.At(iFac);
        OneLine.Empty();

        m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
        OneLine << wxString::FromUTF8(pLand->TerrainType.GetData()) << wxT(" (") << wxString::FromUTF8(sCoord.GetData()) << wxT(") ");
        if (!pLand->CityName.IsEmpty())
            OneLine << wxString::FromUTF8(pLand->CityName.GetData()) << wxT(" ");

        // Pad to consistent width
        wxCoord x, y;
        wxClientDC myDC(m_Frames[AH_FRAME_MAP]);
        do
        {
            OneLine.Append(wxT(" "));
            myDC.GetTextExtent(OneLine.GetData(), &x, &y, NULL, NULL, (m_Fonts[FONT_ERR_DLG]));
        }
        while (x < 245);

        if (pDetail->amount > 0)
            OneLine << wxT("is undertaxed by ") << pDetail->amount << wxT(" silv (") << 100 * (pLand->Taxable - pDetail->amount) / (pLand->Taxable+1) << wxT("% of $") << pLand->Taxable << wxT(").") << wxString::FromUTF8(EOL_SCR);
        else if (pDetail->amount<0)
            OneLine << wxT("is overtaxed  by ") << (-pDetail->amount) << wxT(" silv (") << 100 * (pLand->Taxable - pDetail->amount) / (pLand->Taxable+1) << wxT("% of $") << pLand->Taxable << wxT(").") << wxString::FromUTF8(EOL_SCR);
        else
            OneLine << wxString::FromUTF8(EOL_SCR);

        pDetail->Details << OneLine.ToUTF8();
    }

    Factions.DeleteAll();
}

//--------------------------------------------------------------------------
// Checks trade/production details for a single land
// Calculates production surplus/deficit per faction per product
//--------------------------------------------------------------------------

void CAhApp::CheckTradeDetails(CLand  * pLand, CTaxProdDetailsCollByFaction & TradeDetails)
{
    int             x, k;
    CUnit         * pUnit;
    EValueType      type;
    long            men, lvl, canproduce;
    CStr            sCoord, Skill;
    CProduct      * pProd;
    TProdDetails    details;
    CTaxProdDetails * pFactionInfo = NULL;
    CTaxProdDetails   Dummy;
    int               idx;
    CTaxProdDetailsCollByFaction Factions;
    CTaxProdDetailsCollByFaction AllFactions;
    wxString        OneLine;

    // Check each product in the land
    for (k=0; k<pLand->Products.Count(); k++)
    {
        pProd = (CProduct*)pLand->Products.At(k);
        if (0==pProd->Amount)
            continue;
            
        GetProdDetails(pProd->ShortName.GetData(), details);
        Skill.Empty();
        Skill << details.skillname << PRP_SKILL_POSTFIX;

        // Check each unit that might be producing this product
        for (x=0; x<pLand->Units.Count(); x++)
        {
            pUnit = (CUnit*)pLand->Units.At(x);
            if (pUnit->Flags & UNIT_FLAG_PRODUCING)
            {
                Dummy.FactionId = pUnit->FactionId;
                if (TradeDetails.Search(&Dummy, idx))
                    pFactionInfo = (CTaxProdDetails*)TradeDetails.At(idx);
                else
                {
                    pFactionInfo = new CTaxProdDetails;
                    pFactionInfo->FactionId = pUnit->FactionId;
                    TradeDetails.Insert(pFactionInfo);
                }
                if (Factions.Insert(pFactionInfo))
                    pFactionInfo->amount = pProd->Amount;
                if (AllFactions.Insert(pFactionInfo) )
                    pFactionInfo->HexCount++;

                // If this unit is producing this specific product
                if ( 0==stricmp(pUnit->ProducingItem.GetData(), pProd->ShortName.GetData()))
                {
                    if (!pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) || eLong!=type)
                        continue;

                    // Check skill level
                    if (!pUnit->GetProperty(Skill.GetData(), type, (const void *&)lvl, eNormal) || (eLong!=type) )
                        continue;

                    // Check tool availability
                    long tool = 0;
                    if (!details.toolname.IsEmpty())
                        if (!pUnit->GetProperty(details.toolname.GetData(), type, (const void *&)tool, eNormal) || eLong!=type )
                            tool = 0;
                    if (tool > men)
                        tool = men;

                    // Calculate production capacity
                    canproduce = (long)((((double)men)*lvl + tool*details.toolhelp) / details.months);
                    pFactionInfo->amount -= canproduce;
                }
            }
        }

        // Generate output for each faction
        for (int iFac=0; iFac<Factions.Count(); ++iFac)
        {
            pFactionInfo = (CTaxProdDetails*)Factions.At(iFac);

            m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
            OneLine.Empty();
            OneLine << wxString::FromUTF8(pLand->TerrainType.GetData()) << wxT(" (") << wxString::FromUTF8(sCoord.GetData()) << wxT(") ");
            if (!pLand->CityName.IsEmpty())
                OneLine << wxString::FromUTF8(pLand->CityName.GetData()) << wxT(" ");

            const int nettoProduction = -pFactionInfo->amount;
            wxString prodBalance = wxString::Format("%d", nettoProduction);
            if (nettoProduction >= 0 && nettoProduction <= 3)
                prodBalance = "=";

            // Measure and pad to consistent width
            wxCoord x, y;
            wxClientDC myDC(m_Frames[AH_FRAME_MAP]);
            myDC.GetTextExtent(prodBalance, &x, &y, NULL, NULL, (m_Fonts[FONT_ERR_DLG]));
            const int prodBalanceWidth = x;

            do
            {
                OneLine.Append(wxT(" "));
                myDC.GetTextExtent(OneLine.GetData(), &x, &y, NULL, NULL, (m_Fonts[FONT_ERR_DLG]));
            }
            while (x < 245 - prodBalanceWidth);

            OneLine << prodBalance << " " << wxString::FromUTF8(pProd->ShortName.GetData());

            do
            {
                OneLine.Append(wxT(" "));
                myDC.GetTextExtent(OneLine, &x, &y, NULL, NULL, (m_Fonts[FONT_ERR_DLG]));
            }
            while (x < 295);

            const int percentage = 100 * (pProd->Amount - pFactionInfo->amount) / (pProd->Amount);
            const int myProductionCapacity = pProd->Amount - pFactionInfo->amount;

            OneLine << " (" << percentage << "%), " << myProductionCapacity << wxT("/") << pProd->Amount << wxT(".") << wxString::FromUTF8(EOL_SCR);
            pFactionInfo->Details << OneLine.ToUTF8();
        }
        Factions.DeleteAll();
    }
    // Add empty line between regions
    if (pFactionInfo)
        pFactionInfo->Details << wxString::FromUTF8(EOL_SCR);
    AllFactions.DeleteAll();
}

//-------------------------------------------------------------------------
// Main tax and trade checking function
// Aggregates results from all lands and displays report
//-------------------------------------------------------------------------

void CAhApp::CheckTaxTrade()
{
    CStr                sTax(64);
    CStr                sTrade(64);
    CStr                Report(64), S(64);
    CStr                Details(256);
    int                 n, i;
    CLand             * pLand;
    CPlane            * pPlane;
    CTaxProdDetailsCollByFaction  Taxes;
    CTaxProdDetailsCollByFaction  Trades;
    CTaxProdDetails              *pDetails;

    // Check all lands
    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);

            if (pLand->Flags & LAND_TAX_NEXT)
                CheckTaxDetails(pLand, Taxes);

            if (pLand->Flags & LAND_TRADE_NEXT)
                CheckTradeDetails(pLand, Trades);
        }
    }
    
    // Build report
    Report.Empty();
    for (i=0; i<Taxes.Count(); i++)
    {
        pDetails = (CTaxProdDetails*)Taxes.At(i);
        Report << "Faction " << pDetails->FactionId << " : " << pDetails->HexCount << " TAX regions"   << EOL_SCR
               << pDetails->Details << EOL_SCR  << EOL_SCR;
    }
    for (i=0; i<Trades.Count(); i++)
    {
        pDetails = (CTaxProdDetails*)Trades.At(i);
        Report << "Faction " << pDetails->FactionId << " : " << pDetails->HexCount << " TRADE regions"   << EOL_SCR
               << pDetails->Details << EOL_SCR  << EOL_SCR;
    }

    Taxes.FreeAll();
    Trades.FreeAll();

    ShowError(Report.GetData()      , Report.GetLength()      , TRUE);
}