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
// Shows a list of descriptions
// If only one item, shows single-item dialog; otherwise shows list dialog
//-------------------------------------------------------------------------

void CAhApp::ShowDescriptionList(CCollection & Items, const char * title)
{
    CBaseObject  * pObj;

    if (Items.Count() > 0)
    {
        if (1 == Items.Count())
        {
            // Single item - show simple dialog
            pObj = (CBaseObject*)Items.At(0);
            CShowOneDescriptionDlg dlg(gpApp->m_Frames[AH_FRAME_MAP], pObj->Name.GetData(), pObj->Description.GetData());
            dlg.ShowModal();
        }
        else
        {
            // Multiple items - show list dialog with navigation
            CShowDescriptionListDlg dlg(gpApp->m_Frames[AH_FRAME_MAP], title, &Items);
            dlg.ShowModal();
        }
    }
}

//-------------------------------------------------------------------------
// Views short-named objects (skills, items, objects)
// If ViewAll is TRUE, loads from config; otherwise shows ListNew
//-------------------------------------------------------------------------

void CAhApp::ViewShortNamedObjects(BOOL ViewAll, const char * szSection, const char * szHeader, CBaseColl & ListNew)
{
    CBaseColl     Items;
    CBaseObject * pItem;
    const char  * szName;
    const char  * szValue;
    int           sectidx;

    if (ViewAll)
    {
        // Load all objects from configuration
        sectidx = GetSectionFirst(szSection, szName, szValue);
        while (sectidx >= 0)
        {
            pItem              = new CBaseObject;
            pItem->Name        = szName;
            DecodeConfigLine(pItem->Description, szValue);
            Items.Insert(pItem);

            sectidx = GetSectionNext(sectidx, szSection, szName, szValue);
        }

        ShowDescriptionList(Items, szHeader);
        Items.FreeAll();
    }
    else
    {
        // Show only new objects (from current report)
        ShowDescriptionList(ListNew, szHeader);
    }
}

//-------------------------------------------------------------------------
// Shows all battles that occurred during the turn
//-------------------------------------------------------------------------

void CAhApp::ViewBattlesAll()
{
    ShowDescriptionList(m_pAtlantis->m_Battles, "Battles");
}

//-------------------------------------------------------------------------
// Shows events or errors depending on DoEvents flag
//-------------------------------------------------------------------------

void CAhApp::ViewEvents(BOOL DoEvents)
{
    CBaseColl   Coll;

    if (DoEvents)
    {
        Coll.Insert(&m_pAtlantis->m_Events);
        ShowDescriptionList(Coll, "Events");
    }
    else
    {
        m_MsgSrc.Empty();
        ShowError(m_pAtlantis->m_Errors.Description.GetData(), m_pAtlantis->m_Errors.Description.GetLength(), TRUE);
    }
    Coll.DeleteAll();
}

//-------------------------------------------------------------------------
// Shows security-related events (spy detection, etc.)
//-------------------------------------------------------------------------

void CAhApp::ViewSecurityEvents()
{
    m_MsgSrc.Empty();
    ShowError(m_pAtlantis->m_SecurityEvents.Description.GetData(), m_pAtlantis->m_SecurityEvents.Description.GetLength(), TRUE);
}

//-------------------------------------------------------------------------
// Shows newly produced items
//-------------------------------------------------------------------------

void CAhApp::ViewNewProducts()
{
    ShowDescriptionList(m_pAtlantis->m_NewProducts, "New products");
}

//-------------------------------------------------------------------------
// Shows all gates (structures that connect planes)
//-------------------------------------------------------------------------

void CAhApp::ViewGates()
{
    ShowDescriptionList(m_pAtlantis->m_Gates, "Gates");
}

//-------------------------------------------------------------------------
// Shows all cities and towns on the map
// Collects information from all lands and displays them
//-------------------------------------------------------------------------

void CAhApp::ViewCities()
{
    CBaseObject      * pObj;
    CBaseCollByName    coll;
    int                np,nl;
    CPlane           * pPlane;
    CLand            * pLand;
    CStr               sCoord;

    for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand    = (CLand*)pPlane->Lands.At(nl);
            if (!pLand->CityName.IsEmpty())
            {
                pObj       = new CBaseObject;
                pObj->Name = pLand->CityName;

                m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
                pObj->Description << pLand->TerrainType << " (" << sCoord << ") in " << pLand->Name;
                pObj->Description << ", contains " << pLand->CityName << " [" << pLand->CityType << "]";

                if (!coll.Insert(pObj))
                    delete pObj;
            }
        }
    }

    ShowDescriptionList(coll, "Cities");
    coll.FreeAll();
}

//-------------------------------------------------------------------------
// Shows all provinces (lands) on the map
// Runs twice: first for visited hexes, then for unvisited to ensure coverage
//-------------------------------------------------------------------------

void CAhApp::ViewProvinces()
{
    CBaseObject      * pObj;
    CBaseCollByName    coll;
    int                np,nl;
    CPlane           * pPlane;
    CLand            * pLand;
    CStr               sCoord;
    int                loop;

    // Two passes: first visited hexes, then unvisited
    for (loop=0; loop<2; loop++)
    {
        for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
        {
            pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
            for (nl=0; nl<pPlane->Lands.Count(); nl++)
            {
                pLand      = (CLand*)pPlane->Lands.At(nl);
                if ((pLand->Flags&LAND_VISITED) || 1==loop) // Second pass includes all
                {
                    pObj       = new CBaseObject;
                    pObj->Name = pLand->Name;

                    m_pAtlantis->ComposeLandStrCoord(pLand, sCoord);
                    pObj->Description << pLand->TerrainType << " (" << sCoord << ") in " << pLand->Name;

                    if (!coll.Insert(pObj))
                        delete pObj;
                }
            }
        }
    }

    ShowDescriptionList(coll, "Provinces");
    coll.FreeAll();
}

//-------------------------------------------------------------------------
// Shows general faction information including hex statistics
//-------------------------------------------------------------------------

void CAhApp::ViewFactionInfo()
{
    CStr sMoreInfo(32), sInfo(32);
    int                np,nl;
    CPlane           * pPlane;
    CLand            * pLand;
    long               nLandsTotal = 0, nLandsVisited=0;

    sMoreInfo << EOL_SCR << "-------------------------" << EOL_SCR;
    
    // Count total and visited hexes
    for (np=0; np<m_pAtlantis->m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_pAtlantis->m_Planes.At(np);
        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand    = (CLand*)pPlane->Lands.At(nl);
            nLandsTotal++;
            if (pLand->Flags&LAND_VISITED)
                nLandsVisited++;
        }
    }
    
    sMoreInfo << "Total hexes  : " << nLandsTotal   << EOL_SCR
              << "Visited hexes: " << nLandsVisited << EOL_SCR ;

    sInfo << m_pAtlantis->m_FactionInfo << sMoreInfo;
    CShowOneDescriptionDlg dlg(gpApp->m_Frames[AH_FRAME_MAP],
                               "Faction Info",
                               sInfo.GetData());
    dlg.ShowModal();
}

//-------------------------------------------------------------------------
// Helper for ViewFactionOverview: increments a property value for a faction
// Handles overflow protection and property creation
//-------------------------------------------------------------------------

void CAhApp::ViewFactionOverview_IncrementValue(long FactionId, const char * factionname, CBaseCollById & Factions, const char * propname, long value)
{
    CBaseObject   * pFaction;
    CBaseObject     Dummy;
    int             idx;
    EValueType      type;
    const void    * valuetot;

    Dummy.Id = FactionId;
    if (Factions.Search(&Dummy, idx))
        pFaction = (CBaseObject*)Factions.At(idx);
    else
    {
        // Create new faction entry
        pFaction       = new CBaseObject;
        pFaction->Id   = FactionId;
        if (factionname)
            pFaction->Name = factionname;
        Factions.Insert(pFaction);
    }

    if (!pFaction->GetProperty(propname, type, valuetot, eNormal))
        valuetot = (void*)0;

    // Overflow protection
    if (-1== static_cast<long>(reinterpret_cast<intptr_t>(valuetot)) || 
        0x7fffffff - (long)value < static_cast<long>(reinterpret_cast<intptr_t>(valuetot)) )
        valuetot = (void*)(long)-1; // Mark as overflow
    else
        valuetot = reinterpret_cast<void*>(static_cast<uintptr_t>((static_cast<long>(reinterpret_cast<intptr_t>(valuetot)))) + (long)value);
    
    pFaction->SetProperty(propname, eLong, valuetot, eNormal);
}

//-------------------------------------------------------------------------
// Shows comprehensive faction overview with aggregated data
// Collects and sums various properties (items, skills, men counts) by faction
// Can be restricted to selected area on map
//-------------------------------------------------------------------------

void CAhApp::ViewFactionOverview()
{
    int             unitidx, propidx, nl;
    CUnit         * pUnit;
    CStr            propname;
    CStr            Skill;
    int             skilllen;
    int             maxproplen = 0;
    CStr            Report(128);
    CMapPane      * pMapPane  = (CMapPane* )m_Panes[AH_PANE_MAP];
    BOOL            Selected  = FALSE;
    EValueType      type;
    const void    * value;
    int             idx;
    CBaseObject   * pFaction;
    long            men;

    CBaseColl       Hexes(64);
    CBaseCollById   Factions(16);
    CLand         * pLand;

    // Hint about using selection if none is active
    if (!pMapPane->HaveSelection())
        ShowMessageBoxSwitchable(wxT("Hint"), wxT("Faction overview can be generated using only selected area on the map"), wxT("FACTION_OVERVIEW"));

    // Ask user if they want to restrict to selected hexes
    if (pMapPane->HaveSelection() &&
        wxYES == wxMessageBox(wxT("Use only selected hexes?"), wxT("Confirm"), wxYES_NO, NULL))
        Selected = TRUE;

    skilllen    = strlen(PRP_SKILL_POSTFIX);

    // Collect data from hexes
    pMapPane->GetSelectedOrAllHexes(Hexes, Selected);
    for (nl=0; nl<Hexes.Count(); nl++)
    {
        pLand = (CLand*)Hexes.At(nl);
        for (unitidx=0; unitidx<pLand->Units.Count(); unitidx++)
        {
            pUnit    = (CUnit*)pLand->Units.At(unitidx);
            men      = 0;
            if (pUnit->GetProperty(PRP_MEN, type, value, eOriginal) && (eLong==type) )
                men = static_cast<long>(reinterpret_cast<intptr_t>(value));

            // Iterate through all unit properties
            for (propidx=0; propidx<m_pAtlantis->m_UnitPropertyNames.Count(); propidx++)
            {
                propname = (const char *) gpApp->m_pAtlantis->m_UnitPropertyNames.At(propidx);

                // Skip skill days properties (not aggregate)
                if (IsASkillRelatedProperty(propname.GetData()) &&
                     propname.FindSubStrR(PRP_SKILL_POSTFIX) != propname.GetLength()-skilllen)
                    continue;

                // Skip properties that cannot be aggregated
                if (0==stricmp(propname.GetData(), PRP_ID        ) ||
                    0==stricmp(propname.GetData(), PRP_FACTION_ID) ||
                    0==stricmp(propname.GetData(), PRP_LAND_ID   ) ||
                    0==stricmp(propname.GetData(), PRP_STRUCT_ID ) ||
                    0==stricmp(propname.GetData(), PRP_TEACHING  ) ||
                    0==stricmp(propname.GetData(), PRP_SKILLS    ) ||
                    0==stricmp(propname.GetData(), PRP_MAG_SKILLS) ||
                    0==stricmp(propname.GetData(), PRP_WEIGHT_WALK) ||
                    0==stricmp(propname.GetData(), PRP_WEIGHT_RIDE) ||
                    0==stricmp(propname.GetData(), PRP_WEIGHT_FLY) ||
                    0==stricmp(propname.GetData(), PRP_WEIGHT_SWIM) ||
                    0==stricmp(propname.GetData(), PRP_BEST_SKILL) ||
                    0==stricmp(propname.GetData(), PRP_BEST_SKILL_DAYS) ||
                    0==stricmp(propname.GetData(), PRP_MAG_SKILLS) ||
                    0==stricmp(propname.GetData(), PRP_GUI_COLOR ) ||
                    0==stricmp(propname.GetData(), PRP_SEQUENCE  ) ||
                    0==stricmp(propname.GetData(), PRP_FRIEND_OR_FOE  )
                   )
                    continue;

                if (pUnit->GetProperty(propname.GetData(), type, value, eOriginal) &&
                    (eLong==type) )
                    do
                    {
                        if (propname.FindSubStrR(PRP_SKILL_POSTFIX) == propname.GetLength()-skilllen)
                        {
                            // It's a skill - aggregate men with this skill level
                            propname << static_cast<long>(reinterpret_cast<intptr_t>(value));
                            value    = reinterpret_cast<void*>(static_cast<uintptr_t>(men));
                        }
                        else
                            if (IsASkillRelatedProperty(propname.GetData()))
                                break;

                        if (propname.GetLength() > maxproplen)
                            maxproplen = propname.GetLength();

                        // Add to faction totals
                        ViewFactionOverview_IncrementValue(
                            pUnit->FactionId, 
                            pUnit->pFaction ? pUnit->pFaction->Name.GetData() : NULL, 
                            Factions, propname.GetData(), 
                            static_cast<long>(reinterpret_cast<intptr_t>(value)));

                    } while (FALSE);
            }

            // Track combat positioning
            if (pUnit->Flags & UNIT_FLAG_AVOIDING)
                ViewFactionOverview_IncrementValue(pUnit->FactionId, pUnit->pFaction ? pUnit->pFaction->Name.GetData() : NULL, Factions, "Avoiding", men);
            else
            {
                if (pUnit->Flags & UNIT_FLAG_BEHIND)
                    ViewFactionOverview_IncrementValue(pUnit->FactionId, pUnit->pFaction ? pUnit->pFaction->Name.GetData() : NULL, Factions, "Back Line", men);
                else
                    ViewFactionOverview_IncrementValue(pUnit->FactionId, pUnit->pFaction ? pUnit->pFaction->Name.GetData() : NULL, Factions, "Front Line", men);
            }
        }
    }
    Hexes.DeleteAll();

    // Prepare formatted report
    for (idx=0; idx<Factions.Count(); idx++)
    {
        pFaction = (CBaseObject*)Factions.At(idx);
        Report << "Faction " << pFaction->Id << " " << pFaction->Name << EOL_SCR << EOL_SCR;

        propidx  = 0;
        propname = pFaction->GetPropertyName(propidx);
        while (!propname.IsEmpty())
        {
            if (pFaction->GetProperty(propname.GetData(), type, value, eNormal) &&
                (eLong==type) )
            {
                // Pad for alignment
                while (propname.GetLength() < maxproplen)
                    propname.AddCh(' ');
                Report << propname << "  " << static_cast<long>(reinterpret_cast<intptr_t>(value)) << EOL_SCR;
            }

            propname = pFaction->GetPropertyName(++propidx);
        }
        Report << EOL_SCR << "-------------------------------------------"  << EOL_SCR << EOL_SCR;
    }

    // Display report
    CShowOneDescriptionDlg dlg(gpApp->m_Frames[AH_FRAME_MAP],
                               "Factions Overview",
                               Report.GetData());
    dlg.ShowModal();
    Factions.FreeAll();
}

//-------------------------------------------------------------------------
// Exports mage information to CSV file
// Shows dialog for format options and calls Atlantis parser to generate CSV
//-------------------------------------------------------------------------

void CAhApp::WriteMagesCSV()
{
    CStr FName;

    // Generate default filename based on current turn
    FName.Format("%s%04d.csv", "mages", m_pAtlantis->m_YearMon);

    CExportMagesCSVDlg Dlg(m_Frames[AH_FRAME_MAP], FName.GetData());
    if (wxID_OK == Dlg.ShowModal())
        m_pAtlantis->WriteMagesCSV(Dlg.m_pFileName->GetValue().mb_str(),
                                   0==SafeCmp(Dlg.m_pOrientation->GetValue().mb_str(), SZ_VERTICAL),
                                   Dlg.m_pSeparator->GetValue().mb_str(),
                                   Dlg.m_nFormat
                                  );
}