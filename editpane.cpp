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

#include "stdafx.h"
#include "stdhdr.h"

#include "cstr.h"
#include "collection.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "data.h"
#include "hash.h"

#include "objs.h"
#include "ahapp.h"
#include "editpane.h"

//--------------------------------------------------------------------
// CEditorForPane - Custom text control that forwards events to parent pane
//--------------------------------------------------------------------

class CEditorForPane : public wxTextCtrl
{
public:
    CEditorForPane(CEditPane * parent);

protected:
    void         OnKillFocus(wxFocusEvent& event);
    void         OnMouseEvent(wxMouseEvent& event);

    CEditPane * m_pParent;  // Parent pane to forward events to

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------

BEGIN_EVENT_TABLE(CEditorForPane, wxTextCtrl)
    EVT_KILL_FOCUS       (    CEditorForPane::OnKillFocus      )
    EVT_LEFT_DCLICK      (    CEditorForPane::OnMouseEvent     )
END_EVENT_TABLE()

//--------------------------------------------------------------------

/**
 * Constructor for custom editor control
 * @param parent Parent CEditPane that owns this editor
 */
CEditorForPane::CEditorForPane(CEditPane * parent)
               :wxTextCtrl(parent, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE)
{
    m_pParent = parent;
}

//--------------------------------------------------------------------

/**
 * Handles focus loss event - forwards to parent pane
 * This ensures changes are saved when user tabs out or clicks elsewhere
 */
void CEditorForPane::OnKillFocus(wxFocusEvent& event)
{
    m_pParent->OnKillFocus();
    event.Skip();
}

/**
 * Handles double-click event - forwards to parent pane
 * Used for navigation to referenced units/lands in messages
 */
void CEditorForPane::OnMouseEvent(wxMouseEvent& event)
{
    m_pParent->OnMouseDClick();
}

//====================================================================
// CEditPane Implementation
//====================================================================

BEGIN_EVENT_TABLE(CEditPane, wxPanel)
    EVT_SIZE             (                      CEditPane::OnSize           )
END_EVENT_TABLE()

//--------------------------------------------------------------------

/**
 * Constructor for editable pane
 * @param parent Parent window
 * @param header Header text to display above editor
 * @param editable Whether editor should be editable
 * @param WhichFont Font identifier for this pane
 */
CEditPane::CEditPane(wxWindow* parent, const wxString& header, BOOL editable, int WhichFont)
          :wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize )
{
    m_pSource       = NULL;   // No source initially
    m_pChanged      = NULL;   // No change flag initially

    // Create header text control
    m_pHeader       = new wxStaticText(this, -1, header, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE );
    m_HdrHeight     = 0;      // Will be set in ApplyFonts()
    m_WhichFont     = WhichFont;
    
    // Create editor control
    m_pEditor       = new CEditorForPane(this);
    
    // Store normal background color for editable state
    m_ColorNormal   = m_pEditor->GetBackgroundColour();

    // Calculate read-only background color by applying brightness delta
    m_ColorReadOnly.Set(APPLY_COLOR_DELTA(m_ColorNormal.Red()),
                        APPLY_COLOR_DELTA(m_ColorNormal.Green()),
                        APPLY_COLOR_DELTA(m_ColorNormal.Blue()));

    SetReadOnly(!editable);
}

/**
 * Destructor
 */
CEditPane::~CEditPane()
{
}

//--------------------------------------------------------------------

/**
 * Initializes the pane - applies fonts
 */
void CEditPane::Init()
{
    ApplyFonts();
}

//--------------------------------------------------------------------

/**
 * Sets the data source for the editor
 * @param pSource Pointer to source string (can be NULL)
 * @param pChanged Pointer to change flag (can be NULL)
 */
void CEditPane::SetSource(CStr * pSource, BOOL * pChanged)
{
    m_pSource   = pSource;
    m_pChanged  = pChanged;
    
    // Update editor content from source
    m_pEditor->SetValue(pSource ? wxString::FromUTF8(pSource->GetData()) : wxString(wxT("")));
}

//--------------------------------------------------------------------

/**
 * Update method - currently does nothing, can be overridden by derived classes
 */
void CEditPane::Update()
{
}

//--------------------------------------------------------------------

/**
 * Saves any modifications back to the source
 * @return TRUE if changes were saved
 */
BOOL CEditPane::SaveModifications()
{
    if (m_pEditor->IsModified())
    {
        if (m_pSource)
            m_pSource->SetStr(m_pEditor->GetValue().mb_str());
        if (m_pChanged)
            *m_pChanged = TRUE;
        m_pEditor->DiscardEdits();

        // Return FALSE if both source and changed flag are NULL (no effect)
        if (!m_pSource && !m_pChanged)
            return FALSE;

        return TRUE;
    }
    else
        return FALSE;
}

//--------------------------------------------------------------------

/**
 * Gets current editor content
 * @param value Output string for editor content
 */
void CEditPane::GetValue(CStr & value)
{
    value.SetStr(m_pEditor->GetValue().mb_str());
}

//--------------------------------------------------------------------

/**
 * Applies current font settings to the editor and header
 */
void CEditPane::ApplyFonts()
{
    // Apply font to editor
    m_pEditor->SetFont(*gpApp->m_Fonts[m_WhichFont]);

    if (m_pHeader)
    {
        wxCoord w, h, descent, ext;

        // Apply font to header and calculate its height
        m_pHeader->SetFont(*gpApp->m_Fonts[FONT_EDIT_HDR]);
        m_pHeader->GetTextExtent(wxT("A"), &w, &h, &descent, &ext);

        m_HdrHeight = h + 2;  // Add small padding
    }
}

//--------------------------------------------------------------------

/**
 * Sets read-only state of the editor
 * @param ReadOnly TRUE for read-only, FALSE for editable
 */
void CEditPane::SetReadOnly(BOOL ReadOnly)
{
    if (m_pEditor)
    {
        m_pEditor->SetEditable(!ReadOnly);
        // Use different background color for read-only state
        m_pEditor->SetBackgroundColour(ReadOnly ? m_ColorReadOnly : m_ColorNormal);
    }
}

//--------------------------------------------------------------------

/**
 * Handles focus loss - saves modifications if any
 * Called by CEditorForPane when it loses focus
 */
void CEditPane::OnKillFocus()
{
    if (SaveModifications())
        gpApp->EditPaneChanged(this);  // Notify application of change
}

//--------------------------------------------------------------------

/**
 * Handles resize events - adjusts header and editor sizes
 * Header stays at top, editor takes remaining space
 */
void CEditPane::OnSize(wxSizeEvent& event)
{
    wxSize size = event.GetSize();

    // Position header at top
    if (m_pHeader && (m_HdrHeight > 0))
        m_pHeader->SetSize(0, 0, size.x, m_HdrHeight, wxSIZE_ALLOW_MINUS_ONE);

    // Position editor below header, taking remaining height
    m_pEditor->SetSize(0, m_HdrHeight, size.x, size.y - m_HdrHeight, wxSIZE_ALLOW_MINUS_ONE);
}

//--------------------------------------------------------------------

/**
 * Handles double-click events
 * Called by CEditorForPane when double-click occurs
 * Used for navigation to referenced units/lands in messages
 */
void CEditPane::OnMouseDClick()
{
    gpApp->EditPaneDClicked(this);
}