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

#ifndef QUERY_THREAD_H_INCLUDED
#define QUERY_THREAD_H_INCLUDED

#include <wx/thread.h>
#include <wx/event.h>
#include <wx/msgqueue.h>

#include "net_packet.h"

DECLARE_EVENT_TYPE(wxEVT_THREAD_WORKER_SIGNAL, -1)

class QueryThread : public wxThread
{
public:
    typedef enum
    {
        Message_MIN = 0
        ,Run
        ,Exit
        ,Message_MAX
    } Message;

    typedef enum
    {
         Status_MIN = 0
        ,Running  
        ,Waiting
        ,Exiting
        ,Status_MAX
    } Status;

	QueryThread(wxEvtHandler* EventHandler);
	~QueryThread()
	{
	}

	QueryThread::Status GetStatus();

	void Signal(odalpapi::Server* QueryServer,
	            const std::string& Address,
	            const wxUint16 Port,
	            wxInt32 ServerIndex,
	            wxUint32 ServerTimeout,
	            wxInt8 Retries);

	void GracefulExit();

	virtual void* Entry();

	static int GetIdealThreadCount();

protected:
    void Post(const QueryThread::Message &);
    void Receive(QueryThread::Message &);
	void SetStatus(const QueryThread::Status &);

private:
	wxEvtHandler*      m_EventHandler;
	odalpapi::Server*  m_QueryServer;
	wxInt32            m_ServerIndex;
	wxUint32           m_ServerTimeout;
	wxInt8             m_Retries;
	std::string        m_Address;
	wxUint16           m_Port;

    wxMessageQueue<QueryThread::Message> m_Message;

    wxMutex m_StatusMutex;
    QueryThread::Status m_StatusMessage;
};

#endif // QUERY_THREAD_H_INCLUDED
