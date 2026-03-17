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

#ifndef __AH_UNIT_FRAME_INCL__
#define __AH_UNIT_FRAME_INCL__

/**
 * @class CUnitFrame
 * @brief Frame displaying unit information and lists
 * 
 * This frame manages the display of unit-related information, including
 * lists of units in the current hex, unit details, and commands. It uses
 * a hierarchy of splitter windows to arrange the panes according to the
 * selected layout (2-window, 3-window, etc.).
 */
class CUnitFrame : public CAhFrame
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     */
    CUnitFrame(wxWindow * parent);

    /**
     * @brief Initializes the frame with specified layout
     * @param layout Layout type (AH_LAYOUT_* constants)
     * @param szConfigSection Configuration section for saving state
     */
    virtual void    Init(int layout, const char * szConfigSection);
    
    /**
     * @brief Performs cleanup before frame destruction
     * @param SetClosedFlag Whether to mark the frame as closed
     */
    virtual void    Done(BOOL SetClosedFlag);

    /**
     * @brief Gets the configuration section name for a layout
     * @param layout Layout type
     * @return Configuration section name string
     */
    static const char * GetConfigSection(int layout);

private:
    /**
     * @brief Handles window close event
     * @param event Close event
     */
    void OnCloseWindow(wxCloseEvent& event);

    // Splitter windows for arranging panes
    wxSplitterWindow  * m_Splitter1;   /**< Primary splitter (top-level) */
    wxSplitterWindow  * m_Splitter2;   /**< Secondary splitter (nested) */
    wxSplitterWindow  * m_Splitter3;   /**< Tertiary splitter (nested) */

    DECLARE_EVENT_TABLE()
};

#endif