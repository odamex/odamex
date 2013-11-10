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

#include "typedefs.h"

#include "net_packet.h"

// Thread multiplier value
#define ODA_THRMULVAL 8

// Default number of threads for single processor/core systems
#define ODA_THRDEFVAL 10 

DECLARE_EVENT_TYPE(wxEVT_THREAD_WORKER_SIGNAL, -1);

typedef enum QueryThreadStatus
{
     QueryThread_MIN = 0
    ,QueryThread_Running
    ,QueryThread_Waiting
    ,QueryThread_Exiting
    ,QueryThread_MAX
} QueryThreadStatus_t;

class QueryThread : public wxThread
{
    public:

        QueryThread(wxEvtHandler *EventHandler);
        ~QueryThread() { delete m_Condition; }
        inline QueryThreadStatus_t GetStatus() { return m_Status; };

        void Signal(odalpapi::Server *QueryServer, 
                    const std::string &Address, 
                    const wxUint16 Port, 
                    wxInt32 ServerIndex, 
                    wxUint32 ServerTimeout, 
                    wxInt8 Retries);

        void GracefulExit();
        
        virtual void *Entry();

    private:
        wxEvtHandler      *m_EventHandler;
        odalpapi::Server  *m_QueryServer;
        wxInt32            m_ServerIndex;
        wxUint32           m_ServerTimeout;
        wxInt8             m_Retries;
        std::string        m_Address;
        wxUint16 m_Port;


        QueryThreadStatus_t m_Status;

        wxMutex            m_Mutex;
        wxCondition        *m_Condition;
};

#endif // QUERY_THREAD_H_INCLUDED
