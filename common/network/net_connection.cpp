// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
// Connection establishment and management	
//
//-----------------------------------------------------------------------------

#include <stdarg.h>
#include "i_system.h"
#include "c_cvars.h"
#include "md5.h"
#include "m_ostring.h"

#include "network/net_type.h"
#include "network/net_common.h"
#include "network/net_log.h"
#include "network/net_interface.h"
#include "network/net_connection.h"
#include "network/net_messagemanager.h"

#include "minilzo.h"

#include "cl_main.h"

EXTERN_CVAR(net_maxrate)


// ============================================================================
//
// ConnectionStatistics
//
// ============================================================================

static const double AVG_WEIGHT = 1.0 / 16.0;


ConnectionStatistics::ConnectionStatistics()
{
	clear();
}


//
// ConnectionStatistics::clear
//
void ConnectionStatistics::clear()
{
	mOutgoingBits = mIncomingBits = 0;
	mOutgoingPackets = mIncomingPackets = 0;
	mLostOutgoingPackets = mLostIncomingPackets = 0;

	while (!mOutgoingTimestamps.empty())
		mOutgoingTimestamps.pop();

	mAvgRoundTripTime = 0.0;
	mAvgJitterTime = 0.0;
	mAvgOutgoingPacketLoss = mAvgIncomingPacketLoss = 0.0;

	mAvgOutgoingBitrate = mAvgIncomingBitrate = 0.0;
	mAvgOutgoingSize = mAvgIncomingSize = 0.0;
}


//
// ConnectionStatistics::outgoingPacketSent
//
void ConnectionStatistics::outgoingPacketSent(uint16_t size)
{
	const dtime_t current_timestamp = Net_CurrentTime();

	if (mLastOutgoingTimestamp > 0)
	{
		// calculate how much time elapsed since the last packet was sent
		double time_since_last_send = double(current_timestamp - mLastOutgoingTimestamp) / ONE_SECOND;
		double current_outgoing_bitrate = double(size) / time_since_last_send;

		mAvgOutgoingBitrate = mAvgOutgoingBitrate * (1.0 - AVG_WEIGHT) + current_outgoing_bitrate * AVG_WEIGHT;
	}

	if (mAvgOutgoingSize > 0.0)
		mAvgOutgoingSize = mAvgOutgoingSize * (1.0 - AVG_WEIGHT) + size * AVG_WEIGHT;
	else
		mAvgOutgoingSize = double(size);

	// add the timestamp to the queue
	mOutgoingTimestamps.push(current_timestamp);

	mOutgoingPackets++;	
	mOutgoingBits += size;

	mLastOutgoingTimestamp = current_timestamp;	
}


//
// ConnectionStatistics::outgoingPacketAcknowledged
//
void ConnectionStatistics::outgoingPacketAcknowledged()
{
	const dtime_t current_timestamp = Net_CurrentTime();

	// calculate round-trip-time based on current time and timestamp
	// the packet was sent
	assert(mOutgoingTimestamps.empty() == false);
	double current_rtt = double(current_timestamp - mOutgoingTimestamps.front()) / ONE_SECOND;

	mAvgJitterTime = mAvgJitterTime * (1.0 - AVG_WEIGHT) + (current_rtt - mAvgRoundTripTime) * AVG_WEIGHT;

	if (mAvgRoundTripTime > 0.0)
		mAvgRoundTripTime = mAvgRoundTripTime * (1.0 - AVG_WEIGHT) + current_rtt * AVG_WEIGHT;
	else
		mAvgRoundTripTime = current_rtt;

	mAvgOutgoingPacketLoss = mAvgOutgoingPacketLoss * (1.0 - AVG_WEIGHT);

	mOutgoingTimestamps.pop();
}


//
// ConnectionStatistics::outgoingPacketLost
//
void ConnectionStatistics::outgoingPacketLost()
{
	mAvgOutgoingPacketLoss = mAvgOutgoingPacketLoss * (1.0 - AVG_WEIGHT) + AVG_WEIGHT;

	mLostOutgoingPackets++;

	mOutgoingTimestamps.pop();
}


//
// ConnectionStatistics::incomingPacketReceived
//
void ConnectionStatistics::incomingPacketReceived(uint16_t size)
{
	const dtime_t current_timestamp = Net_CurrentTime();

	if (mLastIncomingTimestamp > 0)
	{
		// calculate how much time elapsed since the last packet was received
		double time_since_last_recv = double(current_timestamp - mLastIncomingTimestamp) / ONE_SECOND;
		double current_incoming_bitrate = double(size) / time_since_last_recv;

		mAvgIncomingBitrate = mAvgIncomingBitrate * (1.0 - AVG_WEIGHT) + current_incoming_bitrate * AVG_WEIGHT;
	}

	mIncomingPackets++;
	mIncomingBits += size;

	mLastIncomingTimestamp = current_timestamp;
}


//
// ConnectionStatistics::incomingPacketLost
//
void ConnectionStatistics::incomingPacketLost()
{
	mAvgIncomingPacketLoss = mAvgIncomingPacketLoss * (1.0 - AVG_WEIGHT) + AVG_WEIGHT;
	mLostIncomingPackets++;
}


// ============================================================================
//
// Connection class Implementation
//
// ============================================================================

Connection::Connection(const ConnectionId& connection_id, NetInterface* interface, const SocketAddress& adr) :
	mConnectionId(connection_id),
	mInterface(interface), mRemoteAddress(adr),
	mTerminated(false),
	mHandshake(NULL),
	mCreationTS(Net_CurrentTime()),
	mTimeOutTS(Net_CurrentTime() + CONNECTION_TIMEOUT),
	mConnectionAttempt(0), mConnectionAttemptTimeOutTS(0)
{ }


Connection::~Connection()
{
	delete mHandshake;
}


bool Connection::isConnected() const
{
	return !isTerminated() && !isNegotiating();
}


bool Connection::isNegotiating() const
{
	return !mTerminated && mHandshake && mHandshake->isNegotiating();
}


bool Connection::isTerminated() const
{
	return mTerminated;
}


bool Connection::isTimedOut() const
{
	return 	Net_CurrentTime() >= mTimeOutTS;
}

//
// Connection::openConnection
//
// Sends the remote host a request to establish a connection.
//
void Connection::openConnection()
{
	Net_Printf("Requesting connection to host %s...\n", getRemoteAddress().getCString());
	assert(mHandshake == NULL);
	mHandshake = createConnectionHandshake();
	service();
}


//
// Connection::closeConnection
//
// Sends the remote host a signal that the connection is ending and
// updates this connection's internal state to prevent further sending or
// receiving over this connection.
//
void Connection::closeConnection()
{
	Net_LogPrintf(LogChan_Connection, "terminating connection to host %s.", getRemoteAddress().getCString());

	delete mHandshake;		// just in case the connection is being closed during connection negotiation
	mHandshake = createDisconnectionHandshake();
	service();

	mTerminated = true;
}


//
// Connection::sendDatagram
//
// Updates statistics and hands the datagram off to NetInterface to send
//
void Connection::sendDatagram(const BitStream& stream)
{
	if (isTerminated())
		return;

	const uint16_t size = stream.bytesWritten();
	const uint8_t* data = stream.getRawData();
	if (mInterface->socketSend(mRemoteAddress, stream.getRawData(), stream.bytesWritten()))
		mConnectionStats.outgoingPacketSent(stream.bitsWritten());
}


//
// Connection::receiveDatagram
//
// Reads an incoming datagram's header, recording the sequence number so
// it can be acknowledged in the next outgoing datagram. Also notifies the
// registered MessageManagers of any outgoing sequence numbers the
// remote host has acknowledged receiving or lost. Notification is guaranteed
// to be handled in order of sequence number.
//
// Inspects the type of an incoming datagram and then hands the packet off to
// the appropriate parsing function.
//
void Connection::receiveDatagram(BitStream& stream)
{
	if (isTerminated())
		return;

	// reset the timeout timestamp
	mTimeOutTS = Net_CurrentTime() + CONNECTION_TIMEOUT;

	mConnectionStats.incomingPacketReceived(stream.bitsWritten());

	if (isNegotiating())
	{
		mHandshake->receive(stream);
	}

	if (isConnected())
	{
		if (validateDatagram(stream))
		{
			readDatagramHeader(stream);
			parseGameDatagram(stream);
		}
	}
}


//
// Connection::service
//
void Connection::service()
{
	if (isTimedOut())
	{
		Net_LogPrintf(LogChan_Connection, "connection to host %s timed-out.",
							getRemoteAddress().getCString());
		closeConnection();
	}

	if (mHandshake)
	{
		mHandshake->service();

		if (mHandshake->isFailed())
			mTerminated = true;

		if (!mHandshake->isNegotiating())
		{
			delete mHandshake;
			mHandshake = NULL;
		}
	}

	// compose a new game packet and send it
	if (isConnected())
	{
		BitStream stream; 
		composeGameDatagram(stream, 0);
		sendDatagram(stream);
	}

	/*
	Printf(PRINT_HIGH, "outgoing bitrate: %0.2f, incoming bitrate: %0.2f\n",
		mConnectionStats.getOutgoingBitrate(), mConnectionStats.getIncomingBitrate());
	*/
}


//
// Connection::registerMessageManager
//
// Registers a MessageManager object that will handle composing or parsing the
// payload portion of outgoing or incoming game packets. The composing/parsing
// functions will be called order in which they are registered.
//
void Connection::registerMessageManager(MessageManager* manager)
{
	if (manager != NULL)
		mMessageManagers.push_back(manager);
}


//
// Connection::composeGameDatagram
//
// Writes the header for a new game packet and calls the registered
// composition callbacks to write game data to the packet.
//
void Connection::composeGameDatagram(BitStream& stream, uint32_t seq)
{
	Net_LogPrintf(LogChan_Connection, "composing datagram to host %s.", getRemoteAddress().getCString());

	composeDatagramHeader(stream);

	// TODO: determine packet_size using flow-control methods
	const uint32_t bandwidth = 64;	// 64kbps
	uint16_t packet_size = bandwidth * 1000 / TICRATE;
	packet_size = std::min(packet_size, stream.writeSize());

	// call the registered packet composition functions to compose the payload

	uint16_t size_left = packet_size;
	for (std::vector<MessageManager*>::iterator it = mMessageManagers.begin(); it != mMessageManagers.end(); ++it)
	{
		MessageManager* composer = *it;
		size_left -= composer->write(stream, size_left, seq);
	}
	CL_SendCmd(stream);

	composeDatagramFooter(stream);
}


//
// Connection::parseGameDatagram
//
// Handles acknowledgement of packets
//
void Connection::parseGameDatagram(BitStream& stream)
{
	Net_LogPrintf(LogChan_Connection, "processing datagram from host %s.", getRemoteAddress().getCString());

	// call the registered packet parser functions to parse the payload
	uint16_t size_left = stream.readSize();
	for (std::vector<MessageManager*>::iterator it = mMessageManagers.begin(); it != mMessageManagers.end(); ++it)
	{
		MessageManager* parser = *it;
		size_left -= parser->read(stream, size_left);
	}

	CL_ParseCommands(stream);
}


//
// Connection::remoteHostReceivedPacket
//
// Called when the Connection receives notice that the remote host received
// a packet sent by this Connection. Connection quality statistics are updated
// and the registered MessageManagers are notified of the packet's receipt.
//
void Connection::remoteHostReceivedPacket(uint32_t seq)
{
	Net_LogPrintf(LogChan_Connection, "remote host %s received packet %u.", 
			getRemoteAddress().getCString(), seq);

	mConnectionStats.outgoingPacketAcknowledged();

	if (isConnected())
	{
		for (MessageManagerList::iterator it = mMessageManagers.begin(); it != mMessageManagers.end(); ++it)
		{
			MessageManager* manager = *it;
			manager->notifyReceived(seq);
		}
	}
}


//
// Connection::remoteHostLostPacket
//
// Called when the Connection receives notice that the remote host lost
// a packet sent by this Connection. Connection quality statistics are updated
// and the registered MessageManagers are notified of the packet's loss.
//
void Connection::remoteHostLostPacket(uint32_t seq)
{
	Net_LogPrintf(LogChan_Connection, "remote host %s did not receive packet %u.", 
			getRemoteAddress().getCString(), seq);

	mConnectionStats.outgoingPacketLost();

	if (isConnected())
	{
		for (MessageManagerList::iterator it = mMessageManagers.begin(); it != mMessageManagers.end(); ++it)
		{
			MessageManager* manager = *it;
			manager->notifyLost(seq);
		}
	}
}


// ============================================================================
//
// Odamex Protocol Version 65 connection implementation
//
// ============================================================================

//
// Odamex65Connection::Odamex65Connection
//
Odamex65Connection::Odamex65Connection(const ConnectionId& connection_id, NetInterface* interface, const SocketAddress& adr) :
	Connection(connection_id, interface, adr),
	mSequence(0), mLastAckSequence(0), mRecvSequence(-1)
{ }


//
// Odamex65Connection::readDatagramHeader
//
// Parses a datagram's header, extracting the incoming datagram sequence number,
// the last outgoing datagram sequence number acknowledged by the remote host,
// and all of the datagrams the remote host acknowledges having received.
//
void Odamex65Connection::readDatagramHeader(BitStream& stream)
{
	PacketSequenceNumber in_seq;
	in_seq.read(stream);

	Net_LogPrintf(LogChan_Connection, "processing game datagram from host %s, sequence %u.",
					getRemoteAddress().getCString(), in_seq.getInteger());

	int32_t diff = SequenceNumberDifference(in_seq, mRecvSequence);
	if (diff > 0)
	{
		// if diff > 1, there is a gap in received sequence numbers and
		// indicates incoming packets have been lost 
		for (int32_t i = 1; i < diff; i++)
			mConnectionStats.incomingPacketLost();

		// Record the received sequence number and update the bitfield that
		// stores acknowledgement for the N previously received sequence numbers.

		// shift the window of received sequence numbers to make room
		mRecvHistory <<= diff;
		// add the previous sequence received
		mRecvHistory.set(diff - 1);

		mRecvSequence = in_seq;
	}
	else
	{
		// drop out-of-order packets
		Net_Warning("dropping out-of-order datagram from host %s " \
				"(current sequence %u, previous sequence %u).",
				getRemoteAddress().getCString(), in_seq.getInteger(), mRecvSequence.getInteger());
		stream.clear();
	}

	if (stream.peekU8() == svc_compressed)
	{
		// Decompress the packet sequence
		stream.readU8();		// read and ignore svc_compressed
		decompressDatagram(stream);
	}
}


//
// Odamex65Connection::decompressDatagram
//
// Decompresses the BitStream.
//
// Supports LZO compression method only.
// [Russell] - reason this was failing is because of huffman routines, so just
// use minilzo for now (cuts a packet size down by roughly 45%), huffman is the
// if 0'd sections
//
void Odamex65Connection::decompressDatagram(BitStream& stream)
{
	uint8_t compression_method = stream.readU8();

	const size_t uncompressed_buf_capacity = BitsToBytes(BitStream::MAX_CAPACITY);
	uint8_t uncompressed_buf[8192];

	if (compression_method & minilzo_mask)
	{
		lzo_uint uncompressed_size = uncompressed_buf_capacity;

		// decompress back onto the receive buffer
		const uint8_t* compressed_buf = stream.getRawData() + stream.bytesRead();
		uint16_t compressed_size = stream.bytesWritten() - stream.bytesRead();

		int ret = lzo1x_decompress_safe(compressed_buf, compressed_size, uncompressed_buf, &uncompressed_size, NULL);

		if (ret == LZO_E_OK)
		{
			stream.clear();
			stream.writeBlob(uncompressed_buf, BytesToBits(uncompressed_size));
		}
		else
		{
			Printf(PRINT_HIGH, "Error: minilzo packet decompression failed with error %X\n", ret);
			stream.clear();
			return;
		}
	}
}


//
// Odamex65Connection::composeDatagramHeader
//
// Writes the header portion of a new datagram.
// Version 65 Protocol only acknowledges datagrams from the server. Gross.
//
void Odamex65Connection::composeDatagramHeader(BitStream& stream)
{
	Net_LogPrintf(LogChan_Connection, "writing datagram header to host %s, sequence %u.",
				getRemoteAddress().getCString(), mSequence.getInteger());

	if (getInterface()->getHostType() == HOST_CLIENT)
	{
		// Add individual "clc_ack" acknowledgement messages for the last N datagrams received
		for (uint16_t i = mRecvHistory.size() - 1; i >  0; i--)
		{
			if (mRecvHistory.get(i))
			{
				PacketSequenceNumber seq(mRecvSequence - i);
				stream.writeU8(clc_ack);
				seq.write(stream);
			}
		}

		stream.writeU8(clc_ack);
		mRecvSequence.write(stream);
	}
	else if (getInterface()->getHostType() == HOST_SERVER)
	{
		// Only the server provides sequence numbers in the datagrams it sends
		mSequence.write(stream);
	}

	mSequence++;
}


//
// Odamex65Connection::composeDatagramFooter
//
void Odamex65Connection::composeDatagramFooter(BitStream& stream)
{
	// do nothing
}


// Odamex64Connection::validateDatagram
//
// Validates that the datagram is correct. This is a no-op for now.
//
bool Odamex65Connection::validateDatagram(const BitStream& stream) const
{
	return true;
}



// ============================================================================
//
// Odamex Protocol Version 100 connection implementation
//
// ============================================================================

//
// Odamex100Connection::Odamex100Connection
//
Odamex100Connection::Odamex100Connection(const ConnectionId& connection_id, NetInterface* interface, const SocketAddress& adr) :
	Connection(connection_id, interface, adr),
	mSequence(rand()),
	mLastAckSequenceValid(false), mRecvSequenceValid(false)
{ }


//
// Odamex100Connection::readDatagramHeader
//
// Parses a datagram's header, extracting the incoming datagram sequence number,
// the last outgoing datagram sequence number acknowledged by the remote host,
// and all of the datagrams the remote host acknowledges having received.
//
void Odamex100Connection::readDatagramHeader(BitStream& stream)
{
	PacketType packet_type;
	PacketSequenceNumber in_seq;
	PacketSequenceNumber ack_seq;
	BitField<32> ack_history;

	packet_type = static_cast<PacketType>(stream.readBits(1));
	in_seq.read(stream);
	ack_seq.read(stream);
	ack_history.read(stream);

	Net_LogPrintf(LogChan_Connection, "processing %s datagram from host %s, sequence %u.",
					packet_type == NEGOTIATION_PACKET ? "negotiation" : "game",
					getRemoteAddress().getCString(), in_seq.getInteger());

	if (mRecvSequenceValid)
	{
		int32_t diff = SequenceNumberDifference(in_seq, mRecvSequence);
		if (diff > 0)
		{
			// if diff > 1, there is a gap in received sequence numbers and
			// indicates incoming packets have been lost 
			for (int32_t i = 1; i < diff; i++)
				mConnectionStats.incomingPacketLost();

			// Record the received sequence number and update the bitfield that
			// stores acknowledgement for the N previously received sequence numbers.

			// shift the window of received sequence numbers to make room
			mRecvHistory <<= diff;
			// add the previous sequence received
			mRecvHistory.set(diff - 1);
		}
		else
		{
			// drop out-of-order packets
			Net_Warning("dropping out-of-order datagram from host %s " \
					"(current sequence %u, previous sequence %u).",
					getRemoteAddress().getCString(), in_seq.getInteger(), mRecvSequence.getInteger());
			stream.clear();
			return;
		}
	}

	mConnectionStats.incomingPacketReceived(stream.bitsWritten());
	mRecvSequence = in_seq;

	if (mLastAckSequenceValid)
	{
		// sanity check (acknowledging a datagram we haven't sent yet?!?!)
		if (ack_seq >= mSequence)
		{
			Net_Warning("remote host %s acknowledges an unknown datagram.", getRemoteAddress().getCString());
			return;
		}

		// sanity check (out of order acknowledgement)
		if (ack_seq < mLastAckSequence)
		{
			Net_Warning("out-of-order datagram acknowledgement from remote host %s.", getRemoteAddress().getCString());
			return;
		}

		for (PacketSequenceNumber seq = mLastAckSequence + 1; seq <= ack_seq; ++seq)
		{
			if (seq < ack_seq - ACKNOWLEDGEMENT_COUNT)
			{
				// seq is too old to fit in the ack_history window
				// consider it lost
				remoteHostLostPacket(seq.getInteger());
			}
			else if (seq == ack_seq)
			{
				// seq is the most recent acknowledgement in the header
				remoteHostReceivedPacket(seq.getInteger());
			}
			else
			{
				// seq's delivery status is indicated by a bit in the
				// ack_history bitfield
				int32_t bit = SequenceNumberDifference(ack_seq, seq - 1);
				if (ack_history.get(bit))
					remoteHostReceivedPacket(seq.getInteger());
				else
					remoteHostLostPacket(seq.getInteger());
			}
		}
	}

	mLastAckSequence = ack_seq;
}


//
// Odamex100Connection::composeDatagramHeader
//
// Writes the header portion of a new datagram. The type of the datagram is
// written first, followed by the outgoing sequence number and then the
// acknowledgement of receipt for recent datagrams.
//
void Odamex100Connection::composeDatagramHeader(BitStream& stream)
{
	Net_LogPrintf(LogChan_Connection, "writing datagram header to host %s, sequence %u.",
				getRemoteAddress().getCString(), mSequence.getInteger());

	stream.writeBit(GAME_PACKET);
	mSequence.write(stream);
	mRecvSequence.write(stream);
	mRecvHistory.write(stream);

	++mSequence;
}


//
// Odamex100Connection::composeDatagramFooter
//
void Odamex100Connection::composeDatagramFooter(BitStream& stream)
{
	// Ensure stream is byte-aligned prior to writing the CRC32
	if (stream.bitsWritten() & 7)
		stream.writeBits(0, 8 - (stream.bitsWritten() & 7));

	// calculate CRC32 for the packet header & payload
	uint32_t crcvalue = Net_CRC32(stream.getRawData(), stream.bytesWritten());

	// write the calculated CRC32 value to the buffer
	stream.writeU32(crcvalue);
}


//
// Odamex100Connection::validateDatagram
//
// Validates that the datagram's CRC32 is correct. This implies that the datagram
// did not get corrupted during transit.
//
bool Odamex100Connection::validateDatagram(const BitStream& stream) const
{
	// verify that the packet header indicates that this is a game packet
	PacketType type = static_cast<PacketType>(stream.peekBit());
	if (type != GAME_PACKET)
		return false;

	// verify the calculated CRC32 value matches the value in the datagram
	uint32_t crcvalue = Net_CRC32(stream.getRawData(), stream.bytesWritten() - sizeof(uint32_t));
	BitStream footer;
	footer.writeBlob(stream.getRawData() + stream.bytesWritten() - sizeof(uint32_t), sizeof(uint32_t));
	return crcvalue != footer.readU32();
}


// ============================================================================
//
// ConnectionHandshake abstract base class implementation
//
// ============================================================================

//
// ConnectionHandshake::ConnectionHandshake
//
ConnectionHandshake::ConnectionHandshake(Connection* connection) :
	Handshake(connection),
	mState(CONN_FAILED),
	mTimeOutTS(Net_CurrentTime() + NEGOTIATION_TIMEOUT)
{
	if (mConnection->getInterface()->getHostType() == HOST_CLIENT)
		mState = CONN_REQUESTING;
	else if (mConnection->getInterface()->getHostType() == HOST_SERVER)
		mState = CONN_LISTENING;
}


//
// ConnectionHandshake::service
//
// Progresses through the handshake FSM.
//
void ConnectionHandshake::service()
{
	if (Net_CurrentTime() >= mTimeOutTS)
	{
		Net_LogPrintf(LogChan_Connection, "timeout while connecting to host %s.",
					mConnection->getRemoteAddress().getCString());
		mState = CONN_FAILED;
	}

	if (mState == CONN_FAILED)
		return;

	if (mConnection->getInterface()->getHostType() == HOST_CLIENT)
	{
		if (mState == CONN_REQUESTING)
		{
			// Request a new connection from a remote server
			if (handleRequestingState())
				mState = CONN_AWAITING_OFFER;
			else
				mState = CONN_FAILED;
		}
		if (mState == CONN_AWAITING_OFFER)
		{
			// Wait for an offer from a remote server
			if (mStream.bitsWritten() > 0)
				mState = CONN_REVIEWING_OFFER;
		}
		if (mState == CONN_REVIEWING_OFFER)
		{
			// Parse the offer from the remote server
			if (handleReviewingOfferState())
				mState = CONN_ACCEPTING_OFFER;
			else
				mState = CONN_FAILED;
			mStream.clear();
		}
		if (mState == CONN_ACCEPTING_OFFER)
		{
			// Accept the connection offer from the remote server
			if (handleAcceptingOfferState())
				mState = CONN_AWAITING_CONFIRMATION;
			else
				mState = CONN_FAILED;
		}
		if (mState == CONN_AWAITING_CONFIRMATION)
		{
			// Wait for confirmation of the connection from the remote server
			if (mStream.bitsWritten() > 0)
				mState = CONN_REVIEWING_CONFIRMATION;
		}
		if (mState == CONN_REVIEWING_CONFIRMATION)
		{
			// parse packet
			if (handleReviewingConfirmationState())
			{
				Net_LogPrintf(LogChan_Connection, "negotiation with host %s successful.",
						mConnection->getRemoteAddress().getCString());
				mState = CONN_CONNECTED;
			}
			else
				mState = CONN_FAILED;
			mStream.clear();
		}
	}
	else if (mConnection->getInterface()->getHostType() == HOST_SERVER)
	{
		if (mState == CONN_LISTENING)
		{
			// Wait for a connection request from a client
			if (mStream.bitsWritten() > 0)
				mState = CONN_REVIEWING_REQUEST;
		}
		if (mState == CONN_REVIEWING_REQUEST)
		{
			// Parse the connection request from the client
			if (handleReviewingRequestState())
				mState = CONN_OFFERING;
			else
				mState = CONN_FAILED;
			mStream.clear();
		}
		if (mState == CONN_OFFERING)
		{
			if (handleOfferingState())
				mState = CONN_AWAITING_ACCEPTANCE;
			else
				mState = CONN_FAILED;
		}
		if (mState == CONN_AWAITING_ACCEPTANCE)
		{
			if (mStream.bitsWritten() > 0)
				mState = CONN_REVIEWING_ACCEPTANCE;
		}
		if (mState == CONN_REVIEWING_ACCEPTANCE)
		{
			if (handleReviewingAcceptanceState())
				mState = CONN_CONFIRMING;
			else
				mState = CONN_FAILED;
			mStream.clear();
		}
		if (mState == CONN_CONFIRMING)
		{
			if (handleConfirmingState())
			{
				Net_LogPrintf(LogChan_Connection, "negotiation with host %s successful.",
						mConnection->getRemoteAddress().getCString());
				mState = CONN_CONNECTED;
			}
			else
				mState = CONN_FAILED;
		}
	}
}


// ============================================================================
//
// Odamex Protocol Version 65 connection handshake
//
// ============================================================================

//
// Odamex65ConnectionHandshake::handleRequestingState
//
bool Odamex65ConnectionHandshake::handleRequestingState()
{
	Net_LogPrintf(LogChan_Connection, "sending connection request datagram to host %s.",
			mConnection->getRemoteAddress().getCString());

	BitStream stream;
	stream.writeU32(SIGNAL_LAUNCHER_CHALLENGE);
	mConnection->sendDatagram(stream);
	return true;
}


//
// Odamex65ConnectionHandshake::handleReviewingOfferState
//
bool Odamex65ConnectionHandshake::handleReviewingOfferState()
{
	Net_LogPrintf(LogChan_Connection, "processing connection offer datagram from host %s.",
			mConnection->getRemoteAddress().getCString());
 
	if (mStream.readU32() == SIGNAL_CHALLENGE)
	{
		mToken = mStream.readU32();
		return CL_PrepareConnect(mStream);
	}
	return false;
}


//
// Odamex65ConnectionHandshake::handleAcceptingOfferState
//
bool Odamex65ConnectionHandshake::handleAcceptingOfferState()
{
	Net_LogPrintf(LogChan_Connection, "sending connection acceptance datagram to host %s.",
				mConnection->getRemoteAddress().getCString());

	BitStream stream;
	stream.writeU32(SIGNAL_CHALLENGE);
	stream.writeU32(mToken);			

	if (CL_TryToConnect(stream))
	{
		mConnection->sendDatagram(stream);
		return true;
	}
	return false;
}


//
// Odamex65ConnectionHandshake::handleReviewingConfirmationState
//
bool Odamex65ConnectionHandshake::handleReviewingConfirmationState()
{
	Net_LogPrintf(LogChan_Connection, "processing connection confirmation datagram from host %s.",
			mConnection->getRemoteAddress().getCString());

	if (mStream.readU32() == 0)
	{
		CL_Connect(mStream);
		return true;
	}
	return false;
}


//
// Odamex65ConnectionHandshake::handleReviewingRequestState
//
bool Odamex65ConnectionHandshake::handleReviewingRequestState()
{
	return false;
}


//
// Odamex65ConnectionHandshake::handleOfferingState
//
bool Odamex65ConnectionHandshake::handleOfferingState()
{
	return false;
}


//
// Odamex65ConnectionHandshake::handleReviewingAcceptanceState
//
bool Odamex65ConnectionHandshake::handleReviewingAcceptanceState()
{
	return false;
}


//
// Odamex65ConnectionHandshake::handleConfirmingState
//
bool Odamex65ConnectionHandshake::handleConfirmingState()
{
	return false;
}


// ============================================================================
//
// Odamex Protocol Version 100 connection handshake
//
// ============================================================================

//
// Odamex100ConnectionHandshake::handleRequestingState
//
bool Odamex100ConnectionHandshake::handleRequestingState()
{
	Net_LogPrintf(LogChan_Connection, "sending connection request datagram to host %s.",
			mConnection->getRemoteAddress().getCString());

	BitStream stream;
	stream.writeBit(Odamex100Connection::NEGOTIATION_PACKET);
	stream.writeU32(SIGNAL_CONNECTION_SEQUENCE);
	stream.writeU32(SIGNAL_GAME_CHALLENGE);
	stream.writeU8(GAMEVER);
	mConnection->sendDatagram(stream);
	return true;
}


//
// Odamex100ConnectionHandshake::handleReviewingOfferState
//
bool Odamex100ConnectionHandshake::handleReviewingOfferState()
{
	Net_LogPrintf(LogChan_Connection, "processing connection offer datagram from host %s.",
			mConnection->getRemoteAddress().getCString());
 
	mToken = mStream.readU32();

	// TODO: process the server's list of supported compression types. select
	// one and record it so it can be sent in the clientAccept packet.
	//
	// BitField<3> compression_availible;
	// compression_availible.read(stream);

	// TODO: process the server's list of supported credential types. select
	// one and record it so it can be sent in the clientAccept packet along
	// with the relevant credential info.
	//
	// BitField<3> credential_type;
	// credential_availible.write(stream);

	return true;
}


//
// Odamex100ConnectionHandshake::handleAcceptingOfferState
//
bool Odamex100ConnectionHandshake::handleAcceptingOfferState()
{
	Net_LogPrintf(LogChan_Connection, "sending connection acceptance datagram to host %s.",
				mConnection->getRemoteAddress().getCString());

	BitStream stream;
	stream.writeBit(Odamex100Connection::NEGOTIATION_PACKET);
	stream.writeU32(mToken);			
	mConnection->sendDatagram(stream);
	return true;
}


//
// Odamex100ConnectionHandshake::handleReviewingConfirmationState
//
bool Odamex100ConnectionHandshake::handleReviewingConfirmationState()
{
	Net_LogPrintf(LogChan_Connection, "processing connection confirmation datagram from host %s.",
			mConnection->getRemoteAddress().getCString());

	if (mStream.readU32() == 0)
		return true;
	return false;
}


//
// Odamex100ConnectionHandshake::handleReviewingRequestState
//
bool Odamex100ConnectionHandshake::handleReviewingRequestState()
{
	return false;
}


//
// Odamex100ConnectionHandshake::handleOfferingState
//
bool Odamex100ConnectionHandshake::handleOfferingState()
{
	return false;
}


//
// Odamex100ConnectionHandshake::handleReviewingAcceptanceState
//
bool Odamex100ConnectionHandshake::handleReviewingAcceptanceState()
{
	return false;
}


//
// Odamex100ConnectionHandshake::handleConfirmingState
//
bool Odamex100ConnectionHandshake::handleConfirmingState()
{
	return false;
}


// ============================================================================
//
// DisconnectionHandshake abstract base class implementation
//
// ============================================================================

void DisconnectionHandshake::service()
{
	if (!mSentNotification)
	{
		if (handleDisconnectingState())
			mSentNotification = true;
	}
}


// ============================================================================
//
// Odamex Protocol Version 65 disconnection handshake
//
// ============================================================================

bool Odamex65DisconnectionHandshake::handleDisconnectingState()
{
	Net_LogPrintf(LogChan_Connection, "sending disconnection notification datagram to host %s.",
			mConnection->getRemoteAddress().getCString());

	BitStream stream;

	if (mConnection->getInterface()->getHostType() == HOST_CLIENT)
	{
		// The client does not send a sequence number with Version 65 protocol.

		stream.writeU8(clc_disconnect);
	}
	else if (mConnection->getInterface()->getHostType() == HOST_SERVER)
	{
		// Only the server sends a sequence number with Version 65 protocol.
		// Fake a sequence number. Use 0xFFFFFFFF to ensure that this won't get dropped as
		// an out-of-sequence datagram.
		stream.writeU32(0xFFFFFFFF);

		stream.writeU8(svc_disconnect);
	}

	mConnection->sendDatagram(stream);
	return true;
}


// ============================================================================
//
// Odamex Protocol Version 100 disconnection handshake
//
// ============================================================================

bool Odamex100DisconnectionHandshake::handleDisconnectingState()
{
	Net_LogPrintf(LogChan_Connection, "sending disconnection notification datagram to host %s.",
			mConnection->getRemoteAddress().getCString());

	BitStream stream;
	stream.writeBit(Odamex100Connection::NEGOTIATION_PACKET);
	stream.writeU32(TERMINATION_SIGNAL);
	mConnection->sendDatagram(stream);
	return true;
}