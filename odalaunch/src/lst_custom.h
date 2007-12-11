// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:  
//	Custom list control, featuring sorting
//	AUTHOR:	Russell Rice
//
//-----------------------------------------------------------------------------


#ifndef LST_CUSTOM_H
#define LST_CUSTOM_H

#include <wx/listctrl.h>
#include <wx/sizer.h>

class wxAdvancedListCtrl : public wxListCtrl
{      
    public:

        wxAdvancedListCtrl()
        {
            SortOrder = 0; 
            SortCol = 0; 

            colRed = 245;
            colGreen = 245;
            colBlue = 245;

        }
        
        wxAdvancedListCtrl( wxWindow *parent,
                wxWindowID winid = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxLC_REPORT,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxListCtrlNameStr)
        {
            Create(parent, winid, pos, size, style, validator, name);
        }

        bool Create(wxWindow* parent, wxWindowID id, 
                    const wxPoint& pos = wxDefaultPosition, 
                    const wxSize& size = wxDefaultSize, 
                    long style = wxLC_ICON, 
                    const wxValidator& validator = wxDefaultValidator, 
                    const wxString& name = wxListCtrlNameStr)
        {
            SetParent(parent);
            GetSizer()->Fit(this);
   
            return true;   
        }

        ~wxAdvancedListCtrl() { }
                  
        wxInt32 GetIndex(wxString str);
               
        wxEvent *Clone(void);

    private:
        void ColourList();
        void ColourListItem(wxInt32 item, wxInt32 grey);

        void OnItemInsert(wxListEvent &event);
        void OnHeaderColumnButtonClick(wxListEvent &event);
        
        wxInt32 SortOrder;
        wxInt32 SortCol;

        wxInt32 colRed;
        wxInt32 colGreen;
        wxInt32 colBlue;

    protected:               
        DECLARE_DYNAMIC_CLASS(wxAdvancedListCtrl)
        DECLARE_EVENT_TABLE()
};

#endif
