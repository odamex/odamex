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
//  The Odamex messenger; packet assembly, reliability protocol, queueing
//
//-----------------------------------------------------------------------------sx
#include "OdaMessenger.h"

#include "i_net.h"

EXTERN_CVAR (log_packetdebug)

//  -------------- Receiving functions --------------

MessageResultEnum OdaMessenger::Receive(buf_t& io_rawBuf)
{
	const PacketHeaderType header {io_rawBuf};

	if (m_sender.GetMode() == SequenceSender::CRITICAL_FAILURE)
	{
		return MessageResultEnum::ABORT;
	}

	if (header.flags & SVF_UNUSED_MASK)
	{
		PrintFmt(PRINT_WARNING, "Protocol flag bits ({}) were not understood", header.flags);
		return MessageResultEnum::ABORT;
	}
	else if (header.flags & SVF_COMPRESSED)
	{
		m_packet.GetCompressorRef().Decompress(io_rawBuf);
	}

	const size_t fullSize               = io_rawBuf.size();
	const size_t startOfReliableData    = io_rawBuf.TellRead();
	const size_t startOfNonReliableData = startOfReliableData + header.reliableSize;
	const size_t sizeOfNonReliableData  = fullSize - startOfNonReliableData;

	if (header.reliableSize)
	{
		if (not m_receiver.RegisterReliablePacket(header.sequence, header.reliableSize, io_rawBuf))
		{
			// Was it a worthless / duplicate retransmit?  Skip over the content.
			io_rawBuf.SeekRead(header.reliableSize, buf_t::BT_CURRENT);
		}

		// Send an ACK to the originating messenger.
		if (not simulated_connection)
		{
			buf_t& ack = m_outgoingHighNonReliableQueue.Obtain();
			ack.WriteUnVarint(msg_ack);
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

bool OdaMessenger::NextReceivedPacket(buf_t& io_rawBuf)
{
	if (m_sender.GetMode() == SequenceSender::CRITICAL_FAILURE)
	{
		return false;
	}

	if (m_quickTurnaroundReceiveBuffer)
	{
		io_rawBuf.swap(*m_quickTurnaroundReceiveBuffer);
		m_quickTurnaroundReceiveBuffer = nullptr;
		return true;
	}
	return m_receiver.NextPacket(io_rawBuf) >= 0;
}

void OdaMessenger::HandleAcks(buf_t& io_rawBuf)
{
	while (io_rawBuf.BytesLeftToRead() > 0)
	{
		const size_t startPosition = io_rawBuf.TellRead();
		const msg_t  messageId     = static_cast<msg_t>(io_rawBuf.ReadUnVarint());
		if (messageId != msg_ack)
		{
			io_rawBuf.SeekRead(startPosition, buf_t::BT_START);
			break;
		}
		const int sequence = io_rawBuf.ReadLong();
		Acknowledge(sequence);
	}
}


//  -------------- Sending functions --------------

// TODO:  Re-implement SIMULATE_LATENCY through outgoing queue management.

void OdaMessenger::ManageBudget(int i_currentTic)
{
	if (i_currentTic != m_latchedTic)
	{
		m_latchedTic = i_currentTic;
		m_byteBudget = std::min(m_perTicBudget, m_byteBudget + m_perTicBudget);
	}
}

void OdaMessenger::Record(const buf_t& messageBuf)
{
	m_recordingBuffer += std::basic_string_view<byte>(messageBuf.ptr(), messageBuf.size());
}

size_t OdaMessenger::PackAsReliable(Packet& io_packet, const buf_t& messageBuf)
{
	if (m_recordingIsEnabled)
	{
		Record(messageBuf);
	}
	return io_packet.AddReliableMessage(messageBuf);
}

size_t OdaMessenger::PackAsUnreliable(Packet& io_packet, const buf_t& messageBuf)
{
	if (m_recordingIsEnabled)
	{
		Record(messageBuf);
	}
	return io_packet.AddUnreliableMessage(messageBuf);
}

MessageResultEnum OdaMessenger::SendAll(int i_currentTic, const netadr_t& i_dest)
{
	// Once a second, recalculate the budget to incorporate any changes made to maxRate.
	const int ticPhase = i_currentTic % TICRATE;
	if (ticPhase == 0)
	{
		const int maxRateInBytes = m_maxRate * 1000;
		m_perTicBudget           = maxRateInBytes / TICRATE;
	}

	ManageBudget(i_currentTic);

	const int startBudget = m_byteBudget;

	m_lastSendSize = 0;

	if (m_recordingIsEnabled)
	{
		m_recordingBuffer.clear();
	}

	if (simulated_connection)
	{
		Clear();
	}

	// Phase zero:  Detect if the client has become dangerously non-responsive,
	//              and abort if so.
	if (SequenceQueueEntryType* oldestOutgoingUnackedEntry = m_sender.IterateUnackedPackets().Next())
	{
		if (i_currentTic > oldestOutgoingUnackedEntry->originatingTic + m_criticalSequenceTimeoutInTics)
		{
			m_sender.SetMode(SequenceSender::CRITICAL_FAILURE);
			return MessageResultEnum::ABORT;
		}
	}

	// First phase - send high-priority non-reliables (acks, servertic, player updates)
	size_t bytesSentBestEffort = 0;
	while (m_outgoingHighNonReliableQueue.SizeInMessages() > 0 and m_byteBudget > 0)
	{
		m_outgoingHighNonReliableQueue.Pack([this](const buf_t& buf) { return PackAsUnreliable(m_highPacket, buf); });

		const size_t sendSize = m_highPacket.Send(i_currentTic, m_sender, i_dest);
		bytesSentBestEffort += sendSize;
		m_byteBudget        -= static_cast<int>(sendSize);
	}

	// Second phase - send reliables, padded out to MAX_UDP_SIZE-ish with non-reliable best-effort messages.
	m_bytesSentWithReliability = 0;
	while (m_outgoingReliableQueue.SizeInMessages() > 0 and m_byteBudget > 0)
	{
		m_outgoingReliableQueue.Pack([this](const buf_t& messageBuf) { return PackAsReliable(m_packet, messageBuf); });

		// Now cover the case where we have leftover space enough for an unreliable portion.
		m_outgoingNonReliableQueue.Pack([this](const buf_t& messageBuf) { return PackAsUnreliable(m_packet, messageBuf); });

		const size_t sendSize = m_packet.Send(i_currentTic, m_sender, i_dest);
		m_bytesSentWithReliability += sendSize;
		m_byteBudget               -= static_cast<int>(sendSize);
	}

	// Okay, done with the "really important" stuff.  Now onto purely best-effort unreliable packets.
	while (m_outgoingNonReliableQueue.SizeInMessages() > 0 and m_byteBudget > 0)
	{
		if (static_cast<int>(m_packet.Size() + m_outgoingNonReliableQueue.Front().size()) > m_byteBudget)
		{
			break;
		}

		if (m_recordingIsEnabled)
		{
			Record(m_outgoingNonReliableQueue.Front());
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
				m_bytesSentWithReliability  += reliableBytes;
				m_byteBudget                -= static_cast<int>(reliableBytes);
			}
		}
	}

	m_outgoingNonReliableQueue.Clear();

	// Last packet.  Send it, even if overbudget...  We'll borrow against the future.
	// If it doesn't have anything, nothing happens, we're good.
	//
	// Please note that m_packet is smart enough to not actually send the packet if
	// it contains only a header.

	const size_t lastReliableBytesSent = m_packet.SizeOfReliablePortion();
	const size_t lastTotalSent         = m_packet.Send(i_currentTic, m_sender, i_dest);
	m_byteBudget                      -= static_cast<int>(lastTotalSent);

	if (lastReliableBytesSent)
	{
		m_bytesSentWithReliability += lastTotalSent;
	}
	else
	{
		bytesSentBestEffort += lastTotalSent;
	}

	m_lastSendSize = std::max(0, startBudget - m_byteBudget);

	const int reliableOverloadAdjustment = m_bytesSentWithReliability > m_reliableOverloadThreshold ? 1 : -1;

	m_reliableOverloadCount = std::max(0, std::min(m_reliableOverloadCount + reliableOverloadAdjustment, 10));

	return m_byteBudget > 0 ? MessageResultEnum::ACCEPT : MessageResultEnum::DEFER;
}

int OdaMessenger::HandleRetransmissions(int i_currentTic, const netadr_t& i_dest)
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
	else if (m_unackedGrowth == 0 and m_sender.GetMode() == SequenceSender::RECOVERY)
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
		// m_retransmitDelayInTics used to be constrained here via std::min(m_retransmitDelayInTics, 5).
		// I wish I had documented exactly why it was not allowed to be more than 5 for quite a while
		// during development.
		//
		// In any case, natural scaling works well now, probably because we have a working throttle.
		if (i_currentTic >= (m_retransmitDelayInTics + sendQueueEntry->originatingTic) or sendQueueEntry->lastRetransmitTic != -1)
		{
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

bool OdaMessenger::Acknowledge(int sequence)
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
