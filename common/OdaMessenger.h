// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
#pragma once

#include "MessageQueue.h"
#include "Packet.h"
#include "SequenceReceiver.h"
#include "SequenceSender.h"

enum class MessageResultEnum
{
	ACCEPT,
	DEFER,
	ABORT
};

class OdaMessenger
{
	const static int DEFAULT_RETRANSMISSIONS_PER_TIC = 25;

	public:

		//  -------------- Receiving functions --------------

		/// Receive and enqueue a packet for processing.  Every packet received with this function must be subsequently fetched via
		/// NextReceivedPacket().
		///
		///  io_rawBuf - the buffer from which to receive data.  Must be a valid buffer containing a complete packet with nothing yet read
		///              from it.  After returning from this function, the given buffer will be left in a valid but indeterminant state.
		///
		/// Return values:
		/// ABORT  - Malformed message.
		/// DEFER  - The receive buffer had only reliable data.  You can defer processing and call Receive() again without losing data.
		/// ACCEPT - Unreliable data and/or acks are awaiting immediately.  Obtain via NextReceivedPacket() and process it before the
		///          next call to Receive() or LOSE IT FOREVER.
		///          Seriously, handle it immediately, because you don't want to let Acks hit the floor!
		MessageResultEnum Receive(buf_t& io_rawBuf);

		/// Fetches the next packet for processing and moves its content into the given raw buffer.  This provides packets
		/// that have been received via the Receive() API, including reliable and unreliable data.  Unreliable data is
		/// available immediately, and reliable data is fetched only in contiguous order.  It is HIGHLY RECOMMENDED to
		/// call this function immediately after ACCEPT is returned from Receive, and then repeatedly after all Receiving is
		/// finished from the associated socket to ensure reliable data is handled in a timely manner.
		//
		/// Returns true if data was available and has been moved into the given raw buffer, false otherwise.
		bool NextReceivedPacket(buf_t& io_rawBuf);

		/// Optional function to handle whatever Acknowledgements sit at the front of the given buffer.
		/// You only need to call this function if you don't want to handle the acks yourself, which there are
		/// certainly cases where you'll likely want to do that.  If there are Acks at the front of the
		/// buffer, they will be consumed and the buffer will be left in a state where the buffer is either
		/// empty or its next message is the first non-ack message in the buffer.
		void HandleAcks(buf_t& io_rawBuf);

		//  -------------- Sending functions --------------

		/// Assembles all new packets from enqueued outgoing messages and transmits them.  Packets and their content are ordered as
		/// Reliable content first, followed by Acks, followed by any remaining non-reliable messages.  The number of packets is
		/// determined by the number of enqueued messages and constrained by MaxRate (see SetMaxRate()).  If the MaxRate cap is
		/// hit, Reliable and Ack messages remain enqueued for subsequent SendAll, but all remaining non-reliable messages are
		/// discarded.
		///
		/// Return values:
		///  ACCEPT - Normal result: Packet(s) were sent as needed without hitting a cap.
		///  DEFER  - Packet(s) may have been sent, but a cap has been enountered.
		MessageResultEnum SendAll(int i_currentTic, const netadr_t& i_dest);

		/// Retransmit the oldest reliable packets that were previously sent and are older than RetransmitDelay
		/// (see Get/Set methods) but haven't yet been acknowledged.
		///
		/// Up to MaxPacketsPerRetransmission (see Get/Set methods) packets may be sent.  Please note that
		/// only the reliable portions of old packets are retransmitted - if the original packet had both
		/// reliable and unreliable data, the unreliable data is NOT included in the retransmission.  If there
		/// are no unacknowledged reliable packets older than the RetransmitDelay, nothing is sent.
		///
		/// Returns the number of bytes sent as part of this retransmission cycle.
		int HandleRetransmissions(int i_currentTic, const netadr_t& i_dest);


		/// Mark a previously-sent reliable message as having been acknowledged by the recipient.  If the old
		/// message has been included in retransmissions, then it stops being retransmitted.
		///
		/// Returns true if this is the first acknowledgement of the given sequence.  False otherwise.
		bool Acknowledge(int sequence);

		/// Return the requested message queue.  Use these queues to Obtain new messages into which to pack
		/// new outgoing data.
		MessageQueue& ReliableBuf() { return m_outgoingReliableQueue; }
		MessageQueue& NetBuf() { return m_outgoingNonReliableQueue; }

		/// Discard all outgoing data that has yet to be sent.
		void Clear()
		{
			m_outgoingReliableQueue.Clear();
			m_outgoingNonReliableQueue.Clear();
			m_outgoingAckQueue.Clear();
		}

		bool MustThrottleTransmission() const { return m_sender.GetMode() == SequenceSender::RECOVERY; }
		fixed_t ThrottleFraction() const { return FixedDiv( m_unackedGrowth << FRACBITS, m_unackedGrowthThreshold << FRACBITS); }

		void SetRetransmitDelay(int i_delayInTics) { m_retransmitDelayInTics = i_delayInTics; }
		void SetPacketsPerRetransmit(int i_maxPackets) { m_maxPacketsPerRetransmission = i_maxPackets; }
		int  GetMaxPacketsPerRetransmission() const { return m_maxPacketsPerRetransmission; }
		void SetMaxRate(int i_maxRate)
		{
			m_maxRate      = i_maxRate;
			m_perTicBudget = (m_maxRate * 1000) / TICRATE;
			m_byteBudget   = m_perTicBudget;
		}

		int GetLastSendSize() const { return m_lastSendSize; }
		int GetPendingAckCount() const { return m_sender.GetPendingAckCount(); }
		int GetNonContiguousRetransmitPackets() const { return m_noncontiguousRetransmitCount; }

	protected:

		static void CompressPacket(buf_t& send, const size_t reserved);

		void ManageBudget(int i_currentTic);

		int SendOldPacket(const SequenceQueueEntryType& queueEntry, const netadr_t& i_dest);

		SequenceSender   m_sender;
		SequenceReceiver m_receiver;

		Packet m_packet;

		// Send buffers
		MessageQueue m_outgoingReliableQueue;
		MessageQueue m_outgoingNonReliableQueue;
		MessageQueue m_outgoingAckQueue;

		buf_t* m_quickTurnaroundReceiveBuffer { nullptr };

		int m_maxPacketsPerRetransmission { DEFAULT_RETRANSMISSIONS_PER_TIC };
		int m_retransmitDelayInTics       { 0 };
		int m_maxRate                     { 0 };

		int m_byteBudget  {  0 };       ///< The live budget.  Signed so that it can also represent debt.
		int m_perTicBudget{  0 };       ///< The value used to reset the budget every tic.
		int m_latchedTic  { -1 };       ///< Used for detecting new tics and resetting the budget.

		// Metrics
		int m_lastSendSize                 {  0 };
		int m_noncontiguousRetransmitCount {  0 };
		int m_previousUnackedCount         {  0 };
		int m_unackedGrowth                {  0 };
		int m_unackedGrowthThreshold       { 10 };
};
