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
#include "stdafx.h"

#include <wx/display.h>

#include "cstr.h"
#include "collection.h"
#include "cfgfile.h"
#include "files.h"
#include "atlaparser.h"
#include "consts.h"
#include "consts_ah.h"
#include "hash.h"

#include "ahapp.h"
#include "ahframe.h"

#ifndef __WXMSW__
    #include "bitmaps/icon_logo.xpm"
#endif

//==========================================================================
// CFlatPanel - A panel that automatically resizes its single child
//==========================================================================

BEGIN_EVENT_TABLE(CFlatPanel, wxPanel)
    EVT_SIZE  (CFlatPanel::OnSize)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

CFlatPanel::CFlatPanel(wxWindow* parent)
          :wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize )
{
    m_pChild = NULL;
}

//--------------------------------------------------------------------------
// Handles resize events - resizes child to fill entire panel
//--------------------------------------------------------------------------

void CFlatPanel::OnSize(wxSizeEvent& event)
{
    wxSize size = event.GetSize();

    if (m_pChild)
        m_pChild->SetSize(0, 0, size.x, size.y, wxSIZE_ALLOW_MINUS_ONE);
}

//==========================================================================
// CAhFrame - Base frame class for all application windows
//==========================================================================

CAhFrame::CAhFrame(wxWindow* parent, const char * title, long style)
           :wxFrame(parent, wxID_ANY, wxString::FromAscii(title), wxDefaultPosition, wxSize(10,10), style)
{
    memset(m_Panes , 0, sizeof(m_Panes ));
}

//--------------------------------------------------------------------------
// Initializes the frame: sets icon, loads saved position/size, applies accelerator
//--------------------------------------------------------------------------

void CAhFrame::Init(int layout, const char * szConfigSection)
{
    int x, y, w, h, saved_pos;

    m_sConfigSection  = szConfigSection;
    m_Layout          = layout;

    SetIcon(wxICON(icon_logo));
    
    // Load saved position and size from config
    x = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_X1));
    y = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_Y1));
    w = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_X2)) - x;
    h = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_Y2)) - y;
    saved_pos = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_USE_SAVED_POS));

    // On first run, adjust position to ensure title bar is visible
    // (important on systems with top menu bar like macOS)
    if (0==saved_pos)
    {
        const char      * szName;
        const char      * szValue;
        if (gpApp->GetSectionFirst(SZ_SECT_REPORTS, szName, szValue) < 0)
        {
            int  x_old, y_old;
            GetPosition(&x_old, &y_old);
            y = y_old;
        }
        gpApp->SetConfig(szConfigSection, SZ_KEY_USE_SAVED_POS, "1");
    }

    // Check if saved position is on a valid display
    if (wxDisplay::GetFromPoint(wxPoint(x, y)) == wxNOT_FOUND)
    {
        x = 0;
        y = 0;
    }

    SetSize(x, y, w, h, wxSIZE_ALLOW_MINUS_ONE);

    // Apply global accelerator table if available
    if (gpApp->m_pAccel)
        SetAcceleratorTable(*gpApp->m_pAccel);
}

//--------------------------------------------------------------------------
// Placeholder for frame update - derived classes override as needed
//--------------------------------------------------------------------------

void CAhFrame::Update()
{
}

//--------------------------------------------------------------------------
// Called when frame is closing - saves position/size and clears pane references
//--------------------------------------------------------------------------

void CAhFrame::Done(BOOL SetClosedFlag)
{
    int x, y, w, h, i;
    if (gpApp)
    {
        GetPosition(&x, &y);
        GetSize    (&w, &h);

        // Save current position and size
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_X1, x);
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_Y1, y);
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_X2, x+w);
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_Y2, y+h);

        // Save open state (inverted because SetClosedFlag TRUE means we're closing)
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_OPEN, SetClosedFlag?"0":"1");

        // Clear pane references
        for (i=0; i<AH_PANE_COUNT; i++)
            if (m_Panes[i])
                SetPane(i, NULL);
    }
}

//--------------------------------------------------------------------------
// Sets a pane in this frame and updates global pane array
//--------------------------------------------------------------------------

void CAhFrame::SetPane(int no, wxWindow * pane)
{
    m_Panes[no] = pane;
    gpApp->m_Panes[no] = pane;
}

//--------------------------------------------------------------------------
// Menu command handlers - forward to application
//--------------------------------------------------------------------------

void CAhFrame::OnSaveOrders(wxCommandEvent& WXUNUSED(event))
{
    gpApp->SaveOrders(TRUE);
}

void CAhFrame::OnNextUnit(wxCommandEvent& event)
{
    gpApp->SelectNextUnit();
}

void CAhFrame::OnPrevUnit(wxCommandEvent& event)
{
    gpApp->SelectPrevUnit();
}

void CAhFrame::OnUnitList(wxCommandEvent& event)
{
    gpApp->SelectUnitsPane();
}

void CAhFrame::OnOrders(wxCommandEvent& event)
{
    gpApp->SelectOrdersPane();
}

//==========================================================================
// CResizableDlg - Base class for resizable dialogs with position/size saving
//==========================================================================

BEGIN_EVENT_TABLE(CResizableDlg, wxDialog)
    EVT_CLOSE  (    CResizableDlg::OnClose )
END_EVENT_TABLE()

CResizableDlg::CResizableDlg(wxWindow * parent, const wxString &title, const char * szConfigSection, long style)
              :wxDialog( parent, -1, title, wxDefaultPosition, wxDefaultSize, style | wxCAPTION  )
{
    m_sConfigSection = szConfigSection;
}

//--------------------------------------------------------------------------
// Sets dialog size from saved configuration
//--------------------------------------------------------------------------

void CResizableDlg::SetSize()
{
    int x, y, w=-1, h=-1;

    x = atol(gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_X1));
    y = atol(gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_Y1));
    
    // Only restore size for resizable dialogs
    if (GetWindowStyle() & wxRESIZE_BORDER)
    {
        w = atol(gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_X2)) - x;
        h = atol(gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_Y2)) - y;
    }

    // Validate display
    if (wxDisplay::GetFromPoint(wxPoint(x, y)) == wxNOT_FOUND)
    {
        x = 0;
        y = 0;
    }

    wxDialog::SetSize(x, y, w, h, wxSIZE_ALLOW_MINUS_ONE);
}

//--------------------------------------------------------------------------
// Sets dialog position only (without changing size)
//--------------------------------------------------------------------------

void CResizableDlg::SetPos()
{
    int x, y;

    x = atol(gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_X1));
    y = atol(gpApp->GetConfig(m_sConfigSection.GetData(), SZ_KEY_Y1));

    if (wxDisplay::GetFromPoint(wxPoint(x, y)) == wxNOT_FOUND)
    {
        x = 0;
        y = 0;
    }

    wxDialog::Move(x, y);
}

//--------------------------------------------------------------------------
// Saves current dialog size and position to configuration
//--------------------------------------------------------------------------

void CResizableDlg::StoreSize()
{
    int x, y, w, h;

    GetPosition(&x, &y);
    GetSize    (&w, &h);

    gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_X1, x);
    gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_Y1, y);
    gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_X2, x+w);
    gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_Y2, y+h);
}

//--------------------------------------------------------------------------
// Handles dialog close - saves size/position before closing
//--------------------------------------------------------------------------

void CResizableDlg::OnClose(wxCloseEvent& event)
{
    StoreSize();
    event.Skip();
}

//==========================================================================