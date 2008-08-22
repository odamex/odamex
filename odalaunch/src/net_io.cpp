// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2008 by The Odamex Team.
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
#include <wx/log.h>

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
	: recv_buf(NULL), send_buf(NULL), Socket(NULL)
{   
    // set pings
    SendPing = 0;
    RecvPing = 0;
       
    // create the memory streams
    ClearSendBuffer();
    ClearRecvBuffer();
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
        m_BadRead = false;
        
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

//
// wxInt32 BufferedSocket::Read32(wxInt32 &Int32)
//
// Read a 32bit value
wxInt32 BufferedSocket::Read32(wxInt32 &Int32)
{
    if (!CanRead(4))
    {
        wxLogDebug(_T("Read32: End of buffer reached!"));
        
        Int32 = 0;
        
        m_BadRead = true;
        
        return 0;
    }
    
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    Int32 = dis.Read32();
    
    return 1;
}

//
// wxInt32 BufferedSocket::Read32(wxUint32 &Uint32)
//
// Read a 32bit value
wxInt32 BufferedSocket::Read32(wxUint32 &Uint32)
{
    if (!CanRead(4))
    {
        wxLogDebug(_T("Read32: End of buffer reached!"));
        
        Uint32 = 0;
        
        m_BadRead = true;
        
        return 0;
    }
    
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    Uint32 = dis.Read32();
    
    return 1;
}

//
// wxInt32 BufferedSocket::Read16(wxInt16 &Int16)
//
// Read a 16bit value
wxInt32 BufferedSocket::Read16(wxInt16 &Int16)
{
    if (!CanRead(2))
    {
        wxLogDebug(_T("Read16: End of buffer reached!"));
        
        Int16 = 0;
        
        m_BadRead = true;
        
        return 0;
    }
        
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    Int16 = dis.Read16();
    
    return 1;
}

//
// wxInt32 BufferedSocket::Read16(wxUint16 &Uint16)
//
// Read an 16bit value
wxInt32 BufferedSocket::Read16(wxUint16 &Uint16)
{
    if (!CanRead(2))
    {
        wxLogDebug(_T("Read16: End of buffer reached!"));
        
        Uint16 = 0;
        
        m_BadRead = true;
        
        return 0;
    }
        
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    Uint16 = dis.Read16();
    
    return 1;
}

//
// wxInt32 BufferedSocket::Read8(wxInt8 &Int8)
//
// Read an 8bit value
wxInt32 BufferedSocket::Read8(wxInt8 &Int8)
{
    if (!CanRead(1))
    {
        wxLogDebug(_T("Read8: End of buffer reached!"));
        
        Int8 = 0;
        
        m_BadRead = true;
        
        return 0;
    }
        
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    Int8 = dis.Read8();
    
    return 1;
}

//
// wxInt32 BufferedSocket::Read8(wxUint8 &Uint8)
//
// Read an 8bit value
wxInt32 BufferedSocket::Read8(wxUint8 &Uint8)
{
    if (!CanRead(1))
    {
        wxLogDebug(_T("Read8: End of buffer reached!"));
        
        Uint8 = 0;
        
        m_BadRead = true;
        
        return 0;
    }
        
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    Uint8 = dis.Read8();
    
    return 1;
}

//
// wxInt32 BufferedSocket::ReadBool(bool &boolval)
//
// Read a bool value
wxInt32 BufferedSocket::ReadBool(bool &val)
{
    if (!CanRead(1))
    {
        wxLogDebug(_T("ReadBool: End of buffer reached!"));
        
        val = false;
        
        m_BadRead = true;
        
        return 0;
    }
        
    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    wxInt8 Value = 0;

    Value = dis.Read8();
    
    if (Value < 0 || Value > 1)
    {
        wxLogDebug(_T("ReadBool: Value is not 0 or 1, possibly corrupted packet"));
        
        val = false;
        
        m_BadRead = true;
        
        return 0;
    }
    
    val = Value ? true : false;
    
    return 1;
}

//
// wxInt32 BufferedSocket::ReadString(wxString &str)
//
// Read a null terminated string
wxInt32 BufferedSocket::ReadString(wxString &str)
{
    if (!CanRead(1))
    {
        wxLogDebug(_T("ReadString: End of buffer reached!"));
        
        str = wxT("");
        
        m_BadRead = true;
        
        return 0;
    }

    wxDataInputStream dis(*recv_buf);
    dis.BigEndianOrdered(BigEndian);

    wxString in_str;
    
    // ooh, a priming read!
    wxChar ch = (wxChar)dis.Read8();

    bool IsRead = CanRead(1);
    
    while (ch != '\0' && IsRead)
    {      
        in_str << ch;
        
        ch = (wxChar)dis.Read8();
        
        IsRead = CanRead(1);
    }
    
    if (!IsRead)
    {
        wxLogDebug(_T("ReadString: End of buffer reached!"));
        
        str = wxT("");
        
        m_BadRead = true;
        
        return 0;
    }
    
    str = in_str;
    
    return 1;
}

//
// void BufferedSocket::Write32(const wxInt32 &val)
//
// Write a 32bit value
void BufferedSocket::Write32(const wxInt32 &val)
{
    if (!CanWrite(4))
    {
        wxLogDebug(_T("Write32: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }
    
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write32(val);
}

//
// void BufferedSocket::Write32(const wxUint32 &val)
//
// Write a 32bit value
void BufferedSocket::Write32(const wxUint32 &val)
{
    if (!CanWrite(4))
    {
        wxLogDebug(_T("Write32: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }
    
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write32(val);
}

//
// void BufferedSocket::Write16(const wxInt16 &val)
//
// Write a 16bit value
void BufferedSocket::Write16(const wxInt16 &val)
{
    if (!CanWrite(2))
    {
        wxLogDebug(_T("Write16: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }
    
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write16(val);
}

//
// void BufferedSocket::Write16(const wxUint16 &val)
//
// Write a 16bit value
void BufferedSocket::Write16(const wxUint16 &val)
{
    if (!CanWrite(2))
    {
        wxLogDebug(_T("Write16: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }
    
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write16(val);
}

//
// void BufferedSocket::Write8(const wxInt8 &val)
//
// Write an 8bit value
void BufferedSocket::Write8(const wxInt8 &val)
{
    if (!CanWrite(1))
    {
        wxLogDebug(_T("Write8: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }

    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write8(val);
}

//
// void BufferedSocket::Write8(const wxUint8 &val)
//
// Write an 8bit value
void BufferedSocket::Write8(const wxUint8 &val)
{
    if (!CanWrite(1))
    {
        wxLogDebug(_T("Write8: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }

    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write8(val);
}

//
// void BufferedSocket::WriteBool(const bool &val)
//
// Write a boolean value
void BufferedSocket::WriteBool(const bool &val)
{
    if (!CanWrite(1))
    {
        wxLogDebug(_T("WriteBool: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }
        
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos.Write8((val ? 1 : 0));
}

//
// void BufferedSocket::WriteString(const wxString &str)
//
// Write a null terminated string
void BufferedSocket::WriteString(const wxString &str)
{
    if (!CanWrite((size_t)str.Length()))
    {
        wxLogDebug(_T("WriteString: End of buffer reached!"));
        
        m_BadWrite = true;
        
        return;
    }
        
    wxDataOutputStream dos(*send_buf);
    dos.BigEndianOrdered(BigEndian);
    
    dos << str.c_str();
}

wxString BufferedSocket::GetAddress() 
{
    wxString retstr;
    
    retstr << to_addr.IPAddress() << _T(":") << wxString::Format(_T("%u"),to_addr.Service());
    
    return retstr;

}

//
// bool BufferedSocket::SetAddress(const wxString &Address)
//
// Sets the outgoing address in "address:port" format, fails if no colon is 
// present
bool BufferedSocket::SetAddress(const wxString &Address)
{
    wxInt32 Colon = Address.Find(wxT(':'), true);
    
    if (Colon == wxNOT_FOUND)
        return false;
    
    wxUint16 Port = wxAtoi(Address.Mid(Colon));
    wxString HostIP = Address.Mid(0, Colon);
    
    SetAddress(HostIP, Port);
    
    return true;
}

void BufferedSocket::ClearRecvBuffer() 
{       
    if (recv_buf != NULL)
        delete recv_buf;
    
    m_BadRead = false;
    
    recv_buf = new wxMemoryInputStream(NULL, 0);
}

void BufferedSocket::ClearSendBuffer() 
{ 
    if (send_buf != NULL)
        delete send_buf;           
    
    m_BadWrite = false;
    
    send_buf = new wxMemoryOutputStream();
}

//
// bool BufferedSocket::CanRead(const size_t &Bytes)
//
// Tells us if we can read X amount of bytes from the receive buffer
bool BufferedSocket::CanRead(const size_t &Bytes)
{
    if (recv_buf == NULL)
    {
        wxLogDebug(_T("CanRead: recv_buf is NULL!"));
        
        return false;
    }
    
    return ((recv_buf->TellI() + Bytes) <= recv_buf->GetSize());
}

//
// bool BufferedSocket::CanWrite(const size_t &Bytes)
//
// Tells us if we can write X amount of bytes to the send buffer
bool BufferedSocket::CanWrite(const size_t &Bytes)
{
    if (send_buf == NULL)
    {
        wxLogDebug(_T("CanWrite: send_buf is NULL!"));
        
        return false;
    }
    
    // [Russell] - hack, you cant set the internal maximum size of a 
    // wxMemoryOutputStream... what?
    return ((send_buf->TellO() + Bytes) <= MAX_PAYLOAD);
}
