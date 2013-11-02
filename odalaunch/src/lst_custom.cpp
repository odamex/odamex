// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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

#include <wx/settings.h>
#include <wx/defs.h>
#include <wx/regex.h>
#include <wx/renderer.h>

IMPLEMENT_DYNAMIC_CLASS(wxAdvancedListCtrl, wxListView)

BEGIN_EVENT_TABLE(wxAdvancedListCtrl, wxListView)
     EVT_LIST_COL_CLICK(-1, wxAdvancedListCtrl::OnHeaderColumnButtonClick)
     EVT_WINDOW_CREATE(wxAdvancedListCtrl::OnCreateControl)
END_EVENT_TABLE()

// Sort arrow
static int ImageList_SortArrowUp = -1;
static int ImageList_SortArrowDown = -1;

wxAdvancedListCtrl::wxAdvancedListCtrl()
{
    SortOrder = 0; 
    SortCol = 0; 

    m_SpecialColumn = -1;

    m_HeaderUsable = true;
}

void wxAdvancedListCtrl::OnCreateControl(wxWindowCreateEvent &event)
{
    ItemShade = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    BgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    
    // Set up the image list.
    AddImageSmall(wxNullImage);
}

// Add any additional bitmaps/icons to the internal image list
int wxAdvancedListCtrl::AddImageSmall(wxImage Image)
{
    if (GetImageList(wxIMAGE_LIST_SMALL) == NULL)
    {
        wxImageList *ImageList = new wxImageList(16, 16, true);
        AssignImageList(ImageList, wxIMAGE_LIST_SMALL);
        
        wxBitmap sort_up(16, 16), sort_down(16, 16);
        wxColour Mask = wxColour(255,255,255);
        
        // Draw sort arrows using the native renderer
        {
            wxMemoryDC renderer_dc;

             // sort arrow up
            renderer_dc.SelectObject(sort_up);
            renderer_dc.SetBackground(*wxTheBrushList->FindOrCreateBrush(Mask, wxSOLID));
            renderer_dc.Clear();
            wxRendererNative::Get().DrawHeaderButtonContents(this, renderer_dc, wxRect(0, 0, 16, 16), 0, wxHDR_SORT_ICON_UP);

             // sort arrow down
            renderer_dc.SelectObject(sort_down);
            renderer_dc.SetBackground(*wxTheBrushList->FindOrCreateBrush(Mask, wxSOLID));
            renderer_dc.Clear();
            wxRendererNative::Get().DrawHeaderButtonContents(this, renderer_dc, wxRect(0, 0, 16, 16), 0, wxHDR_SORT_ICON_DOWN);
        }

        // Add our sort icons to the image list
        ImageList_SortArrowDown = GetImageList(wxIMAGE_LIST_SMALL)->Add(sort_down, Mask);
        ImageList_SortArrowUp = GetImageList(wxIMAGE_LIST_SMALL)->Add(sort_up, Mask);
    }
    
    if (Image.IsOk())
    {
        return GetImageList(wxIMAGE_LIST_SMALL)->Add(Image);
    }
    
    return -1;
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

int wxCALLBACK wxCompareFunction(wxIntPtr item1, wxIntPtr item2, 
                                 wxIntPtr sortData)
{
    wxInt32 SortCol, SortOrder;
    wxListItem Item;
    wxString Str1, Str2;
    wxAdvancedListCtrl *ListCtrl;
    
    ListCtrl = (wxAdvancedListCtrl *)sortData; 
    
    ListCtrl->GetSortColumnAndOrder(SortCol, SortOrder);
    
    Item.SetColumn(SortCol);
    Item.SetMask(wxLIST_MASK_TEXT);
    
    if (SortCol == ListCtrl->GetSpecialSortColumn())
    {
        int Img1, Img2;
        
        Item.SetMask(wxLIST_MASK_IMAGE);
        
        Item.SetId(item1);
    
        ListCtrl->GetItem(Item);
    
        Img1 = Item.GetImage();
    
        Item.SetId(item2);
    
        ListCtrl->GetItem(Item);
    
        Img2 = Item.GetImage();
        
        return SortOrder ? Img2 - Img1 : Img1 - Img2;
    }
    
    Item.SetId(item1);
    
    ListCtrl->GetItem(Item);
    
    Str1 = Item.GetText();
    
    Item.SetId(item2);
    
    ListCtrl->GetItem(Item);
    
    Str2 = Item.GetText();
    
    return SortOrder ? NaturalCompare(Str1, Str2) : NaturalCompare(Str2, Str1);
}

void wxAdvancedListCtrl::Sort()
{
    SetSortArrow(SortCol, SortOrder);
    
    long itemid = GetNextItem(-1);

    // prime 'er up
    while (itemid != -1)
    {                        
        SetItemData(itemid, itemid);
        
        itemid = GetNextItem(itemid);
    }

    SortItems(wxCompareFunction, (wxIntPtr)this);

    ColourList();

    return;
}

void wxAdvancedListCtrl::OnHeaderColumnButtonClick(wxListEvent &event)
{
    if (!m_HeaderUsable)
        return;

    // invert sort order if need be (ascending/descending)
    if (SortCol != event.GetColumn())
        SortOrder = 1;
    else
        SortOrder = !SortOrder;
    
    // column that needs to be sorted, so the rest of the list
    // can be sorted by it
    SortCol = event.GetColumn();

    Sort();
}

void wxAdvancedListCtrl::ResetSortArrows(void)
{
    for (wxInt32 i = 0; i < GetColumnCount(); ++i)
    {
        ClearColumnImage(i);
    }
}

void wxAdvancedListCtrl::SetSortArrow(wxInt32 Column, wxInt32 ArrowState)
{
    // nuke any previously set sort arrows
    ResetSortArrows();

    SetColumnImage(SortCol, ArrowState);
}

void wxAdvancedListCtrl::ColourListItem(wxListItem &info)
{
    static bool SwapColour = false;
    
    wxColour col;
    wxListItemAttr *ListItemAttr = info.GetAttributes();
    
    // Don't change a background colour we didn't set.
    if (ListItemAttr && ListItemAttr->HasBackgroundColour())
    {
        return;
    }
    
    // light grey coolness
    if (SwapColour)
        col = ItemShade;
    else
        col = BgColor;

    SwapColour = !SwapColour;

    info.SetBackgroundColour(col);
}

void wxAdvancedListCtrl::ColourListItem(long item)
{
    wxListItem ListItem;
    ListItem.SetId(item);
    
    ColourListItem(ListItem);
    
    SetItem(ListItem);
}

// recolour the entire list
void wxAdvancedListCtrl::ColourList()
{      
    for (long i = 0; i < GetItemCount(); ++i)
    {
        ColourListItem(i);
    }
}

// Our variation of InsertItem, so we can do magical things!
long wxAdvancedListCtrl::ALCInsertItem(const wxString &Text)
{
    wxListItem ListItem;
    
    ListItem.m_itemId = InsertItem(GetItemCount(), Text, -1);

    ColourListItem(ListItem.m_itemId);

    SetItem(ListItem);

    // wxWidgets bug: Required for sorting colours correctly
    SetItemTextColour(ListItem.m_itemId, GetTextColour());

    return ListItem.m_itemId;
}
