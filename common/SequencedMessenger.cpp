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
			const size_t startOfReliableData    = io_rawBuf.Tell();
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
			m_ackBuffer.WriteByte(clc_ack);
			m_ackBuffer.WriteLong(sequence);
			if (m_reliableBuffer.size() + m_ackBuffer.size() >= 1024)
			{
				Send(i_currentTic, i_dest);
			}
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
	if (SequenceQueueEntryType* queueEntryPtr = m_receiver.NextPacket())
	{
		io_rawBuf.swap(queueEntryPtr->buf);
		return true;
	}
	return false;
}

//  -------------- Sending functions --------------

MessageResultEnum SequencedMessenger::Send(int i_currentTic, const netadr_t& i_dest)
{
	int bps = 0; // bytes per second, not bits per second

	m_lastSendSize = 0;

	if (m_reliableBuffer.overflowed or m_ackBuffer.overflowed)
	{
		SZ_Clear(&m_nonreliableBuffer);
		SZ_Clear(&m_reliableBuffer);
        SZ_Clear(&m_ackBuffer);
		//SV_DropClient(pl);
		return MessageResultEnum::ABORT;
	}
	else
		if (m_nonreliableBuffer.overflowed)
			SZ_Clear(&m_nonreliableBuffer);

	// [SL] 2012-05-04 - Don't send empty packets - they still have overhead
	if (m_reliableBuffer.cursize +
        m_nonreliableBuffer.cursize +
        m_ackBuffer.cursize == 0)
	{
		return MessageResultEnum::DEFER;
	}

	m_outgoingPacketBuffer.clear();

	// Messages without reliability are non-sequenced.
	int sequence = -1;

	if (m_reliableBuffer.cursize)
	{
		// Save the reliable portion of the message for ack checking and retransmission if necessary.
		auto saveMessage = m_sender.ObtainSendPacket(i_currentTic);

		if (saveMessage.buffer)
		{
			// copy the reliable portion into the buffer.
			SZ_Write(saveMessage.buffer, m_reliableBuffer.data.get(), m_reliableBuffer.cursize);

			// Insert Reliable sequence number first thing.
            sequence = saveMessage.sequence;
		}
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
		m_reliableBps += m_reliableBuffer.cursize;
	}

    // Then acks.  They count as part of Reliable traffic for throughput calculation purposes.
    if (m_ackBuffer.cursize)
    {
        SZ_Write(&m_outgoingPacketBuffer, m_ackBuffer.data.get(), m_ackBuffer.cursize);
        m_reliableBps += m_ackBuffer.cursize;
    }
	// add the unreliable part if space is available and rate value
	// allows it
	const int ticPhase = i_currentTic % TICRATE;
	if (ticPhase != 0)
	{
		bps = (int)((double)( (m_unreliableBps + m_reliableBps) * TICRATE)/(double)(i_currentTic%TICRATE));
	}

	if (bps < m_maxRate * 1000)
	{
		if (m_nonreliableBuffer.cursize && (m_outgoingPacketBuffer.maxsize() - m_outgoingPacketBuffer.cursize > m_nonreliableBuffer.cursize) )
		{
			SZ_Write (&m_outgoingPacketBuffer, m_nonreliableBuffer.data.get(), m_nonreliableBuffer.cursize);
			m_unreliableBps += m_nonreliableBuffer.cursize;
		}
	}

    const bool isThrottled = (m_sender.GetMode() == SequenceSender::RECOVERY and m_reliableBuffer.cursize == 0 and m_ackBuffer.cursize == 0);

    SZ_Clear(&m_ackBuffer);
	SZ_Clear(&m_nonreliableBuffer);
	SZ_Clear(&m_reliableBuffer);

	if (m_outgoingPacketBuffer.size() > PACKET_HEADER_SIZE and not isThrottled)
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

	if (ticPhase == 0)
	{
		m_unreliableBps = 0;
		m_reliableBps   = 0;
	}

	return MessageResultEnum::ACCEPT;
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
			bytesSent += SendOldPacket(*sendQueueEntry, i_dest);

		}
	}
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


void SequencedMessenger::CompressPacket(buf_t& send, const size_t reserved)
{
	byte method = 0;
	if (MSG_CompressMinilzo(send, reserved, 0))
	{
		// Successful compression, set the compression flag bit.
		method |= SVF_COMPRESSED;
	}

	send.ptr()[PACKET_FLAG_INDEX] |= method;
}

int SequencedMessenger::SendOldPacket(const SequenceQueueEntryType& queueEntry, const netadr_t& i_dest)
{
	m_outgoingPacketBuffer.clear();

	// This is a lot simpler than a fresh send.  Just send the data we have
	// have saved out.

	MSG_WriteLong (&m_outgoingPacketBuffer, queueEntry.sequence);
	MSG_WriteShort(&m_outgoingPacketBuffer, static_cast<short>(queueEntry.buf.cursize));
	MSG_WriteByte (&m_outgoingPacketBuffer, 0); // Flags, filled out later.

	// copy the reliable message to the packet
	if (queueEntry.buf.cursize)
	{
		SZ_Write(&m_outgoingPacketBuffer, queueEntry.buf.data.get(), queueEntry.buf.cursize);
		m_reliableBps += queueEntry.buf.cursize;
	}

	// compress the data portion of the packet
	if (m_outgoingPacketBuffer.size() > PACKET_HEADER_SIZE)
	{
		CompressPacket(m_outgoingPacketBuffer, PACKET_HEADER_SIZE);
	}

	return NET_SendPacket(m_outgoingPacketBuffer, i_dest);
}
