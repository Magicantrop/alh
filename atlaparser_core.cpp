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

#include "atlaparser.h"
#include <algorithm>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "stdafx.h"
#include "wx/string.h"
#include <wx/regex.h>
#include "ahapp.h"
#include "files.h"
#include "consts.h"
#include "cstr.h"
#include "collection.h"
#include "cfgfile.h"
#include "objs.h"
#include "data.h"
#include "errs.h"
#include "consts_ah.h"
#include "routeplanner.h"

// Constants

const char* Monthes[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
const char* EOL_MS = "\r\n";
const char* EOL_UNIX = "\n";
const char* EOL_SCR = EOL_UNIX;
const char* EOL_FILE = EOL_UNIX;
const char* STRUCT_UNIT_START = "-+*";
const char* Directions[] = { "North", "Northeast", "Southeast", "South", "Southwest", "Northwest",
                             "N", "NE", "SE", "S", "SW", "NW" };
const int DirectionsCount = sizeof(Directions) / sizeof(const char*);
const char* LocationsShipsArcadia[] = { "Northern hexside", "North Eastern hexside",
                                        "South Eastern hexside", "Southern hexside",
                                        "South Western hexside", "North Western hexside",
                                        "hex centre" };

int ExitFlags[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20 };
int EntryFlags[] = { 0x08, 0x10, 0x20, 0x01, 0x02, 0x04 };

int Flags_NW_N_NE = 0x01 | 0x02 | 0x20;
int Flags_N = 0x01;
int Flags_SW_S_SE = 0x04 | 0x08 | 0x10;
int Flags_S = 0x08;

const char* BUG = " - this is a bug!";
const char* NOSETUNIT = " - Cannot set property for unit ";
const char* NOSET = " - Cannot set unit property ";
const char* NOTNUMERIC = " - Property is not numeric for unit ";

const char* ExitEndHeader[] = { HDR_FACTION, HDR_FACTION_STATUS, HDR_ERRORS, HDR_EVENTS, HDR_SILVER,
                                HDR_BATTLES, HDR_ATTACKERS, HDR_ATTITUDES, HDR_SKILLS, HDR_ITEMS, HDR_OBJECTS };
const int ExitEndHeaderCount = sizeof(ExitEndHeader) / sizeof(const char*);
int ExitEndHeaderLen[] = { (int)strlen(HDR_FACTION), (int)strlen(HDR_FACTION_STATUS),
                           (int)strlen(HDR_ERRORS), (int)strlen(HDR_EVENTS),
                           (int)strlen(HDR_SILVER), (int)strlen(HDR_BATTLES),
                           (int)strlen(HDR_ATTACKERS), (int)strlen(HDR_ATTITUDES),
                           (int)strlen(HDR_SKILLS), (int)strlen(HDR_ITEMS), (int)strlen(HDR_OBJECTS) };

const char* BattleEndHeader[] = { HDR_ERRORS, HDR_EVENTS, HDR_SILVER, HDR_ATTITUDES,
                                  HDR_SKILLS, HDR_ITEMS, HDR_OBJECTS, HDR_FACTION,
                                  HDR_FACTION_STATUS, HDR_SILVER };
extern const int BattleEndHeaderCount = sizeof(BattleEndHeader) / sizeof(const char*);
int BattleEndHeaderLen[] = { (int)strlen(HDR_ERRORS), (int)strlen(HDR_EVENTS),
                             (int)strlen(HDR_SILVER), (int)strlen(HDR_ATTITUDES),
                             (int)strlen(HDR_SKILLS), (int)strlen(HDR_ITEMS),
                             (int)strlen(HDR_OBJECTS), (int)strlen(HDR_FACTION),
                             (int)strlen(HDR_FACTION_STATUS), (int)strlen(HDR_SILVER)};

//----------------------------------------------------------------------
// Helper function: checks if a string represents a valid integer
//----------------------------------------------------------------------

BOOL IsInteger(const char * s)
{
    int n = 0;

    if ((!s) || (!*s))
        return FALSE;

    while (*s)
    {
        if ( ('-'==*s) && (n>0) )
            return FALSE;  // Minus sign only allowed at first position
        else
            if ( (*s<'0') || (*s>'9') )
                return FALSE;  // Non-digit character

        n++;
        s++;
    }
    return TRUE;
}

//======================================================================
// CAtlaParser implementation
//======================================================================

CAtlaParser::CAtlaParser()
            : m_UnitFlagsHash(1)
{
    // Intentionally causes division by zero to trigger assertion in debug mode
    int x = 2;
    x = 1/(x-2);
}

//----------------------------------------------------------------------

CAtlaParser::CAtlaParser(CGameDataHelper * pHelper)
            :m_UnitFlagsHash(1), m_sOrderErrors(256)
{
    gpDataHelper      = pHelper;

    m_CrntFactionId   = 0;
    m_ParseErr        = ERR_NOTHING;
    m_nCurLine        = 0;
    m_GatesCount      = 0;
    m_YearMon         = 0;
    m_CurYearMon      = 0;
    m_pSource         = NULL;
    m_pCurLand        = NULL;
    m_pCurStruct      = NULL;
    m_NextStructId    = 1;
    m_OrdersLoaded    = FALSE;
    m_JoiningRep      = FALSE;
    m_IsHistory       = FALSE;
    m_Events.Name     = "Events";
    m_SecurityEvents.Name = "Security Events";
    m_HexEvents.Name  = "Hex Events";
    m_Errors.Name     = "Errors";
    m_ArcadiaSkills   = FALSE;
    m_RegexRulesLoaded = FALSE;

    m_EconomyTaxPillage=false;
    m_EconomyShareAfterBuy = false;
    m_EconomyWork     = false;
    m_EconomyMaintainanceCosts = false;
    m_EconomyShareMaintainance = false;

    // Initialize unit flags hash for quick lookup of flag strings
    m_UnitFlagsHash.Insert("taxing"                      ,     (void*)UNIT_FLAG_TAXING            );
    m_UnitFlagsHash.Insert("on guard"                    ,     (void*)UNIT_FLAG_GUARDING          );
    m_UnitFlagsHash.Insert("avoiding"                    ,     (void*)UNIT_FLAG_AVOIDING          );
    m_UnitFlagsHash.Insert("behind"                      ,     (void*)UNIT_FLAG_BEHIND            );
    m_UnitFlagsHash.Insert("revealing unit"              ,     (void*)UNIT_FLAG_REVEALING_UNIT    );
    m_UnitFlagsHash.Insert("revealing faction"           ,     (void*)UNIT_FLAG_REVEALING_FACTION );
    m_UnitFlagsHash.Insert("holding"                     ,     (void*)UNIT_FLAG_HOLDING           );
    m_UnitFlagsHash.Insert("receiving no aid"            ,     (void*)UNIT_FLAG_RECEIVING_NO_AID  );
    m_UnitFlagsHash.Insert("consuming unit's food"       ,     (void*)UNIT_FLAG_CONSUMING_UNIT    );
    m_UnitFlagsHash.Insert("consuming faction's food"    ,     (void*)UNIT_FLAG_CONSUMING_FACTION );
    m_UnitFlagsHash.Insert("won't cross water"           ,     (void*)UNIT_FLAG_NO_CROSS_WATER    );
    m_UnitFlagsHash.Insert("sharing"                     ,     (void*)UNIT_FLAG_SHARING           ); // Arcadia

    // Multiple spoils types all map to same flag
    m_UnitFlagsHash.Insert("weightless battle spoils"    ,     (void*)UNIT_FLAG_SPOILS            );
    m_UnitFlagsHash.Insert("flying battle spoils"        ,     (void*)UNIT_FLAG_SPOILS            );
    m_UnitFlagsHash.Insert("walking battle spoils"       ,     (void*)UNIT_FLAG_SPOILS            );
    m_UnitFlagsHash.Insert("riding battle spoils"        ,     (void*)UNIT_FLAG_SPOILS            );
    m_UnitFlagsHash.Insert("swiming battle spoils"       ,     (void*)UNIT_FLAG_SPOILS            );
    m_UnitFlagsHash.Insert("sailing battle spoils"       ,     (void*)UNIT_FLAG_SPOILS            );
}

//----------------------------------------------------------------------

CAtlaParser::~CAtlaParser()
{
    Clear();
    m_UnitFlagsHash.FreeAll();
}

//----------------------------------------------------------------------
// Clears all parsed data, freeing memory
//----------------------------------------------------------------------

void CAtlaParser::Clear()
{
    m_Factions.FreeAll();

    m_YearMon       = 0;
    m_CurYearMon    = 0;
    m_JoiningRep    = FALSE;
    m_IsHistory     = FALSE;
    m_Planes.DeleteAll();
    m_PlanesNamed.FreeAll();  // Must go before units, as it checks contents of CLand::Units collection
    m_Units.FreeAll();

    m_CrntFactionId = 0;
    m_CrntFactionPwd.Empty();
    m_OurFactions.FreeAll();
    m_TaxLandStrs.FreeAll();
    m_TradeLandStrs.FreeAll();
    m_BattleLandStrs.FreeAll();
    m_UnitPropertyNames.FreeAll();
    m_UnitPropertyTypes.FreeAll();
    m_LandPropertyNames.FreeAll();

    m_TradeUnitIds.DeleteAll();
    m_Skills.FreeAll();
    m_Items.FreeAll();
    m_Objects.FreeAll();
    m_Battles.FreeAll();
    m_Gates.FreeAll();
    m_Events.Description.Empty();
    m_SecurityEvents.Description.Empty();
    m_HexEvents.Description.Empty();
    m_Errors.Description.Empty();
    m_NewProducts.FreeAll();
    m_TempSailingEvents.FreeAll();
}

//----------------------------------------------------------------------
// Reads next line from file, with optional line merging for wrapped text
//----------------------------------------------------------------------

BOOL CAtlaParser::ReadNextLine(CStr & s)
{
    const bool MergeLines = true;
    if (MergeLines)
    {
        return ReadNextLineMerged(s);
    }
    BOOL Ok=FALSE;

    if (m_pSource)
    {
        Ok = m_pSource->GetNextLine(s);
        if (Ok)
        {
            m_nCurLine++;
            s.TrimRight(TRIM_ALL);
            s << EOL_SCR;
        }
    }
    return Ok;
}

//----------------------------------------------------------------------
// Reads next line and merges with following lines if they are indented
// (handles multi-line descriptions in reports)
//----------------------------------------------------------------------

BOOL CAtlaParser::ReadNextLineMerged(CStr & s)
{
    bool ok = false;

    if (!m_pSource) return false;
    ok = m_pSource->GetNextLine(s);
    if (!ok) return false;

    m_nCurLine++;
    s.TrimRight(TRIM_ALL);

    const char * p = s.GetData();

    // Do not merge region separator or exit list
    bool tryMerge = (strncmp(p, "---------------", 15) != 0 && strncmp(p, "Exits:", 6) != 0);
    if (strlen(p) == 0) tryMerge = false;

    int indent[2] = {0,0};
    while ((*p) == ' ') {++p; ++indent[0]; }
    const bool startsWithPlusSign = *p == '+';
    CStr nextLine;

    // Try to merge subsequent lines with greater indentation
    while (tryMerge)
    {
        if (!m_pSource) break;
        tryMerge = m_pSource->GetNextLine(nextLine);
        if (tryMerge)
        {
            m_nCurLine++;
            p = nextLine.GetData();
            indent[1] = 0;
            while ((*p) == ' ') {++p; ++indent[1]; }

            if ((indent[0] + 2 == indent[1]) && (!startsWithPlusSign || (*p != '-' && *p != '*')))
            {
                // Merge this line with previous
                nextLine.TrimRight(TRIM_ALL);
                nextLine.TrimLeft(TRIM_ALL);
                s << " " << nextLine;
            }
            else
            {
                tryMerge = false;
                PutLineBack(nextLine);
            }
        }
    }
    s << EOL_SCR;
    return ok;
}

//----------------------------------------------------------------------
// Puts a line back into the input stream (for lookahead parsing)
//----------------------------------------------------------------------

void CAtlaParser::PutLineBack (CStr & s)
{
    m_nCurLine--;
    if (m_pSource)
        m_pSource->QueueString(s.GetData());
}

//----------------------------------------------------------------------
// Parses faction information header from report
// Extracts faction ID, name, and turn date
//----------------------------------------------------------------------

int CAtlaParser::ParseFactionInfo(BOOL GetNo, BOOL Join)
{
    int          err    = ERR_OK;
    CStr         Line(128);
    CStr         FNo;
    CStr         Str;
    int          LineNo = 0;
    const char * p;
    const char * s;
    unsigned int i;
    CFaction   * pMyFaction;
    long         yearmon = 0;

    while ((ERR_OK==err) && ReadNextLine(Line))
    {
        m_FactionInfo << Line;

        Line.TrimRight(TRIM_ALL);

        if (Line.IsEmpty())
            break;  // Stop at empty line

        if (GetNo)
            switch (LineNo)
            {
            case 0:  // Faction number line: "Faction Name (ID)"
                s = Line.GetData();
                p = strchr(s, '(');
                if (p)
                {
                    pMyFaction = new CFaction;
                    pMyFaction->Description << Line.GetData() << EOL_SCR;
                    pMyFaction->Name.SetStr(s, (p-s));
                    s = p+1;
                    p = strchr(s, ')');
                    FNo.SetStr(s, (p-s));
                    m_CrntFactionId = atol(FNo.GetData());
                    pMyFaction->Id  = m_CrntFactionId;
                    if (!m_Factions.Insert(pMyFaction))
                        delete pMyFaction;
                    m_OurFactions.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(m_CrntFactionId)));
                    if (!Join) gpDataHelper->SetPlayingFaction((long) m_CrntFactionId);
                }
                break;
            case 1:  // Date line: "December, Year 2"
                p = Str.GetToken(Line.GetData(), ',');
                for (i=0; i<sizeof(Monthes)/sizeof(char*); i++)
                    if (0==Str.FindSubStr(Monthes[i]))
                    {
                        yearmon = i+1;
                        break;
                    }
                p = SkipSpaces(p);
                p = Str.GetToken(p, ' '); // 'Year'
                p = Str.GetToken(p, ' ');
                yearmon += 100*atol(Str.GetData());

                if (m_JoiningRep && m_YearMon != yearmon)
                {
                    err = ERR_INV_TURN;  // Can't join reports from different turns
                    if (m_CrntFactionId > 0)
                        for (i=0; i<(unsigned int)m_OurFactions.Count(); i++)
                            if (static_cast<long>(reinterpret_cast<intptr_t>(m_OurFactions.At(i)))==m_CrntFactionId)
                            {
                                m_OurFactions.AtDelete(i);
                                break;
                            }
                }
                else
                {
                    m_YearMon    = yearmon;
                    m_CurYearMon = yearmon; // Loaded file may not contain year/month information
                }
                break;
            }

        LineNo++;
    }

    return err;
}

//----------------------------------------------------------------------
// Sets a flag on a land identified by coordinate string
//----------------------------------------------------------------------

int  CAtlaParser::SetLandFlag(const char * p, long flag)
{
    CLand * pLand = GetLand(p);

    if (pLand)
        pLand->Flags |= flag;

    return 0;
}

//----------------------------------------------------------------------
// Sets a flag on a land identified by ID
//----------------------------------------------------------------------

int  CAtlaParser::SetLandFlag(long LandId, long flag)
{
    CLand * pLand;

    pLand = GetLand(LandId);
    if (pLand)
        pLand->Flags |= flag;

    return 0;
}

//----------------------------------------------------------------------
// Applies flags collected during parsing (tax, trade, battle) to lands
//----------------------------------------------------------------------

int  CAtlaParser::ApplyLandFlags()
{
    int          i, idx;
    const char * s;
    CBaseObject  Dummy;
    CUnit      * pUnit;

    for (i=0; i<m_TaxLandStrs.Count(); i++)
    {
        s = (const char*)m_TaxLandStrs.At(i);
        SetLandFlag(s, LAND_TAX);
    }

    for (i=0; i<m_TradeLandStrs.Count(); i++)
    {
        s = (const char*)m_TradeLandStrs.At(i);
        SetLandFlag(s, LAND_TRADE);
    }

    for (i=0; i<m_BattleLandStrs.Count(); i++)
    {
        s = (const char*)m_BattleLandStrs.At(i);
        SetLandFlag(s, LAND_BATTLE);
    }

    for (i=0; i<m_TradeUnitIds.Count(); i++)
    {
        Dummy.Id = static_cast<long>(reinterpret_cast<intptr_t>(m_TradeUnitIds.At(i)));
        if (m_Units.Search(&Dummy, idx))
        {
            pUnit = (CUnit*)m_Units.At(idx);
            SetLandFlag(pUnit->LandId, LAND_TRADE);
        }
    }

    m_TaxLandStrs.FreeAll();
    m_TradeLandStrs.FreeAll();
    m_BattleLandStrs.FreeAll();
    m_TradeUnitIds.DeleteAll();

    return 0;
}

//----------------------------------------------------------------------
// Creates or retrieves a plane by name
//----------------------------------------------------------------------

CPlane * CAtlaParser::MakePlane(const char * planename)
{
    static CBaseObject Dummy;
    int      i;
    CPlane * pPlane;

    Dummy.Name = planename;
    if (m_PlanesNamed.Search(&Dummy, i))
        pPlane = (CPlane*)m_PlanesNamed.At(i);
    else
    {
        pPlane       = new CPlane;
        pPlane->Id   = m_Planes.Count();
        pPlane->Name = planename;
        if (!gpDataHelper->GetTropicZone(pPlane->Name.GetData(), pPlane->TropicZoneMin, pPlane->TropicZoneMax))
        {
            pPlane->TropicZoneMin  = TROPIC_ZONE_MAX;
            pPlane->TropicZoneMax  = -(TROPIC_ZONE_MAX);
        }

        // Parse plane size from config: "<x-min>,<y-min>,<x-max>,<y-max>"
        const char * value = gpDataHelper->GetPlaneSize(pPlane->Name.GetData());
        if (value && strlen(value)>4)
        {
            CStr S;
            value = S.GetToken(value, ',');
            pPlane->WestEdge = atol(S.GetData());

            value = S.GetToken(value, ',');
            //y_min = atol(S.GetData());  // Not currently used

            value = S.GetToken(value, ',');
            pPlane->EastEdge = atol(S.GetData());

            value = S.GetToken(value, ',');
            //y_max = atol(S.GetData());  // Not currently used
            pPlane->Width = pPlane->EastEdge - pPlane->WestEdge + 1;
        }
        m_PlanesNamed.Insert(pPlane);
        m_Planes.Insert(pPlane);
    }

    return pPlane;
}

//----------------------------------------------------------------------
// Parses terrain description for a land (hex)
// Handles both full land descriptions and exit descriptions
//----------------------------------------------------------------------

int CAtlaParser::ParseTerrain(CLand * pMotherLand, int ExitDir, CStr & FirstLine, BOOL FullMode, CLand ** ppParsedLand)
{
    int                  err = ERR_OK;
    const char         * p;
    CStr                 Name;
    CStr                 S(64);
    long                 x, y;
    CLand              * pLand = NULL;
    CStr                 CurLine(128);
    CStr                 PlaneName(32);
    CStr                 LandName(32);
    int                  i;
    int                  idxland;
    char                 ch;
    CPlane             * pPlane;
    CStruct            * pStruct;
    BOOL                 DoBreak;
    CBaseObject        * pGate;
    CBaseObject          Dummy;
    int                  no;
    int                  idx;
    CStr                 TempDescr(64);
    BOOL                 HaveEvents = FALSE;
    CStr                 CompositeDescr(64);

    if (FirstLine.IsEmpty())
        goto Exit;

    // Parse format: "TerrainType (x,y[,plane]) in ProvinceName"
    p = SkipSpaces(Name.GetToken(FirstLine.GetData(), '(', TRIM_ALL));
    if (!p )   // Must have '('
        goto Exit;

    // Parse X coordinate
    p = S.GetToken(p, ',');
    if (!IsInteger(S.GetData()))
        goto Exit;
    x = atol(S.GetData());

    // Parse Y coordinate
    p = S.GetToken(p, ",)", ch);
    if (!IsInteger(S.GetData()))
        goto Exit;
    y = atol(S.GetData());
    if (','==ch)
    {
        // Plane name provided
        p = PlaneName.GetToken(p, ')');
    }
    else
        PlaneName = DEFAULT_PLANE;

    if (!p)
        goto Exit;

    // Parse "in" keyword and province name
    p = SkipSpaces(S.GetToken(SkipSpaces(p), ' ', TRIM_ALL));
    if (0!=stricmp(S.GetData(),"in"))
        goto Exit;
    LandName.GetToken(p, ",.", ch, TRIM_ALL);

    // Remove Arcadia III edge location reference for sailing events
    if (strchr(LandName.GetData(), '('))
    {
        int xLocal = strchr(LandName.GetData(), '(') - LandName.GetData();
        LandName.DelSubStr(x, LandName.GetLength()- xLocal);
        LandName.TrimRight(TRIM_ALL);
    }

    pPlane = MakePlane(PlaneName.GetData());

    // Check if land already exists
    Dummy.Id = LandCoordToId(x,y, pPlane->Id);
    if (pPlane->Lands.Search(&Dummy, idxland))
    {
        pLand = (CLand*)pPlane->Lands.At(idxland);
        if (0!=stricmp(pLand->TerrainType.GetData(), Name.GetData()))
        {
            S.Format("*** Terrain changed for %s from '%s' to '%s' - clearing saved description and products! ***",
                     FirstLine.GetData(), pLand->TerrainType.GetData(), Name.GetData());
            GenericErr(1, S.GetData());
            pLand->TerrainType  = Name;
            pLand->Description.Empty();
            pLand->Products.FreeAll();
        }
        if (0!=stricmp(pLand->Name.GetData(), LandName.GetData()) )
        {
            S.Format("*** Province changed for %s from '%s' to '%s' - clearing saved description! ***",
                     FirstLine.GetData(), pLand->Name.GetData(), LandName.GetData());
            GenericErr(1, S.GetData());
            pLand->Name     = LandName;
            pLand->Description.Empty();
        }
    }
    else
    {
        pLand               = new CLand;
        pLand->Id           = LandCoordToId(x,y, pPlane->Id);
        pLand->pPlane       = pPlane;
        pLand->Name         = LandName;
        pLand->TerrainType  = Name;
        pLand->Taxable      = 0;
        pPlane->Lands.Insert(pLand);
    }

    TempDescr = FirstLine;

    if (pMotherLand)  // This is an exit description
    {
        // Exit description may take multiple lines
        while (ReadNextLine(CurLine))
        {
            const char * s = SkipSpaces(CurLine.GetData());
            if (!s || !*s)
                break;

            // Stop if we hit unit/structure marker or colon
            if (strchr(STRUCT_UNIT_START, *s) || strchr(s, ':'))
            {
                PutLineBack(CurLine);
                break;
            }

            CurLine.TrimRight(TRIM_ALL);
            TempDescr << CurLine << EOL_SCR;

            if (strchr(CurLine.GetData(), '.'))
                break;
        }
    }

    // Check exit consistency if this is an exit and on same plane
    if (pMotherLand && (pPlane==pMotherLand->pPlane))
        CheckExit(pPlane, ExitDir, pMotherLand, pLand);

    // For exit-only parsing, just analyze the terrain and return
    if (!FullMode)
    {
        TempDescr.TrimRight(TRIM_ALL);
        pLand->Description.TrimRight(TRIM_ALL);

        if (pLand->Description.IsEmpty())
            pLand->Description = TempDescr;

        AnalyzeTerrain(pMotherLand, pLand, pMotherLand!=NULL, ExitDir, TempDescr);
        if (pMotherLand)
            pMotherLand->Exits << TempDescr << EOL_SCR;
        goto Exit;
    }

    // Full land description parsing below
    // Remove old non-permanent structures (keep gates and shafts)
    for (i=pLand->Structs.Count()-1; i>=0; i--)
    {
        pStruct = (CStruct*)pLand->Structs.At(i);
        if (0==(pStruct->Attr & SA_HIDDEN) &&   // Keep gates!
            0==(pStruct->Attr & SA_SHAFT ) )    // Keep shafts!
            pLand->Structs.AtFree(i);
    }

    // Parse extended land description until exit list or other header
    DoBreak = FALSE;
    while (ReadNextLine(CurLine))
    {
        CurLine.TrimRight(TRIM_ALL);
        p = SkipSpaces(CurLine.GetData());
        if (0==stricmp("Events:", p))
        {
            HaveEvents = TRUE;
            break;
        }
        if (0==stricmp("Exits:", p))
            break;
        for (i=0; i<(int)sizeof(ExitEndHeader)/(int)sizeof(const char *); i++)
            if (0==strnicmp(p, ExitEndHeader[i], ExitEndHeaderLen[i] ))
            {
                CurLine << EOL_FILE;
                PutLineBack(CurLine);
                DoBreak = TRUE;
                break;
            }
        if (DoBreak)
            break;

        // Remove Atlaclient turn marker if present
        no=0;
        while (p && *p && '-'==*p)
        {
            no++;
            p++;
        }
        if (no>20 && ';'==*p)
        {
            no = strlen(p++);
            CurLine.DelSubStr(CurLine.GetLength()-no, no);
            pLand->AtlaclientsLastTurnNo = atol(p);
        }

        TempDescr << CurLine << EOL_SCR;
    }

    // Merge with existing description (for Arno game specific format)
    ComposeHexDescriptionForArnoGame(pLand->Description.GetData(), TempDescr.GetData(), CompositeDescr);
    pLand->Description = CompositeDescr;
    pLand->Description.TrimRight(TRIM_ALL);
    AnalyzeTerrain(NULL, pLand, FALSE, ExitDir, pLand->Description);

    // Parse events if present
    DoBreak = FALSE;
    if (HaveEvents)
    {
        while (ReadNextLine(CurLine))
        {
            CurLine.TrimRight(TRIM_ALL);
            p = SkipSpaces(CurLine.GetData());
            if (0==stricmp("Exits:", p))
                break;
            for (i=0; i<(int)sizeof(ExitEndHeader)/(int)sizeof(const char *); i++)
                if (0==strnicmp(p, ExitEndHeader[i], ExitEndHeaderLen[i] ))
            {
                CurLine << EOL_FILE;
                PutLineBack(CurLine);
                DoBreak = TRUE;
                break;
            }
            if (DoBreak)
                break;
            pLand->Events << CurLine << EOL_SCR;
        }
        pLand->Events.TrimRight(TRIM_ALL);
    }

    if (!m_IsHistory && m_CurYearMon>0)
        pLand->Flags |= LAND_IS_CURRENT;

    m_pCurLand   = pLand;
    m_pCurStruct = NULL;

    // If full region report, clear edge structures from history
    if (pLand->Description.FindSubStr("Wanted") != -1)
    {
        pLand->EdgeStructs.FreeAll();
        pLand->Exits.Empty();
        pLand->CloseAllExits();
    }

    // Parse exit list and gates
    while (ReadNextLine(CurLine))
    {
        if (0==SafeCmp(EOL_SCR, CurLine.GetData()))
            continue;
        DoBreak = TRUE;
        p       = S.GetToken(CurLine.GetData(), ":(", ch);
        switch (ch)
        {
        case ':':  // Exit line: "Direction : description"
            if (0==stricmp(S.GetData(), FLAG_HDR))
            {
                pLand->FlagText[0] = SkipSpaces(p);
                pLand->FlagText[0].TrimRight(TRIM_ALL);
            }
            for (i=0; i<(int)sizeof(Directions)/(int)sizeof(const char*); i++)
                if (0==stricmp(S.GetData(), Directions[i]))
                {
                    pLand->Exits << "  " << S << " : ";
                    pPlane->ExitsCount++;
                    S = p;
                    S.TrimLeft();
                    CLand * pLandRef = NULL;
                    ParseTerrain(pLand, i, S, FALSE, &pLandRef);
                    if (pLandRef)
                    {
                        int xLocal, yLocal, zLocal;
                        LandIdToCoord(pLandRef->Id, xLocal, yLocal, zLocal);
                        pLand->SetExit(i, xLocal, yLocal);
                    }
                    DoBreak = FALSE;
                    break;
                }
            break;
        case '(': // Gate line: "There is a Gate here (Gate 18 of 35)."
            if (0==stricmp(S.GetData(), "There is a Gate here"))
            {
                CStr sCoord;

                p  = SkipSpaces(S.GetToken(p, ' '));
                p  = S.GetToken(p, ' ');  // S = 18
                no = atol(S.GetData());
                pStruct     = new CStruct;
                pStruct->Id = -no;  // Negative ID to avoid conflict with structures
                pStruct->Description = CurLine.GetData();
                pStruct->Kind        = STRUCT_GATE;
                pStruct->Attr        = gpDataHelper->GetStructAttr(pStruct->Kind.GetData(), pStruct->MaxLoad, pStruct->MinSailingPower);
                pLand->AddNewStruct(pStruct);

                p = SkipSpaces(p);
                p = SkipSpaces(S.GetToken(p, ' '));  // S = of
                p = S.GetToken(p, ' ');  // S = 35
                m_GatesCount = atol(S.GetData());
                DoBreak = FALSE;

                pGate = new CBaseObject;
                pGate->Id = no;
                if (m_Gates.Search(pGate, idx))
                {
                    delete pGate;
                    pGate = (CBaseObject*)m_Gates.At(idx);
                }
                else
                    idx = -1;

                ComposeLandStrCoord(pLand, sCoord);
                pGate->Description.Format("Gate % 4d. ", no);
                pGate->Description << pLand->TerrainType << " (" << sCoord << ")";
                pGate->Name        = pGate->Description;

                if (idx<0)
                    m_Gates.Insert(pGate);
            }
            break;
        }

        if (DoBreak)
        {
            PutLineBack(CurLine);
            goto Exit;
        }
    }

Exit:
    if (ppParsedLand)
        *ppParsedLand = pLand;

    return err;
}

//----------------------------------------------------------------------
// Creates or retrieves a unit by ID
//----------------------------------------------------------------------

CUnit * CAtlaParser::MakeUnit(long Id)
{
    CBaseObject  Dummy;
    int          idx;
    CUnit      * pUnit;

    Dummy.Id = Id;
    if (m_Units.Search(&Dummy, idx))
    {
        pUnit    = (CUnit*)m_Units.At(idx);
    }
    else
    {
        pUnit = new CUnit;
        pUnit->Id = Id;
        m_Units.Insert(pUnit);
    }

    return pUnit;
}

//----------------------------------------------------------------------
// Converts skill days to level (standard Atlantis formula)
//----------------------------------------------------------------------

long CAtlaParser::SkillDaysToLevel(long days)
{
    long level = 0;

    while (days>0)
    {
        days -= (level+1)*30;
        if (days>=0)
            level++;
    }
    return level;
}

//----------------------------------------------------------------------
// Parses a unit description from the report
// Extracts name, ID, faction, items, skills, combat spell, and flags
//----------------------------------------------------------------------

int CAtlaParser::ParseUnit(CStr& FirstLine, BOOL Join)
{
#define DOT          '.'
#define COMMA        ','
#define SEMICOLON    ';'
#define STRUCT_LIMIT ",.;"
#define SECT_ITEMS   "Items"
#define SECT_SKILLS  "Skills"
#define SECT_COMBAT  "Combatspell"

    CFaction* pFaction = NULL;
    CUnit* pUnit = NULL;
    CStr          CurLine(128);
    CStr          UnitText(128);
    CStr          UnitPrefix;
    CStr          Line(128);
    CStr          FactName;
    CStr          Section(32);
    CStr          Buf(128);
    CStr          S1(32);
    CStr          S2(32);
    CStr          N1(32);
    CStr          N2(32);
    CStr          N3(32);
    long          n1;
    const char* src = NULL;
    const char* p;
    char          Delimiter;
    char          LastDelimiter = DOT;
    char          ch;
    int           err = ERR_OK;
    int           attitude;
    BOOL          SkillsFound = FALSE;
    BOOL          Valid;

    FirstLine.TrimRight(TRIM_ALL);
    p = FirstLine.GetData();

    // Extract unit prefix (spaces and first word) - indicates own/alien unit
    while (p && *p && *p <= ' ')
        UnitPrefix << *p++;
    while (p && *p && *p > ' ')
        UnitPrefix << *p++;
    while (p && *p && *p <= ' ')
        UnitPrefix << *p++;

    UnitText = p;
    UnitText << EOL_SCR;

    // Read all lines belonging to this unit
    while (ReadNextLine(CurLine))
    {
        p = SkipSpaces(CurLine.GetData());

        if (!p || !*p)
            break;

        // Check if next unit starts
        if ((0 == strnicmp(p, HDR_UNIT_ALIEN, sizeof(HDR_UNIT_ALIEN) - 1)) ||
            (0 == strnicmp(p, HDR_UNIT_OWN, sizeof(HDR_UNIT_OWN) - 1)) ||
            (0 == strnicmp(p, HDR_STRUCTURE, sizeof(HDR_STRUCTURE) - 1))
            )
        {
            PutLineBack(CurLine);
            break;
        }

        CurLine.TrimRight(TRIM_ALL);
        UnitText.AddStr(CurLine.GetData(), CurLine.GetLength());
        UnitText.AddStr(EOL_SCR);
    }

    // Parse unit name and ID - first line always has "Name (ID)"
    p = S1.GetToken(UnitText.GetData(), '(');
    p = N1.GetToken(p, ')');
    n1 = atol(N1.GetData());
    if (n1 <= 0)
        return ERR_INV_UNIT;

    pUnit = MakeUnit(n1);
    if (m_pCurLand)
        m_pCurLand->AddUnit(pUnit);
    
    // Set unit description
    if (0 == strnicmp(SkipSpaces(UnitPrefix.GetData()), HDR_UNIT_OWN, sizeof(HDR_UNIT_OWN) - 1) ||
        UnitPrefix.GetLength() + UnitText.GetLength() > pUnit->Description.GetLength())
    {
        pUnit->Description = UnitPrefix;
        pUnit->Description << UnitText;
        pUnit->Name = S1;

        // Mark if this is our faction's unit
        pUnit->IsOurs = pUnit->IsOurs ||
            (m_CrntFactionId > 0 && pUnit->FactionId == m_CrntFactionId);

        if (pUnit->IsOurs)
        {
            pUnit->InitEndTurnDescription();
        }
    }
    
    // If unit is in a structure, set structure properties
    if (m_pCurStruct)
    {
        SetUnitProperty(pUnit, PRP_STRUCT_ID, eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(m_pCurStruct->Id)), eBoth);
        SetUnitProperty(pUnit, PRP_STRUCT_NAME, eCharPtr, m_pCurStruct->Name.GetData(), eBoth);
        if (0 == m_pCurStruct->OwnerUnitId)
        {
            m_pCurStruct->OwnerUnitId = pUnit->Id;
            SetUnitProperty(pUnit, PRP_STRUCT_OWNER, eCharPtr, YES, eBoth);
        }
    }

    // Parse faction name and ID
    src = p;
    p = strchr(src, '(');
    if (p)
    {
        while (p > src && *(p - 1) != ',')
            p--; // Position at start of faction name

        p = S1.GetToken(p, '(');
        p = N1.GetToken(p, ')');
        n1 = atol(N1.GetData());
        if (n1 > 0)
        {
            pFaction = GetFaction(n1);
            if (!pFaction)
            {
                pFaction = new CFaction;
                pFaction->Name = S1;
                pFaction->Id = n1;
                m_Factions.Insert(pFaction);
            }
            pUnit->FactionId = pFaction->Id;
            pUnit->pFaction = pFaction;

            pUnit->IsOurs = pUnit->IsOurs ||
                (m_CrntFactionId > 0 && pUnit->FactionId == m_CrntFactionId);
        }
    }

    // Determine attitude based on faction relationships
    attitude = ATT_UNDECLARED;
    if (pUnit->IsOurs && (!Join))
    {
        attitude = ATT_FRIEND2; // Our own units are most friendly
    }
    else if (pUnit->IsOurs)
    {
        attitude = gpDataHelper->GetAttitudeForFaction(m_CrntFactionId);
    }
    else if (pUnit->IsOurs || (pUnit->FactionId != 0))
    {
        attitude = gpDataHelper->GetAttitudeForFaction(pUnit->FactionId);
    }
    if ((attitude >= 0) && (attitude < ATT_UNDECLARED))
    {
        SetUnitProperty(pUnit, PRP_FRIEND_OR_FOE, eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(attitude)), eNormal);
    }

    // Parse the rest of the unit description (items, skills, combat spell)
    if (',' == *src)
        src++;
    while (src && *src)
    {
        src = SkipSpaces(Line.GetToken(src, STRUCT_LIMIT, Delimiter, TRIM_ALL));

        if (DOT == LastDelimiter)
        {
            // New section starts
            p = SkipSpaces(Section.GetToken(Line.GetData(), ':'));
            if (p)
            {
                // Remove section name from line
                Line.DelSubStr(0, p - Line.GetData());
            }
            else
            {
                // Section without name - assume Items if not Skills
                if (!SkillsFound && 0 != stricmp(SECT_SKILLS, Section.GetData()))
                    Section = SECT_ITEMS;
            }
        }

        if (0 == stricmp(SECT_ITEMS, Section.GetData()))
        {
            const void* data = NULL;
            if (pUnit)
            {
                Line.Normalize();
                // Check if this is a flag
                if (m_UnitFlagsHash.Locate(Line.GetData(), data))
                {
                    pUnit->Flags |= static_cast<unsigned long>(reinterpret_cast<intptr_t>(data));
                    pUnit->FlagsOrg |= static_cast<unsigned long>(reinterpret_cast<intptr_t>(data));
                }
            }

            // Parse item patterns:
            // S1 (N1) - faction name (already handled)
            // N1 S1 [S2] - amount item [description]
            // S1 [S2] - item [description] (amount 1)
            p = Buf.GetToken(Line.GetData(), "(", ch);
            if (!p)
                p = Buf.GetToken(Line.GetData(), "[", ch);
            switch (ch)
            {
            case '(':     // This is a faction name again
                p = N1.GetToken(p, ')');
                break;

            case '[':
                p = N1.GetInteger(Line.GetData(), Valid);
                if (N1.IsEmpty())
                    n1 = 1;
                else
                    n1 = atol(N1.GetData());

                p = S1.GetToken(p, '[', TRIM_ALL);
                p = S2.GetToken(p, ']', TRIM_ALL);

                if (!p || S2.IsEmpty() || !pUnit)
                {
                    Buf.Format("Unit description error - flag/item in line %d", m_nCurLine);
                    LOG_ERR(ERR_DESIGN, Buf.GetData());
                }
                else
                {
                    SetUnitProperty(pUnit, S2.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(n1)), eBoth);
                    // Check if this is a man property (soldiers)
                    if (gpDataHelper->IsMan(S2.GetData()))
                    {
                        // Check for leader/hero status
                        if (S1.FindSubStr(SZ_LEADER) >= 0)
                            SetUnitProperty(pUnit, PRP_LEADER, eCharPtr, SZ_LEADER, eBoth);
                        if (S1.FindSubStr(SZ_HERO) >= 0)
                            SetUnitProperty(pUnit, PRP_LEADER, eCharPtr, SZ_HERO, eBoth);
                    }
                }
                break;
            }
        }
        else if (0 == stricmp(SECT_SKILLS, Section.GetData()))
        {
            SkillsFound = TRUE;

            // Parse skill patterns:
            // S1 [S2] N1 (N2) - standard skill
            // S1 [S2] N1 (N2/N3) - Arcadia skill with study and experience
            p = S1.GetToken(Line.GetData(), '[', TRIM_ALL);
            p = S2.GetToken(p, ']', TRIM_ALL);
            p = N1.GetToken(p, '(', TRIM_ALL);
            p = N2.GetToken(p, ")/", ch, TRIM_ALL);
            if ('/' == ch)
            {
                p = N3.GetToken(p, ')', TRIM_ALL);
                m_ArcadiaSkills = TRUE;
            }

            if (0 != stricmp("none", S1.GetData()))
                if (!p || !pUnit)
                {
                    Buf.Format("Unit description error - skills in line %d", m_nCurLine);
                    LOG_ERR(ERR_DESIGN, Buf.GetData());
                }
                else
                {
                    // Set skill level property
                    Buf = S2;
                    Buf << PRP_SKILL_POSTFIX;
                    SetUnitProperty(pUnit, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(atol(N1.GetData()))), eBoth);

                    // Set skill days property
                    Buf = S2;
                    Buf << PRP_SKILL_DAYS_POSTFIX;
                    SetUnitProperty(pUnit, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(atol(N2.GetData()))), eBoth);

                    if (m_ArcadiaSkills)
                    {
                        // Arcadia III specific properties
                        long n = atol(N2.GetData());
                        unsigned long i = SkillDaysToLevel(n);

                        Buf = S2;
                        Buf << PRP_SKILL_STUDY_POSTFIX;
                        SetUnitProperty(pUnit, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(i)), eBoth);

                        n = atol(N3.GetData());
                        i = SkillDaysToLevel(n);
                        Buf = S2;
                        Buf << PRP_SKILL_EXPERIENCE_POSTFIX;
                        SetUnitProperty(pUnit, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(i)), eBoth);

                        Buf = S2;
                        Buf << PRP_SKILL_DAYS_EXPERIENCE_POSTFIX;
                        SetUnitProperty(pUnit, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(n)), eBoth);
                    }
                }
        }
        else if (0 == SafeCmpNoSpaces(SECT_COMBAT, Section.GetData()))
        {
            // Parse combat spell: S1 [S2]
            p = S1.GetToken(Line.GetData(), '[', TRIM_ALL);
            if (p)
                p = S2.GetToken(p, ']', TRIM_ALL);
            else
                S2 = Line;
            SetUnitProperty(pUnit, PRP_COMBAT, eCharPtr, S2.GetData(), eBoth);
        }

        if (SEMICOLON == Delimiter)
        {
            // Description section comes last
            SetUnitProperty(pUnit, PRP_DESCRIPTION, eCharPtr, src, eBoth);
            break;
        }
        LastDelimiter = Delimiter;
    }

    // Check if unit can see advanced resources
    LookupAdvancedResourceVisibility(pUnit, m_pCurLand);

    return err;
}

//----------------------------------------------------------------------
// Parses a structure description from the report
//----------------------------------------------------------------------

int CAtlaParser::ParseStructure(CStr& FirstLine)
{
    CStr         S;
    CStr         Name;
    const char* p;
    long         id;
    char         ch;
    CStr         CurLine(64);
    CStr         TmpDescr(64);
    CStr         Kind;
    int          Location = NO_LOCATION;
    int          i;

    if (!m_pCurLand)
        return ERR_OK;  // Просто возвращаем, без goto

    // Format: "+ Structure [ID] : Type"
    p = FirstLine.GetData();
    p = Name.GetToken(p, '[');
    p = S.GetToken(p, ']');
    id = atol(S.GetData());
    if (0 == id)
        return ERR_OK;  // Просто возвращаем, без goto

    TmpDescr = FirstLine;
    TmpDescr.TrimRight(TRIM_ALL);

    // Read additional description lines
    while (ReadNextLine(CurLine))
    {
        const char* s = SkipSpaces(CurLine.GetData());
        if (!s || !*s)
            break;

        // Stop if next unit/structure starts
        if (strchr(STRUCT_UNIT_START, *s))
        {
            PutLineBack(CurLine);
            break;
        }

        CurLine.TrimRight(TRIM_ALL);
        TmpDescr << EOL_SCR << CurLine;
    }

    // Parse type and location (Arcadia ships at hex edges)
    p = S.GetToken(TmpDescr.GetData(), ':');
    p = Kind.GetToken(p, ",.;", ch);
    Name << " [" << id << "]";

    // Check for Arcadia III ships at hex edges
    p = strchr(Kind.GetData(), '(');
    if (p && *p)
    {
        S.GetToken(p + 1, ')');
        S.Normalize();
        for (i = 0; i < (int)sizeof(LocationsShipsArcadia) / (int)sizeof(const char*); i++)
            if (0 == stricmp(S.GetData(), LocationsShipsArcadia[i]))
            {
                Location = i;
                break;
            }
        i = p - Kind.GetData();
        Kind.DelSubStr(i, Kind.GetLength() - i);
        Kind.TrimRight(TRIM_ALL);
    }

    // Теперь создаем структуру (после всех проверок)
    CStruct* pStruct = new CStruct;
    pStruct->Id = id;
    pStruct->Name = &Name.GetData()[sizeof(HDR_STRUCTURE) - 1];
    pStruct->Description = TmpDescr;
    pStruct->Kind = Kind;

    // Получаем атрибуты структуры
    long maxLoad, minSailingPower;
    long attr = gpDataHelper->GetStructAttr(pStruct->Kind.GetData(), maxLoad, minSailingPower);
    pStruct->Attr = attr;
    pStruct->MaxLoad = maxLoad;
    pStruct->MinSailingPower = minSailingPower;

    pStruct->Location = Location;

    // Mark bad roads (needs repair or decaying)
    if (pStruct->Attr & (SA_ROAD_N | SA_ROAD_NE | SA_ROAD_SE | SA_ROAD_S | SA_ROAD_SW | SA_ROAD_NW))
        if (pStruct->Description.FindSubStr("needs") > 0 || pStruct->Description.FindSubStr("decay") > 0)
            pStruct->Attr |= SA_ROAD_BAD;

    m_pCurStruct = m_pCurLand->AddNewStruct(pStruct);

    return ERR_OK;
}

//----------------------------------------------------------------------
// Returns full land coordinate string for display
//----------------------------------------------------------------------

wxString CAtlaParser::getFullStrLandCoord(CLand * pLand)
{
    CStr sCoord;
    wxString s;

    ComposeLandStrCoord(pLand, sCoord);
    s.Printf(wxT("%s (%s)"), pLand->TerrainType.GetData(), sCoord.GetData());
    if (!pLand->CityName.IsEmpty())
        s+= wxString::Format(wxT(" contains %s"), pLand->CityName.GetData());
    return s;
}

//----------------------------------------------------------------------
// Links a shaft structure between two lands
// Updates the description of the source shaft with destination info
//----------------------------------------------------------------------

bool CAtlaParser::LinkShaft(CLand * pLand, CLand * pLandDest, int structIdx)
{
    CStruct    * pStruct;
    CStr         S, T;
    int          n;
    const char * p;

    if (pLand && pLandDest && structIdx >=0 && structIdx < pLand->Structs.Count())
    {
        pStruct = (CStruct*)pLand->Structs.At(structIdx);
        if (pStruct->Attr & SA_SHAFT)
        {
            ComposeLandStrCoord(pLandDest, S);
            // Remove trailing dot if present
            if ('.' ==  pStruct->Description.GetData()[ pStruct->Description.GetLength()-1])
                 pStruct->Description.DelCh( pStruct->Description.GetLength()-1);

            // Remove any previous links
            n = pStruct->Description.FindSubStr("; links to");
            if (n > 0) pStruct->Description.DelSubStr(n, 9999);

            // Get start of last line to decide if we need new line
            p = strrchr( pStruct->Description.GetData(), '\n');
            n = p ? ( pStruct->Description.GetData()-p) : 0;

            T.Empty();
            T << "; links to (" << S << ").";
            if (n+T.GetLength() > 64)
                 pStruct->Description << EOL_SCR << "  ";
            pStruct->Description << T;
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------
// Processes all pending shaft links from m_LandsToBeLinked
//----------------------------------------------------------------------

void CAtlaParser::SetShaftLinks()
{
    CLand      * pLand;
    CLand      * pLandDest;
    CStruct    * pStruct;
    int          i,j;
    EValueType   type;
    const void * value;
    int          subStrResult;

    for (i=0; i<m_LandsToBeLinked.Count(); i++)
    {
        pLand = (CLand*)m_LandsToBeLinked.At(i);

        if (!pLand->GetProperty(PRP_LAND_LINK, type, value, eOriginal) || eLong!=type)
            continue;

        // Find first shaft not yet linked to this destination
        for (j=0; j<pLand->Structs.Count(); j++)
        {
            pStruct = (CStruct*)pLand->Structs.At(j);
            if (pStruct->Attr & SA_SHAFT)
            {
                pLandDest = GetLand(static_cast<long>(reinterpret_cast<intptr_t>(value)));
                subStrResult = pStruct->Description.FindSubStr("links");
                if (pLandDest && subStrResult >= 0 && GetLandFlexible(wxString::FromUTF8(pStruct->Description.GetData())) == pLandDest)
                    break; // Already linked

                if (pLandDest && subStrResult < 0)
                {
                    LinkShaft(pLand, pLandDest, j);
                    break;
                }
            }
        }
    }
    m_LandsToBeLinked.DeleteAll();
}

//----------------------------------------------------------------------
// Applies sailing events to appropriate units
//----------------------------------------------------------------------

void CAtlaParser::ApplySailingEvents()
{
    int                ne, np, nl, nu, idx;
    CBaseObject      * pSailEvent;
    CPlane           * pPlane;
    CLand            * pLand;
    CUnit            * pUnit;
    EValueType         type, type2;
    const void       * value, * value2;

    if (0 == m_TempSailingEvents.Count())
        return;

    for (np=0; np<m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_Planes.At(np);

        for (nl=0; nl<pPlane->Lands.Count(); nl++)
        {
            pLand = (CLand*)pPlane->Lands.At(nl);

            for (ne=0; ne<m_TempSailingEvents.Count(); ne++)
            {
                pSailEvent = (CBaseObject*)m_TempSailingEvents.At(ne);

                if (pLand->Structs.Search(pSailEvent, idx))
                {
                    // Find captain of this ship
                    for (nu=0; nu<pLand->Units.Count(); nu++)
                    {
                        pUnit = (CUnit*)pLand->Units.At(nu);
                        if (pUnit->GetProperty(PRP_STRUCT_ID   , type , value , eOriginal) && eLong==type &&
                            pUnit->GetProperty(PRP_STRUCT_OWNER, type2, value2, eOriginal) && eCharPtr==type2)
                            if (pSailEvent->Id == static_cast<long>(reinterpret_cast<intptr_t>(value)) && value2 && *((char*)value2))
                            {
                                pUnit->Events << pSailEvent->Description;
                            }
                    }
                }
            }
        }
    }
    m_TempSailingEvents.FreeAll();
}

//----------------------------------------------------------------------
// Parses skills section from report
//----------------------------------------------------------------------

int CAtlaParser::ParseSkills()
{
    CStr             CurLine(64);
    CStr             OneSkill(128);
    CStr             S;
    const char     * p;
    CShortNamedObj * pSkill;

    while (ReadNextLine(CurLine))
    {
        CurLine.TrimRight(TRIM_ALL);
        if (CurLine.IsEmpty())
        {
            if (!OneSkill.IsEmpty())
            {
                pSkill = new CShortNamedObj;

                // Format: "Skill Name [CODE] Level: description"
                p = OneSkill.GetData();
                p = S.GetToken(p, '[');     pSkill->Name        = S;
                p = S.GetToken(p, ']');     pSkill->ShortName   = S;
                p = S.GetToken(p, ':');     pSkill->Level       = atol(S.GetData());

                pSkill->Description = OneSkill;
                pSkill->Name.GetToken(OneSkill.GetData(), ':');

                m_Skills.Insert(pSkill);
                OneSkill.Empty();
            }
        }
        else
        {
            // Check format validity
            if (OneSkill.IsEmpty())
            {
                p = S.GetToken(CurLine.GetData(), '[');
                if (!p)
                    break;
                p = S.GetToken(p, ']');
                if (!p)
                    break;
                p = S.GetToken(p, ':');
                if (!p)
                    break;
                if (atol(S.GetData()) <= 0)
                    break;
            }

            OneSkill << CurLine << EOL_SCR;
        }
    }
    CurLine << EOL_FILE;
    PutLineBack(CurLine);

    return 0;
}

//----------------------------------------------------------------------
// Parses items section from report
//----------------------------------------------------------------------

int CAtlaParser::ParseItems()
{
    CStr             CurLine(64);
    CStr             OneItem(128);
    CStr             S;
    const char     * p;
    CShortNamedObj * pItem;

    while (ReadNextLine(CurLine))
    {
        CurLine.TrimRight(TRIM_ALL);
        if (CurLine.IsEmpty())
        {
            if (!OneItem.IsEmpty())
            {
                pItem = new CShortNamedObj;

                // Format: "Item Name [CODE], description"
                p = OneItem.GetData();
                p = S.GetToken(p, '[');     pItem->Name        = S;
                p = S.GetToken(p, ']');     pItem->ShortName   = S;

                pItem->Description = OneItem;
                pItem->Name.GetToken(OneItem.GetData(), ',');

                m_Items.Insert(pItem);
                OneItem.Empty();
            }
        }
        else
        {
            // Check format validity
            if (OneItem.IsEmpty())
            {
                p = S.GetToken(CurLine.GetData(), '[');
                if (!p)
                    break;
                p = S.GetToken(p, ']');
                if (!p)
                    break;
                p = S.GetToken(p, ',');
                if (!p)
                    break;
                if (!S.IsEmpty())
                    break;
            }

            OneItem << CurLine << EOL_SCR;
        }
    }
    CurLine << EOL_FILE;
    PutLineBack(CurLine);

    return 0;
}

//----------------------------------------------------------------------
// Parses objects section from report
//----------------------------------------------------------------------

int CAtlaParser::ParseObjects()
{
    CStr             CurLine(64);
    CStr             OneItem(128);
    CStr             S;
    const char     * p;
    CShortNamedObj * pItem;
    char             ch;

    while (ReadNextLine(CurLine))
    {
        CurLine.TrimRight(TRIM_ALL);
        if (CurLine.IsEmpty())
        {
            if (!OneItem.IsEmpty())
            {
                pItem = new CShortNamedObj;

                // Format: "Object Name: description"
                p = OneItem.GetData();
                p = S.GetToken(p, ':');
                pItem->Name        = S;
                pItem->ShortName   = S;
                pItem->Description = OneItem;

                m_Objects.Insert(pItem);
                OneItem.Empty();
            }
        }
        else
        {
            // Check format validity
            if (OneItem.IsEmpty())
            {
                p = SkipSpaces(S.GetToken(CurLine.GetData(), ':'));
                if (!p)
                    break;
                p = SkipSpaces(S.GetToken(p, " \t", ch));
                if (0!=stricmp(S.GetData(), "This"))
                    break;
                p = SkipSpaces(S.GetToken(p, " \t", ch));
                if (0!=stricmp(S.GetData(), "is"))
                    break;
            }

            OneItem << CurLine << EOL_SCR;
        }
    }
    CurLine << EOL_FILE;
    PutLineBack(CurLine);

    return 0;
}

//----------------------------------------------------------------------
// Main parsing loop - dispatches to appropriate section parsers
//----------------------------------------------------------------------

int CAtlaParser::ParseLines(BOOL Join)
{
    int        err      = ERR_OK;
    CStr       CurLine(64);
    CStr       sErr;
    const char * p;

    while (ReadNextLine(CurLine))
    {
        p = SkipSpaces(CurLine.GetData());

        if      (0==strnicmp(p, HDR_FACTION           , sizeof(HDR_FACTION)-1 ))
            err = ParseFactionInfo(TRUE, Join);

        else if (0==strnicmp(p, HDR_FACTION_STATUS    , sizeof(HDR_FACTION_STATUS)-1 ))
            err = ParseFactionInfo(FALSE, Join);

        else if (0==strnicmp(p, HDR_EVENTS            , sizeof(HDR_EVENTS)-1 ))
            err = ParseEvents();

        else if (0==strnicmp(p, HDR_EVENTS_2          , sizeof(HDR_EVENTS_2)-1 ))
            err = ParseImportantEvents();

        else if (0==strnicmp(p, HDR_ERRORS            , sizeof(HDR_ERRORS)-1 ))
            err = ParseErrors();

        else if (0==strnicmp(p, HDR_SILVER            , sizeof(HDR_SILVER)-1 ))
            err = ParseUnclSilver(CurLine);

        else if (0==strnicmp(p, HDR_ATTITUDES         , sizeof(HDR_ATTITUDES)-1 ))
            err = ParseAttitudes(CurLine, Join);

        else if (0==strnicmp(p, HDR_UNIT_OWN          , sizeof(HDR_UNIT_OWN)-1 ))
            err = ParseUnit(CurLine, Join);

        else if (0==strnicmp(p, HDR_UNIT_ALIEN        , sizeof(HDR_UNIT_ALIEN)-1 ))
            err = ParseUnit(CurLine, Join);

        else if (0==strnicmp(p, HDR_STRUCTURE         , sizeof(HDR_STRUCTURE)-1 ))
            err = ParseStructure(CurLine);

        else if (0==strnicmp(p, HDR_BATTLES           , sizeof(HDR_BATTLES)-1 ))
            err = ParseBattles();

        else if (0==strnicmp(p, HDR_SKILLS            , sizeof(HDR_SKILLS)-1 ))
            err = ParseSkills();

        else if (0==strnicmp(p, HDR_ITEMS             , sizeof(HDR_ITEMS)-1 ))
            err = ParseItems();

        else if (0==strnicmp(p, HDR_OBJECTS           , sizeof(HDR_OBJECTS)-1 ))
            err = ParseObjects();

        else if (0==strnicmp(p, HDR_ORDER_TEMPLATE    , sizeof(HDR_ORDER_TEMPLATE)-1 ))
            err = LoadOrders(*m_pSource, m_CrntFactionId, FALSE); // This is a point of no return

        else
            err = ParseTerrain(NULL, 0, CurLine, TRUE, NULL);

        if (ERR_OK!=err)
        {
            sErr.Empty();
            sErr << "Report parsing error in line " << (long)m_nCurLine  << EOL_SCR
                 << "\"" << CurLine << "\"" << "." << EOL_SCR;

            if (ERR_INV_TURN==err)
                sErr << EOL_SCR << "Merged reports must be from the same turn. This is intended for merging ally reports.";

            LOG_ERR(ERR_PARSE, sErr.GetData());

            if (ERR_INV_TURN==err)
                break;
        }
    }

    return ERR_OK;
}

//----------------------------------------------------------------------
// Main entry point for parsing a report file
//----------------------------------------------------------------------

int CAtlaParser::ParseRep(const char * FNameIn, BOOL Join, BOOL IsHistory)
{
    m_pCurLand   = NULL;
    m_pCurStruct = NULL;
    m_ParseErr   = ERR_NOTHING;
    m_pSource    = new CFileReader;
    m_JoiningRep = Join;
    m_IsHistory  = IsHistory;
    m_CrntFactionId = 0;
    m_CrntFactionPwd.Empty();
    m_nCurLine   = 0;

    if (Join)
        m_FactionInfo << EOL_SCR << EOL_SCR << "-----------------------------------------------"
                      << EOL_SCR << EOL_SCR;

    if (!Join)
        m_YearMon = 0;
    m_CurYearMon  = 0;

    if (!m_pSource->Open(FNameIn))
    {
        m_ParseErr = ERR_FOPEN;
        goto Done;
    }

    m_ParseErr = ParseLines(Join);

    ApplyLandFlags();
    SetExitFlagsAndTropicZone();
    SetShaftLinks();
    ApplySailingEvents();

Done:
    m_pSource->Close();
    delete m_pSource;
    m_pSource = NULL;
    return m_ParseErr;
}

//----------------------------------------------------------------------
// Sets exit flags and saves tropic zone information
//----------------------------------------------------------------------

void CAtlaParser::SetExitFlagsAndTropicZone()
{
    int           np;
    CPlane      * pPlane;

    for (np=0; np<m_Planes.Count(); np++)
    {
        pPlane = (CPlane*)m_Planes.At(np);

        // Save tropic zone if we have current year/month
        if (!m_IsHistory && m_CurYearMon>0 && pPlane->TropicZoneMin <= pPlane->TropicZoneMax)
            gpDataHelper->SetTropicZone(pPlane->Name.GetData(), pPlane->TropicZoneMin, pPlane->TropicZoneMax);
    }
}

//----------------------------------------------------------------------
// Helper for formatting multi-line text with indentation
//----------------------------------------------------------------------

void AddTabbed(CStr & Dest, const char * Src, int Offs)
{
#define SPC "  "
    int  i;
    int  n;
    CStr Spc;

    for (i=0; i<Offs; i++)
        Spc << SPC;
    n = Spc.GetLength();
    Dest << Spc;

    while (Src && *Src)
    {
        if ( (n>=64) && (' '==*Src) )
        {
            Dest << EOL_SCR << Spc << SPC;
            n = Spc.GetLength() + sizeof(SPC) - 1;
        }
        Dest << *Src;
        Src++;
        n++;
    }
    Dest << EOL_SCR;
}

//----------------------------------------------------------------------
// Gets list of units in a specific hex
//----------------------------------------------------------------------

void CAtlaParser::GetUnitList(CCollection * pResultColl, int x, int y, int z)
{
    CLand * pLand;
    int     i;

    pLand = GetLand(x, y, z);
    if (pLand)
        for (i=0; i<pLand->Units.Count(); i++)
            pResultColl->Insert(pLand->Units.At(i));
}

//-------------------------------------------------------------
// Gets faction by ID
//-------------------------------------------------------------

CFaction * CAtlaParser::GetFaction(int id)
{
    CBaseObject  DummyFaction;
    int          idx;

    DummyFaction.Id = id;
    if (m_Factions.Search(&DummyFaction, idx))
        return (CFaction*)m_Factions.At(idx);
    else
        return NULL;
}

//--------------------------------------------------------------------------
// Checks if a land exit is closed (impassable)
//--------------------------------------------------------------------------

bool CAtlaParser::IsLandExitClosed(CLand * pLand, int direction) const
{
    if (!pLand) return false;

    if (pLand->xExit[direction] == EXIT_CLOSED)
    {
        return true;
    }
    if (pLand->xExit[direction] == EXIT_MAYBE)
    {
        // Try to expand the map and see if we can get more information
        int x, y, z;
        LandIdToCoord(pLand->Id, x, y, z);
        ExtrapolateLandCoord(x, y, z, direction);
        pLand = GetLand(x, y, z, TRUE);
        if (pLand)
        {
            if (pLand->xExit[(direction+3)%6] == EXIT_CLOSED)
            {
                return true;
            }
        }
    }
    return false;
}

//--------------------------------------------------------------------------
// Normalizes X coordinate for wrapped worlds
//--------------------------------------------------------------------------

int  CAtlaParser::NormalizeHexX(int NoX, CPlane * pPlane) const
{
    if (pPlane && pPlane->Width>0)
    {
        while (NoX < pPlane->WestEdge)
            NoX += pPlane->Width;
        while (NoX > pPlane->EastEdge)
            NoX -= pPlane->Width;
    }
    return NoX;
}

//-------------------------------------------------------------
// Extrapolates coordinates in a given direction
//-------------------------------------------------------------

void CAtlaParser::ExtrapolateLandCoord(int &x, int &y, int z, int direction) const
{
    // Move on hex grid
    switch (direction % 6)
    {
        case North     : y -= 2;     break;
        case Northeast : y--; x++;   break;
        case Southeast : y++; x++;   break;
        case South     : y += 2;     break;
        case Southwest : y++; x--;   break;
        case Northwest : y--; x--;   break;
    }

    CPlane * pPlane = (CPlane*)m_Planes.At(z);
    x = NormalizeHexX(x, pPlane);
}

//-------------------------------------------------------------
// Gets neighboring land in a given direction
//-------------------------------------------------------------

CLand * CAtlaParser::GetLandExit(CLand * pLand, int direction) const
{
    if (pLand->xExit[direction] == EXIT_MAYBE)
    {
        // Try to expand the map
        int x, y, z;
        LandIdToCoord(pLand->Id, x, y, z);
        ExtrapolateLandCoord(x, y, z, direction);
        return GetLand(x, y, z, FALSE);
    }
    if (pLand->xExit[direction] == EXIT_CLOSED)
    {
        return NULL;
    }
    return GetLand(pLand->xExit[direction], pLand->yExit[direction], pLand->pPlane->Id, TRUE);
}

//-------------------------------------------------------------
// Gets land by coordinates
//-------------------------------------------------------------

CLand * CAtlaParser::GetLand(int x, int y, int nPlane, BOOL AdjustForEdge) const
{
    char     dummy[sizeof(CLand)];
    int      idx;
    CPlane * pPlane;

    pPlane = (CPlane*)m_Planes.At(nPlane);
    if (!pPlane)
        return NULL;

    if (AdjustForEdge)
    {
        x = NormalizeHexX(x, pPlane);
    }

    ((CLand*)&dummy)->Id = LandCoordToId(x,y, pPlane->Id);

    if (pPlane->Lands.Search(&dummy, idx))
        return (CLand*)pPlane->Lands.At(idx);
    else
        return NULL;
}

//-------------------------------------------------------------
// Gets land by ID
//-------------------------------------------------------------

CLand * CAtlaParser::GetLand(long LandId) const
{
    int x, y, z;

    LandIdToCoord(LandId, x, y, z);
    return GetLand(x, y, z, FALSE);
}

//-------------------------------------------------------------
// Composes coordinate string for a land
//-------------------------------------------------------------

void CAtlaParser::ComposeLandStrCoord(CLand * pLand, CStr & LandStr)
{
    int      x, y, z;
    CPlane * pPlane;

    LandStr.Empty();
    if (!pLand)
        return;

    LandIdToCoord(pLand->Id, x, y, z);
    pPlane = (CPlane*)m_Planes.At(z);
    if (!pPlane)
        return;

    LandStr << (long)x << "," << (long)y;
    if (0!=SafeCmp(pPlane->Name.GetData(), DEFAULT_PLANE))
        LandStr << "," << pPlane->Name.GetData();
}

//-------------------------------------------------------------
// Converts coordinate string to land ID
//-------------------------------------------------------------

BOOL CAtlaParser::LandStrCoordToId(const char * landcoords, long & id) const
{
    CStr                 S;
    long                 x, y;
    CPlane             * pPlane;
    CBaseObject          Dummy;
    int                  i;

    // Format: "xxx,yyy[,somewhere]"
    landcoords = S.GetToken(landcoords, ',');
    if (!IsInteger(S.GetData()))
        return FALSE;
    x = atol(S.GetData());

    landcoords = S.GetToken(landcoords, ',');
    if (!IsInteger(S.GetData()))
        return FALSE;
    y = atol(S.GetData());

    if ( (NULL==landcoords) || (0==*landcoords) )
        landcoords = DEFAULT_PLANE;

    Dummy.Name = landcoords;
    if (m_PlanesNamed.Search(&Dummy, i))
        pPlane = (CPlane*)m_PlanesNamed.At(i);
    else
        return FALSE;

    id = LandCoordToId(x, y, pPlane->Id);
    return TRUE;
}

//-------------------------------------------------------------
// Gets land by coordinate string
//-------------------------------------------------------------

CLand * CAtlaParser::GetLand(const char * landcoords) const
{
    long id;

    if (LandStrCoordToId(landcoords, id))
        return GetLand(id);
    else
        return NULL;
}

//----------------------------------------------------------------------
// Composes products line for a land
//----------------------------------------------------------------------

void CAtlaParser::ComposeProductsLine(CLand * pLand, const char * eol, CStr & S)
{
    CStr       Line(64);
    int        i;
    CProduct * pProd;

    if (pLand->Products.Count() > 0)
    {
        S << "  Products:";
        for (i=0; i<pLand->Products.Count(); i++)
        {
            pProd = (CProduct*)pLand->Products.At(i);

            if (pProd->Amount>=0)
            {
                S << " " << pProd->Amount << " " << pProd->LongName << " ["<< pProd->ShortName << "]";
            }
            else
            {
                S << " none";
            }

            if (pLand->Products.Count()-1 == i)
            {
                S << ".";
            }
            else
            {
                S << ",";
            }
        }

        S << eol;
    }
}

//-------------------------------------------------------------
// Saves one hex to file (for history or export)
//-------------------------------------------------------------

BOOL CAtlaParser::SaveOneHex(CFileWriter & Dest, CLand * pLand, CPlane *, SAVE_HEX_OPTIONS * pOptions)
{
    int                i;
    int                g;
    CStr               sLine (128);
    CStruct          * pStruct;
    const char       * p;
    BOOL               TurnNoMarkerWritten = FALSE;
    CUnit            * pUnit;
    EValueType         type;
    const void       * value;

    p = pLand->Description.GetData();
    while (p)
    {
        p = sLine.GetToken(p, '\n', TRIM_NONE);
        sLine.TrimRight(TRIM_ALL);

        // Add Atlaclient turn marker if requested and not already present
        if (pOptions->WriteTurnNo > 0 && !TurnNoMarkerWritten && sLine.GetLength() > 20)
        {
            BOOL         SkipIt = FALSE;
            const char * pLocal = sLine.GetData();
            while (*pLocal)
            {
                if ('-' != *pLocal)
                {
                    SkipIt = TRUE;
                    break;
                }
                pLocal++;
            }
            if (!SkipIt)
            {
                sLine << ";" << pOptions->WriteTurnNo;
                TurnNoMarkerWritten = TRUE;
            }
        }

        sLine << EOL_FILE;
        Dest.WriteBuf(sLine.GetData (), sLine.GetLength ());
    }

    sLine.Empty();

    if (pOptions->SaveResources)
        ComposeProductsLine(pLand, EOL_FILE, sLine);

    // Save exits with proper line endings
    wxString exits = pLand->Exits.GetData();
    exits.Replace("\r\n", "\n", true);
    exits.Replace("\n", EOL_FILE, true);

    sLine << EOL_FILE << "Exits:" << EOL_FILE ;
    sLine << exits.c_str();

    sLine.TrimRight(TRIM_ALL);
    sLine << EOL_FILE << EOL_FILE;

    Dest.WriteBuf(sLine.GetData (), sLine.GetLength());
    sLine.Empty();

    // Save units not in structures
    if (pOptions->SaveUnits)
        for (i=0; i<pLand->UnitsSeq.Count(); i++)
        {
            pUnit = (CUnit *)pLand->UnitsSeq.At(i);
            if (!IS_NEW_UNIT(pUnit) && !pUnit->GetProperty(PRP_STRUCT_ID, type, value, eOriginal) )
            {
                sLine << pUnit->Description;
                sLine.TrimRight(TRIM_ALL);
                sLine << EOL_FILE;
            }
        }

    // Save structures and units inside them
    for (g=0; g<pLand->Structs.Count(); g++)
    {
        pStruct = (CStruct*)pLand->Structs.At(g);
        if (0==(SA_MOBILE & pStruct->Attr) && pOptions->AlwaysSaveImmobStructs // Save history
            || pOptions->SaveStructs || pOptions->SaveUnits)
        {
            sLine << EOL_FILE;
            sLine << pStruct->Description.GetData();
            sLine.TrimRight(TRIM_ALL);
            sLine << EOL_FILE;

            // Units inside this structure
            if (pOptions->SaveUnits)
                for (i=0; i<pLand->UnitsSeq.Count(); i++)
                {
                    pUnit = (CUnit *)pLand->UnitsSeq.At(i);
                    if (!IS_NEW_UNIT(pUnit) && pUnit->GetProperty(PRP_STRUCT_ID, type, value, eOriginal) && eLong==type && static_cast<long>(reinterpret_cast<intptr_t>(value))==pStruct->Id)
                    {
                        sLine << pUnit->Description;
                        sLine.TrimRight(TRIM_ALL);
                        sLine << EOL_FILE;
                    }
                }
            Dest.WriteBuf(sLine.GetData(), sLine.GetLength());
            sLine.Empty();
        }
    }

    sLine << EOL_FILE << EOL_FILE;
    Dest.WriteBuf(sLine.GetData (), sLine.GetLength());

    return TRUE;
}

//-------------------------------------------------------------
// Saves orders for a faction to file
// Handles FORM blocks for new units and @prefix for repeated commands
//-------------------------------------------------------------

int CAtlaParser::SaveOrders(const char* FNameOut, const char* password, BOOL decorate, int factid)
{
    int           i, n, x;
    CUnit* pUnit;
    CFileWriter   Dest;
    CStr          S(64), S1(64);
    int           err = ERR_OK;
    const char* p;
    CLand* pLand, DummyLand;
    CPlane* pPlane;
    char          buf[64];
    struct tm     t;
    time_t        now;
    CFaction* pFaction = NULL;

    if (Dest.Open(FNameOut))
    {
        pFaction = GetFaction(factid);

        // Write orders header
        S.Empty();
        S << "#atlantis " << (long)factid << " \"" << password << '\"' << EOL_FILE << EOL_FILE;
        if (pFaction)
        {
            time(&now);
            t = *localtime(&now);
            i = m_YearMon % 100 - 1;
            if ((i < 12) && (i >= 0))
                sprintf(buf, "%s year %ld,  %s", Monthes[i], m_YearMon / 100, asctime(&t));
            else
                sprintf(buf, "%02ld/%02ld,  %s", m_YearMon % 100, m_YearMon / 100, asctime(&t));
            S << ORDER_CMNT << pFaction->Name << " orders for " << buf << EOL_FILE << EOL_FILE;
        }
        Dest.WriteBuf(S.GetData(), S.GetLength());

        std::map<long, wxString> FormOrders; // Maps parent unit number -> FORM commands block

        // Process all our units, organized by region
        for (n = 0; n < m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)m_Planes.At(n);
            for (i = 0; i < pPlane->Lands.Count(); i++)
            {
                pLand = (CLand*)pPlane->Lands.At(i);

                // First pass: collect orders for new units and store them keyed by parent unit ID
                for (x = 0; x < pLand->Units.Count(); x++)
                {
                    pUnit = (CUnit*)pLand->Units.At(x);
                    if (pUnit->FactionId == factid && IS_NEW_UNIT(pUnit))
                    {
                        // Retrieve parent unit ID from the unit's property (set in SplitUnit)
                        EValueType type;
                        const void* value = nullptr;
                        long parentUnitNumber = 0;

                        if (pUnit->GetProperty(PRP_PARENT_UNIT_ID, type, value, eOriginal) &&
                            type == eLong && value)
                        {
                            parentUnitNumber = static_cast<long>(reinterpret_cast<intptr_t>(value));
                        }

                        if (parentUnitNumber > 0)
                        {
                            std::map<long, wxString>::iterator mi = FormOrders.find(parentUnitNumber);
                            wxString formBlock;
                            if (mi != FormOrders.end())
                            {
                                formBlock = mi->second;
                            }

                            // Check if this new unit has the repeated property (for @FORM commands)
                            EValueType repType;
                            const void* repValue = nullptr;
                            BOOL isRepeated = FALSE;

                            if (pUnit->GetProperty(PRP_FORM_REPEATED, repType, repValue, eOriginal) &&
                                repValue && eCharPtr == repType)
                            {
                                isRepeated = TRUE;
                            }

                            // Build the complete FORM command block
                            formBlock << wxString::FromUTF8(EOL_SCR);
                            if (isRepeated)
                                formBlock << wxT("@");
                            formBlock << wxT("FORM ") << REVERSE_NEW_UNIT_ID(pUnit->Id) << wxString::FromUTF8(EOL_SCR)
                                << wxString::FromUTF8(pUnit->Orders.GetData());
                            formBlock.Trim();
                            formBlock << wxString::FromUTF8(EOL_SCR) << wxT("END") << wxString::FromUTF8(EOL_SCR);

                            // Store the block keyed by the parent unit's number
                            FormOrders[parentUnitNumber] = formBlock;
                        }
                        else
                        {
                            wxString landDescr = wxString::FromUTF8(pLand->Description.GetData());
                            wxString sErr = wxString::Format(wxT("Cannot save new unit orders in %s - parent unit ID not found."),
                                landDescr.BeforeFirst('.').c_str());
                            OrderErr(1, pUnit->Id, sErr.ToUTF8(), pUnit->Name.GetData());
                        }
                    }
                }

                // Second pass: write orders for regular units and insert FORM blocks after their parents
                bool LandDecorationShown = false;
                for (x = 0; x < pLand->Units.Count(); x++)
                {
                    pUnit = (CUnit*)pLand->Units.At(x);
                    if (pUnit->FactionId == factid && !IS_NEW_UNIT(pUnit))
                    {
                        if (decorate && !LandDecorationShown)
                        {
                            // Add region name as a comment for decoration
                            S1.GetToken(pLand->Description.GetData(), '\n');
                            S1.TrimRight(TRIM_ALL);
                            if (S1.FindSubStr(")") > 0)
                            {
                                S1.DelSubStr(S1.FindSubStr(")") + 1, 999);
                            }
                            S.Empty();
                            S << EOL_FILE << ORDER_CMNT << S1 << EOL_FILE;
                            Dest.WriteBuf(S.GetData(), S.GetLength());
                            LandDecorationShown = true;
                        }

                        // Write the unit header
                        S.Empty();
                        S << EOL_FILE << "unit " << pUnit->Id << " ; " << pUnit->Name << EOL_FILE;
                        Dest.WriteBuf(S.GetData(), S.GetLength());

                        // Write each order line
                        p = pUnit->Orders.GetData();
                        while (p && *p)
                        {
                            p = S.GetToken(p, '\n', TRIM_NONE);
                            S.TrimRight(TRIM_ALL);

                            // Check for FORM commands and add @ prefix if they are repeated
                            const char* cmdStart = SkipSpaces(S.GetData());
                            if (strnicmp(cmdStart, "FORM", 4) == 0 &&
                                (cmdStart[4] == ' ' || cmdStart[4] == '\t'))
                            {
                                // Extract new unit number to check if it's a repeated FORM
                                const char* pNum = cmdStart + 4;
                                while (*pNum == ' ' || *pNum == '\t') pNum++;

                                char* endPtr;
                                long newUnitNum = strtol(pNum, &endPtr, 10);

                                if (newUnitNum > 0)
                                {
                                    long absoluteId = NEW_UNIT_ID(newUnitNum, factid);

                                    CBaseObject Dummy;
                                    Dummy.Id = absoluteId;
                                    int idx;
                                    if (m_Units.Search(&Dummy, idx))
                                    {
                                        CUnit* pTargetUnit = (CUnit*)m_Units.At(idx);
                                        EValueType repType;
                                        const void* repValue = nullptr;

                                        // Check for repeated FORM property
                                        if (pTargetUnit->GetProperty(PRP_FORM_REPEATED, repType, repValue, eOriginal) &&
                                            repValue && eCharPtr == repType)
                                        {
                                            // This is a repeated FORM - add @ prefix when writing
                                            Dest.WriteBuf("@", 1);
                                        }
                                    }
                                }
                            }

                            S << EOL_FILE;
                            Dest.WriteBuf(S.GetData(), S.GetLength());
                        }

                        // After writing the unit's own orders, check if it has any FORM blocks to insert
                        std::map<long, wxString>::iterator mi = FormOrders.find(pUnit->Id);
                        if (mi != FormOrders.end())
                        {
                            // Insert the FORM block for new units created by this parent unit
                            p = mi->second.ToUTF8();
                            while (p && *p)
                            {
                                p = S.GetToken(p, '\n', TRIM_NONE);
                                S.TrimRight(TRIM_ALL);
                                S << EOL_FILE;
                                Dest.WriteBuf(S.GetData(), S.GetLength());
                            }
                            FormOrders.erase(mi);
                        }
                    }
                }

                // If there are any FORM blocks left without a parent unit in this region, report an error
                if (!FormOrders.empty())
                {
                    wxString landDescr = wxString::FromUTF8(pLand->Description.GetData());
                    wxString s = wxString::Format(wxT("Cannot save new unit orders in %s - parent unit not found in this region."),
                        landDescr.BeforeFirst('.').c_str());
                    OrderErr(1, 0, s.ToUTF8(), "");
                    FormOrders.clear();
                }
            }
        }

        OrderErrFinalize();
        S.Empty();
        S << EOL_FILE << "#end" << EOL_FILE;
        Dest.WriteBuf(S.GetData(), S.GetLength());
        Dest.Close();
    }
    else
        err = ERR_FOPEN;

    return err;
}

//-------------------------------------------------------------
// Counts men for a faction in every hex
// Used for display purposes
//-------------------------------------------------------------

void CAtlaParser::CountMenForTheFaction(int FactionId)
{
    int           nPlane;
    int           nLand;
    int           nUnit;
    CPlane* pPlane;
    CLand* pLand;
    CUnit* pUnit;
    long          nMen, x;
    EValueType    type;
    const void* pTempValue = nullptr;

    for (nPlane = 0; nPlane < m_Planes.Count(); nPlane++)
    {
        pPlane = (CPlane*)m_Planes.At(nPlane);
        for (nLand = 0; nLand < pPlane->Lands.Count(); nLand++)
        {
            pLand = (CLand*)pPlane->Lands.At(nLand);

            pTempValue = nullptr;
            if (!pLand->GetProperty(PRP_SEL_FACT_MEN, type, pTempValue, eNormal) || (eLong != type) || !pTempValue)
            {
                nMen = 0;
            }
            else
            {
                nMen = static_cast<long>(reinterpret_cast<intptr_t>(pTempValue));
            }

            for (nUnit = 0; nUnit < pLand->Units.Count(); nUnit++)
            {
                pUnit = (CUnit*)pLand->Units.At(nUnit);
                if (FactionId == pUnit->FactionId)
                {
                    pTempValue = nullptr;
                    if (pUnit->GetProperty(PRP_MEN, type, pTempValue, eNormal) && (eLong == type) && pTempValue)
                    {
                        x = static_cast<long>(reinterpret_cast<intptr_t>(pTempValue));
                        nMen += x;
                    }
                }
            }

            pLand->SetProperty(PRP_SEL_FACT_MEN, eLong,
                reinterpret_cast<void*>(static_cast<uintptr_t>(nMen)), eBoth);
        }
    }
}

//----------------------------------------------------------------------
// Creates a new unit via FORM command
//----------------------------------------------------------------------

CUnit* CAtlaParser::SplitUnit(CUnit* pOrigUnit, long newId)
{
    CUnit         Dummy;
    int           idx;
    CUnit* pUnitNew;

    if (!pOrigUnit)
        return NULL;

    // Construct full new unit ID using parent's faction ID
    Dummy.Id = NEW_UNIT_ID(newId, pOrigUnit->FactionId);

    CLand* pLand = GetLand(pOrigUnit->LandId);
    wxASSERT(pLand);

    // Check if unit already exists
    if (pLand->Units.Search(&Dummy, idx))
    {
        pUnitNew = (CUnit*)pLand->Units.At(idx);
    }
    else
    {
        pUnitNew = new CUnit;
        pUnitNew->Id = Dummy.Id;
        pUnitNew->FactionId = pOrigUnit->FactionId;
        pUnitNew->pFaction = pOrigUnit->pFaction;
        pUnitNew->IsOurs = pOrigUnit->IsOurs;

        // Copy ONLY flags from parent unit, not items or skills
        pUnitNew->Flags = pOrigUnit->Flags;
        pUnitNew->FlagsOrg = pOrigUnit->FlagsOrg;

        // Set name
        pUnitNew->Name << "NEW " << newId;

        // Create description in the same format as ParseUnit does
        pUnitNew->Description.Empty();
        pUnitNew->Description << "Created by unit " << pOrigUnit->Name << " (" << pOrigUnit->Id << ")";
        pUnitNew->Description << EOL_SCR;

        // Unit marker: * for our units, - for others
        if (pUnitNew->IsOurs)
            pUnitNew->Description << "* ";
        else
            pUnitNew->Description << "- ";

        // Unit name and ID
        pUnitNew->Description << pUnitNew->Name << " (" << pUnitNew->Id << "), ";

        // Faction info
        if (pUnitNew->pFaction)
        {
            pUnitNew->Description << pUnitNew->pFaction->Name << " (" << pUnitNew->FactionId << ")";
        }

        // Add flags if any (inherited from parent)
        CStr flagsStr;
        if (pUnitNew->Flags & UNIT_FLAG_PILLAGING)  flagsStr << "pillaging, ";
        if (pUnitNew->Flags & UNIT_FLAG_TAXING)     flagsStr << "taxing, ";
        if (pUnitNew->Flags & UNIT_FLAG_PRODUCING)  flagsStr << "producing, ";
        if (pUnitNew->Flags & UNIT_FLAG_GUARDING)   flagsStr << "on guard, ";
        if (pUnitNew->Flags & UNIT_FLAG_AVOIDING)   flagsStr << "avoiding, ";
        if (pUnitNew->Flags & UNIT_FLAG_BEHIND)     flagsStr << "behind, ";
        if (pUnitNew->Flags & UNIT_FLAG_HOLDING)    flagsStr << "holding, ";
        if (pUnitNew->Flags & UNIT_FLAG_RECEIVING_NO_AID) flagsStr << "receiving no aid, ";
        if (pUnitNew->Flags & UNIT_FLAG_NO_CROSS_WATER)   flagsStr << "won't cross water, ";
        if (pUnitNew->Flags & UNIT_FLAG_SHARING)    flagsStr << "sharing, ";

        if (!flagsStr.IsEmpty())
        {
            // Remove trailing comma and space
            flagsStr.DelSubStr(flagsStr.GetLength() - 2, 2);
            pUnitNew->Description << ", " << flagsStr;
        }

        // Empty items section - new unit starts with no items
        pUnitNew->Description << ". Weight: 0. Skills: none.";
        pUnitNew->Description << EOL_SCR;

        // Initialize end turn description for our units
        if (pUnitNew->IsOurs)
        {
            pUnitNew->InitEndTurnDescription();
        }

        // Copy structure-related properties from parent
        EValueType type;
        const void* value;

        if (pOrigUnit->GetProperty(PRP_FRIEND_OR_FOE, type, value, eNormal) && eLong == type)
            SetUnitProperty(pUnitNew, PRP_FRIEND_OR_FOE, type, value, eNormal);
        if (pOrigUnit->GetProperty(PRP_STRUCT_ID, type, value, eOriginal) && eLong == type)
            SetUnitProperty(pUnitNew, PRP_STRUCT_ID, type, value, eBoth);
        if (pOrigUnit->GetProperty(PRP_STRUCT_NAME, type, value, eOriginal) && eCharPtr == type)
            SetUnitProperty(pUnitNew, PRP_STRUCT_NAME, type, value, eBoth);
        pUnitNew->SetProperty(PRP_PARENT_UNIT_ID, eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(pOrigUnit->Id)), eBoth);

        // Add new unit to the land
        pLand->AddUnit(pUnitNew);
    }

    return pUnitNew;
}

//-------------------------------------------------------------
// Loads orders from file (internal version with CFileReader)
//-------------------------------------------------------------

int CAtlaParser::LoadOrders(CFileReader& F, int FactionId, BOOL GetComments)
{
    CStr          Line(64), S(64), No(32);
    const char* p;
    CPlane* pPlane;
    CLand* pLand;
    CUnit* pUnit;
    CUnit* pOrigUnit;
    CUnit         Dummy;
    int           idx;
    char          ch;

    m_CrntFactionPwd.Empty();
    
    // Clear existing orders for this faction
    for (int nPlane = 0; nPlane < m_Planes.Count(); nPlane++)
    {
        pPlane = (CPlane*)m_Planes.At(nPlane);
        for (int nLand = 0; nLand < pPlane->Lands.Count(); nLand++)
        {
            pLand = (CLand*)pPlane->Lands.At(nLand);
            pLand->DeleteAllNewUnits(FactionId);
            for (int nUnit = 0; nUnit < pLand->Units.Count(); nUnit++)
            {
                pUnit = (CUnit*)pLand->Units.At(nUnit);
                if (FactionId == pUnit->FactionId)
                {
                    pUnit->Orders.Empty();
                }
            }
        }
    }

    pUnit = NULL;
    pOrigUnit = NULL;

    while (F.GetNextLine(Line))
    {
        Line.TrimRight(TRIM_ALL);
        p = SkipSpaces(Line.GetData());

        if (0 == SafeCmp(p, "#end"))  // End of orders
            break;
        if (!p || !*p || (';' == *p && ('*' == *(p + 1) || !GetComments)))
            continue;

        p = S.GetToken(p, " \t", ch, TRIM_ALL);
        if (0 == stricmp(S.GetData(), "unit"))
        {
            p = No.GetToken(p, " \t\r\n,;", ch);
            Dummy.Id = atol(No.GetData());
            if (m_Units.Search(&Dummy, idx))
                pUnit = (CUnit*)m_Units.At(idx);
            else
                pUnit = NULL;
        }
        else if ((0 == stricmp(S.GetData(), "form") || 0 == stricmp(S.GetData(), "@form")) && !pOrigUnit)
        {
            BOOL isRepeated = (0 == stricmp(S.GetData(), "@form"));
            p = No.GetToken(p, " \t\r\n,;", ch);

            pOrigUnit = pUnit;
            pUnit = SplitUnit(pOrigUnit, atol(No.GetData()));

            if (isRepeated && pUnit)
            {
                // Mark as repeated FORM
                const char* yes = "1";
                pUnit->SetProperty(PRP_FORM_REPEATED, eCharPtr, yes, eBoth);
            }
        }
        else if (0 == stricmp(S.GetData(), "end") && pOrigUnit)
        {
            pUnit = pOrigUnit;
            pOrigUnit = NULL;
        }
        else
            if (0 == stricmp(S.GetData(), "#atlantis"))
            {
                // Parse "#atlantis NN "password""
                m_CrntFactionPwd = S.GetToken(p, " \t", ch, TRIM_ALL);
                if (!m_CrntFactionPwd.IsEmpty() && '\"' == m_CrntFactionPwd.GetData()[0])
                {
                    m_CrntFactionPwd.DelCh(0);
                    if (!m_CrntFactionPwd.IsEmpty() && '\"' == m_CrntFactionPwd.GetData()[m_CrntFactionPwd.GetLength() - 1])
                        m_CrntFactionPwd.DelCh(m_CrntFactionPwd.GetLength() - 1);
                }
            }
            else
                if (pUnit)
                {
                    Line << EOL_SCR;
                    pUnit->Orders.AddStr(Line.GetData(), Line.GetLength());
                }
    }

    RunOrders(NULL);
    m_OrdersLoaded = TRUE;

    return ERR_OK;
}

//-------------------------------------------------------------
// Loads orders from file (public interface)
//-------------------------------------------------------------

int  CAtlaParser::LoadOrders  (const char * FNameIn, int & FactionId)
{
    CFileReader   F;
    CStr          Line(64), S(64), No(32);
    CUnit       * pUnit;
    CUnit         Dummy;
    const char  * p;
    char          ch;
    int           idx;

    if (FNameIn && *FNameIn && F.Open(FNameIn))
    {
        // Determine faction ID by looking at first unit
        FactionId = 0;
        while (F.GetNextLine(Line))
        {
            Line.TrimRight(TRIM_ALL);
            p = SkipSpaces(Line.GetData());

            if (0==SafeCmp(p, "#end"))  // End of orders
                break;
            if (!p || !*p || ';'==*p || '#'==*p)
                continue;

            p = S.GetToken(p, " \t", ch, TRIM_ALL);
            if (0==stricmp(S.GetData(),"unit"))
            {
                p = No.GetToken(p, " \t\r\n;", ch);
                Dummy.Id = atol(No.GetData());
                if (m_Units.Search(&Dummy, idx))
                {
                    pUnit = (CUnit*)m_Units.At(idx);
                    if (pUnit->FactionId > 0)
                    {
                        FactionId = pUnit->FactionId; // Take the first unit's faction
                        break;
                    }
                }
            }
        }

        F.Close();
        F.Open(FNameIn);
        LoadOrders(F, FactionId, TRUE);
        F.Close();
        return ERR_OK;
    }
    return ERR_FOPEN;
}

//-------------------------------------------------------------
// Counts men with a specific flag in a land
// Used for economy calculations
//-------------------------------------------------------------

long CAtlaParser::CountMenWithFlag(CLand* pLand, unsigned long flag)
{
    long totalMen = 0;
    CUnit* pUnit;

    for (int unitidx = 0; unitidx < pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);

        if (pUnit->IsOurs)
        {
            BOOL hasFlag = (pUnit->Flags & flag) != 0;
            BOOL hasOrder = FALSE;

            // Check corresponding orders depending on flag
            if (flag == UNIT_FLAG_TAXING)
                hasOrder = pUnit->HasTaxOrder();
            else if (flag == UNIT_FLAG_PILLAGING)
                hasOrder = pUnit->HasPillageOrder();
            else if (flag == UNIT_FLAG_ENTERTAINING)
                hasOrder = pUnit->HasOrder("ENTERTAIN") || pUnit->HasOrder("@ENTERTAIN");
            else if (flag == UNIT_FLAG_WORKING)
                hasOrder = pUnit->HasOrder("WORK") || pUnit->HasOrder("@WORK");

            // Count if either flag or explicit order exists
            if (hasFlag || hasOrder)
            {
                totalMen += pUnit->GetMenCount();
            }
        }
    }

    return totalMen;
}

//-------------------------------------------------------------
// Distributes silver among units with a specific flag
// Used for tax, pillage, work, entertainment distribution
//-------------------------------------------------------------

void CAtlaParser::DistributeSilver(CLand* pLand, unsigned long flag, long totalSilver, long totalMen)
{
    if (totalMen <= 0 || totalSilver <= 0)
        return;

    CUnit* pUnit;
    long unitMen;
    long silverPerUnit;

    // Silver code in game (usually "SILV" or "SILVER")
    const wxString silverCode = wxT("SILV");

    for (int unitidx = 0; unitidx < pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);

        if (pUnit->IsOurs)
        {
            BOOL hasFlag = (pUnit->Flags & flag) != 0;
            BOOL hasOrder = FALSE;

            // Check corresponding orders depending on flag
            if (flag == UNIT_FLAG_TAXING)
                hasOrder = pUnit->HasTaxOrder();
            else if (flag == UNIT_FLAG_PILLAGING)
                hasOrder = pUnit->HasPillageOrder();
            else if (flag == UNIT_FLAG_ENTERTAINING)
                hasOrder = pUnit->HasOrder("ENTERTAIN") || pUnit->HasOrder("@ENTERTAIN");
            else if (flag == UNIT_FLAG_WORKING)
                hasOrder = pUnit->HasOrder("WORK") || pUnit->HasOrder("@WORK");

            if (hasFlag || hasOrder)
            {
                unitMen = pUnit->GetMenCount();
                if (unitMen > 0)
                {
                    silverPerUnit = (totalSilver * unitMen) / totalMen;
                    if (silverPerUnit > 0)
                    {
                        // Add silver to unit
                        EValueType type;
                        const void* pValue = nullptr;
                        long currentSilver = 0;

                        if (pUnit->GetProperty(PRP_SILVER, type, pValue, eNormal) &&
                            type == eLong && pValue)
                        {
                            currentSilver = static_cast<long>(reinterpret_cast<intptr_t>(pValue));
                        }

                        currentSilver += silverPerUnit;

                        pUnit->SetProperty(PRP_SILVER, eLong,
                            reinterpret_cast<const void*>(static_cast<intptr_t>(currentSilver)),
                            eNormal);

                        // Update end turn description
                        if (pUnit->m_EndTurnDescription.IsEmpty())
                        {
                            pUnit->m_EndTurnDescription = pUnit->Description;
                        }

                        if (!pUnit->m_EndTurnDescription.IsEmpty())
                        {
                            CStr* pDescr = &(pUnit->m_EndTurnDescription);
                            pUnit->AidItem(pDescr, silverCode, silverPerUnit);
                        }
                    }
                }
            }
        }
    }
}

//-------------------------------------------------------------
// Stub for advanced resource visibility (to be implemented)
//-------------------------------------------------------------

void CAtlaParser::LookupAdvancedResourceVisibility(CUnit * pUnit, CLand * pLand)
{
    // Check if unit can see advanced resources in this land
    // This is a stub implementation - should check unit skills and land properties
    if (!pUnit || !pLand)
        return;
}

//----------------------------------------------------------------------
// Load regex rules from configuration file
//----------------------------------------------------------------------

BOOL CAtlaParser::LoadRegexRules(const char* szFileName)
{
    m_RegexRulesLoaded = m_RegexParser.LoadRules(szFileName);
    return m_RegexRulesLoaded;
}

//----------------------------------------------------------------------
// Process all descriptions with regex rules and collect changes
//----------------------------------------------------------------------

BOOL CAtlaParser::ProcessDescriptionsWithRules(CConfigFile* pConfig)
{
    if (!m_RegexRulesLoaded || !pConfig)
        return FALSE;

    BOOL anyChanges = FALSE;

    // Process ITEM_DESCRIPTIONS
    for (int i = 0; i < m_Items.Count(); i++)
    {
        CShortNamedObj* pItem = (CShortNamedObj*)m_Items.At(i);
        if (!pItem) continue;

        wxString code;
        // Extract item code from description (format: [CODE])
        wxRegEx codeRegex("\\[([A-Z]+)\\]");
        wxString desc = wxString::FromUTF8(pItem->Description.GetSafeCStr());
        if (codeRegex.IsValid() && codeRegex.Matches(desc))
        {
            code = codeRegex.GetMatch(desc, 1);
        }

        std::vector<ExtractedData*> results;
        if (m_RegexParser.ApplyRules(desc, "ITEM", code, results))
        {
            for (size_t j = 0; j < results.size(); j++)
            {
                ExtractedData* pData = results[j];
                if (m_RegexParser.UpdateConfig(pConfig, *pData))
                    anyChanges = TRUE;
                delete pData;
            }
        }
    }

    // Process SKILL_DESCRIPTIONS
    for (int i = 0; i < m_Skills.Count(); i++)
    {
        CShortNamedObj* pSkill = (CShortNamedObj*)m_Skills.At(i);
        if (!pSkill) continue;

        wxString skillCode;
        int skillLevel = 0;
        wxString desc = wxString::FromUTF8(pSkill->Description.GetSafeCStr());

        // Extract skill code and level (format: [CODE] Level: description)
        wxRegEx skillRegex("\\[([A-Z]+)\\]\\s*(\\d+):");
        if (skillRegex.IsValid() && skillRegex.Matches(desc))
        {
            if (skillRegex.GetMatchCount() >= 3)
            {
                skillCode = skillRegex.GetMatch(desc, 1);
                wxString levelStr = skillRegex.GetMatch(desc, 2);
                skillLevel = wxAtoi(levelStr);
            }
        }

        wxString sourceCode;
        sourceCode.Printf("%s_%d", skillCode, skillLevel);

        std::vector<ExtractedData*> results;
        if (m_RegexParser.ApplyRules(desc, "SKILL", sourceCode, results))
        {
            for (size_t j = 0; j < results.size(); j++)
            {
                ExtractedData* pData = results[j];
                if (m_RegexParser.UpdateConfig(pConfig, *pData))
                    anyChanges = TRUE;
                delete pData;
            }
        }
    }

    // Process OBJECT_DESCRIPTIONS
    for (int i = 0; i < m_Objects.Count(); i++)
    {
        CShortNamedObj* pObject = (CShortNamedObj*)m_Objects.At(i);
        if (!pObject) continue;

        wxString objName;
        wxString desc = wxString::FromUTF8(pObject->Description.GetSafeCStr());

        // Extract object name (before colon)
        int colonPos = desc.Find(':');
        if (colonPos != wxNOT_FOUND)
        {
            objName = desc.Left(colonPos).Trim(true).Trim(false);
        }

        std::vector<ExtractedData*> results;
        if (m_RegexParser.ApplyRules(desc, "OBJECT", objName, results))
        {
            for (size_t j = 0; j < results.size(); j++)
            {
                ExtractedData* pData = results[j];
                if (m_RegexParser.UpdateConfig(pConfig, *pData))
                    anyChanges = TRUE;
                delete pData;
            }
        }
    }

    return anyChanges;
}

//----------------------------------------------------------------------
// Update configuration from descriptions (convenience method)
//----------------------------------------------------------------------

BOOL CAtlaParser::UpdateConfigFromDescriptions(CConfigFile* pConfig)
{
    return ProcessDescriptionsWithRules(pConfig);
}