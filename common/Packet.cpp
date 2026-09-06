// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  The Packet assembler and sender class
//
//-----------------------------------------------------------------------------
#include "Packet.h"

#include "SequenceSender.h"

Packet::Packet() :
	m_outgoingPacketBuffer {MAX_UDP_PACKET}
{
	Reset();
}

void Packet::Reset()
{
	m_header = PacketHeaderType();
	m_outgoingPacketBuffer.clear();
	m_header.Pack(m_outgoingPacketBuffer);  // Make sure we always reserve the header.
}

size_t Packet::AddToOutgoingBuffer(const buf_t& i_dataBuffer)
{
	if (m_outgoingPacketBuffer.size() + i_dataBuffer.size() > MAX_UDP_SIZE)
	{
		return 0;
	}

	m_outgoingPacketBuffer.WriteChunk(i_dataBuffer.ptr(), i_dataBuffer.size());
	return i_dataBuffer.size();
}

size_t Packet::AddReliableMessage(const buf_t& i_dataBuffer)
{
	const size_t packedMessageSize = AddToOutgoingBuffer(i_dataBuffer);

	if (packedMessageSize)
	{
		m_header.reliableSize += static_cast<uint16_t>(i_dataBuffer.size());
	}
	return packedMessageSize;
}

size_t Packet::AddUnreliableMessage(const buf_t& i_dataBuffer)
{
	return AddToOutgoingBuffer(i_dataBuffer);
}

void Packet::Compress()
{
	byte method = 0;
	if (m_compressor.Compress(m_outgoingPacketBuffer, PacketHeaderType::PACKET_HEADER_SIZE, 0))
	{
		// Successful compression, set the compression flag bit.
		method |= PacketHeaderType::FLAG_COMPRESSED;
	}

	m_outgoingPacketBuffer.ptr()[PacketHeaderType::PACKET_FLAG_INDEX] |= method;
}

size_t Packet::CompressAndSend(const netadr_t& i_dest)
{
	size_t bytesSent = 0;
	if (m_outgoingPacketBuffer.size() > PacketHeaderType::PACKET_HEADER_SIZE)
	{
		Compress();
		bytesSent = NET_SendPacket(m_outgoingPacketBuffer, i_dest);
	}

	Reset();

	return bytesSent;
}

size_t Packet::ReSend(int i_historicalLocalTic, int i_destinationTic, int sequence, const buf_t& i_dataBuffer, const netadr_t& i_dest)
{
	m_outgoingPacketBuffer.clear();

	m_header.originatorTic  = i_historicalLocalTic;
	m_header.destinationTic = i_destinationTic;
	m_header.sequence       = sequence;
	m_header.reliableSize   = static_cast<uint16_t>(i_dataBuffer.size());
	m_header.flags          = 0;

	m_header.Pack(m_outgoingPacketBuffer);
	AddToOutgoingBuffer(i_dataBuffer);

	return CompressAndSend(i_dest);
}

size_t Packet::Send(int i_currentTic, int i_destinationTic, SequenceSender& i_sender, const netadr_t& i_dest)
{
	m_header.originatorTic  = i_currentTic;
	m_header.destinationTic = i_destinationTic;

	if (m_header.reliableSize)
	{
		// Save off the data for incoming ack checking and retransmission.
		auto saveMessage        = i_sender.ObtainSendPacket();
		m_header.sequence       = saveMessage.headerRef.sequence;
		saveMessage.headerRef   = m_header;
		saveMessage.bufferRef.WriteChunk(m_outgoingPacketBuffer.ptr(),
		                                 m_header.reliableSize,
		                                 PacketHeaderType::PACKET_MESSAGE_INDEX);
	}
	else
	{
		// For consistency and metrics collection, packets that are purely non-reliable
		// use the sequence number of the most-recently-produced reliable packet.  Please
		// note that it very intentionally does NOT affect any need to immediately process
		// the non-reliable data.

		m_header.sequence = i_sender.MostRecentAcquiredSequence();
	}

	m_outgoingPacketBuffer.SeekWrite(0, buf_t::BT_START);
	m_header.Pack(m_outgoingPacketBuffer);

	return CompressAndSend(i_dest);
}

size_t Packet::SendHighPriority(int i_currentTic, int i_destinationTic, SequenceSender& i_sender, const netadr_t& i_dest)
{
	if (m_header.reliableSize)
	{
		PrintFmt(PRINT_WARNING, "High-priority packets cannot include reliable messages, but {} reliable bytes are packed!", m_header.reliableSize);
	}

	m_header.originatorTic  = i_currentTic;
	m_header.destinationTic = i_destinationTic;
	m_header.sequence       = i_sender.MostRecentAcquiredSequence();
	m_header.flags          |= PacketHeaderType::FLAG_HIGH_PRIORITY;

	m_outgoingPacketBuffer.SeekWrite(0, buf_t::BT_START);
	m_header.Pack(m_outgoingPacketBuffer);

	return CompressAndSend(i_dest);
}

