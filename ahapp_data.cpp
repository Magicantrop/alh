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
#include "ahapp.h"

//-------------------------------------------------------------------------

const char * CAhApp::ResolveAlias(const char * alias)
{
    const char * p;
    const char * p1;
    int          cnt = 0;

    p1 = alias;
    do
    {
        // Get the resolved value for current alias
        p = SkipSpaces(GetConfig(SZ_SECT_ALIAS, p1));
        if (p && *p)
            p1 = p;  // Chain to next alias if found
        if (cnt++ >= 10)  // Prevent infinite recursion (max 10 levels)
        {
            break;
        }

    } while (p && *p);  // Continue while we get another alias

    return p1;
}

//-------------------------------------------------------------------------

long CAhApp::GetStudyCost(const char * skill)
{
    long        n;
    const char *p;

    // First resolve any alias, then get study cost from config
    p = ResolveAlias(skill);
    n = atol(GetConfig(SZ_SECT_STUDY_COST, p));

    return n;
}

//-------------------------------------------------------------------------

long CAhApp::GetStructAttr(const char * kind, long & MaxLoad, long & MinSailingPower)
{
    const char * attrlist;
    const char * p;
    CStr         S, Name;
    long         attr = 0;

    MaxLoad         = 0;
    MinSailingPower = 0;

    // Get comma-separated list of attributes for this structure type
    attrlist = GetConfig(SZ_SECT_STRUCTS, ResolveAlias(kind));
    while (attrlist && *attrlist)
    {
        attrlist = S.GetToken(attrlist, ',', TRIM_ALL);
        if (S.IsEmpty())
            break;

        // Parse standard structure flags
        if      (0==stricmp(SZ_ATTR_STRUCT_MOBILE , S.GetData()))      attr |= SA_MOBILE ;
        else if (0==stricmp(SZ_ATTR_STRUCT_HIDDEN , S.GetData()))      attr |= SA_HIDDEN ;
        else if (0==stricmp(SZ_ATTR_STRUCT_SHAFT  , S.GetData()))      attr |= SA_SHAFT  ;
        else if (0==stricmp(SZ_ATTR_STRUCT_GATE   , S.GetData()))      attr |= SA_GATE   ;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_N , S.GetData()))      attr |= SA_ROAD_N ;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_NE, S.GetData()))      attr |= SA_ROAD_NE;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_SE, S.GetData()))      attr |= SA_ROAD_SE;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_S , S.GetData()))      attr |= SA_ROAD_S ;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_SW, S.GetData()))      attr |= SA_ROAD_SW;
        else if (0==stricmp(SZ_ATTR_STRUCT_ROAD_NW, S.GetData()))      attr |= SA_ROAD_NW;
        else
        {
            // Handle numeric attributes (MAX_LOAD, MIN_POWER) that have values
            p = SkipSpaces(Name.GetToken(S.GetData(), ' ', TRIM_ALL));
            if      (0==stricmp(SZ_ATTR_STRUCT_MAX_LOAD, Name.GetData()))   MaxLoad         = atol(p);
            else if (0==stricmp(SZ_ATTR_STRUCT_MIN_SAIL, Name.GetData()))   MinSailingPower = atol(p);
        }
    }
    // Special case: ensure GATE flag is set for gate structures
    if (0 == stricmp(kind, STRUCT_GATE))
        attr |= SA_GATE; // to compensate for legacy missing gate flag in the config
    
    return attr; 
}


//-------------------------------------------------------------------------

BOOL CAhApp::GetItemWeights(const char * item, int *& weights, const char **& movenames, int & movecount )
{
    ItemWeights   Dummy;
    ItemWeights * pWeights;
    int           i;
    const char  * p;
    BOOL          Ok = TRUE;
    BOOL          Update = FALSE;

    Dummy.name = (char *)item;

    // Check if we already have cached weights for this item
    if (m_ItemWeights.Search(&Dummy, i))
        pWeights = (ItemWeights *)m_ItemWeights.At(i);
    else
    {
        CStr S;

        p = SkipSpaces(GetConfig(SZ_SECT_WEIGHT_MOVE, item));

        Ok = (p && *p);
        pWeights          = new ItemWeights;
        pWeights->name    = strdup(item);
        pWeights->weights = (int*)malloc(m_MoveModes.Count()*sizeof(int));

        // Parse weights for each movement mode (walk, ride, fly, swim, etc.)
        for (i=0; i<m_MoveModes.Count(); i++)
        {
            p = SkipSpaces(S.GetToken(p, ','));
            if (i==4 && S.IsEmpty())
            {
                // Handle migration to swimming mode (added in version 2.3.2)
                int x;
                for (x=0; x<DefaultConfigSize; x++)
                    if ( (0==stricmp(SZ_SECT_WEIGHT_MOVE, DefaultConfig[x].szSection)) &&
                         (0==stricmp(item               , DefaultConfig[x].szName))  )
                    {
                        const char * q = DefaultConfig[x].szValue;
                        int          m;
                        for (m=0; m<=i; m++)
                            q = SkipSpaces(S.GetToken(q, ','));
                        Update = TRUE;
                        break;
                    }
            }
            pWeights->weights[i] = atoi(S.GetData());
        }
        // If we updated swimming values, save them back to config
        if (Update && !IsASkillRelatedProperty(item))
        {
            S.Empty();
            for (i=0; i<m_MoveModes.Count(); i++)
            {
                if (i>0)
                    S << ',';
                S << (long)pWeights->weights[i];
            }
            SetConfig(SZ_SECT_WEIGHT_MOVE, item, S.GetData());
        }
        m_ItemWeights.Insert(pWeights);
    }

    weights   = pWeights->weights;
    movenames = (const char **)m_MoveModes.GetItems();
    movecount = m_MoveModes.Count();

    // Warn if weights are missing (except for standard properties)
    if (!Ok)
    {
        CStr S;
        bool skipit = false;
        if (!IsASkillRelatedProperty(item))
        {
            for (i=0; i<STD_UNIT_PROPS_COUNT; i++)
                if (0==stricmp(item, STD_UNIT_PROPS[i]))
                {
                    skipit = TRUE;
                    break;
                }
            if (!skipit)
            {
                S.Empty();
                S << "Warning! Weight and capacities for " << item <<
                     " are unknown and assumed to be zero. Movement modes can not be calculated correct. Update your " <<
                     SZ_CONFIG_FILE << " file!" <<EOL_SCR;
                ShowError(S.GetData(), S.GetLength(), TRUE);
            }
        }
    }

    return Ok;
}

//-------------------------------------------------------------------------

void CAhApp::GetMoveNames(const char **& movenames)
{
    movenames = (const char **)m_MoveModes.GetItems();
}

//-------------------------------------------------------------------------

BOOL CAhApp::GetOrderId(const char * order, long & id)
{
    const void * data = NULL;
    BOOL  Ok;

    // Look up order in hash table (case-sensitive)
    Ok = m_OrderHash.Locate(order, data);
    id = static_cast<long>(reinterpret_cast<intptr_t>(data));

    return Ok;
}

//-------------------------------------------------------------------------

BOOL CAhApp::IsTradeItem(const char * item)
{
    const void * data = NULL;

    // Check if item exists in trade items hash
    return m_TradeItemsHash.Locate(item, data);
}

//-------------------------------------------------------------------------

BOOL CAhApp::IsMan(const char * item)
{
    const void * data = NULL;

    // Check if item exists in men hash
    return m_MenHash.Locate(item, data);
}

//-------------------------------------------------------------------------

BOOL CAhApp::IsMagicSkill(const char * skill)
{
    const void * data = NULL;

    // Check if skill exists in magic skills hash
    return m_MagicSkillsHash.Locate(skill, data);
}

//-------------------------------------------------------------------------

const char * CAhApp::GetWeatherLine(BOOL IsCurrent, BOOL IsGood, int Zone)
{
    const char * szKey = NULL;

    // Select appropriate config key based on weather parameters
    if (IsCurrent)
        if (IsGood)
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_CUR_GOOD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_CUR_GOOD_MEDIUM;
        else
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_CUR_BAD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_CUR_BAD_MEDIUM;
    else
        if (IsGood)
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_NEXT_GOOD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_NEXT_GOOD_MEDIUM;
        else
            if (0==Zone) //Tropic
                szKey = SZ_KEY_WEATHER_NEXT_BAD_TROPIC;
            else
                szKey = SZ_KEY_WEATHER_NEXT_BAD_MEDIUM;

    return GetConfig(SZ_SECT_WEATHER, szKey);
}

//-------------------------------------------------------------------------

long CAhApp::GetMaxRaceSkillLevel(const char* race, const char* skill, const char* leadership, BOOL IsArcadiaSkillSystem)
{
    // Cache skill levels to avoid repeated config parsing
    long  level = 0;
    long  maxlevel = 0;
    CStr  sKey;
    CStr  sVal, S;
    const char* p;
    const void* pTempLevel = nullptr;

    if (!leadership)
        leadership = "";
    sKey << race << ":" << leadership << ":" << skill;

    // Check cache first
    if (!m_MaxSkillHash.Locate(sKey.GetData(), pTempLevel))
    {
        // Parse base skill level from config
        sVal = GetConfig(SZ_SECT_MAX_SKILL_LVL, race);
        p = sVal.GetData();

        p = S.GetToken(p, ',', TRIM_ALL);
        maxlevel = atol(S.GetData());

        p = S.GetToken(p, ',', TRIM_ALL);
        level = atol(S.GetData());

        // Check if skill is specifically listed for this race
        while (p && *p)
        {
            p = S.GetToken(p, ',', TRIM_ALL);
            if (0 == stricmp(skill, S.GetData()))
            {
                level = maxlevel;
                break;
            }
        }

        // Apply Arcadia-specific leadership bonuses
        if (IsArcadiaSkillSystem && *leadership)
        {
            if (IsMagicSkill(skill))
            {
                // For magic skills, only heroes get full access
                if (0 == stricmp(leadership, SZ_HERO))
                {
                    // Re-read magic skill levels from separate config section
                    sVal = GetConfig(SZ_SECT_MAX_MAG_SKILL_LVL, race);
                    p = sVal.GetData();

                    p = S.GetToken(p, ',', TRIM_ALL);
                    maxlevel = atol(S.GetData());

                    p = S.GetToken(p, ',', TRIM_ALL);
                    level = atol(S.GetData());

                    while (p && *p)
                    {
                        p = S.GetToken(p, ',', TRIM_ALL);
                        if (0 == stricmp(skill, S.GetData()))
                        {
                            level = maxlevel;
                            break;
                        }
                    }
                }
                else
                    level = 0;  // Non-heroes can't use magic skills in Arcadia
            }
            else
            {
                // Apply leadership bonus to non-magic skills
                int leader_bonus, hero_bonus, bonus = 0;

                sVal = GetConfig(SZ_SECT_COMMON, SZ_KEY_LEAD_SKILL_BONUS);
                p = sVal.GetData();

                p = S.GetToken(p, ',', TRIM_ALL);
                leader_bonus = atol(S.GetData());

                p = S.GetToken(p, ',', TRIM_ALL);
                hero_bonus = atol(S.GetData());

                if (0 == stricmp(leadership, SZ_LEADER))
                    bonus = leader_bonus;
                else
                    if (0 == stricmp(leadership, SZ_HERO))
                        bonus = hero_bonus;
                level += bonus;
            }
        }

        // Cache the result
        m_MaxSkillHash.Insert(sKey.GetData(), reinterpret_cast<void*>(static_cast<intptr_t>(level)));
    }
    else
    {
        // Use cached value
        level = static_cast<long>(reinterpret_cast<intptr_t>(pTempLevel));
    }

    return level;
}

//-------------------------------------------------------------------------

void CAhApp::GetProdDetails (const char * item, TProdDetails & details)
{
    CStr sVal, S;
    const char * p;
    int x;

    details.Empty();
    
    // Parse required skill
    sVal = GetConfig(SZ_SECT_PROD_SKILL, item);
    if (!sVal.IsEmpty())
    {
        S = details.skillname.GetToken(sVal.GetData(), ' ', TRIM_ALL);
        details.skilllevel = atol(S.GetData());
    }

    // Parse required resources (up to MAX_RES_NUM)
    sVal = GetConfig(SZ_SECT_PROD_RESOURCE, item);
    x = 0;
    p = sVal.GetData();
    while (p && *p && x<MAX_RES_NUM)
    {
        p = details.resname[x].GetToken(SkipSpaces(p), ' ', TRIM_ALL);
        p = S.GetToken(p, ',', TRIM_ALL);
        details.resamt[x] = atol(S.GetData());
        x++;
    }

    // Parse production months
    sVal = GetConfig(SZ_SECT_PROD_MONTHS, item);
    if (!sVal.IsEmpty())
        details.months = atol(sVal.GetData());

    // Parse required tool
    sVal = GetConfig(SZ_SECT_PROD_TOOL, item);
    if (!sVal.IsEmpty())
    {
        S = details.toolname.GetToken(sVal.GetData(), ' ', TRIM_ALL);
        details.toolhelp = atol(S.GetData());
    }
}

//-------------------------------------------------------------------------

BOOL CAhApp::CanSeeAdvResources(const char * skillname, const char * terrain, CLongColl & Levels, CBufColl & Resources)
{
    CStr         ProdSkillLine;
    CStr         ProdLandLine;
    BOOL         Ok = FALSE;
    const char * p1, * p2, *p;
    CStr         Prod1, Prod2, S1;
    long         level;

    Levels.FreeAll();
    Resources.FreeAll();

    // Get resources detectable by this skill
    ProdSkillLine = GetConfig(SZ_SECT_RESOURCE_SKILL,  skillname);
    ProdSkillLine.TrimRight(TRIM_ALL);

    // Get resources present in this terrain
    ProdLandLine = GetConfig(SZ_SECT_RESOURCE_LAND,  terrain);
    ProdLandLine.TrimRight(TRIM_ALL);

    if (!ProdSkillLine.IsEmpty() && !ProdLandLine.IsEmpty())
    {
        // For each resource detectable by skill
        p1 = SkipSpaces(S1.GetToken(ProdSkillLine.GetData(), ',', TRIM_ALL));
        while (!S1.IsEmpty())
        {
            p  = SkipSpaces(Prod1.GetToken(S1.GetData(), ' ', TRIM_ALL));
            level = p ? atol(p) : 0;

            // Check if it exists in this terrain
            p2 = SkipSpaces(Prod2.GetToken(ProdLandLine.GetData(), ',', TRIM_ALL));
            while (!Prod2.IsEmpty())
            {
                if (0==stricmp(Prod1.GetData(), Prod2.GetData()))
                {
                    // Resource found - add to result collections
                    Ok = TRUE;
                    Levels.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(level)));
                    Resources.Insert(strdup(Prod1.GetData()));
                    break;
                }
                p2 = SkipSpaces(Prod2.GetToken(p2, ',', TRIM_ALL));
            }

            p1 = SkipSpaces(S1.GetToken(p1, ',', TRIM_ALL));
        }
    }

    return Ok;
}

//-------------------------------------------------------------------------

int CAhApp::GetAttitudeForFaction(int id)
{
    int player_id = atol( GetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID));
    if(id == player_id) return ATT_FRIEND2;  // Player's own faction
    
    int attitude = ATT_UNDECLARED;
    CAttitude * policy;
    
    // Check declared attitudes
    for(int i=m_Attitudes.Count()-1; i>=0; i--)
    {
        policy = (CAttitude *) m_Attitudes.At(i);
        if(policy->FactionId == id) attitude=policy->Stance;
    }
    
    if(attitude == ATT_UNDECLARED)
    {
        // If no declared attitude, use default for all factions
        for(int i=m_Attitudes.Count()-1; i>=0; i--)
        {
            policy = (CAttitude *) m_Attitudes.At(i);
            if(policy->FactionId == 0) attitude=policy->Stance;
        }
    }
    return attitude;
}

//-------------------------------------------------------------------------
void CAhApp::SetAttitudeForFaction(int id, int attitude)
{
    int att_idx = -1;
    CAttitude * policy;
    
    // Validate attitude value
    if((attitude < ATT_FRIEND1) || (attitude >= ATT_UNDECLARED)) return;
    
    // Find existing declaration for this faction
    for(int i=m_Attitudes.Count()-1; i>=0; i--)
    {
        policy = (CAttitude *) m_Attitudes.At(i);
        if(policy && (policy->FactionId == id)) att_idx=i;
    }
    
    if(att_idx < 0)
    {   // Create new attitude declaration
        policy = new CAttitude;
        policy->FactionId = id;
        policy->SetStance(attitude);
        m_Attitudes.Insert(policy);
    }
    else
    {   // Update existing declaration
        policy = (CAttitude *) m_Attitudes.At(att_idx);
        policy->SetStance(attitude);
    }
}

//-------------------------------------------------------------------------

void CAhApp::GetShortFactName(CStr & S, int FactionId)
{
#define MAX_F_NAME 8
    int           i;
    char          ch;
    CFaction    * pFaction;

    S.Empty();
    pFaction = m_pAtlantis->GetFaction(FactionId);
    if (pFaction)
        S = pFaction->Name;
    else
        S << (long)FactionId;  // Fallback to ID if faction not found

    // Handle generic "faction" name
    if (0==stricmp(S.GetData(), "faction"))
    {
        S.Empty();
        S << "F_" << (long)FactionId << "_";
    }

    // Convert to lowercase and remove non-alphanumeric characters
    S.ToLower();
    for (i=S.GetLength()-1; i>=0; i--)
    {
        ch = S.GetData()[i];
        if ( (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') )
            S.DelCh(i);
    }
    
    // Truncate to maximum length
    if (S.GetLength() > MAX_F_NAME)
        S.DelSubStr(MAX_F_NAME, S.GetLength()-MAX_F_NAME);
    S.TrimRight(TRIM_ALL);
}