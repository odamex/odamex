// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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
//	Low-level socket and buffer class
//	AUTHOR:	Russell Rice (russell at odamex dot net)
//
//-----------------------------------------------------------------------------


#include "net_io.h"

#include <wx/msgdlg.h>

// Endianess switch
const wxByte BufferedSocket::BigEndian = false;

// we need to do something with this, one day
void BufferedSocket::CheckError()
{   
/*
    if (Socket->Error())
        switch(Socket->LastError())
        {
            case wxSOCKET_INVOP:
                wxMessageBox(_T("Error: Invalid Operation"));
                break;
            case wxSOCKET_IOERR:
                wxMessageBox(_T("Error: I/O Error"));
                break;
            case wxSOCKET_INVADDR:
                wxMessageBox(_T("Error: Invalid address passed to Socket"));
                break;
            case wxSOCKET_INVSOCK:
                wxMessageBox(_T("Error: Invalid socket (uninitialized)."));
                break;                
            case wxSOCKET_NOHOST:
                wxMessageBox(_T("Error: No corresponding host."));
                break;
            case wxSOCKET_INVPORT:
                wxMessageBox(_T("Error: Invalid port."));
                break;
            case wxSOCKET_WOULDBLOCK:
                wxMessageBox(_T("Error: The socket is non-blocking and the operation would block."));
                break;
            case wxSOCKET_TIMEDOUT:
                wxMessageBox(_T("Error: The timeout for this operation expired."));
                break;
            case wxSOCKET_MEMERR:
                wxMessageBox(_T("Error: Memory exhausted."));    
                break;
        }*/
}

//  Constructor
BufferedSocket::BufferedSocket()
	: Socket(NULL)
{   
    // set pings
    SendPing = 0;
    RecvPing = 0;
       
    // create the memory streams
    send_buf = new wxMemoryOutputStream();
    recv_buf = new wxMemoryInputStream(NULL, 0);
  
}

//  Destructor
BufferedSocket::~BufferedSocket()
{
    if (send_buf)
        delete send_buf;
        
    if (recv_buf)
        delete recv_buf;       
}

void BufferedSocket::CreateSocket(void)
{
    local_addr.AnyAddress();   
	local_addr.Service(0);
		
    Socket = new wxDatagramSocket(local_addr, wxSOCKET_NONE);
        
    if(!Socket->IsOk())
    {
        Socket->Destroy();
        Socket = NULL;
        
        CheckError();
    }
		
    // get rid of any data in the receive queue    
    Socket->Discard();
    
    // no event handler for us
    Socket->Notify(false); 
}

void BufferedSocket::DestroySocket(void)
{
    if (Socket->IsOk())
    {
        Socket->Destroy();
        Socket = NULL;
    }
}

//  Write a socket
wxInt32 BufferedSocket::SendData(wxInt32 Timeout)
{   
    CreateSocket();

    // create a transfer buffer, from memory stream to socket
    wxStopWatch sw;

    // clear it
    memset(sData, 0, sizeof(sData));
    
    // copy data
    wxInt32 actual_size = send_buf->CopyTo(sData, MAX_PAYLOAD);
    
    // set the start ping
    // (Horrible, needs to be improved)
    SendPing = sw.Time();
                
    // send the data
    if (!Socket->WaitForWrite(0,Timeout))
    {
        CheckError();
        
        SendPing = 0;
        RecvPing = 0;
        
        DestroySocket();
        
        return 0;
    }
    else
        Socket->SendTo(to_addr, sData, actual_size);
    
    CheckError();    
        
    // return the amount of bytes sent
    return Socket->LastCount();
}

//  Read a socket
wxInt32 BufferedSocket::GetData(wxInt32 Timeout)
{   
    // temporary address, for comparison
    wxIPV4address        in_addr;
    
    wxStopWatch sw;

    // clear it
    memset(rData, 0, sizeof(rData));
    
    if (!Socket->WaitForRead(0, Timeout))
    {
        CheckError();
        
        SendPing = 0;
        RecvPing = 0;
        
        DestroySocket();
        
        return 0;
    }
    else
        Socket->RecvFrom(in_addr, rData, MAX_PAYLOAD);

    CheckError();

    // apply the receive ping
    // (Horrible, needs to be improved)
    RecvPing = sw.Time();

    // get number of bytes received
    wxInt32 num_recv = Socket->LastCount();

    // clear the socket queue
    Socket->Discard();

    // ensure received address is the same as our remote address
    if (num_recv > 0)
    if ((in_addr.IPAddress() == to_addr.IPAddress()) && (in_addr.Service() == to_addr.Service()))
    {
    
        // write data to memory stream, if one doesn't already exist
        if (recv_buf != NULL)
        {
            delete recv_buf;
            
            recv_buf = new wxMemoryInputStream(rData, num_recv);
        
        }
        else
           recv_buf = new wxMemoryInputStream(rData, num_recv); 

        DestroySocket();

        // return bytes received   
        return num_recv;
    }
      
    SendPing = 0;
    RecvPing = 0;
    
    DestroySocket();
    
    return 0;
}

//  Read a 32bit value from a packet
wxInt32 BufferedSocket::Read32()
{
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    return dis.Read32();
}

//  Read a 16bit value from a packet
wxInt16 BufferedSocket::Read16()
{
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    return dis.Read16();
}

//  Read an 8bit value from a packet
wxInt8 BufferedSocket::Read8()
{
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    return dis.Read8();
}

//  Read a string from a packet
wxChar *BufferedSocket::ReadString()
{
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    static wxChar str[2048];
    
    memset(str, 0, sizeof(str));
    
    // ooh, a priming read!
    wxChar ch = (wxChar)dis.Read8();

    wxUint16 i = 0;
    
    while (((ch != '\0') && (i < sizeof(str)-1)))
    {
        str[i] = ch;
        
        ch = (wxChar)dis.Read8();
    
        i++;
    }
    
    str[i] = '\0';
    
    return str;
}

//  Write a 32bit value to a packet
void BufferedSocket::Write32(wxInt32 val)
{
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write32(val);
        
    return;
}

//  Write a 16bit value to a packet
void BufferedSocket::Write16(wxInt16 val)
{
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write16(val);
        
    return;
}

//  Write an 8bit value to a packet
void BufferedSocket::Write8(wxInt8 val)
{
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write8(val);
        
    return;
}
/*
wxInt32 BufferedSocket::SetAddress(wxString AddressAndPort)
{
    wxStringTokenizer tokstr(AddressAndPort, _T(':'));
	wxString inAddr, inPort;

    if (tokstr.HasMoreTokens())
        inAddr = tokstr.GetNextToken();
    else
        return -1;
        
    if (tokstr.HasMoreTokens())
        inPort = tokstr.GetNextToken();
    else
        return -1;
        
    to_addr.Hostname(inAddr);
    to_addr.Service(inPort);
    
    return 0;
}
*/
wxString BufferedSocket::GetAddress() 
{
    wxString retstr;
    
    retstr << to_addr.IPAddress() << _T(":") << wxString::Format(_T("%d"),to_addr.Service());
    
    return retstr;

}

void BufferedSocket::ClearRecvBuffer() 
{       
    if (recv_buf != NULL)
    {
        delete recv_buf;
            
        recv_buf = new wxMemoryInputStream(NULL, 0);
        
    }
}

void BufferedSocket::ClearSendBuffer() 
{ 
    if (send_buf != NULL)
    {
        delete send_buf;
            
        send_buf = new wxMemoryOutputStream();

    }
}
