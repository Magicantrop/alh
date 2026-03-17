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

#include "wx/listctrl.h"
#include "wx/splitter.h"

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
#include "editpane.h"
#include "editsframe.h"

// Event table - maps menu commands and window events to handlers
BEGIN_EVENT_TABLE(CEditsFrame, CAhFrame)
    EVT_MENU   (menu_SaveOrders         , CEditsFrame::OnSaveOrders)
    EVT_MENU   (accel_NextUnit          , CEditsFrame::OnNextUnit)
    EVT_MENU   (accel_PrevUnit          , CEditsFrame::OnPrevUnit)
    EVT_MENU   (accel_UnitList          , CEditsFrame::OnUnitList)
    EVT_MENU   (accel_Orders            , CEditsFrame::OnOrders  )
    EVT_CLOSE  (                          CEditsFrame::OnCloseWindow)
END_EVENT_TABLE()

//--------------------------------------------------------------------

/**
 * Constructor for the editors frame
 * @param parent Parent window
 */
CEditsFrame::CEditsFrame(wxWindow * parent)
            :CAhFrame (parent, "Editor panes", (wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN) & ~wxMINIMIZE_BOX)
{
    // Initialize splitter pointers to NULL
    m_Splitter1 = NULL;
    m_Splitter2 = NULL;
    m_Splitter3 = NULL;
}

//--------------------------------------------------------------------

/**
 * Gets the configuration section name for a given layout
 * @param layout Layout type (AH_LAYOUT_* constants)
 * @return Configuration section name string
 */
const char * CEditsFrame::GetConfigSection(int layout)
{
    switch (layout)
    {
    case AH_LAYOUT_2_WIN:        return SZ_SECT_WND_EDITS_2_WIN;
    case AH_LAYOUT_3_WIN:        return SZ_SECT_WND_EDITS_3_WIN;
    default:                     return "";
    }
}

//--------------------------------------------------------------------

/**
 * Initializes the editors frame with specified layout
 * Creates and arranges all editor panes using splitter windows
 * @param layout Layout type (AH_LAYOUT_* constants)
 * @param szConfigSection Configuration section for saving state
 */
void CEditsFrame::Init(int layout, const char * szConfigSection)
{
    CEditPane *p1, *p2, *p3, *p4;
    long y, x;

    // Get the appropriate config section for this layout
    szConfigSection = GetConfigSection(layout);

    switch (layout)
    {
    case AH_LAYOUT_3_WIN:
        // Initialize base frame with saved position/size
        CAhFrame::Init(layout, szConfigSection);

        // Create splitter hierarchy:
        // m_Splitter1 (top-level) splits horizontally into:
        //   - Top pane: Hex description
        //   - Bottom: m_Splitter2
        //     m_Splitter2 splits horizontally into:
        //       - Top: Unit description
        //       - Bottom: m_Splitter3
        //         m_Splitter3 splits vertically into:
        //           - Left: Orders
        //           - Right: Comments/Default orders
        
        m_Splitter1 = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxCLIP_CHILDREN);
        m_Splitter2 = new wxSplitterWindow(m_Splitter1, -1, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxCLIP_CHILDREN);
        m_Splitter3 = new wxSplitterWindow(m_Splitter2, -1, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxCLIP_CHILDREN);

        // Remove borders from inner splitters for cleaner look
        m_Splitter2->SetBorderSize(0);
        m_Splitter3->SetBorderSize(0);

        // Create editor panes
        p1 = new CEditPane(m_Splitter1, wxT("Hex description"),        FALSE, FONT_EDIT_DESCR);
        p2 = new CEditPane(m_Splitter2, wxT("Unit description"),       FALSE, FONT_EDIT_DESCR);
        p3 = new CEditPane(m_Splitter3, wxT("Orders"),                 FALSE, FONT_EDIT_ORDER);
        p4 = new CEditPane(m_Splitter3, wxT("Comments/Default orders"), TRUE,  FONT_EDIT_ORDER);

        // Register panes with application
        SetPane(AH_PANE_MAP_DESCR,     p1);
        SetPane(AH_PANE_UNIT_DESCR,    p2);
        SetPane(AH_PANE_UNIT_COMMANDS, p3);
        SetPane(AH_PANE_UNIT_COMMENTS, p4);

        // Initialize each pane (applies fonts)
        p1->Init();
        p2->Init();
        p3->Init();
        p4->Init();

        // Configure first splitter (Hex description vs rest)
        y = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_HEIGHT_0));
        m_Splitter1->SetMinimumPaneSize(2);
        m_Splitter1->SplitHorizontally(p1, m_Splitter2, y);

        // Configure second splitter (Unit description vs third splitter)
        y = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_HEIGHT_1));
        m_Splitter2->SetMinimumPaneSize(2);
        m_Splitter2->SplitHorizontally(p2, m_Splitter3, y);

        // Configure third splitter (Orders vs Comments)
        x = atol(gpApp->GetConfig(szConfigSection, SZ_KEY_WIDTH_0));
        m_Splitter3->SetMinimumPaneSize(2);
        m_Splitter3->SplitVertically(p3, p4, x);

        break;
    }
}

//--------------------------------------------------------------------

/**
 * Performs cleanup before frame destruction
 * Saves splitter positions to configuration
 * @param SetClosedFlag Whether to mark the frame as closed
 */
void CEditsFrame::Done(BOOL SetClosedFlag)
{
    switch (m_Layout)
    {
    case AH_LAYOUT_3_WIN:
        // Save current splitter positions to config
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_HEIGHT_0, m_Splitter1->GetSashPosition());
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_HEIGHT_1, m_Splitter2->GetSashPosition());
        gpApp->SetConfig(m_sConfigSection.GetData(), SZ_KEY_WIDTH_0,  m_Splitter3->GetSashPosition());
        break;
    }

    // Call base class to save frame position/size
    CAhFrame::Done(SetClosedFlag);
}

//--------------------------------------------------------------------

/**
 * Handles window close event
 * Notifies application and destroys the frame
 * @param event Close event
 */
void CEditsFrame::OnCloseWindow(wxCloseEvent& event)
{
    gpApp->FrameClosing(this);
    Destroy();
}