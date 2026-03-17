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

#ifndef __AH_UNIT_FRAME_FLTR_INCL__
#define __AH_UNIT_FRAME_FLTR_INCL__

//----------------------------------------------------------------

/**
 * @class CUnitFrameFltr
 * @brief Frame displaying filtered unit lists
 * 
 * This frame shows units that match the current filter criteria.
 * Unlike the regular unit frame which shows units in the current hex,
 * this frame can display units from anywhere on the map that match
 * the active filter settings. Supports multiple filter sets and
 * custom Python filters.
 */
class CUnitFrameFltr : public CAhFrame
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     */
    CUnitFrameFltr(wxWindow * parent);

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

    DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------

#endif