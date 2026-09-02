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
#pragma once

#include <memory>
#include <memory_resource>

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

		// With theoretical defaults:
		//      800 KBps * 5 sec = 4000 KB backed-up retransmits max
		//      4000 KB * 256 players = 1024000 KB total ~= 1.05 GB in memory at absolute worst
		constexpr static int DEFAULT_CRITICAL_SEQUENCE_TIMEOUT_IN_TICS =  5 * TICRATE;

		explicit OdaMessenger(std::unique_ptr<std::pmr::unsynchronized_pool_resource>& i_poolPtr)
			: m_sender   { DEFAULT_RELIABILITY_QUEUE_SIZE, std::pmr::polymorphic_allocator<SequenceQueueEntryType> {i_poolPtr.get()}}
			, m_receiver { DEFAULT_RELIABILITY_QUEUE_SIZE, std::pmr::polymorphic_allocator<SequenceQueueEntryType> {i_poolPtr.get()}}
		{
		}

		OdaMessenger(const OdaMessenger&)            = delete;
		OdaMessenger& operator=(const OdaMessenger&) = delete;

		OdaMessenger(OdaMessenger&&)            = default;
		OdaMessenger& operator=(OdaMessenger&&) = default;

		//  -------------- Basic state management --------------
		void SetBitBucket(bool i_isBitBucket) { m_isBitBucket = i_isBitBucket; }

		/// The GetCurrentReceived* accessors return the associated tic numbers of the
		/// packet *currently being processed* in accordance with the most recent call
		/// to NextReceivedPacket().  From call to call of NextReceivedPacket(), these
		/// values should be expected to change and reflect the tic numbers contained
		/// within and associated with the messages contained in that packet.
		const PacketHeaderType& GetCurrentReceivedPacketHeader() const { return m_receivedHeader; }

		int  GetCurrentReceivedPacketSequenceNumber() const { return m_receivedHeader.sequence; }
		int  GetCurrentReceivedRemoteTic() const            { return m_receivedHeader.originatorTic; }
		int  GetCurrentReceivedLocalTic() const             { return m_receivedHeader.destinationTic; }
		bool GetCurrentReceivedIsReliable() const           { return m_receivedHeader.reliableSize > 0; }
		bool GetCurrentReceivedIsHighPriority() const       { return (m_receivedHeader.flags & PacketHeaderType::FLAG_HIGH_PRIORITY) != 0; }

		/// The reason we want to allow explicitly setting the destination tic for outbound headers
		/// is that the server *might not* have used the absolute latest-available command from the
		/// client, and the client uses the newest available.  It's up to the application.
		void SetDestinationTic(int i_tic) { m_destinationTic = i_tic; }

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
		///
		/// PLEASE NOTE: Upon success, the timing / tic numbers associated with the GetCurrentReceived* accessor functions
		///              will be updated to reflect the tic numbers in the received packet.  Please see their descriptions.
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
		///  ABORT  - Critical error sending: Time to drop the connection.
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
		MessageQueue& HighBuf() { return m_outgoingHighNonReliableQueue; }

		/// Discard all outgoing data that has yet to be sent.
		void Clear()
		{
			m_outgoingReliableQueue.Clear();
			m_outgoingNonReliableQueue.Clear();
			m_outgoingHighNonReliableQueue.Clear();
		}

		bool RecordingIsEnabled() const { return m_recordingIsEnabled; }
		void EnableRecording()  { m_recordingIsEnabled = true;  }
		void DisableRecording() { m_recordingIsEnabled = false; m_recordingBuffer.clear(); }

		const std::basic_string<byte>& GetRecordingBufferRef() const { return m_recordingBuffer; }

		bool MustThrottleTransmission() const { return m_sender.GetMode() == SequenceSender::RECOVERY; }
		fixed_t ThrottleFraction() const { return FixedDiv( m_unackedGrowth << FRACBITS, m_unackedGrowthThreshold << FRACBITS); }

		void SetMaxRate(int i_maxRate)
		{
			m_maxRate      = i_maxRate;
			m_perTicBudget = (m_maxRate * 1000) / TICRATE;
			m_byteBudget   = m_perTicBudget;

			// Track when reliable exceeds a certain percentage of the budget.  This could be configurable...
			m_reliableOverloadThreshold = 9 * (m_perTicBudget / 10);
		}
		void SetPacketsPerRetransmit    (int i_maxPackets)   { m_maxPacketsPerRetransmission = i_maxPackets; }
		void SetRetransmitDelay         (int i_delayInTics)  { m_retransmitDelayInTics = i_delayInTics; }
		void SetCriticalSequenceTimeout (int i_timeoutInTics){ m_criticalSequenceTimeoutInTics = i_timeoutInTics; }

		int GetLastReliableSendSize() const           { return static_cast<int>(m_bytesSentWithReliability); }
		int GetLastSendSize() const                   { return m_lastSendSize; }
		int GetMaxPacketsPerRetransmission() const    { return m_maxPacketsPerRetransmission; }
		int GetNonContiguousRetransmitPackets() const { return m_noncontiguousRetransmitCount; }
		size_t GetOutgoingSizeInBytes() const         { return m_outgoingReliableQueue.SizeInBytes()
		                                                     + m_outgoingNonReliableQueue.SizeInBytes()
		                                                     + m_outgoingHighNonReliableQueue.SizeInBytes(); }
		int GetPendingAckCount() const       { return m_sender.GetPendingAckCount(); }
		int GetReliableOverloadCount() const { return m_reliableOverloadCount; }
		int GetTicBudget() const             { return m_perTicBudget; }

	protected:

		size_t PackAsReliable  (Packet& io_packet, const buf_t& messageBuf);
		size_t PackAsUnreliable(Packet& io_packet, const buf_t& messageBuf);

		void Record(const buf_t& messageBuf);

		static void CompressPacket(buf_t& send, const size_t reserved);

		void ManageBudget(int i_currentTic);

		int SendOldPacket(const SequenceQueueEntryType& queueEntry, const netadr_t& i_dest);

		SequenceSender   m_sender;
		SequenceReceiver m_receiver;

		Packet m_packet;
		Packet m_highPacket;

		PacketHeaderType m_receivedHeader;

		// Send buffers
		MessageQueue m_outgoingReliableQueue;
		MessageQueue m_outgoingNonReliableQueue;
		MessageQueue m_outgoingHighNonReliableQueue;

		buf_t            m_immediateReceiveBuffer{ MAX_UDP_PACKET };
		PacketHeaderType m_immediateReceiveHeader;

		int m_maxPacketsPerRetransmission   { DEFAULT_RETRANSMISSIONS_PER_TIC };
		int m_retransmitDelayInTics         { 0 };
		int m_maxRate                       { 0 };
		int m_criticalSequenceTimeoutInTics { DEFAULT_CRITICAL_SEQUENCE_TIMEOUT_IN_TICS };

		int m_byteBudget    {  0 };       ///< The live budget.  Signed so that it can also represent debt.
		int m_perTicBudget  {  0 };       ///< The value used to reset the budget every tic.
		int m_latchedTic    { -1 };       ///< Used for detecting new tics and resetting the budget.
		int m_destinationTic{ -1 };       ///< The remote tic that we're supposed to echo back to the other end.

		int m_reliableOverloadThreshold { 0 };
		int m_reliableOverloadCount     { 0 };

		std::basic_string<byte> m_recordingBuffer;
		bool                    m_recordingIsEnabled { false };

		bool m_isBitBucket { false };   ///< Set this true to always discard all data.
		                                ///< Use it to make a transient "stub" messenger for disconnecting clients.

		// Metrics
		size_t  m_bytesSentWithReliability      {  0 };
		int     m_lastSendSize                  {  0 };
		int     m_noncontiguousRetransmitCount  {  0 };
		int     m_previousUnackedCount          {  0 };
		int     m_unackedGrowth                 {  0 };
		int     m_unackedGrowthThreshold        { 10 };
};
