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
#include "data.h"

#include <wx/arrstr.h>
#include <wx/tokenzr.h>
#include "objs.h"
#include "hash.h"
#include "ahapp.h"

// Static member initialization
CStrStrColl* CUnit::m_PropertyGroupsColl = NULL;
CStr          CUnit::m_CustomFlagNames[UNIT_CUSTOM_FLAG_COUNT];
BOOL          CUnit::m_CustomFlagNamesLoaded = FALSE;

//=============================================================
// Unit Flag Mappings - map report flags to orders and internal bits
//=============================================================

static TUnitFlagMapping s_UnitFlagMappings[] = {
    // Simple flags with 0/1 values
    TUnitFlagMapping("taxing",
                     "AUTOTAX",         "1",        "0",        UNIT_FLAG_TAXING),
    TUnitFlagMapping("on guard",
                     "AUTOGUARD",       "1",        "0",        UNIT_FLAG_GUARDING),
    TUnitFlagMapping("avoiding",
                     "AVOID",           "1",        "0",        UNIT_FLAG_AVOIDING),
    TUnitFlagMapping("behind",
                     "BEHIND",          "1",        "0",        UNIT_FLAG_BEHIND),
    TUnitFlagMapping("holding",
                     "HOLD",            "1",        "0",        UNIT_FLAG_HOLDING),
    TUnitFlagMapping("receiving no aid",
                     "NOAID",           "1",        "0",        UNIT_FLAG_RECEIVING_NO_AID),
    TUnitFlagMapping("won't cross water",
                     "NOCROSS",         "1",        "0",        UNIT_FLAG_NO_CROSS_WATER),
    TUnitFlagMapping("sharing",
                     "SHARE",           "1",        "0",        UNIT_FLAG_SHARING),

    // Special flags with parameters
    // CONSUME command
    TUnitFlagMapping("consuming unit's food",
                    "CONSUME",          "UNIT",     "",         UNIT_FLAG_CONSUMING_UNIT),
    TUnitFlagMapping("consuming faction's food",
                      "CONSUME",        "FACTION",  "",         UNIT_FLAG_CONSUMING_FACTION),

    // REVEAL command
    TUnitFlagMapping("revealing unit",
                    "REVEAL",           "UNIT",     "",         UNIT_FLAG_REVEALING_UNIT),
    TUnitFlagMapping("revealing faction",
                    "REVEAL",           "FACTION",  "",         UNIT_FLAG_REVEALING_FACTION),

    // SPOILS command - multiple types map to same flag
    TUnitFlagMapping("weightless battle spoils",
                    "SPOILS",           "NONE",     "",         UNIT_FLAG_SPOILS),
    TUnitFlagMapping("flying battle spoils",
                    "SPOILS",           "FLY",      "",         UNIT_FLAG_SPOILS),
    TUnitFlagMapping("walking battle spoils",
                    "SPOILS",           "WALK",     "",         UNIT_FLAG_SPOILS),
    TUnitFlagMapping("riding battle spoils",
                    "SPOILS",           "RIDE",     "",         UNIT_FLAG_SPOILS),
    TUnitFlagMapping("swiming battle spoils",
                    "SPOILS",           "SWIM",     "",         UNIT_FLAG_SPOILS),
    TUnitFlagMapping("sailing battle spoils",
                    "SPOILS",           "SAIL",     "",         UNIT_FLAG_SPOILS),
};

static const int s_UnitFlagMappingsCount = sizeof(s_UnitFlagMappings) / sizeof(TUnitFlagMapping);

//=============================================================
// CUnit Implementation
//=============================================================

CStrStrColl* CUnit::GetPropertyGroups()
{
    return m_PropertyGroupsColl;
}

//-------------------------------------------------------------

CUnit::CUnit() : CBaseObject(), Comments(16), DefOrders(32), Orders(32), Errors(32), Events(32)
{
    IsOurs = false;
    FactionId = 0;
    pFaction = NULL;
    LandId = 0;
    Teaching = 0.0;
    pMovement = NULL;
    pMoveA3Points = NULL;
    pStudents = NULL;
    SilvRcvd = 0;
    Flags = 0;
    FlagsOrg = 0;
    FlagsLast = ~Flags;
    reqMovementSpeed = 0;
    memset(Weight, 0, sizeof(Weight));

    m_EndTurnDescription.Empty();
}

//-------------------------------------------------------------

CUnit::~CUnit()
{
    if (pMovement)
    {
        delete pMovement;
        pMovement = NULL;
    }
    if (pMoveA3Points)
    {
        delete pMoveA3Points;
        pMoveA3Points = NULL;
    }
    if (pStudents)
    {
        pStudents->DeleteAll();
        delete pStudents;
        pStudents = NULL;
    }
}

//-------------------------------------------------------------

/**
 * Creates a simple copy of the unit (without relationships)
 * Used for battle dialog and other temporary operations
 */
CUnit* CUnit::AllocSimpleCopy()
{
    CUnit* pUnit = new CUnit;
    int     idx;

    pUnit->Id = Id;
    pUnit->Name = Name;
    pUnit->Description = Description;

    pUnit->IsOurs = IsOurs;
    pUnit->FactionId = FactionId;
    pUnit->pFaction = pFaction;
    pUnit->LandId = LandId;
    pUnit->SilvRcvd = SilvRcvd;
    pUnit->Teaching = Teaching;
    pUnit->Comments = Comments;
    pUnit->DefOrders = DefOrders;
    pUnit->Orders = Orders;
    pUnit->Errors = Errors;
    pUnit->Events = Events;
    pUnit->StudyingSkill = StudyingSkill;
    pUnit->Flags = Flags;
    pUnit->FlagsOrg = FlagsOrg;
    pUnit->FlagsLast = FlagsLast;

    memcpy(pUnit->Weight, Weight, sizeof(Weight));

    // Copy all properties
    const char* propname;
    EValueType    type;
    const void* value;

    idx = 0;
    propname = GetPropertyName(idx);
    while (propname)
    {
        if (GetProperty(propname, type, value, eNormal))
            pUnit->SetProperty(propname, type, value, eNormal);

        propname = GetPropertyName(++idx);
    }

    return pUnit;
}

//-------------------------------------------------------------

/**
 * Extracts user comments from default orders
 * Comments are lines starting with ';' in the DefOrders string
 */
void CUnit::ExtractCommentsFromDefOrders()
{
    const char* p;
    CStr         S;

    Comments.Empty();
    p = DefOrders.GetData();
    while (p)
    {
        p = S.GetToken(p, '\n', TRIM_ALL);
        if ((S.GetLength() > 1) && (';' == S.GetData()[0]))
        {
            Comments.SetStr(&S.GetData()[1], S.GetLength() - 1);
            Comments.TrimLeft(TRIM_ALL);
            break; // Only first comment line is used
        }
    }
}

//-------------------------------------------------------------

/**
 * Resets unit properties to original state (from report)
 * Called before processing orders for a new turn
 */
void CUnit::ResetNormalProperties()
{
    CBaseObject::ResetNormalProperties();
    Teaching = 0;
    StudyingSkill.Empty();
    ProducingItem.Empty();
    SilvRcvd = 0;

    Flags = FlagsOrg;      // Restore original flags
    FlagsLast = ~Flags;     // Force flag string regeneration
    reqMovementSpeed = 0;

    // Recalculate weight and movement capabilities
    CalcWeightsAndMovement();
}

//-------------------------------------------------------------

/**
 * Adds weight contribution from an item type
 * @param nitems Number of items
 * @param weights Weight array for this item type
 * @param movenames Names of movement modes (unused)
 * @param nweights Number of weight values
 */
void CUnit::AddWeight(int nitems, int* weights, const char** movenames, int nweights)
{
    int  i;
    int  NW = std::min(nweights, MOVE_MODE_MAX);

    for (i = 0; i < NW; i++)
        Weight[i] += nitems * weights[i];
}

//-------------------------------------------------------------

/**
 * Calculates unit's weight and movement capabilities
 * Takes into account items, their weights, and special items like wagons
 */
void CUnit::CalcWeightsAndMovement()
{
    int           idx;
    const char* propname;
    int* weights;
    const char** movenames = NULL;
    int           movecount;
    EValueType    type;
    const void* n;
    CStr          sValue;
    int           i;

    memset(Weight, 0, sizeof(Weight));
    SetProperty(PRP_MOVEMENT, eCharPtr, "", eNormal);

    if (!gpDataHelper)
        return;

    // First pass: calculate base weights from all items
    idx = 0;
    propname = GetPropertyName(idx);
    while (propname)
    {
        if (GetProperty(propname, type, n, eNormal) &&
            (eLong == type) &&
            gpDataHelper->GetItemWeights(propname, weights, movenames, movecount))
        {
            AddWeight(static_cast<long>(reinterpret_cast<intptr_t>(n)), weights, movenames, movecount);
        }

        propname = GetPropertyName(++idx);
    }

    if (!movenames)
        return;

    // Second pass: adjust for wagons if needed
    i = 3; // Start with fly mode
    if (Weight[0]) // If unit has weight
        while ((i > 0) && (Weight[i] < Weight[0]))
            i--;
            
    if (0 == i) // Cannot move at all
    {
        // Check if wagons can help
        int nHorses = 0, nWagons = 0;
        idx = 0;
        propname = GetPropertyName(idx);
        while (propname)
        {
            if (GetProperty(propname, type, n, eNormal) &&
                (eLong == type))
            {
                if (gpDataHelper->IsWagonPuller(propname))
                    nHorses += static_cast<long>(reinterpret_cast<intptr_t>(n));
                else if (gpDataHelper->IsWagon(propname))
                    nWagons += static_cast<long>(reinterpret_cast<intptr_t>(n));
            }
            propname = GetPropertyName(++idx);
        }
        int nAdjust = std::min(nHorses, nWagons);
        if (nAdjust > 0)
            Weight[1] += nAdjust * gpDataHelper->WagonCapacity();
    }

    // Determine movement mode name
    sValue.Empty();
    if (Weight[0])
    {
        i = 3;
        while ((i > 0) && (Weight[i] < Weight[0]))
            i--;
    }
    else
        i = 0;
    sValue = movenames[i];

    if (Weight[4] >= Weight[0]) // Can swim
        sValue << ',' << movenames[4];

    SetProperty(PRP_MOVEMENT, eCharPtr, sValue.GetData(), eNormal);
}

//-------------------------------------------------------------

/**
 * Checks if unit is overloaded for its movement mode
 * @param sErr Output error message if overloaded
 */
void CUnit::CheckWeight(CStr& sErr)
{
    int           i;
    const char** movenames;
    int           broken = 0;

    sErr.Empty();
    gpDataHelper->GetMoveNames(movenames);

    // Find first movement mode where capacity < weight
    for (i = 1; i < MOVE_MODE_MAX; i++)
        if (Weight[0] > Weight[i] && Weight[i] > 0)
        {
            broken = i;
            break;
        }

    // Check if higher movement mode works (e.g., can fly but not walk)
    if (broken && broken < MOVE_MODE_MAX - 2) // if flying broken, that's final
    {
        for (i = MOVE_MODE_MAX - 2; i > broken; i--)
            if (Weight[0] <= Weight[i] && Weight[i] > 0)
            {
                broken = 0;
                break;
            }
    }

    if (broken && broken == MOVE_MODE_MAX - 4) // Walk mode broken
        sErr << " - Could " << movenames[broken] << " but is overloaded.";
}

//-------------------------------------------------------------

/**
 * Gets unit's best skill (highest days studied)
 * @param name Output skill name
 * @param days Output days studied
 */
void CUnit::GetBestSkill(wxString& name, long& days)
{
    int idx = 0;
    EValueType type;
    const wxString Suffix = wxString::FromUTF8(PRP_SKILL_DAYS_POSTFIX);
    wxString skillName;
    const void* valuePtr = nullptr;
    name = wxEmptyString;
    days = 0;

    const char* propname_days = GetPropertyName(idx);
    while (propname_days)
    {
        skillName = wxString::FromUTF8(propname_days);
        if (skillName.EndsWith(Suffix))
        {
            if (GetProperty(skillName.ToUTF8(), type, valuePtr, eNormal))
            {
                if (type == eLong)
                {
                    long daysCurrent = static_cast<long>(reinterpret_cast<intptr_t>(valuePtr));
                    if (daysCurrent > days)
                    {
                        days = daysCurrent;
                        name = skillName;
                    }
                }
            }
        }
        propname_days = GetPropertyName(++idx);
    }
    if (days > 0 && name.Length() > Suffix.Length())
        name.Truncate(name.Length() - Suffix.Length());

    name.MakeLower();
}

//-------------------------------------------------------------

/**
 * Gets unit property value
 * Handles built-in properties and dynamic flags
 */
BOOL CUnit::GetProperty(const char* name,
    EValueType& type,
    const void*& value,
    EPropertyType  proptype
)
{
    BOOL Ok = TRUE;

    name = ResolveAlias(name);

    if (0 == stricmp(name, PRP_ID))
    {
        type = eLong;
        value = reinterpret_cast<void*>(static_cast<uintptr_t>(Id));
        return TRUE;
    }

    // Generate flag strings if they've changed
    if (FlagsLast != Flags &&
        (0 == stricmp(name, PRP_FLAGS_STANDARD) ||
            0 == stricmp(name, PRP_FLAGS_CUSTOM) ||
            0 == stricmp(name, PRP_FLAGS_CUSTOM_ABBR)
            )
        )
    {
        CStr sValue, sValueAbbr, sKey;
        int  i, x;

        // Generate standard flags string (single characters)
        if (Flags & UNIT_FLAG_PILLAGING)  sValue << "в‚¬";
        if (Flags & UNIT_FLAG_TAXING)  sValue << '$';
        if (Flags & UNIT_FLAG_PRODUCING)  sValue << 'P';
        if (Flags & UNIT_FLAG_GUARDING)  sValue << 'g';
        if (Flags & UNIT_FLAG_AVOIDING)  sValue << 'a';
        if (Flags & UNIT_FLAG_BEHIND)  sValue << 'b';
        if (Flags & UNIT_FLAG_REVEALING_UNIT)  sValue << 'r';
        else if (Flags & UNIT_FLAG_REVEALING_FACTION)  sValue << 'r';
        if (Flags & UNIT_FLAG_HOLDING)  sValue << 'h';
        if (Flags & UNIT_FLAG_RECEIVING_NO_AID)  sValue << 'i';
        if (Flags & UNIT_FLAG_CONSUMING_UNIT)  sValue << 'c';
        else if (Flags & UNIT_FLAG_CONSUMING_FACTION)  sValue << 'c';
        if (Flags & UNIT_FLAG_NO_CROSS_WATER)  sValue << 'x';
        if (Flags & UNIT_FLAG_SPOILS)  sValue << 's';
        if (Flags & UNIT_FLAG_SHARING)  sValue << 'z';

        type = eCharPtr;
        SetProperty(PRP_FLAGS_STANDARD, type, sValue.GetData(), eNormal);

        // Generate custom flags strings
        LoadCustomFlagNames();
        sValue.Empty();
        x = 1;
        for (i = 0; i < UNIT_CUSTOM_FLAG_COUNT; i++)
        {
            if (Flags & x)
            {
                if (!sValue.IsEmpty())
                    sValue << ',';
                sValue << GetCustomFlagName(i);
                sValueAbbr << (long)(i + 1);
            }
            x <<= 1;
        }
        SetProperty(PRP_FLAGS_CUSTOM, type, sValue.GetData(), eNormal);
        SetProperty(PRP_FLAGS_CUSTOM_ABBR, type, sValueAbbr.GetData(), eNormal);

        FlagsLast = Flags;
    }

    // Try base class properties first, then unit-specific ones
    if (!CBaseObject::GetProperty(name, type, value, proptype))
    {
        if (0 == stricmp(name, PRP_COMMENTS))
        {
            type = eCharPtr;
            value = Comments.GetData();
        }
        else if (0 == stricmp(name, PRP_ORDERS))
        {
            // Decorate orders for display (replace newlines with " | ")
            const char* src = Orders.GetData();
            char* dest;
            int          destlen = 0;

            OrdersDecorated.Empty();
            dest = OrdersDecorated.AllocExtraBuf(Orders.GetLength() * 3 + 1);

            while (src && *src)
            {
                if (*src != '\r' && *src != '\n')
                {
                    destlen++;
                    *dest = *src;
                    dest++;
                }
                else if ('\n' == *src)
                {
                    destlen += 3;
                    *dest = ' ';     dest++;
                    *dest = '|';     dest++;
                    *dest = ' ';     dest++;
                }
                src++;
            }
            OrdersDecorated.UseExtraBuf(destlen);

            type = eCharPtr;
            value = OrdersDecorated.GetData();
        }
        else if (0 == stricmp(name, PRP_FACTION_ID))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(FactionId));
        }
        else if (0 == stricmp(name, PRP_FACTION))
        {
            type = eCharPtr;
            if (pFaction)
                value = pFaction->Name.GetData();
            else
                value = "";
        }
        else if (0 == stricmp(name, PRP_LAND_ID))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(LandId));
        }
        else if (0 == stricmp(name, PRP_WEIGHT))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(Weight[0]));
        }
        else if (0 == stricmp(name, PRP_WEIGHT_WALK))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(Weight[1] - Weight[0]));
        }
        else if (0 == stricmp(name, PRP_WEIGHT_RIDE))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(Weight[2] - Weight[0]));
        }
        else if (0 == stricmp(name, PRP_WEIGHT_FLY))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(Weight[3] - Weight[0]));
        }
        else if (0 == stricmp(name, PRP_WEIGHT_SWIM))
        {
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(Weight[4] - Weight[0]));
        }
        else if (0 == stricmp(name, PRP_BEST_SKILL))
        {
            wxString S;
            long days;
            GetBestSkill(S, days);
            type = eCharPtr;
            const char* s = S.ToUTF8().data();

            // Cache strings to avoid temporary lifetime issues
            static char* arr[200];
            static int arrMax = 0;
            bool found = false;

            for (int i = 0; i < arrMax; ++i)
            {
                if (0 == strcmp(arr[i], s))
                {
                    s = arr[i];
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                arr[arrMax] = strdup(s);
                s = arr[arrMax++];
            }

            value = (void*)(s);
        }
        else if (0 == stricmp(name, PRP_BEST_SKILL_DAYS))
        {
            wxString S;
            long days;
            GetBestSkill(S, days);
            type = eLong;
            value = reinterpret_cast<void*>(static_cast<uintptr_t>(days));
        }
        else if (0 == stricmp(name, PRP_TEACHING))
        {
            type = eLong;
            if (StudyingSkill.IsEmpty())
                value = reinterpret_cast<void*>(static_cast<uintptr_t>((long)ceil(Teaching)));
            else
                value = reinterpret_cast<void*>(static_cast<uintptr_t>((long)floor(Teaching)));

            if (Teaching <= 0)
                Ok = FALSE;
        }
        else
            Ok = FALSE;
    }

    return Ok;
}

//-------------------------------------------------------------

/**
 * Loads custom flag names from configuration
 */
void CUnit::LoadCustomFlagNames()
{
    if (!m_CustomFlagNamesLoaded)
    {
        int  i;
        CStr sKey;

        for (i = 0; i < UNIT_CUSTOM_FLAG_COUNT; i++)
        {
            sKey.Empty();
            sKey << (long)i;
            m_CustomFlagNames[i] = gpApp->GetConfig(SZ_SECT_UNIT_FLAG_NAMES, sKey.GetData());
        }
        m_CustomFlagNamesLoaded = TRUE;
    }
}

//-------------------------------------------------------------

/**
 * Resets custom flag names (forces reload from config)
 */
void CUnit::ResetCustomFlagNames()
{
    int i;

    m_CustomFlagNamesLoaded = FALSE;
    for (i = 0; i < UNIT_CUSTOM_FLAG_COUNT; i++)
        m_CustomFlagNames[i].Empty();
}

//-------------------------------------------------------------

/**
 * Gets custom flag name by index
 */
const char* CUnit::GetCustomFlagName(int no)
{
    if (m_CustomFlagNamesLoaded && no >= 0 && no < UNIT_CUSTOM_FLAG_COUNT)
        return m_CustomFlagNames[no].GetData();
    else
        return NULL;
}

//-------------------------------------------------------------

void CUnit::DebugPrint(CStr& sDest)
{
    CBaseObject::DebugPrint(sDest);

    sDest << "FactionId = " << FactionId << "\n"
        << "LandId    = " << LandId << "\n";
}

//-------------------------------------------------------------

/**
 * Gets skill level by code
 */
int CUnit::GetSkillLevel(const char* skill)
{
    if (!skill || !*skill)
        return 0;

    EValueType type;
    const void* value = NULL;

    if (!GetProperty(PRP_SKILLS, type, (const void*&)value, eNormal))
        return 0;

    if (type != eCharPtr)
        return 0;

    const char* skillsStr = (const char*)value;
    if (!skillsStr || !*skillsStr)
        return 0;

    // Search for "[SKILL]" pattern
    CStr searchPattern;
    searchPattern << "[" << skill << "]";

    const char* p = strstr(skillsStr, searchPattern.GetData());
    if (!p)
        return 0;

    // Skip "[SKILL]"
    p += searchPattern.GetLength();

    // Skip spaces
    while (*p && (*p == ' ' || *p == '\t'))
        p++;

    // Read level number
    int level = 0;
    while (*p && *p >= '0' && *p <= '9')
    {
        level = level * 10 + (*p - '0');
        p++;
    }

    return level;
}

//-------------------------------------------------------------

/**
 * Gets quartermaster (QUAM) skill level
 */
int CUnit::GetQuamLevel()
{
    return GetSkillLevel("QUAM");
}

//-------------------------------------------------------------

/**
 * Checks if unit is a quartermaster (has QUAM skill)
 */
bool CUnit::IsQuartermaster()
{
    return GetQuamLevel() >= 1;
}

//-------------------------------------------------------------

/**
 * Resets end turn description to initial state
 */
void CUnit::ResetEndTurnDescription()
{
    m_EndTurnDescription.Empty();
    m_EndTurnDescription = Description;
}

//-------------------------------------------------------------
// End Turn Description Getters
//-------------------------------------------------------------

wxString CUnit::GetEndTurnUnitName()
{
    wxString unitName;
    if (m_EndTurnDescription.IsEmpty())
        return unitName;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    if (firstLine.StartsWith("* ") || firstLine.StartsWith("- "))
    {
        firstLine = firstLine.Mid(2);
    }

    int openParen = firstLine.Find('(');
    if (openParen != wxNOT_FOUND)
    {
        unitName = firstLine.Left(openParen).Trim(false).Trim(true);
    }

    return unitName;
}

//-------------------------------------------------------------

long CUnit::GetEndTurnUnitId()
{
    long unitId = 0;
    if (m_EndTurnDescription.IsEmpty())
        return unitId;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    if (firstLine.StartsWith("* ") || firstLine.StartsWith("- "))
    {
        firstLine = firstLine.Mid(2);
    }

    int openParen = firstLine.Find('(');
    int closeParen = firstLine.Find(')');

    if (openParen != wxNOT_FOUND && closeParen != wxNOT_FOUND && closeParen > openParen)
    {
        wxString idStr = firstLine.SubString(openParen + 1, closeParen - 1);
        idStr.ToLong(&unitId);
    }

    return unitId;
}

//-------------------------------------------------------------

wxString CUnit::GetEndTurnFactionName()
{
    wxString factionName;
    if (m_EndTurnDescription.IsEmpty())
        return factionName;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    int lastComma = firstLine.Find(',');
    if (lastComma == wxNOT_FOUND)
        return factionName;

    wxString afterUnit = firstLine.Mid(lastComma + 1).Trim(false);

    int openParen = afterUnit.Find('(');
    if (openParen != wxNOT_FOUND)
    {
        factionName = afterUnit.Left(openParen).Trim(false).Trim(true);
    }

    return factionName;
}

//-------------------------------------------------------------

long CUnit::GetEndTurnFactionId()
{
    long factionId = 0;
    if (m_EndTurnDescription.IsEmpty())
        return factionId;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    int lastComma = firstLine.Find(',');
    if (lastComma == wxNOT_FOUND)
        return factionId;

    wxString afterUnit = firstLine.Mid(lastComma + 1).Trim(false);

    int openParen = afterUnit.Find('(');
    int closeParen = afterUnit.Find(')');

    if (openParen != wxNOT_FOUND && closeParen != wxNOT_FOUND && closeParen > openParen)
    {
        wxString idStr = afterUnit.SubString(openParen + 1, closeParen - 1);
        idStr.ToLong(&factionId);
    }

    return factionId;
}

//-------------------------------------------------------------

BOOL CUnit::IsEndTurnOwnUnit()
{
    if (m_EndTurnDescription.IsEmpty())
        return FALSE;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());
    return desc.StartsWith("* ");
}

//-------------------------------------------------------------

long CUnit::GetEndTurnItemAmount(const wxString& itemCode)
{
    if (m_EndTurnDescription.IsEmpty())
        return 0;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    // Extract items part (between flags and first period)
    int lastCommaBeforePeriod = firstLine.Find('.');
    if (lastCommaBeforePeriod == wxNOT_FOUND)
        return 0;

    wxString beforePeriod = firstLine.Left(lastCommaBeforePeriod);

    // Find items start (after second comma)
    int commaCount = 0;
    int itemsStart = 0;
    for (size_t i = 0; i < beforePeriod.Length(); i++)
    {
        if (beforePeriod[i] == ',')
        {
            commaCount++;
            if (commaCount == 2)
            {
                itemsStart = i + 1;
                break;
            }
        }
    }

    if (itemsStart == 0)
        return 0;

    wxString itemsStr = beforePeriod.Mid(itemsStart).Trim(false);

    wxStringTokenizer tkz(itemsStr, ",", wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens())
    {
        wxString item = tkz.GetNextToken().Trim(false).Trim(true);
        if (!item.IsEmpty())
        {
            long amount = 1;
            wxString code;

            wxString firstWord = item.BeforeFirst(' ');
            long tmp;
            if (firstWord.ToLong(&tmp))
            {
                amount = tmp;
                item = item.AfterFirst(' ').Trim(false);
            }

            int openBracket = item.Find('[');
            int closeBracket = item.Find(']');

            if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND)
            {
                code = item.SubString(openBracket + 1, closeBracket - 1);
                if (code == itemCode)
                    return amount;
            }
        }
    }

    return 0;
}

//-------------------------------------------------------------

long CUnit::GetEndTurnItemAmount(const char* itemCode)
{
    return GetEndTurnItemAmount(wxString::FromUTF8(itemCode));
}

//-------------------------------------------------------------

long CUnit::GetEndTurnSkillDays(const wxString& skillCode)
{
    if (m_EndTurnDescription.IsEmpty())
        return 0;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
        return 0;

    wxString afterSkills = desc.Mid(skillsStart + 7).BeforeFirst(';').Trim(false);

    wxStringTokenizer tkz(afterSkills, ",", wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens())
    {
        wxString skill = tkz.GetNextToken().Trim(false).Trim(true);
        if (!skill.IsEmpty())
        {
            int openBracket = skill.Find('[');
            int closeBracket = skill.Find(']');

            if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND)
            {
                wxString code = skill.SubString(openBracket + 1, closeBracket - 1);

                if (code == skillCode)
                {
                    wxString afterCode = skill.Mid(closeBracket + 1).Trim(false);
                    int openParen = afterCode.Find('(');
                    int closeParen = afterCode.Find(')');
                    if (openParen != wxNOT_FOUND && closeParen != wxNOT_FOUND)
                    {
                        wxString daysStr = afterCode.SubString(openParen + 1, closeParen - 1);
                        long days = 0;
                        daysStr.ToLong(&days);
                        return days;
                    }
                }
            }
        }
    }

    return 0;
}

//-------------------------------------------------------------

long CUnit::GetEndTurnSkillLevel(const wxString& skillCode)
{
    if (m_EndTurnDescription.IsEmpty())
        return 0;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
        return 0;

    wxString afterSkills = desc.Mid(skillsStart + 7).BeforeFirst(';').Trim(false);

    wxStringTokenizer tkz(afterSkills, ",", wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens())
    {
        wxString skill = tkz.GetNextToken().Trim(false).Trim(true);
        if (!skill.IsEmpty())
        {
            int openBracket = skill.Find('[');
            int closeBracket = skill.Find(']');

            if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND)
            {
                wxString code = skill.SubString(openBracket + 1, closeBracket - 1);

                if (code == skillCode)
                {
                    wxString afterCode = skill.Mid(closeBracket + 1).Trim(false);
                    wxString levelStr = afterCode.BeforeFirst(' ');
                    long level = 0;
                    levelStr.ToLong(&level);
                    return level;
                }
            }
        }
    }

    return 0;
}

//-------------------------------------------------------------

wxString CUnit::GetEndTurnUserDescription()
{
    wxString userDesc;
    if (m_EndTurnDescription.IsEmpty())
        return userDesc;

    wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());

    int semicolonPos = desc.Find(';');
    if (semicolonPos == wxNOT_FOUND)
        return userDesc;

    userDesc = desc.Mid(semicolonPos + 1).Trim(false);
    return userDesc;
}

//-------------------------------------------------------------
// End Turn Description Setters (work with any CStr*)
//-------------------------------------------------------------

CStr* CUnit::SetEndTurnUnitName(CStr* pDescr, const wxString& unitName)
{
    if (!pDescr || pDescr->IsEmpty())
        return pDescr;

    long unitId = GetEndTurnUnitId(); // From this unit, not from pDescr

    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString firstLine = desc.BeforeFirst('\n');
    wxString prefix = firstLine.StartsWith("* ") ? "* " : "- ";

    int closeParen = firstLine.Find(')');
    wxString rest = firstLine.Mid(closeParen + 1);

    wxString newFirstLine = prefix + unitName + " (" + wxString::Format("%ld", unitId) + ")" + rest;

    wxString remaining = desc.AfterFirst('\n');
    pDescr->Empty();
    pDescr->AddStr(newFirstLine.ToUTF8(), newFirstLine.Length());
    if (!remaining.IsEmpty())
    {
        pDescr->AddStr("\n", 1);
        pDescr->AddStr(remaining.ToUTF8(), remaining.Length());
    }

    return pDescr;
}

//-------------------------------------------------------------

CStr* CUnit::SetEndTurnUnitId(CStr* pDescr, long unitId)
{
    if (!pDescr || pDescr->IsEmpty())
        return pDescr;

    wxString unitName = GetEndTurnUnitName(); // From this unit

    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString firstLine = desc.BeforeFirst('\n');
    wxString prefix = firstLine.StartsWith("* ") ? "* " : "- ";

    int closeParen = firstLine.Find(')');
    wxString rest = firstLine.Mid(closeParen + 1);

    wxString newFirstLine = prefix + unitName + " (" + wxString::Format("%ld", unitId) + ")" + rest;

    wxString remaining = desc.AfterFirst('\n');
    pDescr->Empty();
    pDescr->AddStr(newFirstLine.ToUTF8(), newFirstLine.Length());
    if (!remaining.IsEmpty())
    {
        pDescr->AddStr("\n", 1);
        pDescr->AddStr(remaining.ToUTF8(), remaining.Length());
    }

    return pDescr;
}

//-------------------------------------------------------------

CStr* CUnit::SetEndTurnFactionName(CStr* pDescr, const wxString& factionName)
{
    if (!pDescr || pDescr->IsEmpty())
        return pDescr;

    long factionId = GetEndTurnFactionId(); // From this unit

    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    int unitCloseParen = firstLine.Find(')');
    if (unitCloseParen == wxNOT_FOUND)
        return pDescr;

    wxString beforeFaction = firstLine.Left(unitCloseParen + 1);

    int factionCloseParen = firstLine.Find(')', unitCloseParen + 1);
    if (factionCloseParen == wxNOT_FOUND)
        return pDescr;

    wxString afterFaction = firstLine.Mid(factionCloseParen + 1);

    wxString newFirstLine = beforeFaction + ", " + factionName + " (" + wxString::Format("%ld", factionId) + ")" + afterFaction;

    wxString remaining = desc.AfterFirst('\n');
    pDescr->Empty();
    pDescr->AddStr(newFirstLine.ToUTF8(), newFirstLine.Length());
    if (!remaining.IsEmpty())
    {
        pDescr->AddStr("\n", 1);
        pDescr->AddStr(remaining.ToUTF8(), remaining.Length());
    }

    return pDescr;
}

//-------------------------------------------------------------

CStr* CUnit::SetEndTurnFactionId(CStr* pDescr, long factionId)
{
    if (!pDescr || pDescr->IsEmpty())
        return pDescr;

    wxString factionName = GetEndTurnFactionName(); // From this unit

    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    int unitCloseParen = firstLine.Find(')');
    if (unitCloseParen == wxNOT_FOUND)
        return pDescr;

    wxString beforeFaction = firstLine.Left(unitCloseParen + 1);

    int factionCloseParen = firstLine.Find(')', unitCloseParen + 1);
    if (factionCloseParen == wxNOT_FOUND)
        return pDescr;

    wxString afterFaction = firstLine.Mid(factionCloseParen + 1);

    wxString newFirstLine = beforeFaction + ", " + factionName + " (" + wxString::Format("%ld", factionId) + ")" + afterFaction;

    wxString remaining = desc.AfterFirst('\n');
    pDescr->Empty();
    pDescr->AddStr(newFirstLine.ToUTF8(), newFirstLine.Length());
    if (!remaining.IsEmpty())
    {
        pDescr->AddStr("\n", 1);
        pDescr->AddStr(remaining.ToUTF8(), remaining.Length());
    }

    return pDescr;
}

//-------------------------------------------------------------

CStr* CUnit::SetEndTurnUserDescription(CStr* pDescr, const wxString& userDescription)
{
    if (!pDescr)
        return pDescr;

    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int semicolonPos = desc.Find(';');
    if (semicolonPos != wxNOT_FOUND)
    {
        wxString beforeSemicolon = desc.Left(semicolonPos);
        pDescr->Empty();
        pDescr->AddStr((beforeSemicolon + "; " + userDescription).ToUTF8(),
            beforeSemicolon.Length() + 2 + userDescription.Length());
    }
    else
    {
        pDescr->AddStr("; ", 2);
        pDescr->AddStr(userDescription.ToUTF8(), userDescription.Length());
    }

    return pDescr;
}

//-------------------------------------------------------------
// Static Item Manipulation Methods
//-------------------------------------------------------------

/**
 * Gets an item from description by its code
 */
TUnitItem CUnit::GetItemByCode(CStr* pDescr, const wxString& itemCode)
{
    TUnitItem item;

    if (!pDescr || pDescr->IsEmpty() || itemCode.IsEmpty())
        return item;

    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    int lastCommaBeforePeriod = firstLine.Find('.');
    if (lastCommaBeforePeriod == wxNOT_FOUND)
        return item;

    wxString beforePeriod = firstLine.Left(lastCommaBeforePeriod);

    int commaCount = 0;
    int itemsStart = 0;
    for (size_t i = 0; i < beforePeriod.Length(); i++)
    {
        if (beforePeriod[i] == ',')
        {
            commaCount++;
            if (commaCount == 2)
            {
                itemsStart = i + 1;
                break;
            }
        }
    }

    if (itemsStart == 0)
        return item;

    wxString itemsStr = beforePeriod.Mid(itemsStart).Trim(false);

    wxStringTokenizer tkz(itemsStr, ",", wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens())
    {
        wxString itemStr = tkz.GetNextToken().Trim(false).Trim(true);
        if (!itemStr.IsEmpty())
        {
            long amount = 1;
            wxString alias, code;

            wxString firstWord = itemStr.BeforeFirst(' ');
            long tmp;
            if (firstWord.ToLong(&tmp))
            {
                amount = tmp;
                itemStr = itemStr.AfterFirst(' ').Trim(false);
            }

            int openBracket = itemStr.Find('[');
            int closeBracket = itemStr.Find(']');

            if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND)
            {
                alias = itemStr.Left(openBracket).Trim(true);
                code = itemStr.SubString(openBracket + 1, closeBracket - 1);

                if (code == itemCode)
                {
                    item.itemAlias = alias.ToUTF8().data();
                    item.itemCode = code.ToUTF8().data();
                    item.itemCount = amount;
                    return item;
                }
            }
        }
    }

    return item;
}

//-------------------------------------------------------------

/**
 * Gets all items from a description
 */
std::vector<TUnitItem> CUnit::GetItems(CStr* pDescr)
{
    std::vector<TUnitItem> items;

    if (!pDescr || pDescr->IsEmpty())
        return items;

    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    int periodPos = firstLine.Find(". Weight");
    if (periodPos == wxNOT_FOUND)
        periodPos = firstLine.Find('.');

    if (periodPos == wxNOT_FOUND)
        return items;

    wxString beforePeriod = firstLine.Left(periodPos);

    int commaCount = 0;
    int itemsStart = 0;
    for (int i = 0; i < (int)beforePeriod.Length(); i++)
    {
        if (beforePeriod[i] == ',')
        {
            commaCount++;
            if (commaCount == 2)
            {
                itemsStart = i + 1;
                break;
            }
        }
    }

    if (itemsStart == 0)
    {
        int firstComma = beforePeriod.Find(',');
        if (firstComma != wxNOT_FOUND)
            itemsStart = firstComma + 1;
        else
            itemsStart = 0;
    }

    wxString itemsStr = beforePeriod.Mid(itemsStart).Trim(false);

    wxStringTokenizer tkz(itemsStr, ",");
    while (tkz.HasMoreTokens())
    {
        wxString itemToken = tkz.GetNextToken().Trim(false).Trim(true);
        if (itemToken.IsEmpty())
            continue;

        long amount = 1;
        wxString alias, code;

        wxString firstWord = itemToken.BeforeFirst(' ');
        long tmp;
        if (firstWord.ToLong(&tmp))
        {
            amount = tmp;
            itemToken = itemToken.AfterFirst(' ').Trim(false);
        }

        int openBracket = itemToken.Find('[');
        int closeBracket = itemToken.Find(']');

        if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND && closeBracket > openBracket)
        {
            alias = itemToken.Left(openBracket).Trim(true);
            code = itemToken.SubString(openBracket + 1, closeBracket - 1).Upper();

            TUnitItem item;
            item.itemAlias = alias.ToUTF8().data();
            item.itemCode = code.ToUTF8().data();
            item.itemCount = amount;
            items.push_back(item);
        }
    }

    return items;
}

//-------------------------------------------------------------

/**
 * Converts a vector of items to a formatted string
 */
wxString CUnit::ItemsToStr(const std::vector<TUnitItem>& items)
{
    if (items.empty())
        return wxEmptyString;

    wxString result;
    bool first = true;

    for (const auto& item : items)
    {
        if (!first)
            result << ", ";
        else
            first = false;

        wxString displayName = wxString::FromUTF8(item.itemAlias.GetSafeCStr());
        if (displayName.IsEmpty())
            displayName = wxString::FromUTF8(item.itemCode.GetSafeCStr());

        wxString code = wxString::FromUTF8(item.itemCode.GetSafeCStr());

        if (item.itemCount > 1)
            result << item.itemCount << " " << displayName << " [" << code << "]";
        else
            result << displayName << " [" << code << "]";
    }

    return result;
}

//-------------------------------------------------------------

/**
 * Filters items by group
 */
std::vector<TUnitItem> CUnit::GetItemsByGroup(std::vector<TUnitItem>& a_ItemList, CStr* a_Group)
{
    std::vector<TUnitItem> filteredItems;

    if (a_ItemList.empty() || a_Group->IsEmpty() || !gpDataHelper)
        return filteredItems;

    CStr groupList = gpDataHelper->GetConfString("UNIT_PROPERTY_GROUPS", a_Group->GetSafeCStr());

    for (const auto& item : a_ItemList)
    {
        if (groupList.FindSubStr(item.itemCode.GetSafeCStr()) > 0)
        {
            filteredItems.push_back(item);
        }
    }

    return filteredItems;
}

//-------------------------------------------------------------

/**
 * Gets count of items in a specific group
 */
long CUnit::GetItemsCountByGroup(std::vector<TUnitItem>& a_ItemList, CStr* a_Group)
{
    long itemsCount = 0;

    if (a_ItemList.empty() || a_Group->IsEmpty() || !gpDataHelper)
        return itemsCount;

    CStr groupList = gpDataHelper->GetConfString("UNIT_PROPERTY_GROUPS", a_Group->GetSafeCStr());

    for (const auto& item : a_ItemList)
    {
        if (groupList.FindSubStr(item.itemCode.GetSafeCStr()) > 0)
        {
            itemsCount += item.itemCount;
        }
    }

    return itemsCount;
}

//-------------------------------------------------------------

/**
 * Gets total men count from description
 */
long CUnit::GetMenCountByDescr(CStr* pDescr)
{
    if (!pDescr || pDescr->IsEmpty())
        return 0;

    std::vector<TUnitItem> items = GetItems(pDescr);
    CStr groupName = "men";

    return GetItemsCountByGroup(items, &groupName);
}

//-------------------------------------------------------------
// Weight Calculation Methods
//-------------------------------------------------------------

long CUnit::GetWeight(std::vector<TUnitItem> a_Items)
{
    long totalWeight = 0;

    if (a_Items.empty() || !gpDataHelper)
        return totalWeight;

    for (const auto& item : a_Items)
    {
        if (item.itemCount <= 0)
            continue;

        int* weights = nullptr;
        const char** movenames = nullptr;
        int movecount = 0;

        if (gpDataHelper->GetItemWeights(item.itemCode.GetSafeCStr(), weights, movenames, movecount))
        {
            if (weights && movecount > 0)
            {
                totalWeight += item.itemCount * weights[0];
            }
        }
    }

    return totalWeight;
}

//-------------------------------------------------------------

long CUnit::GetWalkCapacity(std::vector<TUnitItem> a_Items)
{
    long totalCapacity = 0;

    if (a_Items.empty() || !gpDataHelper)
        return totalCapacity;

    for (const auto& item : a_Items)
    {
        if (item.itemCount <= 0)
            continue;

        int* weights = nullptr;
        const char** movenames = nullptr;
        int movecount = 0;

        if (gpDataHelper->GetItemWeights(item.itemCode.GetSafeCStr(), weights, movenames, movecount))
        {
            if (weights && movecount > 1)
            {
                totalCapacity += item.itemCount * weights[1];
            }
        }
    }

    return totalCapacity;
}

//-------------------------------------------------------------

long CUnit::GetFlyCapacity(std::vector<TUnitItem> a_Items)
{
    long totalCapacity = 0;

    if (a_Items.empty() || !gpDataHelper)
        return totalCapacity;

    for (const auto& item : a_Items)
    {
        if (item.itemCount <= 0)
            continue;

        int* weights = nullptr;
        const char** movenames = nullptr;
        int movecount = 0;

        if (gpDataHelper->GetItemWeights(item.itemCode.GetSafeCStr(), weights, movenames, movecount))
        {
            if (weights && movecount > 3)
            {
                totalCapacity += item.itemCount * weights[3];
            }
        }
    }

    return totalCapacity;
}

//-------------------------------------------------------------

long CUnit::GetSwimCapacity(std::vector<TUnitItem> a_Items)
{
    long totalCapacity = 0;

    if (a_Items.empty() || !gpDataHelper)
        return totalCapacity;

    for (const auto& item : a_Items)
    {
        if (item.itemCount <= 0)
            continue;

        int* weights = nullptr;
        const char** movenames = nullptr;
        int movecount = 0;

        if (gpDataHelper->GetItemWeights(item.itemCode.GetSafeCStr(), weights, movenames, movecount))
        {
            if (weights && movecount > 4)
            {
                totalCapacity += item.itemCount * weights[4];
            }
        }
    }

    return totalCapacity;
}

//-------------------------------------------------------------

/**
 * Updates item quantity in description
 * @param pDescr Description to modify
 * @param itemCode Item code to update
 * @param itemCount Amount to add (positive or negative)
 * @return Modified description
 */
CStr* CUnit::AidItem(CStr* pDescr, const wxString& itemCode, long itemCount)
{
    if (!pDescr || pDescr->IsEmpty() || itemCode.IsEmpty() || itemCount == 0)
        return pDescr;

    wxString upperCode = itemCode.Upper();
    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString firstLine = desc.BeforeFirst('\n');

    int periodPos = firstLine.Find(". Weight");
    if (periodPos == wxNOT_FOUND)
        periodPos = firstLine.Find('.');

    if (periodPos == wxNOT_FOUND)
        return pDescr;

    wxString beforePeriod = firstLine.Left(periodPos);

    int commaCount = 0;
    int itemsStart = 0;
    for (int i = 0; i < (int)beforePeriod.Length(); i++)
    {
        if (beforePeriod[i] == ',')
        {
            commaCount++;
            if (commaCount == 2)
            {
                itemsStart = i + 1;
                break;
            }
        }
    }

    if (itemsStart == 0)
    {
        int firstComma = beforePeriod.Find(',');
        if (firstComma != wxNOT_FOUND)
            itemsStart = firstComma + 1;
        else
            itemsStart = 0;
    }

    wxString itemsStr = beforePeriod.Mid(itemsStart).Trim(false);
    std::vector<wxString> itemTokens;

    wxStringTokenizer tkz(itemsStr, ",");
    while (tkz.HasMoreTokens())
    {
        wxString token = tkz.GetNextToken().Trim(false).Trim(true);
        if (!token.IsEmpty())
            itemTokens.push_back(token);
    }

    bool itemFound = false;
    long totalChange = 0;

    for (size_t i = 0; i < itemTokens.size(); i++)
    {
        wxString token = itemTokens[i];

        int openBracket = token.Find('[');
        int closeBracket = token.Find(']');

        if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND && closeBracket > openBracket)
        {
            wxString code = token.SubString(openBracket + 1, closeBracket - 1).Upper();

            if (code == upperCode)
            {
                long currentAmount = 1;
                wxString beforeBracket = token.Left(openBracket).Trim(false);

                wxString firstWord = beforeBracket.BeforeFirst(' ');
                long tmp;
                if (firstWord.ToLong(&tmp))
                {
                    currentAmount = tmp;
                }

                long newAmount = currentAmount + itemCount;

                if (newAmount > 0)
                {
                    wxString itemName = beforeBracket.AfterFirst(' ').Trim(false);
                    if (itemName.IsEmpty())
                        itemName = token.Left(openBracket).Trim(false);

                    token = wxString::Format("%ld %s [%s]",
                        newAmount, itemName, upperCode);

                    itemTokens[i] = token;
                    itemFound = true;
                    totalChange = newAmount - currentAmount;
                }
                else
                {
                    itemTokens.erase(itemTokens.begin() + i);
                    itemFound = true;
                    totalChange = -currentAmount;
                }
                break;
            }
        }
    }

    if (!itemFound && itemCount > 0)
    {
        wxString itemAlias = wxString::FromUTF8(gpDataHelper->GetAliasByCode(upperCode.ToUTF8()));
        if (itemAlias.IsEmpty())
            itemAlias = upperCode;

        wxString newToken;
        if (itemCount > 1)
            newToken = wxString::Format("%ld %s [%s]", itemCount, itemAlias, upperCode);
        else
            newToken = wxString::Format("%s [%s]", itemAlias, upperCode);

        itemTokens.push_back(newToken);
        totalChange = itemCount;
    }

    wxString newItemsStr;
    for (size_t i = 0; i < itemTokens.size(); i++)
    {
        if (i > 0)
            newItemsStr << ", ";
        newItemsStr << itemTokens[i];
    }

    wxString beforeItems = firstLine.Left(itemsStart);
    wxString afterPeriod = desc.Mid(periodPos);

    wxString newFirstLine = beforeItems + " " + newItemsStr + afterPeriod;

    wxString remaining = desc.AfterFirst('\n');
    pDescr->Empty();
    pDescr->AddStr(newFirstLine.ToUTF8(), newFirstLine.Length());

    if (!remaining.IsEmpty())
    {
        pDescr->AddStr("\n", 1);
        pDescr->AddStr(remaining.ToUTF8(), remaining.Length());
    }

    return pDescr;
}

//-------------------------------------------------------------
// Skill Manipulation Methods
//-------------------------------------------------------------

/**
 * Gets a skill from description by its code
 */
TUnitSkill CUnit::GetSkillByCode(CStr* pDescr, const wxString& skillCode)
{
    TUnitSkill skill;

    if (!pDescr || pDescr->IsEmpty() || skillCode.IsEmpty())
        return skill;

    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
        return skill;

    wxString afterSkills = desc.Mid(skillsStart + 7).BeforeFirst(';').Trim(false);

    wxStringTokenizer tkz(afterSkills, ",", wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens())
    {
        wxString skillStr = tkz.GetNextToken().Trim(false).Trim(true);
        if (!skillStr.IsEmpty())
        {
            int openBracket = skillStr.Find('[');
            int closeBracket = skillStr.Find(']');

            if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND)
            {
                wxString alias = skillStr.Left(openBracket).Trim(true);
                wxString code = skillStr.SubString(openBracket + 1, closeBracket - 1);

                if (code == skillCode)
                {
                    wxString afterCode = skillStr.Mid(closeBracket + 1).Trim(false);
                    wxString levelStr = afterCode.BeforeFirst(' ');
                    long level = 0;
                    levelStr.ToLong(&level);

                    int openParen = afterCode.Find('(');
                    int closeParen = afterCode.Find(')');
                    long days = 0;
                    if (openParen != wxNOT_FOUND && closeParen != wxNOT_FOUND)
                    {
                        wxString daysStr = afterCode.SubString(openParen + 1, closeParen - 1);
                        daysStr.ToLong(&days);
                    }

                    skill.skillAlias = alias.ToUTF8().data();
                    skill.skillCode = code.ToUTF8().data();
                    skill.skillLevel = level;
                    skill.skillDays = days;
                    return skill;
                }
            }
        }
    }

    return skill;
}

//-------------------------------------------------------------

/**
 * Gets a skill from vector by its code
 */
TUnitSkill CUnit::GetSkillByCode(const std::vector<TUnitSkill>& skills, const wxString& skillCode)
{
    TUnitSkill result;

    if (skillCode.IsEmpty() || skills.empty())
        return result;

    std::string searchCode = std::string(skillCode.ToUTF8().data());
    std::transform(searchCode.begin(), searchCode.end(), searchCode.begin(), ::tolower);

    for (const auto& skill : skills)
    {
        const char* currentCode = skill.skillCode.GetSafeCStr();

        if (currentCode && *currentCode)
        {
            std::string currentCodeStr(currentCode);
            std::transform(currentCodeStr.begin(), currentCodeStr.end(), currentCodeStr.begin(), ::tolower);

            if (currentCodeStr == searchCode)
            {
                return skill;
            }
        }
    }

    return result;
}

//-------------------------------------------------------------

/**
 * Gets all skills from description
 */
std::vector<TUnitSkill> CUnit::GetAllSkills(CStr* pDescr)
{
    std::vector<TUnitSkill> skills;

    if (!pDescr || pDescr->IsEmpty())
        return skills;

    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
        return skills;

    wxString afterSkills = desc.Mid(skillsStart + 7).BeforeFirst(';').Trim(false);

    wxStringTokenizer tkz(afterSkills, ",", wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens())
    {
        wxString skillStr = tkz.GetNextToken().Trim(false).Trim(true);
        if (!skillStr.IsEmpty())
        {
            int openBracket = skillStr.Find('[');
            int closeBracket = skillStr.Find(']');

            if (openBracket != wxNOT_FOUND && closeBracket != wxNOT_FOUND)
            {
                wxString alias = skillStr.Left(openBracket).Trim(true);
                wxString code = skillStr.SubString(openBracket + 1, closeBracket - 1);
                wxString afterCode = skillStr.Mid(closeBracket + 1).Trim(false);
                wxString levelStr = afterCode.BeforeFirst(' ');
                long level = 0;
                levelStr.ToLong(&level);

                int openParen = afterCode.Find('(');
                int closeParen = afterCode.Find(')');
                long days = 0;
                if (openParen != wxNOT_FOUND && closeParen != wxNOT_FOUND)
                {
                    wxString daysStr = afterCode.SubString(openParen + 1, closeParen - 1);
                    daysStr.ToLong(&days);
                }

                TUnitSkill skill;
                skill.skillAlias = alias.ToUTF8().data();
                skill.skillCode = code.ToUTF8().data();
                skill.skillLevel = level;
                skill.skillDays = days;
                skills.push_back(skill);
            }
        }
    }

    return skills;
}

//-------------------------------------------------------------

/**
 * Converts skill vector to formatted string
 */
wxString CUnit::SkillsVectorToString(const std::vector<TUnitSkill>& skills)
{
    if (skills.empty())
        return wxEmptyString;

    wxString result = "Skills: ";
    bool firstSkill = true;

    for (const auto& skill : skills)
    {
        if (!firstSkill)
            result << ", ";
        else
            firstSkill = false;

        const char* aliasData = skill.skillAlias.GetSafeCStr();
        const char* codeData = skill.skillCode.GetSafeCStr();

        const char* displayName = aliasData;
        if (!displayName || !*displayName)
            displayName = codeData;

        if (displayName && *displayName)
        {
            result << displayName;
        }

        if (codeData && *codeData)
        {
            result << " [" << codeData << "]";
        }

        if (skill.skillLevel > 0)
        {
            result << " " << skill.skillLevel;
        }

        if (skill.skillDays > 0)
        {
            result << " (" << skill.skillDays << ")";
        }
        else if (skill.skillLevel == 0)
        {
            result << " 0";
        }
    }

    return result;
}

//-------------------------------------------------------------

/**
 * Calculates skill level from days
 */
long CUnit::GetSkillLevelByDays(long days)
{
    if (days < 30) return 0;
    if (days < 90) return 1;
    if (days < 180) return 2;
    if (days < 300) return 3;
    if (days < 450) return 4;
    return 5;
}

//-------------------------------------------------------------

/**
 * Updates skill days in description
 */
CStr* CUnit::AidSkillDays(CStr* pDescr, const wxString& skillCode, long daysChange)
{
    if (!pDescr || skillCode.IsEmpty() || daysChange == 0)
        return pDescr;

    wxString skillAlias = wxString::FromUTF8(gpDataHelper->GetAliasByCode(skillCode.ToUTF8()));
    if (skillAlias.IsEmpty())
        skillAlias = skillCode;

    TUnitSkill skill = GetSkillByCode(pDescr, skillCode);

    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
    {
        if (daysChange > 0)
            return AddNewSkill(pDescr, skillCode, daysChange);
        return pDescr;
    }

    int skillsEnd = desc.Find(';', skillsStart);
    if (skillsEnd == wxNOT_FOUND)
        skillsEnd = desc.Length();

    wxString beforeSkills = desc.Left(skillsStart + 7);
    wxString skillsStr = desc.Mid(skillsStart + 7, skillsEnd - skillsStart - 7).Trim(false);
    wxString afterSkills = desc.Mid(skillsEnd);

    bool isNoneOnly = (skillsStr.Find("none") != wxNOT_FOUND && skillsStr.Trim().Length() <= 5);

    if (skill.skillAlias.IsEmpty())
    {
        if (daysChange <= 0)
            return pDescr;

        long newDays = daysChange;
        long newLevel = GetSkillLevelByDays(newDays);

        wxString newSkillStr;
        newSkillStr << skillAlias << " [" << skillCode << "] "
            << newLevel << " (" << newDays << ")";

        if (isNoneOnly)
        {
            skillsStr = newSkillStr;
        }
        else if (skillsStr.IsEmpty())
        {
            skillsStr = newSkillStr;
        }
        else
        {
            skillsStr << ", " << newSkillStr;
        }
    }
    else
    {
        long newDays = skill.skillDays + daysChange;
        if (newDays < 0) newDays = 0;
        if (newDays > 450) newDays = 450;
        long newLevel = GetSkillLevelByDays(newDays);

        wxString oldPattern;
        oldPattern << wxString::FromUTF8(skill.skillAlias.GetData())
            << " [" << wxString::FromUTF8(skill.skillCode.GetData()) << "] "
            << skill.skillLevel << " (" << skill.skillDays << ")";

        wxString newPattern;
        if (newDays > 0)
        {
            newPattern << skillAlias
                << " [" << skillCode << "] "
                << newLevel << " (" << newDays << ")";
        }

        int pos = skillsStr.Find(oldPattern);
        if (pos != wxNOT_FOUND)
        {
            if (newDays > 0)
            {
                skillsStr.replace(pos, oldPattern.Length(), newPattern);
            }
            else
            {
                int commaBefore = skillsStr.rfind(',', pos - 1);
                int commaAfter = skillsStr.find(',', pos + oldPattern.Length());

                if (commaBefore != wxNOT_FOUND && commaAfter != wxNOT_FOUND)
                {
                    skillsStr.erase(commaBefore, commaAfter - commaBefore);
                }
                else if (commaBefore != wxNOT_FOUND)
                {
                    skillsStr.erase(commaBefore);
                }
                else if (commaAfter != wxNOT_FOUND)
                {
                    skillsStr.erase(pos, commaAfter - pos + 1);
                }
                else
                {
                    skillsStr = "none";
                }
            }
        }
        else
        {
            wxString newSkillStr;
            newSkillStr << skillAlias << " [" << skillCode << "] "
                << newLevel << " (" << newDays << ")";

            if (skillsStr.IsEmpty() || skillsStr == "none")
                skillsStr = newSkillStr;
            else
                skillsStr << ", " << newSkillStr;
        }
    }

    skillsStr.Trim();
    if (skillsStr.IsEmpty())
        skillsStr = "none";

    wxString newBeforeSkills = beforeSkills.Trim(false);
    if (!newBeforeSkills.EndsWith(" "))
        newBeforeSkills << " ";

    pDescr->Empty();
    pDescr->AddStr(newBeforeSkills.ToUTF8(), newBeforeSkills.Length());
    pDescr->AddStr(skillsStr.ToUTF8(), skillsStr.Length());
    if (!afterSkills.IsEmpty())
        pDescr->AddStr(afterSkills.ToUTF8(), afterSkills.Length());

    return pDescr;
}

//-------------------------------------------------------------

/**
 * Adds a new skill to description
 */
CStr* CUnit::AddNewSkill(CStr* pDescr, const wxString& skillCode, long days)
{
    if (!pDescr || skillCode.IsEmpty() || days <= 0)
        return pDescr;

    wxString skillAlias = wxString::FromUTF8(gpDataHelper->GetAliasByCode(skillCode.ToUTF8()));
    if (skillAlias.IsEmpty())
        skillAlias = skillCode;

    TUnitSkill existingSkill = GetSkillByCode(pDescr, skillCode);
    if (!existingSkill.skillAlias.IsEmpty())
    {
        return AidSkillDays(pDescr, skillCode, days);
    }

    long level = GetSkillLevelByDays(days);
    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
    {
        int weightPos = desc.Find("Weight:");
        if (weightPos == wxNOT_FOUND)
            return pDescr;

        wxString beforeWeight = desc.Left(weightPos);
        wxString weightAndAfter = desc.Mid(weightPos);

        wxString newSkillsStr;
        newSkillsStr << " Skills: " << skillAlias << " [" << skillCode << "] "
            << level << " (" << days << ");";

        pDescr->Empty();
        pDescr->AddStr(beforeWeight.ToUTF8(), beforeWeight.Length());
        pDescr->AddStr(newSkillsStr.ToUTF8(), newSkillsStr.Length());
        pDescr->AddStr(weightAndAfter.ToUTF8(), weightAndAfter.Length());

        return pDescr;
    }

    int skillsEnd = desc.Find(';', skillsStart);
    if (skillsEnd == wxNOT_FOUND)
        skillsEnd = desc.Length();

    wxString beforeSkills = desc.Left(skillsStart + 7);
    wxString skillsStr = desc.Mid(skillsStart + 7, skillsEnd - skillsStart - 7).Trim();
    wxString afterSkills = desc.Mid(skillsEnd);

    bool isNone = (skillsStr.Find("none") != wxNOT_FOUND && skillsStr.Length() <= 5);

    wxString newSkillStr;
    newSkillStr << skillAlias << " [" << skillCode << "] " << level << " (" << days << ")";

    if (isNone)
    {
        skillsStr = newSkillStr;
    }
    else if (skillsStr.IsEmpty())
    {
        skillsStr = newSkillStr;
    }
    else
    {
        skillsStr << ", " << newSkillStr;
    }

    wxString newBeforeSkills = beforeSkills.Trim(false);
    if (!newBeforeSkills.EndsWith(" "))
        newBeforeSkills << " ";

    pDescr->Empty();
    pDescr->AddStr(newBeforeSkills.ToUTF8(), newBeforeSkills.Length());
    pDescr->AddStr(skillsStr.ToUTF8(), skillsStr.Length());
    pDescr->AddStr(afterSkills.ToUTF8(), afterSkills.Length());

    return pDescr;
}

//-------------------------------------------------------------

/**
 * Removes a skill from description
 */
CStr* CUnit::RemoveSkill(CStr* pDescr, const wxString& skillCode)
{
    if (!pDescr || skillCode.IsEmpty())
        return pDescr;

    TUnitSkill skill = GetSkillByCode(pDescr, skillCode);
    if (skill.skillAlias.IsEmpty())
        return pDescr;

    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
        return pDescr;

    int skillsEnd = desc.Find(';', skillsStart);
    if (skillsEnd == wxNOT_FOUND)
        skillsEnd = desc.Length();

    wxString beforeSkills = desc.Left(skillsStart + 7);
    wxString skillsStr = desc.Mid(skillsStart + 7, skillsEnd - skillsStart - 7);
    wxString afterSkills = desc.Mid(skillsEnd);

    wxString skillPattern;
    if (skill.skillLevel > 0 && skill.skillDays > 0)
        skillPattern << wxString::FromUTF8(skill.skillAlias.GetData())
        << " [" << wxString::FromUTF8(skill.skillCode.GetData()) << "] "
        << skill.skillLevel << " (" << skill.skillDays << ")";
    else if (skill.skillLevel > 0)
        skillPattern << wxString::FromUTF8(skill.skillAlias.GetData())
        << " [" << wxString::FromUTF8(skill.skillCode.GetData()) << "] "
        << skill.skillLevel;
    else
        skillPattern << wxString::FromUTF8(skill.skillAlias.GetData())
        << " [" << wxString::FromUTF8(skill.skillCode.GetData()) << "]";

    int skillPos = skillsStr.Find(skillPattern);
    if (skillPos != wxNOT_FOUND)
    {
        int commaBefore = skillsStr.rfind(',', skillPos - 1);
        int commaAfter = skillsStr.find(',', skillPos + skillPattern.Length());

        if (commaBefore != wxNOT_FOUND && commaAfter != wxNOT_FOUND)
        {
            skillsStr.erase(commaBefore, commaAfter - commaBefore);
        }
        else if (commaBefore != wxNOT_FOUND)
        {
            skillsStr.erase(commaBefore);
        }
        else if (commaAfter != wxNOT_FOUND)
        {
            skillsStr.erase(skillPos, commaAfter - skillPos + 1);
        }
        else
        {
            skillsStr.Clear();
        }
    }

    if (skillsStr.Trim().IsEmpty())
    {
        pDescr->Empty();
        pDescr->AddStr(desc.Left(skillsStart).ToUTF8(), skillsStart);
        pDescr->AddStr(afterSkills.ToUTF8(), afterSkills.Length());
        return pDescr;
    }

    pDescr->Empty();
    pDescr->AddStr(beforeSkills.ToUTF8(), beforeSkills.Length());
    pDescr->AddStr(skillsStr.ToUTF8(), skillsStr.Length());
    pDescr->AddStr(afterSkills.ToUTF8(), afterSkills.Length());

    return pDescr;
}

//-------------------------------------------------------------

/**
 * Recalculates skills when men are transferred between units
 */
void CUnit::RecalcSkills(long menCount, CUnit* pFromUnit)
{
    if (m_EndTurnDescription.IsEmpty() || menCount <= 0)
        return;

    std::vector<TUnitSkill> receiverSkills = GetAllSkills(&m_EndTurnDescription);

    std::vector<TUnitSkill> giverSkills;
    if (pFromUnit && !pFromUnit->m_EndTurnDescription.IsEmpty())
    {
        giverSkills = GetAllSkills(&pFromUnit->m_EndTurnDescription);
    }

    long wasMenCount = GetMenCount() - menCount;
    if (wasMenCount < 0) wasMenCount = 0;

    long newTotalMenCount = wasMenCount + menCount;

    for (auto& receiverSkill : receiverSkills)
    {
        long giverSkillDays = 0;

        for (const auto& giverSkill : giverSkills)
        {
            std::string receiverCode = std::string(receiverSkill.skillCode.GetSafeCStr());
            std::string giverCode = std::string(giverSkill.skillCode.GetSafeCStr());

            std::transform(receiverCode.begin(), receiverCode.end(), receiverCode.begin(), ::tolower);
            std::transform(giverCode.begin(), giverCode.end(), giverCode.begin(), ::tolower);

            if (receiverCode == giverCode)
            {
                giverSkillDays = giverSkill.skillDays;
                break;
            }
        }

        receiverSkill.skillDays = (wasMenCount * receiverSkill.skillDays + menCount * giverSkillDays) / newTotalMenCount;
        receiverSkill.skillLevel = GetSkillLevelByDays(receiverSkill.skillDays);
    }

    for (const auto& giverSkill : giverSkills)
    {
        bool skillExists = false;
        std::string giverCode = std::string(giverSkill.skillCode.GetSafeCStr());
        std::transform(giverCode.begin(), giverCode.end(), giverCode.begin(), ::tolower);

        for (const auto& receiverSkill : receiverSkills)
        {
            std::string receiverCode = std::string(receiverSkill.skillCode.GetSafeCStr());
            std::transform(receiverCode.begin(), receiverCode.end(), receiverCode.begin(), ::tolower);

            if (receiverCode == giverCode)
            {
                skillExists = true;
                break;
            }
        }

        if (!skillExists && giverSkill.skillDays > 0)
        {
            TUnitSkill newSkill = giverSkill;

            if (newTotalMenCount > 0)
            {
                newSkill.skillDays = (menCount * giverSkill.skillDays) / newTotalMenCount;
                newSkill.skillLevel = GetSkillLevelByDays(newSkill.skillDays);

                if (newSkill.skillLevel > 0 || newSkill.skillDays > 0)
                {
                    receiverSkills.push_back(newSkill);
                }
            }
        }
    }

    receiverSkills.erase(
        std::remove_if(receiverSkills.begin(), receiverSkills.end(),
            [](const TUnitSkill& skill) {
                return skill.skillLevel <= 0 && skill.skillDays <= 0;
            }),
        receiverSkills.end()
    );

    wxString skillsString = SkillsVectorToString(receiverSkills);
    ReplaceSkillsBlock(&m_EndTurnDescription, skillsString);
}

//-------------------------------------------------------------

/**
 * Replaces the skills block in description with new string
 */
CStr* CUnit::ReplaceSkillsBlock(CStr* pDescr, const wxString& newSkillsStr)
{
    if (!pDescr || pDescr->IsEmpty())
        return pDescr;

    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int skillsStart = desc.Find("Skills:");

    if (skillsStart == wxNOT_FOUND)
    {
        if (newSkillsStr.IsEmpty())
            return pDescr;

        int weightPos = desc.Find("Weight:");
        if (weightPos != wxNOT_FOUND)
        {
            wxString beforeWeight = desc.Left(weightPos);
            wxString afterWeight = desc.Mid(weightPos);

            pDescr->Empty();
            pDescr->AddStr(beforeWeight.ToUTF8(), beforeWeight.Length());
            pDescr->AddStr(newSkillsStr.ToUTF8(), newSkillsStr.Length());
            if (!newSkillsStr.EndsWith(";"))
                pDescr->AddStr(";", 1);
            pDescr->AddStr(afterWeight.ToUTF8(), afterWeight.Length());
        }
        else
        {
            pDescr->AddStr(" ", 1);
            pDescr->AddStr(newSkillsStr.ToUTF8(), newSkillsStr.Length());
            if (!newSkillsStr.EndsWith(";"))
                pDescr->AddStr(";", 1);
        }

        return pDescr;
    }

    int skillsEnd = desc.Find(';', skillsStart);
    if (skillsEnd == wxNOT_FOUND)
        skillsEnd = desc.Length();

    wxString beforeSkills = desc.Left(skillsStart);
    wxString afterSkills = desc.Mid(skillsEnd);

    pDescr->Empty();
    pDescr->AddStr(beforeSkills.ToUTF8(), beforeSkills.Length());

    if (!newSkillsStr.IsEmpty())
    {
        pDescr->AddStr(newSkillsStr.ToUTF8(), newSkillsStr.Length());
        if (!newSkillsStr.EndsWith(";"))
            pDescr->AddStr(";", 1);
    }

    pDescr->AddStr(afterSkills.ToUTF8(), afterSkills.Length());

    return pDescr;
}

//-------------------------------------------------------------

/**
 * Completely removes skills block from description
 */
CStr* CUnit::RemoveSkillsBlock(CStr* pDescr)
{
    if (!pDescr || pDescr->IsEmpty())
        return pDescr;

    wxString desc = wxString::FromUTF8(pDescr->GetData());

    int skillsStart = desc.Find("Skills:");
    if (skillsStart == wxNOT_FOUND)
        return pDescr;

    int skillsEnd = desc.Find(';', skillsStart);
    if (skillsEnd == wxNOT_FOUND)
        skillsEnd = desc.Length();

    wxString beforeSkills = desc.Left(skillsStart);
    beforeSkills.Trim();

    wxString afterSkills = desc.Mid(skillsEnd + 1);

    pDescr->Empty();
    if (!beforeSkills.IsEmpty())
    {
        pDescr->AddStr(beforeSkills.ToUTF8(), beforeSkills.Length());
    }

    if (!afterSkills.IsEmpty())
    {
        if (!beforeSkills.IsEmpty() && !afterSkills.StartsWith("\n"))
        {
            pDescr->AddStr(" ", 1);
        }
        pDescr->AddStr(afterSkills.ToUTF8(), afterSkills.Length());
    }

    return pDescr;
}

//-------------------------------------------------------------
// Flag Manipulation Methods
//-------------------------------------------------------------

/**
 * Gets flag string by bit (first found)
 */
CStr CUnit::GetFlagString(unsigned long flagBit)
{
    CStr result;
    for (int i = 0; i < s_UnitFlagMappingsCount; i++)
    {
        if (s_UnitFlagMappings[i].flagBit == flagBit)
        {
            result = s_UnitFlagMappings[i].flagString;
            break;
        }
    }
    return result;
}

//-------------------------------------------------------------

/**
 * Clears a flag from description by its string
 */
CStr* CUnit::ClearFlag(CStr* pDescr, const CStr& flagString)
{
    if (!pDescr || pDescr->IsEmpty() || flagString.IsEmpty())
        return pDescr;

    int flagPos = pDescr->FindSubStr(flagString.GetSafeCStr());
    if (flagPos == -1)
        return pDescr;

    int len = flagString.GetLength();

    const char* data = pDescr->GetData();
    if (data && flagPos + len < pDescr->GetLength())
    {
        char nextChar = data[flagPos + len];
        if (nextChar == ',')
        {
            len++;
            if (flagPos + len < pDescr->GetLength() && data[flagPos + len] == ' ')
                len++;
        }
        else if (nextChar == ' ')
        {
            len++;
        }
    }

    pDescr->DelSubStr(flagPos, len);
    return pDescr;
}

//-------------------------------------------------------------

/**
 * Clears all SPOILS flags from description
 */
CStr* CUnit::ClearSpoils(CStr* pDescr)
{
    if (!pDescr || pDescr->IsEmpty())
        return pDescr;

    const CStr spoilsFlags[] = {
        CStr("weightless battle spoils"),
        CStr("flying battle spoils"),
        CStr("walking battle spoils"),
        CStr("riding battle spoils"),
        CStr("swiming battle spoils"),
        CStr("sailing battle spoils")
    };

    for (int i = 0; i < sizeof(spoilsFlags) / sizeof(spoilsFlags[0]); i++)
    {
        ClearFlag(pDescr, CStr(spoilsFlags[i]));
    }

    return pDescr;
}

//-------------------------------------------------------------

/**
 * Sets a flag in description by its string
 */
CStr* CUnit::SetFlag(CStr* pDescr, const CStr& flagString)
{
    if (!pDescr || pDescr->IsEmpty() || flagString.IsEmpty())
        return pDescr;

    if (HasFlag(pDescr, flagString))
        return pDescr;

    wxString desc = wxString::FromUTF8(pDescr->GetData());
    wxString flagStr = wxString::FromUTF8(flagString.GetSafeCStr());

    int insertPos = -1;
    int commaCount = 0;

    for (size_t i = 0; i < desc.Length(); i++)
    {
        if (desc[i] == wxT(','))
        {
            commaCount++;
            if (commaCount == 2)
            {
                insertPos = i + 1;
                break;
            }
        }
    }

    if (insertPos == -1)
    {
        int firstComma = desc.Find(wxT(','));
        if (firstComma != wxNOT_FOUND)
        {
            insertPos = firstComma + 1;
        }
    }

    if (insertPos == -1)
    {
        int firstBracket = desc.Find(wxT('['));
        if (firstBracket != wxNOT_FOUND)
        {
            insertPos = firstBracket;
            for (int i = firstBracket - 1; i >= 0; i--)
            {
                if (desc[i] == wxT(','))
                {
                    insertPos = i + 1;
                    break;
                }
                else if (i == 0)
                {
                    insertPos = 0;
                }
            }
        }
    }

    if (insertPos == -1)
    {
        int periodPos = desc.Find(wxT('.'));
        if (periodPos != wxNOT_FOUND)
        {
            insertPos = periodPos;
        }
        else
        {
            insertPos = desc.Length();
        }
    }

    wxString toInsert = wxT(" ") + flagStr + wxT(",");

    desc.insert(insertPos, toInsert);

    pDescr->Empty();
    pDescr->AddStr(desc.ToUTF8(), desc.Length());

    return pDescr;
}

//-------------------------------------------------------------

/**
 * Gets position of flag string in description
 */
int CUnit::GetPosFlag(CStr* pDescr, const CStr& searchString)
{
    if (!pDescr || pDescr->IsEmpty() || searchString.IsEmpty())
        return -1;

    return pDescr->FindSubStr(searchString.GetSafeCStr());
}

//-------------------------------------------------------------

/**
 * Checks if flag exists in description
 */
bool CUnit::HasFlag(CStr* pDescr, const CStr& flagString)
{
    return GetPosFlag(pDescr, flagString) != -1;
}

//-------------------------------------------------------------

/**
 * Restores original flags if they were changed by orders and then removed
 */
void CUnit::BackErasedFlags()
{
    if (m_EndTurnDescription.IsEmpty() || Description.IsEmpty())
        return;

    for (int i = 0; i < s_UnitFlagMappingsCount; i++)
    {
        const TUnitFlagMapping& flagMapping = s_UnitFlagMappings[i];
        CStr flagString = flagMapping.flagString;

        bool wasInOriginal = HasFlag(&Description, flagString);
        if (!wasInOriginal)
            continue;

        bool hasCancelOrder = false;

        if (!flagMapping.orderDisable.IsEmpty() && !Orders.IsEmpty())
        {
            CStr cancelOrderStr;
            cancelOrderStr << flagMapping.orderBase << " " << flagMapping.orderDisable;

            hasCancelOrder = (GetPosFlag(&Orders, cancelOrderStr) != -1);
        }

        if (!hasCancelOrder)
        {
            bool isInEndTurn = HasFlag(&m_EndTurnDescription, flagString);
            if (!isInEndTurn)
            {
                SetFlag(&m_EndTurnDescription, flagString);
            }
        }
    }
}

//-------------------------------------------------------------

/**
 * Gets all lines containing a specific order
 */
CStr CUnit::GetOrders(const char* orderToFind) const
{
    CStr result;

    if (!Orders.GetSafeCStr() || !orderToFind || !*orderToFind)
        return result;

    const char* src = Orders.GetSafeCStr();
    CStr line(256);
    BOOL insideTurnBlock = FALSE;
    BOOL insideFormBlock = FALSE;

    CStr searchOrderUpper(32);
    searchOrderUpper = orderToFind;
    searchOrderUpper.ToUpper();

    while (src && *src)
    {
        src = line.GetToken(src, '\n', TRIM_SPACES);
        line.TrimRight(TRIM_ALL);

        if (line.IsEmpty())
            continue;

        const char* lineData = line.GetData();

        if ((lineData[0] == ';') || (lineData[0] == '@' && lineData[1] == ';'))
            continue;

        BOOL isSimCommand = (lineData[0] == '@');
        const char* checkData = isSimCommand ? lineData + 1 : lineData;

        CStr lineUpper(256);
        lineUpper = checkData;
        lineUpper.ToUpper();

        if (stricmp(checkData, "TURN") == 0)
        {
            insideTurnBlock = TRUE;
            continue;
        }
        else if (stricmp(checkData, "ENDTURN") == 0)
        {
            insideTurnBlock = FALSE;
            continue;
        }

        if (stricmp(checkData, "FORM") == 0)
        {
            insideFormBlock = TRUE;
            continue;
        }
        else if (stricmp(checkData, "END") == 0 && insideFormBlock)
        {
            insideFormBlock = FALSE;
            continue;
        }

        if (insideTurnBlock || insideFormBlock)
            continue;

        if (strnicmp(checkData, searchOrderUpper.GetData(), searchOrderUpper.GetLength()) == 0)
        {
            if (!result.IsEmpty())
                result << EOL_SCR;
            result << line;
        }
    }

    return result;
}

//-------------------------------------------------------------

BOOL CUnit::HasOrder(const char* orderToFind) const
{
    CStr orders = GetOrders(orderToFind);
    return !orders.IsEmpty();
}

//-------------------------------------------------------------

long CUnit::GetMenCount()
{
    EValueType type;
    const void* pValue = nullptr;

    if (!GetProperty(PRP_MEN, type, pValue, eNormal) || !pValue)
        return 0;

    if (type != eLong)
        return 0;

    return static_cast<long>(reinterpret_cast<intptr_t>(pValue));
}

//-------------------------------------------------------------

BOOL CUnit::HasTaxOrder() const
{
    return HasOrder("TAX") || HasOrder("@TAX");
}

//-------------------------------------------------------------

BOOL CUnit::HasPillageOrder() const
{
    return HasOrder("PILLAGE") || HasOrder("@PILLAGE");
}

//-------------------------------------------------------------

/**
 * Initializes end turn description
 */
void CUnit::InitEndTurnDescription()
{
    m_EndTurnDescription = Description;

    if (IsOurs && !m_EndTurnDescription.IsEmpty())
    {
        wxString desc = wxString::FromUTF8(m_EndTurnDescription.GetData());

        if (desc.Find("At the end of turn:") == wxNOT_FOUND)
        {
            if (!desc.IsEmpty() && !desc.EndsWith("\n"))
                m_EndTurnDescription << EOL_SCR;

            m_EndTurnDescription << "At the end of turn:" << EOL_SCR;
        }
    }
}