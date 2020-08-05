// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	HTTP Downloading.
//
//-----------------------------------------------------------------------------

#include "i_http.h"

#include "i_sdl.h"
#include "curl/curl.h"

#include "i_system.h"

namespace http
{

static bool init = false;

void Init()
{
    Printf(PRINT_HIGH, "http::Init: Init HTTP subsystem (%s)\n", curl_version());
    curl_global_init(CURL_GLOBAL_ALL);
    ::http::init = true;
}

void Shutdown()
{
    if (::http::init == false)
        return;

    curl_global_cleanup();
    ::http::init = false;
}

void Tick()
{

}

void Cancel()
{

}

}
