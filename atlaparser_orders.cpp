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
#include "wx/tokenzr.h"
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

 //--------------------------------------------------------------------------
 // Error reporting macros
 //--------------------------------------------------------------------------

#define SHOW_WARN(msg)                               \
{                                                    \
    if (!skiperror)                                  \
    {                                                \
        ErrorLine.Empty();                           \
        ErrorLine << Line << msg;                    \
        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit); \
    }                                                \
}

#define SHOW_WARN_CONTINUE(msg)                      \
{                                                    \
    if (!skiperror)                                  \
    {                                                \
        ErrorLine.Empty();                           \
        ErrorLine << Line << msg;                    \
        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit); \
    }                                                \
    continue;                                        \
}

#define SHOW_WARN_BREAK(msg)                         \
{                                                    \
    if (!skiperror)                                  \
    {                                                \
        ErrorLine.Empty();                           \
        ErrorLine << Line << msg;                    \
        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit); \
        break;                                       \
    }                                                \
}

//----------------------------------------------------------------------
// SHARE SILVER command processing
// Distributes silver from units with positive balance to cover deficits
//----------------------------------------------------------------------

BOOL CAtlaParser::ShareSilver(CUnit * pMainUnit)
{
    CLand             * pLand = NULL;
    CUnit             * pUnit;
    int                 idx;
    long                unitmoney;
    long                mainmoney;
    long                n;
    EValueType          type;
    CStr                Line(32);
    BOOL                Changed = FALSE;

    do
    {
        // Validate unit and land
        if (!pMainUnit || IS_NEW_UNIT(pMainUnit) || !pMainUnit->IsOurs  )
            break;

        pLand = GetLand(pMainUnit->LandId);
        if (!pLand)
            break;

        RunLandOrders(pLand); // Ensure orders are processed

        // Get main unit's silver
        const void* pValue = nullptr;
        if (!pMainUnit->GetProperty(PRP_SILVER, type, pValue, eNormal) )
        {
            mainmoney = 0;
            type      = eLong;
            long zero = 0;
            if (PE_OK!=pMainUnit->SetProperty(PRP_SILVER, type, reinterpret_cast<const void*>(static_cast<intptr_t>(zero)), eBoth))
                GEN_ERR(pMainUnit, NOSETUNIT << pMainUnit->Id << BUG);
        }
        else if (eLong != type) 
        {
            GEN_ERR(pMainUnit, NOTNUMERIC << pMainUnit->Id << BUG);
        } 
        else
            mainmoney = static_cast<long>(reinterpret_cast<intptr_t>(pValue));

        // Subtract already received silver
        mainmoney -= pMainUnit->SilvRcvd;

        if (mainmoney<=0)
            break;

        // Find units with negative silver and cover their deficits
        for (idx=0; idx<pLand->Units.Count(); idx++)
        {
            pUnit = (CUnit*)pLand->Units.At(idx);
            if (!pUnit->IsOurs)
                continue;

            const void* pUnitValue = nullptr;
            if (!pUnit->GetProperty(PRP_SILVER, type, pUnitValue, eNormal) )
                continue;
            else if (eLong != type)
            {
                GEN_ERR(pUnit, NOTNUMERIC << pUnit->Id << BUG);
            }
            else
                unitmoney = static_cast<long>(reinterpret_cast<intptr_t>(pUnitValue));

            if (unitmoney>=0)
                continue;

            n = -unitmoney;
            if (n>mainmoney)
            {
                n         = mainmoney;
                mainmoney = 0;
            }
            else
                mainmoney -= n;

            // Generate GIVE command to transfer silver
            pMainUnit->Orders.TrimRight(TRIM_ALL);
            if (!pMainUnit->Orders.IsEmpty())
                pMainUnit->Orders << EOL_SCR ;
            if (IS_NEW_UNIT(pUnit))
                pMainUnit->Orders << "GIVE NEW " << (long)REVERSE_NEW_UNIT_ID(pUnit->Id) << " " << n << " SILV";
            else
                pMainUnit->Orders << "GIVE " << pUnit->Id   << " " << n << " SILV";
            Changed = TRUE;
            if (mainmoney<=0)
                break;
        }
        if (Changed)
            RunLandOrders(pLand);
    } while (FALSE);

    OrderErrFinalize();
    return Changed;
}

//-------------------------------------------------------------
// GENERATE GIVE EVERYTHING command
// Creates GIVE commands for all non-standard properties
//-------------------------------------------------------------

BOOL CAtlaParser::GenGiveEverything(CUnit * pFrom, const char * To)
{
    CLand             * pLand = NULL;
    int                 no;
    BOOL                Changed = FALSE;
    const char        * propname;
    int                 i;
    BOOL                skipit;
    EValueType          type;
    long                amount;
    long                amountorg;
    long                x;
    CUnit               Dummy;
    CStr                S;
    const char        * p;
    long                n1;

    do
    {
        // Validate unit and target
        if (!pFrom || IS_NEW_UNIT(pFrom) || !pFrom->IsOurs || !To || !*To )
            break;

        pLand = GetLand(pFrom->LandId);
        if (!pLand)
            break;

        // Validate target unit ID
        p  = To;
        if (!GetTargetUnitId(p, pFrom->FactionId, n1))
        {
            S << "Invalid unit id " << To;
            OrderErr(0, pFrom->Id, S.GetData());
            break;
        }
        Dummy.Id = n1;
        if (n1 != 0 && !pLand->Units.Search(&Dummy, i) )
        {
            S << "Can not find unit " << To;
            OrderErr(0, pFrom->Id, S.GetData());
            break;
        }

        RunLandOrders(pLand);

        pFrom->Orders.TrimRight(TRIM_ALL);
        if (!pFrom->Orders.IsEmpty())
            pFrom->Orders << EOL_SCR ;

        no = 0;
        do
        {
            propname = pFrom->GetPropertyName(no++);
            skipit   = FALSE;

            if (!propname)
                break;

            // Skip standard properties
            for (i=0; i<STD_UNIT_PROPS_COUNT; i++)
                if (0==stricmp(propname, STD_UNIT_PROPS[i]))
                {
                    skipit = TRUE;
                    break;
                }
            if (skipit)
                continue;

            if (IsASkillRelatedProperty(propname))
                continue;

            // Get current and original amounts
            const void* pValue = nullptr;
            if (!pFrom->GetProperty(propname, type, pValue, eNormal) || eLong!=type)
                continue;
            amount = static_cast<long>(reinterpret_cast<intptr_t>(pValue));
            
            const void* pValueOrg = nullptr;
            if (!pFrom->GetProperty(propname, type, pValueOrg, eOriginal) || eLong!=type)
                continue;
            amountorg = static_cast<long>(reinterpret_cast<intptr_t>(pValueOrg));
            
            x = std::min(amount, amountorg);
            if (x<=0)
                continue;

            // Generate GIVE command
            pFrom->Orders << "GIVE " << To << " " << x << " " << propname << EOL_SCR;
            Changed = TRUE;

        } while (propname);

        if (Changed)
            RunLandOrders(pLand);
    }
    while (FALSE);

    OrderErrFinalize();
    return Changed;
}

//-------------------------------------------------------------
// GENERATE TEACH orders
// Creates TEACH commands for students that need teaching
//-------------------------------------------------------------

BOOL CAtlaParser::GenOrdersTeach(CUnit * pMainUnit)
{
    CLand             * pLand = NULL;
    CUnit             * pUnit;
    int                 idx;
    EValueType          type;
    long                n1, n2;
    CStr                Line(32);
    CStr                Skill;
    BOOL                Changed  = FALSE;
    BOOL                leader_checked = FALSE;
    const void        * value;

    do
    {
        if (!pMainUnit || !pMainUnit->IsOurs)
            break;

        pLand = GetLand(pMainUnit->LandId);
        if (!pLand)
            break;

        // Check if unit is leader/hero
        if (!leader_checked && !pMainUnit->GetProperty(PRP_LEADER, type, value, eNormal) )
            break;
        else
            leader_checked = TRUE;

        RunLandOrders(pLand);

        // Find students that need teaching
        for (idx=0; idx<pLand->UnitsSeq.Count(); idx++)
        {
            pUnit = (CUnit*)pLand->UnitsSeq.At(idx);
            if (!pUnit->StudyingSkill.IsEmpty())
            {
                Skill = pUnit->StudyingSkill;
                
                // Get teacher skill level
                const void* pValue1 = nullptr;
                if (!pMainUnit->GetProperty(Skill.GetData(), type, pValue1, eNormal) )
                    n1 = 0;
                else
                    n1 = static_cast<long>(reinterpret_cast<intptr_t>(pValue1));
                    
                // Get student skill level
                const void* pValue2 = nullptr;
                if (!pUnit->GetProperty(Skill.GetData(), type, pValue2, eNormal) )
                    n2 = 0;
                else
                    n2 = static_cast<long>(reinterpret_cast<intptr_t>(pValue2));

                // Try Arcadia skill system if normal teaching not possible
                if (n1 <= n2)
                {
                    int  SkillPos = Skill.FindSubStrR(PRP_SKILL_POSTFIX);
                    if (SkillPos>=0)
                        Skill.DelSubStr(SkillPos, strlen(PRP_SKILL_POSTFIX));

                    Skill << PRP_SKILL_STUDY_POSTFIX;
                    
                    const void* pValue3 = nullptr;
                    if (!pMainUnit->GetProperty(Skill.GetData(), type, pValue3, eNormal) )
                        n1 = 0;
                    else
                        n1 = static_cast<long>(reinterpret_cast<intptr_t>(pValue3));
                        
                    const void* pValue4 = nullptr;
                    if (!pUnit->GetProperty(Skill.GetData(), type, pValue4, eNormal) )
                        n2 = 0;
                    else
                        n2 = static_cast<long>(reinterpret_cast<intptr_t>(pValue4));
                }

                // Check if teaching possible
                if ( (n1 > n2) && (pUnit->Teaching <= 20) )
                {
                    // Count men
                    const void* pMenValue1 = nullptr;
                    if (!pMainUnit->GetProperty(PRP_MEN, type, pMenValue1, eNormal)  || !pMenValue1)
                        break;
                    n1 = static_cast<long>(reinterpret_cast<intptr_t>(pMenValue1));
                    if (n1<=0) break;
                    
                    const void* pMenValue2 = nullptr;
                    if (!pUnit->GetProperty(PRP_MEN, type, pMenValue2, eNormal) || !pMenValue2)
                        continue;
                    n2 = static_cast<long>(reinterpret_cast<intptr_t>(pMenValue2));
                    if (n2<=0) continue;

                    // Check teacher capacity
                    if (n1*(STUDENTS_PER_TEACHER - pMainUnit->Teaching) >= n2)
                    {
                        pMainUnit->Teaching += (double)n2/n1;

                        pMainUnit->Orders.TrimRight(TRIM_ALL);
                        if (!pMainUnit->Orders.IsEmpty())
                            pMainUnit->Orders << EOL_SCR ;
                        if (IS_NEW_UNIT(pUnit))
                            pMainUnit->Orders << "TEACH " << "NEW " << (long)REVERSE_NEW_UNIT_ID(pUnit->Id);
                        else
                            pMainUnit->Orders << "TEACH " << pUnit->Id;
                        Changed = TRUE;
                    }
                }
            }
        }

    } while (FALSE);

    if (Changed)
        RunLandOrders(pLand);

    return Changed;
}

//-------------------------------------------------------------
// DISCARD JUNK ITEMS
// Generates GIVE 0 commands for items marked as junk
//-------------------------------------------------------------

BOOL CAtlaParser::DiscardJunkItems(CUnit * pUnit, const char * junk)
{
    CLand             * pLand = NULL;
    EValueType          type;
    long                value;
    CStr                sJunkItem(32);
    BOOL                Changed = FALSE;

    if (!pUnit || IS_NEW_UNIT(pUnit) || !pUnit->IsOurs )
        return FALSE;

    pLand = GetLand(pUnit->LandId);
    if (!pLand)
        return FALSE;

    RunLandOrders(pLand);

    // Process each junk item type
    while (junk && *junk)
    {
        junk = sJunkItem.GetToken(junk, ',');
        const void* pValue = nullptr;
        if (!pUnit->GetProperty(sJunkItem.GetData(), type, pValue, eNormal) || (eLong!=type))
            continue;
        value = static_cast<long>(reinterpret_cast<intptr_t>(pValue));

        // Generate GIVE 0 command (discard to void)
        pUnit->Orders.TrimRight(TRIM_ALL);
        if (!pUnit->Orders.IsEmpty())
            pUnit->Orders << EOL_SCR ;
        pUnit->Orders << "GIVE 0 " << value << " " << sJunkItem;
        Changed = TRUE;
    }
    if (Changed)
        RunLandOrders(pLand);
    return Changed;
}

//-------------------------------------------------------------
// DETECT SPIES
// Generates GIVE commands to detect spies by silver amounts
//-------------------------------------------------------------

BOOL CAtlaParser::DetectSpies(CUnit * pUnit, long lonum, long hinum, long amount)
{
    CLand             * pLand = NULL;
    BOOL                Changed = FALSE;
    long                no;
    int                 idx = 0;
    CUnit             * pNext;

    if (!pUnit || IS_NEW_UNIT(pUnit) || !pUnit->IsOurs)
        return FALSE;

    pLand = GetLand(pUnit->LandId);
    if (!pLand)
        return FALSE;

    RunLandOrders(pLand);

    pUnit->Orders.TrimRight(TRIM_ALL);
    if (!pUnit->Orders.IsEmpty())
        pUnit->Orders << EOL_SCR ;

    // Generate GIVE commands for all unit IDs in range
    no = lonum;
    while (no<=hinum)
    {
        pNext = (CUnit*)m_Units.At(idx);

        if (pNext)
        {
            while (no<pNext->Id)
            {
                pUnit->Orders << "GIVE " << no << " " << amount << " SILV ;ne"  << EOL_SCR;
                Changed = TRUE;
                no++;
            }
            if (no == pNext->Id)
                no++;
            idx++;
        }
        else
        {
            pUnit->Orders << "GIVE " << no << " " << amount << " SILV ;ne"  << EOL_SCR;
            Changed = TRUE;
            no++;
        }
    }

    if (Changed)
        RunLandOrders(pLand);

    return Changed;
}

//-------------------------------------------------------------
// Helper: Reads a property name from string, resolving aliases
//-------------------------------------------------------------

const char * CAtlaParser::ReadPropertyName(const char * src, CStr & Name)
{
    char ch;
    int  i;
    CStr S;

    Name.Empty();
    src = SkipSpaces(S.GetToken(src, " \t;\n", ch, TRIM_ALL));

    if (!S.IsEmpty())
    {
        // Replace spaces with underscores in property names
        for (i=0; i<S.GetLength(); i++)
            if (' '==S.GetData()[i])
                S.SetCh(i, '_');
        Name = gpDataHelper->ResolveAlias(S.GetData());
    }

    return src;
}

//-------------------------------------------------------------
// Helper: Parses target unit ID from command parameters
// Handles formats: "123", "NEW 5", "FACTION X NEW Y"
//-------------------------------------------------------------

BOOL CAtlaParser::GetTargetUnitId(const char *& p, long FactionId, long & nId)
{
    CStr                N1(32), N(32), X, Y;
    char                ch;

    p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
    if (0==stricmp("FACTION", N1.GetData()))
    {
        // Format: FACTION X NEW Y
        p = SkipSpaces(X.GetToken(p, " \t", ch, TRIM_ALL));  // X
        if (!X.IsInteger())
            return FALSE;
        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));  // NEW
        if (0!=stricmp(N1.GetData(), "NEW"))
            return FALSE;
        p = SkipSpaces(Y.GetToken(p, " \t", ch, TRIM_ALL));  // Y
        if (!Y.IsInteger())
            return FALSE;

        if ( atol(gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_CHECK_NEW_UNIT_FACTION)))
            nId = NEW_UNIT_ID(atol(Y.GetData()), atol(X.GetData()));
        else
            nId = 0;
        return TRUE;
    }
    if (0==stricmp("NEW", N1.GetData()))
    {
        // Format: NEW 5
        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
        if (!N1.IsInteger())
            return FALSE;
        nId = NEW_UNIT_ID(atol(N1.GetData()), FactionId);
        return TRUE;
    }

    // Format: simple number
    if (!N1.IsInteger())
        return FALSE;
    nId = atol(N1.GetData());
    return TRUE;
}

//-------------------------------------------------------------
// Error handling utilities
//-------------------------------------------------------------

void CAtlaParser::OrderErrFinalize()
{
    if (gpDataHelper && !m_sOrderErrors.IsEmpty())
        gpDataHelper->ReportError(m_sOrderErrors.GetData(), m_sOrderErrors.GetLength(), TRUE);

    m_sOrderErrors.Empty();
}

void CAtlaParser::OrderErr(int Severity, int UnitId, const char * Msg, const char * UnitName, CUnit * pUnit)
{
    const char * type;
    CStr         S(32);
    CStr         prefix;
    CStr         land;

    if (0==Severity)
        type = "Error  ";
    else
        type = "Warning";

    if (pUnit && IS_NEW_UNIT(pUnit))
    {
        ComposeLandStrCoord(GetLand(pUnit->LandId), land);
        prefix.Format("(%s) ", land.GetData());
    }
    if (UnitName)
        S.Format("%s%s (%d) %s : %s%s", prefix.GetData(), UnitName, UnitId, type, Msg, EOL_SCR);
    else
        S.Format("%sUnit % 5d %s : %s%s", prefix.GetData(), UnitId, type, Msg, EOL_SCR);

    m_sOrderErrors << S;
}

//-------------------------------------------------------------

void CAtlaParser::GenericErr(int Severity, const char * Msg)
{
    const char * type;
    CStr         S(32);

    if (!gpDataHelper)
        return;

    if (0==Severity)
        type = "Error  ";
    else
        type = "Warning";
    S.Format("%s : %s%s", type, Msg, EOL_SCR);

    gpDataHelper->ReportError(S.GetData(), S.GetLength(), FALSE);
}

//-------------------------------------------------------------
// Teaching helper - called at end of unit processing
//-------------------------------------------------------------

void CAtlaParser::OrderProcess_Teach(BOOL skiperror, CUnit * pUnit)
{
    pUnit->Teaching = 0;
}

//-------------------------------------------------------------
// Main order execution entry point
//-------------------------------------------------------------

void CAtlaParser::RunOrders(CLand* pLand, const char* sCheckTeach)
{
    int         i, n;
    CPlane* pPlane;

    if (!sCheckTeach)
        sCheckTeach = gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_CHECK_TEACH_LVL);

    if (pLand)
    {
        RunLandOrders(pLand, sCheckTeach);
    }
    else  // Run orders for all lands
    {
        for (n = 0; n < m_Planes.Count(); n++)
        {
            pPlane = (CPlane*)m_Planes.At(n);
            for (i = 0; i < pPlane->Lands.Count(); i++)
            {
                pLand = (CLand*)pPlane->Lands.At(i);
                if (pLand)
                    RunLandOrders(pLand, sCheckTeach);
            }
        }
    }
}

//-------------------------------------------------------------
// Process pseudo-comments (;ne, ;@command, etc.)
//-------------------------------------------------------------

void CAtlaParser::RunPseudoComment(CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * comment, int sequence, wxString & destination)
{
    if (!comment || !*comment)
        return;

    // Check for simulation commands
    if (comment[0] == '@')
    {
        // Store destination for MOVE simulation
        if (strncmp(comment, "@move ", 5) == 0)
        {
            destination = wxString::FromUTF8(comment + 5);
        }
    }
}

//-------------------------------------------------------------
// TAX/PILLAGE order processing
// Distributes silver from taxable amount to pillagers or taxers
//-------------------------------------------------------------

void CAtlaParser::RunOrder_TaxPillage(CLand* pLand)
{
    if (pLand->Taxable == 0)
        return;

    // Count people with PILLAGE flag/order
    long pillagers = 0;
    // Count people with TAX flag/order
    long taxers = 0;

    CUnit* pUnit;
    for (int unitidx = 0; unitidx < pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);

        if (pUnit->IsOurs)
        {
            long unitMen = pUnit->GetMenCount();

            BOOL hasPillageFlag = (pUnit->Flags & UNIT_FLAG_PILLAGING) != 0;
            BOOL hasPillageOrder = pUnit->HasPillageOrder();

            if (hasPillageFlag || hasPillageOrder)
                pillagers += unitMen;

            BOOL hasTaxFlag = (pUnit->Flags & UNIT_FLAG_TAXING) != 0;
            BOOL hasTaxOrder = pUnit->HasTaxOrder();

            if (hasTaxFlag || hasTaxOrder)
                taxers += unitMen;
        }
    }

    const int maxTaxPerTaxer = atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_TAX_PER_TAXER));

    // Pillagers get double if they can take all
    if (pillagers > 0 && (pillagers * maxTaxPerTaxer * 2 >= pLand->Taxable))
    {
        long silver = pLand->Taxable * 2;
        DistributeSilver(pLand, UNIT_FLAG_PILLAGING, silver, pillagers);
    }
    else if (taxers > 0)
    {
        long silver = std::min(pLand->Taxable, maxTaxPerTaxer * taxers);
        DistributeSilver(pLand, UNIT_FLAG_TAXING, silver, taxers);
    }
}

//-------------------------------------------------------------
// ENTERTAIN order processing
// Distributes entertainment silver and adds skill days
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Entertain(CLand* pLand)
{
    if (pLand->Entertainment == 0)
        return;

    long entertainers = 0;
    CUnit* pUnit;

    for (int unitidx = 0; unitidx < pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);

        if (pUnit->IsOurs)
        {
            BOOL hasFlag = (pUnit->Flags & UNIT_FLAG_ENTERTAINING) != 0;
            BOOL hasOrder = pUnit->HasOrder("ENTERTAIN") || pUnit->HasOrder("@ENTERTAIN");

            if (hasFlag || hasOrder)
            {
                entertainers += pUnit->GetMenCount();
                pUnit->m_EndTurnDescription = *pUnit->AidSkillDays(&pUnit->m_EndTurnDescription,"ENTE",5);
            }
        }
    }

    if (entertainers > 0)
    {
        const int entertainmentPerMan = atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_ENTERTAINMENT_SILVER));
        long silver = std::min(pLand->Entertainment, entertainmentPerMan * entertainers);
        DistributeSilver(pLand, UNIT_FLAG_ENTERTAINING, silver, entertainers);
    }
}

//-------------------------------------------------------------
// WORK order processing
// Distributes wages to workers
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Work(CLand* pLand)
{
    if (pLand->MaxWages == 0 || pLand->Wages == 0)
        return;

    long workers = 0;
    CUnit* pUnit;

    for (int unitidx = 0; unitidx < pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);

        if (pUnit->IsOurs)
        {
            BOOL hasFlag = (pUnit->Flags & UNIT_FLAG_WORKING) != 0;
            BOOL hasOrder = pUnit->HasOrder("WORK") || pUnit->HasOrder("@WORK");

            if (hasFlag || hasOrder)
                workers += pUnit->GetMenCount();
        }
    }

    if (workers > 0)
    {
        long silver = std::min((double)pLand->MaxWages, pLand->Wages * workers);
        DistributeSilver(pLand, UNIT_FLAG_WORKING, silver, workers);
    }
}

//-------------------------------------------------------------
// UPKEEP calculation (per unit)
// Deducts maintenance costs for leaders/peasants
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Upkeep(CUnit * pUnit, int turns)
{
    EValueType          type;
    long nmen;
    int unitSilver;
    int Maintainance;
    const char * leadership;

    const void* pMenValue = nullptr;
    if (pUnit->GetProperty(PRP_MEN, type, pMenValue, eNormal) && pMenValue)
    {
        nmen = static_cast<long>(reinterpret_cast<intptr_t>(pMenValue));
        if (nmen>0)
        {
            const void* pLeadValue = nullptr;
            if (pUnit->GetProperty(PRP_LEADER, type, pLeadValue, eNormal) && pLeadValue && eCharPtr==type)
            {
                leadership = static_cast<const char*>(pLeadValue);
                if ((0==strcmp(leadership, SZ_LEADER) || 0==strcmp(leadership, SZ_HERO)))
                    Maintainance = nmen * atoi(gpApp->GetConfig(SZ_SECT_COMMON, SZ_UPKEEP_LEADER));
                else
                    Maintainance = nmen * atoi(gpApp->GetConfig(SZ_SECT_COMMON, SZ_UPKEEP_PEASANT));
            }
            else
                Maintainance = nmen * atoi(gpApp->GetConfig(SZ_SECT_COMMON, SZ_UPKEEP_PEASANT));
            
            const void* pSilverValue = nullptr;
            if (!pUnit->GetProperty(PRP_SILVER, type, pSilverValue, eNormal))
            {
                unitSilver = 0;
                pUnit->SetProperty(PRP_SILVER, eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(unitSilver)), eBoth);
            }
            else
                unitSilver = static_cast<int>(reinterpret_cast<intptr_t>(pSilverValue));
            
            unitSilver -= Maintainance * turns;
            pUnit->SetProperty(PRP_SILVER, eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(unitSilver)), eNormal);
        }
    }
}

void CAtlaParser::RunOrder_Upkeep(CLand * pLand)
{
    CUnit * pUnit;

    for (int unitidx=0; unitidx<pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);
        if (pUnit->FactionId == m_CrntFactionId)
            RunOrder_Upkeep(pUnit, 1);
    }
}

//-------------------------------------------------------------
// SHARE SILVER order processing (economy phase)
// Redistributes silver from units with surplus to cover deficits
//-------------------------------------------------------------

void CAtlaParser::RunOrder_ShareSilver (CStr & LineOrig, CStr & ErrorLine, BOOL skiperror, CLand * pLand, SHARE_TYPE shareType, wxString shareName)
{
    EValueType          type;
    CUnit * pUnit;
    long silverNeeded = 0;
    long unitSilver;
    long shareSilver;
    long silverNeededOrig;
    CStr Line;
    CUnit * displayUnit = NULL;

    // Calculate total silver needed (negative balances)
    for (int unitidx=0; unitidx<pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);
        if (pUnit->IsOurs && !IS_NEW_UNIT(pUnit))
            displayUnit = pUnit;
        if (shareType == SHARE_BUY || !pUnit->pMovement || pUnit->pMovement->Count() == 0)
        {
            const void* pValue = nullptr;
            if (pUnit->GetProperty(PRP_SILVER, type, pValue, eNormal))
            {
                unitSilver = static_cast<long>(reinterpret_cast<intptr_t>(pValue));
                if (unitSilver < 0)
                    silverNeeded -= unitSilver;
            }
        }
    }

    if (!silverNeeded) return;
    silverNeededOrig = silverNeeded;

    // Collect surplus silver from units with positive balance
    for (int unitidx=0; unitidx<pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);
        if (!pUnit->IsOurs) continue;
        if (shareType == SHARE_BUY || !pUnit->pMovement || pUnit->pMovement->Count() == 0)
        {
            if ((shareType == SHARE_UPKEEP || pUnit->Flags & UNIT_FLAG_SHARING))
            {
                const void* pValue = nullptr;
                if (pUnit->GetProperty(PRP_SILVER, type, pValue, eNormal))
                {
                    unitSilver = static_cast<long>(reinterpret_cast<intptr_t>(pValue));
                    if (unitSilver > 0)
                    {
                        shareSilver = std::min(unitSilver, silverNeeded);
                        silverNeeded -= shareSilver;
                        unitSilver -= shareSilver;
                        pUnit->SetProperty(PRP_SILVER, eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(unitSilver)), eNormal);
                        if (!silverNeeded)
                            break;
                    }
                }
            }
        }
    }

    // Distribute collected silver to units in deficit
    long silverAvailable = silverNeededOrig - silverNeeded;
    long shortage = silverNeeded;

    for (int unitidx=0; unitidx<pLand->UnitsSeq.Count(); ++unitidx)
    {
        pUnit = (CUnit*)pLand->UnitsSeq.At(unitidx);
        if (!pUnit->IsOurs) continue;
        if (shareType == SHARE_BUY || !pUnit->pMovement || pUnit->pMovement->Count() == 0)
        {
            const void* pValue = nullptr;
            if (pUnit->GetProperty(PRP_SILVER, type, pValue, eNormal))
            {
                unitSilver = static_cast<long>(reinterpret_cast<intptr_t>(pValue));
                if (unitSilver < 0)
                {
                    shareSilver = std::min(-unitSilver, silverAvailable);
                    silverAvailable -= shareSilver;
                    unitSilver += shareSilver;
                    pUnit->SetProperty(PRP_SILVER, eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(unitSilver)), eNormal);
                }
            }
        }
    }

    // Report any remaining shortage
    if (shortage > 0 && displayUnit)
    {
        pUnit = displayUnit;
        CStr sCoord;
        ComposeLandStrCoord(pLand, sCoord);
        wxString message = wxT(" - ") + wxString::FromUTF8(sCoord.GetData());
        message += wxString::Format(" - Shortage of %d silver for %s.", shortage, shareName);
        
        if (!skiperror)
        {
            ErrorLine.Empty();
            ErrorLine << Line << message.ToUTF8();
            OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
        }
    }
}

//-------------------------------------------------------------
// STUDY command processing
// Handles skill study, cost deduction, and teacher tracking
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Study(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params)
{
    EValueType          type;
    long                n1, n2;
    long                unitmoney;
    CStr                S(32);
    CStr                SkillNaked(32);
    long                minlevel = -1;
    long                level;
    long                no, propval;
    const char* lead = "";
    const char* racename;
    const void* pTempValue = nullptr;

    // Variables for tracking teacher
    BOOL                hasTeacher = FALSE;
    CUnit* pTeacher = NULL;
    long                teacherSkillLevel = 0;
    long                studentSkillLevel = 0;
    CStr                teacherComment;

    // Store the command part (everything before ';')
    CStr commandPart;
    CStr commentsPart;
    CStr originalLine = Line;

    do
    {
        params = ReadPropertyName(params, SkillNaked);

        n1 = gpDataHelper->GetStudyCost(SkillNaked.GetData());
        if (n1 <= 0)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Can not study that!";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }

        // Check if unit has men
        pTempValue = nullptr;
        if (!pUnit->GetProperty(PRP_MEN, type, pTempValue, eNormal) || (!pTempValue))
        {
            n2 = 0;
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - There are no men in the unit!";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }
        else if (eLong != type)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - " << NOTNUMERIC << pUnit->Id << BUG;
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            n2 = 0;
        }
        else
        {
            n2 = static_cast<long>(reinterpret_cast<intptr_t>(pTempValue));
            if (n2 <= 0)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - There are no men in the unit!";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                continue;
            }
        }

        // Determine maximum skill level based on race composition
        no = 0;
        racename = pUnit->GetPropertyName(no);
        while (racename)
        {
            if (gpDataHelper->IsMan(racename))
            {
                pTempValue = nullptr;
                if (pUnit->GetProperty(racename, type, pTempValue, eNormal) &&
                    eLong == type && pTempValue)
                {
                    propval = static_cast<long>(reinterpret_cast<intptr_t>(pTempValue));
                    if (propval > 0)
                    {
                        const void* pTempLead = nullptr;
                        pUnit->GetProperty(PRP_LEADER, type, pTempLead, eNormal);
                        if (pTempLead)
                            lead = static_cast<const char*>(pTempLead);

                        level = gpDataHelper->MaxSkillLevel(racename, SkillNaked.GetData(), lead, m_ArcadiaSkills);
                        if (-1 == minlevel)
                            minlevel = level;
                        else if (level < minlevel)
                            minlevel = level;
                    }
                }
            }
            no++;
            racename = pUnit->GetPropertyName(no);
        }

        // Get current skill level from Description
        S = SkillNaked;
        S << PRP_SKILL_POSTFIX;

        wxString skillCode = wxString::FromUTF8(SkillNaked.GetData()).Upper();
        TUnitSkill studentSkillInfo = CUnit::GetSkillByCode(&pUnit->Description, skillCode);
        studentSkillLevel = studentSkillInfo.skillLevel;

        // Check if already at max level
        if (studentSkillLevel >= minlevel && minlevel != -1)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Already knows the skill!";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }

        // Look for a teacher with higher skill level
        if (pLand)
        {
            for (int idx = 0; idx < pLand->UnitsSeq.Count(); idx++)
            {
                CUnit* pPotentialTeacher = (CUnit*)pLand->UnitsSeq.At(idx);

                if (pPotentialTeacher->Id != pUnit->Id && pPotentialTeacher->IsOurs)
                {
                    CStr teachOrders = pPotentialTeacher->GetOrders("TEACH");

                    if (teachOrders.IsEmpty())
                        continue;

                    CStr studentIdStr;
                    studentIdStr.Format("%ld", pUnit->Id);

                    if (teachOrders.FindSubStr(studentIdStr.GetData()) >= 0)
                    {
                        TUnitSkill teacherSkillInfo = CUnit::GetSkillByCode(&pPotentialTeacher->Description, skillCode);
                        long teacherLevel = teacherSkillInfo.skillLevel;

                        if (teacherLevel > studentSkillLevel)
                        {
                            hasTeacher = TRUE;
                            pTeacher = pPotentialTeacher;
                            teacherSkillLevel = teacherLevel;
                            break;
                        }
                    }
                }
            }
        }

        // Deduct study cost
        pTempValue = nullptr;
        if (!pUnit->GetProperty(PRP_SILVER, type, pTempValue, eNormal))
        {
            unitmoney = 0;
            if (PE_OK != pUnit->SetProperty(PRP_SILVER, eLong,
                reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eBoth))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOSETUNIT << pUnit->Id << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                continue;
            }
        }
        else if (eLong != type)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - " << NOTNUMERIC << pUnit->Id << BUG;
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            unitmoney = 0;
        }
        else if (pTempValue)
            unitmoney = static_cast<long>(reinterpret_cast<intptr_t>(pTempValue));
        else
            unitmoney = 0;

        unitmoney -= n1 * n2;

        if (PE_OK != pUnit->SetProperty(PRP_SILVER, eLong,
            reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eNormal))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - " << NOSET << BUG;
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }

        pUnit->StudyingSkill = SkillNaked;
        pUnit->StudyingSkill << PRP_SKILL_POSTFIX;

        // Add teacher comment if applicable
        if (hasTeacher && pTeacher)
        {
            CStr teacherNameWithId;
            teacherNameWithId.Format("%s(%ld)", pTeacher->Name.GetData(), pTeacher->Id);

            if (commentsPart.IsEmpty())
            {
                commentsPart.Format("teached by %s (%s %ld)",
                    teacherNameWithId.GetData(),
                    SkillNaked.GetData(),
                    teacherSkillLevel);
            }
            else
            {
                CStr additionalComment;
                additionalComment.Format(", teached by %s (%s %ld)",
                    teacherNameWithId.GetData(),
                    SkillNaked.GetData(),
                    teacherSkillLevel);
                commentsPart << additionalComment.GetData();
            }
        }

        // Update end turn description
        if (!pUnit->m_EndTurnDescription.IsEmpty())
        {
            long studyDays = hasTeacher ? 60 : 30;
            wxString skillCode = wxString::FromUTF8(SkillNaked.GetData()).Upper();
            pUnit->AidSkillDays(&pUnit->m_EndTurnDescription, skillCode, studyDays);
        }

    } while (FALSE);

    // Rebuild line with comments if changed
    int commentPos = Line.FindSubStr(";");
    if (commentPos >= 0)
    {
        for (int i = 0; i < commentPos; i++)
            commandPart.AddCh(Line.GetData()[i]);
        commandPart.TrimRight(TRIM_ALL);
    }
    else
    {
        commandPart = Line;
        commandPart.TrimRight(TRIM_ALL);
    }

    CStr newLine = commandPart;
    if (!commentsPart.IsEmpty())
        newLine << " ; " << commentsPart;

    if (strcmp(originalLine.GetData(), newLine.GetData()) != 0)
    {
        Line = newLine;

        CStr updatedOrders;
        const char* ordersPtr = pUnit->Orders.GetData();
        CStr currentLine;
        BOOL lineReplaced = FALSE;

        wxString targetCmd = wxString::FromUTF8(commandPart.GetData()).Upper().Trim();

        while (ordersPtr && *ordersPtr)
        {
            ordersPtr = currentLine.GetToken(ordersPtr, '\n', TRIM_ALL);
            wxString thisLine = wxString::FromUTF8(currentLine.GetData()).Trim();
            wxString thisCmd = thisLine.BeforeFirst(';').Trim().Upper();

            if (!lineReplaced && thisCmd == targetCmd)
            {
                if (!updatedOrders.IsEmpty())
                    updatedOrders << EOL_SCR;
                updatedOrders << newLine;
                lineReplaced = TRUE;
            }
            else
            {
                if (!updatedOrders.IsEmpty())
                    updatedOrders << EOL_SCR;
                updatedOrders << currentLine;
            }
        }

        if (lineReplaced)
            pUnit->Orders = updatedOrders;
    }
}

//-------------------------------------------------------------
// NAME command processing
// Sets unit name
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Name(CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand *, const char * params)
{
    CStr  What, Name, NewName;

    params = SkipSpaces(What.GetToken(params, ' ', TRIM_ALL));
    params = Name.GetToken(params, ' ', TRIM_ALL);

    if (0==stricmp(What.GetData(), "unit"))
    {
        if (IS_NEW_UNIT(pUnit))
        {
            NewName << Name << " - " << "NEW " << (long)REVERSE_NEW_UNIT_ID(pUnit->Id);
            Name = NewName;
        }
        pUnit->SetName(Name.GetData());
        if (!pUnit->m_EndTurnDescription.IsEmpty())
            pUnit->SetEndTurnUnitName(&pUnit->m_EndTurnDescription, wxString::FromUTF8(Name.GetData()));
    }
}

//-------------------------------------------------------------
// Resource availability check for production
//-------------------------------------------------------------

BOOL CAtlaParser::CheckResourcesForProduction(CUnit * pUnit, CLand * pLand, CStr & Error)
{
    BOOL                Ok = TRUE;
    unsigned int        x;
    int                 i;
    TProdDetails        details;
    CUnit             * pSharer;
    const char        * propname;
    BOOL                SharerFound;
    EValueType          type;
    long                nlvl  = 0;
    long                ntool = 0;
    long                ncanproduce = 0;
    long                nres  = 0;
    long                nrequired = 0;
    long                nmen  = 0;
    int                 no;
    CStr                S;

    Error.Empty();
    if ((pUnit->Flags & UNIT_FLAG_PRODUCING) && !pUnit->ProducingItem.IsEmpty())
    {
        gpDataHelper->GetProdDetails (pUnit->ProducingItem.GetData(), details);

        // Check men
        const void* pMenValue = nullptr;
        if (!pUnit->GetProperty(PRP_MEN, type, pMenValue, eNormal)  || !pMenValue)
        {
            Error << " - There are no men in the unit!";
            Ok = FALSE;
        }
        else
        {
            nmen = static_cast<long>(reinterpret_cast<intptr_t>(pMenValue));
            if (nmen<=0)
            {
                Error << " - There are no men in the unit!";
                Ok = FALSE;
            }
        }

        // Check production configuration
        if (details.skillname.IsEmpty() || details.months<=0)
        {
            Error << " - Production requirements for item '" << pUnit->ProducingItem << "' are not configured! ";
            Ok = FALSE;
        }

        // Check skill level
        S << details.skillname << PRP_SKILL_POSTFIX;
        const void* pSkillValue = nullptr;
        if (pUnit->GetProperty(S.GetData(), type, pSkillValue, eNormal) && (eLong==type) )
        {
            nlvl = static_cast<long>(reinterpret_cast<intptr_t>(pSkillValue));
            if (nlvl < details.skilllevel)
            {
                Error << " - Skill " << details.skillname << " level " << details.skilllevel << " is required for production";
                Ok = FALSE;
            }
        }
        else
        {
            Error << " - Skill " << details.skillname << " is required for production";
            Ok = FALSE;
        }

        // Check tools
        if (!details.toolname.IsEmpty())
        {
            const void* pToolValue = nullptr;
            if (!pUnit->GetProperty(details.toolname.GetData(), type, pToolValue, eNormal) || eLong!=type )
                ntool = 0;
            else
                ntool = static_cast<long>(reinterpret_cast<intptr_t>(pToolValue));
        }
        if (ntool > nmen)
            ntool = nmen;

        ncanproduce = (long)((((double)nmen)*nlvl + ntool*details.toolhelp) / details.months);

        // Check each required resource
        for (x=0; x<sizeof(details.resname)/sizeof(*details.resname); x++)
        {
            SharerFound = FALSE;
            if (!details.resname[x].IsEmpty())
            {
                const void* pResValue = nullptr;
                if (!pUnit->GetProperty(details.resname[x].GetData(), type, pResValue, eNormal) || eLong!=type )
                    nres = 0;
                else
                    nres = static_cast<long>(reinterpret_cast<intptr_t>(pResValue));

                nrequired = (ncanproduce * details.resamt[x] );

                if (nrequired > nres)
                {
                    // Check for sharing units
                    for (i=0; i<pLand->Units.Count(); i++)
                    {
                        pSharer = (CUnit*)pLand->Units.At(i);
                        if (pSharer->FactionId == pUnit->FactionId && (pSharer->Flags & UNIT_FLAG_SHARING))
                        {
                            no = 0;
                            propname = pSharer->GetPropertyName(no);
                            while (propname)
                            {
                                if (0==stricmp(propname, details.resname[x].GetData()))
                                {
                                    SharerFound = TRUE;
                                    break;
                                }
                                propname = pSharer->GetPropertyName(++no);
                            }
                        }
                    }

                    if (!SharerFound)
                    {
                        if (0==nres)
                            Error << " - " << details.resname[x] << " needed for production!";
                        else
                            Error << " - " << (nrequired - nres) << " more units of " << details.resname[x] << " needed for production at full capacity (" << ncanproduce << ")!";
                        Ok = FALSE;
                    }
                }
            }
        }
    }

    return Ok;
}

//-------------------------------------------------------------
// PRODUCE command processing
// Handles item production with resource consumption
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Produce(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params)
{
    TProdDetails        details;
    const void* value = nullptr;
    EValueType          type;
    long                nlvl = 0;
    long                nmen = 0;
    long                ntool = 0;
    long                nproduce = 0;
    long                maxProduce = 0;
    long                availableResource = 0;
    CStr                S(32), Product(32), Error;
    std::map<wxString, long> resourceRequirements;
    BOOL                wasProducing = (pUnit->Flags & UNIT_FLAG_PRODUCING) != 0;

    do
    {
        params = ReadPropertyName(params, Product);
        pUnit->Flags |= UNIT_FLAG_PRODUCING;
        pUnit->ProducingItem = gpDataHelper->ResolveAlias(Product.GetData());
        Product = pUnit->ProducingItem;
        gpDataHelper->GetProdDetails(Product.GetData(), details);

        // Get men count
        value = nullptr;
        if (!pUnit->GetProperty(PRP_MEN, type, value, eNormal) || !value)
        {
            nmen = 0;
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - There are no men in the unit!";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }
        else if (eLong != type)
        {
            nmen = 0;
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid men property type!";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }
        else
        {
            nmen = static_cast<long>(reinterpret_cast<intptr_t>(value));
            if (nmen <= 0)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - There are no men in the unit!";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                continue;
            }
        }

        // Validate production configuration
        if (details.skillname.IsEmpty() || details.months <= 0)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Production requirements for item '" << Product << "' are not configured! ";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }

        // Check skill level
        S << details.skillname << PRP_SKILL_POSTFIX;
        value = nullptr;
        if (pUnit->GetProperty(S.GetData(), type, value, eNormal) && (eLong == type) && value)
        {
            nlvl = static_cast<long>(reinterpret_cast<intptr_t>(value));
            if (nlvl < details.skilllevel)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Skill " << details.skillname << " level " << details.skilllevel << " is required for production";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                continue;
            }
        }
        else
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Skill " << details.skillname << " is required for production";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            continue;
        }

        // Get tool count
        if (!details.toolname.IsEmpty())
        {
            value = nullptr;
            if (pUnit->GetProperty(details.toolname.GetData(), type, value, eNormal) && eLong == type && value)
                ntool = static_cast<long>(reinterpret_cast<intptr_t>(value));
            else
                ntool = 0;
        }

        if (ntool > nmen)
            ntool = nmen;

        // Calculate production capacity by skill
        long maxBySkill = (long)((((double)nmen) * nlvl + ntool * details.toolhelp) / details.months);
        if (maxBySkill <= 0)
            maxBySkill = 1;

        // Check regional limit from Products line
        long regionalLimit = LONG_MAX;
        wxString productsStr = wxString::FromUTF8(pLand->Description.GetData());
        int pos = productsStr.Find("Products:");
        if (pos != wxNOT_FOUND)
        {
            wxString productsPart = productsStr.Mid(pos + 9);
            wxStringTokenizer tokenizer(productsPart, ",");

            while (tokenizer.HasMoreTokens())
            {
                wxString token = tokenizer.GetNextToken().Trim(false).Trim();
                int startBracket = token.Find('[');
                int endBracket = token.Find(']');

                if (startBracket != wxNOT_FOUND && endBracket != wxNOT_FOUND && endBracket > startBracket)
                {
                    wxString itemCode = token.Mid(startBracket + 1, endBracket - startBracket - 1).Upper().Trim();

                    if (itemCode == Product.GetData())
                    {
                        wxString beforeBracket = token.Left(startBracket).Trim();
                        wxStringTokenizer numTokenizer(beforeBracket, " \t");
                        wxString lastToken;

                        while (numTokenizer.HasMoreTokens())
                            lastToken = numTokenizer.GetNextToken();

                        long amount = 0;
                        if (lastToken.ToLong(&amount))
                            regionalLimit = amount;

                        break;
                    }
                }
            }
        }

        maxProduce = (maxBySkill < regionalLimit) ? maxBySkill : regionalLimit;

        // Check resource availability
        long produceAmount = maxProduce;
        std::map<wxString, long> resourceRequirements;

        for (int x = 0; x < MAX_RES_NUM && !details.resname[x].IsEmpty(); x++)
        {
            long nres = 0;
            value = nullptr;
            if (pUnit->GetProperty(details.resname[x].GetData(), type, value, eNormal) && eLong == type && value)
                nres = static_cast<long>(reinterpret_cast<intptr_t>(value));

            long required = maxProduce * details.resamt[x];
            resourceRequirements[wxString::FromUTF8(details.resname[x].GetData())] = required;

            long possibleWithThisResource = nres / details.resamt[x];
            if (possibleWithThisResource < produceAmount)
                produceAmount = possibleWithThisResource;
        }

        // Initialize end turn description if needed
        if (pUnit->m_EndTurnDescription.IsEmpty())
            pUnit->m_EndTurnDescription = pUnit->Description;

        // Generate production comment
        CStr produceComment;

        if (produceAmount <= 0)
            produceComment << " cannot produce due to lack of resources";
        else if (produceAmount < maxProduce)
        {
            wxString missingResources;
            for (int x = 0; x < MAX_RES_NUM && !details.resname[x].IsEmpty(); x++)
            {
                long nres = 0;
                value = nullptr;
                if (pUnit->GetProperty(details.resname[x].GetData(), type, value, eNormal) && eLong == type && value)
                    nres = static_cast<long>(reinterpret_cast<intptr_t>(value));

                long neededForMax = maxProduce * details.resamt[x];
                if (nres < neededForMax)
                {
                    long needed = neededForMax - nres;
                    if (!missingResources.IsEmpty())
                        missingResources += ", ";
                    missingResources += wxString::Format(wxT("%ld %s"), needed,
                        wxString::FromUTF8(details.resname[x].GetData()));
                }
            }

            produceComment << " can produce " << (long)produceAmount
                << " " << Product.GetData()
                << ", but need " << missingResources.ToUTF8().data() << " yet";
        }
        else
        {
            if (details.resname[0].IsEmpty())
                produceComment << " can produce " << (long)maxProduce << " " << Product.GetData();
            else
                produceComment << " can produce " << (long)produceAmount
                    << " " << Product.GetData() << " from available resources";
        }

        // Update order line with comment
        wxString orderLine = wxString::FromUTF8(Line.GetData());
        int commentPos = orderLine.Find(';');

        wxString commandPart;
        if (commentPos != wxNOT_FOUND)
            commandPart = orderLine.Left(commentPos).Trim(false).Trim();
        else
            commandPart = orderLine.Trim(false).Trim();

        CStr newLine;
        newLine << commandPart.ToUTF8().data() << " ;" << produceComment.GetData();
        Line = newLine;

        // Update unit's orders
        CStr updatedOrders;
        const char* ordersPtr = pUnit->Orders.GetData();
        CStr currentLine;
        BOOL lineReplaced = FALSE;
        wxString targetCmd = commandPart.Upper().Trim(false).Trim();

        while (ordersPtr && *ordersPtr)
        {
            ordersPtr = currentLine.GetToken(ordersPtr, '\n', TRIM_ALL);
            wxString thisLine = wxString::FromUTF8(currentLine.GetData()).Trim(false).Trim();
            wxString thisCmd = thisLine.BeforeFirst(';').Trim(false).Trim().Upper();

            if (!lineReplaced && thisCmd == targetCmd)
            {
                if (!updatedOrders.IsEmpty())
                    updatedOrders << EOL_SCR;
                updatedOrders << newLine;
                lineReplaced = TRUE;
            }
            else
            {
                if (!updatedOrders.IsEmpty())
                    updatedOrders << EOL_SCR;
                updatedOrders << currentLine;
            }
        }

        if (lineReplaced)
            pUnit->Orders = updatedOrders;

        // Update end turn description
        if (!pUnit->m_EndTurnDescription.IsEmpty())
        {
            for (int x = 0; x < MAX_RES_NUM && !details.resname[x].IsEmpty(); x++)
            {
                long usedAmount = produceAmount * details.resamt[x];
                if (usedAmount > 0)
                {
                    pUnit->AidItem(&pUnit->m_EndTurnDescription,
                        wxString::FromUTF8(details.resname[x].GetData()).Upper(),
                        -usedAmount);
                }
            }

            if (produceAmount > 0)
            {
                pUnit->AidItem(&pUnit->m_EndTurnDescription,
                    wxString::FromUTF8(Product.GetData()).Upper(),
                    produceAmount);
            }
        }

        // Check resources if immediate check enabled
        if (gpDataHelper->ImmediateProdCheck())
        {
            if (!CheckResourcesForProduction(pUnit, pLand, Error))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << Error.GetData();
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                continue;
            }
        }

    } while (FALSE);
}

//-------------------------------------------------------------
// Helper for parsing amount and item in GIVE/TAKE/TRANSPORT
//-------------------------------------------------------------

BOOL CAtlaParser::GetItemAndAmountForGive(CStr& Line, CStr& ErrorLine, BOOL skiperror,
    CUnit* pUnit, CLand* pLand, const char* params,
    CStr& Item, int& amount, const char* command,
    CUnit* pUnit2)
{
    BOOL                Ok = FALSE;
    CStr                S1(32);
    char                ch;
    long                item_avail = 0;
    EValueType          type;
    const void* pItemValue = nullptr;

    do
    {
        Item.Empty();
        amount = 0;

        params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));
        params = SkipSpaces(Item.GetToken(params, " \t", ch, TRIM_ALL));

        if (0 != stricmp("UNIT", S1.GetData()))
        {
            if (0 == stricmp("TAKE", command))
            {
                // For TAKE, check target unit's inventory
                pItemValue = nullptr;
                pUnit2->GetProperty(Item.GetData(), type, pItemValue, eNormal);

                if (pItemValue && type == eLong)
                    item_avail = static_cast<long>(reinterpret_cast<intptr_t>(pItemValue));
                else
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Can not take that!";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
            }
            else
            {
                // For GIVE, check source unit's inventory
                pItemValue = nullptr;
                BOOL hasProperty = pUnit->GetProperty(Item.GetData(), type, pItemValue, eNormal);

                if (hasProperty && pItemValue && type == eLong)
                    item_avail = static_cast<long>(reinterpret_cast<intptr_t>(pItemValue));
                else
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Can not " << command << " that!";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
            }
        }

        if (0 == stricmp("UNIT", S1.GetData()))
            Item = S1;
        else if (0 == stricmp("ALL", S1.GetData()))
        {
            // ALL [EXCEPT N] handling
            if (item_avail > INT_MAX || item_avail < INT_MIN)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Value too large for amount";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
            amount = static_cast<int>(item_avail);

            params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));
            if (0 == stricmp("EXCEPT", S1.GetData()))
            {
                params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));

                const char* exceptStr = S1.GetData();
                if (!exceptStr || !*exceptStr)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Missing number after EXCEPT";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }

                char* endPtr = nullptr;
                long exceptAmount = strtol(exceptStr, &endPtr, 10);

                if (endPtr == exceptStr || *endPtr != '\0')
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Invalid number in EXCEPT clause";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }

                if (item_avail < exceptAmount)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - EXCEPT is too big. Use no more than " << item_avail << ".";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }

                long newAmount = item_avail - exceptAmount;
                if (newAmount > INT_MAX || newAmount < INT_MIN)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Resulting value too large";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
                amount = static_cast<int>(newAmount);
            }
        }
        else
        {
            // Simple numeric amount
            const char* amountStr = S1.GetData();
            if (!amountStr || !*amountStr)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Missing amount";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            char* endPtr = nullptr;
            long parsedAmount = strtol(amountStr, &endPtr, 10);

            if (endPtr == amountStr || *endPtr != '\0')
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Invalid number";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            if (parsedAmount > INT_MAX || parsedAmount < INT_MIN)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Value too large for amount";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            amount = static_cast<int>(parsedAmount);
        }

        // Validate amount
        if (amount < 0)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Can not " << command << " negative amount " << static_cast<long>(amount);
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        if (item_avail < static_cast<long>(amount))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Too many. " << command << " " << item_avail << " at most.";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        Ok = TRUE;
    } while (FALSE);

    return Ok;
}

//-------------------------------------------------------------
// WITHDRAW command processing
// Adds/subtracts items (typically silver) from unit
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Withdraw(CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    EValueType          type;
    const void        * value;
    CStr                Item, N;
    int                 amount;
    char                ch;

    do
    {
        params = N.GetToken(SkipSpaces(params), " \t", ch, TRIM_ALL);
        params = Item.GetToken(params, " \t", ch, TRIM_ALL);

        amount = atol(N.GetData());

        // Get current value or initialize to 0
        if (!pUnit->GetProperty(Item.GetData(), type, value, eNormal) )
        {
            type  = eLong;
            long zero = 0;
            value = reinterpret_cast<const void*>(static_cast<intptr_t>(zero));
            if (PE_OK!=pUnit->SetProperty(Item.GetData(), type, value, eNormal))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOSETUNIT << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
        }
        else if (eLong!=type)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - " << NOTNUMERIC << BUG;
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        long currentValue = static_cast<long>(reinterpret_cast<intptr_t>(value));
        long newValue = currentValue + amount;
        if (PE_OK!=pUnit->SetProperty(Item.GetData(), type, reinterpret_cast<const void*>(static_cast<intptr_t>(newValue)), eNormal))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - " << NOSETUNIT << BUG;
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        pUnit->CalcWeightsAndMovement();

    } while (FALSE);
}

//-------------------------------------------------------------
// GIVE command processing
// Transfers items from current unit to target unit
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Give(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params, BOOL IgnoreMissingTarget)
{
    EValueType          type;
    long                n1;
    CBaseObject         Dummy;
    int                 idx;
    CUnit* pUnit2 = NULL;
    const void* value;
    const void* value2;
    CStr                Item;
    int                 amount;

    CStr                giverName;
    giverName = pUnit->Name.GetData();
    if (giverName.IsEmpty())
        giverName.Format("Unit %d", pUnit->Id);

    do
    {
        // Parse target unit ID
        if (!GetTargetUnitId(params, pUnit->FactionId, n1))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid unit id";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }
        if (n1 == pUnit->Id)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Giving to yourself";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }
        if (0 != n1)
        {
            Dummy.Id = n1;
            if (pLand->Units.Search(&Dummy, idx))
                pUnit2 = (CUnit*)pLand->Units.At(idx);
            else if (!IgnoreMissingTarget)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Can not locate target unit";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
        }

        // Parse amount and item
        if (GetItemAndAmountForGive(Line, ErrorLine, skiperror, pUnit, pLand, params, Item, amount, "give", NULL))
        {
            if (0 == stricmp("UNIT", Item.GetData()))
            {
                if (pUnit2 && pUnit->FactionId == pUnit2->FactionId)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Target unit belongs to the same faction";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                }
                break;
            }

            // Update sender's property
            if (!pUnit->GetProperty(Item.GetData(), type, value, eNormal) || (eLong != type))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Can not give " << Item;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            long currentValue = static_cast<long>(reinterpret_cast<intptr_t>(value));
            long newValue = currentValue - amount;

            if (PE_OK != pUnit->SetProperty(Item.GetData(), type, reinterpret_cast<const void*>(static_cast<intptr_t>(newValue)), eNormal))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOSET << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            // Update sender's end turn description
            if (!pUnit->m_EndTurnDescription.IsEmpty())
                pUnit->AidItem(&pUnit->m_EndTurnDescription, wxString::FromUTF8(Item.GetData()).Upper(), -amount);

            if (pUnit2)
            {
                // Update receiver's property
                if (!pUnit2->GetProperty(Item.GetData(), type, value2, eNormal))
                {
                    long zero = 0;
                    value2 = reinterpret_cast<const void*>(static_cast<intptr_t>(zero));
                    if (PE_OK != pUnit2->SetProperty(Item.GetData(), type, value2, eNormal))
                    {
                        if (!skiperror)
                        {
                            ErrorLine.Empty();
                            ErrorLine << Line << " - " << NOSETUNIT << n1 << BUG;
                            OrderErr(1, pUnit2->Id, ErrorLine.GetData(), pUnit2->Name.GetData(), pUnit2);
                        }
                        break;
                    }
                }
                else if (eLong != type)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOTNUMERIC << n1 << BUG;
                        OrderErr(1, pUnit2->Id, ErrorLine.GetData(), pUnit2->Name.GetData(), pUnit2);
                    }
                    break;
                }

                long currentValue2 = static_cast<long>(reinterpret_cast<intptr_t>(value2));
                long newValue2 = currentValue2 + amount;

                if (PE_OK != pUnit2->SetProperty(Item.GetData(), type, reinterpret_cast<const void*>(static_cast<intptr_t>(newValue2)), eNormal))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSET << BUG;
                        OrderErr(1, pUnit2->Id, ErrorLine.GetData(), pUnit2->Name.GetData(), pUnit2);
                    }
                    break;
                }

                // Update receiver's end turn description
                if (!pUnit2->m_EndTurnDescription.IsEmpty())
                    pUnit2->AidItem(&pUnit2->m_EndTurnDescription, wxString::FromUTF8(Item.GetData()).Upper(), amount);

                // Track received silver for special handling
                if (0 == stricmp(PRP_SILVER, Item.GetData()))
                    pUnit2->SilvRcvd += amount;

                // Add comment to receiver's orders
                AddOrUpdateReceivedComment(pUnit2, pUnit, Item.GetData(), amount);

                // Adjust skills if giving men
                AdjustSkillsAfterGivingMen(pUnit, pUnit2, Item, amount);

                // Recalculate weights for receiver
                pUnit2->CalcWeightsAndMovement();
            }

            // Recalculate weights for sender
            pUnit->CalcWeightsAndMovement();
        }

    } while (FALSE);
}

//-------------------------------------------------------------
// Helper: Finds target for SEND command
// Supports DIRECTION or UNIT specifications
//-------------------------------------------------------------

BOOL CAtlaParser::FindTargetsForSend(CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char *& params, CUnit *& pUnit2, CLand *& pLand2)
{
    BOOL                Ok = FALSE;
    CBaseObject         Dummy;
    int                 idx;
    long                target_id;
    int                 X, Y, Z, X2, Y2, Z2, ID;
    int                 i;
    CStr                S1  (32);
    char                ch;

    do
    {
        params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));
        if (0==stricmp("DIRECTION", S1.GetData()))
        {
            // DIRECTION <dir> [UNIT <id>]
            pUnit2 = NULL;
            params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));
            LandIdToCoord(pLand->Id, X, Y, Z);

            for (i=0; i< DirectionsCount; i++)
            {
                if (0==stricmp(S1.GetData(), Directions[i]))
                {
                    ExtrapolateLandCoord(X, Y, Z, i);
                    ID = LandCoordToId(X,Y, pLand->pPlane->Id);
                    pLand2 = GetLand(ID);
                    break;
                }
            }
            if (!pLand2)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Can not find land in given direction";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            if (0==strnicmp("UNIT", params, 4))
            {
                params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));
                if (!GetTargetUnitId(params, pUnit->FactionId, target_id))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Invalid unit Id";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
                if (target_id==pUnit->Id)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Giving to yourself";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
                if (0 == target_id)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Invalid target unit";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
                Dummy.Id = target_id;
                if (pLand2->Units.Search(&Dummy, idx))
                    pUnit2 = (CUnit*)m_Units.At(idx);
                if (!pUnit2)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Invalid target unit";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
            }
        }
        else if (0==stricmp("UNIT", S1.GetData()))
        {
            // UNIT <id> (neighboring hex)
            if (!GetTargetUnitId(params, pUnit->FactionId, target_id))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Invalid unit Id";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
            if (target_id==pUnit->Id)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Giving to yourself";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
            if (0 == target_id)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Invalid target unit";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            Dummy.Id = target_id;
            if (m_Units.Search(&Dummy, idx))
                pUnit2 = (CUnit*)m_Units.At(idx);
            if (!pUnit2)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Invalid target unit";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
            if (pUnit2)
                pLand2 = GetLand(pUnit2->LandId);
            if (pLand2)
            {
                // Check if in neighboring hex
                LandIdToCoord(pLand->Id, X, Y, Z);
                LandIdToCoord(pLand2->Id, X2, Y2, Z2);
                if (Z!=Z2)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Target land on different plane";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
                if (abs(Y-Y2)>2)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Target land is too far away";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
                if (abs(X-X2)>1)
                {
                    if (pLand->pPlane->Width > 0)
                    {
                        if (X2 < X)
                        {
                            Z  = X2;
                            X2 = X;
                            X  = Z;
                        }
                        X += pLand->pPlane->Width;
                        if (abs(X-X2)>1)
                        {
                            if (!skiperror)
                            {
                                ErrorLine.Empty();
                                ErrorLine << Line << " - Target land is too far away";
                                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                            }
                            break;
                        }
                    }
                    else
                    {
                        if (!skiperror)
                        {
                            ErrorLine.Empty();
                            ErrorLine << Line << " - Target land is too far away";
                            OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                        }
                        break;
                    }
                }
            }
        }
        else
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid SEND command";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        Ok = TRUE;
    } while (FALSE);

    return Ok;
}

//-------------------------------------------------------------
// TAKE command processing
// Transfers items from target unit to current unit
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Take(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params, BOOL IgnoreMissingTarget)
{
    EValueType          type;
    long                targetId;
    CBaseObject         dummy;
    int                 idx;
    CUnit* pTargetUnit = NULL;
    const void* value;
    const void* value2;
    CStr                item;
    int                 amount;
    char                ch;

    CStr                giverName;

    do
    {
        // Parse "FROM" keyword
        item.Empty();
        params = SkipSpaces(item.GetToken(params, " \t", ch, TRIM_ALL));

        if (0 != stricmp("FROM", item.GetData()))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid TARGET command";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        // Parse target unit ID
        if (!GetTargetUnitId(params, pUnit->FactionId, targetId))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid unit id";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        if (targetId == pUnit->Id)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Taking from yourself";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        // Find target unit
        if (0 != targetId)
        {
            dummy.Id = targetId;
            if (pLand->Units.Search(&dummy, idx))
                pTargetUnit = (CUnit*)pLand->Units.At(idx);
            else
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Can not locate target unit";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
        }
        else
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid unit id";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        // Check faction - can only take from same faction
        if (pTargetUnit && pUnit->FactionId != pTargetUnit->FactionId)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Target unit must belong to the same faction";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        if (pTargetUnit)
        {
            giverName = pTargetUnit->Name.GetData();
            if (giverName.IsEmpty())
                giverName.Format("Unit %d", pTargetUnit->Id);

            // Parse amount and item
            if (GetItemAndAmountForGive(Line, ErrorLine, skiperror, pUnit, pLand, params, item, amount, "take", pTargetUnit))
            {
                // Cannot take UNIT (people)
                if (0 == stricmp("UNIT", item.GetData()))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Taking unit is not allowed";
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }

                // Update giver's property
                if (!pTargetUnit->GetProperty(item.GetData(), type, value, eNormal) || (eLong != type))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - Can not take " << item;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }

                long currentValue = static_cast<long>(reinterpret_cast<intptr_t>(value));
                long newValue = currentValue - amount;

                if (PE_OK != pTargetUnit->SetProperty(item.GetData(), type,
                    reinterpret_cast<const void*>(static_cast<intptr_t>(newValue)), eNormal))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSET << BUG;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }

                // Update giver's end turn description
                if (!pTargetUnit->m_EndTurnDescription.IsEmpty())
                {
                    pTargetUnit->AidItem(&pTargetUnit->m_EndTurnDescription,
                        wxString::FromUTF8(item.GetData()).Upper(), -amount);
                }

                // Update taker's property
                if (!pUnit->GetProperty(item.GetData(), type, value2, eNormal))
                {
                    long zero = 0;
                    value2 = reinterpret_cast<const void*>(static_cast<intptr_t>(zero));
                    if (PE_OK != pUnit->SetProperty(item.GetData(), type, value2, eNormal))
                    {
                        if (!skiperror)
                        {
                            ErrorLine.Empty();
                            ErrorLine << Line << " - " << NOSETUNIT << targetId << BUG;
                            OrderErr(1, pTargetUnit->Id, ErrorLine.GetData(),
                                pTargetUnit->Name.GetData(), pTargetUnit);
                        }
                        break;
                    }
                }
                else if (eLong != type)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOTNUMERIC << targetId << BUG;
                        OrderErr(1, pTargetUnit->Id, ErrorLine.GetData(),
                            pTargetUnit->Name.GetData(), pTargetUnit);
                    }
                    break;
                }

                long currentValue2 = static_cast<long>(reinterpret_cast<intptr_t>(value2));
                long newValue2 = currentValue2 + amount;

                if (PE_OK != pUnit->SetProperty(item.GetData(), type,
                    reinterpret_cast<const void*>(static_cast<intptr_t>(newValue2)), eNormal))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSET << BUG;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }

                // Special handling for silver
                if (0 == stricmp(PRP_SILVER, item.GetData()))
                {
                    pUnit->SilvRcvd += amount;
                    if (pTargetUnit->SilvRcvd > 0)
                        pTargetUnit->SilvRcvd = std::max(0L, pTargetUnit->SilvRcvd - amount);
                }

                // Update taker's end turn description
                if (!pUnit->m_EndTurnDescription.IsEmpty())
                {
                    pUnit->AidItem(&pUnit->m_EndTurnDescription,
                        wxString::FromUTF8(item.GetData()).Upper(), amount);
                }

                // Special handling for men
                if (gpDataHelper->IsMan(item.GetData()))
                    AdjustSkillsAfterGivingMen(pTargetUnit, pUnit, item, amount);

                // Add comment to taker's orders
                AddOrUpdateReceivedComment(pUnit, pTargetUnit, item.GetData(), amount);

                // Recalculate weights
                pUnit->CalcWeightsAndMovement();
                pTargetUnit->CalcWeightsAndMovement();
            }
        }

    } while (FALSE);
}

//-------------------------------------------------------------
// SEND command processing
// Sends items to another hex with cost calculation
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Send(CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params)
{
    CUnit             * pUnit2 = NULL;
    CLand             * pLand2 = NULL;
    CStr                Item;
    int                 amount, effweight;
    int                 price = 0;
    int                 WeatherMultiplier;
    long                unitmoney;
    EValueType          type;
    const void        * value;
    int               * weights;
    const char       ** movenames;
    int                 movecount;

    do
    {
        // Find target unit and land first
        if (!FindTargetsForSend(Line, ErrorLine, skiperror, pUnit, pLand, params, pUnit2, pLand2))
            return;

        if (!pLand2)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Unable to locate target hex";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            return;
        }

        // Parse amount and item
        if (GetItemAndAmountForGive(Line, ErrorLine, skiperror, pUnit, pLand, params, Item, amount, "send", NULL) )
        {
            if (!pUnit->GetProperty(Item.GetData(), type, value, eNormal) || (eLong!=type))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Can not send " << Item;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                return;
            }

            long currentValue = static_cast<long>(reinterpret_cast<intptr_t>(value));
            long newValue = currentValue - amount;
            
            if (PE_OK!=pUnit->SetProperty(Item.GetData(), type, reinterpret_cast<const void*>(static_cast<intptr_t>(newValue)), eNormal))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOSET << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                return;
            }

            // Calculate sending cost based on item weight and weather
            if ( gpDataHelper->GetItemWeights(Item.GetData(), weights, movenames, movecount) )
            {
                WeatherMultiplier = 2;
                if (pLand2 && pLand2->WeatherWillBeGood)
                    WeatherMultiplier = 1;

                effweight = weights[0] - weights[1];
                if (effweight<0)
                    effweight = 0;
                price  = (int) (amount * floor(sqrt((double)effweight)) * WeatherMultiplier);

                // Deduct cost in silver
                const void* pMoneyValue = nullptr;
                if (!pUnit->GetProperty(PRP_SILVER, type, pMoneyValue, eNormal) )
                {
                    unitmoney = 0;
                    if (PE_OK!=pUnit->SetProperty(PRP_SILVER, type, reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eBoth))
                    {
                        if (!skiperror)
                        {
                            ErrorLine.Empty();
                            ErrorLine << Line << " - " << NOSETUNIT << pUnit->Id << BUG;
                            OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                        }
                        return;
                    }
                }
                else if (eLong!=type)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOTNUMERIC << pUnit->Id << BUG;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    return;
                }
                else
                    unitmoney = static_cast<long>(reinterpret_cast<intptr_t>(pMoneyValue));

                unitmoney -= price;
                if  (PE_OK!=pUnit->SetProperty(PRP_SILVER,   type, reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eNormal))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSET << BUG;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    return;
                }
                pUnit->CalcWeightsAndMovement();
            }
        }

    } while (FALSE);
}

//-------------------------------------------------------------
// Adjust skills when giving men between units
// Distributes skill days proportionally
//-------------------------------------------------------------

void CAtlaParser::AdjustSkillsAfterGivingMen(CUnit* pUnitGive, CUnit* pUnitTake, CStr& item, long AmountGiven)
{
    int                 idx;
    const char* propname_days;
    CStringSortColl     SkillNames(32);
    int                 postlen;
    CStr                S, BasePropName, Prop, PropDays, PropStudy;
    CUnit* tmpunits[2] = { pUnitGive, pUnitTake };
    int                 i;
    char* p;
    long                dayssrc, daystarg, newdays, newskill, mentarg, newdaysexp, newskillexp;
    EValueType          type;
    BOOL                bWasSetGive, bWasSetTake;
    const void* value = nullptr;

    if (!gpDataHelper->IsMan(item.GetData()))
        return;

    // Get receiving unit's total men count
    value = nullptr;
    if (!pUnitTake->GetProperty(PRP_MEN, type, value, eNormal) || eLong != type || !value)
    {
        mentarg = 0;
        LOG_ERR(ERR_UNKNOWN, "There must be men in the unit!");
        return;
    }
    else
    {
        mentarg = static_cast<long>(reinterpret_cast<intptr_t>(value));
        if (mentarg <= 0)
        {
            LOG_ERR(ERR_UNKNOWN, "There must be men in the unit!");
            return;
        }
    }

    // Adjust leadership if giving to new unit
    const void* valueGive = nullptr;
    const void* valueTake = nullptr;

    if (pUnitGive->GetProperty(PRP_LEADER, type, valueGive, eNormal) && eCharPtr != type)
    {
        S = "Wrong property type ";
        CStr bugStr(32);
        bugStr = BUG;
        S << bugStr.GetData();
        OrderErr(1, pUnitGive->Id, S.GetData());
        return;
    }

    if (!pUnitTake->GetProperty(PRP_LEADER, type, valueTake, eNormal))
        pUnitTake->SetProperty(PRP_LEADER, eCharPtr, "", eBoth);
    else if (eCharPtr != type)
    {
        S = "Wrong property type ";
        CStr bugStr(32);
        bugStr = BUG;
        S << bugStr.GetData();
        OrderErr(1, pUnitTake->Id, S.GetData());
        return;
    }

    if (valueGive && !valueTake && 0 == mentarg - AmountGiven)
        pUnitTake->SetProperty(PRP_LEADER, eCharPtr, valueGive, eNormal);

    // Collect all skill properties from both units
    postlen = strlen(PRP_SKILL_DAYS_POSTFIX);
    for (i = 0; i < 2; i++)
    {
        idx = 0;
        propname_days = tmpunits[i]->GetPropertyName(idx);
        while (propname_days)
        {
            S = propname_days;
            if (S.FindSubStrR(PRP_SKILL_DAYS_POSTFIX) == S.GetLength() - postlen)
            {
                p = strdup(propname_days);
                if (!SkillNames.Insert(p))
                    free(p);
            }
            propname_days = tmpunits[i]->GetPropertyName(++idx);
        }
    }

    // Adjust each skill proportionally
    for (i = 0; i < SkillNames.Count(); i++)
    {
        propname_days = (const char*)SkillNames.At(i);
        BasePropName = propname_days;
        BasePropName.DelSubStr(BasePropName.GetLength() - postlen, postlen);

        // Regular skill properties
        Prop = BasePropName;
        Prop << PRP_SKILL_POSTFIX;
        PropDays = BasePropName;
        PropDays << PRP_SKILL_DAYS_POSTFIX;

        value = nullptr;
        if (!pUnitGive->GetProperty(PropDays.GetData(), type, value, eNormal) || !value || eLong != type)
            dayssrc = 0;
        else
            dayssrc = static_cast<long>(reinterpret_cast<intptr_t>(value));

        value = nullptr;
        if (!pUnitTake->GetProperty(PropDays.GetData(), type, value, eNormal) || !value || eLong != type)
        {
            pUnitTake->SetProperty(PropDays.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(0L)), eNormal);
            pUnitTake->SetProperty(Prop.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(0L)), eNormal);
            daystarg = 0;
        }
        else
            daystarg = static_cast<long>(reinterpret_cast<intptr_t>(value));

        // Weighted average of skill days
        newdays = (daystarg * (mentarg - AmountGiven) + dayssrc * AmountGiven) / mentarg;
        newskill = SkillDaysToLevel(newdays);

        pUnitTake->SetProperty(PropDays.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(newdays)), eNormal);
        pUnitTake->SetProperty(Prop.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(newskill)), eNormal);

        // Update end turn description
        if (pUnitTake->IsOurs && !pUnitTake->m_EndTurnDescription.IsEmpty())
        {
            wxString skillCode = wxString::FromUTF8(BasePropName.GetData());
            long daysChange = newdays - daystarg;
            if (daysChange != 0)
                pUnitTake->AidSkillDays(&pUnitTake->m_EndTurnDescription, skillCode, daysChange);
        }

        // Arcadia specific skill properties
        Prop = BasePropName;
        Prop << PRP_SKILL_EXPERIENCE_POSTFIX;
        PropDays = BasePropName;
        PropDays << PRP_SKILL_DAYS_EXPERIENCE_POSTFIX;
        PropStudy = BasePropName;
        PropStudy << PRP_SKILL_STUDY_POSTFIX;

        value = nullptr;
        bWasSetGive = pUnitGive->GetProperty(PropDays.GetData(), type, value, eNormal);
        if (bWasSetGive && value && eLong == type)
            dayssrc = static_cast<long>(reinterpret_cast<intptr_t>(value));
        else
            dayssrc = 0;

        value = nullptr;
        bWasSetTake = pUnitTake->GetProperty(PropDays.GetData(), type, value, eNormal);
        if (bWasSetTake && value && eLong == type)
            daystarg = static_cast<long>(reinterpret_cast<intptr_t>(value));
        else
            daystarg = 0;

        if (bWasSetGive || bWasSetTake)
        {
            // Arcadia skill handling
            PropStudy = BasePropName;
            PropStudy << PRP_SKILL_STUDY_POSTFIX;

            if (!bWasSetTake)
            {
                pUnitTake->SetProperty(PropDays.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(0L)), eNormal);
                pUnitTake->SetProperty(Prop.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(0L)), eNormal);
                pUnitTake->SetProperty(PropStudy.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(0L)), eNormal);
                daystarg = 0;
            }

            pUnitTake->SetProperty(PropStudy.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(newskill)), eNormal);

            newdaysexp = (daystarg * (mentarg - AmountGiven) + dayssrc * AmountGiven) / mentarg;
            newskillexp = SkillDaysToLevel(newdaysexp);

            pUnitTake->SetProperty(PropDays.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(newdaysexp)), eNormal);
            pUnitTake->SetProperty(Prop.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(newskillexp)), eNormal);

            newskill = newskill + newskillexp;
            Prop = BasePropName;
            Prop << PRP_SKILL_POSTFIX;
            pUnitTake->SetProperty(Prop.GetData(), eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(newskill)), eNormal);

            // Update end turn description for Arcadia skills
            if (pUnitTake->IsOurs && !pUnitTake->m_EndTurnDescription.IsEmpty())
            {
                wxString skillCode = wxString::FromUTF8(BasePropName.GetData());
                long daysChange = newdaysexp - daystarg;
                if (daysChange != 0)
                    pUnitTake->AidSkillDays(&pUnitTake->m_EndTurnDescription, skillCode, daysChange);
            }
        }
    }

    SkillNames.FreeAll();
}

//-------------------------------------------------------------
// BUY command processing
// Purchases items from region using silver
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Buy(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params)
{
    EValueType          type;
    CStr                S1(32);
    CStr                N1(32);
    char                ch;
    long                n1 = 0;
    long                peritem = 0;
    long                unitmoney = 0;
    long                unitprop = 0;
    CStr                LandProp(32);
    long                landprop = 0;
    CUnit               DummyGiver;
    const void* value = nullptr;

    do
    {
        // Parse amount and item
        params = SkipSpaces(N1.GetToken(params, " \t", ch, TRIM_ALL));
        params = ReadPropertyName(params, S1);
        n1 = atol(N1.GetData());
        if (S1.IsEmpty() || (n1 <= 0 && 0 != stricmp(N1.GetData(), "ALL")))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid BUY command";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        // Handle peasant aliases
        if (0 == stricmp(S1.GetData(), "peas") || 0 == stricmp(S1.GetData(), "peasant")
            || 0 == stricmp(S1.GetData(), "peasants"))
            if (!pLand->PeasantRace.IsEmpty())
                ReadPropertyName(pLand->PeasantRace.GetData(), S1);

        // Get price per item from land
        MakeQualifiedPropertyName(PRP_SALE_PRICE_PREFIX, S1.GetData(), LandProp);

        value = nullptr;
        if (pLand->GetProperty(LandProp.GetData(), type, value, eNormal) && (eLong == type) && value)
            peritem = static_cast<long>(reinterpret_cast<intptr_t>(value));
        else
            peritem = 0;

        if (peritem > 0)
        {
            // Handle "ALL" amount
            if (0 == stricmp(N1.GetData(), "ALL"))
            {
                MakeQualifiedPropertyName(PRP_SALE_AMOUNT_PREFIX, S1.GetData(), LandProp);

                value = nullptr;
                if (!pLand->GetProperty(LandProp.GetData(), type, value, eNormal) || (eLong != type) || !value)
                    n1 = 0;
                else
                    n1 = static_cast<long>(reinterpret_cast<intptr_t>(value));
            }

            // Get unit's current amount of this item
            value = nullptr;
            if (!pUnit->GetProperty(S1.GetData(), type, value, eNormal) || !value)
            {
                unitprop = 0;
                if (PE_OK != pUnit->SetProperty(S1.GetData(), eLong,
                    reinterpret_cast<const void*>(static_cast<intptr_t>(unitprop)), eBoth))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSETUNIT << pUnit->Id << BUG;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
            }
            else if (eLong != type)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOTNUMERIC << pUnit->Id << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                unitprop = 0;
            }
            else
                unitprop = static_cast<long>(reinterpret_cast<intptr_t>(value));

            // Get unit's silver
            value = nullptr;
            if (!pUnit->GetProperty(PRP_SILVER, type, value, eNormal) || !value)
            {
                unitmoney = 0;
                if (PE_OK != pUnit->SetProperty(PRP_SILVER, eLong,
                    reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eBoth))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSETUNIT << pUnit->Id << BUG;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
            }
            else if (eLong != type)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOTNUMERIC << pUnit->Id << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                unitmoney = 0;
            }
            else
                unitmoney = static_cast<long>(reinterpret_cast<intptr_t>(value));

            // Check available amount in region
            MakeQualifiedPropertyName(PRP_SALE_AMOUNT_PREFIX, S1.GetData(), LandProp);

            value = nullptr;
            if (!pLand->GetProperty(LandProp.GetData(), type, value, eNormal) || (eLong != type) || !value)
                landprop = 0;
            else
                landprop = static_cast<long>(reinterpret_cast<intptr_t>(value));

            if (n1 > landprop)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - That is too MANY! Buy " << landprop << " at max.";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                n1 = landprop;
            }

            // Perform transaction
            unitmoney -= n1 * peritem;
            unitprop += n1;
            landprop -= n1;

            if (unitmoney < 0)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Not enough silver!";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                unitmoney += n1 * peritem;
                unitprop -= n1;
                landprop += n1;
                break;
            }

            BOOL ok1 = (PE_OK == pUnit->SetProperty(S1.GetData(), eLong,
                reinterpret_cast<const void*>(static_cast<intptr_t>(unitprop)), eNormal));

            BOOL ok2 = (PE_OK == pUnit->SetProperty(PRP_SILVER, eLong,
                reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eNormal));

            BOOL ok3 = (PE_OK == pLand->SetProperty(LandProp.GetData(), eLong,
                reinterpret_cast<const void*>(static_cast<intptr_t>(landprop)), eNormal));

            if (!ok1 || !ok2 || !ok3)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOSET << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            // Update end turn description
            if (pUnit->m_EndTurnDescription.IsEmpty())
                pUnit->m_EndTurnDescription = "";

            pUnit->AidItem(&pUnit->m_EndTurnDescription, "SILV", -(n1 * peritem));
            pUnit->AidItem(&pUnit->m_EndTurnDescription, S1.ToUpper(), n1);

            // Recalculate skills if men were bought
            CStr groupList = gpDataHelper->GetConfString("UNIT_PROPERTY_GROUPS", "men");
            if (groupList.FindSubStr(S1.GetSafeCStr()) > 0)
                pUnit->RecalcSkills(n1, NULL);

            if (gpDataHelper->IsTradeItem(S1.GetData()))
                pUnit->Flags |= UNIT_FLAG_PRODUCING;

            pUnit->CalcWeightsAndMovement();

            AdjustSkillsAfterGivingMen(&DummyGiver, pUnit, S1, n1);

            if (gpDataHelper->IsMan(S1.GetData()))
            {
                if (S1.FindSubStr("LEAD") >= 0)
                    SetUnitProperty(pUnit, PRP_LEADER, eCharPtr, SZ_LEADER, eNormal);
            }
        }
        else
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Can not BUY that!";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
        }
    } while (FALSE);
}

//-------------------------------------------------------------
// SELL command processing
// Sells items to region for silver
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Sell(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params)
{
    EValueType          type;
    CStr                S1(32);
    CStr                N1(32);
    char                ch;
    long                n1 = 0;
    long                peritem = 0;
    long                unitmoney = 0;
    long                unitprop = 0;
    CStr                LandProp(32);
    long                landprop = 0;

    do
    {
        // Parse amount and item
        params = SkipSpaces(N1.GetToken(params, " \t", ch, TRIM_ALL));
        params = ReadPropertyName(params, S1);
        n1 = atol(N1.GetData());
        
        if (S1.IsEmpty() || (n1 <= 0 && 0 != stricmp(N1.GetData(), "ALL")))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid SELL command";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        // Get price per item from land
        MakeQualifiedPropertyName(PRP_WANTED_PRICE_PREFIX, S1.GetData(), LandProp);

        const void* value = nullptr;
        if (pLand->GetProperty(LandProp.GetData(), type, value, eNormal) && (eLong == type) && value)
            peritem = static_cast<long>(reinterpret_cast<intptr_t>(value));
        else
            peritem = 0;

        if (peritem > 0)
        {
            // Handle "ALL" amount
            if (0 == stricmp(N1.GetData(), "ALL"))
            {
                MakeQualifiedPropertyName(PRP_WANTED_AMOUNT_PREFIX, S1.GetData(), LandProp);

                value = nullptr;
                if (!pLand->GetProperty(LandProp.GetData(), type, value, eNormal) || (eLong != type) || !value)
                    n1 = 0;
                else
                    n1 = static_cast<long>(reinterpret_cast<intptr_t>(value));
            }

            // Get unit's current amount of this item
            value = nullptr;
            if (!pUnit->GetProperty(S1.GetData(), type, value, eNormal) || !value)
                unitprop = 0;
            else if (eLong != type)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOTNUMERIC << pUnit->Id << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                unitprop = 0;
            }
            else
                unitprop = static_cast<long>(reinterpret_cast<intptr_t>(value));

            // Get unit's silver
            value = nullptr;
            if (!pUnit->GetProperty(PRP_SILVER, type, value, eNormal) || !value)
            {
                unitmoney = 0;
                if (PE_OK != pUnit->SetProperty(PRP_SILVER, eLong,
                    reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eBoth))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSETUNIT << pUnit->Id << BUG;
                        OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                    }
                    break;
                }
            }
            else if (eLong != type)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOTNUMERIC << pUnit->Id << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                unitmoney = 0;
            }
            else
                unitmoney = static_cast<long>(reinterpret_cast<intptr_t>(value));

            // Check available amount in region
            MakeQualifiedPropertyName(PRP_WANTED_AMOUNT_PREFIX, S1.GetData(), LandProp);

            value = nullptr;
            if (!pLand->GetProperty(LandProp.GetData(), type, value, eNormal) || (eLong != type) || !value)
                landprop = 0;
            else
                landprop = static_cast<long>(reinterpret_cast<intptr_t>(value));

            if (n1 > unitprop)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - That is too MANY! Sell " << unitprop << " at max.";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                n1 = unitprop;
            }

            if (n1 > landprop)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Region wants only " << landprop << "!";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                n1 = landprop;
            }

            // Perform transaction
            unitmoney += n1 * peritem;
            unitprop -= n1;
            landprop -= n1;

            BOOL ok1 = (PE_OK == pUnit->SetProperty(S1.GetData(), eLong,
                reinterpret_cast<const void*>(static_cast<intptr_t>(unitprop)), eNormal));

            BOOL ok2 = (PE_OK == pUnit->SetProperty(PRP_SILVER, eLong,
                reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), eNormal));

            BOOL ok3 = (PE_OK == pLand->SetProperty(LandProp.GetData(), eLong,
                reinterpret_cast<const void*>(static_cast<intptr_t>(landprop)), eNormal));

            if (!ok1 || !ok2 || !ok3)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOSET << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            // Update end turn description
            if (pUnit->m_EndTurnDescription.IsEmpty())
                pUnit->m_EndTurnDescription = "";

            pUnit->AidItem(&pUnit->m_EndTurnDescription, "SILV", n1 * peritem);
            pUnit->AidItem(&pUnit->m_EndTurnDescription, S1.ToUpper(), -n1);

            pUnit->CalcWeightsAndMovement();
        }
        else
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Can not SELL that!";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
        }
    } while (FALSE);
}

//--------------------------------------------------------------------------
// Order processing phases
//--------------------------------------------------------------------------

enum
{
    SQ_MIN,
    SQ_FORM,
    SQ_CLAIM,
    SQ_LEAVE,
    SQ_ENTER,
    SQ_PROMOTE,
    SQ_GIVE,
    SQ_TAX_PILLAGE,
    SQ_SELL,
    SQ_BUY,
    SQ_STUDY,
    SQ_TEACH,
    SQ_MOVE,
    SQ_WORK,
    SQ_TRANSPORT, 
    SQ_DISTRIBUTE,
    SQ_MAX
};

//--------------------------------------------------------------------------
// Main land orders processor
// Executes orders in phases to handle dependencies correctly
//--------------------------------------------------------------------------

void CAtlaParser::RunLandOrders(CLand* pLand, const char* sCheckTeach)
{
    CBaseObject         Dummy;
    CUnit* pUnit;
    CUnit* pUnitMaster;
    CUnit* pUnit2;
    CStruct* pStruct;
    CStr                Line(32);
    CStr                Cmd(32);
    CStr                S(32);
    CStr                S1(32);
    CStr                N1(32);
    CStr                ErrorLine(32);
    const char* p;
    const char* src;
    long                n1;
    long                unitmoney;
    int                 mainidx;
    int                 sequence;
    BOOL                errors = FALSE;
    int                 idx;
    EValueType          type;
    int                 X, Y, Z;    // current coordinates for moving unit
    int                 XN = 0, YN = 0;     // saved coords while new unit processed
    int                 LocA3, LocA3N = 0;
    char                ch;
    BOOL                TeachCheckGlb = (BOOL)atoi(sCheckTeach ? sCheckTeach : gpDataHelper->GetConfString(SZ_SECT_COMMON, SZ_KEY_CHECK_TEACH_LVL));
    BOOL                skiperror = false;
    int                 nNestingMode;
    long                order;
    wxString            destination;

    // Reset land and old units
    pLand->ResetNormalProperties();
    pLand->ResetUnitsAndStructs();

    // Track flags set in current phase to avoid duplicates
    std::map<long, unsigned long> flagsSetInPhase;

    // Execute orders in phases
    for (sequence = SQ_MIN + 1; sequence < SQ_MAX; sequence++)
    {
        flagsSetInPhase.clear();

        if (errors)
            break;

        if (SQ_TAX_PILLAGE == sequence)
        {
            if (pLand->Taxable > 0)
                RunOrder_TaxPillage(pLand);
            continue;
        }

        if (SQ_WORK == sequence)
        {
            if (m_EconomyWork)
            {
                RunOrder_Entertain(pLand);
                RunOrder_Work(pLand);
            }
        }

        for (mainidx = 0; mainidx < pLand->UnitsSeq.Count(); mainidx++)
        {
            nNestingMode = 0;
            pUnitMaster = NULL;
            bool isSimCmd = false;
            pUnit = (CUnit*)pLand->UnitsSeq.At(mainidx);

            flagsSetInPhase[pUnit->Id] = 0;

            LandIdToCoord(pLand->Id, X, Y, Z);
            LocA3 = NO_LOCATION;

            src = pUnit->Orders.GetData();
            while (src && *src)
            {
                src = Line.GetToken(src, '\n', TRIM_ALL);

                // Handle simulation commands (@;; prefix)
                p = strstr(Line.GetData(), "@;;");
                if (p && p == Line.GetData())
                {
                    isSimCmd = true;
                    Line.DelSubStr(0, 3);
                }
                else
                {
                    p = strstr(Line.GetData(), ";;");
                    if (p)
                    {
                        p += 2;
                        RunPseudoComment(Line, ErrorLine, skiperror, pUnit, pLand, p, sequence, destination);
                    }
                    isSimCmd = false;
                }

                while (Line.GetData()[0] == ' ')
                    Line.DelSubStr(0, 1);

                // Strip comments
                p = strchr(Line.GetData(), ';');
                if (p)
                {
                    S1 = SkipSpaces(++p);
                    S1.TrimRight(TRIM_ALL);
                    skiperror = (0 == stricmp(S1.GetData(), "ne") || 0 == stricmp(S1.GetData(), "$ne"));
                    if (!nNestingMode)
                        RunPseudoComment(Line, ErrorLine, skiperror, pUnit, pLand, S1.GetData(), sequence, destination);
                    Line.DelSubStr(p - Line.GetData() - 1, Line.GetLength() - (p - Line.GetData() - 1));
                }
                else
                    skiperror = FALSE;

                // Parse command
                p = SkipSpaces(Cmd.GetToken(Line.GetData(), " \t", ch, TRIM_ALL));
                if ('@' == Cmd.GetData()[0])
                {
                    Cmd.DelCh(0);
                    if (Cmd.IsEmpty())
                        p = SkipSpaces(Cmd.GetToken(p, " \t", ch, TRIM_ALL));
                }
                if ('!' == Cmd.GetData()[0])
                {
                    Cmd.DelCh(0);
                    if (Cmd.IsEmpty())
                        p = SkipSpaces(Cmd.GetToken(p, " \t", ch, TRIM_ALL));
                    skiperror = TRUE;
                }

                if (Cmd.IsEmpty())
                    continue;
                if (!gpDataHelper->GetOrderId(Cmd.GetData(), order))
                {
                    if (SQ_MIN + 1 == sequence)
                        SHOW_WARN_CONTINUE(" - unknown order")
                    else
                        continue;
                }

                // Handle TURN/ENDTURN nesting
                if (nNestingMode)
                {
                    if (O_TURN == order || O_TEMPLATE == order || O_ALL == order)
                        SHOW_WARN_CONTINUE(" - Nesting TURN, TEMPLATE and such orders are not allowed")
                    else if (nNestingMode + 1 == order)
                        nNestingMode = 0;
                    continue;
                }

                switch (order)
                {
                case O_TURN:
                case O_TEMPLATE:
                case O_ALL:
                    nNestingMode = order;
                    break;

                case O_ENDTURN:
                case O_ENDTEMPLATE:
                case O_ENDALL:
                    SHOW_WARN_CONTINUE(" - ENDXXX without XXX");
                    break;

                case O_GIVE:
                    if (SQ_GIVE == sequence)
                        RunOrder_Give(Line, ErrorLine, skiperror, pUnit, pLand, p, FALSE);
                    break;

                case O_GIVEIF:
                    if (SQ_GIVE == sequence)
                        RunOrder_Give(Line, ErrorLine, skiperror, pUnit, pLand, p, TRUE);
                    break;

                case O_TAKE:
                    if (SQ_GIVE == sequence)
                        RunOrder_Take(Line, ErrorLine, skiperror, pUnit, pLand, p, TRUE);
                    break;

                case O_SEND:
                    if (SQ_BUY + 1 == sequence)
                        RunOrder_Send(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_WITHDRAW:
                    if (SQ_BUY + 1 == sequence)
                        RunOrder_Withdraw(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_RECRUIT:
                    if (SQ_BUY + 1 == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        Dummy.Id = atol(N1.GetData());
                        if (!pLand->Units.Search(&Dummy, idx))
                            SHOW_WARN_CONTINUE(" - Can not find unit " << N1);
                    }
                    break;

                case O_BUY:
                    if (SQ_BUY == sequence)
                        RunOrder_Buy(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_SELL:
                    if (SQ_SELL == sequence)
                        RunOrder_Sell(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_FORM:
                    p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                    n1 = NEW_UNIT_ID(atol(N1.GetData()), pUnit->FactionId);
                    Dummy.Id = n1;

                    if (SQ_FORM == sequence)
                    {
                        EValueType type;
                        const void* value = nullptr;
                        BOOL isRepeatedForm = FALSE;

                        if (m_Units.Search(&Dummy, idx))
                        {
                            CUnit* pNewUnit = (CUnit*)m_Units.At(idx);
                            if (pNewUnit->GetProperty(PRP_FORM_REPEATED, type, value, eOriginal) &&
                                value && eCharPtr == type)
                                isRepeatedForm = TRUE;
                        }

                        if (!isRepeatedForm)
                            SHOW_WARN_CONTINUE(" - Do not manually enter a FORM command - use the split context-menu");
                    }
                    break;

                case O_ENDFORM:
                    if (SQ_FORM != sequence)
                    {
                        if (!pUnitMaster)
                            if (SQ_FORM + 1 == sequence)
                                SHOW_WARN_CONTINUE(" - FORM was not called!")
                            else
                                continue;

                        if (SQ_TEACH == sequence)
                            OrderProcess_Teach(skiperror, pUnit);

                        pUnit = pUnitMaster;
                        pUnitMaster = NULL;
                        X = XN;
                        Y = YN;
                        LocA3 = LocA3N;
                    }
                    break;

                case O_ATTACK:
                case O_ASSASSINATE:
                case O_STEAL:
                    if (SQ_CLAIM == sequence)
                    {
                        while (p && *p)
                        {
                            p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                            n1 = atol(N1.GetData());
                            Dummy.Id = n1;
                            if (pLand->Units.Search(&Dummy, idx))
                            {
                                pUnit2 = (CUnit*)pLand->Units.At(idx);
                                if (pUnit2->IsOurs)
                                    SHOW_WARN_CONTINUE(" - Unit " << n1 << " is our, can not " << Cmd << "!");
                            }
                            else
                                SHOW_WARN_CONTINUE(" - Unit " << n1 << " is not here, can not " << Cmd << "!");

                            if (O_ATTACK != order)
                                break;
                        }
                    }
                    break;

                case O_NAME:
                    if (SQ_CLAIM == sequence)
                        RunOrder_Name(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_STUDY:
                    if (SQ_STUDY == sequence)
                        RunOrder_Study(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_TEACH:
                    if (SQ_TEACH == sequence)
                        RunOrder_Teach(Line, ErrorLine, skiperror, pUnit, pLand, p, TeachCheckGlb);
                    break;

                case O_CLAIM:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        n1 = atol(N1.GetData());

                        const void* pMoneyValue = nullptr;
                        if (!pUnit->GetProperty(PRP_SILVER, type, pMoneyValue, eNormal))
                        {
                            unitmoney = 0;
                            if (PE_OK != pUnit->SetProperty(
                                    PRP_SILVER, 
                                    eLong, 
                                    reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), 
                                    eBoth))
                                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
                        }
                        else if (eLong != type)
                            SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG)
                        else
                            unitmoney = static_cast<long>(reinterpret_cast<intptr_t>(pMoneyValue));

                        unitmoney += n1;

                        if (PE_OK != pUnit->SetProperty(
                                PRP_SILVER, 
                                eLong, 
                                reinterpret_cast<const void*>(static_cast<intptr_t>(unitmoney)), 
                                eNormal))
                            SHOW_WARN_CONTINUE(NOSET << BUG);
                    }
                    break;

                case O_LEAVE:
                    if (SQ_LEAVE == sequence)
                    {
                        const void* pStructValue = nullptr;
                        if (pUnit->GetProperty(PRP_STRUCT_ID, type, pStructValue, eNormal) && eLong == type && pStructValue)
                        {
                            n1 = static_cast<long>(reinterpret_cast<intptr_t>(pStructValue));
                            pStruct = pLand->GetStructById(n1);
                            if (pStruct && pStruct->OwnerUnitId == pUnit->Id)
                                pStruct->OwnerUnitId = 0;
                        }

                        long zero_long = 0;
                        const char* empty_str = "";

                        if ((PE_OK != pUnit->SetProperty(PRP_STRUCT_NAME, eCharPtr, empty_str, eNormal)) ||
                            (PE_OK != pUnit->SetProperty(PRP_STRUCT_OWNER, eCharPtr, empty_str, eNormal)) ||
                            (PE_OK != pUnit->SetProperty(
                                    PRP_STRUCT_ID, 
                                    eLong, 
                                    reinterpret_cast<const void*>(static_cast<intptr_t>(zero_long)), 
                                    eNormal)))
                            SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
                    }
                    break;

                case O_ENTER:
                    if (SQ_ENTER == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        n1 = atol(N1.GetData());
                        pStruct = pLand->GetStructById(n1);
                        if (pStruct)
                        {
                            if (0 == pStruct->OwnerUnitId)
                            {
                                pStruct->OwnerUnitId = pUnit->Id;
                                S1 = YES;
                            }
                            else
                                S1.Empty();

                            long zero_long = 0;
                            long struct_id = pStruct->Id;
                            const char* empty_str = "";

                            if ((PE_OK != pUnit->SetProperty(PRP_STRUCT_ID, eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(zero_long)), eNormal)) ||
                                (PE_OK != pUnit->SetProperty(PRP_STRUCT_ID, eLong, reinterpret_cast<const void*>(static_cast<intptr_t>(struct_id)), eNormal)) ||
                                (PE_OK != pUnit->SetProperty(PRP_STRUCT_NAME, eCharPtr, empty_str, eNormal)) ||
                                (PE_OK != pUnit->SetProperty(PRP_STRUCT_NAME, eCharPtr, pStruct->Name.GetData(), eNormal)) ||
                                (PE_OK != pUnit->SetProperty(PRP_STRUCT_OWNER, eCharPtr, S1.GetData(), eNormal)))
                                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid structure number " << n1);
                    }
                    break;

                case O_PROMOTE:
                    if (SQ_PROMOTE == sequence)
                        RunOrder_Promote(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_MOVE:
                case O_SAIL:
                case O_ADVANCE:
                    if (SQ_MOVE == sequence)
                    {
                        if (isSimCmd)
                            destination = wxString::FromUTF8(p);
                        else
                            RunOrder_Move(Line, ErrorLine, skiperror, pUnit, pLand, p, X, Y, LocA3, order);
                    }
                    break;

                // Flag-setting orders
                case O_AUTOTAX:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_TAXING))
                            {
                                pUnit->Flags |= UNIT_FLAG_TAXING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_TAXING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_TAXING);
                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_TAXING))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_TAXING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_TAXING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_TAXING);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_GUARD:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_GUARDING) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_AVOIDING))
                            {
                                pUnit->Flags |= UNIT_FLAG_GUARDING;
                                pUnit->Flags &= ~UNIT_FLAG_AVOIDING;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_GUARDING | UNIT_FLAG_AVOIDING);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringGuard = CUnit::GetFlagString(UNIT_FLAG_GUARDING);
                                    CStr flagStringAvoid = CUnit::GetFlagString(UNIT_FLAG_AVOIDING);
                                    if (!flagStringGuard.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagStringGuard);
                                    if (!flagStringAvoid.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringAvoid);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_GUARDING))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_GUARDING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_GUARDING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_GUARDING);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_AVOID:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_AVOIDING) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_GUARDING))
                            {
                                pUnit->Flags |= UNIT_FLAG_AVOIDING;
                                pUnit->Flags &= ~UNIT_FLAG_GUARDING;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_AVOIDING | UNIT_FLAG_GUARDING);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringAvoid = CUnit::GetFlagString(UNIT_FLAG_AVOIDING);
                                    CStr flagStringGuard = CUnit::GetFlagString(UNIT_FLAG_GUARDING);
                                    if (!flagStringAvoid.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagStringAvoid);
                                    if (!flagStringGuard.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringGuard);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_AVOIDING))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_AVOIDING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_AVOIDING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_AVOIDING);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_BEHIND:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_BEHIND))
                            {
                                pUnit->Flags |= UNIT_FLAG_BEHIND;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_BEHIND;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_BEHIND);
                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_BEHIND))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_BEHIND;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_BEHIND;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_BEHIND);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_HOLD:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_HOLDING))
                            {
                                pUnit->Flags |= UNIT_FLAG_HOLDING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_HOLDING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_HOLDING);
                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_HOLDING))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_HOLDING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_HOLDING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_HOLDING);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_NOAID:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_RECEIVING_NO_AID))
                            {
                                pUnit->Flags |= UNIT_FLAG_RECEIVING_NO_AID;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_RECEIVING_NO_AID;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_RECEIVING_NO_AID);
                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_RECEIVING_NO_AID))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_RECEIVING_NO_AID;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_RECEIVING_NO_AID;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_RECEIVING_NO_AID);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_NOCROSS:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_NO_CROSS_WATER))
                            {
                                pUnit->Flags |= UNIT_FLAG_NO_CROSS_WATER;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_NO_CROSS_WATER;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_NO_CROSS_WATER);
                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_NO_CROSS_WATER))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_NO_CROSS_WATER;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_NO_CROSS_WATER;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_NO_CROSS_WATER);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_SPOILS:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (N1.IsEmpty() || 0 == stricmp(N1.GetData(), "ALL"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_SPOILS))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_SPOILS;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_SPOILS;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                    CUnit::ClearSpoils(&pUnit->m_EndTurnDescription);
                            }
                        }
                        else
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_SPOILS))
                            {
                                pUnit->Flags |= UNIT_FLAG_SPOILS;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_SPOILS;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CUnit::ClearSpoils(&pUnit->m_EndTurnDescription);
                                    CStr flagString;
                                    if (0 == stricmp(N1.GetData(), "NONE"))
                                        flagString = "weightless battle spoils";
                                    else if (0 == stricmp(N1.GetData(), "WALK"))
                                        flagString = "walking battle spoils";
                                    else if (0 == stricmp(N1.GetData(), "RIDE"))
                                        flagString = "riding battle spoils";
                                    else if (0 == stricmp(N1.GetData(), "FLY"))
                                        flagString = "flying battle spoils";
                                    else if (0 == stricmp(N1.GetData(), "SWIM"))
                                        flagString = "swiming battle spoils";
                                    else if (0 == stricmp(N1.GetData(), "SAIL"))
                                        flagString = "sailing battle spoils";

                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                    }
                    break;

                case O_SHARE:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_SHARING))
                            {
                                pUnit->Flags |= UNIT_FLAG_SHARING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_SHARING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_SHARING);
                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "0"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_SHARING))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_SHARING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_SHARING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_SHARING);
                                    if (!flagString.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_REVEAL:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "UNIT"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_REVEALING_UNIT) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_REVEALING_FACTION))
                            {
                                pUnit->Flags |= UNIT_FLAG_REVEALING_UNIT;
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_FACTION;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_REVEALING_UNIT | UNIT_FLAG_REVEALING_FACTION);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringUnit = CUnit::GetFlagString(UNIT_FLAG_REVEALING_UNIT);
                                    CStr flagStringFaction = CUnit::GetFlagString(UNIT_FLAG_REVEALING_FACTION);
                                    if (!flagStringFaction.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringFaction);
                                    if (!flagStringUnit.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagStringUnit);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "FACTION"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_REVEALING_FACTION) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_REVEALING_UNIT))
                            {
                                pUnit->Flags |= UNIT_FLAG_REVEALING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_UNIT;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_REVEALING_FACTION | UNIT_FLAG_REVEALING_UNIT);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringUnit = CUnit::GetFlagString(UNIT_FLAG_REVEALING_UNIT);
                                    CStr flagStringFaction = CUnit::GetFlagString(UNIT_FLAG_REVEALING_FACTION);
                                    if (!flagStringUnit.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringUnit);
                                    if (!flagStringFaction.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagStringFaction);
                                }
                            }
                        }
                        else if (N1.IsEmpty())
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_REVEALING_FACTION) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_REVEALING_UNIT))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_REVEALING_UNIT;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_REVEALING_FACTION | UNIT_FLAG_REVEALING_UNIT);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringUnit = CUnit::GetFlagString(UNIT_FLAG_REVEALING_UNIT);
                                    CStr flagStringFaction = CUnit::GetFlagString(UNIT_FLAG_REVEALING_FACTION);
                                    if (!flagStringUnit.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringUnit);
                                    if (!flagStringFaction.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringFaction);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_CONSUME:
                    if (SQ_CLAIM == sequence)
                    {
                        p = SkipSpaces(N1.GetToken(p, " \t", ch, TRIM_ALL));
                        if (0 == stricmp(N1.GetData(), "UNIT"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_CONSUMING_UNIT) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_CONSUMING_FACTION))
                            {
                                pUnit->Flags |= UNIT_FLAG_CONSUMING_UNIT;
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_FACTION;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_CONSUMING_UNIT | UNIT_FLAG_CONSUMING_FACTION);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringUnit = CUnit::GetFlagString(UNIT_FLAG_CONSUMING_UNIT);
                                    CStr flagStringFaction = CUnit::GetFlagString(UNIT_FLAG_CONSUMING_FACTION);
                                    if (!flagStringFaction.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringFaction);
                                    if (!flagStringUnit.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagStringUnit);
                                }
                            }
                        }
                        else if (0 == stricmp(N1.GetData(), "FACTION"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_CONSUMING_FACTION) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_CONSUMING_UNIT))
                            {
                                pUnit->Flags |= UNIT_FLAG_CONSUMING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_UNIT;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_CONSUMING_FACTION | UNIT_FLAG_CONSUMING_UNIT);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringUnit = CUnit::GetFlagString(UNIT_FLAG_CONSUMING_UNIT);
                                    CStr flagStringFaction = CUnit::GetFlagString(UNIT_FLAG_CONSUMING_FACTION);
                                    if (!flagStringUnit.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringUnit);
                                    if (!flagStringFaction.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagStringFaction);
                                }
                            }
                        }
                        else if (N1.IsEmpty())
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_CONSUMING_FACTION) &&
                                !(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_CONSUMING_UNIT))
                            {
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_FACTION;
                                pUnit->Flags &= ~UNIT_FLAG_CONSUMING_UNIT;
                                flagsSetInPhase[pUnit->Id] |= (UNIT_FLAG_CONSUMING_FACTION | UNIT_FLAG_CONSUMING_UNIT);
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagStringUnit = CUnit::GetFlagString(UNIT_FLAG_CONSUMING_UNIT);
                                    CStr flagStringFaction = CUnit::GetFlagString(UNIT_FLAG_CONSUMING_FACTION);
                                    if (!flagStringUnit.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringUnit);
                                    if (!flagStringFaction.IsEmpty())
                                        CUnit::ClearFlag(&pUnit->m_EndTurnDescription, flagStringFaction);
                                }
                            }
                        }
                        else
                            SHOW_WARN_CONTINUE(" - Invalid parameter");
                    }
                    break;

                case O_TRANSPORT:
                    if (SQ_TRANSPORT == sequence)
                        RunOrder_Transport(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;

                case O_PILLAGE:
                    if (SQ_CLAIM == sequence)
                    {
                        if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_PILLAGING))
                        {
                            pUnit->Flags |= UNIT_FLAG_PILLAGING;
                            flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_PILLAGING;
                        }
                    }
                    break;

                case O_TAX:
                    if (SQ_CLAIM == sequence)
                    {
                        if (!pUnit->HasOrder("AUTOTAX 1") && !pUnit->HasOrder("@AUTOTAX 1"))
                        {
                            if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_TAXING))
                            {
                                pUnit->Flags |= UNIT_FLAG_TAXING;
                                flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_TAXING;
                                if (!pUnit->m_EndTurnDescription.IsEmpty())
                                {
                                    CStr flagString = CUnit::GetFlagString(UNIT_FLAG_TAXING);
                                    if (!flagString.IsEmpty())
                                        CUnit::SetFlag(&pUnit->m_EndTurnDescription, flagString);
                                }
                            }
                        }
                    }
                    break;

                case O_ENTERTAIN:
                    if (SQ_CLAIM == sequence)
                    {
                        if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_ENTERTAINING))
                        {
                            pUnit->Flags |= UNIT_FLAG_ENTERTAINING;
                            flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_ENTERTAINING;
                        }
                    }
                    break;

                case O_WORK:
                    if (SQ_CLAIM == sequence)
                    {
                        if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_WORKING))
                        {
                            pUnit->Flags |= UNIT_FLAG_WORKING;
                            flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_WORKING;
                        }
                    }
                    break;

                case O_BUILD:
                    if (SQ_MAX - 1 == sequence)
                    {
                        if (!(flagsSetInPhase[pUnit->Id] & UNIT_FLAG_PRODUCING))
                        {
                            pUnit->Flags |= UNIT_FLAG_PRODUCING;
                            flagsSetInPhase[pUnit->Id] |= UNIT_FLAG_PRODUCING;
                        }
                    }
                    break;

                case O_PRODUCE:
                    if (SQ_MAX - 1 == sequence)
                        RunOrder_Produce(Line, ErrorLine, skiperror, pUnit, pLand, p);
                    break;
                }

                // Copy commands to new unit
                if (pUnitMaster && (SQ_MAX - 1 == sequence) && (O_FORM != order) && !errors)
                    pUnit->Orders << Line << EOL_SCR;
            }

            if (SQ_TEACH == sequence)
                OrderProcess_Teach(skiperror, pUnit);

            if (SQ_MOVE == sequence && !destination.IsEmpty())
            {
                if (!pUnit->pMovement)
                {
                    CLand* pLandHexDest = GetLandFlexible(destination);
                    if (pLandHexDest)
                    {
                        MovementSetup setup = RoutePlanner::GetMovementSetup(pUnit, O_MOVE);
                        wxString route = RoutePlanner::GetRoute(pLand, pLandHexDest, setup, RoutePlanner::ROUTE_MARKUP_TURN);
                        if (!route.IsEmpty())
                        {
                            if (route.Length() > 66)
                                RoutePlanner::GetFirstMove(route);

                            route.Replace("_ ", "", true);
                            RunOrder_Move(Line, ErrorLine, skiperror, pUnit, pLand, (const char*)route.ToUTF8(), X, Y, LocA3, O_MOVE);
                            route = wxString::Format("MOVE%s", route);

                            pUnit->Orders.TrimRight(TRIM_ALL);
                            pUnit->Orders << EOL_SCR;
                            pUnit->Orders << (const char*)route.ToUTF8();
                            Line = (const char*)route.ToUTF8();
                        }
                    }
                }
                destination.Clear();
            }

            if (pUnit->IsOurs)
                pUnit->BackErasedFlags();
        }

        // Economy phases
        if (SQ_BUY == sequence)
        {
            if (m_EconomyShareAfterBuy)
                RunOrder_ShareSilver(Line, ErrorLine, skiperror, pLand, SHARE_BUY, wxT("purchases"));
        }
        if (SQ_STUDY == sequence)
        {
            if (m_EconomyShareMaintainance)
                RunOrder_ShareSilver(Line, ErrorLine, skiperror, pLand, SHARE_STUDY, wxT("study"));
        }
        if (SQ_MAX - 1 == sequence)
        {
            if (m_EconomyMaintainanceCosts)
                RunOrder_Upkeep(pLand);
            if (m_EconomyShareMaintainance)
                RunOrder_ShareSilver(Line, ErrorLine, skiperror, pLand, SHARE_UPKEEP, wxT("maintainance"));
        }
    }

    pLand->CalcStructsLoad();
    pLand->SetFlagsFromUnits();
    OrderErrFinalize();

    if (gpApp)
        gpApp->UpdateUnitDescriptionPane(gpApp->GetSelectedUnit());
}

//---------------------------------------------------------------
// TEACH command processing
// Validates teaching and adds comments
//---------------------------------------------------------------

void CAtlaParser::RunOrder_Teach(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params, BOOL TeachCheckGlb)
{
    EValueType          type;
    long                teacherMen = 0;
    long                studentMen = 0;
    int                 idx;
    CUnit* pUnit2 = NULL;
    CBaseObject         Dummy;
    BOOL                leader_checked = FALSE;

    CStr commandPart;
    CStr commentsPart;
    CStr originalLine = Line;

    // Split line into command and comments
    int commentPos = Line.FindSubStr(";");
    if (commentPos >= 0)
    {
        for (int i = 0; i < commentPos; i++)
            commandPart.AddCh(Line.GetData()[i]);
        commandPart.TrimRight(TRIM_ALL);
        commentsPart = Line.GetData() + commentPos + 1;
        commentsPart.TrimLeft(TRIM_ALL);
    }
    else
    {
        commandPart = Line;
        commandPart.TrimRight(TRIM_ALL);
        commentsPart.Empty();
    }

    while (params && *params)
    {
        if (!leader_checked)
        {
            const void* pLeaderValue = NULL;
            if (!pUnit->GetProperty(PRP_LEADER, type, pLeaderValue, eNormal))
            {
                SHOW_WARN_BREAK(" - Unit " << pUnit->Id << " is not a leader or hero");
                break;
            }
            leader_checked = TRUE;
        }

        long targetUnitId = 0;
        if (!GetTargetUnitId(params, pUnit->FactionId, targetUnitId))
        {
            SHOW_WARN_CONTINUE(" - Invalid unit Id");
            continue;
        }

        if (0 == targetUnitId)
            continue;

        Dummy.Id = targetUnitId;
        if (!pLand->Units.Search(&Dummy, idx))
        {
            SHOW_WARN_CONTINUE(" - Can not find unit " << targetUnitId);
            continue;
        }

        pUnit2 = (CUnit*)pLand->Units.At(idx);
        if (!pUnit2)
        {
            SHOW_WARN_CONTINUE(" - Invalid target unit");
            continue;
        }

        // Check teacher men count
        const void* pTeacherMen = NULL;
        if (pUnit->GetProperty(PRP_MEN, type, pTeacherMen, eNormal) && type == eLong)
            teacherMen = static_cast<long>(reinterpret_cast<intptr_t>(pTeacherMen));

        if (teacherMen <= 0)
        {
            SHOW_WARN_CONTINUE(" - There are no men in the unit!");
            continue;
        }

        // Check student men count
        const void* pStudentMen = NULL;
        if (pUnit2->GetProperty(PRP_MEN, type, pStudentMen, eNormal) && type == eLong)
            studentMen = static_cast<long>(reinterpret_cast<intptr_t>(pStudentMen));

        if (studentMen <= 0)
        {
            SHOW_WARN_CONTINUE(" - There are no men in the student unit!");
            continue;
        }

        CStr studentNameWithId;
        studentNameWithId.Format("%s(%ld)", pUnit2->Name.GetData(), pUnit2->Id);

        // Check if student has STUDY command
        CStr studyOrders = pUnit2->GetOrders("STUDY");

        if (studyOrders.IsEmpty())
        {
            if (!commentsPart.IsEmpty())
                commentsPart << ", ";
            commentsPart << "unit " << studentNameWithId << " is not study";
            continue;
        }

        // Extract skill being studied
        CStr studentSkill = pUnit2->StudyingSkill;
        if (studentSkill.IsEmpty())
        {
            if (!commentsPart.IsEmpty())
                commentsPart << ", ";
            commentsPart << "unit " << studentNameWithId << " has STUDY but no skill";
            continue;
        }

        // Remove skill postfix
        int skillPos = studentSkill.FindSubStrR(PRP_SKILL_POSTFIX);
        if (skillPos >= 0)
        {
            CStr pureSkill = studentSkill;
            pureSkill.DelSubStr(skillPos, strlen(PRP_SKILL_POSTFIX));
            studentSkill = pureSkill;
        }

        // Get teacher skill level
        wxString skillCode = wxString::FromUTF8(studentSkill.GetData()).Upper();
        TUnitSkill teacherSkillInfo = CUnit::GetSkillByCode(&pUnit->Description, skillCode);
        long teacherLevel = teacherSkillInfo.skillLevel;

        if (teacherLevel == 0)
        {
            CStr teacherSkillProp;
            teacherSkillProp << studentSkill.GetData() << PRP_SKILL_POSTFIX;
            const void* pTeacherSkill = NULL;
            if (pUnit->GetProperty(teacherSkillProp.GetData(), type, pTeacherSkill, eNormal) &&
                type == eLong && pTeacherSkill)
                teacherLevel = static_cast<long>(reinterpret_cast<intptr_t>(pTeacherSkill));
        }

        // Get student skill level
        TUnitSkill studentSkillInfo = CUnit::GetSkillByCode(&pUnit2->Description, skillCode);
        long studentLevel = studentSkillInfo.skillLevel;

        // Validate teaching
        if (teacherLevel == 0)
        {
            if (!commentsPart.IsEmpty())
                commentsPart << ", ";
            commentsPart << "unit " << studentNameWithId << " can't be teached (teacher lacks " << studentSkill << ")";
            continue;
        }

        if (teacherLevel <= studentLevel)
        {
            if (!commentsPart.IsEmpty())
                commentsPart << ", ";
            commentsPart << "unit " << studentNameWithId << " can't be teached (teacher " << teacherLevel
                << " <= student " << studentLevel << ")";
            continue;
        }

        // Add student to teacher's collection
        if (!pUnit->pStudents)
            pUnit->pStudents = new CBaseCollById;

        CBaseObject* pDummyStudent = new CBaseObject;
        pDummyStudent->Id = pUnit2->Id;

        int studentIndex = -1;
        if (!pUnit->pStudents->Search(pDummyStudent, studentIndex))
        {
            if (pUnit->pStudents->Insert(pDummyStudent) == -1)
            {
                delete pDummyStudent;
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Failed to add unit " << pUnit2->Id << " to students list";
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
            }
        }
        else
        {
            delete pDummyStudent;
            if (!commentsPart.IsEmpty())
                commentsPart << ", ";
            commentsPart << "unit " << studentNameWithId << " already in teach list";
        }
    }

    // Rebuild line with comments
    CStr newLine = commandPart;
    if (!commentsPart.IsEmpty())
        newLine << " ; " << commentsPart;

    if (strcmp(originalLine.GetData(), newLine.GetData()) != 0)
    {
        Line = newLine;

        CStr updatedOrders;
        const char* ordersPtr = pUnit->Orders.GetData();
        CStr currentLine;
        BOOL lineReplaced = FALSE;

        wxString targetCmd = wxString::FromUTF8(commandPart.GetData()).Upper().Trim();

        while (ordersPtr && *ordersPtr)
        {
            ordersPtr = currentLine.GetToken(ordersPtr, '\n', TRIM_ALL);
            wxString thisLine = wxString::FromUTF8(currentLine.GetData()).Trim();
            wxString thisCmd = thisLine.BeforeFirst(';').Trim().Upper();

            if (!lineReplaced && thisCmd == targetCmd)
            {
                if (!updatedOrders.IsEmpty())
                    updatedOrders << EOL_SCR;
                updatedOrders << newLine;
                lineReplaced = TRUE;
            }
            else
            {
                if (!updatedOrders.IsEmpty())
                    updatedOrders << EOL_SCR;
                updatedOrders << currentLine;
            }
        }

        if (lineReplaced)
            pUnit->Orders = updatedOrders;
    }
}

//-------------------------------------------------------------
// MOVE command processing
// Handles movement with terrain cost calculation and validation
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Move(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params, int& X, int& Y, int& LocA3, long order)
{
    int                 i, idx = 0;
    CStr                S1;
    char                ch;
    EValueType          type;
    long                n1 = 0;
    CStruct* pStruct = nullptr;
    CStr                sErr(32), S;
    CBaseObject         Dummy;
    long                skill = 0, nmen = 0, structid = 0;
    const void* value = nullptr;

    CLand* pLandExit = NULL;
    CLand* pLandCurrent = pLand;
    long                hexId = 0;
    long                newHexId = 0;
    long                currentStruct = 0;
    int                 totalMovementCost = 0;
    bool                pathCanBeTraced = true;
    const int           startMonth = (m_YearMon % 100) - 1;
    bool                throughWall = false;

    bool                unitStartedInShip = false;
    bool                unitActuallyMoved = false;

    const int FirstShipNumber = atol(gpApp->GetConfig(SZ_SECT_COMMON, "FIRST_SHIP_NUMBER"));

    MovementSetup setup = RoutePlanner::GetMovementSetup(pUnit, order);

    do
    {
        // Arcadia III sailing handling
        if (O_SAIL == order)
        {
            value = nullptr;
            if (pUnit->GetProperty(PRP_STRUCT_ID, type, value, eNormal) && eLong == type && value)
            {
                n1 = static_cast<long>(reinterpret_cast<intptr_t>(value));
                pStruct = pLand->GetStructById(n1);
                if (pStruct && NO_LOCATION != pStruct->Location)
                {
                    if (NO_LOCATION == LocA3)
                        LocA3 = pStruct->Location;
                    RunOrder_SailAIII(Line, ErrorLine, skiperror, pUnit, pLand, params, X, Y, LocA3);
                    goto SomeChecks;
                }
            }
        }

        // Check if unit is in a structure
        value = nullptr;
        if (pUnit->GetProperty(PRP_STRUCT_ID, type, value, eNormal) && eLong == type && value)
            currentStruct = static_cast<long>(reinterpret_cast<intptr_t>(value));

        if (order == O_MOVE && currentStruct >= FirstShipNumber)
            unitStartedInShip = true;

        while (params)
        {
            params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));
            for (i = 0; i < DirectionsCount; i++)
            {
                if (0 == stricmp(S1.GetData(), Directions[i]))
                {
                    if (pLandCurrent)
                        hexId = pLandCurrent->Id;

                    if (pLandCurrent)
                    {
                        pLandExit = GetLandExit(pLandCurrent, i % 6);
                        if (pLandExit)
                            newHexId = pLandExit->Id;
                        else if (IsLandExitClosed(pLandCurrent, i % 6))
                            throughWall = true;
                    }

                    if (!pLandExit)
                    {
                        int x, y, z;
                        LandIdToCoord(hexId, x, y, z);
                        ExtrapolateLandCoord(x, y, z, i % 6);
                        newHexId = LandCoordToId(x, y, z);
                    }

                    if (newHexId)
                    {
                        if (!pUnit->pMovement)
                            pUnit->pMovement = new CLongColl;

                        pUnit->pMovement->Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(newHexId)));
                        currentStruct = 0;
                        pLandExit = GetLand(newHexId);
                    }

                    if (pLandExit && IsLandExitClosed(pLandExit, (i + 3) % 6) && (pLandExit->FindExit(hexId) < 0))
                        throughWall = true;

                    if (pLandExit && pLandCurrent && pathCanBeTraced)
                    {
                        int terrainCost = GetTerrainMovementCost(wxString::FromUTF8(pLandExit->TerrainType.GetData()));
                        if (setup.noCross && terrainCost == 999)
                        {
                            pathCanBeTraced = false;
                            SHOW_WARN(" - Trying to move into the ocean!");
                            skiperror = true;
                        }
                        else
                        {
                            bool weather = IsBadWeatherHex(pLandExit, startMonth);
                            bool road = IsRoadConnected(pLandCurrent, pLandExit, i % 6);
                            int cost = RoutePlanner::GetMovementCost(terrainCost, weather, road, setup);
                            totalMovementCost += cost;
                        }
                    }
                    else totalMovementCost += 1;

                    pLandCurrent = pLandExit;
                    hexId = newHexId;
                    newHexId = 0;
                    pLandExit = NULL;

                    if (throughWall)
                        SHOW_WARN_CONTINUE(" - Hexes are not connected - going through a wall perhaps!");

                    unitActuallyMoved = true;
                    break;
                }
            }

            if (0 == stricmp(S1.GetData(), "IN"))
            {
                if (0 == currentStruct)
                    SHOW_WARN_CONTINUE(" - Going IN without being in a structure!")
                else if (pLandCurrent && pathCanBeTraced)
                {
                    if (0 == (pLandCurrent->Flags & LAND_VISITED))
                        SHOW_WARN_CONTINUE(" - Going through shaft in unvisited hex!")
                    else if (pLandCurrent->Structs.Count() < currentStruct)
                        SHOW_WARN_CONTINUE(" - No such structure!")
                    else
                    {
                        CStruct* pTargetStruct = (CStruct*)pLandCurrent->Structs.At(currentStruct - 1);
                        if (pTargetStruct->Attr & SA_SHAFT)
                        {
                            if (pTargetStruct->Description.FindSubStr("links to (") >= 0)
                            {
                                pLandExit = GetLandFlexible(wxString::FromUTF8(pTargetStruct->Description.GetData()));
                                if (pLandExit)
                                {
                                    if (!pUnit->pMovement)
                                        pUnit->pMovement = new CLongColl;
                                    pUnit->pMovement->Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(pLandExit->Id)));
                                    currentStruct = 0;
                                    if (pathCanBeTraced && pLandExit && pLandCurrent)
                                    {
                                        int terrainCost = GetTerrainMovementCost(wxString::FromUTF8(pLandExit->TerrainType.GetData()));
                                        bool weather = IsBadWeatherHex(pLandExit, startMonth);
                                        bool road = false;
                                        int cost = RoutePlanner::GetMovementCost(terrainCost, weather, road, setup);
                                        totalMovementCost += cost;
                                    }
                                    int z;
                                    LandIdToCoord(pLandExit->Id, X, Y, z);
                                    pLandCurrent = pLandExit;
                                }
                                else
                                {
                                    totalMovementCost += 1;
                                    pathCanBeTraced = false;
                                    SHOW_WARN_CONTINUE(" - Cannot find hex at other side of shaft, description broken!");
                                }
                            }
                            else
                            {
                                totalMovementCost += 1;
                                pathCanBeTraced = false;
                                if (!pUnit->pMovement)
                                    pUnit->pMovement = new CLongColl;
                                pUnit->pMovement->Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(pLandCurrent->Id)));
                            }
                        }
                        else
                        {
                            totalMovementCost += 1;
                            pathCanBeTraced = false;
                            SHOW_WARN_CONTINUE(" - Structure is not a shaft!");
                        }
                    }
                }
                else totalMovementCost += 1;
            }
            else
            {
                int structNumber = atoi(S1.GetData());
                if (structNumber > 0 && structNumber < 10000 && currentStruct != structNumber)
                {
                    currentStruct = structNumber;
                    unitActuallyMoved = true;
                }
            }
        }

    SomeChecks:

        // Validate men count
        value = nullptr;
        if (!pUnit->GetProperty(PRP_MEN, type, value, eNormal) || !value)
        {
            nmen = 0;
            SHOW_WARN_CONTINUE(" - There are no men in the unit!");
        }
        else if (eLong != type)
        {
            nmen = 0;
            SHOW_WARN_CONTINUE(" - Invalid men property type!");
        }
        else
        {
            nmen = static_cast<long>(reinterpret_cast<intptr_t>(value));
            if (nmen <= 0)
                SHOW_WARN_CONTINUE(" - There are no men in the unit!");
        }

        // Check ocean crossing without ability
        if (pLandCurrent && !setup.canSwim && !currentStruct && order == O_MOVE && (GetTerrainMovementCost(pLandCurrent->TerrainType.GetData()) == 999))
            SHOW_WARN_CONTINUE(" - flying into ocean, can drown!");

        // Warn if moving out of ship without LEAVE
        if (order == O_MOVE && unitStartedInShip && unitActuallyMoved)
            SHOW_WARN(" - Moving out of a ship fails if the ship sails. Consider using LEAVE.");

        // Check movement speed requirements
        if (pUnit->reqMovementSpeed > 0)
        {
            if (setup.speed < pUnit->reqMovementSpeed)
            {
                wxString msg = wxString::Format(wxT(" - Unit is moving at slower speed than specified! [%d/%d]"), setup.speed, pUnit->reqMovementSpeed);
                SHOW_WARN_CONTINUE(msg.ToUTF8());
            }
        }
        else if (totalMovementCost > setup.speed && gpDataHelper->ShowMoveWarnings())
        {
            wxString msg = wxString::Format(wxT(" - Unit is moving further than it can! [%d/%d]"), totalMovementCost, setup.speed);
            SHOW_WARN_CONTINUE(msg.ToUTF8());
        }

        // SAIL specific checks
        if (O_SAIL == order)
        {
            value = nullptr;
            if (!pUnit->GetProperty(PRP_STRUCT_ID, type, value, eNormal) || eLong != type || !value)
                SHOW_WARN_CONTINUE(" - Must be in a ship to issue SAIL order!")
            else
                structid = static_cast<long>(reinterpret_cast<intptr_t>(value));

            S = "SAIL";
            S << PRP_SKILL_POSTFIX;

            value = nullptr;
            if (!pUnit->GetProperty(S.GetData(), type, value, eNormal) || eLong != type || !value)
                SHOW_WARN_CONTINUE(" - Needs SAIL skill!")
            else
            {
                skill = static_cast<long>(reinterpret_cast<intptr_t>(value));
                if (skill <= 0)
                    SHOW_WARN_CONTINUE(" - Needs SAIL skill!");
            }

            if (structid > 0)
            {
                Dummy.Id = structid;
                if (!pLand->Structs.Search(&Dummy, idx))
                {
                    CStr bugMsg(32);
                    bugMsg = " - Invalid Struct Id ";
                    SHOW_WARN_CONTINUE(bugMsg.GetData());
                }
                else
                {
                    pStruct = (CStruct*)pLand->Structs.At(idx);
                    if (pStruct && nmen > 0 && skill > 0)
                        pStruct->SailingPower += (nmen * skill);
                }
            }
        }
        else
        {
            if (gpDataHelper->ShowMoveWarnings())
            {
                pUnit->CheckWeight(sErr);
                if (!sErr.IsEmpty())
                    SHOW_WARN_CONTINUE(sErr);
            }
        }
    } while (FALSE);
}

//-------------------------------------------------------------
// PROMOTE command processing
// Transfers command of a structure to another unit
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Promote(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params)
{
    long                n1;
    long                id1 = 0, id2 = 0;
    int                 idx;
    CBaseObject         Dummy;
    CUnit* pUnit2;
    EValueType          type;
    CStruct* pStruct;

    do
    {
        if (!GetTargetUnitId(params, pUnit->FactionId, n1))
            SHOW_WARN_CONTINUE(" - Invalid unit Id");

        Dummy.Id = n1;
        if (pLand->Units.Search(&Dummy, idx))
        {
            pUnit2 = (CUnit*)pLand->Units.At(idx);
            params = NULL;

            const void* pStructId1 = nullptr;
            if (!pUnit->GetProperty(PRP_STRUCT_ID, type, pStructId1, eNormal))
                SHOW_WARN_CONTINUE(" - The unit is not inside a struct");
            if (eLong != type)
                SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit->Id << BUG)
            else
                id1 = static_cast<long>(reinterpret_cast<intptr_t>(pStructId1));

            const void* pStructId2 = nullptr;
            if (!pUnit2->GetProperty(PRP_STRUCT_ID, type, pStructId2, eNormal))
                SHOW_WARN_CONTINUE(" - The unit is not inside a struct");
            if (eLong != type)
                SHOW_WARN_CONTINUE(NOTNUMERIC << pUnit2->Id << BUG)
            else
                id2 = static_cast<long>(reinterpret_cast<intptr_t>(pStructId2));

            if (id1 != id2)
                SHOW_WARN_CONTINUE(" - Units are not in the same struct");

            if (PE_OK != pUnit->SetProperty(PRP_STRUCT_OWNER, eCharPtr, "", eNormal))
                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit->Id << BUG);

            if ((PE_OK != pUnit2->SetProperty(PRP_STRUCT_OWNER, eCharPtr, "", eNormal)) ||
                (PE_OK != pUnit2->SetProperty(PRP_STRUCT_OWNER, eCharPtr, YES, eNormal)))
                SHOW_WARN_CONTINUE(NOSETUNIT << pUnit2->Id << BUG);

            pStruct = pLand->GetStructById(id1);
            if (pStruct)
                pStruct->OwnerUnitId = pUnit2->Id;
        }
        else
            SHOW_WARN_CONTINUE(" - Can not find unit " << n1);
    } while (FALSE);
}

//---------------------------------------------------------------------
// Movement location mapping for Arcadia III sailing
//---------------------------------------------------------------------

static struct
{
    eDirection   Location;
    eDirection   Direction;
    int          dX;
    int          dY;
    eDirection   TargetLoc;
} NextLandLoc[] =
{
    { North    , North     ,  0, -2, South     },
    { North    , Northeast ,  1, -1, Northwest },
    { North    , Southeast ,  0,  0, Northeast },
    { North    , Southwest ,  0,  0, Northwest },
    { North    , Northwest , -1, -1, Northeast },

    { Northeast, North     ,  0, -2, Southeast },
    { Northeast, Northeast ,  1, -1, Southwest },
    { Northeast, Southeast ,  1,  1, North     },
    { Northeast, South     ,  0,  0, Southeast },
    { Northeast, Northwest ,  0,  0, North     },

    { Southeast, North     ,  0,  0, Northeast },
    { Southeast, Northeast ,  1, -1, South     },
    { Southeast, Southeast ,  1,  1, Northwest },
    { Southeast, South     ,  0,  2, Northeast },
    { Southeast, Southwest ,  0,  0, South     },

    { South    , Northeast ,  0,  0, Southeast },
    { South    , Southeast ,  1,  1, Southwest },
    { South    , South     ,  0,  2, North     },
    { South    , Southwest , -1,  1, Southeast },
    { South    , Northwest ,  0,  0, Southwest },

    { Southwest, North     ,  0,  0, Northwest },
    { Southwest, Southeast ,  0,  0, South     },
    { Southwest, South     ,  0,  2, Northwest },
    { Southwest, Southwest , -1,  1, Northeast },
    { Southwest, Northwest , -1, -1, South     },

    { Northwest, North     ,  0, -2, Southwest },
    { Northwest, Northeast ,  0,  0, North     },
    { Northwest, South     ,  0,  0, Southwest },
    { Northwest, Southwest , -1,  1, North     },
    { Northwest, Northwest , -1, -1, Southeast },

    { Center   , North     ,  0, -2, South     },
    { Center   , Northeast ,  1, -1, Southwest },
    { Center   , Southeast ,  1,  1, Northwest },
    { Center   , South     ,  0,  2, North     },
    { Center   , Southwest , -1,  1, Northeast },
    { Center   , Northwest , -1, -1, Southeast }
};

//-------------------------------------------------------------
// SAIL command processing for Arcadia III
// Handles ship movement with location tracking
//-------------------------------------------------------------

void CAtlaParser::RunOrder_SailAIII(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params, int& X, int& Y, int& LocA3)
{
    int                 ID;
    int                 i, j;
    CStr                S1;
    char                ch;
    eDirection          Dir;
    BOOL                GoodSail;
    CStr                SailPassed;
    CLand* pNewLand;

    while (params && *params)
    {
        GoodSail = FALSE;

        params = SkipSpaces(S1.GetToken(params, " \t", ch, TRIM_ALL));
        for (i = 0; i < DirectionsCount; i++)
            if (0 == stricmp(S1.GetData(), Directions[i]))
            {
                Dir = (eDirection)(i % 6);

                for (j = 0; j < (int)sizeof(NextLandLoc) / (int)sizeof(*NextLandLoc); j++)
                    if (NextLandLoc[j].Location == LocA3 && NextLandLoc[j].Direction == Dir)
                    {
                        X += NextLandLoc[j].dX;
                        Y += NextLandLoc[j].dY;
                        LocA3 = NextLandLoc[j].TargetLoc;
                        GoodSail = TRUE;
                        break;
                    }

                if (!GoodSail)
                    break;

                X = NormalizeHexX(X, pLand->pPlane);
                ID = LandCoordToId(X, Y, pLand->pPlane->Id);

                pNewLand = GetLand(ID);
                if (!pNewLand || (pNewLand && 0 == stricmp("Ocean", pNewLand->TerrainType.GetData()) ||
                    0 == stricmp("Lake", pNewLand->TerrainType.GetData())))
                    LocA3 = Center;

                if (!pUnit->pMovement)
                    pUnit->pMovement = new CLongColl;
                pUnit->pMovement->Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(ID)));

                if (!pUnit->pMoveA3Points)
                    pUnit->pMoveA3Points = new CLongColl;
                pUnit->pMoveA3Points->Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(LocA3)));

                break;
            }

        if (!GoodSail)
            SHOW_WARN_CONTINUE(" - Can not sail " << SailPassed << S1);
        SailPassed << S1 << ' ';
    }
}

//-------------------------------------------------------------
// TRANSPORT command processing
// Transfers items between units within range
//-------------------------------------------------------------

void CAtlaParser::RunOrder_Transport(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params)
{
    EValueType          type;
    long                n1;
    CBaseObject         Dummy;
    CUnit* pUnit2 = NULL;
    const void* value;
    const void* value2;
    CStr                Item;
    int                 amount;

    CStr                giverName;
    giverName = pUnit->Name.GetData();
    if (giverName.IsEmpty())
        giverName.Format("Unit %d", pUnit->Id);

    do
    {
        // Parse target unit ID
        if (!GetTargetUnitId(params, pUnit->FactionId, n1))
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Invalid unit id";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        if (n1 == pUnit->Id)
        {
            if (!skiperror)
            {
                ErrorLine.Empty();
                ErrorLine << Line << " - Transporting to yourself";
                OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
            }
            break;
        }

        if (0 != n1)
        {
            // Calculate range based on quartermaster skill
            int maxRadius = 11;
            if (pUnit->IsQuartermaster())
            {
                int quamLevel = pUnit->GetQuamLevel();
                maxRadius = 3 + (quamLevel + 1) / 3;
            }

            pUnit2 = FindUnitInRange(n1, pLand, maxRadius);

            if (!pUnit2)
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    CStr errorMsg;
                    errorMsg.Format(" - Can not locate target unit within transport range (%d hexes)", maxRadius);
                    ErrorLine << Line << errorMsg.GetData();
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }
        }

        // Parse amount and item
        if (GetItemAndAmountForGive(Line, ErrorLine, skiperror, pUnit, pLand, params, Item, amount, "transport", NULL))
        {
            if (!pUnit->GetProperty(Item.GetData(), type, value, eNormal) || (eLong != type))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - Can not transport " << Item;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            long currentValue = static_cast<long>(reinterpret_cast<intptr_t>(value));
            long newValue = currentValue - amount;

            if (PE_OK != pUnit->SetProperty(Item.GetData(), type, reinterpret_cast<const void*>(static_cast<intptr_t>(newValue)), eNormal))
            {
                if (!skiperror)
                {
                    ErrorLine.Empty();
                    ErrorLine << Line << " - " << NOSET << BUG;
                    OrderErr(1, pUnit->Id, ErrorLine.GetData(), pUnit->Name.GetData(), pUnit);
                }
                break;
            }

            if (pUnit2)
            {
                if (!pUnit2->GetProperty(Item.GetData(), type, value2, eNormal))
                {
                    long zero = 0;
                    value2 = reinterpret_cast<const void*>(static_cast<intptr_t>(zero));
                    if (PE_OK != pUnit2->SetProperty(Item.GetData(), type, value2, eNormal))
                    {
                        if (!skiperror)
                        {
                            ErrorLine.Empty();
                            ErrorLine << Line << " - " << NOSETUNIT << n1 << BUG;
                            OrderErr(1, pUnit2->Id, ErrorLine.GetData(), pUnit2->Name.GetData(), pUnit2);
                        }
                        break;
                    }
                }
                else if (eLong != type)
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOTNUMERIC << n1 << BUG;
                        OrderErr(1, pUnit2->Id, ErrorLine.GetData(), pUnit2->Name.GetData(), pUnit2);
                    }
                    break;
                }

                long currentValue2 = static_cast<long>(reinterpret_cast<intptr_t>(value2));
                long newValue2 = currentValue2 + amount;

                if (PE_OK != pUnit2->SetProperty(Item.GetData(), type, reinterpret_cast<const void*>(static_cast<intptr_t>(newValue2)), eNormal))
                {
                    if (!skiperror)
                    {
                        ErrorLine.Empty();
                        ErrorLine << Line << " - " << NOSET << BUG;
                        OrderErr(1, pUnit2->Id, ErrorLine.GetData(), pUnit2->Name.GetData(), pUnit2);
                    }
                    break;
                }

                // Add comment to recipient's orders
                AddOrUpdateReceivedComment(pUnit2, pUnit, Item.GetData(), amount);
            }
        }
    } while (FALSE);
}

//-----------------------------------------------------------
// Helper: Adds or updates a comment about received items
//-----------------------------------------------------------

void CAtlaParser::AddOrUpdateReceivedComment(CUnit* pReceiver, CUnit* pSender,
    const char* itemCode, int amount)
{
    if (!pReceiver || !pSender || !itemCode || amount <= 0)
        return;

    CStr senderInfo;
    senderInfo.Format("%s (%d)", pSender->Name.GetData(), pSender->Id);

    CStr newComment;
    newComment.Format(";received %d %s from %s", amount, itemCode, senderInfo.GetData());

    const char* ordersData = pReceiver->Orders.GetData();
    if (ordersData && strstr(ordersData, newComment.GetData()))
        return;

    pReceiver->Orders.TrimRight(TRIM_ALL);
    if (!pReceiver->Orders.IsEmpty())
        pReceiver->Orders << EOL_SCR;
    pReceiver->Orders << newComment;
}

//--------------------------------------
// Helper: Finds a unit within specified radius
// Used by TRANSPORT command
//--------------------------------------

CUnit* CAtlaParser::FindUnitInRange(long unitId, CLand* pStartLand, int maxRadius)
{
    if (!pStartLand || unitId <= 0)
        return NULL;

    CBaseObject Dummy;
    Dummy.Id = unitId;
    int idx;

    // Check current region first
    if (pStartLand->Units.Search(&Dummy, idx))
        return (CUnit*)pStartLand->Units.At(idx);

    for (int i = 0; i < pStartLand->UnitsSeq.Count(); i++)
    {
        CUnit* pCheckUnit = (CUnit*)pStartLand->UnitsSeq.At(i);
        if (pCheckUnit->Id == unitId)
            return pCheckUnit;
    }

    // BFS search within radius
    CLongColl visitedLands;
    CBaseColl searchQueue;

    searchQueue.Insert(pStartLand);
    visitedLands.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(pStartLand->Id)));

    int currentRadius = 0;
    int elementsInCurrentRadius = 1;
    int elementsInNextRadius = 0;

    int queueIndex = 0;
    while (queueIndex < searchQueue.Count() && currentRadius < maxRadius)
    {
        CLand* pCurrentLand = (CLand*)searchQueue.At(queueIndex);
        queueIndex++;
        elementsInCurrentRadius--;

        // Check all neighbors
        for (int dir = 0; dir < 6; dir++)
        {
            CLand* pNeighbor = GetLandExit(pCurrentLand, dir);
            if (pNeighbor)
            {
                BOOL alreadyVisited = FALSE;
                for (int v = 0; v < visitedLands.Count(); v++)
                {
                    long visitedId = static_cast<long>(reinterpret_cast<uintptr_t>(visitedLands.At(v)));
                    if (visitedId == pNeighbor->Id)
                    {
                        alreadyVisited = TRUE;
                        break;
                    }
                }

                if (!alreadyVisited)
                {
                    Dummy.Id = unitId;
                    if (pNeighbor->Units.Search(&Dummy, idx))
                        return (CUnit*)pNeighbor->Units.At(idx);

                    for (int i = 0; i < pNeighbor->UnitsSeq.Count(); i++)
                    {
                        CUnit* pCheckUnit = (CUnit*)pNeighbor->UnitsSeq.At(i);
                        if (pCheckUnit->Id == unitId)
                            return pCheckUnit;
                    }

                    searchQueue.Insert(pNeighbor);
                    visitedLands.Insert(reinterpret_cast<void*>(static_cast<uintptr_t>(pNeighbor->Id)));
                    elementsInNextRadius++;
                }
            }
        }

        if (elementsInCurrentRadius == 0)
        {
            currentRadius++;
            elementsInCurrentRadius = elementsInNextRadius;
            elementsInNextRadius = 0;

            if (currentRadius >= maxRadius)
                break;
        }
    }

    // Fallback to full unit search
    for (int i = 0; i < m_Units.Count(); i++)
    {
        CUnit* pCheckUnit = (CUnit*)m_Units.At(i);
        if (pCheckUnit->Id == unitId)
            return pCheckUnit;
    }

    return NULL;
}

// Helper function to update a specific line in unit's orders
void CAtlaParser::UpdateUnitOrderLine(CUnit* pUnit, const char* newLine)
{
    if (!pUnit || !newLine)
        return;

    wxArrayString lines;
    wxStringTokenizer tkz(wxString::FromUTF8(pUnit->Orders.GetData()), "\n");
    while (tkz.HasMoreTokens())
        lines.Add(tkz.GetNextToken());

    wxString searchPattern = "STUDY";
    wxString newLineStr = wxString::FromUTF8(newLine).Upper();

    for (size_t i = 0; i < lines.size(); i++)
    {
        wxString lineUpper = lines[i].Upper();
        if (lineUpper.Contains(searchPattern))
        {
            lines[i] = wxString::FromUTF8(newLine);
            break;
        }
    }

    pUnit->Orders.Empty();
    for (size_t i = 0; i < lines.size(); i++)
    {
        if (i > 0)
            pUnit->Orders << EOL_SCR;
        pUnit->Orders << lines[i].ToUTF8().data();
    }
}