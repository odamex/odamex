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
//
// AUTHORS: 
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------


#include "net_io.h"

#include <wx/msgdlg.h>
#include <wx/log.h>

// Endianess switch
const wxByte BufferedSocket::BigEndian = false;

// Initialize and shutdown functions for system-specific 
bool InitializeSocketAPI()
{
#ifdef __WXMSW__
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
    {
        wxSafeShowMessage(wxT("Error"), wxT("Could not initialize winsock!"));
    
        return false;
    }
#endif

    return true;
}

void ShutdownSocketAPI()
{
    #ifdef __WXMSW__
    WSACleanup();
    #endif
}

// we need to do something with this, one day
wxUint32 BufferedSocket::CheckError()
{      
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
    DestroySocket();

    Socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(Socket == 0)
    {
        CheckError();
        return false;
    }

    if (connect(Socket, 
                (struct sockaddr *)&m_RemoteAddress, 
                sizeof(m_RemoteAddress)) == -1)
    {
        CheckError();
        return false;
    }

    return true;
}

void BufferedSocket::DestroySocket()
{
    if (Socket != 0)
    {
        // msvc
        #ifdef __WXMSW__
        if (closesocket(Socket) != 0)
        #else
        if (close(Socket) != 0)
        #endif
            wxLogDebug(wxString::Format(wxT("Could not close socket: %d"), Socket));
            
        Socket = 0;
    }
}

//  Write a socket
wxInt32 BufferedSocket::SendData(const wxInt32 &Timeout)
{   
    wxInt32 BytesSent;
    
    if (CreateSocket() == false)
        return 0;

    // create a transfer buffer, from memory stream to socket
    wxStopWatch sw;

    // clear it
    memset(m_SendBuffer, 0, sizeof(m_SendBuffer));
    
    // copy data
    size_t WrittenSize = m_SendBufferHandler->CopyTo(m_SendBuffer, 
                                                      sizeof(m_SendBuffer));
    
    // set the start ping
    // (Horrible, needs to be improved)
    m_SendPing = sw.Time();
                
    // send the data
    BytesSent = send(Socket, m_SendBuffer, WrittenSize, 0);
    
    CheckError();    
        
    // return the amount of bytes sent
    return BytesSent;
}

//  Read a socket
wxInt32 BufferedSocket::GetData(const wxInt32 &Timeout)
{   
    int              fromlen;
    wxInt32          res;
    fd_set           readfds;
    struct timeval   tv;  
    wxStopWatch      sw;

    // clear it
    memset(m_ReceiveBuffer, 0, sizeof(m_ReceiveBuffer));

    wxInt32 ReceivedSize = 0;

    bool DestroyMe = false;
    
    // Wait for read with timeout
    if(Timeout >= 0)
    {
        FD_ZERO(&readfds);
        FD_SET(Socket, &readfds);
        tv.tv_sec = Timeout / 1000;
        tv.tv_usec = (Timeout % 1000) * 1000; // convert milliseconds to microseconds
        res = select(Socket+1, &readfds, NULL, NULL, &tv);
        if(res == -1)
        {
            CheckError();

            DestroyMe = true;
        }
        if(res == 0) // Read Timed Out
            DestroyMe = true;
    }
    
    if (DestroyMe == true)
    {
        m_SendPing = 0;
        m_ReceivePing = 0;
        
        CheckError();
        
        DestroySocket();
        
        return 0;
    }

    ReceivedSize = recv(Socket, m_ReceiveBuffer, sizeof(m_ReceiveBuffer), 0);
                            
    if(ReceivedSize <= 0)
    {
        // -1 = Error; 0 = Closed Connection
        if(ReceivedSize == -1)
            CheckError();
            
        m_SendPing = 0;
        m_ReceivePing = 0;
        
        DestroySocket();
        
        return 0;
    }

    CheckError();

    // apply the receive ping
    m_ReceivePing = sw.Time();

    // ensure received address is the same as our remote address
    if (ReceivedSize > 0)
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
    struct hostent *he;

    if((he = gethostbyname((const char *)Address.char_str())) == NULL)
    {
        CheckError();
        return;
    }

    m_RemoteAddress.sin_family = AF_INET;
    m_RemoteAddress.sin_port = htons(Port);
    m_RemoteAddress.sin_addr = *((struct in_addr *)he->h_addr);
    memset(m_RemoteAddress.sin_zero, '\0', sizeof m_RemoteAddress.sin_zero);
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
    Address = wxString::Format(_T("%s"),inet_ntoa(m_RemoteAddress.sin_addr));
    Port = ntohs(m_RemoteAddress.sin_port);
}

//
// wxString BufferedSocket::GetRemoteAddress()
//
// Gets the outgoing address in "address:port" format
wxString BufferedSocket::GetRemoteAddress()
{
    return wxString::FromAscii(inet_ntoa(m_RemoteAddress.sin_addr)) << 
            _T(":") << 
            wxString::Format(_T("%d"),ntohs(m_RemoteAddress.sin_port));
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
    
    // [Russell] - hack, you cant get the internal maximum size of a 
    // wxMemoryOutputStream... what?
    return ((m_SendBufferHandler->TellO() + Bytes) <= MAX_PAYLOAD);
}
