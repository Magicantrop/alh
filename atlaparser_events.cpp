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

//----------------------------------------------------------------------
// Parses movement events (sailing, walking, etc.) and tracks connections
// between planes via shafts/gates
//----------------------------------------------------------------------

void CAtlaParser::ParseOneMovementEvent(const char * params, const char * structid, const char * fullevent)
{
    CStr         Buf(64);
    CStr         Buf2(64);
    char         ch;
    CLand      * pLand1 = NULL;
    CLand      * pLand2 = NULL;
    CStr         S;
    CBaseObject* pSailEvent;

    // Parse the "from" location
    while (params)
    {
        params = SkipSpaces(S.GetToken(params, " \n", ch, TRIM_ALL));
        if (0==stricmp(S.GetData(), "to"))
            break;
        Buf << S << ' ';
    }
    
    // Parse the "to" location if present
    if (0==stricmp(S.GetData(), "to"))
    {
        while (params)
        {
            params = SkipSpaces(S.GetToken(params, " \n", ch, TRIM_ALL));
            Buf2 << S << ' ';
        }
    }

    // Get land objects for both locations
    ParseTerrain(NULL, 0, Buf, FALSE, &pLand1);
    if (!Buf2.IsEmpty())
        ParseTerrain(NULL, 0, Buf2, FALSE, &pLand2);

    // Check for connections between different planes (shafts/gates)
    if (pLand1 && pLand2  &&  pLand1->pPlane && pLand2->pPlane  && pLand1->pPlane != pLand2->pPlane)
    {
        // Mark both lands as linked (for shaft connections)
        pLand1->SetProperty(PRP_LAND_LINK, eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(pLand2->Id)), eBoth);
        pLand2->SetProperty(PRP_LAND_LINK, eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(pLand1->Id)), eBoth);
        m_LandsToBeLinked.Insert(pLand1);
        m_LandsToBeLinked.Insert(pLand2);
    }

    // Collect sailing events to link with captains later
    if (structid && fullevent)
    {
        pSailEvent              = new CBaseObject;
        pSailEvent->Id          = atol(structid);
        pSailEvent->Description = fullevent;
        m_TempSailingEvents.Insert(pSailEvent);
    }
}

//----------------------------------------------------------------------
// Parses an event line that belongs to a specific unit
// Extracts tax, trade, and security events
//----------------------------------------------------------------------

BOOL CAtlaParser::ParseOneUnitEvent(CStr & EventLine, BOOL IsEvent, int UnitId)
{
    CUnit      * pUnit = NULL;
    const char * p;
    CStr         Name;
    BOOL         Taken = FALSE;
    long         x;
    int          idx;
    CStr         Buf(64);

    // Extract unit name and get unit object
    p = Name.GetToken(EventLine.GetData(), '(');
    if (UnitId>0)
    {
        pUnit = MakeUnit(UnitId);
        if (pUnit->Name.IsEmpty())
            pUnit->Name = Name;
    }
    
    // Add event to unit's event/error list
    if (pUnit)
    {
        if (IsEvent)
            pUnit->Events << EventLine;
        else
            pUnit->Errors << EventLine;
        Taken = TRUE;
    }

    if (IsEvent)
    {
        // Parse movement events
        p = strchr(p, ')');
        while (p && (*p>' ') )
            p++;
        p = SkipSpaces(p);

        p = SkipSpaces(Buf.GetToken(p, ' ', TRIM_ALL));
        if ( (0==stricmp("walks"   , Buf.GetData())) ||
             (0==stricmp("rides"   , Buf.GetData())) ||
             (0==stricmp("flies"   , Buf.GetData())) )
        {
            p = SkipSpaces(Buf.GetToken(p, ' ', TRIM_ALL));
            if (0==stricmp("from"  , Buf.GetData()))
                ParseOneMovementEvent(p, NULL, NULL);
        }
        // Tax/pillage events
        else if ( p && ('$'==*p) && ( (0==stricmp("collects", Buf.GetData())) ||
                                      (0==stricmp("pillages", Buf.GetData())) )
                )
        {
            p = Buf.GetToken(p, '(', TRIM_ALL);
            p = Buf.GetToken(p, ')', TRIM_ALL);
            if (!m_TaxLandStrs.Search((void*)Buf.GetData(), idx))
                m_TaxLandStrs.Insert(strdup(Buf.GetData()));
        }
        // Production events
        else if (0==stricmp("produces", Buf.GetData()))
        {
            p = Buf.GetToken(p, '(', TRIM_ALL);
            p = Buf.GetToken(p, ')', TRIM_ALL);
            if (!m_TradeLandStrs.Search((void*)Buf.GetData(), idx))
                m_TradeLandStrs.Insert(strdup(Buf.GetData()));
        }
        // Work/construction events
        else if (0==stricmp("performs", Buf.GetData()))
        {
            p = SkipSpaces(Buf.GetToken(p, ' ', TRIM_ALL));
            if (0==stricmp("work"        , Buf.GetData()) ||
                0==stricmp("construction", Buf.GetData()) )
            {
                p = Buf.GetToken(EventLine.GetData(), '(', TRIM_ALL);
                p = Buf.GetToken(p                  , ')', TRIM_ALL);
                x = atol(Buf.GetData());
                m_TradeUnitIds.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(x)));
            }
        }
        // Security events (stealing, forbidden entry, etc.)
        else if (0==stricmp("has"    , Buf.GetData()) && EventLine.FindSubStr("stolen")>=0 ||
                 0==stricmp("is"     , Buf.GetData()) && EventLine.FindSubStr("caught")>=0  ||
                 0==stricmp("steals" , Buf.GetData()) ||
                 0==stricmp("is"     , Buf.GetData()) && EventLine.FindSubStr("forbidden")>=0 ||
                 0==stricmp("forbids", Buf.GetData()) && EventLine.FindSubStr("entry")>=0
                )
        {
            BOOL show = TRUE;
            // Check if steals should be shown (configurable)
            if (0==stricmp("steals", Buf.GetData()) &&
                0==atol(gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_SHOW_STEALS)))
                show = FALSE;
            if (show)
                m_SecurityEvents.Description << EventLine;
        }
    }

    return Taken;
}

//----------------------------------------------------------------------
// Parses an event line that belongs to a specific land/hex
//----------------------------------------------------------------------

BOOL CAtlaParser::ParseOneLandEvent(CStr & EventLine, BOOL IsEvent)
{
    const char * p;
    CStr         Buf;
    CStr         S;
    BOOL         Taken = FALSE;
    CLand      * pLand = NULL;

    // Extract land coordinates and add to hex events
    p = Buf.GetToken(EventLine.GetData(), ')');
    p = S.GetToken(p, ',');
    Buf << ") " << S;
    ParseTerrain(NULL, 0, Buf, FALSE, &pLand);
    if (pLand)
        m_HexEvents.Description << EventLine;

    return Taken;
}

//----------------------------------------------------------------------
// Main event dispatcher - determines if event is unit, land, or structure event
//----------------------------------------------------------------------

int CAtlaParser::ParseOneEvent(CStr & EventLine, BOOL IsEvent)
{
    const char * p;
    CStr         Buf(64);
    CStr         StructId;
    CStr         Name;
    long         x;
    char         ch;
    BOOL         Taken = FALSE;
    BOOL         Valid;

    if (EventLine.IsEmpty())
        return 0;
    EventLine << EOL_SCR;

    // Check first character to determine event type
    p = Name.GetToken(EventLine.GetData(), "([", ch, TRIM_ALL);
    switch (ch)
    {
    case '(':  // Unit or land event
        p = Buf.GetInteger(p, Valid);
        if (*p == ')')
        {
            if (0==strnicmp(Name.GetData(), "The address of ", 15))
            {
                // This will go to general events
            }
            else
            {
                // This is a unit event
                x = atol(Buf.GetData());
                Taken = ParseOneUnitEvent(EventLine, IsEvent, x);
            }
        }
        else
        {
            // Maybe a land event
            Taken = ParseOneLandEvent(EventLine, IsEvent);
        }
        break;

    case '[':   // Structure event (ship, probably)
        if (IsEvent)
        {
            p = SkipSpaces(StructId.GetToken(p, ']', TRIM_ALL));
            p = SkipSpaces(Buf.GetToken(p, ' ', TRIM_ALL));
            if (0==stricmp("sails"   , Buf.GetData()))
            {
                p = SkipSpaces(Buf.GetToken(p, ' ', TRIM_ALL));
                if (0==stricmp("from"  , Buf.GetData()))
                    ParseOneMovementEvent(p, StructId.GetData(), EventLine.GetData());
            }
        }
        break;
    }

    // Route unhandled events to appropriate collections
    if (IsEvent)
    {
        if (!Taken)
            m_Events.Description << EventLine;
    }
    else
        m_Errors.Description << EventLine;

    return 0;
}

//----------------------------------------------------------------------
// Parses a block of events (multiple lines)
//----------------------------------------------------------------------

int CAtlaParser::ParseEvents(BOOL IsEvents)
{
    int          err   = ERR_OK;
    CStr         Line(128);
    CStr         OneEvent(128);
    char         ch;

    while ((ERR_OK==err) && ReadNextLine(Line))
    {
        Line.TrimRight(TRIM_ALL);

        if (Line.IsEmpty())
        {
            ParseOneEvent(OneEvent, IsEvents);
            break;  // Stop at empty line
        }

        // New event starts without leading spaces
        ch = Line.GetData()[0];
        if (ch != ' ' && ch != '\t')
        {
            ParseOneEvent(OneEvent, IsEvents);
            OneEvent.Empty();
        }
        if (!OneEvent.IsEmpty())
            OneEvent << EOL_SCR;
        OneEvent << Line;
    }

    return err;
}

//----------------------------------------------------------------------
// Parses a single important event (adds to both hex events and general events)
//----------------------------------------------------------------------

int CAtlaParser::ParseOneImportantEvent(CStr & EventLine)
{
    m_HexEvents.Description << EventLine << EOL_SCR;
    m_Events.Description << EventLine << EOL_SCR;
    return ERR_OK;
}

//----------------------------------------------------------------------
// Parses important events section
//----------------------------------------------------------------------

int CAtlaParser::ParseImportantEvents()
{
    int          err   = ERR_OK;
    CStr         Line(128);
    CStr         OneEvent(128);
    char         ch;
    int          i;
    BOOL         DoBreak = FALSE;

    while ((ERR_OK==err) && ReadNextLine(Line))
    {
        Line.TrimRight(TRIM_ALL);

        // Check for end of section
        for (i=0; i< ExitEndHeaderCount; i++)
            if (0==strnicmp(Line.GetData(), ExitEndHeader[i], ExitEndHeaderLen[i] ))
        {
            Line << EOL_FILE;
            PutLineBack(Line);
            DoBreak = TRUE;
            break;
        }
        if (DoBreak)
            break;

        // New event starts without leading spaces
        ch = Line.GetData()[0];
        if (ch != ' ' && ch != '\t')
        {
            ParseOneImportantEvent(OneEvent);
            OneEvent.Empty();
        }
        if (!OneEvent.IsEmpty())
            OneEvent << EOL_SCR;
        OneEvent << Line;
    }
    ParseOneImportantEvent(OneEvent);

    return err;
}

//----------------------------------------------------------------------
// Parses errors section (reuses event parsing with IsEvents=FALSE)
//----------------------------------------------------------------------

int CAtlaParser::ParseErrors()
{
    return ParseEvents(FALSE);
}

//----------------------------------------------------------------------
// Parses unclaimed silver line and assigns to current faction
//----------------------------------------------------------------------

int CAtlaParser::ParseUnclSilver(CStr & Line)
{
    const char * p;
    const char * s;
    CStr         N;
    CFaction   * pFaction;

    m_FactionInfo << Line;

    Line.TrimRight(TRIM_ALL);
    s = Line.GetData() + sizeof(HDR_SILVER)-1;
    p = strchr(s, '.');

    // Extract number before period
    if (p)
        N.SetStr(s, p-s);
    else
        N.SetStr(s);
    N.TrimLeft();
    N.TrimRight(TRIM_ALL);

    pFaction = GetFaction(m_CrntFactionId);
    if (pFaction)
        pFaction->UnclaimedSilver = atol(N.GetData());

    return ERR_OK;
}

//----------------------------------------------------------------------
// Parses attitudes section - reads declared attitudes between factions
// Can apply attitudes from ally reports or set defaults
//----------------------------------------------------------------------

int CAtlaParser::ParseAttitudes(CStr & Line, BOOL Join)
{
    CStr         Info;
    CStr         FNo;
    CStr         S1;
    const char * str;
    const char * p;
    const char * s;
    char         ch, c;
    int          attitude = ATT_FRIEND1;
    CStr         attitudes[4];
    BOOL         apply_attitudes = TRUE;
    BOOL         def;

    // Get attitude strings from config
    attitudes[ATT_FRIEND1] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_FRIEND1);
    attitudes[ATT_FRIEND2] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_FRIEND2);
    attitudes[ATT_NEUTRAL] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_NEUTRAL);
    attitudes[ATT_ENEMY] = gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_ENEMY);

    if(Join)
    {   // Check config whether to apply ally attitudes
        apply_attitudes  = (0!=SafeCmp(gpDataHelper->GetConfString(SZ_SECT_ATTITUDES, SZ_ATT_APPLY_ON_JOIN),"0"));
    }
    else
    {
        // Set default attitude from config
        while(attitude <= ATT_ENEMY)
        {
            if(0<=attitudes[attitude].FindSubStr("Own")) break;
            attitude++;
        }
        gpDataHelper->SetAttitudeForFaction(-1, attitude);
        attitude = ATT_ENEMY;
    }

    while (!Line.IsEmpty())
    {
        str = Line.GetData();
        m_FactionInfo << Line;

        if(apply_attitudes) // Parse attitudes
        {
            str = Info.GetToken(str, ":,.", ch, TRIM_ALL);
            p   = Info.GetData();

            def = FALSE;
            if(Info.FindSubStr("(default") > 0)
            {
                // Parse default line
                s = S1.GetToken(p, "(", c, TRIM_ALL);
                S1.GetToken(s, ")" ,c , TRIM_ALL);
                s = S1.GetData();
                p = Info.GetToken(s, " ", c, TRIM_ALL);
                def = TRUE;
                m_FactionInfo << EOL_SCR;
            }
            
            // Determine attitude level
            while(attitude >= ATT_FRIEND1)
            {
                if(0<=attitudes[attitude].FindSubStr(p)) break;
                attitude--;
            }
            if((!Join) && def && (attitude >= ATT_FRIEND1) && (attitude < ATT_UNDECLARED))
            {
                gpDataHelper->SetAttitudeForFaction(0, attitude);
            }

            // Parse faction list for this attitude
            while(str)
            {
                str = Info.GetToken(str, ",.", ch, TRIM_ALL);
                p   = Info.GetData();
                switch(ch)
                {
                    case '.':
                        m_FactionInfo << EOL_SCR;
                        break;
                    case ',':
                        if((attitude <= ATT_UNDECLARED) && (attitude >= ATT_FRIEND1))
                        {
                            // Parse faction id
                            if(0==strcmp(p,"none")) break;
                            s = S1.GetToken(p, "(", c, TRIM_ALL);
                            FNo.GetToken(s, ")", c, TRIM_ALL);
                            if (!FNo.IsEmpty())
                            {
                                int id = atol(FNo.GetData());
                                gpDataHelper->SetAttitudeForFaction(id, attitude);
                            }
                        }
                        break;
                }
            }
        }
        ReadNextLineMerged(Line);
        Line.TrimRight(TRIM_ALL);
    }

    return ERR_OK;
}

//----------------------------------------------------------------------
// Checks exit consistency and updates plane boundaries for wrapped worlds
//----------------------------------------------------------------------

void CAtlaParser::CheckExit(CPlane * pPlane, int Direction, CLand * pLandSrc, CLand * pLandExit)
{
    int x1,y1,x2,y2, z, width;

    LandIdToCoord(pLandSrc ->Id, x1, y1, z);
    LandIdToCoord(pLandExit->Id, x2, y2, z);

    // Update plane boundaries based on exit direction
    switch (Direction%6)
    {
    case Northeast:
    case Southeast:
        width = x1-x2+1;
        if (x2<x1 && width>pPlane->Width)
        {
            pPlane->WestEdge   = x2;
            pPlane->EastEdge   = x1;
            pPlane->Width      = width;

            pPlane->EdgeSrcId  = pLandSrc ->Id;
            pPlane->EdgeExitId = pLandExit->Id;
            pPlane->EdgeDir    = Direction%6;
        }
        break;

    case Northwest:
    case Southwest:
        width = x2-x1+1;
        if (x2>x1 && width>pPlane->Width)
        {
            pPlane->WestEdge   = x1;
            pPlane->EastEdge   = x2;
            pPlane->Width      = width;

            pPlane->EdgeSrcId  = pLandSrc ->Id;
            pPlane->EdgeExitId = pLandExit->Id;
            pPlane->EdgeDir    = Direction%6;
        }
        break;
    }

    // Extend plane boundaries if needed
    if (pPlane->Width > 0)
    {
        if (x1 > pPlane->EastEdge)
            pPlane->EastEdge = x1;
        if (x1 < pPlane->WestEdge)
            pPlane->WestEdge = x1;
        if (x2 > pPlane->EastEdge)
            pPlane->EastEdge = x2;
        if (x2 < pPlane->WestEdge)
            pPlane->WestEdge = x2;
        pPlane->Width = pPlane->EastEdge - pPlane->WestEdge + 1;
        pPlane->Width += pPlane->Width & 1; // Ensure even width
    }
}

//----------------------------------------------------------------------
// Parses weather information from hex description
// Determines tropic zones and future weather predictions
//----------------------------------------------------------------------

void CAtlaParser::ParseWeather(const char * src, CLand * pLand)
{
    BOOL         IsCurrent;
    BOOL         IsGood;
    int          Zone;
    unsigned int i;
    CStr         S1, S2;
    const char * p;
    int          x,y,z;
    CPlane     * pPlane = pLand->pPlane;

    if (!src || !pPlane)
        return;

    // Initialize weather line strings if not already done
    if (m_WeatherLine[0].IsEmpty())
    {
        for (i=0; i<sizeof(m_WeatherLine)/sizeof(*m_WeatherLine); i++)
        {
            // bit 0 - IsCurrent
            // bit 1 - IsGood
            // rest - Zone
            IsCurrent = i & 1;
            IsGood    = (i & 2) >> 1;
            Zone      = i >> 2;

            m_WeatherLine[i] = gpDataHelper->GetWeatherLine(IsCurrent, IsGood, Zone);
            m_WeatherLine[i].Normalize();
        }
    }

    src = SkipSpaces(src);
    if ('-'==src[0] && '-'==src[1] && '-'==src[2] )
    {
        // Parse weather line (starts with "---")
        while (*src > ' ')
            src++;
        src = SkipSpaces(src);
        src = S1.GetToken(src, ';', TRIM_ALL);  // Current weather
        src = S2.GetToken(src, '.', TRIM_ALL);  // Next turn weather
        S1.Normalize();
        S2.Normalize();

        for (i=0; i<sizeof(m_WeatherLine)/sizeof(*m_WeatherLine); i++)
        {
            IsCurrent = i & 1;
            IsGood    = (i & 2) >> 1;
            Zone      = i >> 2;

            if (IsCurrent)
                p = S1.GetData();
            else
                p = S2.GetData();
                
            if (0==stricmp(p, m_WeatherLine[i].GetData()))
            {
                if (IsGood )
                {
                    if (!IsCurrent)
                       pLand->WeatherWillBeGood = TRUE;
                }
                else
                {
                    LandIdToCoord(pLand->Id, x,y,z);
                    if (Zone>0)
                        continue; // Only draw tropic line for non-tropical zones
                    if (y>=pPlane->TropicZoneMin && y<=pPlane->TropicZoneMax)
                        continue; // Known coordinate, don't shrink when hexes are no longer visible

                    if (y >= pPlane->TropicZoneMax)
                        pPlane->TropicZoneMax = y;
                    if (y <= pPlane->TropicZoneMin)
                        pPlane->TropicZoneMin = y;
                }
            }
        }
    }
}

//----------------------------------------------------------------------
// Parses wages information from hex description
//----------------------------------------------------------------------

void CAtlaParser::ParseWages(CLand * pLand, const char * str1, const char * str2)
{
    // Format examples:
    //   Wages: $12.4 (Max: $350).    // str1 = "$12"  str2 = "4 (Max: $350)"
    //   Wages: $15 (Max: $10273).

    const char * src;
    CStr         N1, N2;
    CStr         sSrc;
    BOOL         Valid;

    str1 = SkipSpaces(str1);
    if (*str1 != '$')
        return;
    str1++;

    // Parse wage amount (may be decimal)
    if (strchr(str1, '('))
    {
        src = SkipSpaces(N1.GetInteger(str1, Valid));
    }
    else
    {
        sSrc << str1 << '.' << str2;
        src = N1.GetDouble(sSrc.GetData(), Valid);
    }
    pLand->Wages = atof(N1.GetData());

    // Parse maximum wages
    src = SkipSpaces(N1.GetToken(src, '$'));
    src = N1.GetInteger(src, Valid);
    pLand->MaxWages = atol(N1.GetData());
}

//----------------------------------------------------------------------
// Helper for Arno game format - counts tokens in description
//----------------------------------------------------------------------

const char * CountTokensForArno(const char * src, int & count)
{
    const char * p;
    char         ch;
    CStr         Token(32);

    count = 0;
    p = Token.GetToken(src, ')');
    while (p && *p)
    {
        p = Token.GetToken(p, ",.", ch, TRIM_NONE);
        count++;
        if ('.'==ch)
            break;
    }
    return p;
}

//----------------------------------------------------------------------
// Composes hex description for Arno game format
// Merges new description with old, keeping best parts
//----------------------------------------------------------------------

void CAtlaParser::ComposeHexDescriptionForArnoGame(const char * olddescr, const char * newdescr, CStr & CompositeDescr)
{
    const char * pnew;
    CStr         NewWeather(32);
    int          oldcount=0, newcount;
    CStr         Token(32);

    if (!olddescr || !*olddescr)
    {
        CompositeDescr = newdescr;
        return;
    }
    if (!newdescr || !*newdescr)
    {
        CompositeDescr = olddescr;
        return;
    }

    pnew = CountTokensForArno(newdescr, newcount);

    // If new description has more parts, it's better
    if (newcount>oldcount)
    {
        CompositeDescr = newdescr;
        return;
    }

    // If same number of parts, longer is better
    if (newcount==oldcount)
    {
        if (strlen(olddescr) < strlen(newdescr))
            CompositeDescr = newdescr;
        return;
    }

    // New description is worse, but contains good weather line
    // Extract weather line and merge with old description
    while (pnew && *pnew && *pnew!='-')
        pnew++;
    pnew = Token.GetToken(pnew, '\n');
    pnew = NewWeather.GetToken(pnew, '.', TRIM_NONE);

    CompositeDescr.Empty();
    pnew = Token.GetToken(olddescr, '.');
    CompositeDescr << Token << "." << EOL_SCR;

    while (pnew && *pnew && *pnew!='-')
        pnew++;
    pnew = Token.GetToken(pnew, '\n');
    CompositeDescr << Token << EOL_SCR;

    pnew = Token.GetToken(pnew, '.');    // old weather
    CompositeDescr << NewWeather << "." << pnew;
}

//----------------------------------------------------------------------
// Analyzes terrain description, extracting economic data, products, etc.
//----------------------------------------------------------------------

int CAtlaParser::AnalyzeTerrain(CLand * pMotherLand, CLand * pLand, BOOL IsExit, int ExitDir, CStr & Description)
{
    enum         {eMain, eSale, eWanted, eProduct, eNone} SectType;
    CStr         Section(64);
    CStr         Struct (64);
    CStr         S1     (32);
    CStr         S2     (32);
    CStr         N1     (32);
    CStr         N2     (32);
    CStr         Buf    (32);
    long         n1;
    long         n2;
    const char * src;
    const char * str;
    const char * srcold;
    const char * p;
    char         ch;
    CProduct   * pProd;
    int          delpos = 0;
    int          dellen = 0;
    int          idx;
    BOOL         TerrainPassed = FALSE;
    BOOL         ProductsWereEmpty;
    BOOL         Valid;

    SectType = eMain;
    ProductsWereEmpty = (0==pLand->Products.Count());
    
    // Skip terrain coordinates (they confuse edge items)
    srcold   = strchr(Description.GetData(), ')');
    if (srcold)
        srcold = SkipSpaces(srcold++);
    else
        srcold = Description.GetData(); // Should never happen

    src      = Section.GetToken(srcold, '.', TRIM_ALL);
    
    // Parse weather if present (not history, and current turn)
    if (!m_IsHistory && m_CurYearMon>0)
        ParseWeather(src, pLand);
        
    while (!Section.IsEmpty())
    {
        BOOL RerunSection = FALSE;
        str = Section.GetData();
        
        // Skip dashed lines when weather not activated
        str = SkipSpaces(str);
        if ('-'==str[0] && '-'==str[1] && '-'==str[2] )
        {
            while (*str > ' ')
                str++;
            str = SkipSpaces(str);
        }
        
        // Parse land data
        while (str)
        {
            if (RerunSection)
                break;

            str = Struct.GetToken(str, ":,", ch, TRIM_ALL);
            p   = Struct.GetData();
            switch(ch)
            {
            case ':': // Section header
                pLand->Flags|=LAND_VISITED;
                if      (0==stricmp("For Sale", p))
                    SectType = eSale;
                else if (0==stricmp("Wanted"  , p))
                    SectType = eWanted;
                else if (0==stricmp("Products", p))
                {
                    SectType = eProduct;
                    delpos   = srcold - Description.GetData();
                    dellen   = src - srcold;
                }
                else if (0==stricmp("Wages"  , p))
                {
                    // Wages doesn't follow common pattern
                    ParseWages(pLand, str, src);
                }
                else if (0==stricmp("Entertainment available", p))
                {
                    const char * pTmp = strstr(str, "$");
                    if (pTmp)
                        pLand->Entertainment = atoi(++pTmp);
                }
                else
                    SectType = eNone;
                break;

            case ',':
            case  0 :
                switch (SectType)
                {
                case eMain:    // Parse contains, $, and peasants
                    if ('$'==*p)
                    {
                        N1.GetInteger(++p, Valid);
                        pLand->Taxable = atol(N1.GetData());
                    }
                    else
                    {
                        p  = SkipSpaces(S1.GetToken(p, ' ', TRIM_ALL));
                        p  = S2.GetToken(p, ' ', TRIM_ALL);
                        n1 = atol(S1.GetData());
                        if (n1>0)
                        {
                            if (0==stricmp("peasants", S2.GetData()))
                            {
                                pLand->Peasants = n1;
                                p  = S2.GetToken(p, '(', TRIM_ALL);
                                p  = pLand->PeasantRace.GetToken(p, ')', TRIM_ALL);
                                pLand->PeasantRace.Replace('\r', ' ');
                                pLand->PeasantRace.Replace('\n', ' ');
                                pLand->PeasantRace.Replace('\t', ' ');
                                pLand->PeasantRace.Normalize();
                                pLand->PeasantRace.Replace(' ', '_');
                            }
                        }
                        else if (0==stricmp("contains", S1.GetData()))
                        {
                            // City name may contain spaces
                            p = S1.GetToken(Struct.GetData(), ' ', TRIM_ALL);
                            p = pLand->CityName.GetToken(p, '[', TRIM_ALL);
                            if (!p)
                            {
                                // Dot in city name - need to restart section
                                RerunSection = TRUE;
                                break;
                            }
                            p = pLand->CityType.GetToken(p, ']', TRIM_ALL);
                            // Set city type flags
                            if(0==SafeCmp(pLand->CityType.ToLower(),"town"))
                            {
                                pLand->Flags |= LAND_TOWN;
                            }
                            else if (0==SafeCmp(pLand->CityType.ToLower(),"city"))
                            {
                                pLand->Flags |= LAND_CITY;
                            }
                        }
                        else if (!TerrainPassed)
                            TerrainPassed = TRUE;
                        else if (IsExit)
                        {
                            // Edge object in exit description
                            if (pMotherLand)
                            {
                                pMotherLand->AddNewEdgeStruct(S1.GetData(), ExitDir);
                                // Also add to neighboring hex
                                int adj_dir = ExitDir -3;
                                if(adj_dir < 0) adj_dir += 6;
                                pLand->AddNewEdgeStruct(S1.GetData(), adj_dir);
                            }
                        }
                    }
                    break;

                case eSale:
                case eWanted:  // Format: "35 horses [HORS] at $62"
                    p = N1.GetInteger(p, Valid);
                    if (N1.IsEmpty())
                    {
                        N1.GetToken(p, " [", ch);
                        if (0==stricmp(N1.GetData(), "none"))
                            N1 = "-1";
                        else if (0==stricmp(N1.GetData(), "unlimited"))
                            N1 = "10000000";
                        else
                            N1 = "1";
                    }
                    n1 = atol(N1.GetData());
                    p = S1.GetToken(p, '[', TRIM_ALL);
                    p = S2.GetToken(p, ']', TRIM_ALL);
                    p = N2.GetToken(p, '$', TRIM_ALL);
                    N2= p;
                    n2= atol(N2.GetData());

                    if ((!S2.IsEmpty()) && (n2>0) )
                    {
                        // Store sale/wanted amounts and prices as properties
                        if (eSale == SectType)
                            MakeQualifiedPropertyName(PRP_SALE_AMOUNT_PREFIX, S2.GetData(), Buf);
                        else
                            MakeQualifiedPropertyName(PRP_WANTED_AMOUNT_PREFIX, S2.GetData(), Buf);
                        SetLandProperty(pLand, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(n1)), eBoth);

                        if (eSale == SectType)
                            MakeQualifiedPropertyName(PRP_SALE_PRICE_PREFIX, S2.GetData(), Buf);
                        else
                            MakeQualifiedPropertyName(PRP_WANTED_PRICE_PREFIX, S2.GetData(), Buf);
                        SetLandProperty(pLand, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(n2)), eBoth);
                    }
                    break;

                case eProduct: // Format: "40 grain [GRAI], 33 horses [HORS]"
                    p = SkipSpaces(N1.GetToken(p, ' ', TRIM_ALL));
                    n1= atol(N1.GetData());
                    if (0==n1)
                    {
                        if (0==stricmp(N1.GetData(), "none"))
                            n1 = -1;
                        else if  (0==stricmp(N1.GetData(), "unlimited"))
                            n1 = 10000000;
                    }
                    if (n1 >= 0)
                    {
                        pProd = new CProduct;
                        pProd->Amount = n1;
                        p = pProd->LongName.GetToken(p, '[', TRIM_ALL);
                        p = pProd->ShortName.GetToken(p, ']', TRIM_ALL);
                        
                        // Replace existing product or add new
                        if (pLand->Products.Search(pProd, idx))
                            pLand->Products.AtFree(idx);
                        else
                            if (!m_IsHistory && !ProductsWereEmpty)
                            {
                                // New product discovered
                                CStr          sCoord;
                                CBaseObject * pNewProd = new CBaseObject;

                                ComposeLandStrCoord(pLand, sCoord);
                                pNewProd->Name        << pProd->LongName << " discovered in (" << sCoord << ")";
                                pNewProd->Description << pLand->TerrainType << " (" << sCoord << ")"
                                                      << " is a new source of "  << pProd->LongName << EOL_SCR;

                                m_NewProducts.Insert(pNewProd);
                            }

                        pLand->Products.Insert(pProd);

                        // Also set as property for easier searching
                        MakeQualifiedPropertyName(PRP_RESOURCE_PREFIX, pProd->ShortName.GetData(), Buf);
                        SetLandProperty(pLand, Buf.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(n1)), eBoth);
                    }
                    break;

                default:
                    break;
                }
                break;
            }
        }
        
        if (RerunSection)
        {
            // Dot in city name - rejoin with next part
            CStr S;
            src      = S.GetToken(src, '.');
            Section << '.' << S;
        }
        else
        {
            srcold   = src;
            src      = Section.GetToken(src, '.');
            SectType = eNone;
        }
    }

    // Remove processed products section if it was marked for deletion
    if (dellen)
        Description.DelSubStr(delpos, dellen);

    return 0;
}

//----------------------------------------------------------------------
// Stores a battle description and extracts statistics
//----------------------------------------------------------------------

void CAtlaParser::StoreBattle(CStr& Source)
{
    CStr          S1, S2, S3;
    CStr          N1, N2, N3;
    CStr          xStr, yStr, planeStr;
    CStr          tempStr;
    CBattle* pBattle;
    const char* p;
    int           i;
    BOOL          Ok = FALSE;
    BOOL          RegularBattle = FALSE;
    long          battleLandId = 0;
    CLand* pBattleLand = NULL;
    CPlane* pCurrentPlane = NULL;
    char          delim = 0;

    Source.TrimRight(TRIM_ALL);
    if (Source.IsEmpty())
        return;

    p = Source.GetData();
    p = SkipSpaces(N1.GetToken(p, '(', TRIM_ALL));
    p = SkipSpaces(N1.GetToken(p, ')', TRIM_ALL));
    p = SkipSpaces(S1.GetToken(p, ' ', TRIM_ALL));
    p = SkipSpaces(S2.GetToken(p, ' ', TRIM_ALL));

    // Check for assassination format
    if (IsInteger(N1.GetData()) &&
        0 == stricmp("is", S1.GetData()) &&
        0 == stricmp("assassinated", S2.GetData())
        )
    {
        p = S3.GetToken(p, '(', TRIM_ALL);
        p = N3.GetToken(p, ',', TRIM_ALL);
        Ok = TRUE;
    }
    else
    {
        // Regular battle format: "Xbowmen (591) attacks City Guard (24) in swamp (13,33) in Salen!"
        p = Source.GetData();
        p = S1.GetToken(p, '(', TRIM_ALL);
        p = N1.GetToken(p, ')', TRIM_ALL);
        p = S2.GetToken(p, '(', TRIM_ALL);
        p = N2.GetToken(p, ')', TRIM_ALL);
        p = S3.GetToken(p, '(', TRIM_ALL);
        p = N3.GetToken(p, ',', TRIM_ALL);
        Ok = (IsInteger(N1.GetData()) &&
            IsInteger(N2.GetData()) &&
            IsInteger(N3.GetData()) &&
            (0 == strnicmp(S3.GetData(), "in", 2)));
        RegularBattle = TRUE;
    }

    if (Ok)
    {
        // Parse coordinates - N3 now contains X coordinate
        xStr = N3;

        // Parse Y coordinate and possible plane
        // Skip the comma after X coordinate
        p = SkipSpaces(p);

        // Get Y coordinate (up to next comma or closing parenthesis)
        const char* coordPtr = p;
        coordPtr = tempStr.GetToken(coordPtr, ",)", delim, TRIM_ALL);
        yStr = tempStr;

        // Check if there's a comma (indicating plane specification)
        if (delim == ',')
        {
            // There is a plane specified
            coordPtr = SkipSpaces(coordPtr);
            coordPtr = planeStr.GetToken(coordPtr, ')', TRIM_ALL);
        }
        else
        {
            // No plane specified, use default
            planeStr = DEFAULT_PLANE;
        }

        // Get the closing parenthesis position
        p = strchr(p, ')');
        if (p) p++; // Skip past ')'

        // Normalize coordinates
        xStr.Normalize();
        yStr.Normalize();

        // Get the current plane context
        if (m_pCurLand && m_pCurLand->pPlane)
            pCurrentPlane = m_pCurLand->pPlane;
        else if (!planeStr.IsEmpty())
        {
            // Try to find the plane by name
            CBaseObject Dummy;
            Dummy.Name = planeStr.GetData();
            int idx;
            if (m_PlanesNamed.Search(&Dummy, idx))
                pCurrentPlane = (CPlane*)m_PlanesNamed.At(idx);
        }

        if (pCurrentPlane)
        {
            // Create land ID from coordinates and plane
            battleLandId = LandCoordToId(atol(xStr.GetData()), atol(yStr.GetData()), pCurrentPlane->Id);
            pBattleLand = GetLand(battleLandId);
        }

        // Mark land as having a battle
        if (pBattleLand)
        {
            // Direct flag setting if land exists
            pBattleLand->Flags |= LAND_BATTLE;
        }
        else
        {
            // Land doesn't exist yet - store coordinates with plane for later processing
            CStr fullCoords;
            if (!planeStr.IsEmpty())
                fullCoords << xStr << "," << yStr << "," << planeStr;
            else if (pCurrentPlane)
                fullCoords << xStr << "," << yStr << "," << pCurrentPlane->Name;
            else
                fullCoords << xStr << "," << yStr << "," << DEFAULT_PLANE;

            if (!m_BattleLandStrs.Search((void*)fullCoords.GetData(), i))
                m_BattleLandStrs.Insert(strdup(fullCoords.GetData()));
        }

        // Create battle object for history
        // Skip to the end of the coordinates and get the rest
        p = S3.GetToken(p, ')', TRIM_ALL);
        N3 << "," << S3;
        N3.Normalize();

        pBattle = new CBattle;
        pBattle->LandStrId = N3;
        pBattle->Description = Source;
        pBattle->Name.SetStr(Source.GetData(), p - Source.GetData());
        pBattle->Name.Normalize();

        // Add battle statistics if enabled
        if (RegularBattle && atol(gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_BATTLE_STATISTICS)))
        {
            CStr Details(64);
            AnalyzeBattle(Source.GetData(), Details);
            pBattle->Description << EOL_SCR << Details;
        }

        // Avoid duplicate battles
        BOOL duplicate = FALSE;
        for (i = 0; i < m_Battles.Count(); i++)
        {
            CBattle* pExisting = (CBattle*)m_Battles.At(i);
            if (0 == stricmp(pBattle->Name.GetData(), pExisting->Name.GetData()))
            {
                duplicate = TRUE;
                break;
            }
        }

        if (!duplicate)
            m_Battles.Insert(pBattle);
        else
            delete pBattle;
    }
}

//----------------------------------------------------------------------
// Parses battles section, extracting individual battle descriptions
//----------------------------------------------------------------------

int CAtlaParser::ParseBattles()
{
    CStr         CurLine(64);
    CStr         Battle(128);
    CStr         Block1(64), Block2(64);
    const char * p;
    int          i;
    CStr         S1, S2, N1;

    while (ReadNextLine(CurLine))
    {
        CurLine.TrimRight(TRIM_ALL);

        if (CurLine.IsEmpty())
        {
            // Check if this is an assassination
            p = SkipSpaces(Block1.GetData());
            p = SkipSpaces(N1.GetToken(p, '(', TRIM_ALL));
            p = SkipSpaces(N1.GetToken(p, ')', TRIM_ALL));
            p = SkipSpaces(S1.GetToken(p, ' ', TRIM_ALL));
            p = SkipSpaces(S2.GetToken(p, ' ', TRIM_ALL));

            if (IsInteger(N1.GetData()) &&
                0 == stricmp("is"          , S1.GetData()) &&
                0 == stricmp("assassinated", S2.GetData())
                )
            {
                // Assassination
                Battle << Block2 << EOL_SCR;
                StoreBattle(Battle);
                Battle.Empty();
                Block2 = Block1;
                Block1.Empty();
            }
            else
            {
                Battle << Block2 << EOL_SCR;
                Block2 = Block1;
                Block1.Empty();
            }
        }
        else
        {
            if (0==strnicmp(CurLine.GetData(), HDR_ATTACKERS, sizeof(HDR_ATTACKERS)-1))
            {
                // New battle begins
                StoreBattle(Battle);
                Battle.Empty();
                Battle << Block2 << EOL_SCR;
                Block2 = Block1;
                Block1.Empty();
            }
            else
            {
                // Check if battles section has ended
                p = SkipSpaces(CurLine.GetData());
                for (i=0; i< BattleEndHeaderCount; i++)
                    if (0==strnicmp(p, BattleEndHeader[i], BattleEndHeaderLen[i] ))
                    {
                        CurLine << EOL_FILE;
                        PutLineBack(CurLine);
                        Battle << Block2 << Block1 << EOL_SCR;
                        StoreBattle(Battle);
                        Battle.Empty();
                        return 0;
                    }
            }
            Block1 << CurLine << EOL_SCR;
        }
    }

    return 0;
}

//----------------------------------------------------------------------
// Parses a single unit from battle description (for statistics)
//----------------------------------------------------------------------

const char * CAtlaParser::AnalyzeBattle_ParseUnit(const char * src, CUnit *& pUnit, BOOL & InFrontLine)
{
    CStr         Item, S, N1, S1, S2;
    char         ch, ch1;
    const char * p;
    long         n;

    InFrontLine = TRUE;
    pUnit       = NULL;

    while (src && *src)
    {
        src = Item.GetToken(src, ",.(", ch, TRIM_ALL );
        if (Item.IsEmpty())
            return src;
        if (!pUnit)
            pUnit = new CUnit;
        if ('('==ch)
        {
            // Unit ID, faction ID, or balrog - we don't care which exactly
            src = S.GetToken(src, ')', TRIM_ALL);
            src = S.GetToken(src, ",.", ch, TRIM_ALL );
        }

        // Parse unit elements: "behind", "leader [LEAD]", "4 leaders [LEAD]", "crossbow 5"
        if (0==stricmp(Item.GetData(), "behind"))
            InFrontLine = FALSE;
        else
        {
            p = Item.GetData();
            p = N1.GetToken(p, " \n", ch1, TRIM_ALL);
            p = S1.GetToken(p, '[', TRIM_ALL);
            p = S2.GetToken(p, ']', TRIM_ALL);

            if (!S2.IsEmpty())  // This is an item
            {
                n = 1;
                if (N1.IsInteger())
                    n = atol(N1.GetData());
                SetUnitProperty(pUnit, S2.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(n)), eBoth);
            }
            else     // This is a skill
            {
                S1.Empty();
                N1.Empty();
                p = Item.GetData();
                while (p && *p)
                {
                    p = N1.GetToken(p, " \n", ch1, TRIM_ALL);
                    if (p && *p)
                    {
                        if (!S1.IsEmpty())
                            S1 << ' ';
                        S1 << N1;
                    }
                }

                if (N1.IsInteger() && !S1.IsEmpty())
                {
                    S2 = gpDataHelper->ResolveAlias(S1.GetData());
                    S2 << PRP_SKILL_POSTFIX; // This is a skill
                    n = atol(N1.GetData());
                    SetUnitProperty(pUnit, S2.GetData(), eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(n)), eBoth);
                }
            }
        }

        if ('.'==ch)
            break;
    }

    return src;
}

//----------------------------------------------------------------------
// Summarizes unit statistics for a battle side
// Aggregates items and skills
//----------------------------------------------------------------------

void CAtlaParser::AnalyzeBattle_SummarizeUnits(CBaseColl & Units, CStr & Details)
{
    CStr            propname;
    int             i, propidx;
    CUnit         * pUnit;
    CBaseObject     Faction;
    EValueType      type;
    const void    * value;
    const void    * valuetot;
    int             skilllen, maxproplen=0;

    skilllen    = strlen(PRP_SKILL_POSTFIX);
    for (i=0; i<Units.Count(); i++)
    {
        pUnit = (CUnit*)Units.At(i);

        for (propidx=0; propidx<m_UnitPropertyNames.Count(); propidx++)
        {
            propname = (const char *) m_UnitPropertyNames.At(propidx);
            if (!pUnit->GetProperty(propname.GetData(), type, value, eOriginal) || eLong!=type )
                continue;
            if (propname.FindSubStrR(PRP_SKILL_POSTFIX) == propname.GetLength()-skilllen)
            {
                // This is a skill - convert to skill_level_men format
                propname << static_cast<long>(reinterpret_cast<intptr_t>(value));
                if (!pUnit->GetProperty(PRP_MEN, type, value, eOriginal) || eLong!=type )
                    continue;
            }
            
            // Aggregate property
            if (!Faction.GetProperty(propname.GetData(), type, valuetot, eNormal))
                valuetot = (void*)0;

            if (-1== static_cast<long>(reinterpret_cast<intptr_t>(valuetot)) || 0x7fffffff - static_cast<long>(reinterpret_cast<intptr_t>(value)) < static_cast<long>(reinterpret_cast<intptr_t>(valuetot)) )
                valuetot = (void*)(long)-1; // Overflow protection
            else
                valuetot = reinterpret_cast<void*>(static_cast<uintptr_t>((static_cast<long>(reinterpret_cast<intptr_t>(valuetot)) + static_cast<long>(reinterpret_cast<intptr_t>(value)))));
            Faction.SetProperty(propname.GetData(), eLong, valuetot, eNormal);
            if (propname.GetLength() > maxproplen)
                maxproplen = propname.GetLength();
        }
    }

    // Output aggregated statistics
    propidx  = 0;
    propname = Faction.GetPropertyName(propidx);
    while (!propname.IsEmpty())
    {
        if (Faction.GetProperty(propname.GetData(), type, value, eNormal) &&
            (eLong==type) )
        {
            while (propname.GetLength() < maxproplen)
                propname.AddCh(' ');
            Details << "   " << propname << "  " << static_cast<long>(reinterpret_cast<intptr_t>(value)) << EOL_SCR;
        }

        propname = Faction.GetPropertyName(++propidx);
    }
}

//----------------------------------------------------------------------
// Analyzes one side of a battle (attackers or defenders)
// Splits into front line and back line units
//----------------------------------------------------------------------

void CAtlaParser::AnalyzeBattle_OneSide(const char * src, CStr & Details)
{
    CBaseColl   Frontline(64), Backline(64);
    CUnit     * pUnit;
    BOOL        InFrontLine;
    CStr        S1(64), S2(64);

    while (src && *src)
    {
        src = AnalyzeBattle_ParseUnit(src, pUnit, InFrontLine);
        if (!pUnit)
            continue;
        if (InFrontLine)
            Frontline.Insert(pUnit);
        else
            Backline.Insert(pUnit);
    }

    AnalyzeBattle_SummarizeUnits(Frontline, S1);
    AnalyzeBattle_SummarizeUnits(Backline, S2);

    Details << "Front line" << EOL_SCR << S1;
    Details << "Back line" << EOL_SCR << S2 ;

    Frontline.FreeAll();
    Backline.FreeAll();
}

//----------------------------------------------------------------------
// Analyzes a full battle, extracting statistics for both sides
//----------------------------------------------------------------------

void CAtlaParser::AnalyzeBattle(const char * src, CStr & Details)
{
    CStr         Line(64), Attackers(64), Defenders(64);
    CStr         S1, N1, S2;
    const char * p;

    Details.Empty();

    // Skip to attackers section
    while (src && *src)
    {
        src = Line.GetToken(src, '\n', TRIM_ALL);
        if (0==strnicmp(Line.GetData(), HDR_ATTACKERS, sizeof(HDR_ATTACKERS)-1))
            break;
    }
    
    // Read attackers
    while (src && *src)
    {
        src = Line.GetToken(src, '\n', TRIM_ALL);
        if (0==strnicmp(Line.GetData(), HDR_DEFENDERS, sizeof(HDR_ATTACKERS)-1))
            break;
        Attackers << Line << EOL_SCR;
    }

    // Read defenders until round markers
    while (src && *src)
    {
        src = Line.GetToken(src, '\n', TRIM_ALL);

        // Check for free attacks round
        p = Line.GetData();
        p = S1.GetToken(p, '(', TRIM_ALL);
        p = N1.GetToken(p, ')', TRIM_ALL);
        p = S2.GetToken(p, '.', TRIM_ALL);
        if (!S1.IsEmpty() && N1.IsInteger() && 0==stricmp(S2.GetData(), "gets a free round of attacks"))
            break;

        // Check for round marker
        p = Line.GetData();
        p = S1.GetToken(p, ' ', TRIM_ALL);
        p = N1.GetToken(p, ':', TRIM_ALL);
        if (0==stricmp(S1.GetData(), "Round") && N1.IsInteger())
            break;

        Defenders << Line << EOL_SCR;
    }

    Details << EOL_SCR << EOL_SCR << "----------------------------------------------"
            << EOL_SCR << "Battle statistics:" << EOL_SCR << EOL_SCR;
    Details << HDR_ATTACKERS << EOL_SCR;
    AnalyzeBattle_OneSide(Attackers.GetData(), Details);

    Details << EOL_SCR;
    Details << HDR_DEFENDERS << EOL_SCR;
    AnalyzeBattle_OneSide(Defenders.GetData(), Details);
}

//----------------------------------------------------------------------
// Applies default orders from unit comments to units
//----------------------------------------------------------------------

BOOL CAtlaParser::ApplyDefaultOrders(BOOL EmptyOnly)
{
    int           i;
    CUnit* pUnit;
    CStr          NewOrder(32);
    CStr          OldOrder(32);
    const char* pNew;
    const char* pOld;
    BOOL          Exists;
    BOOL          Changed = FALSE;

    for (i = 0; i < m_Units.Count(); i++)
    {
        pUnit = (CUnit*)m_Units.At(i);
        if (pUnit->IsOurs)
        {
            if (EmptyOnly)
            {
                // Apply only if unit has no orders
                pUnit->Orders.TrimRight(TRIM_ALL);
                if (pUnit->Orders.IsEmpty())
                {
                    pNew = pUnit->DefOrders.GetData();
                    while (pNew)
                    {
                        pNew = NewOrder.GetToken(pNew, '\n', TRIM_ALL);
                        if ((!NewOrder.IsEmpty()) && (';' != NewOrder.GetData()[0]))
                        {
                            if (!pUnit->Orders.IsEmpty())
                                pUnit->Orders << EOL_SCR;
                            pUnit->Orders << NewOrder;
                            Changed = TRUE;
                        }
                    }
                }
            }
            else
            {
                // Add only orders that don't already exist
                pNew = pUnit->DefOrders.GetData();
                while (pNew)
                {
                    pNew = NewOrder.GetToken(pNew, '\n', TRIM_ALL);
                    if ((!NewOrder.IsEmpty()) && (';' != NewOrder.GetData()[0]))
                    {
                        Exists = FALSE;
                        pOld = pUnit->Orders.GetData();
                        while (pOld)
                        {
                            pOld = OldOrder.GetToken(pOld, '\n', TRIM_ALL);
                            if (0 == stricmp(OldOrder.GetData(), NewOrder.GetData()))
                            {
                                Exists = TRUE;
                                break;
                            }
                        }
                        if (!Exists)
                        {
                            if (!pUnit->Orders.IsEmpty())
                                pUnit->Orders << EOL_SCR;
                            pUnit->Orders << NewOrder;
                            Changed = TRUE;
                        }
                    }
                }
            }
        }
    }
    if (Changed)
        RunOrders(NULL);

    return Changed;
}

//-------------------------------------------------------------
// Sets a property on a unit and registers property name if new
//-------------------------------------------------------------

int  CAtlaParser::SetUnitProperty(CUnit* pUnit, const char* name, EValueType type, const void* value, EPropertyType proptype)
{
    int       i;
    CStrInt* pSI;

    if (!m_UnitPropertyNames.Search((void*)name, i))
    {
        m_UnitPropertyNames.Insert(strdup(name));
        pSI = new CStrInt(name, (int)type);
        if (!m_UnitPropertyTypes.Insert(pSI))
            delete pSI;
    }
    return pUnit->SetProperty(name, type, value, proptype);
}

//-------------------------------------------------------------
// Sets a property on a land and registers property name if new
//-------------------------------------------------------------

int  CAtlaParser::SetLandProperty(CLand* pLand, const char* name, EValueType type, const void* value, EPropertyType proptype)
{
    int       i;

    if (!m_LandPropertyNames.Search((void*)name, i))
    {
        m_LandPropertyNames.Insert(strdup(name));
    }
    return pLand->SetProperty(name, type, value, proptype);
}

//-------------------------------------------------------------
// Helper for writing one mage skill in CSV format
//-------------------------------------------------------------

void WriteOneMageSkill(CStr& Line, const char* skill, CUnit* pUnit, const char* separator, int format)
{
    CStr                S;
    EValueType          type;
    const void* value;
    long                nlvl = 0, ndays = 0;
    int                 n;

    Line << separator;

    // Get skill level
    S << skill << PRP_SKILL_POSTFIX;
    if (pUnit->GetProperty(S.GetData(), type, value, eNormal) && (eLong == type))
        nlvl = static_cast<long>(reinterpret_cast<intptr_t>(value));

    // Get skill days
    S.Empty();
    S << skill << PRP_SKILL_DAYS_POSTFIX;
    if (pUnit->GetProperty(S.GetData(), type, value, eNormal) && (eLong == type))
        ndays = static_cast<long>(reinterpret_cast<intptr_t>(value));

    switch (format)
    {
    case 0:  // Original decorated format (level with + for extra days)
        if (nlvl > 0)
        {
            Line << "_" << nlvl;
            for (n = 1; n <= nlvl; n++)
                ndays -= n * 30;
            if (ndays < 0)
                ndays = 0;
            n = ndays / 30;

            while (n-- > 0)
                Line << "+";
        }
        else
            Line << "_";
        break;

    case 1:  // Just days count
        Line << ndays;
        break;

    case 2:  // Level and days in parentheses
        if (ndays > 0)
            Line << nlvl << "(" << ndays << ")";
        break;
    }
}

//-------------------------------------------------------------
// Writes mage information to CSV file
// Can output in vertical (skills as rows) or horizontal format
//-------------------------------------------------------------

void CAtlaParser::WriteMagesCSV(const char* FName, BOOL vertical, const char* separator, int format)
{
    CBaseCollById       Mages;
    CUnit* pUnit;
    int                 idx;
    EValueType          type;
    const void* value;
    CStringSortColl     Skills;
    const char* propname;
    int                 i, n, postlen;
    CStr                S, Line(64);
    CFileWriter         Dest;
    BOOL                IsMage;

    postlen = strlen(PRP_SKILL_POSTFIX);
    
    // Find all mages and collect their skills
    for (idx = 0; idx < m_Units.Count(); idx++)
    {
        pUnit = (CUnit*)m_Units.At(idx);

        IsMage = FALSE;
        i = 0;
        propname = pUnit->GetPropertyName(i);
        while (propname)
        {
            if (gpDataHelper->IsRawMagicSkill(propname))
            {
                Mages.Insert(pUnit);
                IsMage = TRUE;
                break;
            }
            propname = pUnit->GetPropertyName(++i);
        }

        if (IsMage)
        {
            i = 0;
            propname = pUnit->GetPropertyName(i);
            while (propname)
            {
                if (pUnit->GetProperty(propname, type, value, eNormal) && (eLong == type))
                {
                    S = propname;
                    if (S.FindSubStrR(PRP_SKILL_POSTFIX) == S.GetLength() - postlen)
                    {
                        S.DelSubStr(S.GetLength() - postlen, postlen);
                        if (!Skills.Search((void*)S.GetData(), n))
                            Skills.Insert(strdup(S.GetData()));
                    }
                }

                propname = pUnit->GetPropertyName(++i);
            }
        }
    }

    if (Dest.Open(FName))
    {
        Line.Empty();
        if (vertical)
        {
            // Vertical format: skills as rows, mages as columns
            Line << "Skill";
            for (i = 0; i < Mages.Count(); i++)
            {
                pUnit = (CUnit*)Mages.At(i);
                Line << separator << pUnit->Id << " " << pUnit->Name;
            }
            Line << EOL_FILE;
            Dest.WriteBuf(Line.GetData(), Line.GetLength());

            for (i = 0; i < Skills.Count(); i++)
            {
                Line.Empty();
                Line << (const char*)Skills.At(i);
                for (idx = 0; idx < Mages.Count(); idx++)
                {
                    pUnit = (CUnit*)Mages.At(idx);
                    WriteOneMageSkill(Line, (const char*)Skills.At(i), pUnit, separator, format);
                }
                Line << EOL_FILE;
                Dest.WriteBuf(Line.GetData(), Line.GetLength());
            }
        }
        else
        {
            // Horizontal format: mages as rows, skills as columns
            Line << "Id" << separator << "Name";
            for (i = 0; i < Skills.Count(); i++)
                Line << separator << (const char*)Skills.At(i);
            Line << EOL_FILE;
            Dest.WriteBuf(Line.GetData(), Line.GetLength());

            for (idx = 0; idx < Mages.Count(); idx++)
            {
                pUnit = (CUnit*)Mages.At(idx);
                Line.Empty();
                Line << pUnit->Id << separator << pUnit->Name;
                for (i = 0; i < Skills.Count(); i++)
                    WriteOneMageSkill(Line, (const char*)Skills.At(i), pUnit, separator, format);

                Line << EOL_FILE;
                Dest.WriteBuf(Line.GetData(), Line.GetLength());
            }
        }
        Dest.Close();
    }

    Mages.DeleteAll();
    Skills.FreeAll();
}

//-------------------------------------------------------------
// Gets movement cost for a terrain type
//-------------------------------------------------------------

int CAtlaParser::GetTerrainMovementCost(wxString Terrain) const
{
    int MovementCost = 2; // Default cost

    std::map<wxString, int>::const_iterator i = TerrainMovementCost.find(Terrain.Lower());
    if (i != TerrainMovementCost.end())
    {
        MovementCost = i->second;
    }
    return MovementCost;
}

//-------------------------------------------------------------
// Checks if two lands are connected by a road in the given direction
//-------------------------------------------------------------

bool CAtlaParser::IsRoadConnected(CLand* pLand0, CLand* pLand1, int direction) const
{
    // Map direction to road attributes
    int road0 = SA_ROAD_N;
    int road1 = SA_ROAD_N;
    int i;
    CStruct* pStruct;

    switch (direction)
    {
    case North: road0 = SA_ROAD_N;  road1 = SA_ROAD_S;     break;
    case Northeast: road0 = SA_ROAD_NE; road1 = SA_ROAD_SW;    break;
    case Southeast: road0 = SA_ROAD_SE; road1 = SA_ROAD_NW;    break;
    case South: road0 = SA_ROAD_S;  road1 = SA_ROAD_N;     break;
    case Southwest: road0 = SA_ROAD_SW; road1 = SA_ROAD_NE;    break;
    case Northwest: road0 = SA_ROAD_NW; road1 = SA_ROAD_SE;    break;
    }

    bool Found = false;
    bool BadRoad = false;

    // Check road in source land
    for (i = 0; i < pLand0->Structs.Count(); i++)
    {
        pStruct = (CStruct*)pLand0->Structs.At(i);
        if (pStruct->Attr & road0)
        {
            BadRoad = ((pStruct->Attr & SA_ROAD_BAD) > 0);
            if (!BadRoad) Found = true;
        }
    }
    if (!Found) return false;
    
    // Check road in destination land
    Found = false;
    for (i = 0; i < pLand1->Structs.Count(); i++)
    {
        pStruct = (CStruct*)pLand1->Structs.At(i);
        if (pStruct->Attr & road1)
        {
            BadRoad = ((pStruct->Attr & SA_ROAD_BAD) > 0);
            if (!BadRoad) Found = true;
        }
    }
    return Found;
}

//-------------------------------------------------------------
// Checks if a hex has bad weather for the given month
// Based on hemisphere and tropical zone
//-------------------------------------------------------------

bool CAtlaParser::IsBadWeatherHex(CLand* pLand, int month) const
{
    int x, y, z;
    LandIdToCoord(pLand->Id, x, y, z);

    CPlane* pPlane = pLand->pPlane;
    if (pPlane && pPlane->TropicZoneMin < pPlane->TropicZoneMax)
    {
        // Weather zones are known
        if (y < pPlane->TropicZoneMin)
        {
            // Northern hemisphere
            switch (month)
            {
            case 0: case 9: case 10: case 11: // Winter months
                return true;
            }
        }
        else if (y > pPlane->TropicZoneMax)
        {
            // Southern hemisphere
            switch (month)
            {
            case 3: case 4: case 5: case 6: // Winter months
                return true;
            }
        }
        else
        {
            // Tropical zone
            switch (month)
            {
            case 4: case 5: case 10: case 11: // Rainy seasons
                return true;
            }
        }
    }
    return false;
}

//-------------------------------------------------------------
// Flexible land lookup - handles various input formats:
// - "text text (coords)"
// - "coords"
// - "cityname"
//-------------------------------------------------------------

CLand* CAtlaParser::GetLandFlexible(const wxString& description) const
{
    const wxString WorkStr = description.BeforeFirst(')').Trim().Trim(false);
    CLand* pLand = GetLand(WorkStr.AfterFirst('(').ToUTF8());

    if (!pLand)
    {
        pLand = GetLand(WorkStr.ToUTF8());
    }
    if (!pLand)
    {
        pLand = GetLandWithCity(WorkStr.BeforeFirst(' '));
    }
    return pLand;
}

//-------------------------------------------------------------
// Finds a land containing a city with the given name
//-------------------------------------------------------------

CLand* CAtlaParser::GetLandWithCity(const wxString& cityName) const
{
    if (cityName.IsEmpty()) return NULL;

    int                np, nl;
    CPlane* pPlane;
    CLand* pLand;

    for (np = 0; np < m_Planes.Count(); ++np)
    {
        pPlane = (CPlane*)m_Planes.At(np);
        for (nl = 0; nl < pPlane->Lands.Count(); ++nl)
        {
            pLand = (CLand*)pPlane->Lands.At(nl);
            if (cityName.CmpNoCase(wxString::FromUTF8(pLand->CityName.GetData())) == 0)
            {
                return pLand;
            }
        }
    }
    return NULL;
}