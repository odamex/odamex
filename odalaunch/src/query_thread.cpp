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
// DESCRIPTION:
//  Multi-Threaded Server Queries
//  AUTHOR: Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include <wx/msgdlg.h>
#include <wx/app.h>
#include <wx/fileconf.h>
#include <wx/log.h> 

#include "plat_utils.h"
#include "query_thread.h"
#include "oda_defs.h"

int NUM_THREADS;

QueryThread::QueryThread(wxEvtHandler* EventHandler) : wxThread(wxTHREAD_JOINABLE), m_EventHandler(EventHandler)
{
	if(Create() != wxTHREAD_NO_ERROR)
		wxLogFatalError("Could not create worker thread!");

	wxThread::Run();
}

// Inter-thread communication
// Message queue from main to worker thread
void QueryThread::Post(const QueryThread::Message &Msg)
{
    m_Message.Post(Msg);
}

void QueryThread::Receive(QueryThread::Message &Msg)
{
    m_Message.Receive(Msg);
}

// Status updates from worker to main thread
void QueryThread::SetStatus(const QueryThread::Status &Sts)
{
    wxMutexLocker ML(m_StatusMutex);
    
    m_StatusMessage = Sts;   
}

QueryThread::Status QueryThread::GetStatus()
{
    wxMutexLocker ML(m_StatusMutex);

    return m_StatusMessage;
}

// Closes the thread normally
void QueryThread::GracefulExit()
{
	// Set the exit code and signal thread to exit
	Post(QueryThread::Exit);

	// Wait until the thread has closed completely
	wxThread::Wait();
}

void QueryThread::Signal(odalpapi::Server* QueryServer, const std::string& Address, const wxUint16 Port, wxInt32 ServerIndex, wxUint32 ServerTimeout, wxInt8 Retries)
{
	// Allow threads to acquire a wait state
	while(GetStatus() != QueryThread::Waiting)
		wxThread::Sleep(5);

	m_QueryServer = QueryServer;
	m_ServerIndex = ServerIndex;
	m_ServerTimeout = ServerTimeout;
	m_Retries = Retries;
	m_Address = Address;
	m_Port = Port;

    Post(QueryThread::Run);
}

void* QueryThread::Entry()
{
	wxCommandEvent newEvent(wxEVT_THREAD_WORKER_SIGNAL, wxID_ANY);
	odalpapi::BufferedSocket Socket;
    int Index;
    QueryThread::Message Msg;
	
	// Keeps the thread alive, it will wait for commands instead of the
	// killing itself/creating itself overhead
	while(1)
	{
        // Wait for work to be posted
        SetStatus(QueryThread::Waiting);
        Receive(Msg);
        
        // Translate the posted command to a status update
        switch (Msg)
        {
            case QueryThread::Exit:
            {
                SetStatus(QueryThread::Exiting);
                return NULL;
            }
            break;

            case QueryThread::Run:
            {
                SetStatus(QueryThread::Running);
            }
            break;

            default:
                return NULL;
        }

        // Set the required data so we can query the server
		m_QueryServer->SetSocket(&Socket);
		m_QueryServer->SetAddress(m_Address, m_Port);
		m_QueryServer->SetRetries(m_Retries);

		// Query the server using Odalpapi
		Index = m_QueryServer->Query(m_ServerTimeout);

        // Set the required fields that is needed, so the caller thread can
        // process the server data
        newEvent.SetId(Index);
		newEvent.SetInt(m_ServerIndex);
		wxPostEvent(m_EventHandler, newEvent);
	}

	return NULL;
}

int QueryThread::GetIdealThreadCount()
{
	int ThreadCount;
    int ThreadMul, ThreadMax;

	// TODO: Replace with a better system
	{
        wxFileConfig ConfigInfo;

        ConfigInfo.Read(QRYTHREADMULTIPLIER, &ThreadMul, ODA_THRMULVAL);
        ConfigInfo.Read(QRYTHREADMAXIMUM, &ThreadMax, ODA_THRMAXVAL);
	}

	// Base number of threads on cpu count in the system (including cores)
	// and multiply that by a fixed value
	ThreadCount = wxThread::GetCPUCount();

	if(ThreadCount != -1)
		ThreadCount *= ThreadMul;

	return clamp(ThreadCount, ThreadMul, ThreadMax);
}
