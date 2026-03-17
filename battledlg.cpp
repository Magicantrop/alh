#include "stdafx.h"
#include "stdhdr.h"
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/tokenzr.h>

// RapidJSON includes
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "battledlg.h"
#include "ahapp.h"
#include "data.h"
#include "consts_ah.h"
#include "battleconverters.h"

//=============================================================================
// Helper function to check if a unit has magical skills
//=============================================================================

static bool HasMagicalSkills(const BattleUnit& unit)
{
    // List of known magic skill codes (matching the one in battleconverters.cpp)
    static const wxArrayString magicSkills = []() {
        wxArrayString skills;
        skills.Add("FIRE");
        skills.Add("FEAR");
        skills.Add("ESHI");
        skills.Add("FSHI");
        skills.Add("ILLU");
        skills.Add("SPIR");
        skills.Add("NECR");
        skills.Add("SUSK");
        skills.Add("WEAT");
        skills.Add("GATE");
        skills.Add("PHEN");
        skills.Add("PHDE");
        skills.Add("EART");
        skills.Add("FORC");
        skills.Add("PATT");
        skills.Add("OBSE");
        return skills;
    }();

    for (const auto& skill : unit.skills)
    {
        for (size_t i = 0; i < magicSkills.size(); i++)
        {
            if (skill.code == magicSkills[i])
                return true;
        }
    }
    return false;
}

//=============================================================================
// Static member initialization
//=============================================================================

BattleSide CBattleDlg::ms_attackers;
BattleSide CBattleDlg::ms_defenders;

//=============================================================================
// Event table - maps UI events to handler methods
//=============================================================================

BEGIN_EVENT_TABLE(CBattleDlg, wxDialog)
EVT_BUTTON(ID_AddItem, CBattleDlg::OnAddItem)
EVT_BUTTON(ID_RemoveItem, CBattleDlg::OnRemoveItem)
EVT_BUTTON(ID_AddSkill, CBattleDlg::OnAddSkill)
EVT_BUTTON(ID_RemoveSkill, CBattleDlg::OnRemoveSkill)
EVT_BUTTON(ID_AddAttacker, CBattleDlg::OnAddAttacker)
EVT_BUTTON(ID_AddDefender, CBattleDlg::OnAddDefender)
EVT_BUTTON(ID_UpdateUnit, CBattleDlg::OnUpdateUnit)
EVT_BUTTON(ID_ClearUnit, CBattleDlg::OnClearUnit)
EVT_BUTTON(ID_ClearAttackers, CBattleDlg::OnClearAttackers)
EVT_BUTTON(ID_ClearDefenders, CBattleDlg::OnClearDefenders)
EVT_BUTTON(ID_LoadJson, CBattleDlg::OnLoadJson)
EVT_BUTTON(ID_SaveJson, CBattleDlg::OnSaveJson)
EVT_BUTTON(wxID_OK, CBattleDlg::OnOk)
EVT_BUTTON(wxID_CANCEL, CBattleDlg::OnCancel)
EVT_LIST_ITEM_SELECTED(wxID_ANY, CBattleDlg::OnUnitSelected)
EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, CBattleDlg::OnUnitRightClick)
EVT_CHOICE(wxID_ANY, CBattleDlg::OnItemSelection)
EVT_CHOICE(wxID_ANY, CBattleDlg::OnSkillSelection)
EVT_MENU(wxID_ANY, CBattleDlg::OnUnitMenu)
END_EVENT_TABLE()

//=============================================================================
// Static methods
//=============================================================================

/**
 * Clears global battle data
 */
    void CBattleDlg::ClearGlobalData()
{
    ms_attackers.units.clear();
    ms_defenders.units.clear();
}

/**
 * Converts a game unit to battle unit format using Description (start of turn state)
 */
BattleUnit CBattleDlg::ConvertGameUnit(CUnit* pUnit, CAhApp* app)
{
    BattleUnit unit;

    if (!pUnit || !app)
        return unit;

    // Set name
    unit.name = wxString::FromUTF8(pUnit->Name.GetData());
    if (unit.name.IsEmpty())
        unit.name = wxString::Format("Unit %ld", pUnit->Id);

    // Get behind flag
    unit.behind = (pUnit->Flags & UNIT_FLAG_BEHIND) != 0;

    //=========================================================================
    // Extract ALL items from Description
    //=========================================================================

    // Get all items from description
    std::vector<TUnitItem> allItems = pUnit->GetItems(&pUnit->Description);

    // Convert all items, no filtering
    for (const auto& item : allItems)
    {
        BattleItem battleItem;
        battleItem.code = wxString::FromUTF8(item.itemCode.GetSafeCStr());
        battleItem.amount = item.itemCount;

        // Get alias for display
        const char* alias = app->ResolveAlias(battleItem.code.ToUTF8());
        battleItem.alias = alias ? wxString::FromUTF8(alias) : battleItem.code;

        unit.items.push_back(battleItem);
    }

    //=========================================================================
    // Extract skills
    //=========================================================================

    std::vector<TUnitSkill> allSkills = pUnit->GetAllSkills(&pUnit->Description);

    for (const auto& skill : allSkills)
    {
        if (skill.skillLevel > 0)
        {
            BattleSkill battleSkill;
            battleSkill.code = wxString::FromUTF8(skill.skillCode.GetSafeCStr());
            battleSkill.level = skill.skillLevel;

            const char* alias = app->ResolveAlias(battleSkill.code.ToUTF8());
            battleSkill.alias = alias ? wxString::FromUTF8(alias) : battleSkill.code;

            unit.skills.push_back(battleSkill);
        }
    }

    //=========================================================================
    // Combat spell
    //=========================================================================

    EValueType type;
    const void* value = nullptr;
    if (pUnit->GetProperty(PRP_COMBAT, type, value, eNormal) && type == eCharPtr)
    {
        const char* spell = static_cast<const char*>(value);
        if (spell && *spell)
        {
            unit.combatSpell = wxString::FromUTF8(spell);
        }
    }

    return unit;
}

/**
 * Adds a game unit to attackers
 */
bool CBattleDlg::AddUnitToAttackers(CUnit* pUnit)
{
    if (!pUnit || !gpApp)
        return false;

    BattleUnit unit = ConvertGameUnit(pUnit, gpApp);
    ms_attackers.units.push_back(unit);
    return true;
}

/**
 * Adds a game unit to defenders
 */
bool CBattleDlg::AddUnitToDefenders(CUnit* pUnit)
{
    if (!pUnit || !gpApp)
        return false;

    BattleUnit unit = ConvertGameUnit(pUnit, gpApp);
    ms_defenders.units.push_back(unit);
    return true;
}

//=============================================================================
// Constructor / Destructor
//=============================================================================

/**
 * Constructor - initializes dialog with data from application config
 */
CBattleDlg::CBattleDlg(wxWindow* parent, CAhApp* app)
    : wxDialog(parent, wxID_ANY, "Prepare Battle JSON", wxDefaultPosition, wxSize(1200, 800)),
    m_pApp(app),
    m_selectedSide(0),
    m_selectedUnitIdx(-1) 
{
    // Load configuration data from application
    LoadConfigData();

    // Load global data into local copies
    LoadGlobalData();

    // Create all UI controls
    CreateControls();

    // Initialize unit lists
    UpdateUnitLists();

    // Clear the unit editor
    ClearUnitEditor();
}

/**
 * Destructor
 */
CBattleDlg::~CBattleDlg()
{
    // Save local changes to global storage
    SaveGlobalData();
}

//=============================================================================
// Global data management
//=============================================================================

/**
 * Loads global data into local copies
 */
void CBattleDlg::LoadGlobalData()
{
    m_attackers = ms_attackers;
    m_defenders = ms_defenders;
}

/**
 * Saves local data to global storage
 */
void CBattleDlg::SaveGlobalData()
{
    ms_attackers = m_attackers;
    ms_defenders = m_defenders;
}


//=============================================================================
// Public methods
//=============================================================================

/**
 * Clears all units from both sides and resets editor
 */
void CBattleDlg::ClearAll()
{
    m_attackers.units.clear();
    m_defenders.units.clear();
    ClearUnitEditor();
    UpdateUnitLists();
}

//=============================================================================
// Data loading methods
//=============================================================================

/**
 * Loads all configuration data from application config files
 * This includes items, skills, and structures from various config sections
 */
void CBattleDlg::LoadConfigData()
{
    // Load items from various property groups
    LoadItemsFromGroup("men", m_itemAliases, m_itemCodes);
    LoadItemsFromGroup("armours", m_itemAliases, m_itemCodes);
    LoadItemsFromGroup("weapons", m_itemAliases, m_itemCodes);
    LoadItemsFromGroup("bows", m_itemAliases, m_itemCodes);
    LoadItemsFromGroup("mounts", m_itemAliases, m_itemCodes);
    LoadItemsFromGroup("mag_items", m_itemAliases, m_itemCodes);
    LoadItemsFromGroup("shields", m_itemAliases, m_itemCodes);
    LoadItemsFromGroup("monsters", m_itemAliases, m_itemCodes);

    // Load skills from skill groups
    LoadSkillsFromGroup("skills", m_skillAliases, m_skillCodes);
    LoadSkillsFromGroup("mag_skills", m_skillAliases, m_skillCodes);

    // Load structures from [STRUCTURES] section
    LoadStructures();

    // Load region types from [RESOURCE_LAND] section
    LoadRegionTypes(); 
}

/**
 * Loads items from a specific property group in the config
 * @param group Group name (e.g., "men", "weapons")
 * @param aliases Output array for display names (from aliases section)
 * @param codes Output array for item codes (original codes from config)
 */
void CBattleDlg::LoadItemsFromGroup(const wxString& group, wxArrayString& aliases, wxArrayString& codes)
{
    // Get comma-separated list of item codes for this group
    const char* groupItems = m_pApp->GetConfig(SZ_SECT_UNITPROP_GROUPS, group.ToUTF8());
    if (!groupItems || !*groupItems)
        return;
    
    CStr token(32);
    const char* p = groupItems;
    
    // Parse comma-separated list
    while (p && *p)
    {
        p = token.GetToken(p, ',');
        wxString code = wxString::FromUTF8(token.GetData()).Trim().Upper();
        if (code.IsEmpty())
            continue;
        
        // Get display alias for this code from [ALIASES] section
        const char* alias = m_pApp->ResolveAlias(code.ToUTF8());
        if (alias && *alias)
        {
            aliases.Add(wxString::FromUTF8(alias));
            codes.Add(code);
        }
        else
        {
            // If no alias found, use code as display name
            aliases.Add(code);
            codes.Add(code);
        }
    }
}

/**
 * Loads skills from a specific property group in the config
 * @param group Group name (e.g., "skills", "mag_skills")
 * @param aliases Output array for display names
 * @param codes Output array for skill codes
 */
void CBattleDlg::LoadSkillsFromGroup(const wxString& group, wxArrayString& aliases, wxArrayString& codes)
{
    const char* groupSkills = m_pApp->GetConfig(SZ_SECT_UNITPROP_GROUPS, group.ToUTF8());
    if (!groupSkills || !*groupSkills)
        return;
    
    CStr token(32);
    const char* p = groupSkills;
    
    while (p && *p)
    {
        p = token.GetToken(p, ',');
        wxString code = wxString::FromUTF8(token.GetData()).Trim().Upper();
        if (code.IsEmpty())
            continue;
        
        // Remove trailing underscore if present (skills in config often have _ suffix)
        if (code.EndsWith("_"))
            code = code.Left(code.Length() - 1);
        
        // Get display alias for this skill code
        const char* alias = m_pApp->ResolveAlias(code.ToUTF8());
        if (alias && *alias)
        {
            aliases.Add(wxString::FromUTF8(alias));
            codes.Add(code);
        }
        else
        {
            aliases.Add(code);
            codes.Add(code);
        }
    }
}

/**
 * Loads structure types from [STRUCTURES] section in config
 * Structures are used for units inside buildings (walls, towers, etc.)
 */
void CBattleDlg::LoadStructures()
{
    m_structureNames.Clear();
    m_structureCodes.Clear();
    
    // Add empty option for units not in any structure
    m_structureNames.Add("None");
    m_structureCodes.Add("");
    
    // Get all entries from [STRUCTURES] section
    const char* szName;
    const char* szValue;
    int idx = m_pApp->GetSectionFirst(SZ_SECT_STRUCTS, szName, szValue);
    
    while (idx >= 0)
    {
        m_structureNames.Add(wxString::FromUTF8(szName));
        m_structureCodes.Add(wxString::FromUTF8(szName));
        idx = m_pApp->GetSectionNext(idx, SZ_SECT_STRUCTS, szName, szValue);
    }
}

//=============================================================================
// UI creation methods
//=============================================================================

/**
 * Creates all dialog controls and arranges them in sizers
 * Organizes UI into three main sections:
 * 1. Two list controls showing attackers and defenders
 * 2. Unit editor panel for creating/modifying units
 * 3. Bottom button panel for JSON operations
 */
void CBattleDlg::CreateControls()
{
    // Increase window size for better display
    SetSize(1400, 900);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    //=========================================================================
    // Top section - Two lists side by side (attackers vs defenders)
    //=========================================================================
    wxBoxSizer* tablesSizer = new wxBoxSizer(wxHORIZONTAL);

    // Attackers list with header
    wxStaticBox* attackersBox = new wxStaticBox(this, wxID_ANY, "Attackers");
    wxStaticBoxSizer* attackersSizer = new wxStaticBoxSizer(attackersBox, wxVERTICAL);

    // Header with "Clear All" button
    wxBoxSizer* attackersHeaderSizer = new wxBoxSizer(wxHORIZONTAL);
    m_clearAttackersBtn = new wxButton(this, ID_ClearAttackers, "Clear All");
    attackersHeaderSizer->AddStretchSpacer();
    attackersHeaderSizer->Add(m_clearAttackersBtn, 0, wxALL, 2);
    attackersSizer->Add(attackersHeaderSizer, 0, wxEXPAND);

    // Attackers list control with optimized columns
    m_attackersList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(650, 400),
        wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_attackersList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 150);
    m_attackersList->AppendColumn("Behind", wxLIST_FORMAT_LEFT, 60);
    m_attackersList->AppendColumn("Structure", wxLIST_FORMAT_LEFT, 80);
    m_attackersList->AppendColumn("Items", wxLIST_FORMAT_LEFT, 180);
    m_attackersList->AppendColumn("Skills", wxLIST_FORMAT_LEFT, 180);
    attackersSizer->Add(m_attackersList, 1, wxEXPAND | wxALL, 5);
    tablesSizer->Add(attackersSizer, 1, wxEXPAND | wxALL, 5);

    // Defenders list with header
    wxStaticBox* defendersBox = new wxStaticBox(this, wxID_ANY, "Defenders");
    wxStaticBoxSizer* defendersSizer = new wxStaticBoxSizer(defendersBox, wxVERTICAL);

    wxBoxSizer* defendersHeaderSizer = new wxBoxSizer(wxHORIZONTAL);
    m_clearDefendersBtn = new wxButton(this, ID_ClearDefenders, "Clear All");
    defendersHeaderSizer->AddStretchSpacer();
    defendersHeaderSizer->Add(m_clearDefendersBtn, 0, wxALL, 2);
    defendersSizer->Add(defendersHeaderSizer, 0, wxEXPAND);

    m_defendersList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(650, 400),
        wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_defendersList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 150);
    m_defendersList->AppendColumn("Behind", wxLIST_FORMAT_LEFT, 60);
    m_defendersList->AppendColumn("Structure", wxLIST_FORMAT_LEFT, 80);
    m_defendersList->AppendColumn("Items", wxLIST_FORMAT_LEFT, 180);
    m_defendersList->AppendColumn("Skills", wxLIST_FORMAT_LEFT, 180);
    defendersSizer->Add(m_defendersList, 1, wxEXPAND | wxALL, 5);
    tablesSizer->Add(defendersSizer, 1, wxEXPAND | wxALL, 5);

    mainSizer->Add(tablesSizer, 1, wxEXPAND | wxALL, 5);

    //=========================================================================
    // JSON Format selection panel (above editor)
    //=========================================================================
    wxStaticBox* converterBox = new wxStaticBox(this, wxID_ANY, "JSON Format");
    wxStaticBoxSizer* converterSizer = new wxStaticBoxSizer(converterBox, wxHORIZONTAL);

    converterSizer->Add(new wxStaticText(this, wxID_ANY, "Format:"),
        0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    m_converterChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(150, -1));
    converterSizer->Add(m_converterChoice, 0, wxALL, 5);

    // Add simulation parameters
    converterSizer->Add(new wxStaticText(this, wxID_ANY, "Battles:"),
        0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    m_battlesSpin = new wxSpinCtrl(this, wxID_ANY, "10", wxDefaultPosition, wxSize(60, -1));
    m_battlesSpin->SetRange(1, 1000);
    converterSizer->Add(m_battlesSpin, 0, wxALL, 5);

    converterSizer->Add(new wxStaticText(this, wxID_ANY, "Seed:"),
        0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    m_seedSpin = new wxSpinCtrl(this, wxID_ANY, "42", wxDefaultPosition, wxSize(80, -1));
    m_seedSpin->SetRange(0, 999999);
    converterSizer->Add(m_seedSpin, 0, wxALL, 5);

    converterSizer->Add(new wxStaticText(this, wxID_ANY, "Region:"),
        0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    m_regionTypeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(100, -1));

    // Load region types from config
    LoadRegionTypes();
    for (size_t i = 0; i < m_regionTypes.size(); i++)
    {
        m_regionTypeChoice->Append(m_regionTypes[i]);
    }
    m_regionTypeChoice->SetSelection(0);

    converterSizer->Add(m_regionTypeChoice, 0, wxALL, 5);
    converterSizer->AddStretchSpacer();

    mainSizer->Add(converterSizer, 0, wxEXPAND | wxALL, 5);

    //=========================================================================
    // Middle section - Unit editor panel (reorganized)
    //=========================================================================
    wxStaticBox* editBox = new wxStaticBox(this, wxID_ANY, "Edit Unit");
    wxStaticBoxSizer* editSizer = new wxStaticBoxSizer(editBox, wxVERTICAL);

    // Top row: Basic properties
    wxFlexGridSizer* topGridSizer = new wxFlexGridSizer(2, 10, 10);
    topGridSizer->AddGrowableCol(1);

    // Unit name
    topGridSizer->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_unitNameCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(300, -1));
    topGridSizer->Add(m_unitNameCtrl, 1, wxEXPAND);

    // Structure selection
    topGridSizer->Add(new wxStaticText(this, wxID_ANY, "Structure:"), 0, wxALIGN_CENTER_VERTICAL);
    m_structureChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1), m_structureNames);
    m_structureChoice->SetSelection(0);
    topGridSizer->Add(m_structureChoice, 1, wxEXPAND);

    // Behind flag checkbox
    topGridSizer->Add(new wxStaticText(this, wxID_ANY, "Behind:"), 0, wxALIGN_CENTER_VERTICAL);
    m_behindCheck = new wxCheckBox(this, wxID_ANY, "Yes");
    topGridSizer->Add(m_behindCheck, 0, wxALIGN_LEFT);

    editSizer->Add(topGridSizer, 0, wxEXPAND | wxALL, 10);

    //=========================================================================
    // Two-column layout for Items and Skills
    //=========================================================================
    wxBoxSizer* twoColSizer = new wxBoxSizer(wxHORIZONTAL);

    // Left column: Items
    wxStaticBox* itemsBox = new wxStaticBox(this, wxID_ANY, "Items");
    wxStaticBoxSizer* itemsSizer = new wxStaticBoxSizer(itemsBox, wxVERTICAL);

    // Items list - increased size
    m_itemsList = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(300, 150));
    itemsSizer->Add(m_itemsList, 1, wxEXPAND | wxALL, 5);

    // Controls for adding items
    wxFlexGridSizer* itemCtrlSizer = new wxFlexGridSizer(2, 5, 5);

    itemCtrlSizer->Add(new wxStaticText(this, wxID_ANY, "Item:"), 0, wxALIGN_CENTER_VERTICAL);
    m_itemChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1), m_itemAliases);
    itemCtrlSizer->Add(m_itemChoice, 1, wxEXPAND);

    itemCtrlSizer->Add(new wxStaticText(this, wxID_ANY, "Amount:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* amountSizer = new wxBoxSizer(wxHORIZONTAL);
    m_itemAmountSpin = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxSize(70, -1));
    m_itemAmountSpin->SetRange(1, 9999);
    amountSizer->Add(m_itemAmountSpin, 0, wxRIGHT, 5);
    amountSizer->Add(new wxStaticText(this, wxID_ANY, "or custom:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_customItemCode = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
    m_customItemCode->SetHint("Code");
    amountSizer->Add(m_customItemCode, 1, wxEXPAND);
    itemCtrlSizer->Add(amountSizer, 1, wxEXPAND);

    itemsSizer->Add(itemCtrlSizer, 0, wxEXPAND | wxALL, 5);

    // Item buttons
    wxBoxSizer* itemButtonsSizer = new wxBoxSizer(wxHORIZONTAL);
    m_addItemBtn = new wxButton(this, ID_AddItem, "Add Item");
    m_removeItemBtn = new wxButton(this, ID_RemoveItem, "Remove Item");
    itemButtonsSizer->Add(m_addItemBtn, 0, wxRIGHT, 5);
    itemButtonsSizer->Add(m_removeItemBtn, 0);
    itemsSizer->Add(itemButtonsSizer, 0, wxALIGN_CENTER | wxALL, 5);

    twoColSizer->Add(itemsSizer, 1, wxEXPAND | wxRIGHT, 10);

    // Right column: Skills
    wxStaticBox* skillsBox = new wxStaticBox(this, wxID_ANY, "Skills");
    wxStaticBoxSizer* skillsSizer = new wxStaticBoxSizer(skillsBox, wxVERTICAL);

    // Skills list - increased size
    m_skillsList = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(300, 150));
    skillsSizer->Add(m_skillsList, 1, wxEXPAND | wxALL, 5);

    // Controls for adding skills
    wxFlexGridSizer* skillCtrlSizer = new wxFlexGridSizer(2, 5, 5);

    skillCtrlSizer->Add(new wxStaticText(this, wxID_ANY, "Skill:"), 0, wxALIGN_CENTER_VERTICAL);
    m_skillChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1), m_skillAliases);
    skillCtrlSizer->Add(m_skillChoice, 1, wxEXPAND);

    skillCtrlSizer->Add(new wxStaticText(this, wxID_ANY, "Level:"), 0, wxALIGN_CENTER_VERTICAL);
    m_skillLevelSpin = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxSize(70, -1));
    m_skillLevelSpin->SetRange(1, 10);
    skillCtrlSizer->Add(m_skillLevelSpin, 0);

    skillsSizer->Add(skillCtrlSizer, 0, wxEXPAND | wxALL, 5);

    // Combat spell selection
    wxBoxSizer* spellSizer = new wxBoxSizer(wxHORIZONTAL);
    spellSizer->Add(new wxStaticText(this, wxID_ANY, "Combat Spell:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_combatSpellChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
    spellSizer->Add(m_combatSpellChoice, 1, wxEXPAND);
    skillsSizer->Add(spellSizer, 0, wxEXPAND | wxALL, 5);

    // Skill buttons
    wxBoxSizer* skillButtonsSizer = new wxBoxSizer(wxHORIZONTAL);
    m_addSkillBtn = new wxButton(this, ID_AddSkill, "Add Skill");
    m_removeSkillBtn = new wxButton(this, ID_RemoveSkill, "Remove Skill");
    skillButtonsSizer->Add(m_addSkillBtn, 0, wxRIGHT, 5);
    skillButtonsSizer->Add(m_removeSkillBtn, 0);
    skillsSizer->Add(skillButtonsSizer, 0, wxALIGN_CENTER | wxALL, 5);

    twoColSizer->Add(skillsSizer, 1, wxEXPAND);

    editSizer->Add(twoColSizer, 1, wxEXPAND | wxALL, 5);

    //=========================================================================
    // Unit action buttons (centered below items and skills)
    //=========================================================================
    wxBoxSizer* unitButtonsSizer = new wxBoxSizer(wxHORIZONTAL);

    m_addAttackerBtn = new wxButton(this, ID_AddAttacker, "Add to Attackers");
    m_addDefenderBtn = new wxButton(this, ID_AddDefender, "Add to Defenders");
    m_updateUnitBtn = new wxButton(this, ID_UpdateUnit, "Update Unit");
    m_clearUnitBtn = new wxButton(this, ID_ClearUnit, "Clear Form");

    unitButtonsSizer->AddStretchSpacer();
    unitButtonsSizer->Add(m_addAttackerBtn, 0, wxRIGHT, 5);
    unitButtonsSizer->Add(m_addDefenderBtn, 0, wxRIGHT, 5);
    unitButtonsSizer->Add(m_updateUnitBtn, 0, wxRIGHT, 5);
    unitButtonsSizer->Add(m_clearUnitBtn, 0);
    unitButtonsSizer->AddStretchSpacer();

    editSizer->Add(unitButtonsSizer, 0, wxEXPAND | wxALL, 10);

    mainSizer->Add(editSizer, 0, wxEXPAND | wxALL, 5);

    //=========================================================================
    // Bottom section - JSON and dialog buttons
    //=========================================================================
    wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    m_loadBtn = new wxButton(this, ID_LoadJson, "Load JSON");
    m_saveBtn = new wxButton(this, ID_SaveJson, "Save JSON");
    m_okBtn = new wxButton(this, wxID_OK, "OK");
    m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");

    bottomSizer->Add(m_loadBtn, 0, wxALL, 5);
    bottomSizer->Add(m_saveBtn, 0, wxALL, 5);
    bottomSizer->AddStretchSpacer();
    bottomSizer->Add(m_okBtn, 0, wxALL, 5);
    bottomSizer->Add(m_cancelBtn, 0, wxALL, 5);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxALL, 5);

    SetSizer(mainSizer);
    Layout();
    Centre();

    // Update combat spell choice after UI is created
    UpdateCombatSpellChoice();

    // Register converters
    RegisterConverters();
}

//=============================================================================
// UI update methods
//=============================================================================

/**
 * Updates the combat spell dropdown with available skills
 * Adds an empty option at the beginning for "no spell"
 */
void CBattleDlg::UpdateCombatSpellChoice()
{
    m_combatSpellChoice->Clear();
    m_combatSpellChoice->Append("");
    for (size_t i = 0; i < m_skillAliases.size(); i++)
    {
        m_combatSpellChoice->Append(m_skillAliases[i]);
    }
}

/**
 * Updates both attacker and defender list controls with current data
 * Formats items and skills as readable strings for display
 */
void CBattleDlg::UpdateUnitLists()
{
    m_attackersList->DeleteAllItems();
    m_defendersList->DeleteAllItems();

    // Update attackers list
    for (size_t i = 0; i < m_attackers.units.size(); i++)
    {
        const BattleUnit& unit = m_attackers.units[i];
        long idx = m_attackersList->InsertItem(i, unit.name);

        // Column 1: Behind flag
        m_attackersList->SetItem(idx, 1, unit.behind ? wxString("Yes") : wxString("No"), -1);

        // Column 2: Structure
        if (unit.structure.IsEmpty())
            m_attackersList->SetItem(idx, 2, "-");
        else
            m_attackersList->SetItem(idx, 2, unit.structure);

        // Column 3: Items
        wxString itemsStr;
        for (size_t j = 0; j < unit.items.size(); j++)
        {
            if (j > 0) itemsStr += ", ";
            itemsStr += wxString::Format("%d %s", unit.items[j].amount, unit.items[j].alias);
        }
        m_attackersList->SetItem(idx, 3, itemsStr);

        // Column 4: Skills
        wxString skillsStr;
        for (size_t j = 0; j < unit.skills.size(); j++)
        {
            if (j > 0) skillsStr += ", ";
            skillsStr += wxString::Format("%s %d", unit.skills[j].alias, unit.skills[j].level);
        }
        if (!unit.combatSpell.IsEmpty())
        {
            if (!skillsStr.IsEmpty()) skillsStr += "; ";
            skillsStr += "Combat: " + unit.combatSpell;
        }
        m_attackersList->SetItem(idx, 4, skillsStr);

        m_attackersList->SetItemData(idx, i);
    }

    // Update defenders list (same format as attackers)
    for (size_t i = 0; i < m_defenders.units.size(); i++)
    {
        const BattleUnit& unit = m_defenders.units[i];
        long idx = m_defendersList->InsertItem(i, unit.name);

        // Column 1: Behind flag
        m_defendersList->SetItem(idx, 1, unit.behind ? wxString("Yes") : wxString("No"), -1);

        // Column 2: Structure
        if (unit.structure.IsEmpty())
            m_defendersList->SetItem(idx, 2, "-");
        else
            m_defendersList->SetItem(idx, 2, unit.structure);

        // Column 3: Items
        wxString itemsStr;
        for (size_t j = 0; j < unit.items.size(); j++)
        {
            if (j > 0) itemsStr += ", ";
            itemsStr += wxString::Format("%d %s", unit.items[j].amount, unit.items[j].alias);
        }
        m_defendersList->SetItem(idx, 3, itemsStr);

        // Column 4: Skills
        wxString skillsStr;
        for (size_t j = 0; j < unit.skills.size(); j++)
        {
            if (j > 0) skillsStr += ", ";
            skillsStr += wxString::Format("%s %d", unit.skills[j].alias, unit.skills[j].level);
        }
        if (!unit.combatSpell.IsEmpty())
        {
            if (!skillsStr.IsEmpty()) skillsStr += "; ";
            skillsStr += "Combat: " + unit.combatSpell;
        }
        m_defendersList->SetItem(idx, 4, skillsStr);

        m_defendersList->SetItemData(idx, i);
    }
}

/**
 * Updates the items list in the editor to reflect current m_editUnit items
 */
void CBattleDlg::UpdateItemsList()
{
    m_itemsList->Clear();
    for (size_t i = 0; i < m_editUnit.items.size(); i++)
    {
        wxString itemStr = wxString::Format("%d x %s [%s]", 
            m_editUnit.items[i].amount, m_editUnit.items[i].alias, m_editUnit.items[i].code);
        m_itemsList->Append(itemStr);
    }
}

/**
 * Updates the skills list in the editor to reflect current m_editUnit skills
 */
void CBattleDlg::UpdateSkillsList()
{
    m_skillsList->Clear();
    for (size_t i = 0; i < m_editUnit.skills.size(); i++)
    {
        wxString skillStr = wxString::Format("%s [%s] level %d",
            m_editUnit.skills[i].alias, m_editUnit.skills[i].code, m_editUnit.skills[i].level);
        m_skillsList->Append(skillStr);
    }
}

//=============================================================================
// Unit editor methods
//=============================================================================

/**
 * Clears all fields in the unit editor and resets the temporary unit
 */
void CBattleDlg::ClearUnitEditor()
{
    m_editUnit = BattleUnit();
    m_editUnit.name = "Unit";
    
    m_unitNameCtrl->SetValue(m_editUnit.name);
    m_structureChoice->SetSelection(0);
    m_behindCheck->SetValue(false);
    m_itemsList->Clear();
    m_skillsList->Clear();
    m_combatSpellChoice->SetSelection(0);
    m_selectedSide = 0;
    m_selectedUnitIdx = -1;
}

/**
 * Fills the editor with data from an existing unit
 * @param unit Unit to display in editor
 */
void CBattleDlg::FillUnitEditor(const BattleUnit& unit)
{
    m_editUnit = unit;
    
    m_unitNameCtrl->SetValue(m_editUnit.name);
    
    int sel = m_structureChoice->FindString(m_editUnit.structure);
    if (sel != wxNOT_FOUND)
        m_structureChoice->SetSelection(sel);
    else
        m_structureChoice->SetSelection(0);
    
    m_behindCheck->SetValue(m_editUnit.behind);
    
    UpdateItemsList();
    UpdateSkillsList();
    
    if (!m_editUnit.combatSpell.IsEmpty())
    {
        sel = m_combatSpellChoice->FindString(m_editUnit.combatSpell);
        if (sel != wxNOT_FOUND)
            m_combatSpellChoice->SetSelection(sel);
        else
            m_combatSpellChoice->SetSelection(0);
    }
    else
    {
        m_combatSpellChoice->SetSelection(0);
    }
}

/**
 * Saves current editor values to the temporary unit
 * Called before operations that need the latest editor state
 */
void CBattleDlg::SaveUnitToEditor()
{
    m_editUnit.name = m_unitNameCtrl->GetValue();
    if (m_editUnit.name.IsEmpty())
        m_editUnit.name = "Unit";
    
    int sel = m_structureChoice->GetSelection();
    if (sel >= 0 && sel < (int)m_structureCodes.size())
        m_editUnit.structure = m_structureCodes[sel];
    
    m_editUnit.behind = m_behindCheck->GetValue();
    
    sel = m_combatSpellChoice->GetSelection();
    if (sel > 0 && sel <= (int)m_skillAliases.size())
        m_editUnit.combatSpell = m_skillAliases[sel - 1];
    else
        m_editUnit.combatSpell.Empty();
}

/**
 * Gets the unit currently being edited
 * @return Current unit from editor
 */
BattleUnit CBattleDlg::GetUnitFromEditor()
{
    SaveUnitToEditor();
    return m_editUnit;
}

/**
 * Validates that a unit has required minimum data
 * @param unit Unit to validate
 * @return true if unit has at least a name and one item
 */
bool CBattleDlg::ValidateUnit(const BattleUnit& unit)
{
    if (unit.name.IsEmpty())
        return false;
    if (unit.items.empty())
    {
        wxMessageBox("Unit must have at least one item!", "Validation Error", wxOK | wxICON_WARNING);
        return false;
    }
    return true;
}

//=============================================================================
// Unit management methods
//=============================================================================

/**
 * Adds current editor unit to specified side
 * @param side 0=attackers, 1=defenders
 */
void CBattleDlg::AddUnitToSide(int side)
{
    SaveUnitToEditor();
    
    if (!ValidateUnit(m_editUnit))
        return;
    
    if (side == 0) // Attackers
    {
        m_attackers.units.push_back(m_editUnit);
    }
    else // Defenders
    {
        m_defenders.units.push_back(m_editUnit);
    }
    
    UpdateUnitLists();
    ClearUnitEditor();
}

/**
 * Updates currently selected unit with editor data
 */
void CBattleDlg::UpdateCurrentUnit()
{
    if (m_selectedUnitIdx < 0)
    {
        wxMessageBox("No unit selected!", "Information", wxOK | wxICON_INFORMATION);
        return;
    }
    
    SaveUnitToEditor();
    
    if (!ValidateUnit(m_editUnit))
        return;
    
    if (m_selectedSide == 0 && m_selectedUnitIdx < (int)m_attackers.units.size())
    {
        m_attackers.units[m_selectedUnitIdx] = m_editUnit;
    }
    else if (m_selectedSide == 1 && m_selectedUnitIdx < (int)m_defenders.units.size())
    {
        m_defenders.units[m_selectedUnitIdx] = m_editUnit;
    }
    
    UpdateUnitLists();
    ClearUnitEditor();
}

//=============================================================================
// JSON operations using RapidJSON
//=============================================================================

/**
 * Converts RapidJSON value to wxString
 */
static wxString JsonValueToWxString(const rapidjson::Value& value)
{
    if (value.IsString())
        return wxString::FromUTF8(value.GetString());
    return wxEmptyString;
}

/**
 * Loads battle data from JSON file using RapidJSON
 * Expected format matches the example in example.json
 * @param filename Path to JSON file
 * @return true on success
 */
bool CBattleDlg::LoadFromJson(const wxString& filename)
{
    // Open file
    FILE* fp = fopen(filename.ToUTF8(), "rb");
    if (!fp)
        return false;
    
    // Read file into buffer
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    // Parse JSON
    rapidjson::Document doc;
    doc.ParseStream(is);
    
    fclose(fp);
    
    // Check for parse errors
    if (doc.HasParseError())
        return false;
    
    // Clear current data
    ClearAll();
    
    // Load attackers
    if (doc.HasMember("attackers") && doc["attackers"].IsObject())
    {
        const rapidjson::Value& attackers = doc["attackers"];
        if (attackers.HasMember("units") && attackers["units"].IsArray())
        {
            const rapidjson::Value& units = attackers["units"];
            for (rapidjson::SizeType i = 0; i < units.Size(); i++)
            {
                const rapidjson::Value& unitJson = units[i];
                BattleUnit unit;
                
                // Parse name
                if (unitJson.HasMember("name") && unitJson["name"].IsString())
                    unit.name = JsonValueToWxString(unitJson["name"]);
                
                // Parse flags
                if (unitJson.HasMember("flags") && unitJson["flags"].IsArray())
                {
                    const rapidjson::Value& flags = unitJson["flags"];
                    for (rapidjson::SizeType j = 0; j < flags.Size(); j++)
                    {
                        if (flags[j].IsString() && 
                            wxString(flags[j].GetString()).Lower() == "behind")
                        {
                            unit.behind = true;
                        }
                    }
                }
                
                // Parse items
                if (unitJson.HasMember("items") && unitJson["items"].IsArray())
                {
                    const rapidjson::Value& items = unitJson["items"];
                    for (rapidjson::SizeType j = 0; j < items.Size(); j++)
                    {
                        const rapidjson::Value& itemJson = items[j];
                        if (itemJson.IsObject())
                        {
                            BattleItem item;
                            
                            if (itemJson.HasMember("abbr") && itemJson["abbr"].IsString())
                                item.code = JsonValueToWxString(itemJson["abbr"]).Upper();
                            
                            if (itemJson.HasMember("amount") && itemJson["amount"].IsInt())
                                item.amount = itemJson["amount"].GetInt();
                            
                            // Get alias for this code
                            const char* alias = m_pApp->ResolveAlias(item.code.ToUTF8());
                            item.alias = alias ? wxString::FromUTF8(alias) : item.code;
                            
                            unit.items.push_back(item);
                        }
                    }
                }
                
                // Parse skills
                if (unitJson.HasMember("skills") && unitJson["skills"].IsArray())
                {
                    const rapidjson::Value& skills = unitJson["skills"];
                    for (rapidjson::SizeType j = 0; j < skills.Size(); j++)
                    {
                        const rapidjson::Value& skillJson = skills[j];
                        if (skillJson.IsObject())
                        {
                            BattleSkill skill;
                            
                            if (skillJson.HasMember("abbr") && skillJson["abbr"].IsString())
                                skill.code = JsonValueToWxString(skillJson["abbr"]).Upper();
                            
                            if (skillJson.HasMember("level") && skillJson["level"].IsInt())
                                skill.level = skillJson["level"].GetInt();
                            
                            // Get alias for this skill code
                            const char* alias = m_pApp->ResolveAlias(skill.code.ToUTF8());
                            skill.alias = alias ? wxString::FromUTF8(alias) : skill.code;
                            
                            unit.skills.push_back(skill);
                        }
                    }
                }
                
                // Parse combat spell
                if (unitJson.HasMember("combatSpell") && unitJson["combatSpell"].IsString())
                    unit.combatSpell = JsonValueToWxString(unitJson["combatSpell"]);
                
                m_attackers.units.push_back(unit);
            }
        }
    }
    
    // Load defenders (same format as attackers)
    if (doc.HasMember("defenders") && doc["defenders"].IsObject())
    {
        const rapidjson::Value& defenders = doc["defenders"];
        if (defenders.HasMember("units") && defenders["units"].IsArray())
        {
            const rapidjson::Value& units = defenders["units"];
            for (rapidjson::SizeType i = 0; i < units.Size(); i++)
            {
                const rapidjson::Value& unitJson = units[i];
                BattleUnit unit;
                
                if (unitJson.HasMember("name") && unitJson["name"].IsString())
                    unit.name = JsonValueToWxString(unitJson["name"]);
                
                if (unitJson.HasMember("flags") && unitJson["flags"].IsArray())
                {
                    const rapidjson::Value& flags = unitJson["flags"];
                    for (rapidjson::SizeType j = 0; j < flags.Size(); j++)
                    {
                        if (flags[j].IsString() && 
                            wxString(flags[j].GetString()).Lower() == "behind")
                        {
                            unit.behind = true;
                        }
                    }
                }
                
                if (unitJson.HasMember("items") && unitJson["items"].IsArray())
                {
                    const rapidjson::Value& items = unitJson["items"];
                    for (rapidjson::SizeType j = 0; j < items.Size(); j++)
                    {
                        const rapidjson::Value& itemJson = items[j];
                        if (itemJson.IsObject())
                        {
                            BattleItem item;
                            
                            if (itemJson.HasMember("abbr") && itemJson["abbr"].IsString())
                                item.code = JsonValueToWxString(itemJson["abbr"]).Upper();
                            
                            if (itemJson.HasMember("amount") && itemJson["amount"].IsInt())
                                item.amount = itemJson["amount"].GetInt();
                            
                            const char* alias = m_pApp->ResolveAlias(item.code.ToUTF8());
                            item.alias = alias ? wxString::FromUTF8(alias) : item.code;
                            
                            unit.items.push_back(item);
                        }
                    }
                }
                
                if (unitJson.HasMember("skills") && unitJson["skills"].IsArray())
                {
                    const rapidjson::Value& skills = unitJson["skills"];
                    for (rapidjson::SizeType j = 0; j < skills.Size(); j++)
                    {
                        const rapidjson::Value& skillJson = skills[j];
                        if (skillJson.IsObject())
                        {
                            BattleSkill skill;
                            
                            if (skillJson.HasMember("abbr") && skillJson["abbr"].IsString())
                                skill.code = JsonValueToWxString(skillJson["abbr"]).Upper();
                            
                            if (skillJson.HasMember("level") && skillJson["level"].IsInt())
                                skill.level = skillJson["level"].GetInt();
                            
                            const char* alias = m_pApp->ResolveAlias(skill.code.ToUTF8());
                            skill.alias = alias ? wxString::FromUTF8(alias) : skill.code;
                            
                            unit.skills.push_back(skill);
                        }
                    }
                }
                
                if (unitJson.HasMember("combatSpell") && unitJson["combatSpell"].IsString())
                    unit.combatSpell = JsonValueToWxString(unitJson["combatSpell"]);
                
                m_defenders.units.push_back(unit);
            }
        }
    }
    
    return true;
}

/**
 * Saves current battle data to JSON file using RapidJSON
 * Format matches the example in example.json
 * @param filename Path to JSON file
 * @return true on success
 */
bool CBattleDlg::SaveToJson(const wxString& filename)
{
    // Create JSON document
    rapidjson::Document doc;
    doc.SetObject();
    
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    
    //=========================================================================
    // Build attackers section
    //=========================================================================
    rapidjson::Value attackers(rapidjson::kObjectType);
    rapidjson::Value attackerUnits(rapidjson::kArrayType);
    
    for (size_t i = 0; i < m_attackers.units.size(); i++)
    {
        const BattleUnit& unit = m_attackers.units[i];
        rapidjson::Value unitJson(rapidjson::kObjectType);
        
        // Name
        unitJson.AddMember("name", 
            rapidjson::Value(unit.name.ToUTF8(), allocator).Move(), 
            allocator);
        
        // Flags
        rapidjson::Value flags(rapidjson::kArrayType);
        if (unit.behind)
        {
            flags.PushBack(rapidjson::Value("behind", allocator).Move(), allocator);
        }
        unitJson.AddMember("flags", flags, allocator);
        
        // Items
        rapidjson::Value items(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.items.size(); j++)
        {
            rapidjson::Value itemJson(rapidjson::kObjectType);
            itemJson.AddMember("abbr", 
                rapidjson::Value(unit.items[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            itemJson.AddMember("amount", unit.items[j].amount, allocator);
            items.PushBack(itemJson, allocator);
        }
        unitJson.AddMember("items", items, allocator);
        
        // Skills
        rapidjson::Value skills(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.skills.size(); j++)
        {
            rapidjson::Value skillJson(rapidjson::kObjectType);
            skillJson.AddMember("abbr", 
                rapidjson::Value(unit.skills[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            skillJson.AddMember("level", unit.skills[j].level, allocator);
            skills.PushBack(skillJson, allocator);
        }
        unitJson.AddMember("skills", skills, allocator);
        
        // Check if unit has magical skills
        bool isMage = HasMagicalSkills(unit);
        
        // Add mage flag for simulation format
        unitJson.AddMember("mage", isMage, allocator);
        
        // Combat spell - always include for mages
        if (!unit.combatSpell.IsEmpty() || isMage)
        {
            // Use combatSpell (no underscore) as in example.json
            rapidjson::Value spellValue(unit.combatSpell.IsEmpty() ? "" : unit.combatSpell.ToUTF8(), allocator);
            unitJson.AddMember("combatSpell", spellValue, allocator);
        }
        
        attackerUnits.PushBack(unitJson, allocator);
    }
    
    attackers.AddMember("units", attackerUnits, allocator);
    doc.AddMember("attackers", attackers, allocator);
    
    //=========================================================================
    // Build defenders section (same format)
    //=========================================================================
    rapidjson::Value defenders(rapidjson::kObjectType);
    rapidjson::Value defenderUnits(rapidjson::kArrayType);
    
    for (size_t i = 0; i < m_defenders.units.size(); i++)
    {
        const BattleUnit& unit = m_defenders.units[i];
        rapidjson::Value unitJson(rapidjson::kObjectType);
        
        unitJson.AddMember("name", 
            rapidjson::Value(unit.name.ToUTF8(), allocator).Move(), 
            allocator);
        
        rapidjson::Value flags(rapidjson::kArrayType);
        if (unit.behind)
        {
            flags.PushBack(rapidjson::Value("behind", allocator).Move(), allocator);
        }
        unitJson.AddMember("flags", flags, allocator);
        
        rapidjson::Value items(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.items.size(); j++)
        {
            rapidjson::Value itemJson(rapidjson::kObjectType);
            itemJson.AddMember("abbr", 
                rapidjson::Value(unit.items[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            itemJson.AddMember("amount", unit.items[j].amount, allocator);
            items.PushBack(itemJson, allocator);
        }
        unitJson.AddMember("items", items, allocator);
        
        rapidjson::Value skills(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.skills.size(); j++)
        {
            rapidjson::Value skillJson(rapidjson::kObjectType);
            skillJson.AddMember("abbr", 
                rapidjson::Value(unit.skills[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            skillJson.AddMember("level", unit.skills[j].level, allocator);
            skills.PushBack(skillJson, allocator);
        }
        unitJson.AddMember("skills", skills, allocator);
        
        // Check if unit has magical skills
        bool isMage = HasMagicalSkills(unit);
        
        // Add mage flag for simulation format
        unitJson.AddMember("mage", isMage, allocator);
        
        // Combat spell - always include for mages
        if (!unit.combatSpell.IsEmpty() || isMage)
        {
            // Use combatSpell (no underscore) as in example.json
            rapidjson::Value spellValue(unit.combatSpell.IsEmpty() ? "" : unit.combatSpell.ToUTF8(), allocator);
            unitJson.AddMember("combatSpell", spellValue, allocator);
        }
        
        defenderUnits.PushBack(unitJson, allocator);
    }
    
    defenders.AddMember("units", defenderUnits, allocator);
    doc.AddMember("defenders", defenders, allocator);
    
    //=========================================================================
    // Write to file
    //=========================================================================
    FILE* fp = fopen(filename.ToUTF8(), "wb");
    if (!fp)
        return false;
    
    char writeBuffer[65536];
    rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    
    rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
    doc.Accept(writer);
    
    fclose(fp);
    return true;
}

/**
 * Generates JSON string from current battle data
 * @return Formatted JSON string
 */
wxString CBattleDlg::GenerateJson()
{
    // Create JSON document
    rapidjson::Document doc;
    doc.SetObject();
    
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    
    // Build attackers
    rapidjson::Value attackers(rapidjson::kObjectType);
    rapidjson::Value attackerUnits(rapidjson::kArrayType);
    
    for (size_t i = 0; i < m_attackers.units.size(); i++)
    {
        const BattleUnit& unit = m_attackers.units[i];
        rapidjson::Value unitJson(rapidjson::kObjectType);
        
        unitJson.AddMember("name", 
            rapidjson::Value(unit.name.ToUTF8(), allocator).Move(), 
            allocator);
        
        rapidjson::Value flags(rapidjson::kArrayType);
        if (unit.behind)
        {
            flags.PushBack(rapidjson::Value("behind", allocator).Move(), allocator);
        }
        unitJson.AddMember("flags", flags, allocator);
        
        rapidjson::Value items(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.items.size(); j++)
        {
            rapidjson::Value itemJson(rapidjson::kObjectType);
            itemJson.AddMember("abbr", 
                rapidjson::Value(unit.items[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            itemJson.AddMember("amount", unit.items[j].amount, allocator);
            items.PushBack(itemJson, allocator);
        }
        unitJson.AddMember("items", items, allocator);
        
        rapidjson::Value skills(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.skills.size(); j++)
        {
            rapidjson::Value skillJson(rapidjson::kObjectType);
            skillJson.AddMember("abbr", 
                rapidjson::Value(unit.skills[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            skillJson.AddMember("level", unit.skills[j].level, allocator);
            skills.PushBack(skillJson, allocator);
        }
        unitJson.AddMember("skills", skills, allocator);
        
        // Check if unit has magical skills
        bool isMage = HasMagicalSkills(unit);
        
        // Add mage flag for simulation format
        unitJson.AddMember("mage", isMage, allocator);
        
        // Combat spell - always include for mages
        if (!unit.combatSpell.IsEmpty() || isMage)
        {
            // Use combatSpell (no underscore) as in example.json
            rapidjson::Value spellValue(unit.combatSpell.IsEmpty() ? "" : unit.combatSpell.ToUTF8(), allocator);
            unitJson.AddMember("combatSpell", spellValue, allocator);
        }
        
        attackerUnits.PushBack(unitJson, allocator);
    }
    
    attackers.AddMember("units", attackerUnits, allocator);
    doc.AddMember("attackers", attackers, allocator);
    
    // Build defenders
    rapidjson::Value defenders(rapidjson::kObjectType);
    rapidjson::Value defenderUnits(rapidjson::kArrayType);
    
    for (size_t i = 0; i < m_defenders.units.size(); i++)
    {
        const BattleUnit& unit = m_defenders.units[i];
        rapidjson::Value unitJson(rapidjson::kObjectType);
        
        unitJson.AddMember("name", 
            rapidjson::Value(unit.name.ToUTF8(), allocator).Move(), 
            allocator);
        
        rapidjson::Value flags(rapidjson::kArrayType);
        if (unit.behind)
        {
            flags.PushBack(rapidjson::Value("behind", allocator).Move(), allocator);
        }
        unitJson.AddMember("flags", flags, allocator);
        
        rapidjson::Value items(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.items.size(); j++)
        {
            rapidjson::Value itemJson(rapidjson::kObjectType);
            itemJson.AddMember("abbr", 
                rapidjson::Value(unit.items[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            itemJson.AddMember("amount", unit.items[j].amount, allocator);
            items.PushBack(itemJson, allocator);
        }
        unitJson.AddMember("items", items, allocator);
        
        rapidjson::Value skills(rapidjson::kArrayType);
        for (size_t j = 0; j < unit.skills.size(); j++)
        {
            rapidjson::Value skillJson(rapidjson::kObjectType);
            skillJson.AddMember("abbr", 
                rapidjson::Value(unit.skills[j].code.ToUTF8(), allocator).Move(), 
                allocator);
            skillJson.AddMember("level", unit.skills[j].level, allocator);
            skills.PushBack(skillJson, allocator);
        }
        unitJson.AddMember("skills", skills, allocator);
        
        // Check if unit has magical skills
        bool isMage = HasMagicalSkills(unit);
        
        // Add mage flag for simulation format
        unitJson.AddMember("mage", isMage, allocator);
        
        // Combat spell - always include for mages
        if (!unit.combatSpell.IsEmpty() || isMage)
        {
            // Use combatSpell (no underscore) as in example.json
            rapidjson::Value spellValue(unit.combatSpell.IsEmpty() ? "" : unit.combatSpell.ToUTF8(), allocator);
            unitJson.AddMember("combatSpell", spellValue, allocator);
        }
        
        defenderUnits.PushBack(unitJson, allocator);
    }
    
    defenders.AddMember("units", defenderUnits, allocator);
    doc.AddMember("defenders", defenders, allocator);
    
    // Convert to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    return wxString::FromUTF8(buffer.GetString());
}

//=============================================================================
// Event handlers
//=============================================================================

/**
 * Handles Add Item button click
 * Adds a new item to the current unit or updates quantity of existing item
 */
void CBattleDlg::OnAddItem(wxCommandEvent& event)
{
    if (event.GetId() == m_addItemBtn->GetId())
    {
        int sel = m_itemChoice->GetSelection();
        wxString code;
        wxString alias;
        
        // Check if custom code is provided
        if (!m_customItemCode->GetValue().IsEmpty())
        {
            code = m_customItemCode->GetValue().Upper();
            alias = code;
        }
        else if (sel >= 0 && sel < (int)m_itemCodes.size())
        {
            code = m_itemCodes[sel];
            alias = m_itemAliases[sel];
        }
        else
        {
            wxMessageBox("Please select an item or enter custom code", "Information", wxOK | wxICON_INFORMATION);
            return;
        }
        
        int amount = m_itemAmountSpin->GetValue();
        
        SaveUnitToEditor();
        
        // Check if item already exists in unit
        bool found = false;
        for (size_t i = 0; i < m_editUnit.items.size(); i++)
        {
            if (m_editUnit.items[i].code == code)
            {
                m_editUnit.items[i].amount += amount;
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            m_editUnit.items.push_back(BattleItem(code, alias, amount));
        }
        
        UpdateItemsList();
    }
}

/**
 * Handles Remove Item button click
 * Removes selected item from current unit
 */
void CBattleDlg::OnRemoveItem(wxCommandEvent& event)
{
    if (event.GetId() == m_removeItemBtn->GetId())
    {
        int sel = m_itemsList->GetSelection();
        if (sel != wxNOT_FOUND && sel >= 0 && sel < (int)m_editUnit.items.size())
        {
            SaveUnitToEditor();
            m_editUnit.items.erase(m_editUnit.items.begin() + sel);
            UpdateItemsList();
        }
    }
}

/**
 * Handles Add Skill button click
 * Adds a new skill to the current unit or updates level of existing skill
 */
void CBattleDlg::OnAddSkill(wxCommandEvent& event)
{
    if (event.GetId() == m_addSkillBtn->GetId())
    {
        int sel = m_skillChoice->GetSelection();
        if (sel >= 0 && sel < (int)m_skillCodes.size())
        {
            wxString code = m_skillCodes[sel];
            wxString alias = m_skillAliases[sel];
            int level = m_skillLevelSpin->GetValue();
            
            SaveUnitToEditor();
            
            // Check if skill already exists
            bool found = false;
            for (size_t i = 0; i < m_editUnit.skills.size(); i++)
            {
                if (m_editUnit.skills[i].code == code)
                {
                    m_editUnit.skills[i].level = level;
                    found = true;
                    break;
                }
            }
            
            if (!found)
            {
                m_editUnit.skills.push_back(BattleSkill(code, alias, level));
            }
            
            UpdateSkillsList();
        }
    }
}

/**
 * Handles Remove Skill button click
 * Removes selected skill from current unit
 */
void CBattleDlg::OnRemoveSkill(wxCommandEvent& event)
{
    if (event.GetId() == m_removeSkillBtn->GetId())
    {
        int sel = m_skillsList->GetSelection();
        if (sel != wxNOT_FOUND && sel >= 0 && sel < (int)m_editUnit.skills.size())
        {
            SaveUnitToEditor();
            m_editUnit.skills.erase(m_editUnit.skills.begin() + sel);
            UpdateSkillsList();
        }
    }
}

/**
 * Handles Add to Attackers button click
 * Adds current unit to attackers side
 */
void CBattleDlg::OnAddAttacker(wxCommandEvent& event)
{
    AddUnitToSide(0);
}

/**
 * Handles Add to Defenders button click
 * Adds current unit to defenders side
 */
void CBattleDlg::OnAddDefender(wxCommandEvent& event)
{
    AddUnitToSide(1);
}

/**
 * Handles Update Unit button click
 * Updates currently selected unit with editor data
 */
void CBattleDlg::OnUpdateUnit(wxCommandEvent& event)
{
    UpdateCurrentUnit();
}

/**
 * Handles Clear button click
 * Clears all fields in the unit editor
 */
void CBattleDlg::OnClearUnit(wxCommandEvent& event)
{
    ClearUnitEditor();
}

/**
 * Handles Clear All Attackers button click
 * Removes all units from attackers side after confirmation
 */
void CBattleDlg::OnClearAttackers(wxCommandEvent& event)
{
    if (wxMessageBox("Clear all attackers?", "Confirm", wxYES_NO | wxICON_QUESTION) == wxYES)
    {
        m_attackers.units.clear();
        UpdateUnitLists();
    }
}

/**
 * Handles Clear All Defenders button click
 * Removes all units from defenders side after confirmation
 */
void CBattleDlg::OnClearDefenders(wxCommandEvent& event)
{
    if (wxMessageBox("Clear all defenders?", "Confirm", wxYES_NO | wxICON_QUESTION) == wxYES)
    {
        m_defenders.units.clear();
        UpdateUnitLists();
    }
}

/**
 * Handles item selection change in dropdown
 * Clears custom code field when an item from dropdown is selected
 */
void CBattleDlg::OnItemSelection(wxCommandEvent& event)
{
    // Clear custom code when selecting from dropdown
    m_customItemCode->SetValue("");
}

/**
 * Handles skill selection change in dropdown
 * (currently does nothing, placeholder for future functionality)
 */
void CBattleDlg::OnSkillSelection(wxCommandEvent& event)
{
    // Nothing to do
}

/**
 * Handles unit selection in either list control
 * Loads selected unit into editor for editing
 */
void CBattleDlg::OnUnitSelected(wxListEvent& event)
{
    long itemIdx = event.GetIndex();
    wxListCtrl* list = (wxListCtrl*)event.GetEventObject();
    
    if (list == m_attackersList)
    {
        m_selectedSide = 0;
        m_selectedUnitIdx = m_attackersList->GetItemData(itemIdx);
        if (m_selectedUnitIdx >= 0 && m_selectedUnitIdx < (int)m_attackers.units.size())
        {
            FillUnitEditor(m_attackers.units[m_selectedUnitIdx]);
        }
    }
    else if (list == m_defendersList)
    {
        m_selectedSide = 1;
        m_selectedUnitIdx = m_defendersList->GetItemData(itemIdx);
        if (m_selectedUnitIdx >= 0 && m_selectedUnitIdx < (int)m_defenders.units.size())
        {
            FillUnitEditor(m_defenders.units[m_selectedUnitIdx]);
        }
    }
}

/**
 * Handles right-click on unit in either list
 * Shows context menu with operations for the unit
 */
void CBattleDlg::OnUnitRightClick(wxListEvent& event)
{
    long itemIdx = event.GetIndex();
    wxListCtrl* list = (wxListCtrl*)event.GetEventObject();
    
    int side = (list == m_attackersList) ? 0 : 1;
    int unitIdx = list->GetItemData(itemIdx);
    
    wxMenu menu;
    menu.Append(MENU_Unit_Edit, "&Edit");
    menu.Append(MENU_Unit_Delete, "&Delete");
    menu.Append(MENU_Unit_Duplicate, "&Duplicate");
    menu.AppendSeparator();
    
    if (side == 0)
        menu.Append(MENU_Unit_MoveToDefenders, "Move to &Defenders");
    else
        menu.Append(MENU_Unit_MoveToAttackers, "Move to &Attackers");
    
    // Store selection for menu handler
    m_selectedSide = side;
    m_selectedUnitIdx = unitIdx;
    
    PopupMenu(&menu, event.GetPoint());
}

/**
 * Handles context menu commands for units
 * Supports Edit, Delete, Duplicate, and Move operations
 */
void CBattleDlg::OnUnitMenu(wxCommandEvent& event)
{
    if (m_selectedUnitIdx < 0)
        return;
    
    BattleUnit unit;
    if (m_selectedSide == 0)
    {
        if (m_selectedUnitIdx >= (int)m_attackers.units.size())
            return;
        unit = m_attackers.units[m_selectedUnitIdx];
    }
    else
    {
        if (m_selectedUnitIdx >= (int)m_defenders.units.size())
            return;
        unit = m_defenders.units[m_selectedUnitIdx];
    }
    
    switch (event.GetId())
    {
        case MENU_Unit_Edit:
            FillUnitEditor(unit);
            break;
            
        case MENU_Unit_Delete:
            if (m_selectedSide == 0)
                m_attackers.units.erase(m_attackers.units.begin() + m_selectedUnitIdx);
            else
                m_defenders.units.erase(m_defenders.units.begin() + m_selectedUnitIdx);
            UpdateUnitLists();
            ClearUnitEditor();
            break;
            
        case MENU_Unit_Duplicate:
            {
                BattleUnit clone = unit.Clone();
                if (m_selectedSide == 0)
                    m_attackers.units.push_back(clone);
                else
                    m_defenders.units.push_back(clone);
                UpdateUnitLists();
            }
            break;
            
        case MENU_Unit_MoveToAttackers:
            if (m_selectedSide == 1)
            {
                m_attackers.units.push_back(unit);
                m_defenders.units.erase(m_defenders.units.begin() + m_selectedUnitIdx);
                UpdateUnitLists();
            }
            break;
            
        case MENU_Unit_MoveToDefenders:
            if (m_selectedSide == 0)
            {
                m_defenders.units.push_back(unit);
                m_attackers.units.erase(m_attackers.units.begin() + m_selectedUnitIdx);
                UpdateUnitLists();
            }
            break;
    }
}

/**
 * Handles OK button click
 * Validates and closes dialog
 */
void CBattleDlg::OnOk(wxCommandEvent& event)
{
    // Save current unit if it's being edited
    SaveUnitToEditor();
    
    // Check if there's at least one unit (optional)
    if (m_attackers.units.empty() && m_defenders.units.empty())
    {
        if (wxMessageBox("No units defined. Continue anyway?", "Confirm", wxYES_NO | wxICON_QUESTION) != wxYES)
            return;
    }
    
    EndModal(wxID_OK);
}

/**
 * Handles Cancel button click
 */
void CBattleDlg::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void CBattleDlg::RegisterConverters()
{
    // Create converters
    m_converters.push_back(new OriginalFormatConverter());

    SimulationFormatConverter* simConverter = new SimulationFormatConverter();
    // Set default values
    simConverter->SetBattles(10);
    simConverter->SetSeed(42);

    // Set default region from config if available
    if (!m_regionTypes.IsEmpty())
        simConverter->SetRegionType(m_regionTypes[0]);
    else
        simConverter->SetRegionType("plain");

    m_converters.push_back(simConverter);

    // Add more converters here in the future

    // Populate choice control
    for (auto* converter : m_converters)
    {
        m_converterChoice->Append(converter->GetName());
    }
    m_converterChoice->SetSelection(0);
}

IBattleConverter* CBattleDlg::GetCurrentConverter()
{
    int sel = m_converterChoice->GetSelection();
    if (sel >= 0 && sel < (int)m_converters.size())
        return m_converters[sel];
    return m_converters.empty() ? nullptr : m_converters[0];
}

// Modify OnLoadJson
void CBattleDlg::OnLoadJson(wxCommandEvent& event)
{
    wxFileDialog dlg(this, "Load Battle JSON", "", "", "JSON files (*.json)|*.json", wxFD_OPEN);
    if (dlg.ShowModal() == wxID_OK)
    {
        FILE* fp = fopen(dlg.GetPath().ToUTF8(), "rb");
        if (!fp) return;

        char readBuffer[65536];
        rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

        rapidjson::Document doc;
        doc.ParseStream(is);
        fclose(fp);

        if (doc.HasParseError())
        {
            wxMessageBox("Failed to parse JSON!", "Error", wxOK | wxICON_ERROR);
            return;
        }

        // Try each converter until one works
        BattleSide tempAttackers, tempDefenders;
        bool success = false;

        for (auto* converter : m_converters)
        {
            tempAttackers.units.clear();
            tempDefenders.units.clear();

            if (converter->FromJson(doc, tempAttackers, tempDefenders))
            {
                // Check if we got any units
                if (!tempAttackers.units.empty() || !tempDefenders.units.empty())
                {
                    m_attackers = tempAttackers;
                    m_defenders = tempDefenders;

                    // Update simulation format parameters if applicable
                    SimulationFormatConverter* simConv =
                        dynamic_cast<SimulationFormatConverter*>(converter);
                    if (simConv)
                    {
                        // Update UI with loaded parameters
                        m_battlesSpin->SetValue(simConv->GetBattles());
                        m_seedSpin->SetValue(simConv->GetSeed());

                        int sel = m_regionTypeChoice->FindString(simConv->GetRegionType());
                        if (sel != wxNOT_FOUND)
                            m_regionTypeChoice->SetSelection(sel);
                    }

                    success = true;
                    break;
                }
            }
        }

        if (success)
        {
            UpdateUnitLists();
            ClearUnitEditor();

            // Update simulation format parameters if applicable
            SimulationFormatConverter* simConv =
                dynamic_cast<SimulationFormatConverter*>(GetCurrentConverter());
            if (simConv)
            {
                m_battlesSpin->SetValue(simConv->GetBattles());
                m_seedSpin->SetValue(simConv->GetSeed());

                // Find and select the region
                wxString region = simConv->GetRegionType();
                int sel = m_regionTypeChoice->FindString(region, true); // case-insensitive
                if (sel == wxNOT_FOUND && !m_regionTypes.IsEmpty())
                    sel = 0; // default to first
                m_regionTypeChoice->SetSelection(sel);
            }

            wxMessageBox("Battle loaded successfully!", "Success", wxOK | wxICON_INFORMATION);
        }
        else
        {
            wxMessageBox("Failed to load battle from JSON - no compatible format found!",
                "Error", wxOK | wxICON_ERROR);
        }
    }
}

// Modify OnSaveJson
void CBattleDlg::OnSaveJson(wxCommandEvent& event)
{
    wxFileDialog dlg(this, "Save Battle JSON", "", "battle.json",
        "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK)
    {
        IBattleConverter* converter = GetCurrentConverter();
        if (!converter) return;

        // Update simulation format parameters if needed
        SimulationFormatConverter* simConv =
            dynamic_cast<SimulationFormatConverter*>(converter);
        if (simConv)
        {
            simConv->SetBattles(m_battlesSpin->GetValue());
            simConv->SetSeed(m_seedSpin->GetValue());

            int sel = m_regionTypeChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                simConv->SetRegionType(m_regionTypeChoice->GetString(sel));
        }

        rapidjson::Document doc;
        if (converter->ToJson(doc, m_attackers, m_defenders))
        {
            FILE* fp = fopen(dlg.GetPath().ToUTF8(), "wb");
            if (fp)
            {
                char writeBuffer[65536];
                rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

                rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
                doc.Accept(writer);

                fclose(fp);
                wxMessageBox("Battle saved successfully!", "Success", wxOK | wxICON_INFORMATION);
                return;
            }
        }

        wxMessageBox("Failed to save battle to JSON!", "Error", wxOK | wxICON_ERROR);
    }
}

/**
 * Loads region types from [RESOURCE_LAND] section in config
 * These are used for the region_type field in simulation format
 */
void CBattleDlg::LoadRegionTypes()
{
    m_regionTypes.Clear();

    // Get all entries from [RESOURCE_LAND] section
    const char* szName;
    const char* szValue;
    int idx = m_pApp->GetSectionFirst(SZ_SECT_RESOURCE_LAND, szName, szValue);

    while (idx >= 0)
    {
        // Add the region name (key) to the list
        wxString region = wxString::FromUTF8(szName).Lower();
        if (!region.IsEmpty())
        {
            m_regionTypes.Add(region);
        }
        idx = m_pApp->GetSectionNext(idx, SZ_SECT_RESOURCE_LAND, szName, szValue);
    }

    // Sort alphabetically for better UX
    m_regionTypes.Sort();

    // If no regions found, add defaults as fallback
    if (m_regionTypes.IsEmpty())
    {
        m_regionTypes.Add("plain");
        m_regionTypes.Add("forest");
        m_regionTypes.Add("mountain");
        m_regionTypes.Add("swamp");
        m_regionTypes.Add("desert");
        m_regionTypes.Add("ice");
    }
}