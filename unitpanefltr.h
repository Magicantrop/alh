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

#ifndef __AH_UNIT_PANE_FLTR_INCL__
#define __AH_UNIT_PANE_FLTR_INCL__

#include "listpane.h"
#include "unitpane.h"
#include "unitfilterdlg.h"
#include <wx/listctrl.h>   // For wxListEvent
#include <wx/window.h>

//----------------------------------------------------------------

/**
 * @class CUnitPaneFltr
 * @brief Filtered unit list pane
 * 
 * This specialized unit pane displays units that match the current
 * filter criteria. Unlike CUnitPane which shows units in the current hex,
 * this pane shows units from anywhere on the map that satisfy the
 * active filter. Supports:
 * - Filter-based unit collection
 * - Land flag clearing for new filter runs
 * - Batch insertion of filtered units
 * - Filter-specific context menu options
 */
class CUnitPaneFltr: public CUnitPane
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param id Window ID (default list_units_hex_fltr)
     */
    CUnitPaneFltr(wxWindow *parent, wxWindowID id = list_units_hex_fltr);
    
    /**
     * @brief Updates the pane with units matching the filter
     * @param pFilter Filter dialog containing current filter settings
     */
    void         Update(CUnitFilterDlg * pFilter);
    
    /**
     * @brief Performs cleanup
     */
    virtual void Done();

    /**
     * @brief Initializes batch insertion of units
     * 
     * Call before adding multiple units via InsertUnit to optimize
     * performance by suspending redraws.
     */
    void InsertUnitInit();
    
    /**
     * @brief Inserts a single unit into the filtered list
     * @param pUnit Unit to add
     */
    void InsertUnit(CUnit * pUnit);
    
    /**
     * @brief Completes batch insertion and refreshes display
     */
    void InsertUnitDone();

    /**
     * @brief Handles filter-related popup menu commands
     * @param event Command event
     */
    void OnPopupMenuFilter   (wxCommandEvent& event) ;

private:
    /**
     * @brief Clears land flags used for filter marking
     */
    void ClearLandFlags();

    //-------------------------------------------------------------------------
    // Event Handlers (override base class)
    //-------------------------------------------------------------------------

    /**
     * @brief Handles unit selection in the list
     * @param event List event
     */
    void OnSelected(wxListEvent& event);
    
    /**
     * @brief Handles column header click for sorting
     * @param event List event
     */
    void OnColClicked(wxListEvent& event);
    
    /**
     * @brief Handles idle events for updates
     * @param event Idle event
     */
    void OnIdle(wxIdleEvent& event);
    
    /**
     * @brief Handles right-click for context menu
     * @param event List event
     */
    void OnRClick(wxListEvent& event);

    //-------------------------------------------------------------------------
    // Filter-Specific Popup Menu Handlers
    //-------------------------------------------------------------------------

    /**
     * @brief Handles sort settings for filtered view
     * @param event Command event
     */
    void OnPopupMenuSetSort    (wxCommandEvent& event) ;
    
    /**
     * @brief Handles issue orders command (overridden for filter view)
     * @param event Command event
     */
    void OnPopupMenuIssueOrders(wxCommandEvent& event);

    //-------------------------------------------------------------------------
    // Private Data Members
    //-------------------------------------------------------------------------

    int  m_ColClickedFltr;      /**< Last clicked column in filter view */
    BOOL m_IsUpdating;          /**< Whether batch update is in progress */
    CBaseColl m_NewUnits;       /**< Temporary collection for batch insertion */

    DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------

#endif