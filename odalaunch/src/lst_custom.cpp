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
END_EVENT_TABLE()

struct sortinfo_t
{
    wxInt32 SortOrder;
    wxInt32 SortCol;
    wxAdvancedListCtrl *ctrl;
};

// Is this string an ip?
bool IsIPAddressAndPort(wxString Address)
{
    // Our addresses are atmost 21 chars (eg 255.255.255.255:65535)
    if (Address.Len() > 21)
        return false;
        
    wxUint32 IP1, IP2, IP3, IP4, Port;
    
    wxSscanf(Address.c_str(), _T("%u.%u.%u.%u:%u"), &IP1, &IP2, &IP3, &IP4, &Port);
             
    // we compare chars, must be digits
    if ((IP1 > 255) || (IP2 > 255) || (IP3 > 255) || (IP4 > 255) || (Port > 65535))
        return false;
    
    return true;
}

wxInt32 CompareIPAddressAndPort(wxString Address1, wxString Address2)
{
    wxUint32 Addr1IP1, Addr1IP2, Addr1IP3, Addr1IP4, Addr1Port;
    wxUint32 Addr2IP1, Addr2IP2, Addr2IP3, Addr2IP4, Addr2Port;

    wxSscanf(Address1.c_str(), 
             _T("%u.%u.%u.%u:%u"), 
             &Addr1IP1, 
             &Addr1IP2, 
             &Addr1IP3, 
             &Addr1IP4,
             &Addr1Port);

    wxSscanf(Address2.c_str(), 
             _T("%u.%u.%u.%u:%u"), 
             &Addr2IP1, 
             &Addr2IP2, 
             &Addr2IP3, 
             &Addr2IP4,
             &Addr2Port);
             
    if (Addr1IP1 > Addr2IP1)
        return 1;
    else if (Addr1IP1 < Addr2IP1)
        return -1;

    if (Addr1IP2 > Addr2IP2)
        return 1;
    else if (Addr1IP2 < Addr2IP2)
        return -1;

    if (Addr1IP3 > Addr2IP3)
        return 1;
    else if (Addr1IP3 < Addr2IP3)
        return -1;

    if (Addr1IP4 > Addr2IP4)
        return 1;
    else if (Addr1IP4 < Addr2IP4)
        return -1;
        
    if (Addr1Port > Addr2Port)
        return 1;
    else if (Addr1Port < Addr2Port)
        return -1;
        
    return 0;
}

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
    
    wxString item1str = lstitem1.GetText();
    wxString item2str = lstitem2.GetText();

    // IP address detection and comparison
    if (IsIPAddressAndPort(item1str) && IsIPAddressAndPort(item2str))
    {
        if (sortinfo->SortOrder == 1) 
            return CompareIPAddressAndPort(item1str, item2str);
        
        return CompareIPAddressAndPort(item2str, item1str);
    }
    
    if (sortinfo->SortOrder == 1) 
        return item1str.CmpNoCase(item2str);
    
    return item2str.CmpNoCase(item1str);
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
