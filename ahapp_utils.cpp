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

#include "ahapp.h"

//-------------------------------------------------------------------------
// Font conversion utilities
//-------------------------------------------------------------------------

/**
 * Converts a wxFont object to a string representation for storage in config
 * Format: "size,family,style,weight,encoding,facename"
 */
void FontToStr(const wxFont * font, CStr & s)
{
    s.Empty();
    s << (long)font->GetPointSize()  << ","
      << (long)font->GetFamily   ()  << ","
      << (long)font->GetStyle    ()  << ","
      << (long)font->GetWeight   ()  << ","
      << (long)font->GetEncoding ()  << ","
      <<       font->GetFaceName ().mb_str() ;
}

//--------------------------------------------------------------------------

#if defined(_WIN32)
   #define AH_DEFAULT_FONT_SIZE 10  // Default font size on Windows
#else
   #define AH_DEFAULT_FONT_SIZE 12  // Default font size on other platforms
#endif

/**
 * Creates a wxFont object from a string representation
 * If string is empty, returns default font
 */
wxFont * NewFontFromStr(const char * p)
{
    int            size;
    wxFontFamily   family;
    wxFontStyle    style;
    wxFontWeight   weight;
    wxFontEncoding encoding;
    wxString       facename;
    wxFont     *   font;

    CStr           S;

    if (p && *p)
    {
        // Parse comma-separated font parameters
        p = S.GetToken(SkipSpaces(p), ',');  size     = atol(S.GetData());
        p = S.GetToken(SkipSpaces(p), ',');  family   = static_cast<wxFontFamily>(atol(S.GetData()));
        p = S.GetToken(SkipSpaces(p), ',');  style    = static_cast<wxFontStyle>( atol(S.GetData()));
        p = S.GetToken(SkipSpaces(p), ',');  weight   = static_cast<wxFontWeight>(atol(S.GetData()));
        p = S.GetToken(SkipSpaces(p), ',');  encoding = static_cast<wxFontEncoding>(atol(S.GetData()));
                                             facename = wxString::FromAscii(SkipSpaces(p));
    }
    else
    {
        // Use defaults
        size     = AH_DEFAULT_FONT_SIZE;
        family   = wxFONTFAMILY_DEFAULT;
        style    = wxFONTSTYLE_NORMAL;
        weight   = wxFONTWEIGHT_NORMAL;
        encoding = wxFONTENCODING_SYSTEM;
        facename = wxT("");
    }

    font = new wxFont(size, family, style, weight, FALSE, facename, encoding);

    return font;
}

//-------------------------------------------------------------------------
// Color conversion utilities
//-------------------------------------------------------------------------

/**
 * Converts a string "r,g,b" to a wxColour object
 */
void StrToColor(wxColour * cr, const char * p)
{
    CStr          S;
    int           r, g, b;

    p = S.GetToken(p, ',');
    r = atol(S.GetData());

    p = S.GetToken(p, ',');
    g = atol(S.GetData());

    p = S.GetToken(p, ',');
    b = atol(S.GetData());

    cr->Set(r,g,b);
}

//--------------------------------------------------------------------------

/**
 * Converts a wxColour object to string "r, g, b"
 */
void ColorToStr(char * p, wxColour * cr)
{
    sprintf(p, "%d, %d, %d",
            (int)(cr->Red()  ),
            (int)(cr->Green()),
            (int)(cr->Blue() )
        );
}

//-------------------------------------------------------------------------
// Path manipulation utilities
//-------------------------------------------------------------------------

#if defined(__WXMSW__)
  // Windows: case-insensitive path comparison
  #define EQUAL_PATH_CHARS(a, b) (tolower(a) == tolower(b))
#else
  // Unix: case-sensitive path comparison
  #define EQUAL_PATH_CHARS(a, b) (a == b)
#endif

#if defined(__WXMSW__)
  #define SEP '\\'  // Windows path separator
#else
  #define SEP '/'   // Unix path separator
#endif

/**
 * Converts absolute path to relative path based on current directory
 */
void MakePathRelative(const char * cur_dir, CStr & path)
{
    wxFileName filename(path.GetData());
    filename.MakeRelativeTo(cur_dir);
    path = (const char*)filename.GetFullPath().c_str();
}

//-------------------------------------------------------------------------

/**
 * Converts relative path to absolute path by prepending current directory
 */
void MakePathFull(const char * cur_dir, CStr & path)
{
    CStr full_path;
    CStr rel_path;

    full_path = cur_dir;
    rel_path = path;

    // Ensure trailing separator
    if (!full_path.IsEmpty() && full_path.GetData()[full_path.GetLength()-1] != SEP)
        full_path.AddCh( SEP);

    // Remove leading "./" if present
    if (!rel_path.IsEmpty())
    {
        if (rel_path.GetData()[0]=='.' && rel_path.GetData()[1]==SEP)
            rel_path.DelSubStr(0,2);
    }

    path = full_path;
    path << rel_path;
}

//-------------------------------------------------------------------------

/**
 * Extracts directory part from a full path
 */
void GetDirFromPath(const char * path, CStr & dir)
{
    int n = 0;
    const char * p;

    if (!path || !*path)
        return;

    dir = path;
    p   = dir.GetData() + (dir.GetLength()-1);
    
    // Find last separator
    while (*p!='\\' && *p!='/' && n<dir.GetLength())
    {
        p--;
        n++;
    }
    if (*p=='\\' || *p=='/')
        n++;

    if (n>0)
        dir.DelSubStr(dir.GetLength()-n, n);
    if (dir.IsEmpty())
        dir = ".";
}

//-------------------------------------------------------------------------

/**
 * Extracts filename part from a full path
 */
void GetFileFromPath(const char * path, CStr & file)
{
    const char * p = strrchr(path, SEP);

    file.Empty();
    if (p && *p)
    {
        p++;
        file = p;
    }
    else
        file = path;
}

//==========================================================================
// CGameDataHelper implementation - forwards calls to CAhApp
//==========================================================================

void  CGameDataHelper::ReportError(const char * msg, int msglen, BOOL orderrelated)
{
    gpApp->ShowError(msg, msglen, !orderrelated);
};

long  CGameDataHelper::GetStudyCost(const char * skill)
{
    return gpApp->GetStudyCost(skill);
};

long  CGameDataHelper::GetStructAttr(const char * kind, long & MaxLoad, long & MinSailingPower)
{
    return gpApp->GetStructAttr(kind, MaxLoad, MinSailingPower);
}

const char *  CGameDataHelper::ResolveAlias(const char * alias)
{
    return gpApp->ResolveAlias(alias);
}

BOOL CGameDataHelper::GetItemWeights(const char * item, int *& weights, const char **& movenames, int & movecount )
{
    // Resolve alias first, then get weights
    return gpApp->GetItemWeights(ResolveAlias(item), weights, movenames, movecount );
}

void CGameDataHelper::GetMoveNames(const char **& movenames)
{
    gpApp->GetMoveNames(movenames);
}

const char * CGameDataHelper::GetConfString(const char * section, const char * param)
{
    if (!section)
        section = SZ_SECT_COMMON;
    return gpApp->GetConfig(section, param);
}

BOOL CGameDataHelper::GetOrderId(const char * order, long & id)
{
    return gpApp->GetOrderId(order, id);
}

BOOL CGameDataHelper::IsTradeItem(const char * item)
{
    return gpApp->IsTradeItem(item);
}

BOOL CGameDataHelper::IsMan(const char * item)
{
    return gpApp->IsMan(item);
}

const char * CGameDataHelper::GetWeatherLine(BOOL IsCurrent, BOOL IsGood, int Zone)
{
    return gpApp->GetWeatherLine(IsCurrent, IsGood, Zone);
}

BOOL CGameDataHelper::GetTropicZone  (const char * plane, long & y_min, long & y_max)
{
    const char * value;
    CStr         S;

    value = SkipSpaces(gpApp->GetConfig(SZ_SECT_TROPIC_ZONE, plane));
    if (!value || !*value)
        return FALSE;

    // Parse "min,max" format
    value = S.GetToken(value, ',');
    y_min = atol(S.GetData());

    value = S.GetToken(value, ',');
    y_max = atol(S.GetData());

    return TRUE;
}

void CGameDataHelper::SetTropicZone  (const char * plane, long y_min, long y_max)
{
    CStr S;
    S << y_min << ',' << y_max;
    gpApp->SetConfig(SZ_SECT_TROPIC_ZONE, plane, S.GetData());
}

const char * CGameDataHelper::GetPlaneSize (const char * plane)
{
    return gpApp->GetConfig(SZ_SECT_PLANE_SIZE, plane);
}

void CGameDataHelper::GetProdDetails (const char * item, TProdDetails & details)
{
    gpApp->GetProdDetails (item, details);
}

long CGameDataHelper::MaxSkillLevel  (const char * race, const char * skill, const char * leadership, BOOL IsArcadiaSkillSystem)
{
    return gpApp->GetMaxRaceSkillLevel(race, skill, leadership, IsArcadiaSkillSystem);
}

BOOL CGameDataHelper::ImmediateProdCheck()
{
    return atol(gpApp->GetConfig(SZ_SECT_COMMON,  SZ_KEY_CHK_PROD_REQ));
}

BOOL CGameDataHelper::CanSeeAdvResources(const char * skillname, const char * terrain, CLongColl & Levels, CBufColl & Resources)
{
    return gpApp->CanSeeAdvResources(skillname, terrain, Levels, Resources);
}

int CGameDataHelper::GetAttitudeForFaction(int id)
{
    return gpApp->GetAttitudeForFaction(id);
}

void CGameDataHelper::SetAttitudeForFaction(int id, int attitude)
{
    gpApp->SetAttitudeForFaction(id, attitude);
}

void CGameDataHelper::SetPlayingFaction(long id)
{
    // Set playing faction to ATT_FRIEND2 (most friendly)
    gpApp->SetAttitudeForFaction(id, ATT_FRIEND2);
    gpApp->SetConfig(SZ_SECT_ATTITUDES, SZ_ATT_PLAYER_ID, id);
}

BOOL CGameDataHelper::ShowMoveWarnings()
{
    return atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_CHECK_MOVE_MODE));
}

BOOL CGameDataHelper::IsRawMagicSkill(const char * skillname)
{
    static int     postlen = strlen(PRP_SKILL_POSTFIX);
    CStr           S;

    S = skillname;
    // Check if this is a skill property (ends with PRP_SKILL_POSTFIX)
    if (S.FindSubStrR(PRP_SKILL_POSTFIX) == S.GetLength()-postlen)
    {
        S.DelSubStr(S.GetLength()-postlen, postlen);
        return gpApp->IsMagicSkill(S.GetData());  // Check base skill name
    }

    return FALSE;
}

/**
 * Checks if an item is a wagon (can carry other items)
 */
BOOL CGameDataHelper::IsWagon(const char * item)
{
    if (!item)
        return FALSE;
    CStr S = gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGONS);
    CStr T;
    const char * p = S.GetData();
    while (p && *p)
    {
        p = T.GetToken(p, ',', TRIM_ALL);
        if (0==stricmp(item, T.GetData()))
            return TRUE;
    }
    return FALSE;
}

/**
 * Checks if an item can pull wagons (e.g., horses)
 */
BOOL CGameDataHelper::IsWagonPuller(const char * item)
{
    if (!item)
        return FALSE;
    CStr S = gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_PULLERS);
    CStr T;
    const char * p = S.GetData();
    while (p && *p)
    {
        p = T.GetToken(p, ',', TRIM_ALL);
        if (0==stricmp(item, T.GetData()))
            return TRUE;
    }
    return FALSE;
}

/**
 * Returns the capacity of wagons (how much they can carry)
 */
int CGameDataHelper::WagonCapacity()
{
    return atol(gpApp->GetConfig(SZ_SECT_COMMON, SZ_KEY_WAGON_CAPACITY));
}

//-------------------------------------------------------------------------
// Reverse alias lookup
//-------------------------------------------------------------------------

/**
 * Gets the alias (key) for a given code (value) from [ALIASES] section
 * Example: if ALIASES has "combat = COMB", then GetAliasByCode("COMB") returns "combat"
 */
const char* CAhApp::GetAliasByCode(const char* code)
{
    if (!code || !*code)
        return code;

    const char* szName = NULL;
    const char* szValue = NULL;

    // Iterate through all entries in ALIASES section
    int idx = GetSectionFirst(SZ_SECT_ALIAS, szName, szValue);
    while (idx >= 0)
    {
        // If the value matches our code, return the key (alias)
        if (szValue && 0 == stricmp(szValue, code))
        {
            return szName;
        }
        idx = GetSectionNext(idx, SZ_SECT_ALIAS, szName, szValue);
    }

    // Not found, return the code itself
    return code;
}

const char* CGameDataHelper::GetAliasByCode(const char* code)
{
    return gpApp->GetAliasByCode(code);
}