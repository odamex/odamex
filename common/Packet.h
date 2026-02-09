#pragma once

#include "i_net.h"

#include "PacketHeaderType.h"
#include "SequenceSender.h"

class Packet
{
    public:
        Packet();

        // This class assumes that Reliable messages will always be packed first.
        // If you have Reliable messages, they MUST go before Unreliable messages.
        size_t AddReliableMessage(const buf_t& i_dataBuffer);
        size_t AddUnreliableMessage(const buf_t& i_dataBuffer);

        void Compress();

        size_t Send(int i_currentTic, SequenceSender& i_sender, const netadr_t& i_dest);
        size_t ReSend(int sequence, const buf_t& i_dataBuffer, const netadr_t& i_dest);

        size_t Size() const { return m_outgoingPacketBuffer.size(); }
        int    RemainingBytes() const { return MAX_UDP_SIZE - static_cast<int>(m_outgoingPacketBuffer.size()); }

        size_t SizeOfReliablePortion() const { return m_header.reliableSize; }

    protected:

        size_t CompressAndSend(const netadr_t& i_dest);
        size_t AddToOutgoingBuffer(const buf_t& i_dataBuffer);

        void Reset();

		buf_t                m_outgoingPacketBuffer { MAX_UDP_PACKET };
        PacketHeaderType     m_header;
};
