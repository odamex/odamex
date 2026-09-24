// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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


#include "lst_custom.h"

#include <sstream>

#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/defs.h>
#include <wx/regex.h>

#ifdef __WXMSW__
#include <wx/msw/wrapcctl.h>

#ifndef LVCFMT_FIXED_WIDTH
#define LVCFMT_FIXED_WIDTH 0x100
#endif
#endif

IMPLEMENT_DYNAMIC_CLASS(wxAdvancedListCtrl, wxListView)

BEGIN_EVENT_TABLE(wxAdvancedListCtrl, wxListView)
	EVT_LIST_COL_CLICK(-1, wxAdvancedListCtrl::OnHeaderColumnButtonClick)
	EVT_LIST_COL_BEGIN_DRAG(-1, wxAdvancedListCtrl::OnHeaderColumnBeginResize)
	EVT_WINDOW_CREATE(wxAdvancedListCtrl::OnCreateControl)
END_EVENT_TABLE()

// Sort arrow
static int ImageList_SortArrowUp = -1;
static int ImageList_SortArrowDown = -1;

// Width of the icon column, sized so the 16px sort arrow image is centred in
// its header. An unlabelled column's header image is inset from the left edge:
// by the native MSW header's 6px bitmap margin, or by 12px in the generic list
// control (GTK, macOS).
#ifdef __WXMSW__
constexpr int ICON_COLUMN_WIDTH = 6 + 16 + 6;
#else
constexpr int ICON_COLUMN_WIDTH = 12 + 16 + 12;
#endif

void DrawSortArrowBitmap(wxBitmap& Bitmap, const wxColour& Mask, bool Up)
{
	// The same 8x4 arrow as wxRendererGeneric::DrawHeaderButtonContents().
	const wxPoint UpArrow[3] = {
		wxPoint(7, 6), wxPoint(11, 10), wxPoint(3, 10)
	};
	const wxPoint DownArrow[3] = {
		wxPoint(3, 6), wxPoint(11, 6), wxPoint(7, 10)
	};

	// Make button light for dark theme and vice versa
	const wxColour Face = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	const wxColour Text = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
	const double Mix = 1.0 / 3.0;

	const wxColour Color(wxColour::AlphaBlend(Text.Red(), Face.Red(), Mix),
	                     wxColour::AlphaBlend(Text.Green(), Face.Green(), Mix),
	                     wxColour::AlphaBlend(Text.Blue(), Face.Blue(), Mix));

	wxMemoryDC dc(Bitmap);
	dc.SetBackground(*wxTheBrushList->FindOrCreateBrush(Mask, wxBRUSHSTYLE_SOLID));
	dc.Clear();

	dc.SetPen(wxPen(Color));
	dc.SetBrush(wxBrush(Color));
	dc.DrawPolygon(3, Up ? UpArrow : DownArrow);
}

wxAdvancedListCtrl::wxAdvancedListCtrl()
{
	SortOrder = 0;
	SortCol = 0;

	m_SpecialColumn = -1;
	m_IconColumn = -1;

	m_HeaderUsable = true;

	// Set here rather than on creation, as on GTK the create event isn't sent
	// until the control is first shown, which can be after it's been filled.
	ItemShade = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	BgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
}

void wxAdvancedListCtrl::OnCreateControl(wxWindowCreateEvent& event)
{
	// Set up the image list.
	AddImageSmall(wxNullImage);
}

void wxAdvancedListCtrl::InsertIconColumn(long Column)
{
	InsertColumn(Column, "", wxLIST_FORMAT_LEFT, ICON_COLUMN_WIDTH);

	m_IconColumn = Column;

	#ifdef __WXMSW__
	// Fix it natively as well, so the header doesn't show a resize cursor
	LVCOLUMN lvc = {};
	lvc.mask = LVCF_FMT;

	if(ListView_GetColumn(GetHwnd(), Column, &lvc))
	{
		lvc.fmt |= LVCFMT_FIXED_WIDTH;
		ListView_SetColumn(GetHwnd(), Column, &lvc);
	}
	#endif
}

bool wxAdvancedListCtrl::SetColumnWidth(int col, int width)
{
	// Keeps the icon column's width fixed, including when the generic header
	// autosizes a column on a double-click of its divider
	if(col == m_IconColumn)
		width = ICON_COLUMN_WIDTH;

	return wxListView::SetColumnWidth(col, width);
}

void wxAdvancedListCtrl::OnHeaderColumnBeginResize(wxListEvent& event)
{
	if(event.GetColumn() == m_IconColumn)
		event.Veto();
	else
		event.Skip();
}

// Hack to set the background color of the icon column
// to not have the alternating colors bleed in
#ifdef __WXMSW__
bool wxAdvancedListCtrl::MSWOnNotify(int idCtrl, WXLPARAM lParam,
                                     WXLPARAM* result)
{
	const bool Processed = wxListView::MSWOnNotify(idCtrl, lParam, result);

	const NMHDR* Header = reinterpret_cast<const NMHDR*>(lParam);

	if(Header->hwndFrom == GetHwnd() && Header->code == NM_CUSTOMDRAW)
	{
		NMLVCUSTOMDRAW* Draw = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);

		if(Draw->nmcd.dwDrawStage == (CDDS_SUBITEM | CDDS_ITEMPREPAINT) &&
		   Draw->iSubItem == m_IconColumn)
		{
			Draw->clrTextBk = RGB(BgColor.Red(), BgColor.Green(), BgColor.Blue());
		}
	}

	return Processed;
}
#endif

// Add any additional bitmaps/icons to the internal image list
int wxAdvancedListCtrl::AddImageSmall(wxImage Image)
{
	#if !ODALAUNCH_USE_LEGACY_IMAGELIST
	if (m_Images.empty())
	{
	#else
	if(GetImageList(wxIMAGE_LIST_SMALL) == nullptr)
	{
		wxImageList* ImageList = new wxImageList(16, 16, true);
		AssignImageList(ImageList, wxIMAGE_LIST_SMALL);
	#endif

		wxBitmap sort_up(16, 16), sort_down(16, 16);
		#if wxCHECK_VERSION(3, 2, 6)
			wxColour Mask = wxSystemSettingsNative::SelectLightDark(wxColour(255, 255, 255), wxColour(0, 0, 0));
		#else
			wxColour Mask = wxColour(255, 255, 255);
		#endif

		DrawSortArrowBitmap(sort_up, Mask, true);
		DrawSortArrowBitmap(sort_down, Mask, false);

		// Add our sort icons to the image list
		#if !ODALAUNCH_USE_LEGACY_IMAGELIST
			sort_up.SetMask(new wxMask(sort_up, Mask));
			sort_down.SetMask(new wxMask(sort_down, Mask));
			ImageList_SortArrowDown = m_Images.size();
			m_Images.push_back(sort_down);
			ImageList_SortArrowUp = m_Images.size();
			m_Images.push_back(sort_up);
			SetSmallImages(m_Images);
		#else
			ImageList_SortArrowDown = GetImageList(wxIMAGE_LIST_SMALL)->Add(sort_down, Mask);
			ImageList_SortArrowUp = GetImageList(wxIMAGE_LIST_SMALL)->Add(sort_up, Mask);
		#endif
	}

	if(Image.IsOk())
	{
		#if !ODALAUNCH_USE_LEGACY_IMAGELIST
			m_Images.push_back(wxBitmapBundle(Image));
			SetSmallImages(m_Images);
			return m_Images.size() - 1;
		#else
			return GetImageList(wxIMAGE_LIST_SMALL)->Add(Image);
		#endif
	}

	return -1;
}

// Removes all images from the image list, except for the ones the control has
// created internally
void wxAdvancedListCtrl::ClearImageList()
{
	#if !ODALAUNCH_USE_LEGACY_IMAGELIST
		if (!m_Images.empty())
		{
			m_Images.resize(ImageList_SortArrowUp + 1);
			SetSmallImages(m_Images);
		}

	#else
		wxImageList* ImageList = GetImageList(wxIMAGE_LIST_SMALL);

		if(!ImageList)
			return;

		// Hack: The start of this classes non-added images begin after the sort
		// arrows
		for(int i = ImageList_SortArrowUp + 1; i < ImageList->GetImageCount(); ++i)
			ImageList->Remove(i);
	#endif
}

//  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
//  Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.

// [AM] The original implementation assumed C strings, so we use this so
//      we don't blow anything up.
static wxUniChar SafeAt(const wxString& string, size_t index)
{
	if (index >= string.Len())
		return '\0';
	return string.at(index);
}

/**
 * @brief Unicode-aware isspace.
 *
 * @detail This can't be complete in such a short space, so instead I borrowed
 *         Golang's definition of a space in the standard Latin-1 space.
 *
 * @param ch Character to check.
 * @return If the passed character is a space.
*/
static bool IsSpace(wxUniChar ch)
{
	wxUniChar::value_type cp = ch.GetValue();
	return cp == '\t' || cp == '\n' || cp == '\v' || cp == '\f' || cp == '\r' ||
	       cp == ' ' || cp == 0x0085 /* NEL */ || cp == 0x00A0 /* NBSP */;
}

/**
 * @brief Unicode-aware isdigit.
 *
 * @detail This can't be complete in such a short space, so instead I borrowed
 *         Golang's definition of a digit in the standard Latin-1 space.
 *
 * @param ch Character to check.
 * @return If the passed character is a space.
 */
static bool IsDigit(wxUniChar ch)
{
	wxUniChar::value_type cp = ch.GetValue();
	return cp >= '0' && cp <= '9';
}

static wxInt32 NaturalCompareLeft(const wxString& aString, size_t aIndex,
                                  const wxString& bString, size_t bIndex)
{
	// Compare two left-aligned numbers: the first to have a
	// different value wins.
	for (;; aIndex++, bIndex++)
	{
		wxUniChar aChar = SafeAt(aString, aIndex);
		wxUniChar bChar = SafeAt(bString, bIndex);

		if (!IsDigit(aChar) && !IsDigit(bChar))
			return 0;
		if (!IsDigit(aChar))
			return -1;
		if (!IsDigit(bChar))
			return +1;
		if (aChar < bChar)
			return -1;
		if (aChar > bChar)
			return +1;
	}

	return 0;
}

static wxInt32 NaturalCompareRight(const wxString& aString, size_t aIndex,
                                   const wxString& bString, size_t bIndex)
{
	int bias = 0;

	// The longest run of digits wins.  That aside, the greatest
	// value wins, but we can't know that it will until we've scanned
	// both numbers to know that they have the same magnitude, so we
	// remember it in BIAS.
	for (;; aIndex++, bIndex++)
	{
		wxUniChar aChar = SafeAt(aString, aIndex);
		wxUniChar bChar = SafeAt(bString, bIndex);

		if (!IsDigit(aChar) && !IsDigit(bChar))
			return bias;
		if (!IsDigit(aChar))
			return -1;
		if (!IsDigit(bChar))
			return +1;
		if (aChar < bChar)
		{
			if (!bias)
				bias = -1;
		}
		else if (aChar > bChar)
		{
			if (!bias)
				bias = +1;
		}
		else if (aChar == '\0' && bChar == '\0')
			return bias;
	}

	return 0;
}

wxInt32 NaturalCompare(wxString aString, wxString bString, bool CaseSensitive = false)
{
	if (!CaseSensitive)
	{
		aString.MakeLower();
		bString.MakeLower();
	}

	size_t aIndex = 0, bIndex = 0;
	wxInt32 result = 0;

	for (;;)
	{
		wxUniChar aChar = SafeAt(aString, aIndex);
		wxUniChar bChar = SafeAt(bString, bIndex);

		// skip over leading spaces or zeros
		while (IsSpace(aChar))
		{
			aIndex += 1;
			aChar = SafeAt(aString, aIndex);
		}

		while (IsSpace(bChar))
		{
			bIndex += 1;
			bChar = SafeAt(bString, bIndex);
		}

		// process run of digits
		if (IsDigit(aChar) && IsDigit(bChar))
		{
			bool fractional = (aChar.GetValue() == '0' || bChar.GetValue() == '0');

			// [AM] String views would really help here.
			if (fractional)
			{
				if ((result = NaturalCompareLeft(aString, aIndex, bString, bIndex)) != 0)
					return result;
			}
			else
			{
				if ((result = NaturalCompareRight(aString, aIndex, bString, bIndex)) != 0)
					return result;
			}
		}

		if (aChar == '\0' && bChar == '\0')
		{
			// The strings compare the same.  Perhaps the caller
			// will want to call strcmp to break the tie.
			return 0;
		}

		if (aChar < bChar)
			return -1;

		if (aChar > bChar)
			return +1;

		aIndex += 1;
		bIndex += 1;
	}
}

int wxCALLBACK wxCompareFunction(wxIntPtr item1, wxIntPtr item2,
                                 wxIntPtr sortData)
{
	wxInt32 SortCol, SortOrder;
	auto* ListCtrl = reinterpret_cast<wxAdvancedListCtrl*>(sortData);
	ListCtrl->GetSortColumnAndOrder(SortCol, SortOrder);

	const auto& key1 = ListCtrl->m_sortData[static_cast<size_t>(item1)];
	const auto& key2 = ListCtrl->m_sortData[static_cast<size_t>(item2)];

	// Bottom-pinned rows always sink below the rest, independent of sort order.
	if(key1.bottom != key2.bottom)
		return key1.bottom ? 1 : -1;

	if(SortCol == ListCtrl->GetSpecialSortColumn())
		return SortOrder ? key2.image - key1.image : key1.image - key2.image;
	else
		return SortOrder ? NaturalCompare(key1.text, key2.text) : NaturalCompare(key2.text, key1.text);
}

void wxAdvancedListCtrl::Sort()
{
	SetSortArrow(SortCol, SortOrder);
	wxListItem Item;
	Item.SetColumn(SortCol);
	if(SortCol == GetSpecialSortColumn())
		Item.SetMask(wxLIST_MASK_IMAGE);
	else
		Item.SetMask(wxLIST_MASK_TEXT);

	long itemid = GetNextItem(-1);

	// prime 'er up
	while(itemid != -1)
	{
		Item.SetId(itemid);
		GetItem(Item);
		auto& sortkey = m_sortData.emplace_back();
		sortkey.bottom = IsSortedToBottom(itemid);
		if(SortCol == GetSpecialSortColumn())
			sortkey.image = Item.GetImage();
		else
			sortkey.text = Item.GetText();

		SetItemData(itemid, static_cast<long>(m_sortData.size() - 1));

		itemid = GetNextItem(itemid);
	}

	SortItems(wxCompareFunction, reinterpret_cast<wxIntPtr>(this));

	m_sortData.clear();

	ColourList();
}

void wxAdvancedListCtrl::OnHeaderColumnButtonClick(wxListEvent& event)
{
	if(!m_HeaderUsable)
		return;

	// invert sort order if need be (ascending/descending)
	if(SortCol != event.GetColumn())
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
	for(wxInt32 i = 0; i < GetColumnCount(); ++i)
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

void wxAdvancedListCtrl::ColourListItem(wxListItem& info)
{
	static bool SwapColour = false;

	wxColour col;
	wxListItemAttr* ListItemAttr = info.GetAttributes();

	// Don't change a background colour we didn't set.
	if(ListItemAttr && ListItemAttr->HasBackgroundColour())
	{
		return;
	}

	// light grey coolness
	if(SwapColour)
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
	for(long i = 0; i < GetItemCount(); ++i)
	{
		ColourListItem(i);
	}
}

// Our variation of InsertItem, so we can do magical things!
long wxAdvancedListCtrl::ALCInsertItem(const wxString& Text)
{
	wxListItem ListItem;

	ListItem.m_itemId = InsertItem(GetItemCount(), Text, -1);

	ColourListItem(ListItem.m_itemId);

	SetItem(ListItem);

	// wxWidgets bug: Required for sorting colours correctly
	SetItemTextColour(ListItem.m_itemId, GetTextColour());

	return ListItem.m_itemId;
}

// Back up the entire list for filtering
void wxAdvancedListCtrl::BackupList()
{
	size_t BackupItemsCount = GetItemCount();
	size_t BackupColumnCount = GetColumnCount();

	BackupItems.clear();

	if(!BackupItemsCount || !BackupColumnCount)
		return;

	BackupItems.resize(BackupItemsCount);

	for(size_t x = 0; x < BackupItemsCount; ++x)
	{
		wxListItem Item;

		Item.SetId(x);

		for(size_t y = 0; y < BackupColumnCount; ++y)
		{
			Item.SetColumn(y);

			Item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_DATA | wxLIST_MASK_IMAGE
			             | wxLIST_MASK_STATE | wxLIST_MASK_WIDTH | wxLIST_MASK_FORMAT);

			GetItem(Item);

			BackupItems[x].push_back(Item);
		}
	}
}

// Reloads everything into the current row
void wxAdvancedListCtrl::DoRestoreRow(size_t row)
{
	long id = InsertItem(row, "");

	for(size_t y = 0; y < BackupItems[row].size(); ++y)
	{
		wxListItem Item = BackupItems[row][y];

		Item.SetId(id);

		SetItem(Item);
	}
}

wxString CreateFilter(wxString s)
{
	wxString Result;

	if(s.IsEmpty())
		return "";

	s.Prepend("*");

	// Uppercase
	s = s.Upper();

	// Replace whitespace with kleene stars for better matching
	s.Replace(' ', '*');

	s += "*";

	return s;
}



void wxAdvancedListCtrl::DoApplyFilter(const wxString& Filter)
{
	wxString FlatColumn;
	wxString FilterReversed;
	size_t xSize = BackupItems.size();
	size_t ySize;

	DeleteAllItems();

	for(size_t x = 0; x < xSize; ++x)
	{
		ySize = BackupItems[x].size();

		for(size_t y = 0; y < ySize; ++y)
		{
			// Creates a tokenized list of all the strings in individual columns
			FlatColumn += BackupItems[x][y].GetText().Upper().Trim(false).Trim(true);
			FlatColumn += ' ';
		}

		if(FlatColumn.Matches(Filter))
		{
			DoRestoreRow(x);
		}

		FlatColumn.Empty();
	}
}

// Restores the entire list if the filter is empty
void wxAdvancedListCtrl::RestoreList()
{
	if(BackupItems.empty())
		return;

	DeleteAllItems();

	for(size_t x = 0; x < BackupItems.size(); ++x)
	{
		InsertItem(x, "");

		for(size_t y = 0; y < BackupItems[x].size(); ++y)
		{
			SetItem(BackupItems[x][y]);
		}
	}
}

void wxAdvancedListCtrl::ApplyFilter(wxString Filter)
{
	// Lock the control so searches are faster
	Freeze();

	// Restore list from last search result
	RestoreList();

	// Backup list from last search result
	BackupList();

	// Replace all non alphanumerics with ?, also prepend/append a *
	Filter = ::CreateFilter(Filter);

	// Run the filtering
	if(Filter != wxEmptyString)
		DoApplyFilter(Filter);

	Sort();

	Thaw();
}
