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

#ifndef __NET_CONNECTION_H__
#define __NET_CONNECTION_H__

#include <vector>
#include <queue>
#include "doomdef.h"

#include "network/net_common.h"
#include "network/net_type.h"
#include "network/net_socketaddress.h"
#include "network/net_connectionmanager.h"

class MessageManager;
class Connection;


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

	void outgoingPacketSent(uint16_t size);
	void outgoingPacketAcknowledged();
	void outgoingPacketLost();
	void incomingPacketReceived(uint16_t size);
	void incomingPacketLost();

	// ---------------------------------------------------------------------------
	// Accessor functions
	// ---------------------------------------------------------------------------

	uint32_t getTotalOutgoingBits() const
	{
		return mOutgoingBits;
	}

	uint32_t getTotalIncomingBits() const
	{
		return mIncomingBits;
	}

	uint32_t getTotalOutgoingPackets() const
	{
		return mOutgoingPackets;
	}

	uint32_t getTotalIncomingPackets() const
	{
		return mIncomingPackets;
	}

	uint32_t getTotalLostOutgoingPackets() const
	{
		return mLostOutgoingPackets;
	}

	uint32_t getTotalLostIncomingPackets() const
	{
		return mLostIncomingPackets;
	}

	double getRoundTripTime() const
	{
		return mAvgRoundTripTime;
	}

	double getJitterTime() const
	{
		return mAvgJitterTime;
	}

	double getOutgoingPacketLoss() const
	{
		return mAvgOutgoingPacketLoss;
	}

	double getIncomingPacketLoss() const
	{
		return mAvgIncomingPacketLoss;
	}

	double getOutgoingBitrate() const
	{
		return mAvgOutgoingBitrate;
	}

	double getIncomingBitrate() const
	{
		return mAvgIncomingBitrate;
	}

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

};



//=============================================================================
//
// Handshake
//
// An abstract base class for finite-state machines for protocols related
// to connection establishment and teardown.
//
//=============================================================================

class Handshake
{
public:
	Handshake(Connection* connection) :
		mConnection(connection)
	{ }

	virtual ~Handshake()
	{ }

	virtual bool isConnected() const
	{
		return false;
	}

	virtual bool isFailed() const
	{
		return false;
	}

	virtual bool isNegotiating() const
	{
		return false;
	}

	virtual void service()
	{ }

	virtual void receive(BitStream& stream)
	{ }

protected:
	Connection*				mConnection;
};


//=============================================================================
//
// ConnectionHandshake
//
// A finite-state machine for establishing a connection between a client and
// a server using a handshaking protocol.
//
//=============================================================================

class ConnectionHandshake : public Handshake
{
public:
	ConnectionHandshake(Connection* connection);

	virtual ~ConnectionHandshake()
	{ }

	virtual bool isConnected() const
	{
		return mState == CONN_CONNECTED;
	}

	virtual bool isFailed() const
	{
		return mState == CONN_FAILED;
	}

	virtual bool isNegotiating() const
	{
		return mState < CONN_CONNECTED;
	}

	void service();

	void receive(BitStream& stream)
	{
		mStream = stream;
		service();
	}

private:
	static const dtime_t NEGOTIATION_TIMEOUT = 2*ONE_SECOND;

	enum HandshakeState
	{
		// Client States
		CONN_REQUESTING,
		CONN_AWAITING_OFFER,
		CONN_REVIEWING_OFFER,
		CONN_ACCEPTING_OFFER,
		CONN_AWAITING_CONFIRMATION,
		CONN_REVIEWING_CONFIRMATION,

		// Server States
		CONN_LISTENING,
		CONN_REVIEWING_REQUEST,
		CONN_OFFERING,
		CONN_AWAITING_ACCEPTANCE,
		CONN_REVIEWING_ACCEPTANCE,
		CONN_CONFIRMING,

		// Client & Server States
		CONN_CONNECTED,
		CONN_FAILED
	};

	HandshakeState			mState;
	dtime_t					mTimeOutTS;

protected:
	BitStream				mStream;

	virtual bool handleRequestingState() = 0;
	virtual bool handleReviewingOfferState() = 0;
	virtual bool handleAcceptingOfferState() = 0;
	virtual bool handleReviewingConfirmationState() = 0;

	virtual bool handleReviewingRequestState() = 0;
	virtual bool handleOfferingState() = 0;
	virtual bool handleReviewingAcceptanceState() = 0;
	virtual bool handleConfirmingState() = 0;
};


// ============================================================================
//
// Odamex Protocol Version 65 connection handshake
//
// ============================================================================

class Odamex65ConnectionHandshake : public ConnectionHandshake
{
public:
	Odamex65ConnectionHandshake(Connection* connection) :
		ConnectionHandshake(connection)
	{ }

	virtual ~Odamex65ConnectionHandshake()
	{ }

protected:
	virtual bool handleRequestingState();
	virtual bool handleReviewingOfferState();
	virtual bool handleAcceptingOfferState();
	virtual bool handleReviewingConfirmationState();

	virtual bool handleReviewingRequestState();
	virtual bool handleOfferingState();
	virtual bool handleReviewingAcceptanceState();
	virtual bool handleConfirmingState();

private:
	uint32_t				mToken;
	dtime_t					mTokenTimeOutTS;

	static const uint32_t	SIGNAL_LAUNCHER_CHALLENGE = 0x000BDBA3;
	static const uint32_t	SIGNAL_CHALLENGE = 0x0054D6D4;
};


// ============================================================================
//
// Odamex Protocol Version 100 connection handshake
//
// ============================================================================

class Odamex100ConnectionHandshake : public ConnectionHandshake
{
public:
	Odamex100ConnectionHandshake(Connection* connection) :
		ConnectionHandshake(connection)
	{ }

	virtual ~Odamex100ConnectionHandshake()
	{ }

protected:
	virtual bool handleRequestingState();
	virtual bool handleReviewingOfferState();
	virtual bool handleAcceptingOfferState();
	virtual bool handleReviewingConfirmationState();

	virtual bool handleReviewingRequestState();
	virtual bool handleOfferingState();
	virtual bool handleReviewingAcceptanceState();
	virtual bool handleConfirmingState();

private:
	uint32_t				mToken;
	dtime_t					mTokenTimeOutTS;

	static const uint32_t	SIGNAL_GAME_CHALLENGE = 0x00ABABAB;
	static const uint32_t	SIGNAL_CONNECTION_SEQUENCE = 0xFFFFFFFF;
};


//=============================================================================
//
// DisconnectionHandshake
//
// A finite-state machine for tearing a connection between a client and
// a server using a handshaking protocol.
//
//=============================================================================

class DisconnectionHandshake : public Handshake
{
public:
	DisconnectionHandshake(Connection* connection) :
		Handshake(connection),
		mSentNotification(false)
	{ }

	virtual ~DisconnectionHandshake()
	{ }

	virtual bool isConnected() const
	{
		return false;
	}

	virtual bool isFailed() const
	{
		return false;
	}

	virtual bool isNegotiating() const
	{
		return false;
	}

	void service();

	void receive(BitStream& stream)
	{
		// do nothing -- ignore all communication after sending notification of disconnection
	}

protected:
	virtual bool handleDisconnectingState() = 0;

private:
	bool					mSentNotification;
};


// ============================================================================
//
// Odamex Protocol Version 65 disconnection handshake
//
// ============================================================================

class Odamex65DisconnectionHandshake : public DisconnectionHandshake
{
public:
	Odamex65DisconnectionHandshake(Connection* connection) :
		DisconnectionHandshake(connection)
	{ }

	virtual ~Odamex65DisconnectionHandshake()
	{ }

protected:
	virtual bool handleDisconnectingState();
};


// ============================================================================
//
// Odamex Protocol Version 100 disconnection handshake
//
// ============================================================================

class Odamex100DisconnectionHandshake : public DisconnectionHandshake
{
public:
	Odamex100DisconnectionHandshake(Connection* connection) :
		DisconnectionHandshake(connection)
	{ }

	virtual ~Odamex100DisconnectionHandshake()
	{ }

protected:
	virtual bool handleDisconnectingState();

private:
	static const uint32_t	TERMINATION_SIGNAL = 0xAAAAAAAA;
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

	ConnectionId getConnectionId() const
	{
		return mConnectionId;
	}

	NetInterface* getInterface() const
	{
		return mInterface;
	}

	const SocketAddress& getRemoteAddress() const
	{
		return mRemoteAddress;
	}

	virtual bool isConnected() const;
	virtual bool isNegotiating() const;
	virtual bool isTerminated() const;
	virtual bool isTimedOut() const;

	virtual void openConnection();
	virtual void closeConnection();

	virtual void service();
	virtual void sendDatagram(const BitStream& stream);
	virtual void receiveDatagram(BitStream& stream);

	virtual void registerMessageManager(MessageManager* manager);

protected:
	ConnectionId			mConnectionId;
	NetInterface*			mInterface;
	SocketAddress			mRemoteAddress;

	bool					mTerminated;
	Handshake*				mHandshake;

	dtime_t					mCreationTS;
	dtime_t					mTimeOutTS;

	uint32_t				mConnectionAttempt;
	dtime_t					mConnectionAttemptTimeOutTS;

protected:
	virtual Handshake* createConnectionHandshake() = 0;
	virtual Handshake* createDisconnectionHandshake() = 0;

	virtual void readDatagramHeader(BitStream& stream) = 0;
	virtual void composeDatagramHeader(BitStream& stream) = 0;
	virtual void composeDatagramFooter(BitStream& stream) = 0;
	virtual bool validateDatagram(const BitStream& stream) const = 0;

	// ------------------------------------------------------------------------
	// Statistical tracking
	// ------------------------------------------------------------------------
	ConnectionStatistics	mConnectionStats;

	void remoteHostReceivedPacket(uint32_t seq);
	void remoteHostLostPacket(uint32_t seq);

	// ------------------------------------------------------------------------
	// Packet composition and parsing
	// ------------------------------------------------------------------------
	typedef std::vector<MessageManager*> MessageManagerList;
	MessageManagerList					mMessageManagers;

	void composeGameDatagram(BitStream& stream, uint32_t seq);
	void parseGameDatagram(BitStream& stream);

private:
	static const dtime_t CONNECTION_TIMEOUT = 4*ONE_SECOND;
};


// ============================================================================
//
// Odamex Protocol Version 65 connection implementation
//
// ============================================================================

class Odamex65Connection : public Connection
{
public:
	typedef SequenceNumber<32> PacketSequenceNumber;

	Odamex65Connection(const ConnectionId& id, NetInterface* interface, const SocketAddress& adr);

	virtual ~Odamex65Connection()
	{ }

protected:
	virtual Handshake* createConnectionHandshake()
	{
		return new Odamex65ConnectionHandshake(this);
	}

	virtual Handshake* createDisconnectionHandshake()
	{
		return new Odamex65DisconnectionHandshake(this);
	}

	void readDatagramHeader(BitStream& stream);
	void composeDatagramHeader(BitStream& stream);
	void composeDatagramFooter(BitStream& stream);
	bool validateDatagram(const BitStream& stream) const;

	// sequence number to be sent in the next datagram
	PacketSequenceNumber	mSequence;
	// most recent sequence number the remote host has acknowledged receiving
	PacketSequenceNumber	mLastAckSequence;
	// most recent sequence number the remote host has sent
	PacketSequenceNumber	mRecvSequence;
	// bitfield representing all of the recently received sequence numbers
	BitField<10>			mRecvHistory;

	void decompressDatagram(BitStream& stream);
};


// ============================================================================
//
// Odamex Protocol Version 100 connection
//
// ============================================================================

class Odamex100Connection : public Connection
{
public:
	typedef SequenceNumber<16> PacketSequenceNumber;

	typedef enum
	{
		GAME_PACKET				= 0,
		NEGOTIATION_PACKET		= 1
	} PacketType;

	Odamex100Connection(const ConnectionId& id, NetInterface* interface, const SocketAddress& adr);

	virtual ~Odamex100Connection()
	{ }

protected:
	virtual Handshake* createConnectionHandshake()
	{
		return new Odamex100ConnectionHandshake(this);
	}

	virtual Handshake* createDisconnectionHandshake()
	{
		return new Odamex100DisconnectionHandshake(this);
	}

	void readDatagramHeader(BitStream& stream);
	void composeDatagramHeader(BitStream& stream);
	void composeDatagramFooter(BitStream& stream);
	bool validateDatagram(const BitStream& stream) const;

	// ------------------------------------------------------------------------
	// Sequence numbers and receipt acknowledgement
	//-------------------------------------------------------------------------
	static const uint8_t ACKNOWLEDGEMENT_COUNT = 32;

	// sequence number to be sent in the next datagram
	PacketSequenceNumber	mSequence;
	// most recent sequence number the remote host has acknowledged receiving
	PacketSequenceNumber	mLastAckSequence;
	// set to true during connection negotiation
	bool					mLastAckSequenceValid;

	// most recent sequence number the remote host has sent
	PacketSequenceNumber	mRecvSequence;
	// set to true during connection negotiation
	bool					mRecvSequenceValid;
	// bitfield representing all of the recently received sequence numbers
	BitField<32>			mRecvHistory;

	void resetState();
};

#endif	// __NET_CONNECTION_H__