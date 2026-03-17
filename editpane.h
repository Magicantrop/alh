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

#ifndef __AH_EDIT_PANE_INCL__
#define __AH_EDIT_PANE_INCL__

class CStr;

/**
 * @class CEditPane
 * @brief Editable text pane with header and change tracking
 * 
 * This panel provides a text editor with a header label, support for
 * read-only mode, change tracking, and automatic font/color application.
 * Used throughout the application for editing unit orders, descriptions,
 * comments, etc.
 */
class CEditPane : public wxPanel
{
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param header Header text to display above editor
     * @param editable Whether the editor should be editable
     * @param WhichFont Font identifier (from FONT_* enum in ahapp.h)
     */
    CEditPane(wxWindow *parent, const wxString &header, BOOL editable, int WhichFont);
    
    /**
     * @brief Destructor
     */
    virtual     ~CEditPane();

    /**
     * @brief Updates pane content from source
     */
    virtual void Update();
    
    /**
     * @brief Initializes the pane (sets up controls)
     */
    virtual void Init();
    
    /**
     * @brief Sets the data source for the editor
     * @param pSource Pointer to source string (can be NULL)
     * @param pChanged Pointer to change flag (can be NULL)
     */
    void         SetSource(CStr * pSource, BOOL * pChanged);
    
    /**
     * @brief Applies current font settings to the editor
     */
    virtual void ApplyFonts();
    
    /**
     * @brief Saves any modifications back to the source
     * @return TRUE if changes were saved, FALSE otherwise
     */
    BOOL         SaveModifications();
    
    /**
     * @brief Handles focus loss (saves modifications)
     */
    void         OnKillFocus();
    
    /**
     * @brief Handles double-click events
     */
    void         OnMouseDClick();
    
    /**
     * @brief Sets read-only state of the editor
     * @param ReadOnly TRUE for read-only, FALSE for editable
     */
    void         SetReadOnly(BOOL ReadOnly);
    
    /**
     * @brief Gets current editor content
     * @param value Output string for editor content
     */
    void         GetValue(CStr & value);

    /** Pointer to the text editor control */
    wxTextCtrl   * m_pEditor;

protected:
    /**
     * @brief Handles resize events to adjust editor size
     * @param event Size event
     */
    void         OnSize      (wxSizeEvent & event);

    CStr         * m_pSource;       /**< Pointer to source data string */
    BOOL         * m_pChanged;       /**< Pointer to change flag */
    wxStaticText * m_pHeader;        /**< Header static text control */
    int            m_HdrHeight;      /**< Height of header (for layout) */
    int            m_WhichFont;      /**< Font identifier for this pane */
    wxColour       m_ColorNormal;    /**< Normal text color */
    wxColour       m_ColorReadOnly;  /**< Read-only text color */

    DECLARE_EVENT_TABLE()
};

#endif