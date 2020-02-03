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

#ifndef __NET_CONNECTION_H__
#define __NET_CONNECTION_H__

#include <vector>
#include <queue>
#include "doomdef.h"

#include "network/net_type.h"
#include "network/net_socketaddress.h"
#include "network/net_connectionmanager.h"
#include "network/net_packet.h"

class MessageManager;


// ============================================================================
//
// ConnectionStatistics class interface
//
// Contains a set of functions to calculate various Connection statistics
// useful for gauging the quality of the connection between two hosts.
//
// ============================================================================

class ConnectionStatistics
{
public:
	ConnectionStatistics();

	void clear();

	void outgoingPacketSent(const Packet::PacketSequenceNumber seq, uint16_t size);
	void outgoingPacketAcknowledged(const Packet::PacketSequenceNumber seq);
	void outgoingPacketLost(const Packet::PacketSequenceNumber seq);
	void incomingPacketReceived(const Packet::PacketSequenceNumber seq, uint16_t size);
	void incomingPacketLost(const Packet::PacketSequenceNumber seq);

	// ---------------------------------------------------------------------------
	// Accessor functions
	// ---------------------------------------------------------------------------

	uint32_t getTotalOutgoingBits() const
	{	return mOutgoingBits;	}

	uint32_t getTotalIncomingBits() const
	{	return mIncomingBits;	}

	uint32_t getTotalOutgoingPackets() const
	{	return mOutgoingPackets;	}

	uint32_t getTotalIncomingPackets() const
	{	return mIncomingPackets;	}

	uint32_t getTotalLostOutgoingPackets() const
	{	return mLostOutgoingPackets;	}

	uint32_t getTotalLostIncomingPackets() const
	{	return mLostIncomingPackets;	}

	double getRoundTripTime() const
	{	return mAvgRoundTripTime;	}

	double getJitterTime() const
	{	return mAvgJitterTime;	}

	double getOutgoingPacketLoss() const
	{	return mAvgOutgoingPacketLoss;	}

	double getIncomingPacketLoss() const
	{	return mAvgIncomingPacketLoss;	}

	double getOutgoingBitrate() const
	{	return mAvgOutgoingBitrate;	}

	double getIncomingBitrate() const
	{	return mAvgIncomingBitrate;	}

private:
	struct PacketRecord
	{
		float			mRoundTripTime;		// in seconds
		float			mJitterTime;		// in seconds
		uint16_t		mSize;				// in bits
		bool			mReceived;			// received or lost
	};

	uint32_t				mOutgoingBits;
	uint32_t				mIncomingBits;

	uint32_t				mOutgoingPackets;
	uint32_t				mIncomingPackets;

	uint32_t				mLostOutgoingPackets;
	uint32_t				mLostIncomingPackets;

	std::queue<dtime_t>		mOutgoingTimestamps;

	double					mAvgRoundTripTime;
	double					mAvgJitterTime;
	double					mAvgOutgoingPacketLoss;
	double					mAvgIncomingPacketLoss;
	double					mAvgOutgoingBitrate;
	double					mAvgIncomingBitrate;

	double					mAvgOutgoingSize;
	double					mAvgIncomingSize;

	dtime_t					mLastOutgoingTimestamp;
	dtime_t					mLastIncomingTimestamp;
};


// ============================================================================
//
// ConnectionQuality class interface
//
// Tracks various connection statistics over a time window and tries to detect
// changes in the connection quality such as network congestion.
//
// ============================================================================
class ConnectionQuality
{
public:
	ConnectionQuality()
	{
		clear();
	}

	void clear() {}

	void updateQuality(const ConnectionStatistics& stats);
	double getQuality() const;

private:
	

};


// ============================================================================
//
// Connection base class Interface
//
// Connection classes represent a coupling between two hosts on a network.
// The class is responsible for maintaining the flow of information between
// the hosts. This can mean measuring bandwidth usage, flow control,
// congestion avoidance, detecting when the link is dead (timed out) and more.
//
// ============================================================================

class Connection
{
public:
	Connection(const ConnectionId& id, NetInterface* interface, const SocketAddress& adr);
	virtual ~Connection();

	ConnectionId getConnectionId() const;
	NetInterface* getInterface() const;
	const SocketAddress& getRemoteAddress() const;
	const char* getRemoteAddressCString() const;

	virtual bool isConnected() const;
	virtual bool isNegotiating() const;
	virtual bool isTerminated() const;
	virtual bool isTimedOut() const;

	virtual void openConnection();
	virtual void closeConnection();

	virtual void service();
	virtual void sendPacket(const Packet& packet);
	virtual void processPacket(Packet& packet);

	virtual void registerMessageManager(MessageManager* manager);

private:
	enum ConnectionState_t
	{
		CONN_LISTENING					= 0x01,
		CONN_REQUESTING					= 0x02,
		CONN_OFFERING					= 0x04,
		CONN_NEGOTIATING				= 0x07,
		CONN_ACCEPTING					= 0x08,
		CONN_CONNECTED					= 0x80,
		CONN_TERMINATED					= 0xFF
	};

	ConnectionId			mConnectionId;
	NetInterface*			mInterface;
	SocketAddress			mRemoteAddress;

	ConnectionState_t		mState;
	dtime_t					mCreationTS;
	dtime_t					mTimeOutTS;

	uint32_t				mConnectionAttempt;
	dtime_t					mConnectionAttemptTimeOutTS;

	uint32_t				mToken;
	dtime_t					mTokenTimeOutTS;

	// ------------------------------------------------------------------------
	// Sequence numbers and receipt acknowledgement
	//-------------------------------------------------------------------------
	static const uint8_t ACKNOWLEDGEMENT_COUNT = 32;

	// sequence number to be sent in the next packet
	Packet::PacketSequenceNumber	mSequence;
	// most recent sequence number the remote host has acknowledged receiving
	Packet::PacketSequenceNumber	mLastAckSequence;
	// set to true during connection negotiation
	bool							mLastAckSequenceValid;

	// most recent sequence number the remote host has sent
	Packet::PacketSequenceNumber	mRecvSequence;
	// set to true during connection negotiation
	bool							mRecvSequenceValid;
	// bitfield representing all of the recently received sequence numbers
	BitField<32>					mRecvHistory;

	// ------------------------------------------------------------------------
	// Statistical tracking
	// ------------------------------------------------------------------------
	ConnectionStatistics	mConnectionStats;

	void remoteHostReceivedPacket(const Packet::PacketSequenceNumber seq);
	void remoteHostLostPacket(const Packet::PacketSequenceNumber seq);

	// ------------------------------------------------------------------------
	// Packet composition and parsing
	// ------------------------------------------------------------------------
	typedef std::vector<MessageManager*> MessageManagerList;
	MessageManagerList					mMessageManagers;

	void composePacketHeader(const Packet::PacketType& type, Packet& packet);
	void composePacketFooter(Packet& packet);
	bool validatePacketCRC(const Packet& packet) const;

	void composeGamePacket(Packet& packet);
	void parseGamePacket(Packet& packet);
	void parseNegotiationPacket(Packet& packet);	

	// ------------------------------------------------------------------------
	// Connection establishment
	// ------------------------------------------------------------------------
	Packet::PacketSequenceNumber generateRandomSequence() const;
	void resetState();

	bool clientRequest();
	bool serverProcessRequest(Packet& packet);
	bool serverOffer();
	bool clientProcessOffer(Packet& packet);
	bool clientAccept();
	bool serverProcessAcceptance(Packet& packet);
	bool serverConfirmAcceptance();
	bool clientProcessConfirmation(Packet& packet);
};

#endif	// __NET_CONNECTION_H__