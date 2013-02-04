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

#ifndef QUERY_THREAD_H_INCLUDED
#define QUERY_THREAD_H_INCLUDED

#include <wx/thread.h>
#include <wx/event.h>

#include "net_packet.h"

DECLARE_EVENT_TYPE(wxEVT_THREAD_WORKER_SIGNAL, -1);

#define NUM_THREADS 10

class QueryThread : public wxThread
{
    public:

        QueryThread();
        QueryThread(wxEvtHandler *EventHandler, odalpapi::Server *QueryServer, wxInt32 ServerIndex, wxUint32 ServerTimeout, wxInt8 Retries) : wxThread(wxTHREAD_JOINABLE),
            m_EventHandler(EventHandler), m_QueryServer(QueryServer), m_ServerIndex(ServerIndex), m_ServerTimeout(ServerTimeout), m_Retries(Retries) {}

        virtual void *Entry();

    private:
        wxEvtHandler      *m_EventHandler;
        odalpapi::Server  *m_QueryServer;
        wxInt32            m_ServerIndex;
        wxUint32           m_ServerTimeout;
        wxInt8             m_Retries;
};

#endif // QUERY_THREAD_H_INCLUDED
