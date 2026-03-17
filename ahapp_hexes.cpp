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
// Updates the hex description pane with information about the selected land
// Combines terrain description, products, structures, flags, events and exits
//-------------------------------------------------------------------------

void CAhApp::UpdateHexEditPane(CLand * pLand)
{
    CStruct     * pStruct;
    CEditPane   * pEditPane;
    int           i;
    BOOL          FlagsEmpty = TRUE;

    m_HexDescrSrc.Empty();

    pEditPane = (CEditPane*)m_Panes[AH_PANE_MAP_DESCR];
    if (pEditPane)
    {
        if (pLand)
        {
            // Start with the land's description
            m_HexDescrSrc << pLand->Description;

            m_HexDescrSrc << EOL_SCR;
            // Add information about products available in this hex
            m_pAtlantis->ComposeProductsLine(pLand, EOL_SCR, m_HexDescrSrc);

            // Add structures present in the hex
            if (pLand->Structs.Count()>0)
            {
                m_HexDescrSrc.TrimRight(TRIM_ALL);
                m_HexDescrSrc << EOL_SCR << "-----------" << EOL_SCR;
                for (i=0; i<pLand->Structs.Count(); i++)
                {
                    pStruct = (CStruct*)pLand->Structs.At(i);
                    m_HexDescrSrc << pStruct->Description;
                    m_HexDescrSrc.TrimRight(TRIM_ALL);
                    // For mobile structures (ships), add load and power info
                    if (pStruct->Attr & SA_MOBILE)
                        m_HexDescrSrc << " Load: " << pStruct->Load << ", Power: " << pStruct->SailingPower << ".";
                    m_HexDescrSrc << EOL_SCR;
                }
            }

            // Check if any flags are set on this hex
            for (i=0; i<LAND_FLAG_COUNT; i++)
                if (!pLand->FlagText[i].IsEmpty())
                {
                    FlagsEmpty = FALSE;
                    break;
                }

            // Add user flags if any
            if (!FlagsEmpty)
            {
                m_HexDescrSrc.TrimRight(TRIM_ALL);
                m_HexDescrSrc << EOL_SCR << "-----------";

                for (i=0; i<LAND_FLAG_COUNT; i++)
                    if (!pLand->FlagText[i].IsEmpty())
                        m_HexDescrSrc << EOL_SCR << pLand->FlagText[i];
            }

            // Add events that occurred in this hex
            if (!pLand->Events.IsEmpty() &&
                 0 != stricmp(SkipSpaces(pLand->Events.GetData()), "none")
               )
                m_HexDescrSrc << EOL_SCR << "Events:" << EOL_SCR << pLand->Events << EOL_SCR;
            
            // Add information about exits to neighboring hexes
            m_HexDescrSrc << EOL_SCR << "Exits:"  << EOL_SCR << pLand->Exits;
        }
        pEditPane->SetSource(&m_HexDescrSrc, NULL);
    }
}

//-------------------------------------------------------------------------
// Updates the unit list pane with units from the specified land
//-------------------------------------------------------------------------

void CAhApp::UpdateHexUnitList(CLand * pLand)
{
    CUnitPane   * pUnitPane = (CUnitPane*)m_Panes[AH_PANE_UNITS_HEX];

    if (pUnitPane)
        pUnitPane->Update(pLand);
}

//-------------------------------------------------------------------------
// Updates edge structures between adjacent hexes
// Determines water boundaries, coastlines, and propagates edge structures
//-------------------------------------------------------------------------

void CAhApp::UpdateEdgeStructs()
{
    int          i, n, k;
    int          d, adj_dir;
    int          x, y, z;
    int          adj_index;
    CPlane     * pPlane;
    CLand      * pLand, * adj_land;
    CStruct    * pEdge;

    for (n=0; n<m_pAtlantis->m_Planes.Count(); n++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(n);
        for (i=0; i<pPlane->Lands.Count(); i++)
        {
            pLand = (CLand*)pPlane->Lands.At(i);
            if(!pLand) continue;
            
            // Set the water flag if terrain is in water terrains list
            if(m_WaterTerrainNames.Search((void *) pLand->TerrainType.ToLower(), k))
            {
                pLand->Flags |= LAND_IS_WATER;
            }
            
            // Process all 6 directions
            for(d=0; d<6; d++)
            {
                adj_dir = (d%6)-3;
                if(adj_dir < 0) adj_dir += 6;
                LandIdToCoord(pLand->Id,x,y,z);
                m_pAtlantis->ExtrapolateLandCoord(x,y,z,d);

                CBaseObject Dummy;
                Dummy.Id = LandCoordToId(x, y, z);
                
                // Check if adjacent land exists
                if (pPlane->Lands.Search(&Dummy, adj_index))
                {
                    adj_land = (CLand *) pPlane->Lands.At(adj_index);
                    if(adj_land)
                    {
                        // Propagate edge structures from current land to adjacent
                        if((pLand->Flags&LAND_IS_CURRENT) && !(adj_land->Flags&LAND_IS_CURRENT))
                        {
                            adj_land->RemoveEdgeStructs(adj_dir);
                            for(k=pLand->EdgeStructs.Count()-1; k>=0; k--)
                            {
                                pEdge = (CStruct*) pLand->EdgeStructs.At(k);
                                if((pEdge != NULL) && (pEdge->Location == d))
                                    adj_land->AddNewEdgeStruct(pEdge->Kind.GetData(), adj_dir);
                            }
                        }
                        
                        // Set coastline bits where land meets water
                        if(m_WaterTerrainNames.Search((void *) adj_land->TerrainType.ToLower(), k))
                        {
                            if(!(pLand->Flags & LAND_IS_WATER))
                                adj_land->CoastBits |= ExitFlags[adj_dir];
                        }
                        else if(pLand->Flags&LAND_IS_WATER)
                        {
                            pLand->CoastBits |= ExitFlags[d];
                        }
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------------------
// Finds all units that are moving into the specified hex
// Checks movement paths of all units on all planes
//-------------------------------------------------------------------------

void CAhApp::GetUnitsMovingIntoHex(long HexId, CBaseColl& FoundUnits) const
{
    CLand* pLand;
    CUnit* pUnit;
    int              nl, nu, np;
    int             unitHexId;

    for (np = 0; np < m_pAtlantis->m_Planes.Count(); np++)
    {
        CPlane* pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
        if (pPlane)
        {
            for (nl = 0; nl < pPlane->Lands.Count(); nl++)
            {
                pLand = (CLand*)pPlane->Lands.At(nl);
                for (nu = 0; nu < pLand->Units.Count(); nu++)
                {
                    pUnit = (CUnit*)pLand->Units.At(nu);
                    // Check if unit has a movement path
                    if (pUnit->pMovement && pUnit->pMovement->Count() > 0)
                    {
                        int lastIndex = pUnit->pMovement->Count() - 1;
                        unitHexId = static_cast<long>(reinterpret_cast<intptr_t>(pUnit->pMovement->At(lastIndex)));

                        // If the last hex in movement path matches target, unit is moving here
                        if (HexId == unitHexId)
                            FoundUnits.Insert(pUnit);
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------------------
// Displays units moving into the current hex
// Can show either in a dialog box or in the filtered units pane
//-------------------------------------------------------------------------

void CAhApp::ShowUnitsMovingIntoHex(long CurHexId, CPlane * pCurPlane)
{
    CUnit          * pUnit;
    int              i;
    CUnitPaneFltr  * pUnitPaneF = NULL;
    CStr             UnitText(128), S(16);
    CBaseColl        FoundUnits;

    GetUnitsMovingIntoHex(CurHexId, FoundUnits);

    if (FoundUnits.Count() > 0)
    {
        // Check if output should go to filter window or message box
        if (1==atol(SkipSpaces(GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_OUTPUT_LIST))))
        {
            // Output will go into the unit filter window
            OpenUnitFrameFltr(FALSE);
            pUnitPaneF = (CUnitPaneFltr*)m_Panes [AH_PANE_UNITS_FILTER];
            pUnitPaneF->InsertUnitInit();
        }

        // Process each found unit
        for (i=0; i<FoundUnits.Count(); i++)
        {
            pUnit = (CUnit*)FoundUnits.At(i);
            if (pUnitPaneF)
                pUnitPaneF->InsertUnit(pUnit);  // Add to filter pane
            else
            {
                S.Format("Unit % 5d", pUnit->Id);
                UnitText << S << EOL_SCR;        // Add to text buffer
            }
        }

        if (pUnitPaneF)
            pUnitPaneF->InsertUnitDone();        // Finalize batch insert
        else
            ShowError(UnitText.GetData(), UnitText.GetLength(), TRUE);
    }
    else
        wxMessageBox(wxT("Found no units moving into the current hex."), wxT("Units moving"), wxOK | wxCENTRE, m_Frames[AH_FRAME_MAP]);

    FoundUnits.DeleteAll();
}

//-------------------------------------------------------------------------
// Shows detailed financial analysis for a land
// Calculates tax income, work income, maintenance costs, and balances per faction
//-------------------------------------------------------------------------

void CAhApp::ShowLandFinancial(CLand * pCurLand)
{
    CUnit            * pUnit;
    int                idx, factidx;
    long               CurFaction;
    long               SilvOrg = 0;
    long               SilvRes = 0;
    long               TaxOur  = 0;
    long               TaxTheir= 0;
    long               WorkOur  = 0;
    long               WorkTheir= 0;
    long               Maintain = 0;
    long               men;
    long               MovedOut = 0;
    long               Workers  = 0;
    EValueType         type;
    const void       * value;
    CBaseObject        Report;
    CBaseCollByName    coll;
    CStr               sCoord;
    CLongSortColl      Factions;
    long               TaxPerTaxer;
    long               UpkeepLeader;
    long               UpkeepPeasant;
    const char       * leadership;

    if (!pCurLand)
        return;

    // Get economic parameters from configuration
    TaxPerTaxer = atol(GetConfig(SZ_SECT_COMMON, SZ_KEY_TAX_PER_TAXER));
    UpkeepLeader = atol(GetConfig(SZ_SECT_COMMON, SZ_UPKEEP_LEADER));
    UpkeepPeasant = atol(GetConfig(SZ_SECT_COMMON, SZ_UPKEEP_PEASANT));

    // Collect unique faction IDs present in this land
    for (idx=0; idx<pCurLand->Units.Count(); idx++)
    {
        pUnit    = (CUnit*)pCurLand->Units.At(idx);
        if (pUnit->FactionId != 0)
            Factions.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(pUnit->FactionId)));
    }

    // Analyze each faction separately
    for (factidx=0; factidx<Factions.Count(); factidx++)
    {
        CurFaction = static_cast<long>(reinterpret_cast<intptr_t>(Factions.At(factidx)));
        SilvOrg  = 0;
        SilvRes  = 0;
        TaxOur   = 0;
        TaxTheir = 0;
        WorkOur  = 0;
        WorkTheir= 0;
        Maintain = 0;
        MovedOut = 0;
        Workers  = 0;

        // Scan all units in the land
        for (idx=0; idx<pCurLand->Units.Count(); idx++)
        {
            pUnit    = (CUnit*)pCurLand->Units.At(idx);
            if (!pUnit->IsOurs)  // Only consider our units for detailed tracking
                continue;
            if (pUnit->FactionId == CurFaction)
            {
                // Track silver changes (original vs current)
                if (pUnit->GetProperty(PRP_SILVER, type, value, eOriginal) && eLong==type)
                    SilvOrg += static_cast<long>(reinterpret_cast<intptr_t>(value));
                if (pUnit->GetProperty(PRP_SILVER, type, value, eNormal) && eLong==type)
                {
                    SilvRes += static_cast<long>(reinterpret_cast<intptr_t>(value));

                    // Track silver moved out with units
                    if (pUnit->pMovement && pUnit->pMovement->Count()>0 && (static_cast<long>(reinterpret_cast<intptr_t>(value)) > 0))
                        MovedOut += static_cast<long>(reinterpret_cast<intptr_t>(value));
                }
            }

            // Count men for various activities
            if (pUnit->GetProperty(PRP_MEN, type, (const void *&)men, eNormal) && eLong==type)
            {
                // Tax collection
                if (pUnit->Flags & UNIT_FLAG_TAXING)
                    if (pUnit->FactionId == CurFaction)
                        TaxOur += men*TaxPerTaxer;
                    else
                        TaxTheir += men*TaxPerTaxer;

                // Work/entertainment
                if (pUnit->Flags & UNIT_FLAG_WORKING)
                    if (pUnit->FactionId == CurFaction)
                    {
                        Workers += men;
                        WorkOur +=  (long)(men*pCurLand->Wages);
                    }
                    else
                        WorkTheir +=  (long)(men*pCurLand->Wages);

                // Maintenance costs (only for units that are staying, not moving)
                if (pUnit->FactionId == CurFaction && (!pUnit->pMovement || pUnit->pMovement->Count()==0))
                    if (pUnit->GetProperty(PRP_LEADER, type, (const void *&)leadership, eNormal) && eCharPtr==type &&
                        (0==strcmp(leadership, SZ_LEADER) || 0==strcmp(leadership, SZ_HERO)))
                        Maintain += men*UpkeepLeader;  // Leaders cost more
                    else
                        Maintain += men*UpkeepPeasant; // Regular peasants
            }
        }

        // Adjust tax if total exceeds taxable amount
        long TotalTax = TaxOur + TaxTheir;
        if (TotalTax > 0 && TotalTax > pCurLand->Taxable)
            TaxOur =  (long)(((double)pCurLand->Taxable) / TotalTax * TaxOur);

        // Adjust wages if total exceeds maximum wages
        long TotalWages = WorkOur + WorkTheir;
        if (TotalWages > 0 && TotalWages > pCurLand->MaxWages)
            WorkOur =  (long)(((double)pCurLand->MaxWages) / TotalWages * WorkOur);

        // Generate report if this faction has maintenance costs
        if (Maintain>0)
        {
            Report.Description << EOL_SCR << "Faction " << (long)CurFaction << EOL_SCR;
            Report.Description << "==========" << EOL_SCR;
            Report.Description << "SILV in the beginning       "   << SilvOrg << EOL_SCR;
            Report.Description << "SILV after executing orders "   << SilvRes << EOL_SCR;
            Report.Description << "Expected Tax Income         "   << TaxOur << EOL_SCR;
            Report.Description << "Expected Work Income        "   << WorkOur << EOL_SCR;
            Report.Description << "Expected Maintenance       -"   << Maintain << EOL_SCR;
            Report.Description << "Moved out                  -"   << MovedOut << EOL_SCR;
            Report.Description << "                            -------"    << EOL_SCR;
            Report.Description << "Expected Balance            "   << (SilvRes + TaxOur + WorkOur - Maintain - MovedOut) << EOL_SCR;
            Report.Description << ""    << EOL_SCR;
            Report.Description << "Workers                     "   << Workers << EOL_SCR;
            Report.Description << "Max workers                 "   << (long)(((double)pCurLand->MaxWages)/pCurLand->Wages) << EOL_SCR;
        }
    }

    // Display the report
    m_pAtlantis->ComposeLandStrCoord(pCurLand, sCoord);
    Report.Name << "Financial report for " << sCoord;
    coll.Insert(&Report);

    ShowDescriptionList(coll, "Financial report");
    coll.DeleteAll();
}

//-------------------------------------------------------------------------
// Adds a temporary hex to the map
// Used for manual hex creation when terrain is unknown
//-------------------------------------------------------------------------

void CAhApp::AddTempHex(int X, int Y, int Plane)
{
    CLand  * pCurLand = m_pAtlantis->GetLand(X, Y, Plane, TRUE);
    if (pCurLand)
        return;  // Hex already exists

    CPlane * pPlane = (CPlane*)m_pAtlantis->m_Planes.At(Plane);
    if (!pPlane)
        return;

    assert(Plane == pPlane->Id);

    CStr     sTerrain;
    wxString strTerrain = wxGetTextFromUser(wxT("Terrain"), wxT("Please specify terrain type"));
    sTerrain = strTerrain.mb_str();

    if (sTerrain.IsEmpty())
        return;

    // Create new land with minimal information
    CLand * pLand       = new CLand;
    pLand->Id           = LandCoordToId ( X,Y, pPlane->Id );
    pLand->pPlane       = pPlane;
    pLand->Name         = SZ_MANUAL_HEX_PROVINCE;  // Special marker for temporary hexes
    pLand->TerrainType  = sTerrain;
    pLand->Taxable      = 0;
    pLand->Description  << sTerrain << " (" << (long)X << "," << (long)Y << ") in " SZ_MANUAL_HEX_PROVINCE;
    pPlane->Lands.Insert ( pLand );
}

//-------------------------------------------------------------------------
// Deletes a temporary hex from the map
// Only works for hexes created with AddTempHex
//-------------------------------------------------------------------------

void CAhApp::DelTempHex(int X, int Y, int Plane)
{
    int      idx;
    CLand  * pCurLand = m_pAtlantis->GetLand(X, Y, Plane, TRUE);
    if (!pCurLand)
        return;

    CPlane * pPlane = (CPlane*)m_pAtlantis->m_Planes.At(Plane);
    if (!pPlane)
        return;

    assert(Plane == pPlane->Id);

    // Find and remove the land from its plane
    if (pPlane->Lands.Search(pCurLand, idx))
        pPlane->Lands.AtFree(idx);
}

//-------------------------------------------------------------------------
// Shows dialog for configuring hex export options
// Returns user selections for filename, mode, and content options
//-------------------------------------------------------------------------

BOOL CAhApp::GetExportHexOptions(CStr & FName, CStr & FMode, SAVE_HEX_OPTIONS & options, eHexIncl & HexIncl,
                                 bool & InclTurnNoAcl )
{
    static CStr     stFName;
    static bool     stOverwrite     = FALSE;
    static eHexIncl stHexIncl       = HexNew;
    static bool     stInclStructs   = TRUE;
    static bool     stInclUnits     = TRUE;
    static bool     stInclTurnNoAcl = FALSE;
    static bool     stInclResources = TRUE;

    CHexExportDlg   dlg(m_Frames[AH_FRAME_MAP]);

    memset(&options, 0, sizeof(options));
    options.SaveUnits = TRUE;

    // Set default filename based on current turn
    if (stFName.IsEmpty())
        stFName.Format("map.%04d", m_pAtlantis->m_YearMon);

    // Initialize dialog with previous selections
    dlg.m_tcFName         ->SetValue(wxString::FromAscii(stFName.GetData()));

    dlg.m_rbHexNew        ->SetValue(HexNew      == stHexIncl);
    dlg.m_rbHexCurrent    ->SetValue(HexCurrent  == stHexIncl);
    dlg.m_rbHexSelected   ->SetValue(HexSelected == stHexIncl);
    dlg.m_rbHexAll        ->SetValue(HexAll      == stHexIncl);

    dlg.m_rbFileOverwrite ->SetValue(false);
    dlg.m_rbFileAppend    ->SetValue(true);

    dlg.m_chbInclStructs  ->SetValue(stInclStructs  );
    dlg.m_chbInclUnits    ->SetValue(stInclUnits    );
    dlg.m_chbInclTurnNoAcl->SetValue(stInclTurnNoAcl);
    dlg.m_chbInclResources->SetValue(stInclResources);

    // Show dialog and process results
    if (wxID_OK == dlg.ShowModal())
    {
        stFName.SetStr(dlg.m_tcFName->GetValue().mb_str());

        // Determine which hexes to export
        if (dlg.m_rbHexNew->GetValue())
            stHexIncl = HexNew;
        else if (dlg.m_rbHexCurrent->GetValue())
            stHexIncl = HexCurrent;
        else if (dlg.m_rbHexSelected->GetValue())
            stHexIncl = HexSelected;
        else if (dlg.m_rbHexAll->GetValue())
            stHexIncl = HexAll;

        stOverwrite = dlg.m_rbFileOverwrite->GetValue();

        // What to include in export
        stInclStructs   = dlg.m_chbInclStructs  ->GetValue();
        stInclUnits     = dlg.m_chbInclUnits    ->GetValue();
        stInclTurnNoAcl = dlg.m_chbInclTurnNoAcl->GetValue();
        stInclResources = dlg.m_chbInclResources->GetValue();

        // Return selected options
        FName = stFName;
#if defined(_MSC_VER)
        FMode = stOverwrite?"wb":"ab";
#else
        FMode = stOverwrite?"w":"a";
#endif
        options.SaveStructs  = stInclStructs;
        options.SaveUnits    = stInclUnits;
        options.SaveResources= stInclResources;
        HexIncl = stHexIncl;
        InclTurnNoAcl = stInclTurnNoAcl;

        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------
// Exports a single hex to the destination file
// Checks if hex should be exported based on "only new" flag and visited status
//-------------------------------------------------------------------------

void CAhApp::ExportOneHex(CFileWriter& Dest, CPlane* pPlane, CLand* pLand, SAVE_HEX_OPTIONS& options, bool InclTurnNoAcl, bool OnlyNew)
{
    if (!pLand)
    {
        wxLogDebug(wxT("ExportOneHex: pLand is NULL"));
        return;
    }

    int x, y, z;
    LandIdToCoord(pLand->Id, x, y, z);

    CStr               sData, sName;
    const char* p;
    int                ym_first = 0;
    int                ym_last = 0;

    m_pAtlantis->ComposeLandStrCoord(pLand, sName);

    // Get first and last visited turn for this hex
    p = sData.GetToken(GetConfig(SZ_SECT_LAND_VISITED, sName.GetData()), ',');
    if (!sData.IsEmpty())
    {
        ym_last = atol(sData.GetData());
        sData.GetToken(SkipSpaces(p), ',');
        ym_first = atol(sData.GetData());
    }

    // Add turn number in Atlaclient format if requested
    if (InclTurnNoAcl)
        options.WriteTurnNo = (ym_last / 100 - 1) * 12 + ym_last % 100;
    else
        options.WriteTurnNo = 0;

    // Export only if it's new (first visited this turn) or we're exporting all
    if (ym_first == m_pAtlantis->m_YearMon || !OnlyNew)
    {
        m_pAtlantis->SaveOneHex(Dest, pLand, pPlane, &options);
    }
}

//-------------------------------------------------------------------------
// Main hex export function
// Handles selection of hexes based on user options and exports them
// Can use custom selection from pCustomSelectedHexes (for area selection)
//-------------------------------------------------------------------------

void CAhApp::ExportHexes(CLongColl* pCustomSelectedHexes)
{
    CStr               sData, sName;
    CMapPane* pMapPane = (CMapPane*)m_Panes[AH_PANE_MAP];

    CLand* pLand;
    CFileWriter        Dest;
    int                nl;
    CPlane* pPlane;
    SAVE_HEX_OPTIONS   options;
    eHexIncl           HexIncl;
    bool               InclTurnNoAcl;

    // Get export options from user
    if (GetExportHexOptions(sName, sData, options, HexIncl, InclTurnNoAcl) &&
        Dest.Open(sName.GetData(), sData.GetData()))
    {
        // Case 1: Export only current hex
        if (HexCurrent == HexIncl)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(pMapPane->m_SelPlane);
            pLand = m_pAtlantis->GetLand(pMapPane->m_SelHexX, pMapPane->m_SelHexY, pMapPane->m_SelPlane, TRUE);
            if (pLand)
            {
                ExportOneHex(Dest, pPlane, pLand, options, InclTurnNoAcl, FALSE);
            }
        }
        // Case 2: Export selected hexes (from rectangle selection or custom collection)
        else if (HexSelected == HexIncl)
        {
            // If custom collection provided (from area selection), use it
            if (pCustomSelectedHexes && pCustomSelectedHexes->Count() > 0)
            {
                for (nl = 0; nl < pCustomSelectedHexes->Count(); nl++)
                {
                    long hexId = static_cast<long>(reinterpret_cast<intptr_t>(pCustomSelectedHexes->At(nl)));
                    pLand = m_pAtlantis->GetLand(hexId);
                    if (pLand)
                    {
                        ExportOneHex(Dest, pLand->pPlane, pLand, options, InclTurnNoAcl, HexNew == HexIncl);
                    }
                }
            }
            else
            {
                // Standard behavior - get selected hexes from map pane
                CBaseColl  Hexes(64);
                pMapPane->GetSelectedOrAllHexes(Hexes, true);
                for (nl = 0; nl < Hexes.Count(); nl++)
                {
                    pLand = (CLand*)Hexes.At(nl);
                    if (pLand)
                    {
                        ExportOneHex(Dest, pLand->pPlane, pLand, options, InclTurnNoAcl, HexNew == HexIncl);
                    }
                }
                Hexes.DeleteAll();
            }
        }
        // Case 3: Export all hexes (or only new ones)
        else // HexAll or HexNew
        {
            CBaseColl  Hexes(64);
            pMapPane->GetSelectedOrAllHexes(Hexes, false);
            for (nl = 0; nl < Hexes.Count(); nl++)
            {
                pLand = (CLand*)Hexes.At(nl);
                if (pLand)
                {
                    ExportOneHex(Dest, pLand->pPlane, pLand, options, InclTurnNoAcl, HexNew == HexIncl);
                }
            }
            Hexes.DeleteAll();
        }

        Dest.Close();
    }
}

//-------------------------------------------------------------------------
// Finds profitable trade routes between hexes in the selected area
// Compares sell prices and wanted prices for goods to find arbitrage opportunities
//-------------------------------------------------------------------------

void CAhApp::FindTradeRoutes()
{
    CMapPane    * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];
    CBaseColl     Hexes(64);
    CLand       * pSellLand, * pBuyLand;
    int           i, j;
    CStr          Report(64);
    int           idx;
    const char  * propnameprice;
    EValueType    type;
    const void  * value;
    CStr          GoodsName(32), PropName(32), sCoord(32);
    long          nSaleAmount, nSalePrice, nBuyAmount, nBuyPrice;

    if (!pMapPane)
        return;

    // Get selected hexes (or all if none selected)
    pMapPane->GetSelectedOrAllHexes(Hexes, TRUE);
    if (0==Hexes.Count())
    {
        wxMessageBox(wxT("Please select area on the map first."));
        return;
    }
    wxBeginBusyCursor();

    // For each hex, check what it sells
    for (i=0; i<Hexes.Count(); i++)
    {
        pSellLand = (CLand*)Hexes.At(i);

        idx      = 0;
        propnameprice = pSellLand->GetPropertyName(idx);
        while (propnameprice)
        {
            // Look for sale price properties
            if (pSellLand->GetProperty(propnameprice, type, value, eOriginal) &&
                eLong==type &&
                0==strncmp(propnameprice, PRP_SALE_PRICE_PREFIX, sizeof(PRP_SALE_PRICE_PREFIX)-1))
            {
                nSalePrice = static_cast<long>(reinterpret_cast<intptr_t>(value));
                // Extract goods name from property name
                GoodsName = &(propnameprice[sizeof(PRP_SALE_PRICE_PREFIX)-1]);

                // Get sale amount for this goods
                PropName.Empty();
                PropName << PRP_SALE_AMOUNT_PREFIX << GoodsName;
                if (!pSellLand->GetProperty(PropName.GetData(), type, value, eOriginal) || eLong!=type)
                    continue;
                nSaleAmount = static_cast<long>(reinterpret_cast<intptr_t>(value));

                // Now check all other hexes for demand of this goods
                for (j=0; j<Hexes.Count(); j++)
                {
                    pBuyLand = (CLand*)Hexes.At(j);
                    if (pBuyLand == pSellLand) continue;  // Skip self

                    // Check if buyer wants this goods
                    PropName.Empty();
                    PropName << PRP_WANTED_PRICE_PREFIX << GoodsName;
                    if (!pBuyLand->GetProperty(PropName.GetData(), type, value, eOriginal) || eLong!=type)
                        continue;
                    nBuyPrice = static_cast<long>(reinterpret_cast<intptr_t>(value));

                    PropName.Empty();
                    PropName << PRP_WANTED_AMOUNT_PREFIX << GoodsName;
                    if (!pBuyLand->GetProperty(PropName.GetData(), type, value, eOriginal) || eLong!=type)
                        continue;
                    nBuyAmount = static_cast<long>(reinterpret_cast<intptr_t>(value));

                    // If buy price > sell price, we have a profitable route
                    if (nBuyPrice > nSalePrice)
                    {
                        m_pAtlantis->ComposeLandStrCoord(pSellLand, sCoord);
                        Report << pSellLand->TerrainType << " (" << sCoord << ") " << EOL_SCR;
                        m_pAtlantis->ComposeLandStrCoord(pBuyLand, sCoord);
                        Report << "         to " << pBuyLand->TerrainType << " (" << sCoord << ")   ("
                               << nBuyPrice << "-" << nSalePrice << ")*" << std::min(nSaleAmount,nBuyAmount)
                               << " " << GoodsName
                               << " = " << (nBuyPrice - nSalePrice) * std::min(nSaleAmount,nBuyAmount) << EOL_SCR;
                    }
                }
            }
            propnameprice = pSellLand->GetPropertyName(++idx);
        }
    }

    // Show results
    if (Report.IsEmpty())
        wxMessageBox(wxT("No trade routes found."));
    else
        ShowError(Report.GetData()      , Report.GetLength()      , TRUE);

    Hexes.DeleteAll();
    wxEndBusyCursor();
}