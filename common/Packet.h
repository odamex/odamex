#pragma once

#include "i_net.h"

#include "PacketHeaderType.h"

class SequenceSender;

class Packet
{
	public:
		Packet();

		// This class assumes that Reliable messages will always be packed first.
		// If you have Reliable messages, they MUST go before Unreliable messages.
		size_t AddReliableMessage(const buf_t& i_dataBuffer);
		size_t AddAckMessage(const buf_t& i_dataBuffer);
		size_t AddUnreliableMessage(const buf_t& i_dataBuffer);

		void Compress();

		// Sender functions.
		//
		// These actually ultimately put the packet onto the wire.  After a call to
		// these functions, the in-memory packet data buffer is cleared and the instance
		// of this class may be reused to send any number of additional packets.
		//
		// Send() is intended for the transmission of a freshly-assembled packet.
		// If the packet contains a reliable section, then that section is saved off
		// into the given i_sender for ack and retransmit handling.
		//
		// ReSend() is intended for sending retransmission buffers.
		size_t Send(int i_currentTic, SequenceSender& i_sender, const netadr_t& i_dest);
		size_t ReSend(int sequence, const buf_t& i_dataBuffer, const netadr_t& i_dest);

		size_t Size() const { return m_outgoingPacketBuffer.size(); }
		int    RemainingBytes() const { return MAX_UDP_SIZE - static_cast<int>(m_outgoingPacketBuffer.size()); }

		size_t SizeOfReliablePortion() const { return m_header.reliableSize; }

		MiniLzo& GetCompressorRef() { return m_compressor; }

	protected:

		size_t CompressAndSend(const netadr_t& i_dest);
		size_t AddToOutgoingBuffer(const buf_t& i_dataBuffer);

		void Reset();

		buf_t            m_outgoingPacketBuffer { MAX_UDP_PACKET };
		MiniLzo          m_compressor;
		PacketHeaderType m_header;
};
