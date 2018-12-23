// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
// DESCRIPTION: OdaGet file download utility
//
//-----------------------------------------------------------------------------

#include "frm_odaget.h"

#include <wx/xrc/xmlres.h>
#include <wx/uri.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <wx/event.h>
#include <wx/wxchar.h>

DEFINE_EVENT_TYPE(EVENT_HTTP_THREAD)
DEFINE_EVENT_TYPE(EVENT_FTP_THREAD)

BEGIN_EVENT_TABLE(frmOdaGet, wxFrame)
	EVT_CLOSE(frmOdaGet::OnClose)
	EVT_BUTTON(XRCID("m_Download"), frmOdaGet::OnDownload)
	EVT_BUTTON(wxID_CANCEL, frmOdaGet::OnCancel)
	EVT_COMMAND(-1, EVENT_HTTP_THREAD, frmOdaGet::OnHttpThreadMessage)
	EVT_COMMAND(-1, EVENT_FTP_THREAD, frmOdaGet::OnFtpThreadMessage)
END_EVENT_TABLE()

#define SIZE_UPDATE 1024

// Helper class for getting write notifications to disk
class wxMyFileOutputStream : public wxFileOutputStream
{
public:
	wxMyFileOutputStream(const wxString& fileName,
	                     wxEvtHandler* EvtHandler,
	                     wxEventType EventType,
	                     wxThread* Thread) : wxFileOutputStream(fileName),
		m_EventHandler(EvtHandler), m_EventType(EventType),
		m_Thread(Thread)
	{ }

	// override some virtual functions to resolve ambiguities, just as in
	// wxFileStream

	virtual bool IsOk() const
	{
		return wxFileOutputStream::IsOk();
	}

protected:
	virtual wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode)
	{
		// This is needed so thread sunspension works on other platforms
		m_Thread->TestDestroy();

		return wxFileOutputStream::OnSysSeek(pos, mode);
	}

	virtual wxFileOffset OnSysTell() const
	{
		// This is needed so thread sunspension works on other platforms
		m_Thread->TestDestroy();

		return wxFileOutputStream::OnSysTell();
	}

	size_t OnSysWrite(const void* buffer, size_t size)
	{
		wxCommandEvent Event(m_EventType, wxID_ANY);

		Event.SetId(SIZE_UPDATE);
		Event.SetInt((size_t)size);
		wxQueueEvent(m_EventHandler, Event.Clone());

		// This is needed so thread sunspension works on other platforms
		m_Thread->TestDestroy();

		return wxFileOutputStream::OnSysWrite(buffer, size);
	}

private:
	wxThread* m_Thread;
	wxEvtHandler* m_EventHandler;
	wxEventType m_EventType;
};

frmOdaGet::frmOdaGet(wxTopLevelWindow* parent, wxWindowID id, wxString SaveLocation)
	: m_SaveLocation(SaveLocation)
{
	wxXmlResource::Get()->LoadFrame(this, parent, "frmOdaGet");

	if(parent)
	{
		SetIcons(parent->GetIcons());
	}

	m_HTTPThread = NULL;
	m_FTPThread = NULL;

	m_DownloadURL = XRCCTRL(*this, "m_DownloadURL", wxTextCtrl);
	m_LocationDisplay = XRCCTRL(*this, "m_LocationDisplay", wxTextCtrl);
	m_DownloadGauge = XRCCTRL(*this, "m_DownloadGauge", wxGauge);
}

void frmOdaGet::DeleteThreads()
{
	if(m_HTTPThread && m_HTTPThread->IsRunning())
	{
		m_HTTPThread->CloseConnection();

		m_HTTPThread->Wait();
		delete m_HTTPThread;

		m_HTTPThread = NULL;
	}

	if(m_FTPThread && m_FTPThread->IsRunning())
	{
		m_FTPThread->CloseConnection();

		m_FTPThread->Wait();
		delete m_FTPThread;

		m_FTPThread = NULL;
	}
}

frmOdaGet::~frmOdaGet()
{
	DeleteThreads();
}

void frmOdaGet::OnClose(wxCloseEvent& event)
{
	DeleteThreads();

	m_LocationDisplay->Clear();
	m_DownloadURL->Clear();
	m_DownloadGauge->SetValue(0);

	Hide();
}

void frmOdaGet::OnCancel(wxCommandEvent& event)
{
	Close();
}

//
// void frmOdaGet::OnDownload(wxCommandEvent &event)
//
// User clicks the Download button
void frmOdaGet::OnDownload(wxCommandEvent& event)
{
	wxString URL = m_DownloadURL->GetValue();

	URL.MakeLower();

	m_DownloadGauge->SetValue(0);

	DeleteThreads();

	if(URL != "")
	{
		wxURI URI(m_DownloadURL->GetValue());
		wxString Scheme;

		// Is it an ftp or http scheme? (ftp:// or http://)
		if(URI.HasScheme())
			Scheme = URI.GetScheme();

		if(Scheme == "http")
		{
			m_HTTPThread = new HTTPThread(this,
			                              m_DownloadURL->GetValue(),
			                              m_SaveLocation);

			m_HTTPThread->Run();
		}

		if(Scheme == "ftp")
		{
			m_FTPThread = new FTPThread(this,
			                            m_DownloadURL->GetValue(),
			                            m_SaveLocation);

			m_FTPThread->Run();
		}
	}
}

//
// void frmOdaGet::OnFtpThreadMessage(wxCommandEvent &event)
//
// Events sent back by FTP thread
void frmOdaGet::OnFtpThreadMessage(wxCommandEvent& event)
{
	wxString String;

	switch(event.GetId())
	{
	case FTP_BADURL:
	{
		String = wxString::Format("Invalid URL: %s\n",
		                          event.GetString().c_str());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case FTP_CONNECTED:
	{
		String = wxString::Format("Connected to %s:%u\n",
		                          event.GetString().c_str(),
		                          event.GetInt());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case FTP_DISCONNECTED:
	{
		String = wxString::Format("Failed to connect to %s:%u\n",
		                          event.GetString().c_str(),
		                          event.GetInt());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case FTP_GOTFILEINFO:
	{
		m_FileSize = event.GetInt();

		if(m_FileSize > 0)
		{
			String = wxString::Format("File size is %d\n",
			                          m_FileSize);

			m_DownloadGauge->SetRange(event.GetInt());
		}
		else
		{
			String = "File size not available\n";

			m_DownloadGauge->SetRange(10);
		}

		m_LocationDisplay->AppendText(String);
	}
	break;

	case FTP_DOWNLOADING:
	{
		String = wxString::Format("Now downloading file to %s\n",
		                          event.GetString().c_str());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case FTP_DOWNLOADERROR:
	{
		String = wxString::Format("Download of file %s failed\n",
		                          event.GetString().c_str());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case FTP_DOWNLOADTERMINATED:
	{
		String = "User stopped download\n";

		m_LocationDisplay->AppendText(String);
	}
	break;

	case SIZE_UPDATE:
	{
		if(m_FileSize > 0)
		{
			int i = m_DownloadGauge->GetValue();

			m_DownloadGauge->SetValue(event.GetInt() + i);
		}
		else
		{
			// Our pseudo-progress indicator
			m_DownloadGauge->SetValue(m_DownloadGauge->GetValue() ? 0 : 10);
		}
	}
	break;

	case FTP_DOWNLOADCOMPLETE:
	{
		String = "Download complete\n";

		m_LocationDisplay->AppendText(String);
	}
	break;

	default:
		break;
	}
}

//
// void frmOdaGet::OnHttpThreadMessage(wxCommandEvent &event)
//
// Events sent back by HTTP thread
void frmOdaGet::OnHttpThreadMessage(wxCommandEvent& event)
{
	wxString String;

	switch(event.GetId())
	{
	case HTTP_BADURL:
	{
		String = wxString::Format("Invalid URL: %s\n",
		                          event.GetString().c_str());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case HTTP_CONNECTED:
	{
		String = wxString::Format("Connected to %s:%u\n",
		                          event.GetString().c_str(),
		                          event.GetInt());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case HTTP_DISCONNECTED:
	{
		String = wxString::Format("Failed to connect to %s:%u\n",
		                          event.GetString().c_str(),
		                          event.GetInt());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case HTTP_GOTFILEINFO:
	{
		String = wxString::Format("File size is %llu\n",
		                          (size_t)event.GetInt());

		m_DownloadGauge->SetRange((size_t)event.GetInt());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case HTTP_DOWNLOADING:
	{
		String = wxString::Format("Now downloading file to %s\n",
		                          event.GetString().c_str());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case HTTP_DOWNLOADERROR:
	{
		String = wxString::Format("Download of file %s failed\n",
		                          event.GetString().c_str());

		m_LocationDisplay->AppendText(String);
	}
	break;

	case HTTP_DOWNLOADTERMINATED:
	{
		String = "User stopped download\n";

		m_LocationDisplay->AppendText(String);
	}
	break;

	case SIZE_UPDATE:
	{
		int i = m_DownloadGauge->GetValue();

		m_DownloadGauge->SetValue((size_t)event.GetInt() + i);
	}
	break;

	case HTTP_DOWNLOADCOMPLETE:
	{
		String = "Download complete\n";

		m_LocationDisplay->AppendText(String);
	}
	break;

	default:
		break;
	}
}

URIResult URIHandler::ParseURL(const wxString& URL)
{
	wxURI URI(URL);

	if(!URI.HasServer())
	{
		return URI_BADDOMAIN;
	}

	m_Server = URI.GetServer();

	if(!URI.HasPath())
	{
		return URI_BADPATH;
	}

	m_Path = URI.GetPath();

	// We support searching directories for files, this will be used later on
	wxInt32 FilePosition = (m_Path.Find('/', true) + 1);

	if(FilePosition >= m_Path.Length())
	{
		if(m_File == "")
			return URI_BADFILE;
	}

	// Save the directory as well
	m_Directory = m_Path.Mid(0, FilePosition - 1);

	if(m_File == "")
		m_File = m_Path.Mid(FilePosition);

	if(URI.HasPort())
		m_Port = wxAtoi(URI.GetPort());

	if(URI.HasUserInfo())
	{
		wxString UserInfo = URI.GetUserInfo();

		// These are deprecated as of RFC 1396
		m_User = URI.GetUser();

		if(UserInfo.Find(':', false) != -1)
			m_Password = URI.GetPassword();
	}

	return URI_SUCCESS;
}

wxInt32 ExtractCompressedFile(const wxString& Filename, const wxString& SaveLocation)
{
	wxFileInputStream FileInputStream(Filename);

	if(!FileInputStream.IsOk())
		return 0;

	// Zip file handling
	// TODO: Complete this
	/*
	wxZipInputStream ZipInputStream(InputStream);

	if (ZipInputStream.IsOk())
	{
	    while (ZipEntry.reset(ZipInputStream.GetNextEntry()), ZipEntry.get() != NULL)
	    {
	        // access meta-data
	        wxString Name = ZipEntry->GetName();
	        // read 'zip' to access the entry's data
	        ZipInputStream.
	    }
	}*/

	// Some older versions of zlib can't open gzip's
	/*
	if (wxZlibInputStream::CanHandleGZip())
	{
	    wxZlibInputStream ZlibInputStream(FileInputStream);
	}*/
}

//
// void *FTPThread::Entry()
//
// FTP file download thread
void* FTPThread::Entry()
{
	wxCommandEvent Event(EVENT_FTP_THREAD, wxID_ANY);
	wxInputStream* InputStream;
	int FileSize = 0;

	URIHandler URI(m_File);

	switch(URI.ParseURL(m_URL))
	{
	case URI_BADDOMAIN:
	{
		Event.SetId(FTP_BADURL);
		Event.SetString("No domain specified");
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}
	break;

	case URI_BADPATH:
	{
		Event.SetId(FTP_BADURL);
		Event.SetString("Path to file not specified");
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}
	break;

	case URI_BADFILE:
	{
		Event.SetId(FTP_BADURL);
		Event.SetString("This is a directory, not a file");
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}
	break;

	default:
	{

	}
	break;
	}

	m_File = URI.GetFile();

	wxUint16 Port = URI.GetPort();

	if(!Port)
		Port = 21;

	// Blame wx
	wxString User = URI.GetUser();
	wxString Password = URI.GetPassword();

	if(User != "")
		m_FTP.SetUser(User);

	if(Password != "")
		m_FTP.SetPassword(Password);

	wxIPV4address IPV4address;

	IPV4address.Hostname(URI.GetServer());
	IPV4address.Service(Port);

	// Stupid wxFTP..
	wxLog::EnableLogging(false);

	// Try passive mode first
	m_FTP.SetPassive(true);

	// Try to connect to the server
	// Why can't this accept a port parameter? :'(
	if(m_FTP.Connect(IPV4address))
	{
		// Successful connection
		Event.SetId(FTP_CONNECTED);
		Event.SetString(URI.GetServer());
		Event.SetInt(Port);
		wxQueueEvent(m_EventHandler, Event.Clone());
	}
	else
	{
		// Try connecting again in Active mode
		m_FTP.SetPassive(false);

		if(m_FTP.Connect(IPV4address))
		{
			Event.SetId(FTP_CONNECTED);
			Event.SetString(URI.GetServer());
			Event.SetInt(Port);
			wxQueueEvent(m_EventHandler, Event.Clone());
		}
		else
		{
			// We failed miserably
			Event.SetId(FTP_DISCONNECTED);
			Event.SetString(URI.GetServer());
			Event.SetInt(Port);
			wxQueueEvent(m_EventHandler, Event.Clone());

			return NULL;
		}
	}

	// Change the directory
	m_FTP.ChDir(URI.GetDirectory());

	// Binary transfer mode
	m_FTP.SetBinary();

	// Try and get the file size first
	int statuscode;
	wxString Command;

	Command.Printf("SIZE %s", m_File);

	char ret = m_FTP.SendCommand(Command);

	if(ret == '2')
	{
		if(wxSscanf(m_FTP.GetLastResult().c_str(), "%i %i",
		            &statuscode, &FileSize) != 2)
		{
			// Try wx's version
			if(!FileSize)
				FileSize = m_FTP.GetFileSize(m_File);
		}
	}
	else
	{
		// Try wx's version
		FileSize = m_FTP.GetFileSize(m_File);
	}

	// Try to locate the file
	if((InputStream = m_FTP.GetInputStream(m_File)))
	{
		// We now got the stream for the file, return some data
		Event.SetId(FTP_GOTFILEINFO);
		Event.SetInt(FileSize);
		wxQueueEvent(m_EventHandler, Event.Clone());
	}
	else
	{
		// Location of file is invalid
		Event.SetId(FTP_DOWNLOADERROR);
		Event.SetString(URI.GetPath());
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}

	// Create the file
	wxFileName FileName(m_SaveLocation, m_File);

	wxMyFileOutputStream FileOutputStream(FileName.GetFullPath(),
	                                      m_EventHandler,
	                                      EVENT_FTP_THREAD,
	                                      this);

	if(FileOutputStream.IsOk())
	{
		Event.SetId(FTP_DOWNLOADING);
		Event.SetString(FileName.GetFullPath());
		wxQueueEvent(m_EventHandler, Event.Clone());
	}
	else
	{
		Event.SetId(FTP_DOWNLOADERROR);
		Event.SetString(FileName.GetFullPath());
		wxQueueEvent(m_EventHandler, Event.Clone());

		delete InputStream;

		return NULL;
	}

	// Download the file
	FileOutputStream.Write(*InputStream);

	// Download done
	Event.SetId(FTP_DOWNLOADCOMPLETE);
	Event.SetString(FileName.GetFullPath());
	wxQueueEvent(m_EventHandler, Event.Clone());

	delete InputStream;

	return NULL;
}

//
// void *HTTPThread::Entry()
//
// HTTP file download thread
void* HTTPThread::Entry()
{
	wxCommandEvent Event(EVENT_HTTP_THREAD, wxID_ANY);
	wxInputStream* InputStream;
	size_t FileSize = 0;

	URIHandler URI(m_File);

	switch(URI.ParseURL(m_URL))
	{
	case URI_BADDOMAIN:
	{
		Event.SetId(HTTP_BADURL);
		Event.SetString("No domain specified");
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}
	break;

	case URI_BADPATH:
	{
		Event.SetId(HTTP_BADURL);
		Event.SetString("Path to file not specified");
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}
	break;

	case URI_BADFILE:
	{
		Event.SetId(HTTP_BADURL);
		Event.SetString("This is a directory, not a file");
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}
	break;

	default:
	{

	}
	break;
	}

	m_File = URI.GetFile();

	m_HTTP.SetUser(URI.GetUser());
	m_HTTP.SetPassword(URI.GetPassword());

	wxUint16 Port = URI.GetPort();

	if(!Port)
		Port = 80;

	// Try to connect to the server
	if(m_HTTP.Connect(URI.GetServer(), Port))
	{
		// Successful connection
		Event.SetId(HTTP_CONNECTED);
		Event.SetString(URI.GetServer());
		Event.SetInt(Port);
		wxQueueEvent(m_EventHandler, Event.Clone());
	}
	else
	{
		// We failed miserably
		Event.SetId(HTTP_DISCONNECTED);
		Event.SetString(URI.GetServer());
		Event.SetInt(Port);
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}

	// Try to locate the file
	if((InputStream = m_HTTP.GetInputStream(URI.GetPath())))
	{
		FileSize = InputStream->GetSize();

		// We now got the stream for the file, return some data
		Event.SetId(HTTP_GOTFILEINFO);
		Event.SetInt((size_t)FileSize);
		wxQueueEvent(m_EventHandler, Event.Clone());
	}
	else
	{
		// Location of file is invalid
		Event.SetId(HTTP_DOWNLOADERROR);
		Event.SetString(URI.GetPath());
		wxQueueEvent(m_EventHandler, Event.Clone());

		return NULL;
	}

	// Create the file
	wxFileName FileName(m_SaveLocation, m_File);

	wxMyFileOutputStream FileOutputStream(FileName.GetFullPath(),
	                                      m_EventHandler,
	                                      EVENT_HTTP_THREAD,
	                                      this);

	if(FileOutputStream.IsOk())
	{
		Event.SetId(HTTP_DOWNLOADING);
		Event.SetString(FileName.GetFullPath());
		wxQueueEvent(m_EventHandler, Event.Clone());
	}
	else
	{
		Event.SetId(HTTP_DOWNLOADERROR);
		Event.SetString(FileName.GetFullPath());
		wxQueueEvent(m_EventHandler, Event.Clone());

		delete InputStream;

		return NULL;
	}

	FileOutputStream.Write(*InputStream);

	// Download done
	Event.SetId(HTTP_DOWNLOADCOMPLETE);
	Event.SetString(FileName.GetFullPath());
	wxQueueEvent(m_EventHandler, Event.Clone());

	delete InputStream;

	return NULL;
}
