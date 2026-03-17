#ifndef __BATTLE_CONVERTERS_H__
#define __BATTLE_CONVERTERS_H__

#include <wx/string.h>
#include <vector>
#include "rapidjson/document.h"

// Forward declarations - include battledlg.h for structure definitions
#include "battledlg.h"  // Provides BattleSide and BattleUnit structure definitions

// Forward declaration for CAhApp
class CAhApp;

/**
 * @interface IBattleConverter
 * @brief Abstract base interface for battle JSON format converters
 * 
 * This interface defines the contract for all battle data converters,
 * allowing conversion between JSON formats and the internal BattleSide
 * structures used by the application.
 */
class IBattleConverter
{
public:
    virtual ~IBattleConverter() {}
    
    /**
     * @brief Gets the display name of the converter
     * @return User-friendly name for UI display
     */
    virtual wxString GetName() const = 0;
    
    /**
     * @brief Gets the description of the format
     * @return Description explaining the format characteristics
     */
    virtual wxString GetDescription() const = 0;
    
    /**
     * @brief Converts from JSON document to internal battle structures
     * @param doc JSON document containing battle data
     * @param attackers Output structure for attackers data
     * @param defenders Output structure for defenders data
     * @param app Pointer to application instance (may be needed for data lookups)
     * @return true if conversion succeeded, false otherwise
     */
    virtual bool FromJson(const rapidjson::Document& doc, 
                          BattleSide& attackers, 
                          BattleSide& defenders,
                          CAhApp* app = nullptr) = 0;
    
    /**
     * @brief Converts from internal battle structures to JSON document
     * @param doc Output JSON document to populate
     * @param attackers Attackers data to convert
     * @param defenders Defenders data to convert
     * @return true if conversion succeeded, false otherwise
     */
    virtual bool ToJson(rapidjson::Document& doc, 
                        const BattleSide& attackers, 
                        const BattleSide& defenders) = 0;
};

/**
 * @class OriginalFormatConverter
 * @brief Converter for the original Atlantis Little Helper battle format
 * 
 * This converter handles the original format where attackers and defenders
 * are represented as objects containing arrays of units. This is the
 * traditional format used by the application.
 */
class OriginalFormatConverter : public IBattleConverter
{
public:
    wxString GetName() const override;
    wxString GetDescription() const override;
    
    /**
     * @brief Parses original format JSON into battle structures
     * @param doc JSON document in original format
     * @param attackers Output attackers structure
     * @param defenders Output defenders structure
     * @param app Application instance (may be used for item/skill lookups)
     * @return true if parsing succeeded
     */
    bool FromJson(const rapidjson::Document& doc, 
                  BattleSide& attackers, 
                  BattleSide& defenders,
                  CAhApp* app = nullptr) override;
    
    /**
     * @brief Converts battle structures to original JSON format
     * @param doc Output JSON document
     * @param attackers Attackers data
     * @param defenders Defenders data
     * @return true if conversion succeeded
     */
    bool ToJson(rapidjson::Document& doc, 
                const BattleSide& attackers, 
                const BattleSide& defenders) override;
};

/**
 * @class SimulationFormatConverter
 * @brief Converter for the simulation website battle format
 * 
 * This converter handles the format used by online battle simulators,
 * which includes additional parameters like number of battles, random seed,
 * and region type. Matches the simulate_example.json structure.
 */
class SimulationFormatConverter : public IBattleConverter
{
public:
    SimulationFormatConverter();
    
    wxString GetName() const override;
    wxString GetDescription() const override;
    
    /**
     * @brief Parses simulation format JSON into battle structures
     * @param doc JSON document in simulation format
     * @param attackers Output attackers structure
     * @param defenders Output defenders structure
     * @param app Application instance (may be used for item/skill lookups)
     * @return true if parsing succeeded
     */
    bool FromJson(const rapidjson::Document& doc, 
                  BattleSide& attackers, 
                  BattleSide& defenders,
                  CAhApp* app = nullptr) override;
    
    /**
     * @brief Converts battle structures to simulation JSON format
     * @param doc Output JSON document
     * @param attackers Attackers data
     * @param defenders Defenders data
     * @return true if conversion succeeded
     */
    bool ToJson(rapidjson::Document& doc, 
                const BattleSide& attackers, 
                const BattleSide& defenders) override;
    
    //--------------------------------------------------------------------------
    // Simulation Format Specific Parameters
    //--------------------------------------------------------------------------
    
    /**
     * @brief Sets the number of battles to simulate
     * @param battles Number of battle iterations
     */
    void SetBattles(int battles);
    
    /**
     * @brief Sets the random seed for deterministic simulations
     * @param seed Random seed value
     */
    void SetSeed(int seed);
    
    /**
     * @brief Sets the region type for terrain-based calculations
     * @param type Region type string (e.g., "plain", "mountain", "forest")
     */
    void SetRegionType(const wxString& type);
    
    /**
     * @brief Gets the current number of battles setting
     * @return Number of battles
     */
    int GetBattles() const;
    
    /**
     * @brief Gets the current random seed
     * @return Random seed value
     */
    int GetSeed() const;
    
    /**
     * @brief Gets the current region type
     * @return Region type string
     */
    wxString GetRegionType() const;
    
private:
    int m_battles;        /**< Number of battles to simulate */
    int m_seed;           /**< Random seed for reproducibility */
    wxString m_regionType; /**< Terrain/region type for battle calculations */
};

#endif