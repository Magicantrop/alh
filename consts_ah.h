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

#ifndef __CONSTANTS_FOR_US__
#define __CONSTANTS_FOR_US__

/**
 * @struct DefaultConfigRec
 * @brief Structure for default configuration entries
 */
typedef struct
{
    const char * szSection;  /**< Configuration section name */
    const char * szName;      /**< Parameter name */
    const char * szValue;     /**< Default value */
} DefaultConfigRec;

extern DefaultConfigRec DefaultConfig[];   /**< Array of default configuration entries */
extern int              DefaultConfigSize; /**< Number of default configuration entries */

//---------------------------------------------------------------------------
// Menu Command IDs
//---------------------------------------------------------------------------

enum
{
    // Main menu commands
    menu_LoadReport    = 1  , /**< Load report file */
    menu_JoinReport         , /**< Join additional report */
    menu_LoadOrders         , /**< Load orders file */
    menu_SaveOrders         , /**< Save orders to current file */
    menu_SaveOrdersAs       , /**< Save orders to new file */
    menu_Quit               , /**< Quit application */
    menu_QuitNoSave         , /**< Quit without saving */
    menu_Options            , /**< Open options dialog */
    menu_ViewSkillsAll      , /**< View all skills */
    menu_ViewSkillsNew      , /**< View new skills */
    menu_ViewItemsAll       , /**< View all items */
    menu_ViewItemsNew       , /**< View new items */
    menu_ViewObjectsAll     , /**< View all objects */
    menu_ViewObjectsNew     , /**< View new objects */
    menu_ViewBattlesAll     , /**< View all battles */
    menu_ViewEvents         , /**< View events */
    menu_ViewErrors         , /**< View errors */
    menu_ViewGates          , /**< View gates */
    menu_ViewCities         , /**< View cities */
    menu_ViewProvinces      , /**< View provinces */
    menu_ViewNewProducts    , /**< View new products */
    menu_ViewFactionInfo    , /**< View faction information */
    menu_ViewFactionOverview, /**< View faction overview */
    menu_ViewSecurityEvents , /**< View security events */
    menu_WindowUnits        , /**< Show units window */
    menu_WindowMessages     , /**< Show messages window */
    menu_WindowEditors      , /**< Show editors window */
    menu_WindowUnitsFltr    , /**< Show filtered units window */
    menu_ListColUnits       , /**< Edit units list columns */
    menu_ListColUnitsFltr   , /**< Edit filtered units list columns */
    menu_ListColumns        , /**< Edit list columns (generic) */
    menu_ApplyDefaultOrders , /**< Apply default orders to units */
    menu_RerunOrders        , /**< Re-run orders through parser */
    menu_ShaftConnect       , /**< Open shaft connection GUI */
    menu_WriteMagesCSV      , /**< Export mages to CSV */
    menu_CheckMonthLongOrd  , /**< Check for month-long orders */
    menu_CheckTaxTrade      , /**< Check tax and trade */
    menu_CheckProduction    , /**< Check production */
    menu_CheckSailing       , /**< Check sailing */
    menu_TaxPillage         , /**< Toggle tax/pillage economy */
    menu_ShareAfterBuy      , /**< Toggle share after buy economy */
    menu_Work               , /**< Toggle work economy */
    menu_MaintainanceCosts  , /**< Toggle maintenance costs */
    menu_ShareMaintainance  , /**< Toggle share maintenance */
    menu_FindHexes          , /**< Find hexes dialog */
    menu_ExportHexes        , /**< Export hexes to file */
    menu_FlagNames          , /**< Show flag names */
    menu_FlagsAllSet        , /**< Set all flags */
    menu_FindTradeRoutes    , /**< Find trade routes */
    menu_About              , /**< About dialog */
    menu_PrepareBattle      , /**< Prepare battle dialog */

    // Map popup menu commands
    menu_Popup_Flag         , /**< Set flag on hex */
    menu_Popup_Center       , /**< Center map on hex */
    menu_Popup_ShareSilv    , /**< Share silver in hex */
    menu_Popup_Teach        , /**< Generate teaching orders */
    menu_Popup_Split        , /**< Split unit */
    menu_Popup_DiscardJunk  , /**< Discard junk items */
    menu_Popup_DetectSpies  , /**< Detect spies */
    menu_Popup_Filter       , /**< Filter units */
    menu_Popup_SetSort      , /**< Set sort order */
    menu_Popup_Battles      , /**< Show battles */
    menu_Popup_GiveEverything, /**< Give everything to another unit */
    menu_Popup_ScoutSimple  , /**< Scout (simple) */
    menu_Popup_ScoutMove    , /**< Scout (move) */
    menu_Popup_ScoutObserver, /**< Scout (observer) */
    menu_Popup_ScoutStealth , /**< Scout (stealth) */
    menu_Popup_ScoutGuard   , /**< Scout (guard) */
    menu_Popup_AddToTracking, /**< Add to tracking group */
    menu_Popup_UnitFlags    , /**< Set unit flags */
    menu_Popup_IssueOrders  , /**< Issue orders dialog */
    menu_Popup_WhoMovesHere , /**< Show units moving here */
    menu_Popup_Financial    , /**< Show financial info */
    menu_Popup_New_Hex      , /**< Add temporary hex */
    menu_Popup_Del_Hex      , /**< Delete temporary hex */
    menu_Popup_DistanceRing , /**< Show distance ring */

    /* NEW - Additional popup commands */
    menu_Popup_Give         , /**< Give items */
    menu_Popup_Take         , /**< Take items */
    menu_Popup_MakeTemplate , /**< Make orders template */
    menu_Popup_CreateFromTemplate, /**< Create orders from template */
    menu_Popup_Transport    , /**< Transport items */
    menu_Popup_Produce      , /**< Produce items */

    /* Battle Simulator popup commands */
    menu_Popup_AddToAttackers, /**< Add unit to attackers */
    menu_Popup_AddToDefenders, /**< Add unit to defenders */

    /* Selection area commands */
    menu_Selection_Export   , /**< Export selected area */
    menu_Selection_Clear    , /**< Clear selection */
    menu_Selection_FindUnit , /**< Find unit in selection */
    menu_Selection_FindItem , /**< Find item in selection */
    menu_Selection_FindResource, /**< Find resource in selection */
    menu_Selection_FindSkill, /**< Find skill in selection */

    // Accelerator key IDs
    accel_NextUnit          , /**< Next unit accelerator */
    accel_PrevUnit          , /**< Previous unit accelerator */
    accel_UnitList          , /**< Unit list accelerator */
    accel_Orders            , /**< Orders accelerator */

    // Toolbar button IDs
    tool_zoomin             , /**< Zoom in */
    tool_zoomout            , /**< Zoom out */
    tool_centerout          , /**< Center out */
    tool_prevzoom           , /**< Previous zoom */
    tool_showcoord          , /**< Show coordinates */
    tool_shownames          , /**< Show names */
    tool_planeup            , /**< Plane up */
    tool_planedwn           , /**< Plane down */
    tool_lastvisitturn      , /**< Last visited turn */
    tool_prevturn           , /**< Previous turn */
    tool_nextturn           , /**< Next turn */
    tool_lastturn           , /**< Last turn */
    tool_findhex            , /**< Find hex */

    // Other control IDs
    list_units_hex          , /**< Units in hex list control */
    list_units_hex_fltr     , /**< Filtered units in hex list control */
    combo_TargetUnit        , /**< Target unit combo box */
    btn_CheckAll            , /**< Check all button */
    btn_UncheckAll            /**< Uncheck all button */
};

//---------------------------------------------------------------------------
// File Dialog Filters
//---------------------------------------------------------------------------

#if defined(_WIN32)
  #define SZ_REP_FILES                  "Report (*.rep)|*.rep|Report (*.atl)|*.atl|Text (*.txt)|*.txt|All|*.*||"
  #define SZ_ORD_FILES                  "Order (*.ord)|*.ord|Text (*.txt)|*.txt|All|*.*||"
  #define SZ_CSV_FILES                  "Comma Separated (*.csv)|*.csv|All|*.*||"
  #define SZ_ALL_FILES                  "All|*.*||"
#else
  #define SZ_REP_FILES                  "Report (*.rep)|*.rep|Report (*.atl)|*.atl|Text (*.txt)|*.txt|All|*||"
  #define SZ_ORD_FILES                  "Order (*.ord)|*.ord|Text (*.txt)|*.txt|All|*||"
  #define SZ_CSV_FILES                  "Comma Separated (*.csv)|*.csv|All|*||"
  #define SZ_ALL_FILES                  "All|*||"
#endif

#define SZ_CB_DATA_FILES                "Text (*.txt)|*.txt|All|*.*||"  /**< Crossbow data files filter */

//---------------------------------------------------------------------------
// Standard File Names
//---------------------------------------------------------------------------

#define SZ_CONFIG_FILE                  "ah.cfg"           /**< Main configuration file */
#define SZ_CONFIG_STATE_FILE            "ah.st.cfg"        /**< State configuration file */
#define SZ_HISTORY_FILE                 "ah.his"           /**< History file */
#define SZ_COMMON_PY_FILE               "ah.common.py"     /**< Common Python script file */

//---------------------------------------------------------------------------
// End-of-Line Formats
//---------------------------------------------------------------------------

#define SZ_EOL_MS                       "MS"               /**< MS Windows style EOL (CRLF) */
#define SZ_EOL_UNIX                     "UNIX"             /**< Unix style EOL (LF) */

//---------------------------------------------------------------------------
// Configuration Sections
//---------------------------------------------------------------------------

#define SZ_SECT_COLORS                  "COLORS"           /**< Colors configuration */
#define SZ_SECT_COMMON                  "COMMON"           /**< Common settings */
#define SZ_SECT_FONTS                   "FONTS"            /**< Font settings */
#define SZ_SECT_FONTS_2                 "FONTS_2"          /**< Additional font settings */
#define SZ_SECT_UNITLIST_HDR            "UNIT_LIST_HDR"    /**< Unit list headers */
#define SZ_SECT_UNITLIST_HDR_FLTR       "UNIT_LIST_HDR_FLTR" /**< Filtered unit list headers */
#define SZ_SECT_UNITPROP_GROUPS         "UNIT_PROPERTY_GROUPS" /**< Unit property groups */
#define SZ_SECT_STUDY_COST              "STUDYING_COST"    /**< Study costs per skill */
#define SZ_SECT_ALIAS                   "ALIASES"          /**< Item/skill aliases */
#define SZ_SECT_STRUCTS                 "STRUCTURES"       /**< Structure definitions */
#define SZ_SECT_PROD_SKILL              "PRODUCTION_SKILL" /**< Production skills */
#define SZ_SECT_PROD_RESOURCE           "PRODUCTION_RESOURCE" /**< Production resources */
#define SZ_SECT_PROD_TOOL               "PRODUCTION_TOOL"  /**< Production tools */
#define SZ_SECT_PROD_MONTHS             "PRODUCTION_MONTHS" /**< Production months */
#define SZ_SECT_MAX_SKILL_LVL           "MAX_SKILL_LEVELS" /**< Max skill levels (non-magic) */
#define SZ_SECT_MAX_MAG_SKILL_LVL       "MAX_SKILL_LEVELS_MAGIC" /**< Max magic skill levels */

#define SZ_SECT_MAP_PANE                "PANE_MAP"         /**< Map pane settings */
#define SZ_SECT_WEIGHT_MOVE             "WEIGHTS_CAPACITIES" /**< Item weights and capacities */
#define SZ_SECT_UNIT_FLAG_NAMES         "UNIT_FLAG_NAMES"  /**< Unit flag names */
#define SZ_SECT_WND_MAP_2_WIN           "WINDOW_MAP"       /**< Map window (2-window layout) */
#define SZ_SECT_WND_MAP_3_WIN           "WINDOW_MAP_1"     /**< Map window (3-window layout) */
#define SZ_SECT_WND_MAP_1_WIN           "WINDOW_MAP_2"     /**< Map window (1-window layout) */
#define SZ_SECT_WND_SHAFTS              "WINDOW_SHAFTS"    /**< Shafts window */
#define SZ_SECT_WND_UNITS_2_WIN         "WINDOW_UNITS"     /**< Units window (2-window layout) */
#define SZ_SECT_WND_UNITS_3_WIN         "WINDOW_UNITS_1"   /**< Units window (3-window layout) */
#define SZ_SECT_WND_UNITS_FLTR          "WINDOW_UNITS_FLTR" /**< Filtered units window */
#define SZ_SECT_WND_HEX_FLTR            "WINDOW_HEX_FLTR"  /**< Filtered hex window */
#define SZ_SECT_WND_EDITS_2_WIN         "WINDOW_EDITORS"   /**< Editors window (2-window layout) */
#define SZ_SECT_WND_EDITS_3_WIN         "WINDOW_EDITORS_1" /**< Editors window (3-window layout) */
#define SZ_SECT_WND_MSG                 "WINDOW_MESSAGES"  /**< Messages window */
#define SZ_SECT_WND_DESCR_ONE           "WINDOW_DESCRIPTION_ONE" /**< Single description window */
#define SZ_SECT_WND_DESCR_LIST          "WINDOW_DESCRIPTION_LIST" /**< Description list window */
#define SZ_SECT_WND_EXP_MAGES_CSV       "WINDOW_EXPORT_MAGES_CSV" /**< Export mages CSV window */
#define SZ_SECT_WND_EXP_HEXES           "WINDOW_EXPORT_HEXES_DLG" /**< Export hexes dialog */
#define SZ_SECT_WND_UNITS_FLTR_DLG      "WINDOW_UNITS_FLTR_DLG" /**< Units filter dialog */
#define SZ_SECT_WND_HEX_FLTR_DLG        "WINDOW_HEX_FLTR_DLG" /**< Hex filter dialog */
#define SZ_SECT_WND_LST_COLEDIT_DLG     "WINDOW_LIST_COL_EDIT_DLG" /**< List column edit dialog */
#define SZ_SECT_WND_OPTIONS_DLG         "WINDOW_OPTIONS"   /**< Options dialog */
#define SZ_SECT_WND_SPLIT_UNIT_DLG      "WINDOW_SPLIT_UNIT_DLG" /**< Split unit dialog */
#define SZ_SECT_WND_LAND_UNIT_FLAGS_DLG "WINDOW_LAND_UNIT_FLAGS_DLG" /**< Land/unit flags dialog */
#define SZ_SECT_WND_HEX_FLAGS_DLG       "WINDOW_HEX_FLAGS_DLG" /**< Hex flags dialog */
#define SZ_SECT_WND_GET_TEXT_DLG        "WINDOW_GET_TEXT_DLG" /**< Get text dialog */

#define SZ_SECT_LIST_COL_CURRENT        "LIST_COL_CUR_SET" /**< Current list column set */
#define SZ_SECT_LIST_COL_UNIT           "LIST_COL_UNIT_"   /**< Unit list columns */
#define SZ_SECT_LIST_COL_UNIT_DEF       "LIST_COL_UNIT_DEFAULT" /**< Default unit list columns */
#define SZ_SECT_LIST_COL_UNIT_FLTR_DEF  "LIST_COL_UNIT_DEFAULT_FLTR" /**< Default filtered unit list columns */

#define SZ_SECT_UNIT_FILTER             "UNIT_FLTR_"       /**< Unit filters */
#define SZ_SECT_HEX_FILTER              "HEX_FLTR_"        /**< Hex filters */
#define SZ_SECT_EDGE_STRUCTS            "EDGE_STRUCTURES"  /**< Edge structures */

#define SZ_SECT_TERRAIN_COST            "TERRAIN_COST"     /**< Terrain movement costs */
#define SZ_SECT_WEATHER                 "WEATHER"          /**< Weather descriptions */
#define SZ_SECT_RESOURCE_SKILL          "RESOURCE_SKILL"   /**< Resource detection skills */
#define SZ_SECT_RESOURCE_LAND           "RESOURCE_LAND"    /**< Land resources */
#define SZ_SECT_ATTITUDES               "ATTITUDES"        /**< Faction attitudes */

//---------------------------------------------------------------------------
// State Configuration Sections
//---------------------------------------------------------------------------

#define SZ_SECT_DEF_ORDERS              "DEFAULT_ORDERS"   /**< Default orders */
#define SZ_SECT_ORDERS                  "ORDER_FILES"      /**< Orders files history */
#define SZ_SECT_REPORTS                 "REPORT_FILES"     /**< Report files history */
#define SZ_SECT_LAND_FLAGS              "LAND_FLAGS"       /**< Land flags */
#define SZ_SECT_LAND_VISITED            "LAND_LAST_VISITED" /**< Last visited land */
#define SZ_SECT_SKILLS                  "SKILL_DESCRIPTIONS" /**< Skill descriptions */
#define SZ_SECT_ITEMS                   "ITEM_DESCRIPTIONS" /**< Item descriptions */
#define SZ_SECT_OBJECTS                 "OBJECT_DESCRIPTIONS" /**< Object descriptions */
#define SZ_SECT_PASSWORDS               "PASSWORDS"        /**< Faction passwords */
#define SZ_SECT_UNIT_TRACKING           "UNIT_TRACKING_GROUPS" /**< Unit tracking groups */
#define SZ_SECT_FOLDERS                 "FOLDERS"          /**< Folder paths */
#define SZ_SECT_DO_NOT_SHOW_THESE       "DO_NOT_SHOW_THESE" /**< Items to hide */
#define SZ_SECT_TROPIC_ZONE             "TROPIC_ZONE"      /**< Tropic zone definitions */
#define SZ_SECT_PLANE_SIZE              "PLANE_SIZE"       /**< Plane dimensions */
#define SZ_SECT_UNIT_FLAGS              "UNIT_FLAGS"       /**< Unit flags */

//---------------------------------------------------------------------------
// Compatibility Definitions (for 1.0)
//---------------------------------------------------------------------------

#define SZ_SECT_WND_MAP                  SZ_SECT_WND_MAP_2_WIN
#define SZ_SECT_WND_UNITS                SZ_SECT_WND_UNITS_2_WIN

//---------------------------------------------------------------------------
// Configuration Keys - Weather
//---------------------------------------------------------------------------

#define SZ_KEY_WEATHER_CUR_GOOD_TROPIC   "CUR_GOOD_TROPIC"  /**< Current good weather (tropic) */
#define SZ_KEY_WEATHER_CUR_GOOD_MEDIUM   "CUR_GOOD_MEDIUM"  /**< Current good weather (medium) */
#define SZ_KEY_WEATHER_CUR_BAD_TROPIC    "CUR_BAD_TROPIC"   /**< Current bad weather (tropic) */
#define SZ_KEY_WEATHER_CUR_BAD_MEDIUM    "CUR_BAD_MEDIUM"   /**< Current bad weather (medium) */
#define SZ_KEY_WEATHER_NEXT_GOOD_TROPIC  "NEXT_GOOD_TROPIC" /**< Next good weather (tropic) */
#define SZ_KEY_WEATHER_NEXT_GOOD_MEDIUM  "NEXT_GOOD_MEDIUM" /**< Next good weather (medium) */
#define SZ_KEY_WEATHER_NEXT_BAD_TROPIC   "NEXT_BAD_TROPIC"  /**< Next bad weather (tropic) */
#define SZ_KEY_WEATHER_NEXT_BAD_MEDIUM   "NEXT_BAD_MEDIUM"  /**< Next bad weather (medium) */

//---------------------------------------------------------------------------
// Configuration Keys - Folders
//---------------------------------------------------------------------------

#define SZ_KEY_FOLDER_ORDERS             "ORDERS"           /**< Orders folder */
#define SZ_KEY_FOLDER_REP_LOAD           "REPORT_LOAD"      /**< Report load folder */
#define SZ_KEY_FOLDER_REP_JOIN           "REPORT_JOIN"      /**< Report join folder */

//---------------------------------------------------------------------------
// Configuration Keys - Window Layout
//---------------------------------------------------------------------------

#define SZ_KEY_Y1                        "WIN_TOP"          /**< Window top coordinate */
#define SZ_KEY_Y2                        "WIN_BOTTOM"       /**< Window bottom coordinate */
#define SZ_KEY_X1                        "WIN_LEFT"         /**< Window left coordinate */
#define SZ_KEY_X2                        "WIN_RIGHT"        /**< Window right coordinate */
#define SZ_KEY_USE_SAVED_POS             "USE_SAVED_POS"    /**< Use saved position flag */
#define SZ_KEY_HEIGHT_0                   "HEIGHT_0"        /**< Splitter height 0 */
#define SZ_KEY_HEIGHT_1                   "HEIGHT_1"        /**< Splitter height 1 */
#define SZ_KEY_HEIGHT_2                   "HEIGHT_2"        /**< Splitter height 2 */
#define SZ_KEY_HEIGHT_3                   "HEIGHT_3"        /**< Splitter height 3 */
#define SZ_KEY_WIDTH_0                    "WIDTH_0"         /**< Splitter width 0 */
#define SZ_KEY_WIDTH_1                    "WIDTH_1"         /**< Splitter width 1 */
#define SZ_KEY_WIDTH_2                    "WIDTH_2"         /**< Splitter width 2 */
#define SZ_KEY_WIDTH_3                    "WIDTH_3"         /**< Splitter width 3 */

//---------------------------------------------------------------------------
// Configuration Keys - Map Settings
//---------------------------------------------------------------------------

#define SZ_KEY_HEX_SIZE                  "HEX_SIZE"         /**< Hex size on map */
#define SZ_KEY_HEX_SIZE_OLD              "HEX_SIZE_OLD"     /**< Old hex size */
#define SZ_KEY_ATLA_X0                   "ATLA_LEFT"        /**< Map left coordinate */
#define SZ_KEY_ATLA_Y0                   "ATLA_TOP"         /**< Map top coordinate */
#define SZ_KEY_HEX_SEL_X                 "HEX_SELECTED_X"   /**< Selected hex X */
#define SZ_KEY_HEX_SEL_Y                 "HEX_SELECTED_Y"   /**< Selected hex Y */
#define SZ_KEY_STATE                     "SHOW_STATE"       /**< Show state */
#define SZ_KEY_PLANE_SEL                 "PLANE_SELECTED"   /**< Selected plane */
#define SZ_KEY_OPEN                      "IS_OPEN"          /**< Window open flag */
#define SZ_KEY_LOAD_ORDER                "LOAD_ORDERS"      /**< Load orders on start */
#define SZ_KEY_LOAD_REP                  "LOAD_REPORT"      /**< Load report on start */
#define SZ_KEY_EOL                       "EOL"              /**< End-of-line format */
#define SZ_KEY_PWD_OLD                   "PASSWORD"         /**< Old password storage */
#define SZ_KEY_FIRST_SHIP_NUMBER         "FIRST_SHIP_NUMBER" /**< First ship number */
#define SZ_KEY_MOVEMEMENT_SPEED_WALK     "MOVEMENT_SPEED_WALK" /**< Walking speed */
#define SZ_KEY_MOVEMEMENT_SPEED_RIDE     "MOVEMENT_SPEED_RIDE" /**< Riding speed */
#define SZ_KEY_MOVEMEMENT_SPEED_FLY      "MOVEMENT_SPEED_FLY" /**< Flying speed */
#define SZ_KEY_HATCH_UNVISITED           "HATCH_UNVISITED"  /**< Hatch unvisited hexes */
#define SZ_KEY_SORT1                     "SORT_PRIMARY"     /**< Primary sort */
#define SZ_KEY_SORT2                     "SORT_SECONDARY"   /**< Secondary sort */
#define SZ_KEY_SORT3                     "SORT_TERTIARY"    /**< Tertiary sort */
#define SZ_KEY_RCLICK_CENTERS            "RCLICK_CENTERS"   /**< Right-click centers */
#define SZ_KEY_DEFAULT_EMPTY_ONLY        "DEFAULT_EMPTY_ONLY" /**< Default orders for empty only */
#define SZ_KEY_REP_CACHE_COUNT           "REPORT_CACHE_MAX" /**< Max reports in cache */
#define SZ_KEY_MIN_SEL_MEN               "MEN_THRESHOLD"    /**< Minimum men for selection */
#define SZ_KEY_BRIGHT_DELTA              "BRIGHTNESS_DELTA" /**< Brightness delta */
#define SZ_KEY_DECORATE_ORDERS           "DECORATE_ORDERS"  /**< Decorate orders flag */
#define SZ_KEY_LAYOUT                    "LAYOUT"           /**< Window layout */
#define SZ_KEY_CHECK_TEACH_LVL           "CHECK_TEACH_LEVEL" /**< Check teach level */
#define SZ_KEY_WATER_TERRAINS            "WATER_TERRAINS"   /**< Water terrains */
#define SZ_KEY_CHECK_OUTPUT_LIST         "CHECK_OUTPUT_LIST" /**< Check output list */
#define SZ_KEY_PWD_READ                  "READ_PWD_FROM_REP" /**< Read password from report */
#define SZ_KEY_CHK_PROD_REQ              "IMMEDIATE_PROD_CHECK" /**< Immediate production check */
#define SZ_KEY_VALID_ORDERS              "VALID_ORDERS"     /**< Valid orders */
#define SZ_KEY_DASH_BAD_ROADS            "DASH_BAD_ROADS"   /**< Dash bad roads */
#define SZ_KEY_CHECK_MOVE_MODE           "CHECK_MOVE_MODE"  /**< Check move mode */
#define SZ_KEY_CHECK_NEW_UNIT_FACTION    "CHECK_FACTION_X_NEW_Y" /**< Check new unit faction */

//---------------------------------------------------------------------------
// Configuration Keys - Colors
//---------------------------------------------------------------------------

#define SZ_KEY_COLOR_TEST                "Test colour"      /**< Test color */
#define SZ_KEY_COLOR_RED                 "COLOR_RED"        /**< Red color */
#define SZ_KEY_COLOR_BLACK               "COLOR_BLACK"      /**< Black color */
#define SZ_KEY_MAP_GRID                  "GRID_NORMAL"      /**< Normal grid color */
#define SZ_KEY_MAP_GRID_SEL              "GRID_SELECTED"    /**< Selected grid color */
#define SZ_KEY_MAP_TROPIC_LINE           "TROPIC_LINE"      /**< Tropic line color */
#define SZ_KEY_MAP_RING                  "DISTANCE_RING"    /**< Distance ring color */
#define SZ_KEY_MAP_COASTLINE             "COASTLINE"        /**< Coastline color */

//---------------------------------------------------------------------------
// Configuration Keys - Unit Display
//---------------------------------------------------------------------------

#define SZ_UNIT_MOVING_OUT               "UNIT_MOVING_OUT"  /**< Unit moving out color */
#define SZ_UNIT_GUARDING                 "UNIT_GUARDING"    /**< Unit guarding color */
#define SZ_UNIT_ARRIVING                 "UNIT_ARRIVING"    /**< Unit arriving color */
#define SZ_UNIT_NO_MONTHLONG             "UNIT_NO_MONTHLONG" /**< No month-long order color */
#define SZ_UNIT_MULTI_MONTHLONG          "UNIT_MULTI_MONTHLONG" /**< Multiple month-long orders color */
#define SZ_MONSTER_BG                    "MONSTER_BG"
#define SZ_MONSTER_TEXT                  "MONSTER_TEXT"
//---------------------------------------------------------------------------
// Configuration Keys - Roads
//---------------------------------------------------------------------------

#define SZ_KEY_MAP_ROAD_OLD              "MAP_ROADS"        /**< Old road color */
#define SZ_KEY_MAP_ROAD_BAD_OLD          "MAP_ROADS_BAD"    /**< Old bad road color */
#define SZ_KEY_MAP_ROAD                  "MAP_ROAD"         /**< Road color */
#define SZ_KEY_MAP_ROAD_BAD              "MAP_ROAD_BAD"     /**< Bad road color */

//---------------------------------------------------------------------------
// Configuration Keys - Unit Colors by Faction Relation
//---------------------------------------------------------------------------

#define SZ_KEY_MAP_TRUSTED               "TRUSTED_UNITS"    /**< Trusted units color */
#define SZ_KEY_MAP_PREFERRED             "PREFERRED_UNITS"  /**< Preferred units color */
#define SZ_KEY_MAP_TOLERATED             "TOLERATED_UNITS"  /**< Tolerated units color */
#define SZ_KEY_MAP_ENEMY                 "ENEMY_UNITS"      /**< Enemy units color */
#define SZ_KEY_MAP_PREFIX                "MAP_"             /**< Map color prefix */

//---------------------------------------------------------------------------
// Configuration Keys - Terrain Colors
//---------------------------------------------------------------------------

#define SZ_KEY_MAP_UNKNOWN               "UNKNOWN"          /**< Unknown terrain color */
#define SZ_KEY_MAP_DESERT                "DESERT"           /**< Desert color */
#define SZ_KEY_MAP_FOREST                "FOREST"           /**< Forest color */
#define SZ_KEY_MAP_JUNGLE                "JUNGLE"           /**< Jungle color */
#define SZ_KEY_MAP_MOUNTAIN              "MOUNTAIN"         /**< Mountain color */
#define SZ_KEY_MAP_OCEAN                 "OCEAN"            /**< Ocean color */
#define SZ_KEY_MAP_PLAINS                "PLAIN"            /**< Plains color */
#define SZ_KEY_MAP_SWAMP                 "SWAMP"            /**< Swamp color */
#define SZ_KEY_MAP_TUNDRA                "TUNDRA"           /**< Tundra color */
#define SZ_KEY_MAP_NEXUS                 "NEXUS"            /**< Nexus color */
#define SZ_KEY_MAP_CAVERN                "CAVERN"           /**< Cavern color */
#define SZ_KEY_MAP_TUNNELS               "TUNNELS"          /**< Tunnels color */
#define SZ_KEY_MAP_UNDERFOREST           "UNDERFOREST"      /**< Underforest color */

//---------------------------------------------------------------------------
// Additional Terrain Types
//---------------------------------------------------------------------------

#define SZ_KEY_MAP_CHASM                 "chasm"            /**< Chasm terrain */
#define SZ_KEY_MAP_DEEPFOREST            "deepforest"       /**< Deep forest */
#define SZ_KEY_MAP_GROTTO                "grotto"           /**< Grotto terrain */
#define SZ_KEY_MAP_LAKE                  "lake"             /**< Lake terrain */

//---------------------------------------------------------------------------
// Configuration Keys - Map Flags
//---------------------------------------------------------------------------

#define SZ_KEY_MAP_FLAG_1                "FLAG_1"           /**< Flag 1 color */
#define SZ_KEY_MAP_FLAG_2                "FLAG_2"           /**< Flag 2 color */
#define SZ_KEY_MAP_FLAG_3                "FLAG_3"           /**< Flag 3 color */

//---------------------------------------------------------------------------
// Configuration Keys - Spy Detection
//---------------------------------------------------------------------------

#define SZ_KEY_SPY_DETECT_LO             "SPY_DETECT_LOW"   /**< Spy detection low bound */
#define SZ_KEY_SPY_DETECT_HI             "SPY_DETECT_HIGH"  /**< Spy detection high bound */
#define SZ_KEY_SPY_DETECT_AMT            "SPY_DETECT_AMOUNT" /**< Spy detection amount */
#define SZ_KEY_SPY_DETECT_WARNING        "SPY_DETECT_WARNING" /**< Spy detection warning */

//---------------------------------------------------------------------------
// Configuration Keys - Map Display
//---------------------------------------------------------------------------

#define SZ_KEY_HEX_SIZE_LIST             "HEX_SIZE_LIST"    /**< Hex size in lists */
#define SZ_KEY_SHOW_TRACKS               "SHOW_TRACKS"      /**< Show tracks flag */
#define SZ_KEY_WALL_WIDTH                "WALL_WIDTH"       /**< Wall width */
#define SZ_KEY_FLAG_WIDTH                "FLAG_WIDTH"       /**< Flag width */
#define SZ_KEY_ROAD_WIDTH                "ROAD_WIDTH"       /**< Road width */

//---------------------------------------------------------------------------
// Configuration Keys - Filters
//---------------------------------------------------------------------------

#define SZ_KEY_UNIT_FLTR_PROPERTY        "UNIT_FLTR_PROPERTY_" /**< Unit filter property */
#define SZ_KEY_UNIT_FLTR_COMPARE         "UNIT_FLTR_COMPARE_"  /**< Unit filter compare */
#define SZ_KEY_UNIT_FLTR_VALUE           "UNIT_FLTR_VALUE_"    /**< Unit filter value */
#define SZ_KEY_UNIT_FLTR_TRACKING        "UNIT_FLTR_TRACKING" /**< Unit filter tracking */
#define SZ_KEY_UNIT_FLTR_PYTHON_CODE     "PYTHON_CODE"       /**< Unit filter Python code */
#define SZ_KEY_UNIT_FLTR_SOURCE          "SOURCE"            /**< Unit filter source */
#define SZ_KEY_UNIT_FLTR_SOURCE_PYTHON   "Python"            /**< Python source identifier */
#define SZ_KEY_UNIT_FLTR_SELECTED_HEXES  "SELECTED_HEXES"    /**< Selected hexes filter */
#define SZ_KEY_UNIT_FLTR_SHOW_ON_MAP     "SHOW_ON_MAP"       /**< Show on map filter */

#define SZ_KEY_HEX_FLTR_PROPERTY        "HEX_FLTR_PROPERTY_" /**< Hex filter property */
#define SZ_KEY_HEX_FLTR_COMPARE         "HEX_FLTR_COMPARE_"  /**< Hex filter compare */
#define SZ_KEY_HEX_FLTR_VALUE           "HEX_FLTR_VALUE_"    /**< Hex filter value */
#define SZ_KEY_HEX_FLTR_PYTHON_CODE     "PYTHON_CODE"        /**< Hex filter Python code */
#define SZ_KEY_HEX_FLTR_SOURCE          "SOURCE"             /**< Hex filter source */
#define SZ_KEY_HEX_FLTR_SOURCE_PYTHON   "Python"             /**< Python source identifier */

//---------------------------------------------------------------------------
// Configuration Keys - Movement and Orders
//---------------------------------------------------------------------------

#define SZ_KEY_MOVEMENTS                 "MOVEMENT_MODES"    /**< Movement modes */
#define SZ_KEY_ORD_MONTH_LONG            "MONTH_LONG_ORDERS" /**< Month-long orders */
#define SZ_KEY_ORD_DUPLICATABLE          "MONTH_LONG_DUP_ORD" /**< Duplicatable month-long orders */

//---------------------------------------------------------------------------
// Configuration Keys - CSV Export
//---------------------------------------------------------------------------

#define SZ_KEY_SEPARATOR                 "SEPARATOR"         /**< CSV separator */
#define SZ_KEY_ORIENTATION               "ORIENTATION"       /**< CSV orientation */
#define SZ_KEY_FORMAT                    "FORMAT"            /**< CSV format */

//---------------------------------------------------------------------------
// Configuration Keys - List Columns
//---------------------------------------------------------------------------

#define SZ_KEY_LIS_COL_UNITS_HEX         "UNITS_HEX"         /**< Units in hex columns */
#define SZ_KEY_LIS_COL_UNITS_FILTER      "UNITS_FILTER"      /**< Filtered units columns */

//---------------------------------------------------------------------------
// Configuration Keys - Filter Sets
//---------------------------------------------------------------------------

#define SZ_KEY_FLTR_SET                  "FILTER_SET"        /**< Filter set identifier */

//---------------------------------------------------------------------------
// Configuration Keys - Economy
//---------------------------------------------------------------------------

#define SZ_KEY_LEAD_SKILL_BONUS          "LEAD_SKILL_BONUS"  /**< Leadership skill bonus */
#define SZ_KEY_TAX_PER_TAXER             "TAX_PER_TAXER"     /**< Tax per taxer */
#define SZ_KEY_ENTERTAINMENT_SILVER      "ENTERTAINMENT_SILVER" /**< Entertainment silver */

//---------------------------------------------------------------------------
// Configuration Keys - Display Options
//---------------------------------------------------------------------------

#define SZ_KEY_ICONS                     "ICONS"             /**< Icons set */
#define SZ_KEY_BATTLE_STATISTICS         "BATTLE_STATISTICS" /**< Battle statistics display */
#define SZ_KEY_SHOW_STEALS               "SHOW_STEALS_EVENT" /**< Show steals event */

//---------------------------------------------------------------------------
// Configuration Keys - Wagons
//---------------------------------------------------------------------------

#define SZ_KEY_WAGONS                    "WAGONS"            /**< Wagons enabled */
#define SZ_KEY_WAGON_PULLERS             "WAGON_PULLERS"     /**< Wagon pullers */
#define SZ_KEY_WAGON_CAPACITY            "WAGON_CAPACITY"    /**< Wagon capacity */

//---------------------------------------------------------------------------
// Configuration Keys - Upkeep
//---------------------------------------------------------------------------

#define SZ_UPKEEP_LEADER                 "UPKEEP_LEADER"     /**< Leader upkeep cost */
#define SZ_UPKEEP_PEASANT                "UPKEEP_PEASANT"    /**< Peasant upkeep cost */

//---------------------------------------------------------------------------
// Orientation Values
//---------------------------------------------------------------------------

#define SZ_VERTICAL                      "Vertical"          /**< Vertical orientation */
#define SZ_HORIZONTAL                    "Horizontal"        /**< Horizontal orientation */

//---------------------------------------------------------------------------
// Structure Attributes
//---------------------------------------------------------------------------

#define SZ_ATTR_STRUCT_HIDDEN            "HIDDEN"            /**< Hidden structure */
#define SZ_ATTR_STRUCT_MOBILE            "MOBILE"            /**< Mobile structure */
#define SZ_ATTR_STRUCT_SHAFT             "SHAFT"             /**< Shaft structure */
#define SZ_ATTR_STRUCT_GATE              "GATE"              /**< Gate structure */
#define SZ_ATTR_STRUCT_ROAD_N            "ROAD_N"            /**< Road north */
#define SZ_ATTR_STRUCT_ROAD_NE           "ROAD_NE"           /**< Road northeast */
#define SZ_ATTR_STRUCT_ROAD_SE           "ROAD_SE"           /**< Road southeast */
#define SZ_ATTR_STRUCT_ROAD_S            "ROAD_S"            /**< Road south */
#define SZ_ATTR_STRUCT_ROAD_SW           "ROAD_SW"           /**< Road southwest */
#define SZ_ATTR_STRUCT_ROAD_NW           "ROAD_NW"           /**< Road northwest */
#define SZ_ATTR_STRUCT_MAX_LOAD          "MAX_LOAD"          /**< Maximum load capacity */
#define SZ_ATTR_STRUCT_MIN_SAIL          "MIN_POWER"         /**< Minimum sailing power */

//---------------------------------------------------------------------------
// Unit Types
//---------------------------------------------------------------------------

#define SZ_LEADER                        "leader"            /**< Leader unit type */
#define SZ_HERO                          "hero"              /**< Hero unit type */

//---------------------------------------------------------------------------
// Attitude Configuration
//---------------------------------------------------------------------------

#define SZ_ATT_PLAYER_ID                 "PLAYER_FACTION_ID" /**< Player faction ID */
#define SZ_ATT_APPLY_ON_JOIN             "APPLY_ATTITUDES_ON_JOIN" /**< Apply attitudes on join */
#define SZ_ATT_FRIEND1                   "TRUSTED_ATTITUDES" /**< Trusted attitudes */
#define SZ_ATT_FRIEND2                   "PREFERRED_ATTITUDES" /**< Preferred attitudes */
#define SZ_ATT_NEUTRAL                   "TOLERATED_ATTITUDES" /**< Tolerated attitudes */
#define SZ_ATT_ENEMY                     "ENEMY_ATTITUDES"   /**< Enemy attitudes */

//---------------------------------------------------------------------------
// Default Values
//---------------------------------------------------------------------------

#define SZ_DEFAULT_FONT_PROP             "9,77,90,90,1,arial" /**< Default proportional font */
#define SZ_DEFAULT_FONT_MONO             "10,77,90,90,1,courier new" /**< Default monospaced font */
#define SZ_DEFAULT_MOVEMENT_MODE         "None,Walk,Ride,Fly,Swim" /**< Default movement modes */

//---------------------------------------------------------------------------
// Icon Sets
//---------------------------------------------------------------------------

#define SZ_ICONS_ADVANCED                "ADVANCED"          /**< Advanced icons */
#define SZ_ICONS_SIMPLE                  "SIMPLE"            /**< Simple icons */

//---------------------------------------------------------------------------
// Special Values
//---------------------------------------------------------------------------

#define SZ_MANUAL_HEX_PROVINCE           "_Unknown_Temp_"    /**< Temporary unknown province marker */

#endif