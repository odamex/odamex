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
// DESCRIPTION: OdaGet file download utility
//
//-----------------------------------------------------------------------------

#include "frm_odaget.h"

#include <wx/xrc/xmlres.h>
#include <wx/uri.h>
#include <wx/wfstream.h>

DEFINE_EVENT_TYPE(EVENT_HTTP_THREAD)
DEFINE_EVENT_TYPE(EVENT_FTP_THREAD)

BEGIN_EVENT_TABLE(frmOdaGet, wxFrame)
    EVT_CLOSE(frmOdaGet::OnClose)
    EVT_BUTTON(XRCID("m_Download"), frmOdaGet::OnDownload)
    EVT_BUTTON(wxID_CANCEL, frmOdaGet::OnCancel)
    EVT_COMMAND(-1, EVENT_HTTP_THREAD, frmOdaGet::OnHttpThreadMessage)
    EVT_COMMAND(-1, EVENT_FTP_THREAD, frmOdaGet::OnFtpThreadMessage)
END_EVENT_TABLE()

frmOdaGet::frmOdaGet(wxTopLevelWindow* parent, wxWindowID id, wxString SaveLocation)
 : m_SaveLocation(SaveLocation)
{
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("frmOdaGet"));

    if (parent)
    {
        SetIcons(parent->GetIcons());
    }

    m_HTTPThread = NULL;
    m_FTPThread = NULL;

    m_DownloadURL = XRCCTRL(*this, "m_DownloadURL", wxTextCtrl);
    m_LocationDisplay = XRCCTRL(*this, "m_LocationDisplay", wxTextCtrl);
    m_DownloadGauge = XRCCTRL(*this, "m_DownloadGauge", wxGauge);
}

frmOdaGet::~frmOdaGet()
{
    if (m_HTTPThread && m_HTTPThread->IsRunning())
    {
        m_HTTPThread->Wait();
        delete m_HTTPThread;

        m_HTTPThread = NULL;
    }

    if (m_FTPThread && m_FTPThread->IsRunning())
    {
        m_FTPThread->Wait();
        delete m_FTPThread;

        m_FTPThread = NULL;
    }
}

void frmOdaGet::OnClose(wxCloseEvent &event)
{
    if (m_HTTPThread && m_HTTPThread->IsRunning())
    {
        m_HTTPThread->Wait();
        m_HTTPThread = NULL;
    }

    if (m_FTPThread && m_FTPThread->IsRunning())
    {
        m_FTPThread->Wait();
        m_FTPThread = NULL;
    }

    m_LocationDisplay->Clear();
    m_DownloadURL->Clear();

    Hide();
}

void frmOdaGet::OnCancel(wxCommandEvent &event)
{
    Close();
}

//
// void frmOdaGet::OnDownload(wxCommandEvent &event)
//
// User clicks the Download button
void frmOdaGet::OnDownload(wxCommandEvent &event)
{
    wxString URL = m_DownloadURL->GetValue();

    URL.MakeLower();

    m_DownloadGauge->SetValue(0);

    if (m_HTTPThread && m_HTTPThread->IsRunning())
    {
        m_HTTPThread->Wait();
        m_HTTPThread = NULL;
    }

    if (m_FTPThread && m_FTPThread->IsRunning())
    {
        m_FTPThread->Wait();
        m_FTPThread = NULL;
    }

    if (URL != wxT(""))
    {
        wxURI URI(m_DownloadURL->GetValue());
        wxString Scheme;

        // Is it an ftp or http scheme? (ftp:// or http://)
        if (URI.HasScheme())
            Scheme = URI.GetScheme();

        if (Scheme == wxT("http"))
        {
            m_HTTPThread = new HTTPThread(this,
                                m_DownloadURL->GetValue(),
                                m_SaveLocation);

            if (m_HTTPThread->Create((unsigned int)0) == wxTHREAD_NO_ERROR)
            {
                m_HTTPThread->Run();
            }
        }

        if (Scheme == wxT("ftp"))
        {
            m_FTPThread = new FTPThread(this,
                                m_DownloadURL->GetValue(),
                                m_SaveLocation);

            if (m_FTPThread->Create((unsigned int)0) == wxTHREAD_NO_ERROR)
            {
                m_FTPThread->Run();
            }
        }
    }
}

//
// void frmOdaGet::OnFtpThreadMessage(wxCommandEvent &event)
//
// Events sent back by FTP thread
void frmOdaGet::OnFtpThreadMessage(wxCommandEvent &event)
{
    wxString String;

    switch (event.GetId())
    {
        case FTP_BADURL:
        {
            String = wxString::Format(wxT("Invalid URL: %s\n"),
                                      event.GetString().c_str());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case FTP_CONNECTED:
        {
            String = wxString::Format(wxT("Connected to %s:%u\n"),
                                      event.GetString().c_str(),
                                      event.GetInt());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case FTP_DISCONNECTED:
        {
            String = wxString::Format(wxT("Failed to connect to %s:%u\n"),
                                      event.GetString().c_str(),
                                      event.GetInt());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case FTP_GOTFILEINFO:
        {
            String = wxString::Format(wxT("File size is %u\n"),
                                      (size_t)event.GetClientData());

            m_DownloadGauge->SetRange((size_t)event.GetClientData());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case FTP_DOWNLOADING:
        {
            String = wxString::Format(wxT("Now downloading file to %s\n"),
                                      event.GetString().c_str());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case FTP_DOWNLOADERROR:
        {
            String = wxString::Format(wxT("Download of file %s failed\n"),
                                        event.GetString().c_str());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case FTP_DOWNLOADTERMINATED:
        {
            String = wxT("User stopped download\n");

            m_LocationDisplay->AppendText(String);
        }
        break;

        case FTP_POSITION:
        {
            m_DownloadGauge->SetValue((size_t)event.GetClientData());
        }
        break;

        case FTP_DOWNLOADCOMPLETE:
        {
            String = wxT("Download complete\n");

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
void frmOdaGet::OnHttpThreadMessage(wxCommandEvent &event)
{
    wxString String;

    switch (event.GetId())
    {
        case HTTP_BADURL:
        {
            String = wxString::Format(wxT("Invalid URL: %s\n"),
                                      event.GetString().c_str());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case HTTP_CONNECTED:
        {
            String = wxString::Format(wxT("Connected to %s:%u\n"),
                                      event.GetString().c_str(),
                                      event.GetInt());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case HTTP_DISCONNECTED:
        {
            String = wxString::Format(wxT("Failed to connect to %s:%u\n"),
                                      event.GetString().c_str(),
                                      event.GetInt());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case HTTP_GOTFILEINFO:
        {
            String = wxString::Format(wxT("File size is %u\n"),
                                      (size_t)event.GetClientData());

            m_DownloadGauge->SetRange((size_t)event.GetClientData());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case HTTP_DOWNLOADING:
        {
            String = wxString::Format(wxT("Now downloading file to %s\n"),
                                      event.GetString().c_str());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case HTTP_DOWNLOADERROR:
        {
            String = wxString::Format(wxT("Download of file %s failed\n"),
                                        event.GetString().c_str());

            m_LocationDisplay->AppendText(String);
        }
        break;

        case HTTP_DOWNLOADTERMINATED:
        {
            String = wxT("User stopped download\n");

            m_LocationDisplay->AppendText(String);
        }
        break;

        case HTTP_POSITION:
        {
            m_DownloadGauge->SetValue((size_t)event.GetClientData());
        }
        break;

        case HTTP_DOWNLOADCOMPLETE:
        {
            String = wxT("Download complete\n");

            m_LocationDisplay->AppendText(String);
        }
        break;

        default:
            break;
    }
}

URIResult URIHandler::ParseURL(const wxString &URL)
{
    wxURI URI(URL);

    if (!URI.HasServer())
    {
        return URI_BADDOMAIN;
    }

    m_Server = URI.GetServer();

    if (!URI.HasPath())
    {
        return URI_BADPATH;
    }

    m_Path = URI.GetPath();

    // We support searching directories for files, this will be used later on
    wxInt32 FilePosition = (m_Path.Find(wxT('/'), true) + 1);

    if (FilePosition >= m_Path.Length())
    {
        if (m_File == wxT(""))
            return URI_BADFILE;
    }

    // Save the directory as well
    m_Directory = m_Path.Mid(0, FilePosition - 1);

    if (m_File == wxT(""))
        m_File = m_Path.Mid(FilePosition);

    if (URI.HasPort())
        m_Port = wxAtoi(URI.GetPort());

    if (URI.HasUserInfo())
    {
        wxString UserInfo = URI.GetUserInfo();

        // These are deprecated as of RFC 1396
        m_User = URI.GetUser();

        if (UserInfo.Find(wxT(':'), false) != -1)
            m_Password = URI.GetPassword();
    }

    return URI_SUCCESS;
}

wxInt32 ExtractCompressedFile(const wxString &Filename, const wxString &SaveLocation)
{
    wxFileInputStream FileInputStream(Filename);

    if (!FileInputStream.IsOk())
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
void *FTPThread::Entry()
{
    wxCommandEvent Event(EVENT_FTP_THREAD, wxID_ANY );
    wxInputStream *InputStream;
    size_t FileSize = 0;

    URIHandler URI(m_File);

    switch (URI.ParseURL(m_URL))
    {
        case URI_BADDOMAIN:
        {
            Event.SetId(FTP_BADURL);
            Event.SetString(wxT("No domain specified"));
            wxPostEvent(m_EventHandler, Event);

            return NULL;
        }
        break;

        case URI_BADPATH:
        {
            Event.SetId(FTP_BADURL);
            Event.SetString(wxT("Path to file not specified"));
            wxPostEvent(m_EventHandler, Event);

            return NULL;
        }
        break;

        case URI_BADFILE:
        {
            Event.SetId(FTP_BADURL);
            Event.SetString(wxT("This is a directory, not a file"));
            wxPostEvent(m_EventHandler, Event);

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

    if (!Port)
        Port = 21;

    // Blame wx
    wxString User = URI.GetUser();
    wxString Password = URI.GetPassword();

    if (User != wxT(""))
        m_FTP.SetUser(User);

    if (Password != wxT(""))
        m_FTP.SetPassword(Password);

    wxIPV4address IPV4address;

    IPV4address.Hostname(URI.GetServer());
    IPV4address.Service(Port);

    // Try to connect to the server
    // Why can't this accept a port parameter? :'(
    if (m_FTP.Connect(IPV4address))
    {
        // Successful connection
        Event.SetId(FTP_CONNECTED);
        Event.SetString(URI.GetServer());
        Event.SetInt(Port);
        wxPostEvent(m_EventHandler, Event);
    }
    else
    {
        // We failed miserably
        Event.SetId(FTP_DISCONNECTED);
        Event.SetString(URI.GetServer());
        Event.SetInt(Port);
        wxPostEvent(m_EventHandler, Event);

        return NULL;
    }

    // Change the directory
    m_FTP.ChDir(URI.GetDirectory());

    // Try to locate the file
    if ((InputStream = m_FTP.GetInputStream(m_File)))
    {
        FileSize = InputStream->GetSize();

        // We now got the stream for the file, return some data
        Event.SetId(FTP_GOTFILEINFO);
        Event.SetClientData((void *)FileSize);
        wxPostEvent(m_EventHandler, Event);
    }
    else
    {
        // Location of file is invalid
        Event.SetId(FTP_DOWNLOADERROR);
        Event.SetString(URI.GetPath());
        wxPostEvent(m_EventHandler, Event);

        return NULL;
    }

    // Create the file
    wxFileName FileName(m_SaveLocation, m_File);

    wxFileOutputStream FileOutputStream(FileName.GetFullPath());

    if (FileOutputStream.IsOk())
    {
        Event.SetId(FTP_DOWNLOADING);
        Event.SetString(FileName.GetFullPath());
        wxPostEvent(m_EventHandler, Event);
    }
    else
    {
        Event.SetId(FTP_DOWNLOADERROR);
        Event.SetString(FileName.GetFullPath());
        wxPostEvent(m_EventHandler, Event);

        delete InputStream;

        return NULL;
    }

    // Download the file
    wxChar Data[1024];

    size_t i = 0;

    while (InputStream->CanRead())
    {
        InputStream->Read(&Data, 1024);

        Event.SetId(FTP_POSITION);
        Event.SetClientData((void *)i);
        wxPostEvent(m_EventHandler, Event);

        // User wanted us to exit
        if (TestDestroy())
        {
            Event.SetId(FTP_DOWNLOADTERMINATED);
            Event.SetString(m_File);
            wxPostEvent(m_EventHandler, Event);

            delete InputStream;

            return NULL;
        }

        FileOutputStream.Write(&Data, InputStream->LastRead());

        Sleep(1);

        if (InputStream->LastRead() == 0)
            break;

        i += InputStream->LastRead();
    }

    // Download done
    Event.SetId(FTP_DOWNLOADCOMPLETE);
    Event.SetString(FileName.GetFullPath());
    wxPostEvent(m_EventHandler, Event);

    delete InputStream;

    return NULL;
}

//
// void *HTTPThread::Entry()
//
// HTTP file download thread
void *HTTPThread::Entry()
{
    wxCommandEvent Event(EVENT_HTTP_THREAD, wxID_ANY );
    wxInputStream *InputStream;
    size_t FileSize = 0;

    URIHandler URI(m_File);

    switch (URI.ParseURL(m_URL))
    {
        case URI_BADDOMAIN:
        {
            Event.SetId(HTTP_BADURL);
            Event.SetString(wxT("No domain specified"));
            wxPostEvent(m_EventHandler, Event);

            return NULL;
        }
        break;

        case URI_BADPATH:
        {
            Event.SetId(HTTP_BADURL);
            Event.SetString(wxT("Path to file not specified"));
            wxPostEvent(m_EventHandler, Event);

            return NULL;
        }
        break;

        case URI_BADFILE:
        {
            Event.SetId(HTTP_BADURL);
            Event.SetString(wxT("This is a directory, not a file"));
            wxPostEvent(m_EventHandler, Event);

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

    if (!Port)
        Port = 80;

    // Try to connect to the server
    if (m_HTTP.Connect(URI.GetServer(), Port))
    {
        // Successful connection
        Event.SetId(HTTP_CONNECTED);
        Event.SetString(URI.GetServer());
        Event.SetInt(Port);
        wxPostEvent(m_EventHandler, Event);
    }
    else
    {
        // We failed miserably
        Event.SetId(HTTP_DISCONNECTED);
        Event.SetString(URI.GetServer());
        Event.SetInt(Port);
        wxPostEvent(m_EventHandler, Event);

        return NULL;
    }

    // Try to locate the file
    if ((InputStream = m_HTTP.GetInputStream(URI.GetPath())))
    {
        FileSize = InputStream->GetSize();

        // We now got the stream for the file, return some data
        Event.SetId(HTTP_GOTFILEINFO);
        Event.SetClientData((void *)FileSize);
        wxPostEvent(m_EventHandler, Event);
    }
    else
    {
        // Location of file is invalid
        Event.SetId(HTTP_DOWNLOADERROR);
        Event.SetString(URI.GetPath());
        wxPostEvent(m_EventHandler, Event);

        return NULL;
    }

    // Create the file
    wxFileName FileName(m_SaveLocation, m_File);

    wxFileOutputStream FileOutputStream(FileName.GetFullPath());

    if (FileOutputStream.IsOk())
    {
        Event.SetId(HTTP_DOWNLOADING);
        Event.SetString(FileName.GetFullPath());
        wxPostEvent(m_EventHandler, Event);
    }
    else
    {
        Event.SetId(HTTP_DOWNLOADERROR);
        Event.SetString(FileName.GetFullPath());
        wxPostEvent(m_EventHandler, Event);

        delete InputStream;

        return NULL;
    }

    // Download the file
    wxChar Data[1024];

    size_t i = 0;

    while (InputStream->CanRead())
    {
        InputStream->Read(&Data, 1024);

        Event.SetId(HTTP_POSITION);
        Event.SetClientData((void *)i);
        wxPostEvent(m_EventHandler, Event);

        // User wanted us to exit
        if (TestDestroy())
        {
            Event.SetId(HTTP_DOWNLOADTERMINATED);
            Event.SetString(m_File);
            wxPostEvent(m_EventHandler, Event);

            delete InputStream;

            return NULL;
        }

        FileOutputStream.Write(&Data, InputStream->LastRead());

        Sleep(1);


        if (InputStream->LastRead() == 0)
            break;

        i += InputStream->LastRead();
    }

    // Download done
    Event.SetId(HTTP_DOWNLOADCOMPLETE);
    Event.SetString(FileName.GetFullPath());
    wxPostEvent(m_EventHandler, Event);

    delete InputStream;

    return NULL;
}
