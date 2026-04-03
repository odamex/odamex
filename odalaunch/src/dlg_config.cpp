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
//  Config dialog
//  AUTHOR: Russell Rice, John D Corrado
//
//-----------------------------------------------------------------------------


#include "net_packet.h"
#include "dlg_config.h"
#include "oda_defs.h"
#include "plat_utils.h"

#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/recguard.h>

#include "main.h"

// Event table for widgets
BEGIN_EVENT_TABLE(dlgConfig,wxDialog)

	EVT_BUTTON(wxID_OK, dlgConfig::OnOK)

	// Picker events
	EVT_DIRPICKER_CHANGED(XRCID("Id_DirCtrlChooseOdamexPath"), dlgConfig::OnFileDirChange)
	EVT_FILEPICKER_CHANGED(XRCID("Id_FilePickSoundFile"), dlgConfig::OnFileDirChange)
	EVT_COLOURPICKER_CHANGED(XRCID("Id_ClrPickServerLineHighlighter"), dlgConfig::OnClrPickerChange)
    EVT_COLOURPICKER_CHANGED(XRCID("Id_ClrPickCustomServerHighlight"), dlgConfig::OnClrPickerChange)

	// Misc events
	EVT_CHECKBOX(XRCID("Id_ChkCtrlGetListOnStart"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkCtrlShowBlockedServers"), dlgConfig::OnCheckedBox)
	//EVT_CHECKBOX(XRCID("Id_ChkCtrlCheckForUpdates"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkCtrlEnableBroadcasts"), dlgConfig::OnCheckedBox)
	//EVT_CHECKBOX(XRCID("Id_ChkCtrlLoadChatOnStart"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkFlashTaskbar"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkSystemBeep"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkPlaySound"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkColorServerLine"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkColorCustomServers"), dlgConfig::OnCheckedBox)
	EVT_CHECKBOX(XRCID("Id_ChkAutoRefresh"), dlgConfig::OnCheckedBox)

	EVT_NOTEBOOK_PAGE_CHANGED(XRCID("Id_Notebook"), dlgConfig::OnNotebookPageChanged)

	EVT_SPINCTRL(XRCID("Id_SpnCtrlMasterTimeout"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlServerTimeout"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlRetry"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlThreadMul"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlThreadMax"), dlgConfig::OnSpinValChange)

	EVT_TEXT(XRCID("Id_TxtCtrlExtraCmdLineArgs"), dlgConfig::OnTextChange)

	EVT_SPINCTRL(XRCID("Id_SpnCtrlPQGood"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlPQPlayable"), dlgConfig::OnSpinValChange)
	EVT_SPINCTRL(XRCID("Id_SpnCtrlPQLaggy"), dlgConfig::OnSpinValChange)

	EVT_SPINCTRL(XRCID("Id_SpnRefreshInterval"), dlgConfig::OnSpinValChange)

END_EVENT_TABLE()

// Window constructor
dlgConfig::dlgConfig(wxWindow* parent, wxWindowID id) :
	m_Notebook(nullptr)
{
	// Set up the dialog and its widgets
	wxXmlResource::Get()->LoadDialog(this, parent, "dlgConfig");

	m_ChkCtrlGetListOnStart = XRCCTRL(*this, "Id_ChkCtrlGetListOnStart", wxCheckBox);
	m_ChkCtrlShowBlockedServers = XRCCTRL(*this, "Id_ChkCtrlShowBlockedServers", wxCheckBox);
	m_ChkCtrlEnableBroadcasts = XRCCTRL(*this, "Id_ChkCtrlEnableBroadcasts", wxCheckBox);
	m_ChkCtrlFlashTaskBar = XRCCTRL(*this, "Id_ChkFlashTaskbar", wxCheckBox);
	m_ChkCtrlPlaySystemBeep = XRCCTRL(*this, "Id_ChkSystemBeep", wxCheckBox);
	m_ChkCtrlPlaySoundFile = XRCCTRL(*this, "Id_ChkPlaySound", wxCheckBox);
	m_ChkCtrlHighlightServerLines = XRCCTRL(*this, "Id_ChkColorServerLine", wxCheckBox);
	m_ChkCtrlHighlightCustomServers = XRCCTRL(*this, "Id_ChkColorCustomServers", wxCheckBox);
	m_ChkCtrlkAutoServerRefresh = XRCCTRL(*this, "Id_ChkAutoRefresh", wxCheckBox);

	m_DirCtrlChooseOdamexPath = XRCCTRL(*this, "Id_DirCtrlChooseOdamexPath", wxDirPickerCtrl);
	m_FilePickCtrlSoundFile = XRCCTRL(*this, "Id_FilePickSoundFile", wxFilePickerCtrl);
	m_ClrPickServerLineHighlighter = XRCCTRL(*this, "Id_ClrPickServerLineHighlighter",
	                                 wxColourPickerCtrl);
    m_ClrPickCustomServerHighlight = XRCCTRL(*this, "Id_ClrPickCustomServerHighlight",
	                                 wxColourPickerCtrl);

    m_SpnCtrlThreadMul = XRCCTRL(*this, "Id_SpnCtrlThreadMul", wxSpinCtrl);
    m_SpnCtrlThreadMax = XRCCTRL(*this, "Id_SpnCtrlThreadMax", wxSpinCtrl);
	m_SpnCtrlMasterTimeout = XRCCTRL(*this, "Id_SpnCtrlMasterTimeout", wxSpinCtrl);
	m_SpnCtrlServerTimeout = XRCCTRL(*this, "Id_SpnCtrlServerTimeout", wxSpinCtrl);
	m_SpnCtrlRetry = XRCCTRL(*this, "Id_SpnCtrlRetry", wxSpinCtrl);

	m_SpnRefreshInterval = XRCCTRL(*this, "Id_SpnRefreshInterval", wxSpinCtrl);

	m_TxtCtrlExtraCmdLineArgs = XRCCTRL(*this, "Id_TxtCtrlExtraCmdLineArgs", wxTextCtrl);

	m_SpnCtrlPQGood = XRCCTRL(*this, "Id_SpnCtrlPQGood", wxSpinCtrl);
	m_SpnCtrlPQPlayable = XRCCTRL(*this, "Id_SpnCtrlPQPlayable", wxSpinCtrl);
	m_SpnCtrlPQLaggy = XRCCTRL(*this, "Id_SpnCtrlPQLaggy", wxSpinCtrl);

	m_StcBmpPQGood = XRCCTRL(*this, "Id_StcBmpPQGood", wxStaticBitmap);
	m_StcBmpPQPlayable = XRCCTRL(*this, "Id_StcBmpPQPlayable", wxStaticBitmap);
	m_StcBmpPQLaggy = XRCCTRL(*this, "Id_StcBmpPQLaggy", wxStaticBitmap);
	m_StcBmpPQBad = XRCCTRL(*this, "Id_StcBmpPQBad", wxStaticBitmap);

	// Ping quality icons
	m_StcBmpPQGood->SetBitmap(wxXmlResource::Get()->LoadBitmap("bullet_green"));
	m_StcBmpPQPlayable->SetBitmap(wxXmlResource::Get()->LoadBitmap("bullet_orange"));
	m_StcBmpPQLaggy->SetBitmap(wxXmlResource::Get()->LoadBitmap("bullet_red"));
	m_StcBmpPQBad->SetBitmap(wxXmlResource::Get()->LoadBitmap("bullet_gray"));
}

// Window destructor
dlgConfig::~dlgConfig()
{

}

void dlgConfig::Show()
{
	LoadSettings();

	UserChangedSetting = false;

	// Queue notebook page changed event
	m_Notebook = XRCCTRL(*this, "Id_Notebook", wxNotebook);
	wxBookCtrlEvent* event = new wxBookCtrlEvent(wxEVT_NOTEBOOK_PAGE_CHANGED, m_Notebook->GetId());
	event->SetSelection(m_Notebook->GetSelection());
	QueueEvent(event);

	ShowModal();
}

void dlgConfig::OnCheckedBox(wxCommandEvent& event)
{
	UserChangedSetting = true;
}

void dlgConfig::OnFileDirChange(wxFileDirPickerEvent& event)
{
	UserChangedSetting = true;
}

void dlgConfig::OnClrPickerChange(wxColourPickerEvent& event)
{
	UserChangedSetting = true;
}

// Ping quality spin control changes
void dlgConfig::OnSpinValChange(wxSpinEvent& event)
{
	wxInt32 PQGood, PQPlayable, PQLaggy;

	PQGood = m_SpnCtrlPQGood->GetValue();
	PQPlayable = m_SpnCtrlPQPlayable->GetValue();
	PQLaggy = m_SpnCtrlPQLaggy->GetValue();

	// Ensure spin control values are at least greater than one another

	if(PQGood >= PQPlayable)
	{
		PQPlayable = PQGood + 1;
		m_SpnCtrlPQPlayable->SetValue(PQPlayable);
	}

	if(PQPlayable >= PQLaggy)
	{
		PQLaggy = PQPlayable + 1;
		m_SpnCtrlPQLaggy->SetValue(PQLaggy);
	}

	UserChangedSetting = true;
}

// User pressed ok button
void dlgConfig::OnOK(wxCommandEvent& event)
{
	wxMessageDialog msgdlg(this, "Save settings?", "Save settings?",
	                       wxYES_NO | wxICON_QUESTION | wxSTAY_ON_TOP);

	if(UserChangedSetting == false)
	{
		Close();

		return;
	}

	if(msgdlg.ShowModal() == wxID_YES)
	{
		// reset 'dirty' flag
		UserChangedSetting = false;

		// Save settings to configuration file
		SaveSettings();
	}
	else
		UserChangedSetting = false;

	// Close window
	Close();
}

void dlgConfig::OnTextChange(wxCommandEvent& event)
{
	UserChangedSetting = true;
}

void dlgConfig::OnNotebookPageChanged(wxBookCtrlEvent& event)
{
	// This is a workaround for notebook layout issues on some platforms
	if(nullptr != m_Notebook)
	{
		wxWindowList pages = m_Notebook->GetChildren();

		if(static_cast<int>(pages.size()) > event.GetSelection())
		{
			wxPanel* page = dynamic_cast<wxPanel*>(pages[event.GetSelection()]);

			if(nullptr != page)
			{
				page->Layout();
			}
		}
	}
}

// TODO: Design a cleaner system for loading/saving these settings

// Load settings from configuration file
void dlgConfig::LoadSettings()
{
	wxFileConfig ConfigInfo;

	// Allow $ in directory names
	ConfigInfo.SetExpandEnvVars(false);

	bool UseBroadcast;
	bool GetListOnStart, ShowBlockedServers, LoadChatOnLS, CheckForUpdates;
	bool FlashTaskBar, PlaySystemBell, PlaySoundFile, HighlightServers;
	bool CustomServersHighlight;

	bool AutoServerRefresh;
	int ThreadMul, ThreadMax, MasterTimeout, ServerTimeout, RetryCount;
    int RefreshInterval;
	wxString OdamexDirectory, ExtraCmdLineArgs;
	wxString SoundFile, HighlightColour, CustomServerColour;
	wxInt32 PQGood, PQPlayable, PQLaggy;

	ConfigInfo.Read(USEBROADCAST, &UseBroadcast, ODA_QRYUSEBROADCAST);
	ConfigInfo.Read(GETLISTONSTART, &GetListOnStart, ODA_UIGETLISTONSTART);
	ConfigInfo.Read(SHOWBLOCKEDSERVERS, &ShowBlockedServers,
	                ODA_UISHOWBLOCKEDSERVERS);
    ConfigInfo.Read(CHECKFORUPDATES, &CheckForUpdates,
	                ODA_UIAUTOCHECKFORUPDATES);
	ConfigInfo.Read(ODAMEX_DIRECTORY, &OdamexDirectory, OdaGetInstallDir());
	ConfigInfo.Read(MASTERTIMEOUT, &MasterTimeout, ODA_QRYMASTERTIMEOUT);
	ConfigInfo.Read(SERVERTIMEOUT, &ServerTimeout, ODA_QRYSERVERTIMEOUT);
	ConfigInfo.Read(RETRYCOUNT, &RetryCount, ODA_QRYGSRETRYCOUNT);
	ConfigInfo.Read(EXTRACMDLINEARGS, &ExtraCmdLineArgs, "");
	ConfigInfo.Read(ICONPINGQGOOD, &PQGood, ODA_UIPINGQUALITYGOOD);
	ConfigInfo.Read(ICONPINGQPLAYABLE, &PQPlayable,
	                ODA_UIPINGQUALITYPLAYABLE);
	ConfigInfo.Read(ICONPINGQLAGGY, &PQLaggy, ODA_UIPINGQUALITYLAGGY);
	ConfigInfo.Read(LOADCHATONLS, &LoadChatOnLS, ODA_UILOADCHATCLIENTONLS);
	ConfigInfo.Read(POLFLASHTBAR, &FlashTaskBar, ODA_UIPOLFLASHTASKBAR);
	ConfigInfo.Read(POLPLAYSYSTEMBELL, &PlaySystemBell, ODA_UIPOLPLAYSYSTEMBELL);
	ConfigInfo.Read(POLPLAYSOUND, &PlaySoundFile, ODA_UIPOLPLAYSOUND);
	ConfigInfo.Read(POLPSWAVFILE, &SoundFile, "");
	ConfigInfo.Read(POLHLSERVERS, &HighlightServers, ODA_UIPOLHIGHLIGHTSERVERS);
	ConfigInfo.Read(POLHLSCOLOUR, &HighlightColour, ODA_UIPOLHSHIGHLIGHTCOLOUR);
	ConfigInfo.Read(ARTENABLE, &AutoServerRefresh, ODA_UIARTENABLE);
	ConfigInfo.Read(ARTREFINTERVAL, &RefreshInterval, ODA_UIARTREFINTERVAL);
    ConfigInfo.Read(QRYTHREADMULTIPLIER, &ThreadMul, ODA_THRMULVAL);
    ConfigInfo.Read(QRYTHREADMAXIMUM, &ThreadMax, ODA_THRMAXVAL);
	ConfigInfo.Read(CSHLSERVERS, &CustomServersHighlight, ODA_UICSHIGHTLIGHTSERVERS);
	ConfigInfo.Read(CSHLCOLOUR, &CustomServerColour, ODA_UICSHSHIGHLIGHTCOLOUR);

	m_ChkCtrlEnableBroadcasts->SetValue(UseBroadcast);
	m_ChkCtrlGetListOnStart->SetValue(GetListOnStart);
	m_ChkCtrlShowBlockedServers->SetValue(ShowBlockedServers);
	//m_ChkCtrlCheckForUpdates->SetValue(CheckForUpdates);
	//m_ChkCtrlLoadChatOnLS->SetValue(LoadChatOnLS);
	m_ChkCtrlFlashTaskBar->SetValue(FlashTaskBar);
	m_ChkCtrlPlaySystemBeep->SetValue(PlaySystemBell);
	m_ChkCtrlPlaySoundFile->SetValue(PlaySoundFile);
	m_ChkCtrlHighlightServerLines->SetValue(HighlightServers);
	m_ChkCtrlHighlightCustomServers->SetValue(CustomServersHighlight);
	m_ChkCtrlkAutoServerRefresh->SetValue(AutoServerRefresh);

	m_DirCtrlChooseOdamexPath->SetPath(OdamexDirectory);
	m_FilePickCtrlSoundFile->SetPath(SoundFile);
	m_ClrPickServerLineHighlighter->SetColour(HighlightColour);
    m_ClrPickCustomServerHighlight->SetColour(CustomServerColour);

    m_SpnCtrlThreadMul->SetValue(ThreadMul);
    m_SpnCtrlThreadMax->SetValue(ThreadMax);
	m_SpnCtrlMasterTimeout->SetValue(MasterTimeout);
	m_SpnCtrlServerTimeout->SetValue(ServerTimeout);
	m_SpnCtrlRetry->SetValue(RetryCount);

	m_SpnRefreshInterval->SetValue(RefreshInterval);

	m_TxtCtrlExtraCmdLineArgs->SetValue(ExtraCmdLineArgs);

	m_SpnCtrlPQGood->SetValue(PQGood);
	m_SpnCtrlPQPlayable->SetValue(PQPlayable);
	m_SpnCtrlPQLaggy->SetValue(PQLaggy);

}

// Save settings to configuration file
void dlgConfig::SaveSettings()
{
	wxFileConfig ConfigInfo;

	// Allow $ in directory names
	ConfigInfo.SetExpandEnvVars(false);

	ConfigInfo.Write(MASTERTIMEOUT, m_SpnCtrlMasterTimeout->GetValue());
	ConfigInfo.Write(SERVERTIMEOUT, m_SpnCtrlServerTimeout->GetValue());
	ConfigInfo.Write(RETRYCOUNT, m_SpnCtrlRetry->GetValue());
	ConfigInfo.Write(EXTRACMDLINEARGS, m_TxtCtrlExtraCmdLineArgs->GetValue());
	ConfigInfo.Write(GETLISTONSTART, m_ChkCtrlGetListOnStart->GetValue());
	ConfigInfo.Write(SHOWBLOCKEDSERVERS, m_ChkCtrlShowBlockedServers->GetValue());
	ConfigInfo.Write(ODAMEX_DIRECTORY, m_DirCtrlChooseOdamexPath->GetPath());
	ConfigInfo.Write(ICONPINGQGOOD, m_SpnCtrlPQGood->GetValue());
	ConfigInfo.Write(ICONPINGQPLAYABLE, m_SpnCtrlPQPlayable->GetValue());
	ConfigInfo.Write(ICONPINGQLAGGY, m_SpnCtrlPQLaggy->GetValue());
	ConfigInfo.Write(USEBROADCAST, m_ChkCtrlEnableBroadcasts->GetValue());
	ConfigInfo.Write(POLFLASHTBAR, m_ChkCtrlFlashTaskBar->GetValue());
	ConfigInfo.Write(POLPLAYSYSTEMBELL, m_ChkCtrlPlaySystemBeep->GetValue());
	ConfigInfo.Write(POLPLAYSOUND, m_ChkCtrlPlaySoundFile->GetValue());
	ConfigInfo.Write(POLPSWAVFILE, m_FilePickCtrlSoundFile->GetPath());
	ConfigInfo.Write(POLHLSERVERS, m_ChkCtrlHighlightServerLines->GetValue());
	ConfigInfo.Write(POLHLSCOLOUR, m_ClrPickServerLineHighlighter->GetColour().GetAsString(wxC2S_HTML_SYNTAX));
	ConfigInfo.Write(ARTENABLE, m_ChkCtrlkAutoServerRefresh->GetValue());
	ConfigInfo.Write(ARTREFINTERVAL, m_SpnRefreshInterval->GetValue());
    ConfigInfo.Write(QRYTHREADMULTIPLIER, m_SpnCtrlThreadMul->GetValue());
    ConfigInfo.Write(QRYTHREADMAXIMUM,  m_SpnCtrlThreadMax->GetValue());
	ConfigInfo.Write(CSHLSERVERS, m_ChkCtrlHighlightCustomServers->GetValue());
	ConfigInfo.Write(CSHLCOLOUR, m_ClrPickCustomServerHighlight->GetColour().GetAsString(wxC2S_HTML_SYNTAX));

	ConfigInfo.Flush();
}
