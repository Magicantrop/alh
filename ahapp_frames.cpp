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

#include "stdhdr.h"
#include "ahapp.h"
#include "ahframe.h"
#include "mapframe.h"
#include "unitframe.h"
#include "shaftsframe.h"
#include "unitframefltr.h"
#include "unitpanefltr.h"
#include "unitfilterdlg.h"
#include "msgframe.h"
#include "editsframe.h"
#include "mapframe.h"
#include "mappane.h"
#include "unitpane.h"
#include "editpane.h"
#include "optionsdlg.h"

//-------------------------------------------------------------------------
// Frame Management Methods
//-------------------------------------------------------------------------

/**
 * Opens or raises the main map frame
 * Creates new frame if it doesn't exist, otherwise brings existing to front
 */
void CAhApp::OpenMapFrame()
{
    if (!m_Frames[AH_FRAME_MAP])
    {
        // Create new map frame with current layout
        m_Frames[AH_FRAME_MAP] = new CMapFrame(NULL, m_layout);
        m_Frames[AH_FRAME_MAP]->Init(m_layout, NULL);
        m_Frames[AH_FRAME_MAP]->Show(TRUE);
    }
    else
        m_Frames[AH_FRAME_MAP]->Raise();  // Bring existing to front
}

//-------------------------------------------------------------------------

/**
 * Opens or raises the units frame
 * Shows units in current hex with their details
 */
void CAhApp::OpenUnitFrame()
{
    if (!m_Frames[AH_FRAME_UNITS])
    {
        // Create new units frame (parent is map frame)
        m_Frames[AH_FRAME_UNITS] = new CUnitFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_UNITS]->Init(m_layout, NULL);
        m_Frames[AH_FRAME_UNITS]->Show(TRUE);
    }
    else
        m_Frames[AH_FRAME_UNITS]->Raise();
}

//--------------------------------------------------------------------------

/**
 * Opens or raises the shaft connection GUI
 * Used for managing connections between planes via shafts
 */
void CAhApp::ShowShaftConnectGUI()
{
    if (!m_Frames[AH_FRAME_SHAFTS])
    {
        m_Frames[AH_FRAME_SHAFTS] = new ShaftsFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_SHAFTS]->Init(m_layout, NULL);
        m_Frames[AH_FRAME_SHAFTS]->Show(TRUE);
    }
    else
        m_Frames[AH_FRAME_SHAFTS]->Raise();
}

//-------------------------------------------------------------------------

/**
 * Opens or raises the filtered units frame
 * @param PopUpSettings If TRUE, immediately shows filter settings dialog
 * 
 * This frame shows units that match current filter criteria,
 * not just units in current hex
 */
void CAhApp::OpenUnitFrameFltr(BOOL PopUpSettings)
{
    if (!m_Frames[AH_FRAME_UNITS_FLTR])
    {
        // Create new filtered units frame
        m_Frames[AH_FRAME_UNITS_FLTR] = new CUnitFrameFltr(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_UNITS_FLTR]->Init(m_layout, NULL);
        m_Frames[AH_FRAME_UNITS_FLTR]->Show(TRUE);

        CUnitPaneFltr   * pUnitPaneF = (CUnitPaneFltr*)m_Panes [AH_PANE_UNITS_FILTER];
        wxCommandEvent    event;

        if (pUnitPaneF)
            if  (PopUpSettings)
                pUnitPaneF->OnPopupMenuFilter(event);  // Show filter dialog immediately
            else
                pUnitPaneF->Update(NULL);               // Just update with current filter
    }
    else
        m_Frames[AH_FRAME_UNITS_FLTR]->Raise();
}

//-------------------------------------------------------------------------

/**
 * Opens the messages frame
 * Displays game messages, events, and errors
 */
void CAhApp::OpenMsgFrame()
{
    if (!m_Frames[AH_FRAME_MSG])
    {
        m_Frames[AH_FRAME_MSG] = new CMsgFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_MSG]->Init(m_layout, NULL);
        m_MsgSrc.Empty();  // Clear message source for fresh display
        m_Frames[AH_FRAME_MSG]->Show(TRUE);
    }
    // Note: No Raise() here - message frame doesn't support raising
}

//-------------------------------------------------------------------------

/**
 * Opens or raises the editors frame
 * Contains order editors, descriptions, and comments
 */
void CAhApp::OpenEditsFrame()
{
    if (!m_Frames[AH_FRAME_EDITS])
    {
        m_Frames[AH_FRAME_EDITS] = new CEditsFrame(m_Frames[AH_FRAME_MAP]);
        m_Frames[AH_FRAME_EDITS]->Init(m_layout, NULL);
        m_Frames[AH_FRAME_EDITS]->Show(TRUE);
    }
    else
        m_Frames[AH_FRAME_EDITS]->Raise();
}

//-------------------------------------------------------------------------
// UI Appearance Methods
//-------------------------------------------------------------------------

/**
 * Applies current font settings to all panes
 * Called when font preferences are changed
 */
void CAhApp::ApplyFonts()
{
    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyFonts();
    if (m_Panes[AH_PANE_MAP_DESCR    ]) ((CEditPane*)m_Panes[AH_PANE_MAP_DESCR    ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNITS_HEX    ]) ((CUnitPane*)m_Panes[AH_PANE_UNITS_HEX    ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNITS_FILTER ]) ((CUnitPane*)m_Panes[AH_PANE_UNITS_FILTER ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNIT_DESCR   ]) ((CEditPane*)m_Panes[AH_PANE_UNIT_DESCR   ])->ApplyFonts();
    if (m_Panes[AH_PANE_UNIT_COMMANDS]) ((CEditPane*)m_Panes[AH_PANE_UNIT_COMMANDS])->ApplyFonts();
    if (m_Panes[AH_PANE_UNIT_COMMENTS]) ((CEditPane*)m_Panes[AH_PANE_UNIT_COMMENTS])->ApplyFonts();
    if (m_Panes[AH_PANE_MSG          ]) ((CEditPane*)m_Panes[AH_PANE_MSG          ])->ApplyFonts();
}

//-------------------------------------------------------------------------

/**
 * Applies current color settings to map display
 */
void CAhApp::ApplyColors()
{
    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyColors();
    // Note: Only map pane uses colors - other panes use system defaults
}

//-------------------------------------------------------------------------

/**
 * Applies current icon set to map display
 */
void CAhApp::ApplyIcons()
{
    if (m_Panes[AH_PANE_MAP          ]) ((CMapPane *)m_Panes[AH_PANE_MAP          ])->ApplyIcons();
}

//-------------------------------------------------------------------------

/**
 * Opens the options dialog for application preferences
 */
void CAhApp::OpenOptionsDlg()
{
    int rc;

    COptionsDialog *dialog = new COptionsDialog(m_Frames[AH_FRAME_MAP]);
    {
        dialog->Init();
        rc = dialog->ShowModal();
        if (wxID_OK==rc)
        {
            // Changes are applied immediately by dialog->Done()
        }
        dialog->Done();
    }
}

//-------------------------------------------------------------------------

/**
 * Creates keyboard accelerator table for common commands
 * Sets up Ctrl+key shortcuts:
 * - Ctrl+S: Save Orders
 * - Ctrl+N: Next Unit
 * - Ctrl+P: Previous Unit
 * - Ctrl+U: Unit List
 * - Ctrl+O: Orders
 */
void CAhApp::CreateAccelerator()
{
    static wxAcceleratorEntry entries[5];
    entries[0].Set(wxACCEL_CTRL,  (int)'S',     menu_SaveOrders);
    entries[1].Set(wxACCEL_CTRL,  (int)'N',     accel_NextUnit );
    entries[2].Set(wxACCEL_CTRL,  (int)'P',     accel_PrevUnit );
    entries[3].Set(wxACCEL_CTRL,  (int)'U',     accel_UnitList );
    entries[4].Set(wxACCEL_CTRL,  (int)'O',     accel_Orders   );

    m_pAccel = new wxAcceleratorTable(5, entries);
}