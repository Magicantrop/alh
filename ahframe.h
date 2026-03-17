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

#ifndef __AH_FRAME_INCL__
#define __AH_FRAME_INCL__

#include <wx/dialog.h>
#include <wx/panel.h>

//-------------------------------------------------------------------

/**
 * @class CAhFrame
 * @brief Base frame class for all Atlantis Little Helper windows
 * 
 * This class extends wxFrame to provide common functionality for all
 * application frames, including layout management, configuration storage,
 * and standard command handling.
 */
class CAhFrame : public wxFrame
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param title Frame title
     * @param style Window style flags
     */
    CAhFrame(wxWindow* parent, const char * title, long style);

    /**
     * @brief Initializes the frame with specific layout and configuration
     * @param layout Layout type identifier
     * @param szConfigSection Configuration section name for storing frame settings
     */
    virtual void    Init(int layout, const char * szConfigSection);
    
    /**
     * @brief Updates frame content based on current application state
     */
    virtual void    Update();
    
    /**
     * @brief Performs cleanup operations before frame destruction
     * @param SetClosedFlag Whether to mark the frame as closed in the application
     */
    virtual void    Done(BOOL SetClosedFlag);

    /** Configuration section name for storing frame settings (position, size, etc.) */
    CStr            m_sConfigSection;

    /** Array of panes contained within this frame */
    wxWindow      * m_Panes [AH_PANE_COUNT ];

protected:
    /**
     * @brief Assigns a pane to a specific position in the frame
     * @param no Pane index (from AH_PANE_* enum)
     * @param pane Pointer to the pane window
     */
    void            SetPane(int no, wxWindow * pane);
    
    /**
     * @brief Event handler for Save Orders command
     * @param event Command event
     */
    void            OnSaveOrders(wxCommandEvent& event);
    
    /**
     * @brief Event handler for Next Unit command
     * @param event Command event
     */
    void            OnNextUnit(wxCommandEvent& event);
    
    /**
     * @brief Event handler for Previous Unit command
     * @param event Command event
     */
    void            OnPrevUnit(wxCommandEvent& event);
    
    /**
     * @brief Event handler for Unit List command
     * @param event Command event
     */
    void            OnUnitList(wxCommandEvent& event);
    
    /**
     * @brief Event handler for Orders command
     * @param event Command event
     */
    void            OnOrders(wxCommandEvent& event);

    /** Current layout type identifier */
    int             m_Layout;
};

//-------------------------------------------------------------------

/**
 * @class CFlatPanel
 * @brief A panel that automatically resizes its single child to fill the panel
 * 
 * This panel is designed to contain exactly one child window and will
 * automatically resize that child to fill the entire panel area when the
 * panel is resized.
 */
class CFlatPanel : public wxPanel
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     */
    CFlatPanel(wxWindow* parent);
    
    /**
     * @brief Sets the child window to be automatically resized
     * @param child Pointer to the child window
     */
    void SetChild(wxWindow* child) {m_pChild = child;};

private:
    /**
     * @brief Handles size events to resize the child window
     * @param event Size event
     */
    void OnSize(wxSizeEvent& event);

    /** Pointer to the child window that will be automatically resized */
    wxWindow * m_pChild;

    DECLARE_EVENT_TABLE()
};

//-------------------------------------------------------------------

/**
 * @class CResizableDlg
 * @brief Base class for resizable dialogs with automatic size/position storage
 * 
 * This class extends wxDialog to provide automatic saving and restoration
 * of dialog size and position based on configuration section names.
 */
class CResizableDlg : public wxDialog
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param title Dialog title
     * @param szConfigSection Configuration section name for storing size/position
     * @param style Window style flags (default includes resize border and system menu)
     */
    CResizableDlg(wxWindow * parent, const wxString& title, const char * szConfigSection, long style = wxRESIZE_BORDER | wxSYSTEM_MENU );

protected:
    /**
     * @brief Sets dialog size from stored configuration
     */
    void SetSize();
    
    /**
     * @brief Stores current dialog size to configuration
     */
    void StoreSize();
    
    /**
     * @brief Sets dialog position from stored configuration
     */
    void SetPos();
    
    /**
     * @brief Handles close event to store size/position before closing
     * @param event Close event
     */
    void OnClose (wxCloseEvent& event);

    /** Configuration section name for storing dialog settings */
    CStr m_sConfigSection;

    DECLARE_EVENT_TABLE()
};

//-------------------------------------------------------------------

#endif