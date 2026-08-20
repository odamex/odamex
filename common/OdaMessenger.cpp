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

	m_immediateReceiveBuffer.clear();

	if (m_sender.GetMode() == SequenceSender::CRITICAL_FAILURE)
	{
		return MessageResultEnum::ABORT;
	}

	// If somehow we've overflowed on header decode, just shrug, discard, and move on.
	if (io_rawBuf.overflowed)
	{
		return MessageResultEnum::ABORT;
	}

	if (header.flags & PacketHeaderType::FLAG_UNUSED_MASK)
	{
		PrintFmt(PRINT_WARNING, "Protocol flag bits ({}) were not understood\n", header.flags);
		return MessageResultEnum::ABORT;
	}

	if (header.flags & PacketHeaderType::FLAG_HIGH_PRIORITY and header.reliableSize)
	{
		PrintFmt(PRINT_WARNING, "High priority packet {} had a reliable payload: {} bytes\n",
		         -header.sequence,
		         header.reliableSize);
		return MessageResultEnum::ABORT;
	}

	if (m_isBitBucket)
	{
		return MessageResultEnum::DEFER;
	}

	if (header.flags & PacketHeaderType::FLAG_COMPRESSED)
	{
		m_packet.GetCompressorRef().Decompress(io_rawBuf);
	}

	const size_t fullSize               = io_rawBuf.size();
	const size_t startOfReliableData    = io_rawBuf.TellRead();

	// Do some sanity checking.
	//
	// No need to check startOfReliableData because that and fullSize are direct results of packet reception
	// and header read.  Once we start looking at the values in the header, then we take a closer look.
	//
	// Can't look beyond the end of the message.  Discard and move on if it's that bad.
	if (fullSize < startOfReliableData + header.reliableSize)   // note: reliableSize is unsigned
	{
		return MessageResultEnum::ABORT;
	}

	if (header.reliableSize)
	{
		if (not m_receiver.RegisterReliablePacket(header, header.reliableSize, io_rawBuf))
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

	const size_t bestEffortSize = io_rawBuf.BytesLeftToRead();

	if (bestEffortSize > 0)
	{
		if (bestEffortSize > m_immediateReceiveBuffer.maxsize())
		{
			m_immediateReceiveBuffer.resize(bestEffortSize + 1);    // +1 because that's what buf_t needs...
		}

		// IMPORTANT NOTE:  The best-effort packet queuing and deferred reception is a part of a future
		//                  broader mobj rollback reconciliation feature.  We disable it for now because
		//                  if we don't have mobj rollback enabled, then it can cause updates that should
		//                  be handled in a timely manner to be deferred if network latency jitter or packet
		//                  loss is prevalent, causing a visible backwards stutter on missiles and other
		//                  things.
		//
		//                  Do not enable the following until holistic rollback is implemented.

#ifdef ODAMESSENGER_ENABLE_BE_PACKET_QUEUE

		// One subtlety: Best effort / normal-priority messages that are "too old" are still handled
		//               because they could still have data mobjs that's more current than the mobjs'
		//               last reliable update, which could be even older.
		//
		// Another subtlety: the header sequence will be -1 for any best-effort-only packets that predate
		//                   any reliable messages.  Therefore we can't consider the sequence to be "old"
		//                   unless it has a value >= 0.
		//
		// Another one: When a best-effort payload and reliable payload are delivered together, the BE
		//              portion is considered "too new" which means it gets placed in the BE queue of
		//              the same table entry that the reliable portion goes into.  After we handle the
		//              reliable portion, subsequent BE-only packets are handled via the immediate buffer.

		// TODO:  Perhaps header.seque should be checked against receiver.CurrentSequence instead of the
		//        current received packet sequence number, because the latter can be misordered in some
		//        situations.

		const bool isHighPriority   = (header.flags & PacketHeaderType::FLAG_HIGH_PRIORITY) != 0;
		const bool isNormalPriority = not isHighPriority;
		const bool isHighTooOld     = isHighPriority   and header.sequence >= 0 and header.sequence < GetCurrentReceivedPacketSequenceNumber();
		const bool isNormalTooNew   = isNormalPriority and header.sequence > GetCurrentReceivedPacketSequenceNumber();

		// No matter what, we want to handle any acks and ping requests that are in the packet
		// immediately, regardless of whether they're older or newer than expected.  These are
		// critical to keeping retransmissions under control under rough network conditions.
		// We do this by copying these messages into the immediate receive buffer so that they
		// get evaluated very shortly after we return from this function, assuming
		// NextReceivedPacket() is called shortly thereafter.

		if (isHighTooOld or isNormalTooNew)
		{
			const size_t startOfBestEffort = io_rawBuf.TellRead();
			while (io_rawBuf.BytesLeftToRead())
			{
				const auto msgFormatID = msg_t(io_rawBuf.ReadUnVarint());
				switch (msgFormatID)
				{
					case msg_ack:
						m_immediateReceiveBuffer.WriteUnVarint(msgFormatID);
						m_immediateReceiveBuffer.WriteLong(io_rawBuf.ReadLong());   // sequence number
						break;

					case svc_pingrequest:
						{
							const size_t msgSize = io_rawBuf.ReadUnVarint();
							m_immediateReceiveBuffer.WriteUnVarint(msgFormatID);
							m_immediateReceiveBuffer.WriteUnVarint(msgSize);
							m_immediateReceiveBuffer.WriteChunk(io_rawBuf.ReadChunk(msgSize), msgSize);
						}
						break;

					default:
						io_rawBuf.SeekRead(io_rawBuf.ReadUnVarint(), buf_t::BT_CURRENT);    // read msg size + skip
						break;
				}
			}
			io_rawBuf.SeekRead(startOfBestEffort, buf_t::BT_START);
		}

		if (isNormalTooNew)
		{
			// Anything else in this "too new" best-effort payload will be handled after its reliable packet comes in.
			// FYI - It doesn't hurt to have a duplicate ack handled whenever the owning packet is considered "current".

			m_receiver.RegisterBestEffortPacket(header, bestEffortSize, io_rawBuf);
		}
		else if (not isHighTooOld)
		{
			m_immediateReceiveBuffer.WriteChunk(io_rawBuf.ReadChunk(bestEffortSize), bestEffortSize);
		}

		if (m_immediateReceiveBuffer.size())
		{
			m_immediateReceiveHeader = header;
			return MessageResultEnum::ACCEPT;
		}

#else // ... we're not deferring best-effort packet reception.  Do it immediately!

		m_immediateReceiveBuffer.WriteChunk(io_rawBuf.ReadChunk(bestEffortSize), bestEffortSize);
		m_immediateReceiveHeader = header;
		return MessageResultEnum::ACCEPT;

#endif

	}

	return MessageResultEnum::DEFER;
}

bool OdaMessenger::NextReceivedPacket(buf_t& io_rawBuf)
{
	if (m_isBitBucket or m_sender.GetMode() == SequenceSender::CRITICAL_FAILURE)
	{
		return false;
	}

	// Make sure that a high-priority immediate receive buffer goes first, even ahead of
	// any reliable packets.
	if (m_immediateReceiveBuffer.size() and (m_immediateReceiveHeader.flags & PacketHeaderType::FLAG_HIGH_PRIORITY) != 0)
	{
		io_rawBuf.swap(m_immediateReceiveBuffer);
		m_immediateReceiveBuffer.clear();

		m_receivedHeader = m_immediateReceiveHeader;
		return true;
	}

	// Now do the reliable packets.
	if (const auto nextHeader = m_receiver.NextPacket(io_rawBuf))
	{
		m_receivedHeader = nextHeader.value();      // it's a std::optional.
		return true;
	}

	// Finally do any best-effort mobj updates...  Please note that this can still be an
	// out-of-order reception if the best-effort update originated after a dropped or
	// jittered-out reliable packet.
	if (m_immediateReceiveBuffer.size())
	{
		io_rawBuf.swap(m_immediateReceiveBuffer);
		m_immediateReceiveBuffer.clear();

		m_receivedHeader = m_immediateReceiveHeader;
		return true;
	}

	return false;
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

	if (m_isBitBucket or simulated_connection)
	{
		Clear();
	}

	// Phase zero:  Detect if the client has become dangerously non-responsive,
	//              and abort if so.
	if (m_criticalSequenceTimeoutInTics > 0)
	{
		if (SequenceQueueEntryType* oldestOutgoingUnackedEntry = m_sender.IterateUnackedPackets().Next())
		{
			if (i_currentTic > oldestOutgoingUnackedEntry->header.originatorTic + m_criticalSequenceTimeoutInTics)
			{
				m_sender.SetMode(SequenceSender::CRITICAL_FAILURE);
				return MessageResultEnum::ABORT;
			}
		}
	}

	// First phase - send high-priority non-reliables (acks, servertic, player updates)
	size_t bytesSentBestEffort = 0;
	while (m_outgoingHighNonReliableQueue.SizeInMessages() > 0 and m_byteBudget > 0)
	{
		m_outgoingHighNonReliableQueue.Pack([this](const buf_t& buf) { return PackAsUnreliable(m_highPacket, buf); });

		const size_t sendSize = m_highPacket.SendHighPriority(i_currentTic, m_destinationTic, m_sender, i_dest);
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

		const size_t sendSize = m_packet.Send(i_currentTic, m_destinationTic, m_sender, i_dest);
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
				const size_t bestEffortBytes = m_packet.Send(i_currentTic, m_destinationTic, m_sender, i_dest);
				bytesSentBestEffort         += bestEffortBytes;
				m_byteBudget                -= static_cast<int>(bestEffortBytes);
			}
			else
			{
				const size_t reliableBytes = m_packet.Send(i_currentTic, m_destinationTic, m_sender, i_dest);
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
	const size_t lastTotalSent         = m_packet.Send(i_currentTic, m_destinationTic, m_sender, i_dest);
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
	if (m_isBitBucket)
	{
		return 0;
	}

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
	int previousPacketSeq          = sendQueueEntry ? sendQueueEntry->header.sequence - 1 : -1;
	m_noncontiguousRetransmitCount = 0;

	for (; sendQueueEntry != nullptr; sendQueueEntry = iter.Next())
	{
		// m_retransmitDelayInTics used to be constrained here via std::min(m_retransmitDelayInTics, 5).
		// I wish I had documented exactly why it was not allowed to be more than 5 for quite a while
		// during development.
		//
		// In any case, natural scaling works well now, probably because we have a working throttle.
		if (i_currentTic >= (m_retransmitDelayInTics + sendQueueEntry->header.originatorTic) or sendQueueEntry->lastRetransmitTic != -1)
		{
			if (++retransmissionsSent > m_maxPacketsPerRetransmission or m_byteBudget <= 0)
			{
				break;
			}
			m_noncontiguousRetransmitCount += previousPacketSeq != sendQueueEntry->header.sequence - 1 ? 1 : 0;
			previousPacketSeq = sendQueueEntry->header.sequence;

			sendQueueEntry->lastRetransmitTic = i_currentTic;
			const int resendSize = static_cast<int>(m_packet.ReSend(sendQueueEntry->header.originatorTic,
			                                                        sendQueueEntry->header.destinationTic,
			                                                        sendQueueEntry->header.sequence,
			                                                        sendQueueEntry->buf,
			                                                        i_dest));
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
