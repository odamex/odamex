// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2009 by The Odamex Team.
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
wxUint32 BufferedSocket::CheckError()
{   
    if (Socket->Error())
    {
        switch(Socket->LastError())
        {
            case wxSOCKET_DUMMY:
            case wxSOCKET_NOERROR:
            {
                return 0;
            }
            break;
            
            case wxSOCKET_INVOP:
            {
                wxLogDebug(_T("Error: Invalid Operation"));
                return 1;
            }
            break;
            
            case wxSOCKET_IOERR:
            {
                wxLogDebug(_T("Error: I/O Error"));
                return 2;
            }
            break;
            
            case wxSOCKET_INVADDR:
            {
                wxLogDebug(_T("Error: Invalid address passed to Socket"));
                return 3;
            }
            break;
            
            case wxSOCKET_INVSOCK:
            {
                wxLogDebug(_T("Error: Invalid socket (uninitialized)."));
                return 4;
            }
            break;                
            
            case wxSOCKET_NOHOST:
            {
                wxLogDebug(_T("Error: No corresponding host."));
                return 5;
            }
            break;
            
            case wxSOCKET_INVPORT:
            {
                wxLogDebug(_T("Error: Invalid port."));
                return 6;
            }
            break;
            
            case wxSOCKET_WOULDBLOCK:
            {
                wxLogDebug(_T("Error: The socket is non-blocking and the operation would block."));
                return 0;
            }
            break;
            
            case wxSOCKET_TIMEDOUT:
            {
                wxLogDebug(_T("Error: The timeout for this operation expired."));
                return 7;
            }
            break;
            
            case wxSOCKET_MEMERR:
            {
                wxLogDebug(_T("Error: Memory exhausted."));
                return 8;
            }
            break;
        }
    }
    
    return 0;
}

//  Constructor
BufferedSocket::BufferedSocket()
	: m_ReceiveBufferHandler(NULL), m_SendBufferHandler(NULL), Socket(NULL)
{   
    // set pings
    m_SendPing = 0;
    m_ReceivePing = 0;
       
    // create the memory streams
    ClearSendBuffer();
    ClearRecvBuffer();
}

//  Destructor
BufferedSocket::~BufferedSocket()
{
    if (m_SendBufferHandler)
    {
        delete m_SendBufferHandler;
        m_SendBufferHandler = NULL;
    }
    
    if (m_ReceiveBufferHandler)
    {
        delete m_ReceiveBufferHandler;
        m_ReceiveBufferHandler = NULL;
    }
        
    DestroySocket();
}

bool BufferedSocket::CreateSocket()
{
    m_LocalAddress.AnyAddress();   
	m_LocalAddress.Service(0);

    DestroySocket();

    Socket = new wxDatagramSocket(m_LocalAddress, wxSOCKET_NONE);
    
    if(!Socket->IsOk())
    {
        CheckError();
        
        DestroySocket();
        
        return false;
    }
		
    // get rid of any data in the receive queue    
    Socket->Discard();
    
    // no event handler for us
    Socket->Notify(false);
    
    return true;
}

void BufferedSocket::DestroySocket()
{
    if (Socket)
    {
        Socket->Destroy();
        Socket = NULL;
    }
}

//  Write a socket
wxInt32 BufferedSocket::SendData(const wxInt32 &Timeout)
{   
    if (!CreateSocket())
        return 0;

    // create a transfer buffer, from memory stream to socket
    wxStopWatch sw;

    // clear it
    memset(m_SendBuffer, 0, sizeof(m_SendBuffer));
    
    // copy data
    size_t WrittenSize = m_SendBufferHandler->CopyTo(m_SendBuffer, MAX_PAYLOAD);
    
    // set the start ping
    // (Horrible, needs to be improved)
    m_SendPing = sw.Time();
                
    // send the data
    Socket->SendTo(m_RemoteAddress, m_SendBuffer, WrittenSize);
    
    CheckError();    
        
    // return the amount of bytes sent
    return Socket->LastCount();
}

//  Read a socket
wxInt32 BufferedSocket::GetData(const wxInt32 &Timeout)
{   
    // temporary address, for comparison
    wxIPV4address ReceivedAddress;
    
    wxStopWatch sw;

    // clear it
    memset(m_ReceiveBuffer, 0, sizeof(m_ReceiveBuffer));

    wxUint32 ReceivedSize = 0;
    wxInt32 TimeLeft = 0;

    bool DestroyMe = false;
    
    if (Socket->WaitForRead(0, Timeout) == true)
    {
        Socket->RecvFrom(ReceivedAddress, m_ReceiveBuffer, MAX_PAYLOAD);

        ReceivedSize = Socket->LastCount();
    }
    else
        DestroyMe = true;
    
    if (ReceivedSize == 0 || DestroyMe == true)
    {
        CheckError();
        
        m_SendPing = 0;
        m_ReceivePing = 0;
        
        DestroySocket();
        
        return 0;
    }

    CheckError();

    // apply the receive ping
    // (Horrible, needs to be improved)
    m_ReceivePing = sw.Time();

    // clear the socket queue
    Socket->Discard();

    // ensure received address is the same as our remote address
    if (ReceivedSize > 0)
    if ((ReceivedAddress.IPAddress() == m_RemoteAddress.IPAddress()) && 
        (ReceivedAddress.Service() == m_RemoteAddress.Service()))
    {
        m_BadRead = false;
        
        // write data to memory stream, if one doesn't already exist
        if (m_ReceiveBufferHandler != NULL)
        {
            delete m_ReceiveBufferHandler;
            
            m_ReceiveBufferHandler = new wxMemoryInputStream(m_ReceiveBuffer, ReceivedSize);
        }
        else
           m_ReceiveBufferHandler = new wxMemoryInputStream(m_ReceiveBuffer, ReceivedSize); 

        DestroySocket();

        // return bytes received   
        return ReceivedSize;
    }
      
    m_SendPing = 0;
    m_ReceivePing = 0;
    
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
    
    wxDataInputStream dis(*m_ReceiveBufferHandler);
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
    
    wxDataInputStream dis(*m_ReceiveBufferHandler);
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
        
    wxDataInputStream dis(*m_ReceiveBufferHandler);
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
        
    wxDataInputStream dis(*m_ReceiveBufferHandler);
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
        
    wxDataInputStream dis(*m_ReceiveBufferHandler);
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
        
    wxDataInputStream dis(*m_ReceiveBufferHandler);
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
        
    wxDataInputStream dis(*m_ReceiveBufferHandler);
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

    wxDataInputStream dis(*m_ReceiveBufferHandler);
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
    
    wxDataOutputStream dos(*m_SendBufferHandler);
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
    
    wxDataOutputStream dos(*m_SendBufferHandler);
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
    
    wxDataOutputStream dos(*m_SendBufferHandler);
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
    
    wxDataOutputStream dos(*m_SendBufferHandler);
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

    wxDataOutputStream dos(*m_SendBufferHandler);
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

    wxDataOutputStream dos(*m_SendBufferHandler);
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
        
    wxDataOutputStream dos(*m_SendBufferHandler);
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
        
    wxDataOutputStream dos(*m_SendBufferHandler);
    dos.BigEndianOrdered(BigEndian);
    
    dos << str.c_str();
}

//
// bool BufferedSocket::SetRemoteAddress(const wxString &Address)
//
// Sets the outgoing address
void BufferedSocket::SetRemoteAddress(const wxString &Address, const wxInt16 &Port)
{
    m_RemoteAddress.Hostname(Address); 
    m_RemoteAddress.Service(Port); 
}

//
// bool BufferedSocket::SetRemoteAddress(const wxString &Address)
//
// Sets the outgoing address in "address:port" format, fails if no colon is 
// present
bool BufferedSocket::SetRemoteAddress(const wxString &Address)
{
    wxInt32 Colon = Address.Find(wxT(':'), true);
    
    if (Colon == wxNOT_FOUND)
        return false;
    
    wxUint16 Port = wxAtoi(Address.Mid(Colon));
    wxString HostIP = Address.Mid(0, Colon);
    
    SetRemoteAddress(HostIP, Port);
    
    return true;
}

//
// void BufferedSocket::GetRemoteAddress(wxString &Address, wxUint16 &Port)
//
// Gets the outgoing address
void BufferedSocket::GetRemoteAddress(wxString &Address, wxUint16 &Port)
{
    Address = m_RemoteAddress.IPAddress();
    Port = m_RemoteAddress.Service();
}

//
// wxString BufferedSocket::GetRemoteAddress()
//
// Gets the outgoing address in "address:port" format
wxString BufferedSocket::GetRemoteAddress()
{
    return m_RemoteAddress.IPAddress() << _T(":") << wxString::Format(_T("%u"),m_RemoteAddress.Service());
}

void BufferedSocket::ClearRecvBuffer() 
{       
    if (m_ReceiveBufferHandler != NULL)
    {
        delete m_ReceiveBufferHandler;
        m_ReceiveBufferHandler = NULL;
    }
    
    m_BadRead = false;
    
    m_ReceiveBufferHandler = new wxMemoryInputStream(NULL, 0);
}

void BufferedSocket::ClearSendBuffer() 
{ 
    if (m_SendBufferHandler != NULL)
    {
        delete m_SendBufferHandler;
        m_SendBufferHandler = NULL;
    }
    
    m_BadWrite = false;
    
    m_SendBufferHandler = new wxMemoryOutputStream();
}

//
// bool BufferedSocket::CanRead(const size_t &Bytes)
//
// Tells us if we can read X amount of bytes from the receive buffer
bool BufferedSocket::CanRead(const size_t &Bytes)
{
    if (m_ReceiveBufferHandler == NULL)
    {
        wxLogDebug(_T("CanRead: m_ReceiveBufferHandler is NULL!"));
        
        return false;
    }
    
    return ((m_ReceiveBufferHandler->TellI() + Bytes) <= m_ReceiveBufferHandler->GetSize());
}

//
// bool BufferedSocket::CanWrite(const size_t &Bytes)
//
// Tells us if we can write X amount of bytes to the send buffer
bool BufferedSocket::CanWrite(const size_t &Bytes)
{
    if (m_SendBufferHandler == NULL)
    {
        wxLogDebug(_T("CanWrite: m_SendBufferHandler is NULL!"));
        
        return false;
    }
    
    // [Russell] - hack, you cant set the internal maximum size of a 
    // wxMemoryOutputStream... what?
    return ((m_SendBufferHandler->TellO() + Bytes) <= MAX_PAYLOAD);
}
