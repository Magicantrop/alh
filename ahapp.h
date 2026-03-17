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

#ifndef __AH_APP_INCL__
#define __AH_APP_INCL__

#include <wx/accel.h>
#include <wx/app.h>

#include "atlaparser.h"
#include "cfgfile.h"
#include "collection.h"
#include "cstr.h"

/**
 * @enum FrameType
 * @brief Identifiers for different frame windows in the application
 */
enum
{
    AH_FRAME_MAP          =  0,   /**< Map display frame */
    AH_FRAME_UNITS            ,   /**< Units list frame */
    AH_FRAME_MSG              ,   /**< Messages frame */
    AH_FRAME_EDITS            ,   /**< Edits/orders frame */
    AH_FRAME_UNITS_FLTR       ,   /**< Filtered units frame */
    AH_FRAME_SHAFTS           ,   /**< Shafts/connections frame */

    AH_FRAME_COUNT
};

/**
 * @enum PaneType
 * @brief Identifiers for different panes within frames
 * @note When adding new panes, update CAhApp::ApplyFonts and CAhApp::ApplyColors!
 */
enum
{
    AH_PANE_MAP            = 0,   /**< Main map pane */
    AH_PANE_MAP_DESCR         ,   /**< Map description pane */
    AH_PANE_UNITS_HEX         ,   /**< Units in current hex pane */
    AH_PANE_UNITS_FILTER      ,   /**< Units filter pane */
    AH_PANE_UNIT_DESCR        ,   /**< Unit description pane */
    AH_PANE_UNIT_COMMANDS     ,   /**< Unit commands pane */
    AH_PANE_UNIT_COMMENTS     ,   /**< Unit comments pane */
    AH_PANE_MSG               ,   /**< Messages pane */

    AH_PANE_COUNT
};

/**
 * @enum LayoutType
 * @brief Available window layout configurations
 */
enum
{
    AH_LAYOUT_2_WIN        = 0,   /**< Two-window layout */
    AH_LAYOUT_3_WIN           ,   /**< Three-window layout */
    AH_LAYOUT_1_WIN           ,   /**< Single-window layout */
    AH_LAYOUT_1_WIN_WIDE      ,   /**< Wide single-window layout */

    AH_LAYOUT_COUNT
};

/**
 * @enum FontType
 * @brief Font identifiers used throughout the application
 */
enum
{
    FONT_EDIT_DESCR        = 0,   /**< Font for edit descriptions */
    FONT_EDIT_ORDER           ,   /**< Font for order text */
    FONT_MAP_COORD            ,   /**< Font for map coordinates */
    FONT_MAP_TEXT             ,   /**< Font for map text labels */
    FONT_UNIT_LIST            ,   /**< Font for unit lists */
    FONT_EDIT_HDR             ,   /**< Font for edit headers */
    FONT_VIEW_DLG             ,   /**< Font for view dialogs */
    FONT_ERR_DLG              ,   /**< Font for error dialogs */

    FONT_COUNT
};

/**
 * @enum ConfigFileType
 * @brief Configuration file identifiers
 */
enum
{
    CONFIG_FILE_CONFIG     = 0,   /**< Main configuration file */
    CONFIG_FILE_STATE         ,   /**< State/UI configuration file */

    CONFIG_FILE_COUNT
};

// Forward declarations
class CAhFrame;
class CEditPane;
class CHexFilterDlg;

/**
 * @enum eRepSeq
 * @brief Report navigation sequence identifiers
 */
enum eRepSeq  {repFirst, repPrev, repNext, repLast, repLastVisited};

/**
 * @enum eHexIncl
 * @brief Hex inclusion modes for export operations
 */
enum eHexIncl {HexNew,               /**< Only new hexes */
               HexCurrent,           /**< Current hex only */
               HexSelected,          /**< Hexes selected by rectangle drag */
               HexAll,               /**< All hexes */
               };

// Utility function declarations
void FontToStr(const wxFont * font, CStr & s);
wxFont * NewFontFromStr(const char * p);

void StrToColor(wxColour * cr, const char * p);
void ColorToStr(char * p, wxColour * cr);
void MakePathRelative(const char * cur_dir, CStr & path);
void MakePathFull(const char * cur_dir, CStr & path);
void GetDirFromPath(const char * path, CStr & dir);
void GetFileFromPath(const char * path, CStr & file);

/**
 * @def APPLY_COLOR_DELTA
 * @brief Macro to apply brightness delta to color components
 */
#define APPLY_COLOR_DELTA(x) ((unsigned char )(std::max(std::min((int)(x-gpApp->m_Brightness_Delta),255),0)))

//-------------------------------------------------------------------------------

/**
 * @class CAtlaSortColl
 * @brief Sorted collection for Atlantis parser objects, sorted by year/month
 */
class CAtlaSortColl : public CSortedCollection
{
public:
    CAtlaSortColl()           : CSortedCollection() {};
    CAtlaSortColl(int nDelta) : CSortedCollection(nDelta) {};
    
protected:
    /**
     * @brief Frees a CAtlaParser item from the collection
     * @param pItem Pointer to the item to free
     */
    virtual void FreeItem(void * pItem)
    {
        if (pItem)
            delete (CAtlaParser*)pItem;
    };
    
    /**
     * @brief Compares two CAtlaParser items by year/month
     * @param pItem1 First item to compare
     * @param pItem2 Second item to compare
     * @return 1 if pItem1 > pItem2, -1 if pItem1 < pItem2, 0 if equal
     */
    virtual int Compare(void * pItem1, void * pItem2) const
    {
        if ( ((CAtlaParser*)pItem1)->m_YearMon > ((CAtlaParser*)pItem2)->m_YearMon)
            return 1;
        else
            if ( ((CAtlaParser*)pItem1)->m_YearMon < ((CAtlaParser*)pItem2)->m_YearMon)
                return -1;
            else
                return 0;
    };
};

//-------------------------------------------------------------------------------

/**
 * @struct ItemWeights
 * @brief Structure to hold item movement weights for different transport modes
 */
struct ItemWeights
{
    char * name;      /**< Item name */
    int  * weights;   /**< Weight values for different movement modes */
};

/**
 * @class CWeightsColl
 * @brief Sorted collection for item weights, sorted by item name
 */
class CWeightsColl : public CSortedCollection
{
public:
    CWeightsColl()           : CSortedCollection() {};
    CWeightsColl(int nDelta) : CSortedCollection(nDelta) {};
    
protected:
    /**
     * @brief Frees an ItemWeights structure from the collection
     * @param pItem Pointer to the ItemWeights to free
     */
    virtual void FreeItem(void * pItem)
    {
        if (pItem)
        {
            if (((ItemWeights*)pItem)->name)
                free(((ItemWeights*)pItem)->name);
            if (((ItemWeights*)pItem)->weights)
                free(((ItemWeights*)pItem)->weights);
            delete (ItemWeights*)pItem;
        }
    };
    
    /**
     * @brief Compares two ItemWeights structures by item name
     * @param pItem1 First item to compare
     * @param pItem2 Second item to compare
     * @return Comparison result from SafeCmp
     */
    virtual int Compare(void * pItem1, void * pItem2) const
    {
        return SafeCmp(((ItemWeights*)pItem1)->name, ((ItemWeights*)pItem2)->name);
    };
};

// Configuration line encoding/decoding utilities
void EncodeConfigLine(CStr& dest, const char* src);
void DecodeConfigLine(CStr& dest, const char* src);

//-------------------------------------------------------------------------------

/**
 * @class CAhApp
 * @brief Main application class for Atlantis Little Helper
 * 
 * This class manages the entire application lifecycle, including window management,
 * data loading/saving, configuration handling, and user interface coordination.
 * It inherits from wxApp and serves as the central coordination point for all
 * application components.
 */
class CAhApp : public wxApp
{
public:
    CAhApp();
    ~CAhApp();

    // wxApp overrides
    virtual bool OnInit();
    virtual int OnExit();

    //--------------------------------------------------------------------------
    // Core UI Management
    //--------------------------------------------------------------------------
    
    /**
     * @brief Redraws all application windows
     */
    void Redraw();
    
    /**
     * @brief Handles frame closing events
     * @param pFrame The frame being closed
     */
    void FrameClosing(CAhFrame* pFrame);
    
    /**
     * @brief Removes a frame from internal tracking
     * @param no Frame identifier
     * @param frameclosed Whether the frame is actually closed
     */
    void ForgetFrame(int no, BOOL frameclosed);
    
    /**
     * @brief Updates the map frame title with current information
     */
    void SetMapFrameTitle();
    
    /**
     * @brief Temporarily selects a unit (without persisting the selection)
     * @param pUnit Unit to select temporarily
     */
    void SelectTempUnit(CUnit* pUnit);
    
    /**
     * @brief Permanently selects a unit
     * @param pUnit Unit to select
     */
    void SelectUnit(CUnit* pUnit);
    
    /**
     * @brief Selects a land hex
     * @param pLand Land to select
     */
    void SelectLand(CLand* pLand);
    
    /**
     * @brief Selects a land hex by coordinates
     * @param landcoords Coordinate string (e.g., "(x,y)")
     * @return TRUE if land was found and selected
     */
    BOOL SelectLand(const char* landcoords);
    
    /**
     * @brief Called when map selection changes
     */
    void OnMapSelectionChange();
    
    /**
     * @brief Called when unit hex selection changes
     * @param idx Index of selected unit
     */
    void OnUnitHexSelectionChange(long idx);
    
    /**
     * @brief Called when edit pane content changes
     * @param pPane The edit pane that changed
     */
    void EditPaneChanged(CEditPane* pPane);
    
    /**
     * @brief Called when edit pane is double-clicked
     * @param pPane The edit pane that was double-clicked
     */
    void EditPaneDClicked(CEditPane* pPane);
    
    /**
     * @brief Updates the unit description pane with current unit data
     * @param pUnit Unit to display
     */
    void UpdateUnitDescriptionPane(CUnit *pUnit);

    //--------------------------------------------------------------------------
    // Configuration Management
    //--------------------------------------------------------------------------
    
    /**
     * @brief Sets a string configuration value
     * @param szSection Configuration section
     * @param szName Parameter name
     * @param szNewValue New value
     */
    void SetConfig(const char* szSection, const char* szName, const char* szNewValue);
    
    /**
     * @brief Sets a numeric configuration value
     * @param szSection Configuration section
     * @param szName Parameter name
     * @param lNewValue New value
     */
    void SetConfig(const char* szSection, const char* szName, long lNewValue);
    
    /**
     * @brief Gets a configuration value as string
     * @param szSection Configuration section
     * @param szName Parameter name
     * @return Configuration value or NULL if not found
     */
    const char* GetConfig(const char* szSection, const char* szName);
    
    /**
     * @brief Gets the first entry in a configuration section
     * @param szSection Section name
     * @param szName Output parameter name
     * @param szValue Output parameter value
     * @return Index for subsequent calls to GetSectionNext, or -1 if none
     */
    int GetSectionFirst(const char* szSection, const char*& szName, const char*& szValue);
    
    /**
     * @brief Gets the next entry in a configuration section
     * @param idx Previous index from GetSectionFirst/GetSectionNext
     * @param szSection Section name
     * @param szName Output parameter name
     * @param szValue Output parameter value
     * @return Next index or -1 if no more
     */
    int GetSectionNext(int idx, const char* szSection, const char*& szName, const char*& szValue);
    
    /**
     * @brief Removes an entire configuration section
     * @param szSection Section to remove
     */
    void RemoveSection(const char* szSection);
    
    /**
     * @brief Gets the name of the next section in configuration file
     * @param fileno Configuration file index
     * @param szStart Starting section name (NULL for first)
     * @return Next section name or NULL
     */
    const char* GetNextSectionName(int fileno, const char* szStart);
    
    /**
     * @brief Moves all entries from source to destination section
     * @param fileno Configuration file index
     * @param src Source section name
     * @param dest Destination section name
     */
    void MoveSectionEntries(int fileno, const char* src, const char* dest);
    
    /**
     * @brief Upgrades configuration files to current format
     */
    void UpgradeConfigFiles();
    
    /**
     * @brief Upgrades faction ID configuration
     */
    void UpgradeConfigByFactionId();
    
    /**
     * @brief Composes orders section name for a faction
     * @param Sect Output string for section name
     * @param FactionId Faction ID
     */
    void ComposeConfigOrdersSection(CStr& Sect, int FactionId);
    
    /**
     * @brief Determines which configuration file a section belongs to
     * @param szSection Section name
     * @return Configuration file index
     */
    int GetConfigFileNo(const char* szSection);

    //--------------------------------------------------------------------------
    // Window Management
    //--------------------------------------------------------------------------
    
    /**
     * @brief Opens or shows the map frame
     */
    void OpenMapFrame();
    
    /**
     * @brief Opens or shows the unit frame
     */
    void OpenUnitFrame();
    
    /**
     * @brief Opens or shows the messages frame
     */
    void OpenMsgFrame();
    
    /**
     * @brief Opens or shows the edits frame
     */
    void OpenEditsFrame();
    
    /**
     * @brief Opens the filtered unit frame
     * @param PopUpSettings Whether to show settings dialog
     */
    void OpenUnitFrameFltr(BOOL PopUpSettings);
    
    /**
     * @brief Shows the shaft connection GUI
     */
    void ShowShaftConnectGUI();
    
    /**
     * @brief Applies current font settings to all windows
     */
    void ApplyFonts();
    
    /**
     * @brief Applies current color settings to all windows
     */
    void ApplyColors();
    
    /**
     * @brief Applies current icon settings
     */
    void ApplyIcons();
    
    /**
     * @brief Opens the options dialog
     */
    void OpenOptionsDlg();
    
    /**
     * @brief Creates and sets the accelerator table
     */
    void CreateAccelerator();

    //--------------------------------------------------------------------------
    // Game Data Access
    //--------------------------------------------------------------------------
    
    /**
     * @brief Resolves an alias to its actual value
     * @param alias Alias to resolve
     * @return Resolved value or NULL
     */
    const char* ResolveAlias(const char* alias);
    
    /**
     * @brief Gets the study cost for a skill
     * @param skill Skill name
     * @return Study cost in silver
     */
    long GetStudyCost(const char* skill);
    
    /**
     * @brief Gets structure attributes
     * @param kind Structure kind
     * @param MaxLoad Output maximum load capacity
     * @param MinSailingPower Output minimum sailing power
     * @return attr
     */
    
    long GetStructAttr(const char* kind, long& MaxLoad, long& MinSailingPower);

    /**
     * @brief Gets movement weights for an item
     * @param item Item name
     * @param weights Output weight array
     * @param movenames Output movement mode names
     * @param movecount Output number of movement modes
     * @return TRUE if item found
     */
    BOOL GetItemWeights(const char* item, int*& weights, const char**& movenames, int& movecount);
    
    /**
     * @brief Gets available movement mode names
     * @param movenames Output array of movement mode names
     */
    void GetMoveNames(const char**& movenames);
    
    /**
     * @brief Gets order ID by order name
     * @param order Order name
     * @param id Output order ID
     * @return TRUE if order found
     */
    BOOL GetOrderId(const char* order, long& id);
    
    /**
     * @brief Checks if item is a trade item
     * @param item Item name
     * @return TRUE if trade item
     */
    BOOL IsTradeItem(const char* item);
    
    /**
     * @brief Checks if item represents men/soldiers
     * @param item Item name
     * @return TRUE if item is men
     */
    BOOL IsMan(const char* item);
    
    /**
     * @brief Checks if skill is a magic skill
     * @param skill Skill name
     * @return TRUE if magic skill
     */
    BOOL IsMagicSkill(const char* skill);
    
    /**
     * @brief Gets weather description line
     * @param IsCurrent Whether current weather
     * @param IsGood Whether weather is good
     * @param Zone Weather zone
     * @return Weather description string
     */
    const char* GetWeatherLine(BOOL IsCurrent, BOOL IsGood, int Zone);
    
    /**
     * @brief Gets production details for an item
     * @param item Item name
     * @param details Output production details structure
     */
    void GetProdDetails(const char* item, TProdDetails& details);
    
    /**
     * @brief Gets maximum skill level for a race
     * @param race Race name
     * @param skill Skill name
     * @param leadership Leadership skill level
     * @param IsArcadiaSkillSystem Whether using Arcadia skill system
     * @return Maximum achievable skill level
     */
    long GetMaxRaceSkillLevel(const char* race, const char* skill, const char* leadership, BOOL IsArcadiaSkillSystem);
    
    /**
     * @brief Checks if advanced resources can be seen
     * @param skillname Skill name
     * @param terrain Terrain type
     * @param Levels Output skill levels
     * @param Resources Output resource list
     * @return TRUE if resources visible
     */
    BOOL CanSeeAdvResources(const char* skillname, const char* terrain, CLongColl& Levels, CBufColl& Resources);
    
    /**
     * @brief Gets attitude for a faction
     * @param id Faction ID
     * @return Attitude value
     */
    int GetAttitudeForFaction(int id);
    
    /**
     * @brief Sets attitude for a faction
     * @param id Faction ID
     * @param attitude Attitude value
     */
    void SetAttitudeForFaction(int id, int attitude);
    
    /**
     * @brief Gets short faction name
     * @param S Output string for short name
     * @param FactionId Faction ID
     */
    void GetShortFactName(CStr& S, int FactionId);

    //--------------------------------------------------------------------------
    // Orders Management
    //--------------------------------------------------------------------------
    
    /**
     * @brief Loads orders from default location
     */
    void LoadOrders();
    
    /**
     * @brief Loads orders from specified file
     * @param FNameIn Input filename
     * @return 0 on success, error code on failure
     */
    int LoadOrders(const char* FNameIn);
    
    /**
     * @brief Saves orders using existing filename
     * @param UsingExistingName Whether to use existing name
     * @return 0 on success, error code on failure
     */
    int SaveOrders(BOOL UsingExistingName);
    
    /**
     * @brief Saves orders to specified file for a faction
     * @param FNameOut Output filename
     * @param FactionId Faction ID
     * @return 0 on success, error code on failure
     */
    int SaveOrders(const char* FNameOut, int FactionId);
    
    /**
     * @brief Sets orders changed flag
     * @param Changed New changed state
     */
    void SetOrdersChanged(BOOL Changed);
    
    /**
     * @brief Gets orders changed flag
     * @return TRUE if orders have unsaved changes
     */
    BOOL GetOrdersChanged();
    
    /**
     * @brief Re-runs orders through parser
     */
    void RerunOrders();
    
    /**
     * @brief Checks for month-long orders
     */
    void CheckMonthLongOrders();

    //--------------------------------------------------------------------------
    // Report Management
    //--------------------------------------------------------------------------
    
    /**
     * @brief Loads a report
     * @param Join Whether to join with current data
     * @return 0 on success, error code on failure
     */
    int LoadReport(BOOL Join);
    
    /**
     * @brief Loads report from specified file
     * @param FNameIn Input filename
     * @param Join Whether to join with current data
     * @return 0 on success, error code on failure
     */
    int LoadReport(const char* FNameIn, BOOL Join);
    
    /**
     * @brief Pre-load report preparation
     */
    void PreLoadReport();
    
    /**
     * @brief Post-load report processing
     */
    void PostLoadReport();
    
    /**
     * @brief Switches to a different report
     * @param whichrep Which report to switch to (first/prev/next/last/last visited)
     */
    void SwitchToRep(eRepSeq whichrep);
    
    /**
     * @brief Switches to report for specific year/month
     * @param YearMon Year/month value (year*100+month)
     */
    void SwitchToYearMon(long YearMon);
    
    /**
     * @brief Checks if switch to report is possible
     * @param whichrep Which report to check
     * @param RepIdx Output report index if available
     * @return TRUE if switch is possible
     */
    BOOL CanSwitchToRep(eRepSeq whichrep, int& RepIdx);
    
    /**
     * @brief Gets previous turn's report parser
     * @param pPrevTurn Output pointer to previous turn parser
     * @return TRUE if previous turn exists
     */
    BOOL GetPrevTurnReport(CAtlaParser*& pPrevTurn);
    
    /**
     * @brief Checks tax and trade for current selection
     */
    void CheckTaxTrade();
    
    /**
     * @brief Checks production for current selection
     */
    void CheckProduction();
    
    /**
     * @brief Checks sailing for current selection
     */
    void CheckSailing();
    
    /**
     * @brief Checks tax details for a land hex
     * @param pLand Land to check
     * @param TaxDetails Output tax details collection
     */
    void CheckTaxDetails(CLand* pLand, CTaxProdDetailsCollByFaction& TaxDetails);
    
    /**
     * @brief Checks trade details for a land hex
     * @param pLand Land to check
     * @param TradeDetails Output trade details collection
     */
    void CheckTradeDetails(CLand* pLand, CTaxProdDetailsCollByFaction& TradeDetails);

    //--------------------------------------------------------------------------
    // View Functions
    //--------------------------------------------------------------------------
    
    /**
     * @brief Shows a description list dialog
     * @param Items Collection of items to display
     * @param title Dialog title
     */
    void ShowDescriptionList(CCollection& Items, const char* title);
    
    /**
     * @brief Views short-named objects (factions, cities, etc.)
     * @param ViewAll Whether to view all or filtered
     * @param szSection Configuration section
     * @param szHeader Dialog header
     * @param ListNew List of new items
     */
    void ViewShortNamedObjects(BOOL ViewAll, const char* szSection, const char* szHeader, CBaseColl& ListNew);
    
    /**
     * @brief Views all battles
     */
    void ViewBattlesAll();
    
    /**
     * @brief Views events
     * @param DoEvents Whether to show events
     */
    void ViewEvents(BOOL DoEvents);
    
    /**
     * @brief Views security events
     */
    void ViewSecurityEvents();
    
    /**
     * @brief Views new products
     */
    void ViewNewProducts();
    
    /**
     * @brief Views gates
     */
    void ViewGates();
    
    /**
     * @brief Views cities
     */
    void ViewCities();
    
    /**
     * @brief Views provinces
     */
    void ViewProvinces();
    
    /**
     * @brief Views faction information
     */
    void ViewFactionInfo();
    
    /**
     * @brief Views faction overview
     */
    void ViewFactionOverview();
    
    /**
     * @brief Increments a value in faction overview
     * @param FactionId Faction ID
     * @param factionname Faction name
     * @param Factions Faction collection
     * @param propname Property name to increment
     * @param value Value to increment by
     */
    void ViewFactionOverview_IncrementValue(long FactionId, const char* factionname, CBaseCollById& Factions, const char* propname, long value);
    
    /**
     * @brief Writes mages information to CSV file
     */
    void WriteMagesCSV();

    //--------------------------------------------------------------------------
    // Hex Management
    //--------------------------------------------------------------------------
    
    /**
     * @brief Updates hex edit pane with land information
     * @param pLand Land to display
     */
    void UpdateHexEditPane(CLand* pLand);
    
    /**
     * @brief Updates hex unit list with units in land
     * @param pLand Land containing units
     */
    void UpdateHexUnitList(CLand* pLand);
    
    /**
     * @brief Updates edge structures for map display
     */
    void UpdateEdgeStructs();
    
    /**
     * @brief Gets units moving into a hex
     * @param HexId Target hex ID
     * @param FoundUnits Output collection of found units
     */
    void GetUnitsMovingIntoHex(long HexId, CBaseColl& FoundUnits) const;
    
    /**
     * @brief Shows units moving into current hex
     * @param CurHexId Current hex ID
     * @param pCurPlane Current plane
     */
    void ShowUnitsMovingIntoHex(long CurHexId, CPlane* pCurPlane);
    
    /**
     * @brief Shows financial information for a land
     * @param pCurLand Land to analyze
     */
    void ShowLandFinancial(CLand* pCurLand);
    
    /**
     * @brief Adds a temporary hex to the map
     * @param X X coordinate
     * @param Y Y coordinate
     * @param Plane Plane number
     */
    void AddTempHex(int X, int Y, int Plane);
    
    /**
     * @brief Removes a temporary hex from the map
     * @param X X coordinate
     * @param Y Y coordinate
     * @param Plane Plane number
     */
    void DelTempHex(int X, int Y, int Plane);
    
    /**
     * @brief Exports hexes to file
     * @param pCustomSelectedHexes Optional custom selection of hexes
     */
    void ExportHexes(CLongColl* pCustomSelectedHexes);
    
    /**
     * @brief Gets export options from user
     * @param FName Output filename
     * @param FMode Output file mode
     * @param options Output export options
     * @param HexIncl Output hex inclusion mode
     * @param InclTurnNoAcl Output whether to include turn numbers without ACL
     * @return TRUE if user confirmed export
     */
    BOOL GetExportHexOptions(CStr& FName, CStr& FMode, SAVE_HEX_OPTIONS& options, eHexIncl& HexIncl, bool& InclTurnNoAcl);
    
    /**
     * @brief Exports a single hex to file
     * @param Dest Destination file writer
     * @param pPlane Plane containing the hex
     * @param pLand Land to export
     * @param options Export options
     * @param InclTurnNoAcl Whether to include turn numbers without ACL
     * @param OnlyNew Whether to export only new hexes
     */
    void ExportOneHex(CFileWriter& Dest, CPlane* pPlane, CLand* pLand, SAVE_HEX_OPTIONS& options, bool InclTurnNoAcl, bool OnlyNew);
    
    /**
     * @brief Finds and displays trade routes
     */
    void FindTradeRoutes();

    //--------------------------------------------------------------------------
    // Flags Management
    //--------------------------------------------------------------------------
    
    /**
     * @brief Loads comments from file
     */
    void LoadComments();
    
    /**
     * @brief Saves comments to file
     */
    void SaveComments();
    
    /**
     * @brief Loads land flags from file
     */
    void LoadLandFlags();
    
    /**
     * @brief Saves land flags to file
     */
    void SaveLandFlags();
    
    /**
     * @brief Loads unit flags from file
     */
    void LoadUnitFlags();
    
    /**
     * @brief Saves unit flags to file
     */
    void SaveUnitFlags();
    
    /**
     * @brief Sets all land unit flags based on current settings
     */
    void SetAllLandUnitFlags();

    //--------------------------------------------------------------------------
    // Miscellaneous Functions
    //--------------------------------------------------------------------------
    
    /**
     * @brief Shows an error message
     * @param msg Error message
     * @param msglen Message length
     * @param ignore_disabled Whether to ignore if errors are disabled
     */
    void ShowError(const char* msg, int msglen, BOOL ignore_disabled);
    
    /**
     * @brief Gets currently selected unit
     * @return Pointer to selected unit or NULL
     */
    CUnit* GetSelectedUnit();
    
    /**
     * @brief Redraws movement tracks on map
     */
    void RedrawTracks();
    
    /**
     * @brief Checks if application can close
     * @return TRUE if safe to close
     */
    BOOL CanCloseApp();
    
    /**
     * @brief Saves command history to file
     * @param FNameOut Output filename
     * @return 0 on success, error code on failure
     */
    int SaveHistory(const char* FNameOut);
    
    /**
     * @brief Initializes standard output redirection
     */
    void StdRedirectInit();
    
    /**
     * @brief Cleans up standard output redirection
     */
    void StdRedirectDone();
    
    /**
     * @brief Reads more data from redirected output
     * @param FromStdout Whether to read from stdout (TRUE) or stderr (FALSE)
     * @param sData Output string with read data
     */
    void StdRedirectReadMore(BOOL FromStdout, CStr& sData);
    
    /**
     * @brief Checks redirected output files for new data
     */
    void CheckRedirectedOutputFiles();
    
    /**
     * @brief Initializes movement modes from configuration
     */
    void InitMoveModes();
    
    /**
     * @brief Initializes movement speed data
     */
    void InitMovementSpeed();
    
    /**
     * @brief Loads terrain movement cost configuration
     */
    void LoadTerrainCostConfig();
    
    /**
     * @brief Selects the next unit in current list
     */
    void SelectNextUnit();
    
    /**
     * @brief Selects the previous unit in current list
     */
    void SelectPrevUnit();
    
    /**
     * @brief Focuses on units pane
     */
    void SelectUnitsPane();
    
    /**
     * @brief Focuses on orders pane
     */
    void SelectOrdersPane();
    
    /**
     * @brief Shows units that have moved this turn
     */
    void ViewMovedUnits();
    
    /**
     * @brief Edits list columns for current view
     * @param command Edit command identifier
     */
    void EditListColumns(int command);
    
    /**
     * @brief Gets list column configuration section
     * @param sectprefix Section prefix
     * @param key Configuration key
     * @return Full section name
     */
    const char* GetListColSection(const char* sectprefix, const char* key);
    
    /**
     * @brief Gets alias by code
     * @param code Alias code
     * @return Alias string or NULL
     */
    const char* GetAliasByCode(const char* code);

    //--------------------------------------------------------------------------
    // Public Member Variables
    //--------------------------------------------------------------------------
    
    CAtlaParser* m_pAtlantis;                    /**< Current Atlantis parser instance */
    CAhFrame* m_Frames[AH_FRAME_COUNT];          /**< Application frames */
    wxWindow* m_Panes[AH_PANE_COUNT];            /**< Application panes */
    wxFont* m_Fonts[FONT_COUNT];                 /**< Application fonts */
    const char* m_FontDescr[FONT_COUNT];         /**< Font descriptions */
    CStrStrColl m_UnitPropertyGroups;            /**< Unit property groups for display */
    BOOL m_CommentsChanged;                       /**< Whether comments have unsaved changes */
    BOOL m_DiscardChanges;                         /**< Whether to discard changes on close */
    BOOL m_UpgradeLandFlags;                       /**< Whether land flags need upgrade */
    wxAcceleratorTable* m_pAccel;                 /**< Accelerator table for keyboard shortcuts */
    long m_Brightness_Delta;                       /**< Brightness adjustment for map display */

private:
    CAtlaSortColl m_Reports;                      /**< Collection of loaded reports */
    CLongSortColl m_ReportDates;                   /**< Report dates for quick navigation */
    BOOL m_FirstLoad;                              /**< Whether this is first load of application */
    CStr m_HexDescrSrc;                            /**< Source text for hex description */
    CStr m_UnitDescrSrc;                            /**< Source text for unit description */
    CStr m_MsgSrc;                                  /**< Source text for messages */
    long m_SelUnitIdx;                              /**< Currently selected unit index */
    int m_layout;                                   /**< Current window layout */
    BOOL m_DisableErrs;                             /**< Whether error messages are disabled */
    CBufColl m_MoveModes;                           /**< Collection of movement modes */
    CWeightsColl m_ItemWeights;                     /**< Item weights for movement calculation */
    CConfigFile m_Config[CONFIG_FILE_COUNT];        /**< Configuration files */
    CStringSortColl m_ConfigSectionsState;          /**< State of configuration sections */
    BOOL m_OrdersAreChanged;                         /**< Whether orders have unsaved changes */
    CStr m_sTitle;                                   /**< Application title */
    CHashStrToLong m_OrderHash;                      /**< Hash mapping order names to IDs */
    CHashStrToLong m_TradeItemsHash;                  /**< Hash of trade items */
    CHashStrToLong m_MenHash;                         /**< Hash of man/soldier items */
    CHashStrToLong m_MaxSkillHash;                    /**< Hash for maximum skill levels */
    CHashStrToLong m_MagicSkillsHash;                 /**< Hash of magic skills */
    int m_nStdoutLastPos;                             /**< Last read position in stdout redirect */
    int m_nStderrLastPos;                             /**< Last read position in stderr redirect */
    CBaseColl m_Attitudes;                            /**< Faction attitudes collection */
    CStringSortColl m_WaterTerrainNames;               /**< Names of water terrains */
};

extern CAhApp* gpApp;                                /**< Global application instance */
extern CGameDataHelper ThisGameDataHelper;          /**< Game data helper instance */

#endif