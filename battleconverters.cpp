// Disable C++17 iterator deprecation warning for RapidJSON
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "battleconverters.h"
#include "ahapp.h"  // Для CAhApp
#include <wx/log.h>
#include <map>

//=============================================================================
// Helper functions
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
 * Checks if a unit has any magical skills
 */
static bool HasMagicalSkills(const BattleUnit& unit)
{
    // List of known magic skill codes (can be expanded)
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

/**
 * Parses a unit from JSON structure
 * @param unitJson JSON object containing unit data
 * @param unit Output BattleUnit structure
 * @param app Application instance for resolving aliases (optional)
 */
static void ParseUnitFromJson(const rapidjson::Value& unitJson, BattleUnit& unit, CAhApp* app)
{
    // Parse name
    if (unitJson.HasMember("name") && unitJson["name"].IsString())
        unit.name = JsonValueToWxString(unitJson["name"]);
    
    // Parse flags array (for compatibility with multiple formats)
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
    
    // Parse behind flag directly (some formats use direct field)
    if (unitJson.HasMember("behind") && unitJson["behind"].IsBool())
    {
        unit.behind = unitJson["behind"].GetBool();
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
                
                // Get item code (abbr)
                if (itemJson.HasMember("abbr") && itemJson["abbr"].IsString())
                    item.code = JsonValueToWxString(itemJson["abbr"]).Upper();
                
                // Get item count/amount - different formats use different field names
                if (itemJson.HasMember("count") && itemJson["count"].IsInt())
                    item.amount = itemJson["count"].GetInt();
                else if (itemJson.HasMember("amount") && itemJson["amount"].IsInt())
                    item.amount = itemJson["amount"].GetInt();
                
                // Get display alias for this item code if app is provided
                if (app)
                {
                    const char* alias = app->ResolveAlias(item.code.ToUTF8());
                    item.alias = alias ? wxString::FromUTF8(alias) : item.code;
                }
                else
                {
                    item.alias = item.code;
                }
                
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
                
                // Get skill code (abbr)
                if (skillJson.HasMember("abbr") && skillJson["abbr"].IsString())
                    skill.code = JsonValueToWxString(skillJson["abbr"]).Upper();
                
                // Get skill level
                if (skillJson.HasMember("level") && skillJson["level"].IsInt())
                    skill.level = skillJson["level"].GetInt();
                
                // Get display alias for this skill code if app is provided
                if (app)
                {
                    const char* alias = app->ResolveAlias(skill.code.ToUTF8());
                    skill.alias = alias ? wxString::FromUTF8(alias) : skill.code;
                }
                else
                {
                    skill.alias = skill.code;
                }
                
                unit.skills.push_back(skill);
            }
        }
    }
    
    // Parse combat spell - check both naming conventions
    if (unitJson.HasMember("combat_spell") && unitJson["combat_spell"].IsString())
        unit.combatSpell = JsonValueToWxString(unitJson["combat_spell"]);
    else if (unitJson.HasMember("combatSpell") && unitJson["combatSpell"].IsString())
        unit.combatSpell = JsonValueToWxString(unitJson["combatSpell"]);
}

/**
 * Writes a unit to JSON structure
 * @param unitJson JSON object to write to
 * @param unit BattleUnit to write
 * @param allocator RapidJSON allocator
 */
static void WriteUnitToJson(rapidjson::Value& unitJson, 
                            const BattleUnit& unit, 
                            rapidjson::Document::AllocatorType& allocator)
{
    // Name
    rapidjson::Value nameValue(unit.name.ToUTF8(), allocator);
    unitJson.AddMember("name", nameValue, allocator);
    
    // Behind flag as boolean (preferred for simulation format)
    unitJson.AddMember("behind", unit.behind, allocator);
    
    // Also add as flags array for compatibility with other formats
    rapidjson::Value flags(rapidjson::kArrayType);
    if (unit.behind)
    {
        rapidjson::Value flagValue("behind", allocator);
        flags.PushBack(flagValue, allocator);
    }
    unitJson.AddMember("flags", flags, allocator);
    
    // Items array
    rapidjson::Value items(rapidjson::kArrayType);
    for (size_t j = 0; j < unit.items.size(); j++)
    {
        rapidjson::Value itemJson(rapidjson::kObjectType);
        
        // Item code (abbr)
        rapidjson::Value abbrValue(unit.items[j].code.ToUTF8(), allocator);
        itemJson.AddMember("abbr", abbrValue, allocator);
        
        // Item count - simulation format uses "count"
        itemJson.AddMember("count", unit.items[j].amount, allocator);
        
        items.PushBack(itemJson, allocator);
    }
    unitJson.AddMember("items", items, allocator);
    
    // Skills array
    rapidjson::Value skills(rapidjson::kArrayType);
    for (size_t j = 0; j < unit.skills.size(); j++)
    {
        rapidjson::Value skillJson(rapidjson::kObjectType);
        
        // Skill code (abbr)
        rapidjson::Value abbrValue(unit.skills[j].code.ToUTF8(), allocator);
        skillJson.AddMember("abbr", abbrValue, allocator);
        
        // Skill level
        skillJson.AddMember("level", unit.skills[j].level, allocator);
        
        skills.PushBack(skillJson, allocator);
    }
    unitJson.AddMember("skills", skills, allocator);
    
    // Check if unit has magical skills
    bool isMage = HasMagicalSkills(unit);
    
    // Add mage flag for simulation format
    unitJson.AddMember("mage", isMage, allocator);
    
    // Combat spell - always include for mages, optionally for others
    if (!unit.combatSpell.IsEmpty() || isMage)
    {
        // Use combat_spell (with underscore) as in example files
        rapidjson::Value spellValue(unit.combatSpell.IsEmpty() ? "" : unit.combatSpell.ToUTF8(), allocator);
        unitJson.AddMember("combat_spell", spellValue, allocator);
    }
}

//=============================================================================
// OriginalFormatConverter implementation
//=============================================================================

wxString OriginalFormatConverter::GetName() const
{
    return "New Origin Format";
}

wxString OriginalFormatConverter::GetDescription() const
{
    return "Format with attackers/defenders as objects containing units arrays";
}

bool OriginalFormatConverter::FromJson(const rapidjson::Document& doc, 
                                        BattleSide& attackers, 
                                        BattleSide& defenders,
                                        CAhApp* app)
{
    // Clear existing data
    attackers.units.clear();
    defenders.units.clear();
    
    // Load attackers
    if (doc.HasMember("attackers") && doc["attackers"].IsObject())
    {
        const rapidjson::Value& attackersObj = doc["attackers"];
        if (attackersObj.HasMember("units") && attackersObj["units"].IsArray())
        {
            const rapidjson::Value& units = attackersObj["units"];
            for (rapidjson::SizeType i = 0; i < units.Size(); i++)
            {
                BattleUnit unit;
                ParseUnitFromJson(units[i], unit, app);
                attackers.units.push_back(unit);
            }
        }
    }
    
    // Load defenders
    if (doc.HasMember("defenders") && doc["defenders"].IsObject())
    {
        const rapidjson::Value& defendersObj = doc["defenders"];
        if (defendersObj.HasMember("units") && defendersObj["units"].IsArray())
        {
            const rapidjson::Value& units = defendersObj["units"];
            for (rapidjson::SizeType i = 0; i < units.Size(); i++)
            {
                BattleUnit unit;
                ParseUnitFromJson(units[i], unit, app);
                defenders.units.push_back(unit);
            }
        }
    }
    
    return true;
}

bool OriginalFormatConverter::ToJson(rapidjson::Document& doc, 
                                      const BattleSide& attackers, 
                                      const BattleSide& defenders)
{
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    
    // Build attackers section
    rapidjson::Value attackersObj(rapidjson::kObjectType);
    rapidjson::Value attackerUnits(rapidjson::kArrayType);
    
    for (size_t i = 0; i < attackers.units.size(); i++)
    {
        rapidjson::Value unitJson(rapidjson::kObjectType);
        WriteUnitToJson(unitJson, attackers.units[i], allocator);
        attackerUnits.PushBack(unitJson, allocator);
    }
    
    attackersObj.AddMember("units", attackerUnits, allocator);
    doc.AddMember("attackers", attackersObj, allocator);
    
    // Build defenders section
    rapidjson::Value defendersObj(rapidjson::kObjectType);
    rapidjson::Value defenderUnits(rapidjson::kArrayType);
    
    for (size_t i = 0; i < defenders.units.size(); i++)
    {
        rapidjson::Value unitJson(rapidjson::kObjectType);
        WriteUnitToJson(unitJson, defenders.units[i], allocator);
        defenderUnits.PushBack(unitJson, allocator);
    }
    
    defendersObj.AddMember("units", defenderUnits, allocator);
    doc.AddMember("defenders", defendersObj, allocator);
    
    return true;
}

//=============================================================================
// SimulationFormatConverter implementation
//=============================================================================

SimulationFormatConverter::SimulationFormatConverter() 
    : m_battles(10)
    , m_seed(42)
    , m_regionType("plain")
{
}

wxString SimulationFormatConverter::GetName() const
{
    return "New Age Format";
}

wxString SimulationFormatConverter::GetDescription() const
{
    return "Format with battles, seed, region_type (for simulation website)";
}

bool SimulationFormatConverter::FromJson(const rapidjson::Document& doc, 
                                          BattleSide& attackers, 
                                          BattleSide& defenders,
                                          CAhApp* app)
{
    // Clear existing data
    attackers.units.clear();
    defenders.units.clear();
    
    // Load battles parameter (optional)
    if (doc.HasMember("battles") && doc["battles"].IsInt())
        m_battles = doc["battles"].GetInt();
    
    // Load seed parameter (optional)
    if (doc.HasMember("seed") && doc["seed"].IsInt())
        m_seed = doc["seed"].GetInt();
    
    // Load region_type parameter (optional)
    if (doc.HasMember("region_type") && doc["region_type"].IsString())
        m_regionType = JsonValueToWxString(doc["region_type"]);
    
    // Load attackers (direct array in simulation format)
    if (doc.HasMember("attacker") && doc["attacker"].IsObject())
    {
        const rapidjson::Value& attackerObj = doc["attacker"];
        if (attackerObj.HasMember("units") && attackerObj["units"].IsArray())
        {
            const rapidjson::Value& units = attackerObj["units"];
            for (rapidjson::SizeType i = 0; i < units.Size(); i++)
            {
                BattleUnit unit;
                ParseUnitFromJson(units[i], unit, app);
                attackers.units.push_back(unit);
            }
        }
    }
    
    // Load defenders (with buildings support)
    if (doc.HasMember("defender") && doc["defender"].IsObject())
    {
        const rapidjson::Value& defenderObj = doc["defender"];
        
        // Load buildings (units inside structures)
        if (defenderObj.HasMember("buildings") && defenderObj["buildings"].IsArray())
        {
            const rapidjson::Value& buildings = defenderObj["buildings"];
            for (rapidjson::SizeType b = 0; b < buildings.Size(); b++)
            {
                const rapidjson::Value& building = buildings[b];
                
                // Get building type (structure)
                wxString structureType;
                if (building.HasMember("type") && building["type"].IsString())
                    structureType = JsonValueToWxString(building["type"]);
                
                // Load units inside this building
                if (building.HasMember("units") && building["units"].IsArray())
                {
                    const rapidjson::Value& units = building["units"];
                    for (rapidjson::SizeType i = 0; i < units.Size(); i++)
                    {
                        BattleUnit unit;
                        ParseUnitFromJson(units[i], unit, app);
                        
                        // Set structure type for this unit
                        unit.structure = structureType;
                        
                        defenders.units.push_back(unit);
                    }
                }
            }
        }
        
        // Load standalone units (not in buildings)
        if (defenderObj.HasMember("units") && defenderObj["units"].IsArray())
        {
            const rapidjson::Value& units = defenderObj["units"];
            for (rapidjson::SizeType i = 0; i < units.Size(); i++)
            {
                BattleUnit unit;
                ParseUnitFromJson(units[i], unit, app);
                defenders.units.push_back(unit);
            }
        }
    }
    
    return true;
}

bool SimulationFormatConverter::ToJson(rapidjson::Document& doc, 
                                        const BattleSide& attackers, 
                                        const BattleSide& defenders)
{
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    
    // Add simulation parameters
    doc.AddMember("battles", m_battles, allocator);
    doc.AddMember("seed", m_seed, allocator);
    
    rapidjson::Value regionValue(m_regionType.ToUTF8(), allocator);
    doc.AddMember("region_type", regionValue, allocator);
    
    // Build attacker section
    rapidjson::Value attackerObj(rapidjson::kObjectType);
    rapidjson::Value attackerUnits(rapidjson::kArrayType);
    
    for (size_t i = 0; i < attackers.units.size(); i++)
    {
        rapidjson::Value unitJson(rapidjson::kObjectType);
        WriteUnitToJson(unitJson, attackers.units[i], allocator);
        attackerUnits.PushBack(unitJson, allocator);
    }
    
    attackerObj.AddMember("units", attackerUnits, allocator);
    doc.AddMember("attacker", attackerObj, allocator);
    
    // Build defender section (with buildings support)
    rapidjson::Value defenderObj(rapidjson::kObjectType);
    
    // Group units by structure for buildings
    std::map<wxString, std::vector<const BattleUnit*>> unitsByStructure;
    std::vector<const BattleUnit*> standaloneUnits;
    
    for (size_t i = 0; i < defenders.units.size(); i++)
    {
        const BattleUnit& unit = defenders.units[i];
        if (!unit.structure.IsEmpty())
        {
            unitsByStructure[unit.structure].push_back(&unit);
        }
        else
        {
            standaloneUnits.push_back(&unit);
        }
    }
    
    // Buildings array
    if (!unitsByStructure.empty())
    {
        rapidjson::Value buildings(rapidjson::kArrayType);
        
        for (const auto& pair : unitsByStructure)
        {
            rapidjson::Value building(rapidjson::kObjectType);
            
            // Building type
            rapidjson::Value typeValue(pair.first.ToUTF8(), allocator);
            building.AddMember("type", typeValue, allocator);
            
            // Units inside this building
            rapidjson::Value buildingUnits(rapidjson::kArrayType);
            for (const BattleUnit* unit : pair.second)
            {
                rapidjson::Value unitJson(rapidjson::kObjectType);
                WriteUnitToJson(unitJson, *unit, allocator);
                buildingUnits.PushBack(unitJson, allocator);
            }
            
            building.AddMember("units", buildingUnits, allocator);
            buildings.PushBack(building, allocator);
        }
        
        defenderObj.AddMember("buildings", buildings, allocator);
    }
    
    // Standalone units (not in buildings)
    if (!standaloneUnits.empty())
    {
        rapidjson::Value standaloneArray(rapidjson::kArrayType);
        for (const BattleUnit* unit : standaloneUnits)
        {
            rapidjson::Value unitJson(rapidjson::kObjectType);
            WriteUnitToJson(unitJson, *unit, allocator);
            standaloneArray.PushBack(unitJson, allocator);
        }
        defenderObj.AddMember("units", standaloneArray, allocator);
    }
    
    doc.AddMember("defender", defenderObj, allocator);
    
    return true;
}

// Setters and getters for simulation parameters
void SimulationFormatConverter::SetBattles(int battles) 
{ 
    m_battles = battles; 
}

void SimulationFormatConverter::SetSeed(int seed) 
{ 
    m_seed = seed; 
}

void SimulationFormatConverter::SetRegionType(const wxString& type) 
{ 
    m_regionType = type; 
}

int SimulationFormatConverter::GetBattles() const 
{ 
    return m_battles; 
}

int SimulationFormatConverter::GetSeed() const 
{ 
    return m_seed; 
}

wxString SimulationFormatConverter::GetRegionType() const 
{ 
    return m_regionType; 
}