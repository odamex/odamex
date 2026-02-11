#include "SequencedMessenger.h"

#include "i_net.h"

dtime_t I_MSTime (void);
EXTERN_CVAR (log_packetdebug)

//  -------------- Receiving functions --------------

MessageResultEnum SequencedMessenger::Receive(buf_t& io_rawBuf)
{
    const PacketHeaderType header {io_rawBuf};

	if (header.flags & SVF_UNUSED_MASK)
	{
		PrintFmt(PRINT_WARNING, "Protocol flag bits ({}) were not understood", header.flags);
		return MessageResultEnum::ABORT;
	}
	else if (header.flags & SVF_COMPRESSED)
	{
		MSG_DecompressMinilzo(io_rawBuf);
	}


    const size_t fullSize               = io_rawBuf.size();
	const size_t startOfReliableData    = io_rawBuf.TellRead();
	const size_t startOfAcks            = startOfReliableData + header.reliableSize;
    const size_t startOfNonReliableData = startOfReliableData + header.reliableSize + header.ackSize;
	const size_t sizeOfNonReliableData  = fullSize - startOfNonReliableData;

	if (header.reliableSize)
	{
		if (not m_receiver.RegisterReliablePacket(header.sequence, header.reliableSize, io_rawBuf))
        {
            // Was it a worthless / duplicate retransmit?  Skip over the content.
            io_rawBuf.SeekRead(header.reliableSize, buf_t::BT_CURRENT);
        }

		// Send an ACK to the server only if it contained reliable data.
		if (not simulated_connection)
		{
			buf_t& ack = m_outgoingAckQueue.Obtain();
			ack.WriteByte(clc_ack);
			ack.WriteLong(header.sequence);
		}
	}
    if (io_rawBuf.BytesLeftToRead() > 0)
    {
        m_quickTurnaroundReceiveBuffer = &io_rawBuf;
        return MessageResultEnum::ACCEPT;
    }

	return MessageResultEnum::DEFER;
}

bool SequencedMessenger::NextReceivedPacket(buf_t& io_rawBuf)
{
	if (m_quickTurnaroundReceiveBuffer)
	{
        io_rawBuf.swap(*m_quickTurnaroundReceiveBuffer);
        m_quickTurnaroundReceiveBuffer = nullptr;
		return true;
	}
	return m_receiver.NextPacket(io_rawBuf) >= 0;
}

//  -------------- Sending functions --------------

void SequencedMessenger::ManageBudget(int i_currentTic)
{
    if (i_currentTic != m_latchedTic)
    {
        m_latchedTic = i_currentTic;
        m_byteBudget = std::min(m_perTicBudget, m_byteBudget + m_perTicBudget);
    }
}

MessageResultEnum SequencedMessenger::Send(int i_currentTic, const netadr_t& i_dest)
{
	const int ticPhase = i_currentTic % TICRATE;
	if (ticPhase == 0)
	{
        const int maxRateInBytes = m_maxRate * 1000;
        m_perTicBudget           = maxRateInBytes / TICRATE;
	}

    ManageBudget(i_currentTic);

    const int startBudget = m_byteBudget;

	m_lastSendSize = 0;

    auto addUnreliableFunctor = [this](const buf_t& messageBuf)
    {
        return m_packet.AddUnreliableMessage(messageBuf);
    };

    auto addAckFunctor = [this](const buf_t& messageBuf)
    {
        return m_packet.AddAckMessage(messageBuf);
    };

    if (simulated_connection)
    {
        Clear();
    }

    // First phase - send reliables, padded out to MAX_UDP_SIZE-ish with Acks.
    size_t bytesSentWithReliability = 0;
    while (m_outgoingReliableQueue.SizeInMessages() > 0 and m_byteBudget > 0)
    {
        m_outgoingReliableQueue.Pack([this](const buf_t& messageBuf) { return m_packet.AddReliableMessage(messageBuf); });

        m_outgoingAckQueue.Pack(addAckFunctor);

        // Now cover the case where we have all our acks out and there's still leftover space enough for an unreliable portion.
        m_outgoingNonReliableQueue.Pack(addUnreliableFunctor);

        const size_t sendSize = m_packet.Send(i_currentTic, m_sender, i_dest);
        bytesSentWithReliability += sendSize;
        m_byteBudget             -= static_cast<int>(sendSize);
    }

    // Now get any remaining Acks out.  This also covers the case where we just didn't have any reliable packets to send.
    // For accounting purposes against target rate, we consider Acks to have the same priority as reliable traffic because
    // it fundamentally is!
    while(m_outgoingAckQueue.SizeInMessages() > 0 and m_byteBudget > 0)
    {
        m_outgoingAckQueue.Pack(addAckFunctor);

        // Welp, we filled up the packet with all-acks?  Send it.
        if (m_outgoingAckQueue.SizeInMessages() > 0)
        {
            const size_t sendSize = m_packet.Send(i_currentTic, m_sender, i_dest);
            bytesSentWithReliability += sendSize;
            m_byteBudget             -= static_cast<int>(sendSize);
        }
    }

    size_t bytesSentBestEffort = 0;
    // Okay, done with the "really important" stuff.  Now onto best-effort unreliable stuff.
    while (m_outgoingNonReliableQueue.SizeInMessages() > 0 and m_byteBudget > 0)
    {
        if (static_cast<int>(m_packet.Size() + m_outgoingNonReliableQueue.Front().size()) > m_byteBudget)
        {
            break;
        }

        if (m_packet.AddUnreliableMessage(m_outgoingNonReliableQueue.Front()))
        {
            m_outgoingNonReliableQueue.Pop();
        }
        else
        {
            if (m_packet.SizeOfReliablePortion() == 0)
            {
                const size_t bestEffortBytes = m_packet.Send(i_currentTic, m_sender, i_dest);
                bytesSentBestEffort         += bestEffortBytes;
                m_byteBudget                -= static_cast<int>(bestEffortBytes);
            }
            else
            {
                const size_t reliableBytes = m_packet.Send(i_currentTic, m_sender, i_dest);
                bytesSentWithReliability  += reliableBytes;
                m_byteBudget              -= static_cast<int>(reliableBytes);
            }
        }
    }

    m_outgoingNonReliableQueue.Clear();

    // Last packet.  Send it, even if overbudget...  We'll borrow against the future.
    // If it doesn't have anything, nothing happens, we're good.
    const size_t lastReliableBytesSent = m_packet.SizeOfReliablePortion();
    const size_t lastTotalSent         = m_packet.Send(i_currentTic, m_sender, i_dest);
    m_byteBudget                      -= static_cast<int>(lastTotalSent);

    if (lastReliableBytesSent)
    {
        bytesSentWithReliability += lastTotalSent;
    }
    else
    {
        bytesSentBestEffort += lastTotalSent;
    }

    m_lastSendSize = std::max(0, startBudget - m_byteBudget);

    return MessageResultEnum::ACCEPT;
}

int SequencedMessenger::HandleRetransmissions(int i_currentTic, const netadr_t& i_dest)
{
	int retransmissionsSent = 0;
	int bytesSent = 0;

    ManageBudget(i_currentTic);

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
            // TODO: Working Throttle!
            //       With 800 KB rate at the nuts.wad wakeup with +50 msec lag (on incoming and outgoing), 10% packet loss
            //       causes a 900KB - 1000KB spike that causes retransmissions to fail.  For now we just live with that.
			if (++retransmissionsSent > m_maxPacketsPerRetransmission or m_byteBudget <= 0)
			{
				break;
			}
			m_noncontiguousRetransmitCount += previousPacketSeq != sendQueueEntry->sequence - 1 ? 1 : 0;
			previousPacketSeq = sendQueueEntry->sequence;

			sendQueueEntry->lastRetransmitTic = i_currentTic;
            const int resendSize = static_cast<int>(m_packet.ReSend(sendQueueEntry->sequence, sendQueueEntry->buf, i_dest));
			bytesSent    += resendSize;
            m_byteBudget -= resendSize;
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
