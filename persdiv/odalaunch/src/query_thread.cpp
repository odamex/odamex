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

#include "query_thread.h"

void *QueryThread::Entry()
{   
    wxCommandEvent newEvent(wxEVT_THREAD_WORKER_SIGNAL, wxID_ANY );
    
    m_QueryServer->SetRetries(m_Retries);

    newEvent.SetId(m_QueryServer->Query(m_ServerTimeout));
    newEvent.SetInt(m_ServerIndex);
    wxPostEvent(m_EventHandler, newEvent);

    return NULL;
}
