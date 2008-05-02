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


#include "lst_custom.h"

IMPLEMENT_DYNAMIC_CLASS(wxAdvancedListCtrl, wxListCtrl)

BEGIN_EVENT_TABLE(wxAdvancedListCtrl, wxListCtrl)
     EVT_LIST_COL_CLICK(-1, wxAdvancedListCtrl::OnHeaderColumnButtonClick)
     EVT_LIST_INSERT_ITEM(-1, wxAdvancedListCtrl::OnItemInsert)
     EVT_WINDOW_CREATE(wxAdvancedListCtrl::OnCreateControl)
END_EVENT_TABLE()

// this is so we can jump over the sort arrow images
enum
{
    LIST_SORT_ARROW_UP = 0
    ,LIST_SORT_ARROW_DOWN
    ,FIRST_IMAGE
};

void wxAdvancedListCtrl::OnCreateControl(wxWindowCreateEvent &event)
{
    SortOrder = 0; 
    SortCol = 0; 

    colRed = 245;
    colGreen = 245;
    colBlue = 245;

    // sort buttons
    AssignImageList(new wxImageList(16, 15, true, FIRST_IMAGE), wxIMAGE_LIST_SMALL);
    GetImageList(wxIMAGE_LIST_SMALL)->Add(wxArtProvider::GetBitmap(wxART_GO_UP));
    GetImageList(wxIMAGE_LIST_SMALL)->Add(wxArtProvider::GetBitmap(wxART_GO_DOWN)); 
}

// Add any additional bitmaps/icons to the internal image list
void wxAdvancedListCtrl::AddImage(wxBitmap Bitmap)
{
    GetImageList(wxIMAGE_LIST_SMALL)->Add(Bitmap);
}

// Adjusts the index, so it jumps over the sort arrow images.
void wxAdvancedListCtrl::SetColumnImage(wxListItem &li, wxInt32 ImageIndex)
{
    if (ImageIndex < -1)
        ImageIndex = -1;

    li.SetImage(((ImageIndex == -1) ? ImageIndex : FIRST_IMAGE + ImageIndex));
}

// [Russell] - These are 2 heavily modified routines of the javascript natural 
// compare by Kristof Coomans (it was easier to follow than the original C 
// version by Martin Pool), their versions were under the ZLIB license (which 
// is compatible with the GPL).
//
// Original Javascript version by Kristof Coomans
//      http://sourcefrog.net/projects/natsort/natcompare.js
//
// Do not contact the mentioned authors about my version.
wxInt32 NaturalCompareWorker(const wxString &String1, const wxString &String2)
{
    wxInt32 Direction = 0;

    wxChar String1Char, String2Char;

    for (wxUint32 String1Counter = 0, String2Counter = 0;
         String1.Len() > 0 && String2.Len() > 0; 
         ++String1Counter, ++String2Counter) 
    {
        String1Char = String1[String1Counter];
        String2Char = String2[String2Counter];

        if (!wxIsdigit(String1Char) && !wxIsdigit(String2Char)) 
        {
            return Direction;
        } 
        
        if (!wxIsdigit(String1Char)) 
        {
            return -1;
        } 
        
        if (!wxIsdigit(String2Char)) 
        {
            return 1;
        } 
        
        if (String1Char < String2Char) 
        {
            if (Direction == 0) 
            {
                Direction = -1;
            }
        } 
        
        if (String1Char > String2Char) 
        {
            if (Direction == 0)
            {
                Direction = 1;
            }
        } 
        
        if (String1Char == 0 && String2Char == 0) 
        {
            return Direction;
        }
    }
    
    return 0;
}

wxInt32 NaturalCompare(wxString String1, wxString String2, bool CaseSensitive = false) 
{
    wxInt32 StringCounter1 = 0, StringCounter2 = 0;
	wxInt32 String1Zeroes = 0, String2Zeroes = 0;
	wxChar String1Char, String2Char;
	wxInt32 Result;

    if (!CaseSensitive)
    {
        String1.MakeLower();
        String2.MakeLower();
    }

    while (true)
    {
        String1Zeroes = 0;
        String2Zeroes = 0;

        String1Char = String1[StringCounter1];
        String2Char = String2[StringCounter2];

        // skip past whitespace or zeroes in first string
        while (wxIsspace(String1Char) || String1Char == '0' ) 
        {
            if (String1Char == '0') 
            {
                String1Zeroes++;
            } 
            else 
            {
                String1Zeroes = 0;
            }

            String1Char = String1[++StringCounter1];
        }
        
        // skip past whitespace or zeroes in second string
        while (wxIsspace(String2Char) || String2Char == '0') 
        {
            if (String2Char == '0') 
            {
                String2Zeroes++;
            } 
            else 
            {
                String2Zeroes = 0;
            }

            String2Char = String2[++StringCounter2];
        }

        // We encountered some digits, compare these.
        if (wxIsdigit(String1Char) && wxIsdigit(String2Char)) 
        {
            if ((Result = NaturalCompareWorker(
                String1.Mid(StringCounter1), 
                String2.Mid(StringCounter2))) != 0) 
            {
                return Result;
            }
        }

        if ((String1Char == 0) && (String2Char == 0)) 
        {
            return (String1Zeroes - String2Zeroes);
        }

        if (String1Char < String2Char) 
        {
            return -1;
        } 
        else if (String1Char > String2Char) 
        {
            return 1;
        }

        ++StringCounter1; 
        ++StringCounter2;
    }
}

struct sortinfo_t
{
    wxInt32 SortOrder;
    wxInt32 SortCol;
    wxAdvancedListCtrl *ctrl;
};

int wxCALLBACK SortRoutine(long item1, long item2, long sortData) 
{    
    sortinfo_t *sortinfo = (sortinfo_t *)sortData;
    
    wxListItem lstitem1, lstitem2;

    lstitem1.SetId(item1);
    lstitem1.SetColumn(sortinfo->SortCol);
    lstitem1.SetMask(wxLIST_MASK_TEXT);
    sortinfo->ctrl->GetItem(lstitem1);

    lstitem2.SetId(item2);
    lstitem2.SetColumn(sortinfo->SortCol);
    lstitem2.SetMask(wxLIST_MASK_TEXT);
    sortinfo->ctrl->GetItem(lstitem2);
    
    if (sortinfo->SortOrder == 1) 
        return NaturalCompare(lstitem1.GetText(), lstitem2.GetText());

    return NaturalCompare(lstitem2.GetText(), lstitem1.GetText());
}

void wxAdvancedListCtrl::OnHeaderColumnButtonClick(wxListEvent &event)
{
    // invert sort order if need be (ascending/descending)
    if (SortCol != event.GetColumn())
        SortOrder = 1;
    else
        SortOrder = !SortOrder;
    
    // column that needs to be sorted, so the rest of the list
    // can be sorted by it
    SortCol = event.GetColumn();

    SetSortArrow(SortCol, SortOrder);

    // prime 'er up
    long item = this->GetNextItem(-1);
      
    while(item != -1) 
    {                    
        this->SetItemData(item, item); 
 
        item = this->GetNextItem(item);
    }

    // information for sorting routine callback
    sortinfo_t sortinfo;
    
    sortinfo.ctrl = (wxAdvancedListCtrl *)event.GetEventObject();
    sortinfo.SortCol = SortCol;
    sortinfo.SortOrder = SortOrder;
    
    // sort the list by column
    this->SortItems(SortRoutine, (long)&sortinfo);

    // recolour the list
    ColourList();
}

void wxAdvancedListCtrl::ResetSortArrows(void)
{
    wxListItem li;
    li.SetMask(wxLIST_MASK_IMAGE);    
    li.SetImage(-1);

    for (wxInt32 i = 0; i < GetColumnCount(); ++i)
    {
        SetColumn(i, li);        
    }
}

void wxAdvancedListCtrl::SetSortArrow(wxInt32 Column, wxInt32 ArrowState)
{
    // nuke any previously set sort arrows
    ResetSortArrows();

    wxListItem li;
    li.SetMask(wxLIST_MASK_IMAGE);
    li.SetImage(ArrowState);

    SetColumn(SortCol, li);
}

void wxAdvancedListCtrl::ColourListItem(wxInt32 item, wxInt32 grey)
{
    wxColour col = GetItemBackgroundColour(item);
    
    if ((col != wxColour(colRed, colGreen, colBlue)) &&
       (col != *wxWHITE))
    {
        return;
    }
    
    // light grey coolness
    if (grey)
        col.Set(colRed, colGreen, colBlue);
    else
        col.Set(255, 255, 255);

    // apply to index.
    this->SetItemBackgroundColour(item, col); 
}

// colour the previous item on insertion, it won't colour THIS item..?
void wxAdvancedListCtrl::OnItemInsert(wxListEvent &event)
{   
    // FIXME: Remove these from here, for performance reasons.
    ColourList();   
}

// recolour the entire list
void wxAdvancedListCtrl::ColourList()
{      
    for (wxInt32 i = 0; i < this->GetItemCount(); i++)
        ColourListItem(i, (i % 2));
}

// get an index location of the text field in the list
wxInt32 wxAdvancedListCtrl::GetIndex(wxString str)
{
	wxInt32 i = 0, list_index = -1;
	wxString addr = _T("");

    wxListItem listitem;

    if (!(str.IsEmpty()) && (this->GetItemCount() > 0))
	for (i = 0; i < this->GetItemCount(); i++)
	{
        listitem.SetId(i);
        listitem.SetColumn(7);
        listitem.SetMask(wxLIST_MASK_TEXT);

        this->GetItem(listitem);
        
        if (listitem.GetText().IsSameAs(str))
        {
            list_index = i;
            break;
        }
    }
	
	return list_index;
}
