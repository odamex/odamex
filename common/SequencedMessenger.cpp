#include "SequencedMessenger.h"

#include "i_net.h"

dtime_t I_MSTime (void);
EXTERN_CVAR (log_packetdebug)

//  -------------- Receiving functions --------------

MessageResultEnum SequencedMessenger::Receive(buf_t& io_rawBuf, int i_currentTic, const netadr_t& i_dest)
{
	const int  sequence     = io_rawBuf.ReadLong();    // Packet sequence number.
	const int  reliableSize = io_rawBuf.ReadShort();   // Reliable size / Start of Unreliable data
	const byte flags        = io_rawBuf.ReadByte();    // Flag bits.
	if (flags & SVF_UNUSED_MASK)
	{
		PrintFmt(PRINT_WARNING, "Protocol flag bits ({}) were not understood", flags);
		return MessageResultEnum::ABORT;
	}
	else if (flags & SVF_COMPRESSED)
	{
		MSG_DecompressMinilzo(io_rawBuf);
	}

	m_receiveBuffer.clear();
	if (sequence >= 0)
	{
		// If this packet has both reliable and unreliable data, receive the unreliable
		// portion immediately, then truncate the packet and defer the rest for ordered
		// processing.
		const bool alsoHasNonReliableData = reliableSize < io_rawBuf.BytesLeftToRead();
		if (alsoHasNonReliableData)
		{
			const size_t startOfReliableData    = io_rawBuf.TellRead();
			const size_t startOfNonReliableData = startOfReliableData + reliableSize;
			const size_t sizeOfNonReliableData  = io_rawBuf.size() - startOfNonReliableData;

			m_receiveBuffer.WriteChunk(reinterpret_cast<char*>(io_rawBuf.ptr()),
			                           sizeOfNonReliableData,
			                           startOfNonReliableData);

			const size_t sizeOfMessageWithNonReliableTruncated = startOfNonReliableData;

			io_rawBuf.setcursize(sizeOfMessageWithNonReliableTruncated);
		}

		m_receiver.RegisterReceivedPacket(sequence, io_rawBuf);

		// Send an ACK to the server only if it contained reliable data.
		if (not simulated_connection)
		{
			buf_t& ack = m_ackBuffer.Obtain();
			ack.WriteByte(clc_ack);
			ack.WriteLong(sequence);
		}
		return alsoHasNonReliableData ? MessageResultEnum::ACCEPT : MessageResultEnum::DEFER;
	}
	else
	{
		m_receiveBuffer.swap(io_rawBuf);
	}
	return MessageResultEnum::ACCEPT;
}

bool SequencedMessenger::NextReceivedPacket(buf_t& io_rawBuf)
{
	if (m_receiveBuffer.size() > 0)
	{
		io_rawBuf.swap(m_receiveBuffer);
		m_receiveBuffer.clear();
		return true;
	}
	return m_receiver.NextPacket(io_rawBuf) >= 0;
}

//  -------------- Sending functions --------------

MessageResultEnum SequencedMessenger::Send(int i_currentTic, const netadr_t& i_dest)
{
	const int ticPhase = i_currentTic % TICRATE;
	if (ticPhase == 0)
	{
		m_unreliableBps = 0;
		m_reliableBps   = 0;

        const int maxRateInBytes = m_maxRate * 1000;
        m_bpsBudget              = std::min(maxRateInBytes, m_bpsBudget + maxRateInBytes);
	}

    const int startBudget = m_bpsBudget;
	int bps = 0; // bytes per second, not bits per second

	m_lastSendSize = 0;

    auto addUnreliableFunctor = [this](const buf_t& messageBuf)
    {
        return m_packet.AddUnreliableMessage(messageBuf);
    };

    // First phase - send reliables, padded out to MAX_UDP_SIZE-ish with Acks.
    size_t bytesSentWithReliability = 0;
    while (m_reliableBuffer.SizeInMessages() > 0)
    {
        m_reliableBuffer.Pack([this](const buf_t& messageBuf) { return m_packet.AddReliableMessage(messageBuf); });

        m_ackBuffer.Pack(addUnreliableFunctor);

        // Now cover the case where we have all our acks out and there's still leftover space enough for an unreliable portion.
        m_nonreliableBuffer.Pack(addUnreliableFunctor);

        bytesSentWithReliability += m_packet.Send(i_currentTic, m_sender, i_dest);
    }

    // Now get any remaining Acks out.  This also covers the case where we just didn't have any reliable packets to send.
    // For accounting purposes against target rate, we consider Acks to have the same priority as reliable traffic because
    // it fundamentally is!
    while(m_ackBuffer.SizeInMessages() > 0)
    {
        const size_t preAckSize = m_packet.Size();
        m_ackBuffer.Pack(addUnreliableFunctor);
        const size_t packedAckSize = m_packet.Size() - preAckSize;

        // Welp, we filled up the packet with all-acks?  Send it.
        if (m_ackBuffer.SizeInMessages() > 0)
        {
            bytesSentWithReliability += m_packet.Send(i_currentTic, m_sender, i_dest);
        }
    }

    m_reliableBps += bytesSentWithReliability;
    m_bpsBudget   -= static_cast<int>(bytesSentWithReliability);

    size_t bytesSentBestEffort = 0;
    // Okay, done with the "really important" stuff.  Now onto best-effort unreliable stuff.
    while (m_nonreliableBuffer.SizeInMessages() > 0)
    {
        if (static_cast<int>(m_nonreliableBuffer.Front().size() + m_packet.Size()) < m_bpsBudget)
        {
            if (m_packet.AddUnreliableMessage(m_nonreliableBuffer.Front()) == 0)
            {
                if (m_packet.SizeOfReliablePortion() == 0)
                {
                    const size_t bestEffortBytes = m_packet.Send(i_currentTic, m_sender, i_dest);
                    bytesSentBestEffort += bestEffortBytes;
                    m_bpsBudget         -= static_cast<int>(bestEffortBytes);
                }
                else
                {
                    const size_t reliableBytes = m_packet.Send(i_currentTic, m_sender, i_dest);
                    bytesSentWithReliability += reliableBytes;
                    m_bpsBudget              -= static_cast<int>(reliableBytes);
                }
            }
        }
        m_nonreliableBuffer.Pop();
    }

    // Last packet.  Send it, even if overbudget...  We'll borrow against the future.
    // If it doesn't have anything, nothing happens, we're good.
    const size_t lastReliableBytesSent = m_packet.SizeOfReliablePortion();
    const size_t lastTotalSent         = m_packet.Send(i_currentTic, m_sender, i_dest);
    m_bpsBudget              -= static_cast<int>(lastTotalSent);
    bytesSentWithReliability += lastReliableBytesSent;

    m_lastSendSize = std::max(0, startBudget - m_bpsBudget);

    return MessageResultEnum::ACCEPT;




#if 0
    // Acks are really important to get out ASAP.  Start there.

    size_t totalAckBytes = m_ackBuffer;
    while (m_ackBuffer.SizeInMessages() > 0)
    {
        const size_t acksInBytes = m_ackBuffer.Pack(m_outgoingPacketBuffer, MAX_UDP_SIZE);

        // Okay we must send an ack-ful packet.
        if (m_ackBuffer.SizeInMessages() > 0)
        {
        }

    const size_t ackBytes    = m_ackBuffer.SizeInBytes();
    const int    nonAckBytes = static_cast<int>(MAX_UDP_SIZE) - static_cast<int>(ackBytes);

    while (m_ackBuffer.SizeInMessages() > 0)
    {
	int sequence = -1;
	// Save the reliable portion of the message for ack checking and retransmission if necessary.
	auto saveMessage = m_sender.ObtainSendPacket(i_currentTic);

	if (saveMessage.buffer)
	{
		// copy the reliable portion into the buffer.
		SZ_Write(saveMessage.buffer, m_reliableBuffer.data.get(), m_reliableBuffer.cursize);

		// Insert Reliable sequence number first thing.
        sequence = saveMessage.sequence;
	}

	m_outgoingPacketBuffer.clear();
	MSG_WriteLong (&m_outgoingPacketBuffer, sequence);
	MSG_WriteShort(&m_outgoingPacketBuffer, static_cast<short>(m_reliableBuffer.cursize));  // Reliable size
	MSG_WriteByte (&m_outgoingPacketBuffer, 0);                                             // Flags, filled out later.
            // Send the packet.
        }
    }

    SizeInMessages

	// Messages without reliability are non-sequenced.
	int sequence = -1;

	if (m_reliableBuffer.cursize)
	{
        // If we fall into an 'else' case here, it's because we just cannot get a reliable packet
        // in the sequence.  Something has gone really wrong.  Our best shot is to send the data
        // as unreliable... :(
	}

	MSG_WriteLong (&m_outgoingPacketBuffer, sequence);
	MSG_WriteShort(&m_outgoingPacketBuffer, static_cast<short>(m_reliableBuffer.cursize));  // Reliable size
	MSG_WriteByte (&m_outgoingPacketBuffer, 0);                                             // Flags, filled out later.

    // NOTE: The receiver COULD look at whether there's a non-zero Reliable Size alongside a
    //       negative sequence and conclude that the sender is totally exhausted.

	// copy the reliable message to the packet first
	if (m_reliableBuffer.cursize)
	{
		SZ_Write (&m_outgoingPacketBuffer, m_reliableBuffer.data.get(), m_reliableBuffer.cursize);
	}

    // Then acks.  They count as part of Reliable traffic for throughput calculation purposes.
    if (m_ackBuffer.cursize)
    {
        SZ_Write(&m_outgoingPacketBuffer, m_ackBuffer.data.get(), m_ackBuffer.cursize);
        m_reliableBps += m_ackBuffer.cursize;
    }
	// add the unreliable part if space is available and rate value
	// allows it
	if (ticPhase != 0)
	{
		bps = (int)((double)( (m_unreliableBps + m_reliableBps) * TICRATE)/(double)(i_currentTic%TICRATE));
	}

	if (bps < m_maxRate * 1000 and m_sender.GetMode() != SequenceSender::RECOVERY)
	{
		if (m_nonreliableBuffer.cursize && (m_outgoingPacketBuffer.maxsize() - m_outgoingPacketBuffer.cursize > m_nonreliableBuffer.cursize) )
		{
			SZ_Write (&m_outgoingPacketBuffer, m_nonreliableBuffer.data.get(), m_nonreliableBuffer.cursize);
			m_unreliableBps += m_nonreliableBuffer.cursize;
		}
	}

    //const bool isThrottled = (m_sender.GetMode() == SequenceSender::RECOVERY and m_reliableBuffer.cursize == 0 and m_ackBuffer.cursize == 0);

    SZ_Clear(&m_ackBuffer);
	SZ_Clear(&m_nonreliableBuffer);
	SZ_Clear(&m_reliableBuffer);

	if (m_outgoingPacketBuffer.size() > PACKET_HEADER_SIZE)
	{
		// compress the packet, but not the sequence id
		CompressPacket(m_outgoingPacketBuffer, PACKET_HEADER_SIZE);

		if (log_packetdebug)
		{
			// FIXME: Have the player ID handy for debug messages.
			//PrintFmt(PRINT_HIGH, "ply {:03}, size {:04}, tic {:07}, time {:011}\n",
			//        pl.id, m_outgoingPacketBuffer.cursize, i_currentTic, I_MSTime());
		}

#ifdef SIMULATE_LATENCY
		SV_SendPacketDelayed(m_outgoingPacketBuffer, pl);
#else
		m_lastSendSize = NET_SendPacket(m_outgoingPacketBuffer, i_dest);
#endif
	}

	return MessageResultEnum::ACCEPT;
#endif

}

int SequencedMessenger::HandleRetransmissions(int i_currentTic, const netadr_t& i_dest)
{
	int retransmissionsSent = 0;
	int bytesSent = 0;

    // Gather metrics
    m_unackedGrowth       += m_sender.GetPendingAckCount() > m_previousUnackedCount ? 1 : -1;
    m_unackedGrowth        = std::max(0, m_unackedGrowth);
    m_previousUnackedCount = m_sender.GetPendingAckCount();

    if (m_unackedGrowth > 10 && m_sender.GetMode() == SequenceSender::NORMAL)
    {
        m_sender.SetMode(SequenceSender::RECOVERY);
    }
    else if (m_unackedGrowth == 0)
    {
        m_sender.SetMode(SequenceSender::NORMAL);
    }

    if (m_sender.GetMode() == SequenceSender::RECOVERY)
    {
        m_retransmitDelayInTics = 0;
    }

	auto                    iter = m_sender.IterateUnackedPackets();
	SequenceQueueEntryType* sendQueueEntry = iter.Next();

	// If we have retransmissions, setup previousPacketSeq to appear that the first retransmitted
	// packet counts as the first of a contiguous run of packets.
	int previousPacketSeq          = sendQueueEntry ? sendQueueEntry->sequence - 1 : -1;
	m_noncontiguousRetransmitCount = 0;

	for (; sendQueueEntry != nullptr; sendQueueEntry = iter.Next())
	{
		if (i_currentTic >= (std::min(m_retransmitDelayInTics, 5) + sendQueueEntry->originatingTic) or sendQueueEntry->lastRetransmitTic != -1)
		{
			if (++retransmissionsSent > m_maxPacketsPerRetransmission)
			{
				break;
			}
			m_noncontiguousRetransmitCount += previousPacketSeq != sendQueueEntry->sequence - 1 ? 1 : 0;
			previousPacketSeq = sendQueueEntry->sequence;

			sendQueueEntry->lastRetransmitTic = i_currentTic;
			bytesSent += static_cast<int>(m_packet.ReSend(sendQueueEntry->sequence, sendQueueEntry->buf, i_dest));

		}
	}
	m_reliableBps += bytesSent;

    m_bpsBudget -= bytesSent;

	return bytesSent;
}

bool SequencedMessenger::Acknowledge(int sequence)
{
    const bool isFreshAck = m_sender.Acknowledge(sequence);
    if (isFreshAck)
    {
        if (m_sender.GetMode() == SequenceSender::RECOVERY and m_sender.GetPendingAckCount() < m_maxPacketsPerRetransmission)
        {
            m_sender.SetMode(SequenceSender::NORMAL);
        }
    }
	return isFreshAck;
}
