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
//	Multi-Threaded Server Queries
//	AUTHOR:	Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <wx/msgdlg.h>
#include <wx/app.h>

#include "query_thread.h"

int NUM_THREADS;

QueryThread::QueryThread(wxEvtHandler *EventHandler) : wxThread(wxTHREAD_JOINABLE), m_EventHandler(EventHandler)
{
    if (Create() != wxTHREAD_NO_ERROR)
    {
        wxMessageBox(_T("Could not create worker thread!"),
                     _T("Error"),
                     wxOK | wxICON_ERROR);

        wxExit();
    }

    m_Condition = new wxCondition(m_Mutex);

    Run();
}

void QueryThread::GracefulExit()
{
    // Allow threads to acquire a wait state
    while (m_Status != QueryThread_Waiting)
        Sleep(5);

    // Set the exit code and signal thread to exit
    m_Status = QueryThread_Exiting;

    m_Condition->Signal();

    // Wait until the thread has closed completely
    Wait();
}

void QueryThread::Signal( odalpapi::Server *QueryServer, const std::string &Address, const wxUint16 Port, wxInt32 ServerIndex, wxUint32 ServerTimeout, wxInt8 Retries)
{
    m_QueryServer = QueryServer;
    m_ServerIndex = ServerIndex;
    m_ServerTimeout = ServerTimeout;
    m_Retries = Retries; 
    m_Address = Address;
    m_Port = Port;

    m_Condition->Signal();
}

void *QueryThread::Entry()
{   
    wxCommandEvent newEvent(wxEVT_THREAD_WORKER_SIGNAL, wxID_ANY );
    odalpapi::BufferedSocket Socket;

    // Keeps the thread alive, it will wait for commands instead of the
    // killing itself/creating itself overhead
    while (1)
    {
        m_Status = QueryThread_Waiting;

        m_Mutex.Lock();

        // Put the thread to sleep and wait for a signal
        m_Condition->Wait();

        // We got signaled to do some work, so lets do it
        if (m_Status == QueryThread_Exiting)
            break;

        m_Status = QueryThread_Running;

        m_QueryServer->SetSocket(&Socket);
        m_QueryServer->SetAddress(m_Address, m_Port);
        
        m_QueryServer->SetRetries(m_Retries);

        newEvent.SetId(m_QueryServer->Query(m_ServerTimeout));
        newEvent.SetInt(m_ServerIndex);
        wxPostEvent(m_EventHandler, newEvent);
    }
    
    return NULL;
}
