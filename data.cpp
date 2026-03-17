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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <wx/regex.h>

#include "stdafx.h"
#include "stdhdr.h"
#include "data.h"
#include "cstr.h"
#include "collection.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"
#include "objs.h"
#include "hash.h"
#include "ahapp.h"

CGameDataHelper* gpDataHelper = NULL;

const char* STRUCT_GATE = "GATE";

// Standard unit properties that are built-in (not from config)
const char* STD_UNIT_PROPS[] =
{
    PRP_FACTION_ID           ,
    PRP_FACTION              ,
    PRP_LAND_ID              ,
    PRP_STRUCT_ID            ,
    PRP_COMMENTS             ,
    PRP_ORDERS               ,
    PRP_STRUCT_OWNER         ,
    PRP_STRUCT_NAME          ,
    PRP_TEACHING             ,
    PRP_WEIGHT               ,
    PRP_WEIGHT_WALK          ,
    PRP_WEIGHT_RIDE          ,
    PRP_WEIGHT_FLY           ,
    PRP_WEIGHT_SWIM          ,
    PRP_MOVEMENT             ,
    PRP_DESCRIPTION          ,
    PRP_COMBAT               ,
    PRP_GUI_COLOR            ,
    PRP_MEN                  ,
    PRP_BEST_SKILL           ,
    PRP_BEST_SKILL_DAYS      ,
    PRP_SKILLS               ,
    PRP_MAG_SKILLS           ,
    PRP_STUFF                ,
    PRP_HORS                 ,
    PRP_WEAPONS              ,
    PRP_ARMOURS              ,
    PRP_MAG_ITEMS            ,
    PRP_JUNK_ITEMS           ,
    PRP_SEL_FACT_MEN         ,
    PRP_SEQUENCE             ,
    PRP_FRIEND_OR_FOE
};
const int STD_UNIT_PROPS_COUNT = sizeof(STD_UNIT_PROPS) / sizeof(*STD_UNIT_PROPS);

// Postfixes that identify skill-related properties
const char* SKILL_UNIT_POSTFIXES[] =
{
    PRP_SKILL_POSTFIX                ,  // _ (skill level)
    PRP_SKILL_STUDY_POSTFIX          ,  // _s (study level for Arcadia)
    PRP_SKILL_EXPERIENCE_POSTFIX     ,  // _x (experience level for Arcadia)
    PRP_SKILL_DAYS_POSTFIX           ,  // _d (skill days)
    PRP_SKILL_DAYS_EXPERIENCE_POSTFIX,  // _dx (experience days for Arcadia)
};
const int SKILL_UNIT_POSTFIXES_COUNT = sizeof(SKILL_UNIT_POSTFIXES) / sizeof(*SKILL_UNIT_POSTFIXES);

//=============================================================
// Helper function to identify skill-related properties
//=============================================================

BOOL IsASkillRelatedProperty(const char* propname)
{
    int i;
    const char* p;

    // Check if property ends with any of the skill postfixes
    for (i = 0; i < SKILL_UNIT_POSTFIXES_COUNT; i++)
    {
        p = strrchr(propname, SKILL_UNIT_POSTFIXES[i][0]);
        if (p && 0 == strcmp(p, SKILL_UNIT_POSTFIXES[i]))
            return TRUE;
    }
    return FALSE;
}

//=============================================================
// CBaseObject Implementation
//=============================================================

CBaseObject::CBaseObject() : Name(32), Description(128)
{
    Id = 0;
}

//-------------------------------------------------------------

void CBaseObject::Done()
{
    // Base implementation does nothing
}

//-------------------------------------------------------------

const char* CBaseObject::ResolveAlias(const char* alias)
{
    if (gpDataHelper)
        return gpDataHelper->ResolveAlias(alias);
    else
        return alias;
}

//-------------------------------------------------------------

/**
 * Gets property value by name
 * First checks dynamic properties, then falls back to built-in properties
 */
BOOL CBaseObject::GetProperty(const char* name,
    EValueType& type,
    const void*& value,
    EPropertyType  proptype
)
{
    BOOL Ok = TRUE;

    name = ResolveAlias(name);

    if (!TPropertyHolder::GetProperty(name, type, value, proptype))
    {
        // Built-in properties
        if (0 == stricmp(name, PRP_ID))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(Id));
        }
        else if (0 == stricmp(name, PRP_NAME))
        {
            type = eCharPtr;
            value = Name.GetData();
        }
        else if (0 == stricmp(name, PRP_FULL_TEXT))
        {
            type = eCharPtr;
            value = Description.GetData();
        }
        else
            Ok = FALSE;
    }
    return Ok;
}

//-------------------------------------------------------------

/**
 * Sets object name, saving original value if not already saved
 */
void CBaseObject::SetName(const char* newname)
{
    EValueType    type;
    const void* value;

    // Save original name if not already saved
    if (!GetProperty(PRP_ORG_NAME, type, value, eNormal))
        SetProperty(PRP_ORG_NAME, eCharPtr, (void*)Name.GetData(), eBoth);
    Name = newname;
}

//-------------------------------------------------------------

/**
 * Sets object description, saving original if not already saved
 */
void CBaseObject::SetDescription(const char* newdescr)
{
    EValueType    type;
    const void* value;

    // Save original description if not already saved
    if (!GetProperty(PRP_ORG_DESCR, type, value, eNormal))
        SetProperty(PRP_ORG_DESCR, eCharPtr, (void*)Description.GetData(), eBoth);
    Description = newdescr;
}

//-------------------------------------------------------------

/**
 * Restores original name from saved value
 */
void CBaseObject::ResetName()
{
    EValueType    type;
    const void* value;

    if (GetProperty(PRP_ORG_NAME, type, value, eNormal) && eCharPtr == type)
        Name = (const char*)value;
}

//-------------------------------------------------------------

/**
 * Restores original description from saved value
 */
void CBaseObject::ResetDescription()
{
    EValueType    type;
    const void* value;

    if (GetProperty(PRP_ORG_DESCR, type, value, eNormal) && eCharPtr == type)
        Description = (const char*)value;
}

//-------------------------------------------------------------

/**
 * Resets all normal properties to their original values
 */
void CBaseObject::ResetNormalProperties()
{
    TPropertyHolder::ResetNormalProperties();
    ResetName();
    ResetDescription();
}

//-------------------------------------------------------------

void CBaseObject::DebugPrint(CStr& sDest)
{
    sDest << "\n"
        << "Id          = " << Id << "\n"
        << "Name        = " << Name.GetData() << "\n"
        << "Description = " << Description.GetData() << "\n";
}

//=============================================================
// CFaction Implementation
//=============================================================

void CFaction::DebugPrint(CStr& sDest)
{
    CBaseObject::DebugPrint(sDest);
    sDest << "UnclaimedSilver = " << UnclaimedSilver << "\n";
}

//=============================================================
// CAttitude Implementation
//=============================================================

void CAttitude::SetStance(int newstance)
{
    Stance = newstance; // TODO: Add precedence logic
}

//-------------------------------------------------------------

BOOL CAttitude::IsDeclaredAs(int attitude)
{
    return (Stance == attitude);
}

//-------------------------------------------------------------

BOOL CAttitude::IsEqual(CAttitude* attitude)
{
    return (attitude->IsDeclaredAs(Stance));
}

//=============================================================
// CLand Implementation
//=============================================================

CLand::CLand() : CBaseObject(), Units(32), UnitsSeq(32)
{
    Flags = 0;
    AlarmFlags = 0;
    EventFlags = 0;
    guiUnit = 0;
    pPlane = NULL;
    CoastBits = 0;
    AtlaclientsLastTurnNo = 0;
    guiColor = -1;
    WeatherWillBeGood = FALSE;
    Wages = 0.0;
    MaxWages = 0;
    Entertainment = 0;
    for (int i = 0; i <= ATT_UNDECLARED; i++) Troops[i] = 0;
    ResetAllExits();  // Initialize all exits as "maybe"
}

//-------------------------------------------------------------

CLand::~CLand()
{
    ResetUnitsAndStructs();
    Structs.FreeAll();      // Free structures
    EdgeStructs.FreeAll();   // Free edge structures
    Units.FreeAll();         // Free units
    UnitsSeq.DeleteAll();    // Clear sequence list (doesn't free units)
    Products.FreeAll();      // Free products
}

void CLand::DebugPrint(CStr& sDest)
{
    CBaseObject::DebugPrint(sDest);
    sDest << "Taxable   = " << Taxable << "\n";
}

//-------------------------------------------------------------

/**
 * Adds a unit to the land
 * Updates both ID-based and sequence-based collections
 */
BOOL CLand::AddUnit(CUnit* pUnit)
{
    if (Units.Insert(pUnit))
    {
        pUnit->LandId = Id;
        Flags |= LAND_UNITS;
        UnitsSeq.Insert(pUnit);
        // Store sequence number for later retrieval
        pUnit->SetProperty(PRP_SEQUENCE, eLong, reinterpret_cast<void*>(static_cast<uintptr_t>(UnitsSeq.Count())), eNormal);
        return TRUE;
    }
    else
        return FALSE;
}

//-------------------------------------------------------------

/**
 * Removes a unit from the land
 * Removes from both ID-based and sequence-based collections
 */
void CLand::RemoveUnit(CUnit* pUnit)
{
    int i;
    CUnit* p;

    if (Units.Search(pUnit, i))
    {
        Units.AtDelete(i);

        // Also remove from sequence list
        for (i = UnitsSeq.Count() - 1; i >= 0; i--)
        {
            p = (CUnit*)UnitsSeq.At(i);
            if (p->Id == pUnit->Id)
            {
                UnitsSeq.AtDelete(i);
                break;
            }
        }
    }
}

//-------------------------------------------------------------

/**
 * Deletes all new units of specified faction (or all if factionId==0)
 */
void CLand::DeleteAllNewUnits(int factionId)
{
    int i;
    CUnit* pUnit;

    // Remove from sequence list first
    for (i = UnitsSeq.Count() - 1; i >= 0; i--)
    {
        pUnit = (CUnit*)UnitsSeq.At(i);
        if (IS_NEW_UNIT(pUnit) && (pUnit->FactionId == factionId || 0 == factionId))
            UnitsSeq.AtDelete(i);
    }

    // Then free from main collection
    for (i = Units.Count() - 1; i >= 0; i--)
    {
        pUnit = (CUnit*)Units.At(i);
        if (IS_NEW_UNIT(pUnit) && (pUnit->FactionId == factionId || 0 == factionId))
            Units.AtFree(i);
    }
}

//-------------------------------------------------------------

/**
 * Resets units and structures to their original state
 * Called before processing orders
 */
void CLand::ResetUnitsAndStructs()
{
    int i, k;
    CUnit* pUnit;
    CStruct* pStruct;

    for (i = Units.Count() - 1; i >= 0; i--)
    {
        pUnit = (CUnit*)Units.At(i);
        pUnit->ResetNormalProperties();

        // Initialize end turn description for our units
        if (pUnit->IsOurs)
        {
            pUnit->m_EndTurnDescription = pUnit->Description;
        }

        // Clear movement data
        if (pUnit->pMovement)
        {
            delete pUnit->pMovement;
            pUnit->pMovement = NULL;
        }
        if (pUnit->pMoveA3Points)
        {
            delete pUnit->pMoveA3Points;
            pUnit->pMoveA3Points = NULL;
        }
        if (pUnit->pStudents)
            pUnit->pStudents->DeleteAll(); // Keep collection but clear it
    }

    for (k = 0; k < Structs.Count(); k++)
    {
        pStruct = (CStruct*)Structs.At(k);
        pStruct->ResetNormalProperties();
    }
}

//-------------------------------------------------------------

/**
 * Gets next available number for a new unit
 * Scans existing NEW units to find next unused number
 */
int CLand::GetNextNewUnitNo()
{
    int no = 1;
    int i, x;
    CUnit* pUnit;

    for (i = Units.Count() - 1; i >= 0; i--)
    {
        pUnit = (CUnit*)Units.At(i);
        if (IS_NEW_UNIT(pUnit))
        {
            x = REVERSE_NEW_UNIT_ID(pUnit->Id);
            if (x >= no)
                no = x + 1;
        }
    }
    if (no > 0xFFFE) // 0xFFFF is reserved
    {
        no = 1; // Wrap around
    }
    return no;
}

//-------------------------------------------------------------

/**
 * Finds structure by ID
 */
CStruct* CLand::GetStructById(long id)
{
    CStruct* pStruct = NULL;
    CBaseObject Dummy;
    int i;

    Dummy.Id = id;
    if (Structs.Search(&Dummy, i))
        pStruct = (CStruct*)Structs.At(i);

    return pStruct;
}

//-------------------------------------------------------------
CStruct* CLand::AddNewStruct(CStruct* pNewStruct)
{
    int       idx;
    CStruct* pStruct;

    if (Structs.Search(pNewStruct, idx))
    {
        pStruct = (CStruct*)Structs.At(idx);

        // Для шахт сохраняем ссылки
        if (pStruct->Attr & SA_SHAFT)
        {
            int  x1, x2, x3;
            BOOL Link;

            x1 = pNewStruct->Description.FindSubStr(";");
            x2 = pNewStruct->Description.FindSubStr("links");
            x3 = pNewStruct->Description.FindSubStr("to");
            Link = (x1 >= 0 && x1 < x2 && x2 < x3);

            if (Link || pNewStruct->Description.GetLength() > pStruct->Description.GetLength())
                pStruct->Description = pNewStruct->Description;
        }
        else
        {
            pStruct->Description = pNewStruct->Description;
        }

        pStruct->Name = pNewStruct->Name;
        pStruct->LandId = pNewStruct->LandId;
        pStruct->OwnerUnitId = pNewStruct->OwnerUnitId;
        pStruct->Load = pNewStruct->Load;
        pStruct->Attr = pNewStruct->Attr;
        pStruct->Kind = pNewStruct->Kind;
        pStruct->Location = pNewStruct->Location;

        delete pNewStruct;

   }
    else
    {
        Structs.Insert(pNewStruct);

        // Устанавливаем флаги ТОЛЬКО для новой структуры
        if (0 == pNewStruct->Attr)
        {
            Flags |= LAND_STR_GENERIC;
        }
        else
        {
            if (pNewStruct->Attr & SA_HIDDEN)     Flags |= LAND_STR_HIDDEN;
            if (pNewStruct->Attr & SA_MOBILE)     Flags |= LAND_STR_MOBILE;
            if (pNewStruct->Attr & SA_SHAFT)      Flags |= LAND_STR_SHAFT;
            if (pNewStruct->Attr & SA_GATE)       Flags |= LAND_STR_GATE;
            if (pNewStruct->Attr & SA_ROAD_N)     Flags |= LAND_STR_ROAD_N;
            if (pNewStruct->Attr & SA_ROAD_NE)    Flags |= LAND_STR_ROAD_NE;
            if (pNewStruct->Attr & SA_ROAD_SE)    Flags |= LAND_STR_ROAD_SE;
            if (pNewStruct->Attr & SA_ROAD_S)     Flags |= LAND_STR_ROAD_S;
            if (pNewStruct->Attr & SA_ROAD_SW)    Flags |= LAND_STR_ROAD_SW;
            if (pNewStruct->Attr & SA_ROAD_NW)    Flags |= LAND_STR_ROAD_NW;
        }

        pStruct = pNewStruct;
    }

    return pStruct;
}

//-------------------------------------------------------------

/**
 * Calculates alarm and troop strength flags based on units present
 * Used for map display and alerts
 */
void CLand::SetFlagsFromUnits()
{
    int i;
    int alarm = ATT_FRIEND1;
    int guard_stance = -1;
    int claim = -1;
    long player_id;
    long men, weapons, armours;
    const void* stance;
    const void* troops;
    const void* gear_weapon;
    const void* gear_armour;
    EValueType type;
    CUnit* pUnit;

    Flags &= ~(LAND_TAX_NEXT | LAND_TRADE_NEXT);
    AlarmFlags = 0;
    player_id = atol(gpApp->GetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID));

    // Reset troop counts
    for (i = ATT_UNDECLARED; i >= 0; i--) Troops[i] = 0;

    // Analyze each unit
    for (i = Units.Count() - 1; i >= 0; i--)
    {
        men = 0;
        weapons = 0;
        armours = 0;
        pUnit = (CUnit*)UnitsSeq.At(i);
        
        // Presence flags
        if (pUnit->FactionId == player_id) AlarmFlags |= PRESENCE_OWN;
        
        // Tax/Pillage next turn flags
        if (((pUnit->Flags & UNIT_FLAG_TAXING) || (pUnit->Flags & UNIT_FLAG_PILLAGING)) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
            Flags |= LAND_TAX_NEXT;
            
        // Production next turn flags
        if ((pUnit->Flags & UNIT_FLAG_PRODUCING) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
            Flags |= LAND_TRADE_NEXT;
            
        // Get stance (attitude) of this unit
        if (!((pUnit->GetProperty(PRP_FRIEND_OR_FOE, type, stance, eNormal)) && (type == eLong)))
        {
            stance = reinterpret_cast<void*>(static_cast<uintptr_t>(gpDataHelper->GetAttitudeForFaction(0)));
        }
        
        // Guard stance
        if ((pUnit->Flags & UNIT_FLAG_GUARDING) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
        {
            if ((static_cast<long>(reinterpret_cast<intptr_t>(stance)) > guard_stance) && 
                (static_cast<long>(reinterpret_cast<intptr_t>(stance)) < ATT_UNDECLARED))
                guard_stance = static_cast<long>(reinterpret_cast<intptr_t>(stance));
        }
        
        // Claim stance (taxers/producers)
        if (((pUnit->Flags & UNIT_FLAG_TAXING) || (pUnit->Flags & UNIT_FLAG_PILLAGING) || 
             (pUnit->Flags & UNIT_FLAG_PRODUCING)) && !(pUnit->Flags & UNIT_FLAG_GIVEN))
        {
            if ((static_cast<long>(reinterpret_cast<intptr_t>(stance)) > claim) && 
                (static_cast<long>(reinterpret_cast<intptr_t>(stance)) < ATT_UNDECLARED))
                claim = static_cast<long>(reinterpret_cast<intptr_t>(stance));
        }
        
        // Calculate troop strength for map display
        if ((static_cast<long>(reinterpret_cast<intptr_t>(stance)) <= ATT_UNDECLARED) && 
            (static_cast<long>(reinterpret_cast<intptr_t>(stance)) >= 0))
        {
            // Update highest presence alarm level
            if (static_cast<long>(reinterpret_cast<intptr_t>(stance)) > alarm) 
                alarm = static_cast<long>(reinterpret_cast<intptr_t>(stance));
                
            // Get men count
            if ((pUnit->GetProperty(PRP_MEN, type, troops, eNormal)) && (type == eLong))
                men = static_cast<long>(reinterpret_cast<intptr_t>(troops));
                
            // Get weapons count
            if ((pUnit->GetProperty(PRP_WEAPONS, type, gear_weapon, eNormal)) && (type == eLong))
                weapons = static_cast<long>(reinterpret_cast<intptr_t>(gear_weapon));
                
            // Get armour count
            if ((pUnit->GetProperty(PRP_ARMOURS, type, gear_armour, eNormal)) && (type == eLong))
                armours = static_cast<long>(reinterpret_cast<intptr_t>(gear_armour));
                
            // Normalize counts (can't have more weapons than men, more armour than weapons)
            if (weapons > men) weapons = men;
            if (armours > weapons) armours = weapons;
            
            // Calculate troop strength value (weighted formula)
            Troops[static_cast<long>(reinterpret_cast<intptr_t>(stance))] += 2 * men + 2 * weapons + armours + 1;
        }
    }

    // Determine claim flag if none set
    if (claim < 0)
    {
        long max = 0;
        for (i = ATT_UNDECLARED - 1; i >= 0; i--)
        {
            if (Troops[i] > max)
            {
                claim = i;
                max = Troops[i];
            }
        }
    }
    
    // Set claim flags
    switch (claim)
    {
    case ATT_FRIEND1:
        AlarmFlags |= CLAIMED_BY_FRIEND;
        break;
    case ATT_FRIEND2:
        AlarmFlags |= CLAIMED_BY_OWN;
        break;
    case ATT_NEUTRAL:
        AlarmFlags |= CLAIMED_BY_NEUTRAL;
        break;
    case ATT_ENEMY:
        AlarmFlags |= CLAIMED_BY_ENEMY;
    }

    // Set guard flags
    if ((guard_stance >= 0) && (guard_stance < ATT_UNDECLARED)) AlarmFlags |= GUARDED;
    switch (guard_stance)
    {
    case ATT_FRIEND1:
        AlarmFlags |= GUARDED_BY_FRIEND;
        break;
    case ATT_FRIEND2:
        AlarmFlags |= GUARDED_BY_OWN;
        break;
    case ATT_NEUTRAL:
        AlarmFlags |= GUARDED_BY_NEUTRAL;
        break;
    case ATT_ENEMY:
        AlarmFlags |= GUARDED_BY_ENEMY;
    }

    // Set presence flags
    switch (alarm)
    {
    case ATT_FRIEND1:
        AlarmFlags |= PRESENCE_FRIEND;
        break;
    case ATT_NEUTRAL:
        AlarmFlags |= PRESENCE_NEUTRAL;
        break;
    case ATT_ENEMY:
        AlarmFlags |= PRESENCE_ENEMY;
    }
    
    // Set alarm if unfriendly presence exceeds guard/claim
    if ((alarm > guard_stance) && (alarm > claim) && (alarm > ATT_FRIEND2)) 
        AlarmFlags |= ALARM;

    // Normalize troop counts to 0-3 scale for map display
    int minimen = 2 * atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_MIN_SEL_MEN));
    for (i = ATT_UNDECLARED; i >= 0; i--)
    {
        long limit = 2000;
        men = Troops[i];
        Troops[i] = 0;
        // Convert to 0-3 scale using thresholds
        for (int f = 10; f >= 3; f -= 1)
        {
            long c = (long)((float)f * f / 100 * limit);
            if (men >= c) Troops[i]++;
        }
        if (men >= minimen) Troops[i]++;
    }
}

//-------------------------------------------------------------

/**
 * Sets exit coordinates for a direction
 */
void CLand::SetExit(int direction, int x, int y)
{
    if (direction >= 0 && direction < 6)
    {
        xExit[direction] = x;
        yExit[direction] = y;
    }
}

//-------------------------------------------------------------

/**
 * Marks all exits as closed (impassable)
 */
void CLand::CloseAllExits()
{
    for (int i = 0; i < 6; ++i) 
    { 
        xExit[i] = EXIT_CLOSED; 
        yExit[i] = EXIT_CLOSED; 
    }
}

//-------------------------------------------------------------

/**
 * Resets all exits to "maybe" (unknown)
 */
void CLand::ResetAllExits()
{
    for (int i = 0; i < 6; ++i) 
    { 
        xExit[i] = EXIT_MAYBE; 
        yExit[i] = EXIT_MAYBE; 
    }
}

/**
 * Finds direction to a hex by its ID
 * Returns -1 if not found
 */
int CLand::FindExit(long hexId) const
{
    int x, y, z;
    LandIdToCoord(hexId, x, y, z);
    for (int i = 0; i < 6; ++i)
    {
        if (xExit[i] == x && yExit[i] == y)
            return i;
    }
    return -1;
}

//-------------------------------------------------------------

/**
 * Calculates load for structures based on units inside
 */
void CLand::CalcStructsLoad()
{
    int i, k;
    CUnit* pUnit;
    CBaseObject Dummy;
    CStruct* pStruct;
    EValueType type;
    const void* value;

    // Reset loads
    for (i = 0; i < Structs.Count(); i++)
    {
        pStruct = (CStruct*)Structs.At(i);
        pStruct->Load = 0;
    }

    // Add weight of units in structures
    for (i = Units.Count() - 1; i >= 0; i--)
    {
        pUnit = (CUnit*)UnitsSeq.At(i);
        if (pUnit->GetProperty(PRP_STRUCT_ID, type, value, eNormal) && 
            (eLong == type) && (static_cast<long>(reinterpret_cast<intptr_t>(value)) > 0))
        {
            Dummy.Id = static_cast<long>(reinterpret_cast<intptr_t>(value));
            if (Structs.Search(&Dummy, k))
            {
                pStruct = (CStruct*)Structs.At(k);
                pStruct->Load += pUnit->Weight[0]; // Walk weight
            }
        }
    }
}

//-------------------------------------------------------------

/**
 * Removes edge structures for a specific direction
 */
void CLand::RemoveEdgeStructs(int direction)
{
    CStruct* pEdge;
    for (int i = EdgeStructs.Count() - 1; i >= 0; i--)
    {
        pEdge = (CStruct*)EdgeStructs.At(i);
        if ((pEdge != NULL) && (pEdge->Location == direction % 6))
        {
            EdgeStructs.AtFree(i);
        }
    }
}

//-------------------------------------------------------------

/**
 * Adds a new edge structure
 */
void CLand::AddNewEdgeStruct(const char* name, int direction)
{
    CStruct* pEdge = new CStruct;

    pEdge->Kind = name;
    pEdge->Location = direction % 6;
    EdgeStructs.Insert(pEdge);
}

//--------------------------------------------------------------------------
// Get all regions in radius (RECURSION!!)
// Uses xExit/yExit arrays to navigate between hexes
//--------------------------------------------------------------------------
void CLand::GetRegionsInRadius(int radius, std::vector<CLand*>& result) const
{
    // Base case: radius == 0
    if (radius <= 0)
    {
        // Check if this land is already in the result vector
        bool found = false;
        for (size_t i = 0; i < result.size(); i++)
        {
            if (result[i] == this)
            {
                found = true;
                break;
            }
        }

        // If not found, add it
        if (!found)
        {
            result.push_back(const_cast<CLand*>(this));
        }

        return; // Return the result (same as input or with new region added)
    }
    else
    {
        // Recursive case: radius > 0

        // First, add current land if not already present
        bool found = false;
        for (size_t i = 0; i < result.size(); i++)
        {
            if (result[i] == this)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            result.push_back(const_cast<CLand*>(this));
        }

        // Iterate through all 6 possible directions
        for (int i = 0; i < 6; i++)
        {
            // Check if exit exists (not closed and not unknown)
            if (this->xExit[i] != EXIT_CLOSED && this->xExit[i] != EXIT_MAYBE)
            {
                // Get neighbor land by coordinates
                CLand* pNeighbor = NULL;

                // Need to access the global Atlantis parser to get land by coordinates
                if (gpApp && gpApp->m_pAtlantis)
                {
                    int x, y, z;
                    LandIdToCoord(this->Id, x, y, z);

                    // Use the neighbor's coordinates from exit arrays
                    pNeighbor = gpApp->m_pAtlantis->GetLand(this->xExit[i],
                        this->yExit[i],
                        z,
                        TRUE);
                }

                // If neighbor found, recursively process it with reduced radius
                if (pNeighbor)
                {
                    pNeighbor->GetRegionsInRadius(radius - 1, result);
                }
            }
        }

        return; // Return the accumulated result
    }
}

//=============================================================
// CStruct Implementation
//=============================================================

void CStruct::ResetNormalProperties()
{
    TPropertyHolder::ResetNormalProperties();
    Load = 0;
    SailingPower = 0;
}

//=============================================================
// Collection Classes Implementation
//=============================================================

CBaseColl::CBaseColl() : CCollection()
{
}

CBaseColl::CBaseColl(int nDelta) : CCollection(nDelta)
{
}

/**
 * Frees a CBaseObject from collection
 * Only frees objects with positive IDs (not temporary)
 */
void CBaseColl::FreeItem(void* pItem)
{
    if (!pItem) return;
    CBaseObject* pBase = static_cast<CBaseObject*>(pItem);
    if (pBase->Id > 0)
    {
        pBase->Done();
        delete pBase;
    }
}

//-------------------------------------------------------------

CBaseCollById::CBaseCollById() : CSortedCollection()
{
}

CBaseCollById::CBaseCollById(int nDelta) : CSortedCollection(nDelta)
{
}

void CBaseCollById::FreeItem(void* pItem)
{
    if (pItem)
    {
        CBaseObject* pBase = (CBaseObject*)pItem;
        try
        {
            if (pBase->Id > 0)
            {
                pBase->Done();
                delete pBase;
            }
        }
        catch (...)
        {
            // Ignore exceptions during cleanup
        }
    }
}

//-------------------------------------------------------------

/**
 * Compares two objects by ID for sorting
 */
int CBaseCollById::Compare(void* pItem1, void* pItem2) const
{
    CBaseObject* pBase1 = (CBaseObject*)pItem1;
    CBaseObject* pBase2 = (CBaseObject*)pItem2;

    if (!pBase1 || !pBase2)
        return 0;

    if (pBase1->Id > pBase2->Id)
        return 1;
    else if (pBase1->Id < pBase2->Id)
        return -1;
    else
        return 0;
}

//-------------------------------------------------------------

CPlane::~CPlane()
{
    Lands.FreeAll(); // Free all lands in this plane
}

//-------------------------------------------------------------

/**
 * Compares two objects by name for sorting
 */
int CBaseCollByName::Compare(void* pItem1, void* pItem2) const
{
    CBaseObject* pBase1 = (CBaseObject*)pItem1;
    CBaseObject* pBase2 = (CBaseObject*)pItem2;

    if (!pBase1 || !pBase2)
        return 0;

    return stricmp(pBase1->Name.GetData(), pBase2->Name.GetData());
}

//=============================================================
// TProdDetails Implementation
//=============================================================

void TProdDetails::Empty()
{
    int i;

    skillname.Empty();
    skilllevel = 0;
    months = 0;
    toolname.Empty();
    toolhelp = 0;
    for (i = 0; i < MAX_RES_NUM; i++)
    {
        resname[i].Empty();
        resamt[i] = 0;
    }
}

//=============================================================
// Utility Functions
//=============================================================

/**
 * Creates qualified property name with dot separator
 * Format: "prefix.shortname"
 */
void MakeQualifiedPropertyName(const char* prefix, const char* shortname, CStr& FullName)
{
    FullName.Empty();
    FullName << prefix;

    if (!FullName.IsEmpty() && '.' != FullName.GetData()[FullName.GetLength() - 1])
        FullName << ".";
    FullName << shortname;
}

//-------------------------------------------------------------

/**
 * Splits qualified property name into prefix and short name
 */
void SplitQualifiedPropertyName(const char* fullname, CStr& Prefix, CStr& ShortName)
{
    const char* p;

    Prefix.Empty();
    ShortName.Empty();

    if (fullname && *fullname)
    {
        p = strrchr(fullname, '.');
        if (p)
        {
            ShortName = p + 1;
            Prefix.AddBuf(fullname, p - fullname);
        }
    }
}

//--------------------------------------------------------------------------

/**
 * Evaluates object against multiple filter conditions
 * Used by filter dialogs
 */
BOOL EvaluateBaseObjectByBoxes(CBaseObject* pObj, CStr* Property, eCompareOp* CompareOp, CStr* sValue, long* lValue, int count)
{
    int i;
    EValueType type;
    const void* value;
    BOOL ok = TRUE;

    for (i = 0; i < count; i++)
    {
        if (!Property[i].IsEmpty() && (NOP != CompareOp[i]))
        {
            if (!pObj->GetProperty(Property[i].GetData(), type, value, eNormal))
            {
                // Property doesn't exist - create default value based on type
                CStrInt* pSI, SI(Property[i].GetData(), 0);
                int idx;

                if (gpApp->m_pAtlantis->m_UnitPropertyTypes.Search(&SI, idx))
                {
                    pSI = (CStrInt*)gpApp->m_pAtlantis->m_UnitPropertyTypes.At(idx);
                    type = (EValueType)pSI->m_value;
                    if (eLong == type)
                        value = reinterpret_cast<void*>(static_cast<uintptr_t>(0));
                    else
                        value = "";
                }
                else
                {
                    type = eLong;
                    value = reinterpret_cast<void*>(static_cast<uintptr_t>(0));
                }
            }
            
            // Apply comparison based on value type
            switch (type)
            {
            case eLong:
                switch (CompareOp[i])
                {
                case GT: ok = (static_cast<long>(reinterpret_cast<intptr_t>(value)) > lValue[i]); break;
                case GE: ok = (static_cast<long>(reinterpret_cast<intptr_t>(value)) >= lValue[i]); break;
                case EQ: ok = (static_cast<long>(reinterpret_cast<intptr_t>(value)) == lValue[i]); break;
                case LE: ok = (static_cast<long>(reinterpret_cast<intptr_t>(value)) <= lValue[i]); break;
                case LT: ok = (static_cast<long>(reinterpret_cast<intptr_t>(value)) < lValue[i]); break;
                case NE: ok = (static_cast<long>(reinterpret_cast<intptr_t>(value)) != lValue[i]); break;
                default: break;
                }
                break;

            case eCharPtr:
                switch (CompareOp[i])
                {
                case GT: ok = (stricmp((const char*)value, sValue[i].GetData()) > 0); break;
                case GE: ok = (stricmp((const char*)value, sValue[i].GetData()) >= 0); break;
                case EQ: ok = (stricmp((const char*)value, sValue[i].GetData()) == 0); break;
                case LE: ok = (stricmp((const char*)value, sValue[i].GetData()) <= 0); break;
                case LT: ok = (stricmp((const char*)value, sValue[i].GetData()) < 0); break;
                case NE: ok = (stricmp((const char*)value, sValue[i].GetData()) != 0); break;
                default: break;
                }
                break;

            default:
                ok = FALSE;
            }
            if (!ok)
                break;
        }
    }

    return ok;
}

//=============================================================
// Land ID Conversion Functions
//=============================================================

/**
 * Packs (x,y,z) coordinates into a single 32-bit ID
 * 
 * Format:
 * - x: bits 0-11 (12 bits) + XY_DELTA offset
 * - y: bits 12-23 (12 bits) + XY_DELTA offset
 * - z: bits 24-31 (8 bits)
 * 
 * XY_DELTA is used to handle negative coordinates
 */
#define XY_DELTA   0x00000800  // 2048, enough for -2048 to +2047 range
#define X_MASK     0x00000FFF  // 12 bits
#define Y_MASK     0x00FFF000  // 12 bits shifted 12
#define Z_MASK     0xFF000000  // 8 bits shifted 24
#define Y_SHIFT    12
#define Z_SHIFT    24

long LandCoordToId(int x, int y, int z)
{
    return ((z << Z_SHIFT) & Z_MASK) |
           (((y + XY_DELTA) << Y_SHIFT) & Y_MASK) |
           ((x + XY_DELTA) & X_MASK);
}

//-------------------------------------------------------------

void LandIdToCoord(long id, int& x, int& y, int& z)
{
    x = (id & X_MASK) - XY_DELTA;
    y = ((id & Y_MASK) >> Y_SHIFT) - XY_DELTA;
    z = (id & Z_MASK) >> Z_SHIFT;
}

//-------------------------------------------------------------

/**
 * Test function to verify coordinate encoding/decoding
 * Deliberately causes division by zero if any conversion fails
 */
void TestLandId()
{
    int x0, y0, z0;
    int x1, y1, z1;
    int x = 0;
    long id;

#define _x_min_  -48
#define _x_max_  500
#define _y_min_  -48
#define _y_max_  500
#define _z_min_  0
#define _z_max_  255

    for (z0 = _z_min_; z0 <= _z_max_; z0++)
        for (y0 = _y_min_; y0 <= _y_max_; y0++)
            for (x0 = _x_min_; x0 <= _x_max_; x0++)
            {
                id = LandCoordToId(x0, y0, z0);
                LandIdToCoord(id, x1, y1, z1);
                if ((x0 != x1) || (y0 != y1) || (z0 != z1))
                {
                    // Crash if conversion fails
                    x = 10 / x;
                }
            }
    x = 0;
    x = 1;
}