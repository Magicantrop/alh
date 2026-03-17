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

#ifndef __DATA_H_INCL__
#define __DATA_H_INCL__

#include "wx/string.h"
#include "cstr.h"
#include "collection.h"
#include "objs.h"
#include <string.h>
#include <wx/arrstr.h>
#include "compat.h"

/**
 * @enum eCompareOp
 * @brief Comparison operators for filtering
 */
typedef enum { GT = 0, GE, EQ, LE, LT, NE, NOP } eCompareOp;

//---------------------------------------------------------------------------
// Common Property Names
//---------------------------------------------------------------------------

#define PRP_ID                          "id"           /**< Object ID property */
#define PRP_NAME                        "name"         /**< Object name property */
#define PRP_FULL_TEXT                   "fulltext"     /**< Full text property */

//---------------------------------------------------------------------------
// Special Property Names
//---------------------------------------------------------------------------

#define PRP_ORG_NAME                    "org$name"     /**< Original name (before changes) */
#define PRP_ORG_DESCR                   "org$descr"    /**< Original description (before changes) */

//---------------------------------------------------------------------------
// Unit Property Names
//---------------------------------------------------------------------------

#define PRP_FACTION_ID                  "factionid"    /**< Faction ID property */
#define PRP_FACTION                     "faction"      /**< Faction name property */
#define PRP_LAND_ID                     "landid"       /**< Land ID property */
#define PRP_STRUCT_ID                   "structid"     /**< Structure ID property */
#define PRP_COMMENTS                    "comments"     /**< Unit comments property */
#define PRP_ORDERS                      "orders"       /**< Unit orders property */
#define PRP_STRUCT_OWNER                "structowner"  /**< Structure owner property */
#define PRP_STRUCT_NAME                 "structname"   /**< Structure name property */
#define PRP_TEACHING                    "teaching"     /**< Teaching status property */
#define PRP_WEIGHT                      "weight"       /**< Total weight property */
#define PRP_WEIGHT_WALK                 "weight_walk"  /**< Walking weight property */
#define PRP_WEIGHT_RIDE                 "weight_ride"  /**< Riding weight property */
#define PRP_WEIGHT_FLY                  "weight_fly"   /**< Flying weight property */
#define PRP_WEIGHT_SWIM                 "weight_swim"  /**< Swimming weight property */
#define PRP_BEST_SKILL                  "best_skill"   /**< Best skill name property */
#define PRP_BEST_SKILL_DAYS             "best_skill_days" /**< Best skill days property */
#define PRP_MOVEMENT                    "movement"     /**< Movement mode property */
#define PRP_SILVER                      "silv"         /**< Silver amount property */
#define PRP_LEADER                      "leadership"   /**< Leadership skill property */

#define PRP_FLAGS_STANDARD              "flags_standard" /**< Standard flags property */
#define PRP_FLAGS_CUSTOM                "flags_custom"   /**< Custom flags property */
#define PRP_FLAGS_CUSTOM_ABBR           "flags_custom_abbr" /**< Custom flags abbreviations */

#define PRP_PARENT_UNIT_ID              "parent_unit_id" /**< Parent unit ID (for split units) */

#define PRP_GUI_COLOR                   "gui_color"    /**< GUI display color property */
#define PRP_MEN                         "men"          /**< Men/soldiers property */
#define PRP_SKILLS                      "skills"       /**< Skills property */
#define PRP_MAG_SKILLS                   "mag_skills"  /**< Magic skills property */
#define PRP_STUFF                       "stuff"        /**< Miscellaneous items property */
#define PRP_FOOD                        "foods"        /**< Food items property */
#define PRP_HORS                        "mounts"       /**< Mounts property */
#define PRP_ARMOURS                     "armours"      /**< Armor items property */
#define PRP_WEAPONS                     "weapons"      /**< Weapon items property */
#define PRP_MAG_ITEMS                   "mag_items"    /**< Magic items property */
#define PRP_JUNK_ITEMS                  "junk_items"   /**< Junk items property */
#define PRP_TRADE_ITEMS                 "trade_items"  /**< Trade items property */
#define PRP_SEL_FACT_MEN                "sel_fact_men" /**< Selected faction men property */
#define PRP_SEQUENCE                    "sequence"     /**< Order sequence property */
#define PRP_DESCRIPTION                 "description"  /**< Unit description property */
#define PRP_COMBAT                      "combat_spell" /**< Combat spell property */
#define PRP_FRIEND_OR_FOE               "attitude"     /**< Attitude towards faction */

//---------------------------------------------------------------------------
// Unit Property Postfixes
//---------------------------------------------------------------------------

#define PRP_SKILL_POSTFIX                  "_"          /**< Skill postfix */
#define PRP_SKILL_STUDY_POSTFIX            "_s"         /**< Skill study postfix */
#define PRP_SKILL_EXPERIENCE_POSTFIX       "_x"         /**< Skill experience postfix */
#define PRP_SKILL_DAYS_POSTFIX             "_d"         /**< Skill days postfix */
#define PRP_SKILL_DAYS_EXPERIENCE_POSTFIX  "_dx"        /**< Skill days experience postfix */
#define PRP_GIVE_ALL_POSTFIX               "_ga"        /**< Give all postfix */

//---------------------------------------------------------------------------
// Land Property Names
//---------------------------------------------------------------------------

#define PRP_SALE_AMOUNT_PREFIX         "Sell.Amount."  /**< Sell amount prefix */
#define PRP_SALE_PRICE_PREFIX          "Sell.Price."   /**< Sell price prefix */
#define PRP_WANTED_AMOUNT_PREFIX       "Want.Amount."  /**< Wanted amount prefix */
#define PRP_WANTED_PRICE_PREFIX        "Want.Price."   /**< Wanted price prefix */
#define PRP_RESOURCE_PREFIX            "Resource."     /**< Resource prefix */
#define PRP_LAND_LINK                  "*landlink"     /**< Land link property */
#define PRP_FORM_REPEATED              "FormRepeated"  /**< Form repeated property */

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

#define MOVE_MODE_MAX                   5              /**< Maximum number of movement modes */
#define LAND_FLAG_COUNT                  3              /**< Number of land flags */

//---------------------------------------------------------------------------
// Land Flags
//---------------------------------------------------------------------------

#define LAND_UNITS          0x00000001  /**< Has units flag */
#define LAND_VISITED        0x00000002  /**< Visited flag */
#define LAND_TAX            0x00000004  /**< Tax taken this turn flag */
#define LAND_TRADE          0x00000008  /**< Trade done this turn flag */
#define LAND_BATTLE         0x00000010  /**< Battle occurred flag */
#define LAND_SET_EXITS      0x00000020  /**< Exits set flag */
#define LAND_HAS_FLAGS      0x00000040  /**< Has flags flag */
#define LAND_IS_CURRENT     0x00000080  /**< Currently selected flag */

#define LAND_STR_HIDDEN     0x00000100  /**< Has hidden structure */
#define LAND_STR_MOBILE     0x00000200  /**< Has mobile structure */
#define LAND_STR_SHAFT      0x00000400  /**< Has shaft structure */
#define LAND_STR_GATE       0x00000800  /**< Has gate structure */
#define LAND_STR_ROAD_N     0x00001000  /**< Has road north */
#define LAND_STR_ROAD_NE    0x00002000  /**< Has road northeast */
#define LAND_STR_ROAD_SE    0x00004000  /**< Has road southeast */
#define LAND_STR_ROAD_S     0x00008000  /**< Has road south */
#define LAND_STR_ROAD_SW    0x00010000  /**< Has road southwest */
#define LAND_STR_ROAD_NW    0x00020000  /**< Has road northwest */
#define LAND_STR_GENERIC    0x00040000  /**< Has generic structure */
#define LAND_STR_SPECIAL   (LAND_STR_HIDDEN  | LAND_STR_MOBILE  | LAND_STR_SHAFT   | LAND_STR_GATE   | \
                            LAND_STR_ROAD_N  | LAND_STR_ROAD_NE | LAND_STR_ROAD_SE | LAND_STR_ROAD_S | \
                            LAND_STR_ROAD_SW | LAND_STR_ROAD_NW) /**< Mask for special structures */

#define LAND_TAX_NEXT       0x00080000  /**< Tax scheduled for next turn */
#define LAND_TRADE_NEXT     0x00100000  /**< Trade scheduled for next turn */
#define LAND_LOCATED_UNITS  0x00200000  /**< Units located here */
#define LAND_LOCATED_LAND   0x00400000  /**< Land located (for exits) */
#define LAND_TOWN           0x00800000  /**< Is a town */
#define LAND_CITY           0x01000000  /**< Is a city */
#define LAND_IS_WATER       0x02000000  /**< Is water terrain */

//---------------------------------------------------------------------------
// Alarm Flags
//---------------------------------------------------------------------------

#define PRESENCE_OWN        0x0001      /**< Own faction presence */
#define PRESENCE_FRIEND     0x0002      /**< Friend faction presence */
#define PRESENCE_NEUTRAL    0x0004      /**< Neutral faction presence */
#define PRESENCE_ENEMY      0x0008      /**< Enemy faction presence */
#define GUARDED_BY_OWN      0x0010      /**< Guarded by own faction */
#define GUARDED_BY_FRIEND   0x0020      /**< Guarded by friend faction */
#define GUARDED_BY_NEUTRAL  0x0040      /**< Guarded by neutral faction */
#define GUARDED_BY_ENEMY    0x0080      /**< Guarded by enemy faction */
#define GUARDED             0x0100      /**< Guarded (general) */
#define CLAIMED_BY_OWN      0x0200      /**< Claimed by own faction */
#define CLAIMED_BY_FRIEND   0x0400      /**< Claimed by friend faction */
#define CLAIMED_BY_NEUTRAL  0x0800      /**< Claimed by neutral faction */
#define CLAIMED_BY_ENEMY    0x1000      /**< Claimed by enemy faction */
#define ALARM               0x2000      /**< Alarm triggered */

//---------------------------------------------------------------------------
// Structure Attributes
//---------------------------------------------------------------------------

#define SA_HIDDEN   0x0001              /**< Hidden structure */
#define SA_MOBILE   0x0002              /**< Mobile structure (can move) */
#define SA_SHAFT    0x0004              /**< Shaft structure (connects planes) */
#define SA_GATE     0x0008              /**< Gate structure */
#define SA_ROAD_N   0x0010              /**< Road north */
#define SA_ROAD_NE  0x0020              /**< Road northeast */
#define SA_ROAD_SE  0x0040              /**< Road southeast */
#define SA_ROAD_S   0x0080              /**< Road south */
#define SA_ROAD_SW  0x0100              /**< Road southwest */
#define SA_ROAD_NW  0x0200              /**< Road northwest */
#define SA_ROAD_BAD 0x0400              /**< Bad road (requires repair) */

//---------------------------------------------------------------------------
// Unit Flags - Standard from top, custom from bottom
//---------------------------------------------------------------------------

#define UNIT_FLAG_GIVEN             0x10000000  /**< Unit has given orders */
#define UNIT_FLAG_PILLAGING         0x08000000  /**< Unit is pillaging */
#define UNIT_FLAG_TAXING            0x04000000  /**< Unit is taxing */
#define UNIT_FLAG_PRODUCING         0x02000000  /**< Unit is producing */
#define UNIT_FLAG_GUARDING          0x01000000  /**< Unit is guarding */
#define UNIT_FLAG_AVOIDING          0x00800000  /**< Unit is avoiding */
#define UNIT_FLAG_BEHIND            0x00400000  /**< Unit is behind */
#define UNIT_FLAG_REVEALING_UNIT    0x00200000  /**< Unit is revealing unit */
#define UNIT_FLAG_REVEALING_FACTION 0x00100000  /**< Unit is revealing faction */
#define UNIT_FLAG_HOLDING           0x00080000  /**< Unit is holding */
#define UNIT_FLAG_RECEIVING_NO_AID  0x00040000  /**< Unit is receiving no aid */
#define UNIT_FLAG_CONSUMING_UNIT    0x00020000  /**< Unit is consuming unit */
#define UNIT_FLAG_CONSUMING_FACTION 0x00010000  /**< Unit is consuming faction */
#define UNIT_FLAG_NO_CROSS_WATER    0x00008000  /**< Unit cannot cross water */
#define UNIT_FLAG_SPOILS            0x00004000  /**< Unit takes spoils */
#define UNIT_FLAG_SHARING           0x00002000  /**< Unit is sharing */
#define UNIT_FLAG_TEMP              0x00001000  /**< Temporary unit */
#define UNIT_FLAG_ENTERTAINING      0x00000800  /**< Unit is entertaining */
#define UNIT_FLAG_WORKING           0x00000400  /**< Unit is working */

#define UNIT_CUSTOM_FLAG_COUNT   8              /**< Number of custom flags */
#define UNIT_CUSTOM_FLAG_MASK    0xFF           /**< Mask for custom flags */

#define NO_LOCATION         (-1)                 /**< No location indicator */

extern const char* STD_UNIT_PROPS[];             /**< Standard unit properties */
extern const int    STD_UNIT_PROPS_COUNT;        /**< Count of standard unit properties */

extern const char* STRUCT_GATE;                  /**< Gate structure identifier */

//---------------------------------------------------------------------------
// Attitude Types
//---------------------------------------------------------------------------

enum {
    ATT_FRIEND1 = 0,    /**< Trusted friends */
    ATT_FRIEND2,        /**< Preferred friends */
    ATT_NEUTRAL,        /**< Neutral factions */
    ATT_ENEMY,          /**< Enemy factions */
    ATT_UNDECLARED      /**< Undeclared attitude */
};

//---------------------------------------------------------------------------
// Unit ID Macros
//---------------------------------------------------------------------------

/**
 * @def NEW_UNIT_ID
 * @brief Creates a new unit ID based on sequence number and faction ID
 * @param _n Sequence number
 * @param _FactId Faction ID
 * @return Combined unit ID
 */
#define NEW_UNIT_ID(_n, _FactId) ((_FactId << 16) | _n)

/**
 * @def REVERSE_NEW_UNIT_ID
 * @brief Extracts sequence number from a new unit ID
 * @param _n Combined unit ID
 * @return Sequence number
 */
#define REVERSE_NEW_UNIT_ID(_n) (_n & 0xFFFF)

/**
 * @def IS_NEW_UNIT_ID
 * @brief Checks if an ID is a new unit ID
 * @param _Id Unit ID to check
 * @return TRUE if new unit ID
 */
#define IS_NEW_UNIT_ID(_Id)   ((_Id & 0xFFFF0000) != 0)

/**
 * @def IS_NEW_UNIT
 * @brief Checks if a unit is a new unit
 * @param _pUnit Pointer to unit
 * @return TRUE if new unit
 */
#define IS_NEW_UNIT(_pUnit)   IS_NEW_UNIT_ID(_pUnit->Id)

class CPlane;

//-----------------------------------------------------------------
// CBaseObject - Base class for all game objects
//-----------------------------------------------------------------

/**
 * @class CBaseObject
 * @brief Base class for all game objects with property handling
 * 
 * Provides common functionality for all game entities including
 * ID management, naming, description, and property access.
 */
class CBaseObject : public TPropertyHolder
{
public:
    CBaseObject();
    virtual ~CBaseObject() {};
    virtual void Done();

    /**
     * @brief Resolves an alias to its actual value
     * @param alias Alias to resolve
     * @return Resolved value or NULL
     */
    virtual const char* ResolveAlias(const char* alias);
    
    /**
     * @brief Gets a property value by name
     * @param name Property name
     * @param type Output property type
     * @param value Output pointer to property value
     * @param proptype Property type (normal/special)
     * @return TRUE if property found
     */
    virtual BOOL GetProperty(const char* name,
        EValueType& type,
        const void*& value,
        EPropertyType  proptype = eNormal
    );
    
    long Id;                 /**< Object unique identifier */
    CStr Name;               /**< Object name */
    CStr Description;        /**< Object description */

    void SetName(const char* newname);
    void SetDescription(const char* newdescr);
    void ResetName();
    void ResetDescription();
    virtual void ResetNormalProperties();

    virtual void DebugPrint(CStr& sDest);
    virtual void Clear() { Id = 0; Name.Empty(); Description.Empty(); };
};

//-----------------------------------------------------------------
// CBaseColl - Basic object collection
//-----------------------------------------------------------------

/**
 * @class CBaseColl
 * @brief Basic collection for CBaseObject pointers
 */
class CBaseColl : public CCollection
{
public:
    CBaseColl();
    CBaseColl(int nDelta);
protected:
    virtual void FreeItem(void* pItem);
};

//-----------------------------------------------------------------
// CBaseCollById - Sorted collection by ID
//-----------------------------------------------------------------

/**
 * @class CBaseCollById
 * @brief Sorted collection for objects with IDs
 * 
 * Maintains objects in sorted order based on their ID.
 */
class CBaseCollById : public CSortedCollection
{
public:
    CBaseCollById();
    CBaseCollById(int nDelta);
protected:
    virtual void FreeItem(void* pItem);
    virtual int Compare(void* pItem1, void* pItem2) const;
};

//-----------------------------------------------------------------
// CProduct - Product information
//-----------------------------------------------------------------

/**
 * @class CProduct
 * @brief Represents a product (resource) in a land
 */
class CProduct
{
public:
    long  Amount;          /**< Product amount */
    CStr  ShortName;       /**< Short name/code */
    CStr  LongName;        /**< Full descriptive name */
};

//-----------------------------------------------------------------
// CProductColl - Sorted product collection
//-----------------------------------------------------------------

/**
 * @class CProductColl
 * @brief Sorted collection of products by short name
 */
class CProductColl : public CSortedCollection
{
public:
    CProductColl() : CSortedCollection() {};
    CProductColl(int nDelta) : CSortedCollection(nDelta) {};
protected:
    virtual void FreeItem(void* pItem) { delete (CProduct*)pItem; };
    virtual int Compare(void* pItem1, void* pItem2) const
    {
        return(SafeCmp(((CProduct*)pItem1)->ShortName.GetData(),
            ((CProduct*)pItem2)->ShortName.GetData()));
    };
};

//-----------------------------------------------------------------
// CFaction - Faction class
//-----------------------------------------------------------------

/**
 * @class CFaction
 * @brief Represents a game faction
 */
class CFaction : public CBaseObject
{
public:
    CFaction() : CBaseObject() { UnclaimedSilver = 0; };
    long UnclaimedSilver;    /**< Unclaimed silver for this faction */

    virtual void DebugPrint(CStr& sDest);
};

//-----------------------------------------------------------------
// CAttitude - Faction attitude class
//-----------------------------------------------------------------

/**
 * @class CAttitude
 * @brief Represents attitude between factions
 */
class CAttitude : public CBaseObject
{
public:
    int FactionId;          /**< Target faction ID */
    int Stance;             /**< Attitude stance (ATT_* constants) */
    void    SetStance(int new_stance);
    BOOL    IsDeclaredAs(int attitude);
    BOOL    IsEqual(CAttitude* attitude);
};

//-----------------------------------------------------------------
// TUnitItem - Unit item structure
//-----------------------------------------------------------------

/**
 * @struct TUnitItem
 * @brief Represents an item in a unit's inventory
 */
struct TUnitItem
{
    CStr  itemAlias;   /**< Item display alias (e.g., "humans", "horse") */
    CStr  itemCode;    /**< Item code (e.g., "HUMN", "HORS") */
    long  itemCount;   /**< Item quantity */

    TUnitItem() : itemCount(0) {}
    TUnitItem(const wxString& alias, const wxString& code, long count)
        : itemAlias(alias.ToUTF8().data()), itemCode(code.ToUTF8().data()), itemCount(count) {}
};

//-----------------------------------------------------------------
// TUnitSkill - Unit skill structure
//-----------------------------------------------------------------

/**
 * @struct TUnitSkill
 * @brief Represents a skill possessed by a unit
 */
struct TUnitSkill
{
    CStr skillAlias;    /**< Skill display name (e.g., "force") */
    CStr skillCode;     /**< Skill code (e.g., "FORC") */
    long skillLevel;    /**< Skill level (1-10) */
    long skillDays;     /**< Days studied towards next level */

    TUnitSkill() : skillLevel(0), skillDays(0) {}
};

//-----------------------------------------------------------------
// TUnitFlagMapping - Unit flag mapping structure
//-----------------------------------------------------------------

/**
 * @struct TUnitFlagMapping
 * @brief Maps report flags to orders and internal flags
 */
struct TUnitFlagMapping
{
    CStr flagString;          /**< Flag name from report (e.g., "taxing") */
    CStr orderBase;         /**< Base order (e.g., "AUTOTAX", "CONSUME") */
    CStr orderValue;        /**< Value to enable (e.g., "1", "UNIT") */
    CStr orderDisable;      /**< Value to disable (e.g., "0", "") */
    unsigned long flagBit;  /**< Internal flag bit (e.g., UNIT_FLAG_TAXING) */

    TUnitFlagMapping() : flagBit(0) {}

    TUnitFlagMapping(const char* name,
        const char* baseOrder, const char* enValue,
        const char* disValue, unsigned long bit)
        : flagString(name),
        orderBase(baseOrder), orderValue(enValue),
        orderDisable(disValue), flagBit(bit) {}
};

//-----------------------------------------------------------------
// CUnit - Unit class
//-----------------------------------------------------------------

/**
 * @class CUnit
 * @brief Represents a game unit (group of men)
 * 
 * Contains all information about a unit including its location,
 * items, skills, orders, and various flags and states.
 */
class CUnit : public CBaseObject
{
public:
    CUnit();
    virtual ~CUnit();
    
    /**
     * @brief Gets the unit's best skill
     * @param name Output skill name
     * @param days Output skill days
     */
    void GetBestSkill(wxString& name, long& days);
    
    virtual BOOL GetProperty(const char* name,
        EValueType& type,
        const void*& value,
        EPropertyType  proptype = eNormal
    );
    virtual CStrStrColl* GetPropertyGroups();
    void    ExtractCommentsFromDefOrders();
    virtual void ResetNormalProperties();
    void    CheckWeight(CStr& sErr);
    void    CalcWeightsAndMovement();

    /**
     * @brief Gets skill level for a specific skill
     * @param skill Skill code
     * @return Skill level (0 if not known)
     */
    int     GetSkillLevel(const char* skill);
    
    /**
     * @brief Gets quartermaster skill level
     * @return Quartermaster level
     */
    int     GetQuamLevel();
    
    /**
     * @brief Checks if unit is a quartermaster
     * @return true if unit has quartermaster skill
     */
    bool    IsQuartermaster();

    /**
     * @brief Creates a simple copy of the unit (without relationships)
     * @return Pointer to new unit
     */
    CUnit* AllocSimpleCopy();

    static void         LoadCustomFlagNames();
    static void         ResetCustomFlagNames();
    static const char* GetCustomFlagName(int no);

    //-------------------------------------------------------------------------
    // End Turn Description Methods
    //-------------------------------------------------------------------------

    void    InitEndTurnDescription();
    void    ResetEndTurnDescription();

    wxString GetEndTurnUnitName();
    long     GetEndTurnUnitId();
    wxString GetEndTurnFactionName();
    long     GetEndTurnFactionId();
    BOOL     IsEndTurnOwnUnit();

    //-------------------------------------------------------------------------
    // Flag Methods
    //-------------------------------------------------------------------------

    static CStr GetFlagString(unsigned long flagBit);
    static CStr* ClearFlag(CStr* pDescr, const CStr& flagString);
    static CStr* SetFlag(CStr* pDescr, const CStr& flagString);
    static CStr* ClearSpoils(CStr* pDescr);
    static int GetPosFlag(CStr* pDescr, const CStr& searchString);
    static bool HasFlag(CStr* pDescr, const CStr& flagString);
    void BackErasedFlags();

    //-------------------------------------------------------------------------
    // Order Methods
    //-------------------------------------------------------------------------

    CStr GetOrders(const char* orderToFind) const;
    BOOL HasOrder(const char* orderToFind) const;
    BOOL HasTaxOrder() const;
    BOOL HasPillageOrder() const;
    
    //-------------------------------------------------------------------------
    // End Turn Item/Skill Methods
    //-------------------------------------------------------------------------

    long GetMenCount();
    long     GetEndTurnItemAmount(const wxString& itemCode);
    long     GetEndTurnItemAmount(const char* itemCode);
    long     GetEndTurnSkillDays(const wxString& skillCode);
    long     GetEndTurnSkillLevel(const wxString& skillCode);
    wxString GetEndTurnUserDescription();

    //-------------------------------------------------------------------------
    // Description Modification Methods
    //-------------------------------------------------------------------------

    CStr* SetEndTurnUnitName(CStr* pDescr, const wxString& unitName);
    CStr* SetEndTurnUnitId(CStr* pDescr, long unitId);
    CStr* SetEndTurnFactionName(CStr* pDescr, const wxString& factionName);
    CStr* SetEndTurnFactionId(CStr* pDescr, long factionId);

    //-------------------------------------------------------------------------
    // Item Manipulation Methods
    //-------------------------------------------------------------------------

    static TUnitItem GetItemByCode(CStr* pDescr, const wxString& itemCode);
    static std::vector<TUnitItem> GetItemsByGroup(CStr* pDescr, const wxString& group);
    CStr* AidItem(CStr* pDescr, const wxString& itemCode, long itemCount);
    std::vector<TUnitItem> GetItems(CStr* pDescr);
    wxString ItemsToStr(const std::vector<TUnitItem>& items);
    std::vector<TUnitItem> GetItemsByGroup(std::vector<TUnitItem>& a_ItemList, CStr* a_Group);
    long GetItemsCountByGroup(std::vector<TUnitItem>& a_ItemList, CStr* a_Group);
    long GetMenCountByDescr(CStr* pDescr);
    long GetWeight(std::vector<TUnitItem> a_Items);
    long GetWalkCapacity(std::vector<TUnitItem> a_Items);
    long GetFlyCapacity(std::vector<TUnitItem> a_Items);
    long GetSwimCapacity(std::vector<TUnitItem> a_Items);

    CStr* SetEndTurnUserDescription(CStr* pDescr, const wxString& userDescription);

    //-------------------------------------------------------------------------
    // Skill Manipulation Methods
    //-------------------------------------------------------------------------

    static TUnitSkill GetSkillByCode(CStr* pDescr, const wxString& skillCode);
    static TUnitSkill GetSkillByCode(const std::vector<TUnitSkill>& skills, const wxString& skillCode);
    static std::vector<TUnitSkill> GetAllSkills(CStr* pDescr);
    static CStr* AidSkillDays(CStr* pDescr, const wxString& skillCode, long newDays);
    static CStr* AddNewSkill(CStr* pDescr, const wxString& skillCode, long days);
    static CStr* RemoveSkill(CStr* pDescr, const wxString& skillCode);
    static long GetSkillLevelByDays(long days);
    wxString SkillsVectorToString(const std::vector<TUnitSkill>& skills);
    void RecalcSkills(long menCount, CUnit* pFromUnit);
    CStr* ReplaceSkillsBlock(CStr* pDescr, const wxString& newSkillsStr);
    CStr* RemoveSkillsBlock(CStr* pDescr);

    //-------------------------------------------------------------------------
    // Public Data Members
    //-------------------------------------------------------------------------

    bool            IsOurs;              /**< Whether unit belongs to our faction */
    long            FactionId;           /**< Faction ID */
    CFaction*       pFaction;            /**< Pointer to faction object */
    long            LandId;              /**< Current land ID */
    long            Weight[MOVE_MODE_MAX]; /**< Weights for different movement modes */
    long            SilvRcvd;            /**< Silver received this turn */
    double          Teaching;            /**< Teaching ratio (students/teacher or days/student) */
    CStr            Comments;            /**< User comments */
    CStr            DefOrders;           /**< Default orders */
    CStr            Orders;              /**< Current orders */
    CStr            OrdersDecorated;     /**< Decorated orders (with comments) */
    CStr            Errors;              /**< Order errors */
    CStr            Events;              /**< Unit events */
    CStr            StudyingSkill;       /**< Currently studying skill */
    CStr            ProducingItem;       /**< Currently producing item */
    CLongColl*      pMovement;           /**< Collection of hex IDs to move through */
    CLongColl*      pMoveA3Points;       /**< Arcadia III movement points */
    CBaseCollById*  pStudents;           /**< Collection of students (for teaching) */
    unsigned long   Flags;               /**< Current unit flags */
    unsigned long   FlagsOrg;            /**< Original flags (from report) */
    unsigned long   FlagsLast;           /**< Last known flags */
    int             reqMovementSpeed;    /**< Required movement speed for long paths */

    CStr            m_EndTurnDescription; /**< End turn description string */

    static CStrStrColl* m_PropertyGroupsColl; /**< Property groups collection */

    virtual void DebugPrint(CStr& sDest);

protected:
    void    AddWeight(int nitems, int* weights, const char** movenames, int nweights);

    static CStr     m_CustomFlagNames[UNIT_CUSTOM_FLAG_COUNT]; /**< Custom flag names */
    static BOOL     m_CustomFlagNamesLoaded;                   /**< Whether custom flags loaded */
};

//-----------------------------------------------------------------
// CStruct - Structure class
//-----------------------------------------------------------------

/**
 * @class CStruct
 * @brief Represents a structure in the game (building, ship, etc.)
 */
class CStruct : public CBaseObject
{
public:
    CStruct() : CBaseObject() {
        LandId = 0; OwnerUnitId = 0; Attr = 0; Location = NO_LOCATION;
        Load = 0; SailingPower = 0; MaxLoad = 0; MinSailingPower = 0;
    };
    virtual void ResetNormalProperties();
    
    long LandId;             /**< Land ID where structure is located */
    long OwnerUnitId;        /**< ID of owning unit */
    long Attr;               /**< Structure attributes (SA_* flags) */
    CStr Kind;               /**< Structure kind/type */
    int  Location;           /**< Location within hex (0 for ground) */
    long Load;               /**< Current load */
    long SailingPower;       /**< Current sailing power */
    long MaxLoad;            /**< Maximum load capacity */
    long MinSailingPower;    /**< Minimum sailing power required */
};

//-----------------------------------------------------------------
// CLand - Land/Hex class
//-----------------------------------------------------------------

/**
 * @class CLand
 * @brief Represents a single hex (land/sea) on the map
 * 
 * Contains all information about a hex including its terrain,
 * units, structures, resources, and various flags.
 */
class CLand : public CBaseObject
{
public:
    CLand();
    virtual ~CLand();

    BOOL      AddUnit(CUnit* pUnit);
    void      RemoveUnit(CUnit* pUnit);
    void      DeleteAllNewUnits(int factionId);
    void      ResetUnitsAndStructs();
    CStruct* GetStructById(long id);
    void      CalcStructsLoad();
    void      SetFlagsFromUnits();
    CStruct* AddNewStruct(CStruct* pNewStruct);
    void      RemoveEdgeStructs(int direction);
    void      AddNewEdgeStruct(const char* name, int direction);
    int       GetNextNewUnitNo();
    void      SetExit(int direction, int x, int y);
    void      CloseAllExits();
    void      ResetAllExits();
    int       FindExit(long hexId) const;

    /**
    * @brief Gets all lands within a specified radius of this land
    * @param radius Maximum distance in hexes
    * @param result Output vector of lands within radius
    */
    void GetRegionsInRadius(int radius, std::vector<CLand*>& result) const;


    virtual void DebugPrint(CStr& sDest);

    long          Taxable;           /**< Taxable silver */
    long          Peasants;          /**< Number of peasants */
    CStr          PeasantRace;       /**< Peasant race */
    CStr          TerrainType;       /**< Terrain type */
    CStr          CityName;          /**< City name (if any) */
    CStr          CityType;          /**< City type (town/city) */
    CStr          FlagText[LAND_FLAG_COUNT]; /**< User flag text */
    CStr          Exits;             /**< Exit descriptions */
    CStr          Events;            /**< Hex events */
    CBaseCollById Structs;           /**< Structures in hex */
    CBaseCollById Units;             /**< Units in hex (by ID) */
    CBaseColl     UnitsSeq;          /**< Units in report order */
    CBaseColl     EdgeStructs;       /**< Edge structures */
    CProductColl  Products;          /**< Products/resources */
    unsigned long Flags;             /**< Land flags */
    unsigned long AlarmFlags;        /**< Alarm flags */
    unsigned long EventFlags;        /**< Event flags */
    int           xExit[6];          /**< Exit X coordinates */
    int           yExit[6];          /**< Exit Y coordinates */
    int           CoastBits;         /**< Coastline bits */
    CPlane*       pPlane;            /**< Pointer to containing plane */
    int           AtlaclientsLastTurnNo; /**< Last turn number (Atlaclient format) */
    BOOL          WeatherWillBeGood; /**< Whether weather will be good */
    double        Wages;             /**< Current wages */
    long          MaxWages;          /**< Maximum wages */
    long          Entertainment;     /**< Entertainment silver */
    long          Troops[ATT_UNDECLARED + 1]; /**< Troop counts by attitude */

    long          guiUnit;           /**< GUI-specific unit reference */
    int           guiColor;          /**< GUI display color */

    int           TotalMovementCost; /**< Total movement cost (for routing) */
    long          ArrivedFromHexId;  /**< Hex ID we arrived from */
    CStr          MoveDirection;     /**< Direction we came from */
};

const int EXIT_CLOSED = -255;        /**< Closed exit indicator */
const int EXIT_MAYBE = -254;         /**< Possible exit indicator */

//-----------------------------------------------------------------
// CPlane - Plane class
//-----------------------------------------------------------------

#define TROPIC_ZONE_MAX   0x00001000    /**< Maximum tropic zone offset */

/**
 * @class CPlane
 * @brief Represents a game plane (Overworld, Underworld, etc.)
 */
class CPlane : public CBaseObject
{
public:
    CPlane() : Lands(32) {
        EastEdge = 0;  WestEdge = 0;   Width = 0;
        EdgeSrcId = 0; EdgeExitId = 0; EdgeDir = 0; ExitsCount = 0;
        TropicZoneMin = TROPIC_ZONE_MAX; TropicZoneMax = -(TROPIC_ZONE_MAX);
    };
    virtual ~CPlane();

    CBaseCollById Lands;           /**< Lands in this plane */
    int           EastEdge;        /**< Eastern edge coordinate */
    int           WestEdge;        /**< Western edge coordinate */
    int           Width;           /**< Plane width */

    long          EdgeSrcId;       /**< Edge source ID */
    long          EdgeExitId;      /**< Edge exit ID */
    int           EdgeDir;         /**< Edge direction */
    long          ExitsCount;      /**< Exit count (for history compatibility) */

    long          TropicZoneMin;   /**< Minimum Y for tropic zone */
    long          TropicZoneMax;   /**< Maximum Y for tropic zone */
};

//-----------------------------------------------------------------
// CShortNamedObj - Object with short name
//-----------------------------------------------------------------

/**
 * @class CShortNamedObj
 * @brief Object with a short name and level (e.g., skill, item)
 */
class CShortNamedObj : public CBaseObject
{
public:
    CShortNamedObj() : CBaseObject() { Level = 0; };
    CStr ShortName;        /**< Short name/code */
    long Level;            /**< Level/value */
};

//-----------------------------------------------------------------
// CBattle - Battle information
//-----------------------------------------------------------------

/**
 * @class CBattle
 * @brief Represents a battle that occurred
 */
class CBattle : public CBaseObject
{
public:
    CStr LandStrId;        /**< Land coordinate string where battle occurred */
};

//-----------------------------------------------------------------
// CBaseCollByName - Sorted collection by name
//-----------------------------------------------------------------

/**
 * @class CBaseCollByName
 * @brief Sorted collection for objects by name
 */
class CBaseCollByName : public CBaseCollById
{
public:
    CBaseCollByName() : CBaseCollById() {};
    CBaseCollByName(int nDelta) : CBaseCollById(nDelta) {};
protected:
    virtual int Compare(void* pItem1, void* pItem2) const;
};

//-----------------------------------------------------------------
// TProdDetails - Production details structure
//-----------------------------------------------------------------

#define MAX_RES_NUM 8                /**< Maximum number of resources per production */

/**
 * @struct TProdDetails
 * @brief Production details for an item
 */
class TProdDetails
{
public:
    CStr skillname;                  /**< Required skill name */
    long skilllevel;                 /**< Required skill level */

    long months;                     /**< Months to produce */
    CStr resname[MAX_RES_NUM];       /**< Required resource names */
    long resamt[MAX_RES_NUM];        /**< Required resource amounts */

    CStr toolname;                   /**< Required tool name */
    long toolhelp;                   /**< Tool help value */

    void Empty();                    /**< Clears all fields */
};

//-----------------------------------------------------------------
// CGameDataHelper - Game data helper class
//-----------------------------------------------------------------

/**
 * @class CGameDataHelper
 * @brief Provides access to game rules and configuration data
 * 
 * Central access point for game-specific data like study costs,
 * structure attributes, item weights, etc.
 */
class CGameDataHelper
{
public:
    void         ReportError(const char* msg, int msglen, BOOL orderrelated);
    long         GetStudyCost(const char* skill);
    long         GetStructAttr(const char* kind, long& MaxLoad, long& MinSailingPower);
    const char* GetConfString(const char* section, const char* param);
    BOOL         GetOrderId(const char* order, long& id);
    BOOL         IsTradeItem(const char* item);
    BOOL         IsMan(const char* item);
    const char* GetWeatherLine(BOOL IsCurrent, BOOL IsGood, int Zone);
    const char* ResolveAlias(const char* alias);
    BOOL         GetItemWeights(const char* item, int*& weights, const char**& movenames, int& movecount);
    void         GetMoveNames(const char**& movenames);
    BOOL         GetTropicZone(const char* plane, long& y_min, long& y_max);
    const char* GetPlaneSize(const char* plane);
    void         SetTropicZone(const char* plane, long y_min, long y_max);
    void         GetProdDetails(const char* item, TProdDetails& details);
    long         MaxSkillLevel(const char* race, const char* skill, const char* leadership, BOOL IsArcadiaSkillSystem);
    BOOL         ImmediateProdCheck();
    BOOL         CanSeeAdvResources(const char* skillname, const char* terrain, CLongColl& Levels, CBufColl& Resources);
    BOOL         ShowMoveWarnings();
    BOOL         IsRawMagicSkill(const char* skillname);
    int          GetAttitudeForFaction(int id);
    void         SetAttitudeForFaction(int id, int attitude);
    void         SetPlayingFaction(long id);
    BOOL         IsWagon(const char* item);
    BOOL         IsWagonPuller(const char* item);
    int          WagonCapacity();

    const char* GetAliasByCode(const char* code);
};

extern CGameDataHelper* gpDataHelper;    /**< Global game data helper instance */

//-----------------------------------------------------------------
// CTaxProdDetails - Tax/Production details for a faction
//-----------------------------------------------------------------

/**
 * @class CTaxProdDetails
 * @brief Tax or production details for a single faction
 */
class CTaxProdDetails
{
public:
    CTaxProdDetails() { HexCount = 0; FactionId = 0; amount = 0; }

    long  FactionId;       /**< Faction ID */
    long  HexCount;        /**< Number of hexes */
    long  amount;          /**< Total amount */
    CStr  Details;         /**< Detailed description */
};

//-----------------------------------------------------------------
// CTaxProdDetailsCollByFaction - Collection by faction ID
//-----------------------------------------------------------------

/**
 * @class CTaxProdDetailsCollByFaction
 * @brief Sorted collection of tax/production details by faction ID
 */
class CTaxProdDetailsCollByFaction : public CSortedCollection
{
public:
    CTaxProdDetailsCollByFaction() : CSortedCollection() {};
    CTaxProdDetailsCollByFaction(int nDelta) : CSortedCollection(nDelta) {};
protected:
    virtual void FreeItem(void* pItem)
    {
        CTaxProdDetails* p = (CTaxProdDetails*)pItem;
        delete p;
    };
    virtual int Compare(void* pItem1, void* pItem2) const
    {
        CTaxProdDetails* p1 = (CTaxProdDetails*)pItem1;
        CTaxProdDetails* p2 = (CTaxProdDetails*)pItem2;

        if (p1->FactionId > p2->FactionId)
            return 1;
        else if (p1->FactionId < p2->FactionId)
            return -1;
        else
            return 0;
    };
};

//-----------------------------------------------------------------
// Global Utility Functions
//-----------------------------------------------------------------

/**
 * @brief Converts coordinates to a single land ID
 * @param x X coordinate
 * @param y Y coordinate
 * @param z Plane number
 * @return Combined land ID
 */
long LandCoordToId(int x, int y, int z);

/**
 * @brief Extracts coordinates from a land ID
 * @param id Land ID
 * @param x Output X coordinate
 * @param y Output Y coordinate
 * @param z Output plane number
 */
void LandIdToCoord(long id, int& x, int& y, int& z);

/**
 * @brief Tests land ID conversion (debug function)
 */
void TestLandId();

/**
 * @brief Checks if property name is skill-related
 * @param propname Property name to check
 * @return TRUE if skill-related
 */
BOOL IsASkillRelatedProperty(const char* propname);

/**
 * @brief Creates qualified property name (prefix + shortname)
 * @param prefix Property prefix
 * @param shortname Short name
 * @param FullName Output full qualified name
 */
void MakeQualifiedPropertyName(const char* prefix, const char* shortname, CStr& FullName);

/**
 * @brief Splits qualified property name into prefix and short name
 * @param fullname Full qualified name
 * @param Prefix Output prefix
 * @param ShortName Output short name
 */
void SplitQualifiedPropertyName(const char* fullname, CStr& Prefix, CStr& ShortName);

/**
 * @brief Evaluates object against filter conditions
 * @param pObj Object to evaluate
 * @param Property Property names array
 * @param CompareOp Compare operators array
 * @param Value String values array
 * @param lValue Long values array
 * @param count Number of conditions
 * @return TRUE if object matches all conditions
 */
BOOL EvaluateBaseObjectByBoxes(CBaseObject* pObj, CStr* Property, eCompareOp* CompareOp, CStr* Value, long* lValue, int count);

#endif