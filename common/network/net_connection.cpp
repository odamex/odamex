// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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

#include "cl_main.h"

EXTERN_CVAR(net_maxrate)


// ============================================================================
//
// Connection management signals
//
// ============================================================================

//static const uint32_t LAUNCHER_CHALLENGE = 0x000BDBA3;
//static const uint32_t CHALLENGE = 0x0054D6D4;


static const uint32_t CONNECTION_SEQUENCE = 0xFFFFFFFF;
static const uint32_t TERMINATION_SEQUENCE = 0xAAAAAAAA;
static const uint32_t GAME_CHALLENGE = 0x00ABABAB;




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
void ConnectionStatistics::outgoingPacketSent(const Packet::PacketSequenceNumber seq, uint16_t size)
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
void ConnectionStatistics::outgoingPacketAcknowledged(const Packet::PacketSequenceNumber seq)
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
void ConnectionStatistics::outgoingPacketLost(const Packet::PacketSequenceNumber seq)
{
	mAvgOutgoingPacketLoss = mAvgOutgoingPacketLoss * (1.0 - AVG_WEIGHT) + AVG_WEIGHT;

	mLostOutgoingPackets++;

	mOutgoingTimestamps.pop();
}


//
// ConnectionStatistics::incomingPacketReceived
//
void ConnectionStatistics::incomingPacketReceived(const Packet::PacketSequenceNumber seq, uint16_t size)
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
void ConnectionStatistics::incomingPacketLost(const Packet::PacketSequenceNumber seq)
{
	mAvgIncomingPacketLoss = mAvgIncomingPacketLoss * (1.0 - AVG_WEIGHT) + AVG_WEIGHT;
	mLostIncomingPackets++;
}


// ============================================================================
//
// Connection class Implementation
//
// ============================================================================

static const dtime_t CONNECTION_TIMEOUT = 4*ONE_SECOND;
static const dtime_t NEGOTIATION_TIMEOUT = 2*ONE_SECOND;
static const uint32_t NEGOTIATION_ATTEMPTS = 4;
static const dtime_t TOKEN_TIMEOUT = 4*ONE_SECOND;
static const dtime_t TERMINATION_TIMEOUT = 4*ONE_SECOND;

// ----------------------------------------------------------------------------
// Public functions
// ----------------------------------------------------------------------------

Connection::Connection(const ConnectionId& connection_id, NetInterface* interface, const SocketAddress& adr) :
	mConnectionId(connection_id),
	mInterface(interface), mRemoteAddress(adr),
	mCreationTS(Net_CurrentTime()),
	mTimeOutTS(Net_CurrentTime() + CONNECTION_TIMEOUT),
	mConnectionAttempt(0), mConnectionAttemptTimeOutTS(0),
	mToken(0), mTokenTimeOutTS(0),
	mSequence(generateRandomSequence()),
	mLastAckSequenceValid(false), mRecvSequenceValid(false)
{
	resetState();
}


Connection::~Connection()
{ }


ConnectionId Connection::getConnectionId() const
{
	return mConnectionId;
}


NetInterface* Connection::getInterface() const
{
	return mInterface;
}


const SocketAddress& Connection::getRemoteAddress() const
{
	return mRemoteAddress;
}


bool Connection::isConnected() const
{
	return mState == CONN_CONNECTED;
}


bool Connection::isNegotiating() const
{
	return mState & CONN_NEGOTIATING;
}


bool Connection::isTerminated() const
{
	return mState & CONN_TERMINATED;
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
	clientRequest();
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

	Packet packet = PacketFactory::createPacket();
	BitStream& stream = packet.getPayload();
	stream.writeU32(TERMINATION_SEQUENCE);
	sendPacket(packet);

	mState = CONN_TERMINATED;
	mTimeOutTS = Net_CurrentTime() + TERMINATION_TIMEOUT;
}


//
// Connection::sendPacket
//
// Updates statistics and hands the packet off to NetInterface to send
//
void Connection::sendPacket(const Packet& packet)
{
	if (mState == CONN_TERMINATED)
		return;

	const uint16_t packet_size = BitsToBytes(packet.getSize());
	const uint8_t* data = packet.getPayload().getRawData();
	if (mInterface->socketSend(mRemoteAddress, data, packet_size))
		mConnectionStats.outgoingPacketSent(mSequence, packet.getSize());
}


//
// Connection::validatePacketCRC
//
// Validates that the packet's CRC32 is correct. This implies that the packet
// did not get corrupted during transit.
//
bool Connection::validatePacketCRC(const Packet& packet) const
{
	const BitStream& stream = packet.getPayload();

	// verify the calculated CRC32 value matches the value in the packet
	uint32_t crcvalue = Net_CRC32(stream.getRawData(), stream.bytesWritten() - sizeof(uint32_t));
	BitStream footer;
	footer.writeBlob(stream.getRawData() + stream.bytesWritten() - sizeof(uint32_t), sizeof(uint32_t));
	return crcvalue != footer.readU32();
}


//
// Connection::processPacket
//
// Reads an incoming packet's header, recording the packet sequence number so
// it can be acknowledged in the next outgoing packet. Also notifies the
// registered MessageManagers of any outgoing packet sequence numbers the
// remote host has acknowledged receiving or lost. Notification is guaranteed
// to be handled in order of sequence number.
//
// Inspects the type of an incoming packet and then hands the packet off to
// the appropriate parsing function.
//
void Connection::processPacket(Packet& packet)
{
	if (mState == CONN_TERMINATED)
		return;

	if (isNegotiating())
	{
		parseNegotiationPacket(packet);
		return;
	}

	BitStream& stream = packet.getPayload();

	if (!validatePacketCRC(packet))
	{
		// packet failed CRC32 verificiation
		Net_Warning("corrupted packet from host %s.", getRemoteAddress().getCString());
		return;
	}

	Packet::PacketType packet_type;
	Packet::PacketSequenceNumber in_seq;
	Packet::PacketSequenceNumber ack_seq;
	BitField<32> ack_history;

	// examine the packet's header
	packet_type = static_cast<Packet::PacketType>(stream.readBits(1));
	in_seq.read(stream);
	ack_seq.read(stream);
	ack_history.read(stream);

	Net_LogPrintf(LogChan_Connection, "processing %s packet from host %s, sequence %u.",
					packet_type == Packet::NEGOTIATION_PACKET ? "negotiation" : "game",
					getRemoteAddress().getCString(), in_seq.getInteger());

	if (mRecvSequenceValid)
	{
		int32_t diff = SequenceNumberDifference(in_seq, mRecvSequence);
		if (diff > 0)
		{
			// if diff > 1 there is a gap in received sequence numbers and
			// indicates incoming packets have been lost 
			for (int32_t i = 1; i < diff; i++)
				mConnectionStats.incomingPacketLost(mRecvSequence + i);

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
			Net_Warning("dropping out-of-order packet from host %s " \
					"(current sequence %u, previous sequence %u).",
					getRemoteAddress().getCString(), in_seq.getInteger(), mRecvSequence.getInteger());
			return;
		}
	}

	mConnectionStats.incomingPacketReceived(in_seq, packet.getSize());
	mRecvSequence = in_seq;

	if (mLastAckSequenceValid)
	{
		// sanity check (acknowledging a packet we haven't sent yet?!?!)
		if (ack_seq >= mSequence)
		{
			Net_Warning("remote host %s acknowledges an unknown packet.", getRemoteAddress().getCString());
			return;
		}

		// sanity check (out of order acknowledgement)
		if (ack_seq < mLastAckSequence)
		{
			Net_Warning("out-of-order packet acknowledgement from remote host %s.", getRemoteAddress().getCString());
			return;
		}

		for (Packet::PacketSequenceNumber seq = mLastAckSequence + 1; seq <= ack_seq; ++seq)
		{
			if (seq < ack_seq - ACKNOWLEDGEMENT_COUNT)
			{
				// seq is too old to fit in the ack_history window
				// consider it lost
				remoteHostLostPacket(seq);
			}
			else if (seq == ack_seq)
			{
				// seq is the most recent acknowledgement in the header
				remoteHostReceivedPacket(seq);
			}
			else
			{
				// seq's delivery status is indicated by a bit in the
				// ack_history bitfield
				int32_t bit = SequenceNumberDifference(ack_seq, seq - 1);
				if (ack_history.get(bit))
					remoteHostReceivedPacket(seq);
				else
					remoteHostLostPacket(seq);
			}
		}
	}

	mLastAckSequence = ack_seq;

	// inspect the packet type
	if (packet_type == Packet::NEGOTIATION_PACKET)
		parseNegotiationPacket(packet);
	else if (packet_type == Packet::GAME_PACKET)
		parseGamePacket(packet);
	else
		Net_Warning("unknown packet type from %s.", getRemoteAddress().getCString());

	mTimeOutTS = Net_CurrentTime() + CONNECTION_TIMEOUT;
}


//
// Connection::service
//
void Connection::service()
{
	// check if we should try to negotiate again
	if (isNegotiating() && mConnectionAttempt > 0 &&
		mConnectionAttempt <= NEGOTIATION_ATTEMPTS &&
		Net_CurrentTime() >= mConnectionAttemptTimeOutTS)
	{
		Net_Printf("Re-requesting connection to server %s...", getRemoteAddress().getCString());
		clientRequest();
	}

	// compose a new game packet and send it
	if (isConnected())
	{
		Packet packet = PacketFactory::createPacket();
		composeGamePacket(packet);
		sendPacket(packet);
	}

	Printf(PRINT_HIGH, "outgoing bitrate: %f, incoming bitrate: %f\n",
		mConnectionStats.getOutgoingBitrate(), mConnectionStats.getIncomingBitrate());
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

// ----------------------------------------------------------------------------
// Private connection negotiation functions
// ----------------------------------------------------------------------------

//
// Connection::generateRandomSequence
//
// Returns a randomized sequence number appropriate for the initial sequence
// number of a new connection.
//
Packet::PacketSequenceNumber Connection::generateRandomSequence() const
{
	return Packet::PacketSequenceNumber(rand());
}


//
// Connection::resetState
//
// Sets the state of the negotiation back to its default state. This is used
// when there is an error in the negotiation process and the negotiation needs
// to be retried.
//
void Connection::resetState()
{
	mState = CONN_LISTENING;
	mLastAckSequenceValid = false;
	mRecvSequenceValid = false;
	mRecvHistory.clear();
	mSequence = generateRandomSequence();
	mConnectionAttempt = 0;
	mConnectionStats.clear();
}


//
// Connection::clientRequest
//
// Contact the server and initiate a game connection. 
//
bool Connection::clientRequest()
{
	resetState();
	mState = CONN_REQUESTING;

	++mConnectionAttempt;
	mConnectionAttemptTimeOutTS = Net_CurrentTime() + NEGOTIATION_TIMEOUT;

	Net_LogPrintf(LogChan_Connection, "sending connection request packet to host %s.",
			getRemoteAddress().getCString());

	Packet packet = PacketFactory::createPacket();
	BitStream& stream = packet.getPayload();
	stream.writeU32(LAUNCHER_CHALLENGE);
	sendPacket(packet);
	return true;
}


//
// Connection::serverProcessRequest
//
// Handle a client's request to initiate a connection.
//
bool Connection::serverProcessRequest(Packet& packet)
{
	Net_LogPrintf(LogChan_Connection, "processing connection request packet from host %s.",
			getRemoteAddress().getCString());

	// received a valid sequence number from the client
	mRecvSequenceValid = true;

	BitStream& stream = packet.getPayload();

	if (stream.readU32() != CONNECTION_SEQUENCE)
	{
		//terminate(TERM_BADHANDSHAKE);
		return false;
	}
	if (stream.readU32() != GAME_CHALLENGE)
	{
		//terminate(TERM_BADHANDSHAKE);
		return false;
	}
	if (stream.readU8() != GAMEVER)
	{
		//terminate(TERM_VERSIONMISMATCH);
		return false;
	}

	return true;
}


//
// Connection::serverOffer
//
//
bool Connection::serverOffer()
{
	Net_LogPrintf(LogChan_Connection, "sending connection offer packet to host %s.",
			getRemoteAddress().getCString());

	Packet packet = PacketFactory::createPacket();
	composePacketHeader(Packet::NEGOTIATION_PACKET, packet);

	BitStream& stream = packet.getPayload();

	// denis - each launcher reply contains a random token so that
	// the server will only allow connections with a valid token
	// in order to protect itself from ip spoofing
	mToken = rand() * time(0);
	mTokenTimeOutTS = Net_CurrentTime() + TOKEN_TIMEOUT;
	stream.writeU32(mToken);

	// TODO: Server gives a list of supported compression types as a bitfield
	// enum {
	//		COMPR_NONE = 0,
	//		COMPR_LZO = 1,
	//		COMPR_HUFFMAN = 2
	// } CompressionType;
	//
	// BitField<3> compression_availible;
	// compression_availible.write(stream);

	// TODO: Server gives a list of supported credential types as a bitfield
	// enum {
	//		CRED_NONE = 0,
	//		CRED_PASSWORD = 1,
	//		CRED_LOCALACCT = 2,
	//		CRED_MASTERACCT = 3
	// } CredentialType;
	//
	// BitField<3> credential_type;
	// credential_availible.write(stream);

	sendPacket(packet);
	return true;
}	


//
// Connection::clientProcessOffer
//
//
bool Connection::clientProcessOffer(Packet& packet)
{
	Net_LogPrintf(LogChan_Connection, "processing connection offer packet from host %s.",
			getRemoteAddress().getCString());
 
	// have not received a valid sequence number from the server
	mRecvSequenceValid = false;
	
	BitStream& stream = packet.getPayload();

	if (stream.readU32() == CHALLENGE)
	{
		mToken = stream.readU32();	// save the token

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

		return CL_PrepareConnect(packet);
	}
	else
	{
		// TODO: Reset connection, issue warning
		return false;
	}
}


//
// Connection::clientAccept
//
//
bool Connection::clientAccept()
{
	mState = CONN_ACCEPTING;
	
	Net_LogPrintf(LogChan_Connection, "sending connection acceptance packet to host %s.",
				getRemoteAddress().getCString());

	Packet packet = PacketFactory::createPacket();
	BitStream& stream = packet.getPayload();

	stream.writeU32(CHALLENGE);
	stream.writeU32(mToken);			

	if (CL_TryToConnect(packet))
	{
		sendPacket(packet);
		return true;
	}
	else
	{
		return false;
	}
}


//
// Connection::serverProcessAcceptance
//
//
bool Connection::serverProcessAcceptance(Packet& packet)
{
	Net_LogPrintf(LogChan_Connection, "processing connection acceptance packet from host %s.",
				getRemoteAddress().getCString());

	// the client has acknowledged receiving the server's first packet
	mLastAckSequenceValid = true;

	BitStream& stream = packet.getPayload();
	
	// server verifies the token it received from the client matches
	// what it had sent earlier
	uint32_t token = stream.readU32();
	if (token != mToken || Net_CurrentTime() >= mTokenTimeOutTS)
	{
		//terminate(Connection::TERM_BADHANDSHAKE);
		return false;
	}

	// Read and verify the supplied password.
	OString password_hash = stream.readString();
	/*
	if (!getInterface()->getPassword().empty())
	{
		if (password_hash.compare(MD5SUM(getInterface()->getPassword())) != 0)
		{
			//terminate(TERM_BADPASSWORD);
			return false;
		}
	}	
	*/

	return true;
}


//
// Connection::serverConfirmAcceptance
//
//
bool Connection::serverConfirmAcceptance()
{
	Net_LogPrintf(LogChan_Connection, "sending connection confirmation packet to host %s.",
			getRemoteAddress().getCString());

	Packet packet = PacketFactory::createPacket();
	composePacketHeader(Packet::NEGOTIATION_PACKET, packet);

	// TODO: maybe tell the client its ClientId, etc

	sendPacket(packet);
	return true;
}


//
// Connection::clientProcessConfirmation
//
//
bool Connection::clientProcessConfirmation(Packet& packet)
{
	Net_LogPrintf(LogChan_Connection, "processing connection confirmation packet from host %s.",
			getRemoteAddress().getCString());

	// the server has acknowledged receiving the client's first packet
	mLastAckSequenceValid = true;

	return true;
}


//
// Connection::parseNegotiationPacket
//
// Handles incoming connection setup/tear-down data for this connection.
//
void Connection::parseNegotiationPacket(Packet& packet)
{
	BitStream& stream = packet.getPayload();

	if (stream.peekU32() == TERMINATION_SEQUENCE)
	{
		mState = CONN_TERMINATED;
		closeConnection();
	}

	if (mState == CONN_TERMINATED)
		return;

	// Once negotiation is successful, we should no longer receive
	// negotiation packets. If we do, it's an indication that the remote
	// host lost our last packet and is retrying the from the beginning.
	if (mState == CONN_CONNECTED)
		resetState();

	if (mState == CONN_ACCEPTING)
	{
		if (clientProcessConfirmation(packet))
		{
			mState = CONN_CONNECTED;
			Net_LogPrintf(LogChan_Connection, "negotiation with host %s successful.",
					getRemoteAddress().getCString());
			Net_Printf("Successfully negotiated connection with host %s.", 
					getRemoteAddress().getCString());
		}
		else
		{
			resetState();
		}
	}

	// server offering connection to client?
	if (mState == CONN_OFFERING)
	{
		if (serverProcessAcceptance(packet))
		{
			// server confirms connected status to client
			serverConfirmAcceptance();
			mState = CONN_CONNECTED;
			Net_LogPrintf(LogChan_Connection, "negotiation with host %s successful.",
					getRemoteAddress().getCString());
		}
		else
		{
			resetState();
		}
	}

	// client requesting connection to server?
	if (mState == CONN_REQUESTING)
	{
		if (clientProcessOffer(packet))
		{
			// client accepts connection to server
			clientAccept();
			mState = CONN_ACCEPTING;
		}
		else
		{
			resetState();
		}
	}

	// server listening for requests?
	if (mState == CONN_LISTENING)
	{
		if (serverProcessRequest(packet))
		{
			// server offers connection to client
			serverOffer();
			mState = CONN_OFFERING;
		}
		else
		{
			resetState();
		}
	}
}


//
// Connection::composePacketHeader
//
// Writes the header portion of a new packet. The type of the packet is
// written first, followed by the outgoing sequence number and then the
// acknowledgement of receipt for recent packets.
//
void Connection::composePacketHeader(const Packet::PacketType& type, Packet& packet)
{
	Net_LogPrintf(LogChan_Connection, "writing packet header to host %s, sequence %u.",
				getRemoteAddress().getCString(), mSequence.getInteger());


	BitStream& stream = packet.getPayload();

	stream.writeBit(type);
	mSequence.write(stream);
	mRecvSequence.write(stream);
	mRecvHistory.write(stream);

	++mSequence;
}


//
// Connection::composePacketFooter
//
void Connection::composePacketFooter(Packet& packet)
{
	BitStream& stream = packet.getPayload();

	// calculate CRC32 for the packet header & payload
	uint32_t crcvalue = Net_CRC32(stream.getRawData(), stream.bytesWritten());

	// write the calculated CRC32 value to the buffer
	stream.writeU32(crcvalue);
}


//
// Connection::composeGamePacket
//
// Writes the header for a new game packet and calls the registered
// composition callbacks to write game data to the packet.
//
void Connection::composeGamePacket(Packet& packet)
{
	Net_LogPrintf(LogChan_Connection, "composing packet to host %s.", getRemoteAddress().getCString());

	composePacketHeader(Packet::GAME_PACKET, packet);

	BitStream& stream = packet.getPayload();

	// TODO: determine packet_size using flow-control methods
	const uint32_t bandwidth = 64;	// 64kbps
	uint16_t packet_size = bandwidth * 1000 / TICRATE;
	packet_size = std::min(packet_size, stream.writeSize());

	// call the registered packet composition functions to compose the payload

	uint16_t size_left = packet_size;
	for (std::vector<MessageManager*>::iterator it = mMessageManagers.begin(); it != mMessageManagers.end(); ++it)
	{
		MessageManager* composer = *it;
		size_left -= composer->write(stream, size_left, mSequence);
	}
}

//
// Connection::parseGamePacket
//
// Handles acknowledgement of packets
//
void Connection::parseGamePacket(Packet& packet)
{
	Net_LogPrintf(LogChan_Connection, "processing packet from host %s.", getRemoteAddress().getCString());

	BitStream& stream = packet.getPayload();

	// call the registered packet parser functions to parse the payload
	uint16_t size_left = stream.readSize();
	for (std::vector<MessageManager*>::iterator it = mMessageManagers.begin(); it != mMessageManagers.end(); ++it)
	{
		MessageManager* parser = *it;
		size_left -= parser->read(stream, size_left);
	}
}


//
// Connection::remoteHostReceivedPacket
//
// Called when the Connection receives notice that the remote host received
// a packet sent by this Connection. Connection quality statistics are updated
// and the registered MessageManagers are notified of the packet's receipt.
//
void Connection::remoteHostReceivedPacket(const Packet::PacketSequenceNumber seq)
{
	Net_LogPrintf(LogChan_Connection, "remote host %s received packet %u.", 
			getRemoteAddress().getCString(), seq.getInteger());

	mConnectionStats.outgoingPacketAcknowledged(seq);

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
void Connection::remoteHostLostPacket(const Packet::PacketSequenceNumber seq)
{
	Net_LogPrintf(LogChan_Connection, "remote host %s did not receive packet %u.", 
			getRemoteAddress().getCString(), seq.getInteger());

	mConnectionStats.outgoingPacketLost(seq);

	if (isConnected())
	{
		for (MessageManagerList::iterator it = mMessageManagers.begin(); it != mMessageManagers.end(); ++it)
		{
			MessageManager* manager = *it;
			manager->notifyLost(seq);
		}
	}
}
