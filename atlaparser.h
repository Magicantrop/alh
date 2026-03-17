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

#ifndef __AH_REP_PARSER_H__
#define __AH_REP_PARSER_H__

#include <map>
#include <vector>

#include "data.h"
#include "files.h"
#include "hash.h"
#include "consts_ah.h"
#include "consts.h"
#include "regexparser.h"


extern const char * Monthes[];

extern const char * EOL_MS   ;
extern const char * EOL_UNIX ;
extern const char * EOL_SCR  ;
extern const char * EOL_FILE ;

#define DEFAULT_PLANE "Overworld"

/**
 * @enum eDirection
 * @brief Direction identifiers for movement and exits
 */
typedef enum { North=0, Northeast,   Southeast,   South,   Southwest,   Northwest, Center }   eDirection;

extern int   ExitFlags [];
extern int   EntryFlags[];

extern int Flags_NW_N_NE;
extern int Flags_N      ;
extern int Flags_SW_S_SE;
extern int Flags_S      ;

/**
 * @enum OrderType
 * @brief Order identifiers for all supported game commands
 * 
 * Note: O_ENDXXX must immediately follow O_XXX (e.g., O_ENDTURN after O_TURN)
 */
enum {
    O_ADDRESS = 1,      /**< @ADDRESS command */
    O_ADVANCE,          /**< @ADVANCE command */
    O_ARMOR,            /**< @ARMOR command */
    O_ASSASSINATE,      /**< @ASSASSINATE command */
    O_ATTACK,           /**< @ATTACK command */
    O_AUTOTAX,          /**< @AUTOTAX command */
    O_AVOID,            /**< @AVOID command */
    O_BEHIND,           /**< @BEHIND command */
    O_BUILD,            /**< @BUILD command */
    O_BUY,              /**< @BUY command */
    O_CAST,             /**< @CAST command */
    O_CLAIM,            /**< @CLAIM command */
    O_COMBAT,           /**< @COMBAT command */
    O_CONSUME,          /**< @CONSUME command */
    O_DECLARE,          /**< @DECLARE command */
    O_DESCRIBE,         /**< @DESCRIBE command */
    O_DESTROY,          /**< @DESTROY command */
    O_ENDFORM,          /**< @ENDFORM command */
    O_ENTER,            /**< @ENTER command */
    O_ENTERTAIN,        /**< @ENTERTAIN command */
    O_EVICT,            /**< @EVICT command */
    O_EXCHANGE,         /**< @EXCHANGE command */
    O_FACTION,          /**< @FACTION command */
    O_FIND,             /**< @FIND command */
    O_FORGET,           /**< @FORGET command */
    O_FORM,             /**< @FORM command */
    O_GIVE,             /**< @GIVE command */
    O_GIVEIF,           /**< @GIVEIF command */
    O_TAKE,             /**< @TAKE command */
    O_SEND,             /**< @SEND command */
    O_GUARD,            /**< @GUARD command */
    O_HOLD,             /**< @HOLD command */
    O_LEAVE,            /**< @LEAVE command */
    O_MOVE,             /**< @MOVE command */
    O_NAME,             /**< @NAME command */
    O_NOAID,            /**< @NOAID command */
    O_NOCROSS,          /**< @NOCROSS command */
    O_NOSPOILS,         /**< @NOSPOILS command */
    O_OPTION,           /**< @OPTION command */
    O_PASSWORD,         /**< @PASSWORD command */
    O_PILLAGE,          /**< @PILLAGE command */
    O_PREPARE,          /**< @PREPARE command */
    O_PRODUCE,          /**< @PRODUCE command */
    O_PROMOTE,          /**< @PROMOTE command */
    O_QUIT,             /**< @QUIT command */
    O_RESTART,          /**< @RESTART command */
    O_REVEAL,           /**< @REVEAL command */
    O_SAIL,             /**< @SAIL command */
    O_SELL,             /**< @SELL command */
    O_SHARE,            /**< @SHARE command - MZ: added for Arcadia */
    O_SHOW,             /**< @SHOW command */
    O_SPOILS,           /**< @SPOILS command */
    O_STEAL,            /**< @STEAL command */
    O_STUDY,            /**< @STUDY command */
    O_TAX,              /**< @TAX command */
    O_TEACH,            /**< @TEACH command */
    O_WEAPON,           /**< @WEAPON command */
    O_WITHDRAW,         /**< @WITHDRAW command */
    O_WORK,             /**< @WORK command */
    O_RECRUIT,          /**< @RECRUIT command */
    O_TRANSPORT,        /**< @TRANSPORT command */

    O_TYPE,             /**< @TYPE command for templates */
    O_LABEL,            /**< @LABEL command for templates */

    // Must be in this sequence! O_ENDXXX == O_XXX+1
    O_TURN,             /**< @TURN command - block start */
    O_ENDTURN,          /**< @ENDTURN command - block end */
    O_TEMPLATE,         /**< @TEMPLATE command - block start */
    O_ENDTEMPLATE,      /**< @ENDTEMPLATE command - block end */
    O_ALL,              /**< @ALL command - block start */
    O_ENDALL,           /**< @ENDALL command - block end */

    NORDERS             /**< Total number of order types */
};

/**
 * @enum SHARE_TYPE
 * @brief Types of silver sharing operations
 */
enum SHARE_TYPE {
    SHARE_BUY,          /**< Share for buying */
    SHARE_STUDY,        /**< Share for studying */
    SHARE_UPKEEP        /**< Share for upkeep costs */
};

/**
 * @struct SAVE_HEX_OPTIONS_STRUCT
 * @brief Options for saving hex data to file
 */
typedef struct SAVE_HEX_OPTIONS_STRUCT
{
    BOOL   SaveStructs;              /**< Whether to save structures in hex */
    BOOL   AlwaysSaveImmobStructs;   /**< Always save immobile structures even if SaveStructs is false */
    BOOL   SaveUnits;                 /**< Whether to save units in hex */
    BOOL   SaveResources;             /**< Whether to save resources */
    long   WriteTurnNo;               /**< Add turn number in Atlaclient format */
} SAVE_HEX_OPTIONS;

/**
 * @brief Checks if a string represents a valid integer
 * @param s String to check
 * @return TRUE if string is a valid integer
 */
BOOL IsInteger(const char* s);

//======================================================================

/**
 * @class CAtlaParser
 * @brief Main parser class for Atlantis game reports and orders
 * 
 * This class handles parsing of Atlantis game reports, processing orders,
 * managing game state (units, factions, lands), and providing various
 * game-related calculations and utilities.
 */
class CAtlaParser
{
public:
    CAtlaParser();
    CAtlaParser(CGameDataHelper * pHelper);
    ~CAtlaParser();
    
    /**
     * @brief Clears all parsed data
     */
    void       Clear();
    
    /**
     * @brief Parses a game report file
     * @param FNameIn Input filename
     * @param Join Whether to join with existing data
     * @param IsHistory Whether this is a history file
     * @return 0 on success, error code on failure
     */
    int        ParseRep(const char * FNameIn, BOOL Join, BOOL IsHistory);
    
    /**
     * @brief Saves orders to file
     * @param FNameOut Output filename
     * @param password Faction password
     * @param decorate Whether to add decorative headers
     * @param factid Faction ID
     * @return 0 on success, error code on failure
     */
    int        SaveOrders  (const char * FNameOut, const char * password, BOOL decorate, int factid);
    
    /**
     * @brief Loads orders from file
     * @param FNameIn Input filename
     * @param FactionId Output faction ID
     * @return 0 on success, error code on failure
     */
    int        LoadOrders  (const char * FNameIn, int & FactionId);
    
    /**
     * @brief Executes orders for all units in a land
     * @param pLand Land to process
     * @param sCheckTeach Optional teacher check string
     */
    void       RunOrders(CLand * pLand, const char * sCheckTeach = NULL);
    
    /**
     * @brief Handles silver sharing for a unit
     * @param pMainUnit Unit that initiates sharing
     * @return TRUE if sharing was successful
     */
    BOOL       ShareSilver(CUnit * pMainUnit);
    
    /**
     * @brief Generates teaching orders for a unit
     * @param pMainUnit Unit to generate orders for
     * @return TRUE if orders were generated
     */
    BOOL       GenOrdersTeach(CUnit * pMainUnit);
    
    /**
     * @brief Generates GIVE EVERYTHING orders
     * @param pFrom Source unit
     * @param To Target unit identifier
     * @return TRUE if orders were generated
     */
    BOOL       GenGiveEverything(CUnit * pFrom, const char * To);
    
    /**
     * @brief Discards junk items from a unit
     * @param pUnit Unit to process
     * @param junk Junk item identifier
     * @return TRUE if items were discarded
     */
    BOOL       DiscardJunkItems(CUnit * pUnit, const char * junk);
    
    /**
     * @brief Detects spies based on item counts
     * @param pUnit Unit to check
     * @param lonum Lower bound for detection
     * @param hinum Upper bound for detection
     * @param amount Amount to check
     * @return TRUE if spies detected
     */
    BOOL       DetectSpies(CUnit * pUnit, long lonum, long hinum, long amount);
    
    /**
     * @brief Applies default orders to units with no orders
     * @param EmptyOnly Only apply to units with empty orders
     * @return TRUE if orders were applied
     */
    BOOL       ApplyDefaultOrders(BOOL EmptyOnly);
    
    /**
     * @brief Parses crossbow data file
     * @param FNameIn Input filename
     * @return 0 on success, error code on failure
     */
    int        ParseCBDataFile(const char * FNameIn);
    
    /**
     * @brief Writes mage information to CSV file
     * @param FName Output filename
     * @param vertical Whether to use vertical format
     * @param separator CSV separator character
     * @param format Output format identifier
     */
    void       WriteMagesCSV(const char * FName, BOOL vertical, const char * separator, int format);

    /**
     * @brief Checks if a land exit is closed
     * @param pLand Land to check
     * @param direction Direction to check
     * @return TRUE if exit is closed
     */
    bool       IsLandExitClosed(CLand * pLand, int direction) const;
    
    /**
     * @brief Gets neighboring land in specified direction
     * @param pLand Source land
     * @param direction Direction to move
     * @return Pointer to neighboring land, or NULL if none
     */
    CLand    * GetLandExit(CLand * pLand, int direction) const;
    
    /**
     * @brief Gets land by coordinates
     * @param x X coordinate
     * @param y Y coordinate
     * @param nPlane Plane number
     * @param AdjustForEdge Whether to adjust for map edges
     * @return Pointer to land, or NULL if not found
     */
    CLand    * GetLand(int x, int y, int nPlane, BOOL AdjustForEdge=FALSE) const;
    
    /**
     * @brief Gets land by ID
     * @param LandId Land identifier
     * @return Pointer to land, or NULL if not found
     */
    CLand    * GetLand(long LandId) const;
    
    /**
     * @brief Gets land by coordinate string
     * @param landcoords Coordinate string (e.g., "48,52[,somewhere]")
     * @return Pointer to land, or NULL if not found
     */
    CLand    * GetLand(const char * landcoords) const;
    
    /**
     * @brief Flexible land lookup by description
     * @param description Land description
     * @return Pointer to land, or NULL if not found
     */
    CLand    * GetLandFlexible(const wxString & description) const;
    
    /**
     * @brief Gets land containing a city
     * @param cityName City name to search for
     * @return Pointer to land, or NULL if not found
     */
    CLand    * GetLandWithCity(const wxString & cityName) const;
    
    /**
     * @brief Gets list of units in a specific hex
     * @param pResultColl Output collection for units
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Plane
     */
    void       GetUnitList(CCollection * pResultColl, int x, int y, int z);
    
    /**
     * @brief Counts men for a faction
     * @param FactionId Faction ID
     */
    void       CountMenForTheFaction(int FactionId);
    
    /**
     * @brief Composes products line for a land
     * @param pLand Land to process
     * @param eol End-of-line string
     * @param S Output string
     */
    void       ComposeProductsLine(CLand * pLand, const char * eol, CStr & S);
    
    /**
     * @brief Converts land coordinate string to ID
     * @param landcoords Coordinate string
     * @param id Output land ID
     * @return TRUE if conversion successful
     */
    BOOL       LandStrCoordToId(const char * landcoords, long & id) const;
    
    /**
     * @brief Normalizes X coordinate for map edges
     * @param NoX X coordinate
     * @param pPlane Current plane
     * @return Normalized X coordinate
     */
    int        NormalizeHexX(int NoX, CPlane *) const;
    
    /**
     * @brief Composes coordinate string for a land
     * @param pLand Land to process
     * @param LandStr Output string
     */
    void       ComposeLandStrCoord(CLand * pLand, CStr & LandStr);
    
    /**
     * @brief Gets faction by ID
     * @param id Faction ID
     * @return Pointer to faction, or NULL if not found
     */
    CFaction * GetFaction(int id);
    
    /**
     * @brief Saves a single hex to file
     * @param Dest Destination file writer
     * @param pLand Land to save
     * @param pPlane Plane containing the land
     * @param pOptions Save options
     * @return TRUE if save successful
     */
    BOOL       SaveOneHex(CFileWriter & Dest, CLand * pLand, CPlane * pPlane, SAVE_HEX_OPTIONS * pOptions);
    
    /**
     * @brief Converts skill days to level
     * @param days Number of days studied
     * @return Skill level achieved
     */
    long       SkillDaysToLevel(long days);
    
    /**
     * @brief Splits a unit into two
     * @param pOrigUnit Original unit
     * @param newId ID for new unit
     * @return Pointer to new unit, or NULL on failure
     */
    CUnit *    SplitUnit(CUnit * pOrigUnit, long newId);
    
    /**
     * @brief Links two lands via shaft structure
     * @param pLand Source land
     * @param pLandDest Destination land
     * @param structIdx Structure index
     * @return TRUE if link successful
     */
    bool       LinkShaft(CLand * pLand, CLand * pLandDest, int structIdx);
    
    /**
     * @brief Gets full coordinate string for land
     * @param pLand Land to process
     * @return Full coordinate string
     */
    wxString   getFullStrLandCoord(CLand *);
    
    /**
     * @brief Checks if a unit has resources for production
     * @param pUnit Unit to check
     * @param pLand Land where production occurs
     * @param Error Output error message if check fails
     * @return TRUE if resources are sufficient
     */
    BOOL       CheckResourcesForProduction(CUnit * pUnit, CLand * pLand, CStr & Error);
    
    /**
     * @brief Extrapolates land coordinates in a direction
     * @param x Input/output X coordinate
     * @param y Input/output Y coordinate
     * @param z Plane
     * @param direction Direction to move
     */
    void       ExtrapolateLandCoord(int &x, int &y, int z, int direction) const;

    //--------------------------------------------------------------------------
    // Movement
    //--------------------------------------------------------------------------
    
    /**
     * @brief Gets movement cost for terrain type
     * @param Terrain Terrain name
     * @return Movement cost in movement points
     */
    int        GetTerrainMovementCost(wxString Terrain) const;
    
    /**
     * @brief Checks if roads connect two lands in a direction
     * @param pLand1 First land
     * @param pLand2 Second land
     * @param direction Direction from first to second
     * @return TRUE if roads are connected
     */
    bool       IsRoadConnected(CLand *, CLand *, int direction) const;
    
    /**
     * @brief Checks if a hex has bad weather for the month
     * @param pLand Land to check
     * @param month Month number
     * @return TRUE if weather is bad
     */
    bool       IsBadWeatherHex(CLand * pLand, int month) const;

    //--------------------------------------------------------------------------
    // Public Member Variables
    //--------------------------------------------------------------------------
    
    int               m_CrntFactionId;           /**< Currently selected faction ID */
    CStr              m_CrntFactionPwd;           /**< Password for current faction */
    CLongColl         m_OurFactions;              /**< Collection of our faction IDs */
    CBaseObject       m_Events;                    /**< Game events collection */
    CBaseObject       m_SecurityEvents;            /**< Security events collection */
    CBaseObject       m_HexEvents;                  /**< Hex-specific events */
    CBaseObject       m_Errors;                     /**< Parse errors collection */
    CBaseColl         m_NewProducts;                /**< Newly produced items */

    CBaseCollById     m_Factions;                   /**< Factions indexed by ID */
    CBaseCollById     m_Units;                      /**< Units indexed by ID */
    CBaseColl         m_Planes;                     /**< Planes collection */
    long              m_YearMon;                     /**< Current year/month (year*100+month) */
    CStringSortColl   m_UnitPropertyNames;          /**< Unit property names */
    CStrIntColl       m_UnitPropertyTypes;          /**< Unit property types */
    CStringSortColl   m_LandPropertyNames;          /**< Land property names */
    CBaseColl         m_Skills;                      /**< Skills collection */
    CBaseColl         m_Items;                       /**< Items collection */
    CBaseColl         m_Objects;                     /**< Objects collection */
    CBaseColl         m_Battles;                     /**< Battles collection */
    CBaseCollById     m_Gates;                       /**< Gates indexed by ID */

    long              m_nCurLine;                    /**< Current line number while parsing */
    long              m_GatesCount;                  /**< Number of gates found */
    int               m_ParseErr;                     /**< Parse error code */
    BOOL              m_OrdersLoaded;                 /**< Whether orders are loaded */
    CStr              m_FactionInfo;                  /**< Faction information string */
    BOOL              m_ArcadiaSkills;                /**< Whether using Arcadia skill system */
    BOOL              m_RegexRulesLoaded;             /**< Whether regex rules are loaded */
    std::map<wxString, int> TerrainMovementCost;     /**< Terrain movement cost mapping */

    bool              m_EconomyTaxPillage;           /**< Whether tax/pillage economy is enabled */
    bool              m_EconomyShareAfterBuy;        /**< Whether to share silver after buying */
    bool              m_EconomyWork;                  /**< Whether work economy is enabled */
    bool              m_EconomyMaintainanceCosts;     /**< Whether maintenance costs are enabled */
    bool              m_EconomyShareMaintainance;     /**< Whether to share maintenance costs */

    /**
     * @brief Reads a property name from source string
     * @param src Source string
     * @param Name Output property name
     * @return Pointer after the property name
     */
    const char * ReadPropertyName(const char * src, CStr & Name);

    /**
     * @brief Loads regex rules from file
     * @param szFileName Rule filename
     * @return TRUE if loaded successfully
     */
    BOOL LoadRegexRules(const char* szFileName);
    
    /**
     * @brief Processes descriptions using loaded regex rules
     * @param pConfig Configuration file to update
     * @return TRUE if processing successful
     */
    BOOL ProcessDescriptionsWithRules(CConfigFile* pConfig);
    
    /**
     * @brief Updates configuration from descriptions
     * @param pConfig Configuration file to update
     * @return TRUE if update successful
     */
    BOOL UpdateConfigFromDescriptions(CConfigFile* pConfig);

protected:
    //--------------------------------------------------------------------------
    // Parsing Methods
    //--------------------------------------------------------------------------
    
    int          ParseFactionInfo(BOOL GetNo, BOOL Join);
    int          ParseEvents(BOOL IsEvents=TRUE);
    int          ParseUnclSilver(CStr & Line);
    int          ParseAttitudes(CStr & Line, BOOL Join);
    int          ParseTerrain (CLand * pMotherLand, int ExitDir, CStr & FirstLine, BOOL FullMode, CLand ** ppParsedLand);
    int          AnalyzeTerrain(CLand * pMotherLand, CLand * pLand, BOOL IsExit, int ExitDir, CStr & Description);
    void         ComposeHexDescriptionForArnoGame(const char * olddescr, const char * newdescr, CStr & CompositeDescr);

    void         ParseWages(CLand * pLand, const char * str1, const char * str2);
    void         CheckExit(CPlane * pPlane, int Direction, CLand * pLandSrc, CLand * pLandExit);
    int          ParseUnit(CStr & FirstLine, BOOL Join);
    int          ParseStructure (CStr & FirstLine);
    int          ParseErrors();
    int          ParseLines(BOOL Join);
    BOOL         ParseOneUnitEvent(CStr & EventLine, BOOL IsEvent, int UnitId);
    BOOL         ParseOneLandEvent(CStr & EventLine, BOOL IsEvent);
    void         ParseOneMovementEvent(const char * params, const char * structid, const char * fullevent);
    int          ParseOneEvent(CStr & EventLine, BOOL IsEvent);
    void         ParseWeather(const char * src, CLand * pLand);
    int          ApplyLandFlags();
    int          SetLandFlag(const char * p, long flag);
    int          SetLandFlag(long LandId, long flag);
    int          ParseBattles();
    int          ParseSkills();
    int          ParseItems();
    int          ParseObjects();
    void         SetExitFlagsAndTropicZone();
    int          SetUnitProperty(CUnit * pUnit, const char * name, EValueType type, const void * value, EPropertyType proptype);
    int          SetLandProperty(CLand * pLand, const char * name, EValueType type, const void * value, EPropertyType proptype);
    int          LoadOrders  (CFileReader & F, int FactionId, BOOL GetComments);
    void         StoreBattle(CStr & Source);
    void         AnalyzeBattle(const char * src, CStr & Details);
    void         AnalyzeBattle_OneSide(const char * src, CStr & Details);
    const char * AnalyzeBattle_ParseUnit(const char * src, CUnit *& pUnit, BOOL & InFrontLine);
    void         AnalyzeBattle_SummarizeUnits(CBaseColl & Units, CStr & Details);
    void         SetShaftLinks();
    void         ApplySailingEvents();
    BOOL         GetTargetUnitId(const char *& p, long FactionId, long & nId);
    int          ParseOneImportantEvent(CStr & EventLine);
    int          ParseImportantEvents();

    CUnit      * MakeUnit(long Id);
    CPlane     * MakePlane(const char * planename);

    //--------------------------------------------------------------------------
    // File Reading Utilities
    //--------------------------------------------------------------------------
    
    BOOL         ReadNextLine(CStr & s);
    BOOL         ReadNextLineMerged(CStr & s);
    void         PutLineBack (CStr & s);

    //--------------------------------------------------------------------------
    // Error Handling
    //--------------------------------------------------------------------------
    
    void         GenericErr(int Severity, const char * Msg);
    void         OrderErr(int Severity, int UnitId, const char * Msg, const char * UnitName = NULL, CUnit * = NULL);
    void         OrderErrFinalize();

    //--------------------------------------------------------------------------
    // Order Processing
    //--------------------------------------------------------------------------
    
    void         RunLandOrders(CLand * pLand, const char * sCheckTeach = NULL);
    void         OrderProcess_Teach(BOOL skiperror, CUnit * pUnit);

    // Order handlers
    void         RunOrder_Teach            (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params, BOOL TeachCheckGlb);
    void         RunOrder_Move             (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params, int & X, int & Y, int & LocA3, long order);
    void         RunOrder_Promote          (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Sell             (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Buy              (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Give             (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params, BOOL IgnoreMissingTarget);
    void         RunOrder_Take             (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params, BOOL IgnoreMissingTarget);
    void         RunOrder_Send             (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Produce          (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Study            (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_Name             (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunOrder_SailAIII         (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params, int & X, int & Y, int & LocA3);
    BOOL         FindTargetsForSend        (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char *& params, CUnit *& pUnit2, CLand *& pLand2);
    BOOL         GetItemAndAmountForGive   (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params, CStr & Item, int & amount, const char * command, CUnit * pUnit2);
    void         RunOrder_Withdraw         (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params);
    void         RunPseudoComment          (CStr & Line, CStr & ErrorLine, BOOL skiperror, CUnit * pUnit, CLand * pLand, const char * params, int sequence, wxString &destination);
    void         RunOrder_TaxPillage       (CLand *);
    void         RunOrder_Entertain        (CLand *);
    void         RunOrder_Work             (CLand *);
    void         RunOrder_ShareSilver      (CStr & Line, CStr & ErrorLine, BOOL skiperror, CLand *, SHARE_TYPE, wxString shareName);
    void         RunOrder_Upkeep           (CLand *);
    void         RunOrder_Upkeep           (CUnit *, int turns);
    void         DistributeSilver          (CLand* pLand, unsigned long flag, long totalSilver, long totalMen);

    long         CountMenWithFlag          (CLand* pLand, unsigned long flag);

    void         RunOrder_Transport(CStr& Line, CStr& ErrorLine, BOOL skiperror, CUnit* pUnit, CLand* pLand, const char* params);
    void         AddOrUpdateReceivedComment(CUnit* pReceiver, CUnit* pSender, const char* itemCode, int amount);
    CUnit*       FindUnitInRange(long unitId, CLand* pStartLand, int maxRadius);

    void         AdjustSkillsAfterGivingMen(CUnit * pUnitGive, CUnit * pUnitTake, CStr & item, long AmountGiven);
    void         LookupAdvancedResourceVisibility(CUnit * pUnit, CLand * pLand);

    int          ParseCBHex   (const char * FirstLine);
    int          ParseCBStruct(const char * FirstLine);

    void         UpdateUnitOrderLine(CUnit* pUnit, const char* newLine);

    //--------------------------------------------------------------------------
    // Protected Member Variables
    //--------------------------------------------------------------------------
    
    CFileReader    * m_pSource;                     /**< Current file reader */

    CStringSortColl  m_TaxLandStrs;                  /**< Tax-related land strings */
    CStringSortColl  m_TradeLandStrs;                 /**< Trade-related land strings */
    CStringSortColl  m_BattleLandStrs;                /**< Battle-related land strings */
    CLongSortColl    m_TradeUnitIds;                  /**< Trade-related unit IDs */
    CBaseCollByName  m_PlanesNamed;                   /**< Planes indexed by name */
    CBaseColl        m_LandsToBeLinked;               /**< Lands pending shaft linking */
    CHashStrToLong   m_UnitFlagsHash;                 /**< Unit flags hash map */
    CBaseColl        m_TempSailingEvents;             /**< Temporary sailing events */

    CLand          * m_pCurLand   ;                   /**< Current land being parsed */
    CStruct        * m_pCurStruct ;                   /**< Current structure being parsed */
    int              m_NextStructId;                  /**< Next available structure ID */
    CStr             m_sOrderErrors;                  /**< Order errors string */
    BOOL             m_JoiningRep;                     /**< Whether joining an ally's report */
    BOOL             m_IsHistory;                      /**< Whether parsing a history file */
    long             m_CurYearMon;                     /**< Year/month for current file */
    CStr             m_WeatherLine[8];                 /**< Weather description lines */
    CRegexParser     m_RegexParser;                      /**< Regular expression parser for description processing */
};

//--------------------------------------------------------------------------
// Global Macros
//--------------------------------------------------------------------------

#define CONTAINS    "contains "
#define SILVER      "silver"
#define FLAG_HDR    "$Flag"
#define YES         "Yes"
#define ORDER_CMNT  ";*** "

/**
 * @def GEN_ERR
 * @brief Macro for generating order errors
 */
#define GEN_ERR(pUnit, msg)                 \
{                                           \
    Line << msg;                            \
    OrderErr(0, pUnit->Id, Line.GetData()); \
    return Changed;                         \
}

//--------------------------------------------------------------------------
// Global Constants
//--------------------------------------------------------------------------

extern const char* Monthes[];
extern const char* EOL_MS;
extern const char* EOL_UNIX;
extern const char* EOL_SCR;
extern const char* EOL_FILE;
extern const char* STRUCT_UNIT_START;
extern const char* Directions[];
extern const int DirectionsCount;
extern const char* LocationsShipsArcadia[];
extern int ExitFlags[];
extern int EntryFlags[];
extern int Flags_NW_N_NE;
extern int Flags_N;
extern int Flags_SW_S_SE;
extern int Flags_S;
extern const char* ExitEndHeader[];
extern const int ExitEndHeaderCount;
extern int ExitEndHeaderLen[];
extern const char* BattleEndHeader[];
extern const int BattleEndHeaderCount;
extern int BattleEndHeaderLen[];

extern const char* BUG;
extern const char* NOSETUNIT;
extern const char* NOSET;
extern const char* NOTNUMERIC;

#endif