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

#include "plat_utils.h"
#include "query_thread.h"
#include "oda_defs.h"

int NUM_THREADS;

QueryThread::QueryThread(wxEvtHandler* EventHandler) : wxThread(wxTHREAD_JOINABLE), m_EventHandler(EventHandler)
{
	if(Create() != wxTHREAD_NO_ERROR)
	{
		wxMessageBox("Could not create worker thread!",
		             "Error",
		             wxOK | wxICON_ERROR);

		wxExit();
	}

	Run();
}

void QueryThread::SetStatus(QueryThreadStatus_t Status)
{
	wxMutexLocker MutexLocker(m_StatusMutex);

	m_Status = Status;
}

QueryThreadStatus_t QueryThread::GetStatus()
{
	QueryThreadStatus_t Status;

	wxMutexLocker MutexLocker(m_StatusMutex);

	Status = m_Status;

	return Status;
}

void QueryThread::GracefulExit()
{
	// Allow threads to acquire a wait state
	while(GetStatus() != QueryThread_Waiting)
		Sleep(5);

	// Set the exit code and signal thread to exit
	SetStatus(QueryThread_Exiting);

	m_Semaphore.Post();

	// Wait until the thread has closed completely
	Wait();
}

void QueryThread::Signal(odalpapi::Server* QueryServer, const std::string& Address, const wxUint16 Port, wxInt32 ServerIndex, wxUint32 ServerTimeout, wxInt8 Retries)
{
	m_QueryServer = QueryServer;
	m_ServerIndex = ServerIndex;
	m_ServerTimeout = ServerTimeout;
	m_Retries = Retries;
	m_Address = Address;
	m_Port = Port;

    m_Semaphore.Post();
}

void* QueryThread::Entry()
{
	wxCommandEvent newEvent(wxEVT_THREAD_WORKER_SIGNAL, wxID_ANY);
	odalpapi::BufferedSocket Socket;

	// Keeps the thread alive, it will wait for commands instead of the
	// killing itself/creating itself overhead
	while(1)
	{
		SetStatus(QueryThread_Waiting);

		// Put the thread to sleep and wait for a signal
		m_Semaphore.Wait();

        if(GetStatus() == QueryThread_Exiting)
			break;

		// We got signaled to do some work, so lets do it
		SetStatus(QueryThread_Running);

		m_QueryServer->SetSocket(&Socket);
		m_QueryServer->SetAddress(m_Address, m_Port);

		m_QueryServer->SetRetries(m_Retries);

		newEvent.SetId(m_QueryServer->Query(m_ServerTimeout));
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
