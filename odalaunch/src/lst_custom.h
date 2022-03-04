// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//  Custom list control, featuring sorting
//  AUTHOR: Russell Rice
//
//-----------------------------------------------------------------------------


#ifndef LST_CUSTOM_H
#define LST_CUSTOM_H

#include "odalaunch.h"

#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/image.h>
#include <wx/imaglist.h>

#include <vector>

class wxAdvancedListCtrl : public wxListView
{
public:
	wxAdvancedListCtrl();
	virtual ~wxAdvancedListCtrl() { };

	void HeaderUsable(bool state)
	{
		m_HeaderUsable = state;
	}

	void SetSortColumnAndOrder(wxInt32& Column, wxInt32& Order)
	{
		SortCol = Column;
		SortOrder = Order;

		SetSortArrow(SortCol, SortOrder);
	}

	void GetSortColumnAndOrder(wxInt32& Column, wxInt32& Order)
	{
		Column = SortCol;
		Order = SortOrder;
	}

	void SetSortColumnIsSpecial(const wxInt32& Column)
	{
		m_SpecialColumn = Column;
	}

	wxInt32 GetSpecialSortColumn()
	{
		return m_SpecialColumn;
	}

	void Sort();

	int AddImageSmall(wxImage Image);
	void ClearImageList();
	long ALCInsertItem(const wxString& Text = "");

	void ApplyFilter(wxString Filter = wxEmptyString);

	wxEvent* Clone();

private:
	void OnCreateControl(wxWindowCreateEvent& event);
	void OnHeaderColumnButtonClick(wxListEvent& event);

	void ColourList();
	void ColourListItem(wxListItem& info);
	void ColourListItem(long item);

	void ResetSortArrows();
	void SetSortArrow(wxInt32 Column, wxInt32 ArrowState);

	void FlipRow(long Row, long NextRow);
	void Sort(wxInt32 Column, wxInt32 Order = 0, wxInt32 Lowest = 0, wxInt32 Highest = -1);

	// Filtering functions
	void BackupList();
	void DoRestoreRow(size_t row);
	void DoApplyFilter(const wxString&);
	void RestoreList();

	wxInt32 SortOrder;
	wxInt32 SortCol;

	wxColour ItemShade;
	wxColour BgColor;

	wxInt32 m_SpecialColumn;

	bool m_HeaderUsable;

	std::vector<std::vector<wxListItem> > BackupItems;
protected:
	DECLARE_DYNAMIC_CLASS(wxAdvancedListCtrl)
	DECLARE_EVENT_TABLE()
};

#endif
