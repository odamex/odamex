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

#ifndef __FRM_ODAGET__
#define __FRM_ODAGET__

#include <wx/frame.h>
#include <wx/gauge.h>
#include <wx/textctrl.h>

#include <wx/protocol/http.h>
#include <wx/protocol/ftp.h>
#include <wx/thread.h>

enum URIResult
{
     MIN_URIRESULTS
    ,URI_SUCCESS
    ,URI_BADDOMAIN
    ,URI_BADPATH
    ,URI_BADFILE
    ,MAX_URIRESULTS
};

// This class handles parsing of uniform resource identifiers (URI's) for
// our http and ftp thread classes
class URIHandler
{
    public:
        URIHandler(const wxString &File) : 
            m_User(wxT("")), m_Password(wxT("")), m_Server(wxT("")), m_Port(0), 
            m_Path(wxT("")), m_Directory(wxT("")),  m_File(File) 
        { 
            
        }
        ~URIHandler() { }
           
        URIResult ParseURL(const wxString &URL);     
        
        const wxString &GetUser() const { return m_User; }
        const wxString &GetPassword() const { return m_Password; }
        const wxString &GetServer() const { return m_Server; }
        const wxUint16 &GetPort() const { return m_Port; }
        const wxString &GetPath() const { return m_Path; }
        const wxString &GetDirectory() const { return m_Directory; }
        const wxString &GetFile() const { return m_File; }
              
    protected:
        wxString m_User;
        wxString m_Password;
        wxString m_Server;
        wxUint16 m_Port;
        wxString m_Path;
        wxString m_Directory;
        wxString m_File;
};

// FTP Stuff

DECLARE_EVENT_TYPE(EVENT_FTP_THREAD, -1);

enum FTPEvent
{
     MIN_FTPEVENTS
    ,FTP_BADURL
    ,FTP_CONNECTED
    ,FTP_DISCONNECTED
    ,FTP_GOTFILEINFO
    ,FTP_DOWNLOADING
    ,FTP_DOWNLOADERROR
    ,FTP_DOWNLOADTERMINATED
    ,FTP_POSITION
    ,FTP_DOWNLOADCOMPLETE
    ,MAX_FTPEVENTS
};

class FTPThread : public wxThread
{
    public:
        FTPThread(wxEvtHandler *EventHandler, wxString URL, wxString SaveLocation, wxString File = wxT("")) : 
         wxThread(wxTHREAD_JOINABLE), m_EventHandler(EventHandler), m_URL(URL), m_SaveLocation(SaveLocation)
        { 
            m_File = File;
        }
        
        ~FTPThread() { }
    
    private:
        virtual void *Entry();
        
        wxFTP m_FTP;
        wxEvtHandler *m_EventHandler;
        wxString m_URL;
        wxString m_SaveLocation;
        wxString m_File;
};

// HTTP Stuff

DECLARE_EVENT_TYPE(EVENT_HTTP_THREAD, -1);

enum HTTPEvent
{
     MIN_HTTPEVENTS
    ,HTTP_BADURL
    ,HTTP_CONNECTED
    ,HTTP_DISCONNECTED
    ,HTTP_GOTFILEINFO
    ,HTTP_DOWNLOADING
    ,HTTP_DOWNLOADERROR
    ,HTTP_DOWNLOADTERMINATED
    ,HTTP_POSITION
    ,HTTP_DOWNLOADCOMPLETE
    ,MAX_HTTPEVENTS
};

class HTTPThread : public wxThread
{
    public:
        HTTPThread(wxEvtHandler *EventHandler, wxString URL, wxString SaveLocation, wxString File = wxT("")) : 
         wxThread(wxTHREAD_JOINABLE), m_EventHandler(EventHandler), m_URL(URL), m_SaveLocation(SaveLocation)
        { 
            m_File = File;
            
            m_HTTP.SetHeader(wxT("Accept"), wxT("text/*"));
            m_HTTP.SetHeader(wxT("User-Agent"), wxT("OdaGet 0.1"));
            
            m_HTTP.SetTimeout(60);
        }
        
        ~HTTPThread() { }
   
    private:
        virtual void *Entry();
        
        wxHTTP m_HTTP;
        wxEvtHandler *m_EventHandler;
        wxString m_URL;
        wxString m_SaveLocation;
        wxString m_File;
};

class frmOdaGet : public wxFrame
{
    public:
        frmOdaGet(wxTopLevelWindow* parent, wxWindowID id = -1, wxString SaveLocation = wxT(""));
		virtual ~frmOdaGet();
    private:
        void OnClose(wxCloseEvent &event);
        void OnCancel(wxCommandEvent &event);
        void OnDownload(wxCommandEvent &event);
        
        void OnHttpThreadMessage(wxCommandEvent &event);
        void OnFtpThreadMessage(wxCommandEvent &event);
        
        FTPThread *m_FTPThread;
        HTTPThread *m_HTTPThread;
        
        wxTextCtrl *m_DownloadURL;
        wxTextCtrl *m_LocationDisplay;
        wxGauge *m_DownloadGauge;
        
        wxString m_SaveLocation;
        
        DECLARE_EVENT_TABLE()
};

#endif
