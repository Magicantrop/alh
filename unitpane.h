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

#ifndef __AH_UNIT_PANE_INCL__
#define __AH_UNIT_PANE_INCL__

#include "listpane.h"      // For CListPane base class
#include <wx/listctrl.h>   // For wxListEvent
#include <wx/window.h>

/**
 * @enum ScoutType
 * @brief Types of scout units that can be created
 */
enum ScoutType {
    SCOUT_SIMPLE,      /**< Basic scout with no special abilities */
    SCOUT_MOVE,        /**< Scout optimized for movement */
    SCOUT_OBSERVER,    /**< Scout focused on observation */
    SCOUT_STEALTH,     /**< Scout with stealth capabilities */
    SCOUT_GUARD        /**< Scout set to guard mode */
};

/**
 * @class CUnitPane
 * @brief List pane for displaying units with sorting and context menus
 * 
 * This specialized list control displays units (either in current hex
 * or filtered view) with customizable columns, sorting, and context
 * menus for various unit operations. Supports:
 * - Multi-column display with configurable headers
 * - Sorting by multiple columns
 * - Context menus for unit actions (give, take, split, scout, etc.)
 * - Month-long order indicators
 * - Unit selection and navigation
 */
class CUnitPane: public CListPane
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param id Window ID (default list_units_hex)
     */
    CUnitPane(wxWindow *parent, wxWindowID id = list_units_hex);
    
    /**
     * @brief Initializes the unit pane
     * @param pParentFrame Parent frame
     * @param szConfigSection Configuration section for pane settings
     * @param szConfigSectionHdr Configuration section for column headers
     */
    virtual void Init(CAhFrame * pParentFrame, const char * szConfigSection, const char * szConfigSectionHdr);
    
    /**
     * @brief Performs cleanup
     */
    virtual void Done();
    
    /**
     * @brief Updates the pane with units from a land
     * @param pLand Land containing units to display
     */
    void         Update(CLand * pLand);
    
    /**
     * @brief Applies current font settings to the list
     */
    virtual void ApplyFonts();
    
    /**
     * @brief Gets unit at specified index
     * @param index List index (0-based)
     * @return Pointer to unit, or NULL if index invalid
     */
    CUnit      * GetUnit(long index);
    
    /**
     * @brief Sorts the list using current sort keys
     */
    virtual void Sort();
    
    /**
     * @brief Selects a unit by its ID
     * @param UnitId Unit ID to select
     */
    void         SelectUnit(long UnitId);
    
    /**
     * @brief Selects the next unit in the list
     */
    void         SelectNextUnit();
    
    /**
     * @brief Selects the previous unit in the list
     */
    void         SelectPrevUnit();

    /**
     * @brief Loads unit list header configuration
     */
    void         LoadUnitListHdr();
    
    /**
     * @brief Saves unit list header configuration
     */
    void         SaveUnitListHdr();
    
    /**
     * @brief Reloads header from specified configuration section
     * @param szConfigSectionHdr Configuration section for headers
     */
    void         ReloadHdr(const char * szConfigSectionHdr);

    /**
     * @brief Gets month-long order information for a unit
     * @param pUnit Unit to check
     * @param hasMultiple Output: true if unit has multiple month-long orders
     * @param hasNone Output: true if unit has no month-long orders
     * @param warningAdded Output: true if warning was added
     * @return Formatted string for month-long order column
     */
    wxString GetMonthLongOrdersForUnit(CUnit* pUnit, bool& hasMultiple, bool& hasNone, bool& warningAdded);
    
    /**
     * @brief Sets data source and selects an item
     * @param selmode Selection mode (by ID or by index)
     * @param seldata Selection data
     * @param FullUpdate Whether to force full refresh
     */
    virtual void SetData(eSelMode selmode, long seldata, BOOL FullUpdate);

    bool HasMonsters(CUnit* pUnit);

    // Public data members
    TPropertyHolderColl * m_pUnits;   /**< Collection of units being displayed */
    CLand               * m_pCurLand;  /**< Current land (if displaying hex units) */


protected:
    //-------------------------------------------------------------------------
    // Event Handlers
    //-------------------------------------------------------------------------

    /**
     * @brief Handles unit selection in the list
     * @param event List event
     */
    void         OnSelected(wxListEvent& event);
    
    /**
     * @brief Handles column header click for sorting
     * @param event List event
     */
    void         OnColClicked(wxListEvent& event);
    
    /**
     * @brief Handles idle events for updates
     * @param event Idle event
     */
    void         OnIdle(wxIdleEvent& event);
    
    /**
     * @brief Handles right-click for context menu
     * @param event List event
     */
    void         OnRClick(wxListEvent& event);
    
    /**
     * @brief Creates a scout unit from the selected unit
     * @param pUnit Source unit
     * @param type Scout type to create
     * @return TRUE if scout created successfully
     */
    bool         CreateScout(CUnit *, ScoutType);

    CAhFrame            * m_pFrame;              /**< Parent frame */
    CStr                  m_sConfigSection;      /**< Pane configuration section */
    CStr                  m_sConfigSectionHdr;   /**< Header configuration section */

    int                   m_ColClicked;          /**< Last clicked column (for sorting) */
    int                   m_nMonthLongColumn;    /**< Column index for month-long orders */

public:
    //-------------------------------------------------------------------------
    // Popup Menu Event Handlers
    //-------------------------------------------------------------------------

    void OnPopupMenuShareSilv         (wxCommandEvent& event);
    void OnPopupMenuTeach             (wxCommandEvent& event);
    void OnPopupMenuSplit             (wxCommandEvent& event);
    void OnPopupMenuDiscardJunk       (wxCommandEvent& event);
    void OnPopupMenuDetectSpies       (wxCommandEvent& event);
    void OnPopupMenuGiveEverything    (wxCommandEvent& event);
    void OnPopupMenuAddUnitToTracking (wxCommandEvent& event);
    void OnPopupMenuUnitFlags         (wxCommandEvent& event);
    void OnPopupMenuIssueOrders       (wxCommandEvent& event);

    void OnPopupMenuScoutSimple       (wxCommandEvent& event);
    void OnPopupMenuScoutMove         (wxCommandEvent& event);
    void OnPopupMenuScoutObserver     (wxCommandEvent& event);
    void OnPopupMenuScoutStealth      (wxCommandEvent& event);
    void OnPopupMenuScoutGuard        (wxCommandEvent& event);

    /* NEW popup menu items */
    void OnPopupMenuGive              (wxCommandEvent& event);  /**< GIVE command dialog */
    void OnPopupMenuTake              (wxCommandEvent& event);  /**< TAKE command dialog */
    void OnPopupMenuMakeTemplate      (wxCommandEvent& event);  /**< Create template from unit */
    void OnPopupMenuCreateFromTemplate(wxCommandEvent& event);  /**< Apply template to unit */
    void OnPopupMenuTransport         (wxCommandEvent& event);  /**< TRANSPORT command dialog */
    void OnPopupMenuProduce           (wxCommandEvent& event);  /**< PRODUCE command dialog */

    /* Battle Simulator integration */
    void OnPopupMenuAddToAttackers(wxCommandEvent& event);      /**< Add unit to attackers side */
    void OnPopupMenuAddToDefenders(wxCommandEvent& event);      /**< Add unit to defenders side */

    /**
     * @brief Creates orders from template for a unit
     * @param pUnit Unit to apply template to
     * @param templateName Name of template to use
     * @param count Number of times to apply (for multi-turn production)
     * @param repeatEveryTurn Whether to add @ prefix for repeat
     * @return TRUE if template was applied successfully
     */
    bool CreateFromTemplate(CUnit* pUnit,
            const wxString& templateName,
            int count,
            bool repeatEveryTurn);

    DECLARE_EVENT_TABLE()
};

#endif