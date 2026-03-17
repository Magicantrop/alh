#ifndef __BATTLE_DLG_H__
#define __BATTLE_DLG_H__

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/listbox.h>
#include <wx/arrstr.h>
#include <vector>

// Forward declarations
class CAhApp;
class CUnit;
class IBattleConverter;

//=============================================================================
// Data Structures for Battle Simulation
//=============================================================================

/**
 * @struct BattleItem
 * @brief Represents an item in a unit's inventory for battle simulation
 */
struct BattleItem
{
    wxString code;      /**< Item code (e.g., "HUMN", "SWOR") */
    wxString alias;     /**< Display name from aliases section */
    int amount;         /**< Quantity of the item */

    BattleItem() : amount(0) {}
    BattleItem(const wxString& c, const wxString& a, int amt)
        : code(c), alias(a), amount(amt) {}
};

/**
 * @struct BattleSkill
 * @brief Represents a skill possessed by a unit for battle simulation
 */
struct BattleSkill
{
    wxString code;      /**< Skill code (e.g., "COMB", "FIRE") */
    wxString alias;     /**< Display name from aliases section */
    int level;          /**< Skill level (typically 1-10) */

    BattleSkill() : level(0) {}
    BattleSkill(const wxString& c, const wxString& a, int lvl)
        : code(c), alias(a), level(lvl) {}
};

/**
 * @struct BattleUnit
 * @brief Represents a single unit in a battle simulation
 */
struct BattleUnit
{
    wxString name;                      /**< Unit name */
    wxString structure;                  /**< Structure type (if unit is in/on a structure) */
    bool behind;                         /**< Behind flag for combat positioning */
    std::vector<BattleItem> items;       /**< Unit's items/inventory */
    std::vector<BattleSkill> skills;     /**< Unit's skills */
    wxString combatSpell;                 /**< Combat spell for mages */

    BattleUnit() : behind(false) {}

    /**
     * @brief Creates a deep copy of the unit
     * @return Copy of the unit with all items and skills
     */
    BattleUnit Clone() const
    {
        BattleUnit clone;
        clone.name = name;
        clone.structure = structure;
        clone.behind = behind;
        clone.combatSpell = combatSpell;
        clone.items = items;
        clone.skills = skills;
        return clone;
    }
};

/**
 * @struct BattleSide
 * @brief Represents one side of a battle (attackers or defenders)
 */
struct BattleSide
{
    std::vector<BattleUnit> units;       /**< Units on this side of the battle */
};

//---------------------------------------------------------------------------
// Control IDs
//---------------------------------------------------------------------------

enum
{
    ID_AddItem = wxID_HIGHEST + 1,      /**< Add item button ID */
    ID_RemoveItem,                       /**< Remove item button ID */
    ID_AddSkill,                          /**< Add skill button ID */
    ID_RemoveSkill,                         /**< Remove skill button ID */
    ID_AddAttacker,                           /**< Add attacker button ID */
    ID_AddDefender,                             /**< Add defender button ID */
    ID_UpdateUnit,                                /**< Update unit button ID */
    ID_ClearUnit,                                   /**< Clear unit button ID */
    ID_ClearAttackers,                                 /**< Clear attackers button ID */
    ID_ClearDefenders,                                   /**< Clear defenders button ID */
    ID_LoadJson,                                           /**< Load JSON button ID */
    ID_SaveJson,                                             /**< Save JSON button ID */
    ID_AddFromUnit                                            /**< Add from unit menu ID */
};

//---------------------------------------------------------------------------
// Context Menu Command IDs
//---------------------------------------------------------------------------

enum
{
    MENU_Unit_Edit = 20000,               /**< Edit unit context menu command */
    MENU_Unit_Delete,                       /**< Delete unit context menu command */
    MENU_Unit_Duplicate,                      /**< Duplicate unit context menu command */
    MENU_Unit_MoveToAttackers,                   /**< Move unit to attackers context menu command */
    MENU_Unit_MoveToDefenders,                      /**< Move unit to defenders context menu command */
    MENU_Unit_AddToAttackers,                           /**< Add to attackers from unit pane command */
    MENU_Unit_AddToDefenders                              /**< Add to defenders from unit pane command */
};

//=============================================================================
// Battle Dialog Class
//=============================================================================

/**
 * @class CBattleDlg
 * @brief Dialog for creating and editing battle JSON data for external simulation websites
 * 
 * This dialog provides a comprehensive UI for managing two armies with units,
 * items, skills, and structures. It supports multiple JSON formats and can
 * import units directly from the game.
 */
class CBattleDlg : public wxDialog
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param app Pointer to main application instance
     */
    CBattleDlg(wxWindow* parent, CAhApp* app);

    /**
     * @brief Destructor
     */
    virtual ~CBattleDlg();

    //---------------------------------------------------------------------------
    // Static Methods for Global Battle Data
    //---------------------------------------------------------------------------

    /**
     * @brief Gets the global attackers data
     * @return Reference to global attackers BattleSide structure
     */
    static BattleSide& GetGlobalAttackers() { return ms_attackers; }
    
    /**
     * @brief Gets the global defenders data
     * @return Reference to global defenders BattleSide structure
     */
    static BattleSide& GetGlobalDefenders() { return ms_defenders; }
    
    /**
     * @brief Clears all global battle data
     */
    static void ClearGlobalData();

    //---------------------------------------------------------------------------
    // Methods to Add Units from Game
    //---------------------------------------------------------------------------

    /**
     * @brief Adds a game unit to the attackers side
     * @param pUnit Pointer to the game unit
     * @return true if unit was successfully added
     */
    static bool AddUnitToAttackers(CUnit* pUnit);
    
    /**
     * @brief Adds a game unit to the defenders side
     * @param pUnit Pointer to the game unit
     * @return true if unit was successfully added
     */
    static bool AddUnitToDefenders(CUnit* pUnit);
    
    /**
     * @brief Converts a game unit to a BattleUnit structure
     * @param pUnit Pointer to the game unit
     * @param app Pointer to application instance (for data lookups)
     * @return BattleUnit structure with converted data
     */
    static BattleUnit ConvertGameUnit(CUnit* pUnit, CAhApp* app);

    //---------------------------------------------------------------------------
    // Accessors for Battle Data
    //---------------------------------------------------------------------------

    /**
     * @brief Gets the local attackers data
     * @return Reference to local attackers BattleSide structure
     */
    BattleSide& GetAttackers() { return m_attackers; }
    
    /**
     * @brief Gets the local defenders data
     * @return Reference to local defenders BattleSide structure
     */
    BattleSide& GetDefenders() { return m_defenders; }
    
    /**
     * @brief Sets the local attackers data
     * @param attackers New attackers BattleSide structure
     */
    void SetAttackers(const BattleSide& attackers) { m_attackers = attackers; }
    
    /**
     * @brief Sets the local defenders data
     * @param defenders New defenders BattleSide structure
     */
    void SetDefenders(const BattleSide& defenders) { m_defenders = defenders; }

    /**
     * @brief Clears all units from both sides and resets editor
     */
    void ClearAll();

private:
    //---------------------------------------------------------------------------
    // Static Data Members
    //---------------------------------------------------------------------------

    static BattleSide ms_attackers;      /**< Global attackers data (shared across instances) */
    static BattleSide ms_defenders;      /**< Global defenders data (shared across instances) */

    //---------------------------------------------------------------------------
    // Instance Data Members
    //---------------------------------------------------------------------------

    CAhApp* m_pApp;                       /**< Pointer to main application instance */
    BattleSide m_attackers;                /**< Local attackers side data (copy of global) */
    BattleSide m_defenders;                 /**< Local defenders side data (copy of global) */
    int m_selectedSide;                      /**< Currently selected side (0=attackers, 1=defenders) */
    int m_selectedUnitIdx;                    /**< Index of selected unit in its side */

    //---------------------------------------------------------------------------
    // UI Controls - Unit Creation/Editing
    //---------------------------------------------------------------------------

    wxTextCtrl* m_unitNameCtrl;              /**< Unit name input field */
    wxChoice* m_structureChoice;              /**< Structure type dropdown */
    wxCheckBox* m_behindCheck;                 /**< Behind flag checkbox */

    wxListBox* m_itemsList;                     /**< List of items in current unit */
    wxChoice* m_itemChoice;                       /**< Item selection dropdown */
    wxSpinCtrl* m_itemAmountSpin;                   /**< Item quantity spinner */
    wxTextCtrl* m_customItemCode;                     /**< Custom item code input field */
    wxButton* m_addItemBtn;                              /**< Add item button */
    wxButton* m_removeItemBtn;                             /**< Remove item button */

    wxListBox* m_skillsList;                       /**< List of skills in current unit */
    wxChoice* m_skillChoice;                          /**< Skill selection dropdown */
    wxSpinCtrl* m_skillLevelSpin;                        /**< Skill level spinner */
    wxButton* m_addSkillBtn;                               /**< Add skill button */
    wxButton* m_removeSkillBtn;                              /**< Remove skill button */

    wxChoice* m_combatSpellChoice;                     /**< Combat spell selection dropdown */

    wxButton* m_addAttackerBtn;                          /**< Add to attackers button */
    wxButton* m_addDefenderBtn;                           /**< Add to defenders button */
    wxButton* m_updateUnitBtn;                              /**< Update current unit button */
    wxButton* m_clearUnitBtn;                                 /**< Clear editor button */
    wxButton* m_clearAttackersBtn;                              /**< Clear all attackers button */
    wxButton* m_clearDefendersBtn;                               /**< Clear all defenders button */

    wxListCtrl* m_attackersList;                /**< List control displaying attackers */
    wxListCtrl* m_defendersList;                  /**< List control displaying defenders */

    wxButton* m_loadBtn;                           /**< Load JSON button */
    wxButton* m_saveBtn;                            /**< Save JSON button */
    wxButton* m_okBtn;                                /**< OK button */
    wxButton* m_cancelBtn;                              /**< Cancel button */

    //---------------------------------------------------------------------------
    // Dropdown Data
    //---------------------------------------------------------------------------

    wxArrayString m_itemAliases;                    /**< Display names for items */
    wxArrayString m_itemCodes;                        /**< Internal codes for items */
    wxArrayString m_skillAliases;                        /**< Display names for skills */
    wxArrayString m_skillCodes;                            /**< Internal codes for skills */
    wxArrayString m_structureNames;                          /**< Display names for structures */
    wxArrayString m_structureCodes;                            /**< Internal codes for structures */
    wxArrayString m_regionTypes;                                  /**< Region types loaded from config */

    //---------------------------------------------------------------------------
    // Editor State
    //---------------------------------------------------------------------------

    BattleUnit m_editUnit;                           /**< Unit currently being edited */

    //---------------------------------------------------------------------------
    // Converter Selection
    //---------------------------------------------------------------------------

    wxChoice* m_converterChoice;                     /**< Dropdown for selecting JSON format */
    std::vector<IBattleConverter*> m_converters;     /**< List of available converters */

    //---------------------------------------------------------------------------
    // Simulation Format Parameters
    //---------------------------------------------------------------------------

    wxSpinCtrl* m_battlesSpin;                        /**< Number of battles spinner (simulation format) */
    wxSpinCtrl* m_seedSpin;                           /**< Random seed spinner (simulation format) */
    wxChoice* m_regionTypeChoice;                      /**< Region type dropdown (simulation format) */

    //=========================================================================
    // Helper Methods
    //=========================================================================

    /**
     * @brief Gets the currently selected converter
     * @return Pointer to current converter or nullptr if none selected
     */
    IBattleConverter* GetCurrentConverter();

    /**
     * @brief Registers all available JSON converters
     */
    void RegisterConverters();

    //=========================================================================
    // Data Loading Methods
    //=========================================================================

    /**
     * @brief Loads all configuration data from application config files
     */
    void LoadConfigData();

    /**
     * @brief Loads items from a specific property group
     * @param group Group name (e.g., "men", "weapons")
     * @param aliases Output array for display names
     * @param codes Output array for item codes
     */
    void LoadItemsFromGroup(const wxString& group, wxArrayString& aliases, wxArrayString& codes);

    /**
     * @brief Loads skills from a specific property group
     * @param group Group name (e.g., "skills", "mag_skills")
     * @param aliases Output array for display names
     * @param codes Output array for skill codes
     */
    void LoadSkillsFromGroup(const wxString& group, wxArrayString& aliases, wxArrayString& codes);

    /**
     * @brief Loads structure types from [STRUCTURES] configuration section
     */
    void LoadStructures();

    /**
     * @brief Loads region types from [RESOURCE_LAND] configuration section
     */
    void LoadRegionTypes();

    //=========================================================================
    // UI Creation and Update Methods
    //=========================================================================

    /**
     * @brief Creates all dialog controls
     */
    void CreateControls();

    /**
     * @brief Updates the unit list displays for both sides
     */
    void UpdateUnitLists();

    /**
     * @brief Updates the items list in the editor
     */
    void UpdateItemsList();

    /**
     * @brief Updates the skills list in the editor
     */
    void UpdateSkillsList();

    /**
     * @brief Updates the combat spell dropdown with available skills
     */
    void UpdateCombatSpellChoice();

    //=========================================================================
    // Unit Editor Methods
    //=========================================================================

    /**
     * @brief Clears all fields in the unit editor
     */
    void ClearUnitEditor();

    /**
     * @brief Fills the editor with data from a unit
     * @param unit Unit to display in editor
     */
    void FillUnitEditor(const BattleUnit& unit);

    /**
     * @brief Saves current editor values to the temporary unit
     */
    void SaveUnitToEditor();

    /**
     * @brief Gets the unit from editor
     * @return Current unit being edited
     */
    BattleUnit GetUnitFromEditor();

    /**
     * @brief Validates unit data for completeness and correctness
     * @param unit Unit to validate
     * @return true if unit has valid data
     */
    bool ValidateUnit(const BattleUnit& unit);

    /**
     * @brief Adds current editor unit to specified side
     * @param side 0=attackers, 1=defenders
     */
    void AddUnitToSide(int side);

    /**
     * @brief Updates currently selected unit with editor data
     */
    void UpdateCurrentUnit();

    /**
     * @brief Loads global data into local copies
     */
    void LoadGlobalData();

    /**
     * @brief Saves local data to global storage
     */
    void SaveGlobalData();

    //=========================================================================
    // JSON Operations
    //=========================================================================

    /**
     * @brief Loads battle data from JSON file
     * @param filename Path to JSON file
     * @return true on success, false on failure
     */
    bool LoadFromJson(const wxString& filename);

    /**
     * @brief Saves battle data to JSON file
     * @param filename Path to JSON file
     * @return true on success, false on failure
     */
    bool SaveToJson(const wxString& filename);

    /**
     * @brief Generates JSON string from current battle data
     * @return JSON string representation of battle data
     */
    wxString GenerateJson();

    //=========================================================================
    // Event Handlers
    //=========================================================================

    void OnAddItem(wxCommandEvent& event);
    void OnRemoveItem(wxCommandEvent& event);
    void OnAddSkill(wxCommandEvent& event);
    void OnRemoveSkill(wxCommandEvent& event);
    void OnAddAttacker(wxCommandEvent& event);
    void OnAddDefender(wxCommandEvent& event);
    void OnUpdateUnit(wxCommandEvent& event);
    void OnClearUnit(wxCommandEvent& event);
    void OnClearAttackers(wxCommandEvent& event);
    void OnClearDefenders(wxCommandEvent& event);
    void OnItemSelection(wxCommandEvent& event);
    void OnSkillSelection(wxCommandEvent& event);
    void OnUnitSelected(wxListEvent& event);
    void OnUnitRightClick(wxListEvent& event);
    void OnUnitMenu(wxCommandEvent& event);
    void OnLoadJson(wxCommandEvent& event);
    void OnSaveJson(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // __BATTLE_DLG_H__