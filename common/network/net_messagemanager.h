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
// The MessageManager classes manage the delivery and receipt of messages to
// and from the remote host. Each MessageManger implementation provides
// certain guarantees about the delivery of a message. This guarantee becomes
// important in instances where the packet transmitting messages is lost and
// some data may need to be resent. The guarantees are as follows:
//
// TiccmdManager: TiccmdMessages (player controller input) are sent in-order
// and each TiccmdMessage is repeated in every outgoing packet until that
// particular message has been acknowledged as being received. This is cruical
// to ensure that TiccmdMessages is received as soon as possible, even
// with packet loss.
//
// ReplicationManager: Handles sending and receiving state updates for game
// entities, such as actors and moving sectors. The ReplicationManager is
// responsible for tracking which fields of an entity's state changed and the
// time the change occured. This is so that the ReplicationManager can send
// clients only the state fields that have changed since the last acknowledged
// replication update for that entity. Lost ReplicationMessages are not resent
// because state data is likely to be stale by the time it is resent. However,
// a new ReplicationMessage is created with fresh state data that includes the
// fields contained in the lost message and is sent at the scheduled interval.
//
// UrgentEventManager: UrgentEventMessages are messages that need to be
// delivered in-order and as fast as possible, even with packet loss. Similar
// to the TiccmdManager, the UrgentEventManager repeats all of its outgoing
// messages into every packet until those messages have been acknowledged
// being received. This guarantees that UrgentEventMessages will be received
// before messages of other types, which is cruicial in cases where you want
// map change messages to arrive before any other messages to avoid harmful
// side-effects of applying messages before loading the correct map.
//
// OrderedEventManager: Delivers event messages that are guaranteed to be
// received and processed in-order. Each event message is assigned a sequential
// ID number and messages are processed according to the ordering of their ID.
// A remote host can process ordered event messages until it finds a gap in the
// sequential ID numbers. At that point, it must wait until the missing event
// messages are resent and moved to the front of the processing queue.
//
// EventManager: Sends unguaranteed event messages to the remote host. A
// message is only sent once, even if the packet transmitting it is lost.
// The message is freed immediately after being written into a packet.
//
// The order in which MessageManagers are allowed to write into a packet is
// as follows:
//     UrgentEventManager
//     TiccmdManager
//     OrderedEventManager
//     EventManager
//     ReplicationManager
//
//-----------------------------------------------------------------------------

#ifndef __NET_MESSAGEMANAGER_H__
#define __NET_MESSAGEMANAGER_H__

#include <list>
#include <map>
#include <queue>

#include "network/net_type.h"
#include "network/net_connection.h"

// Stub class for Message

typedef enum
{
	NM_NoOp					= 0,		// does nothing
	NM_Replication			= 1,
	NM_Ticcmd				= 2,
	NM_LoadMap				= 10,
	NM_ClientStatus			= 11,
	NM_Chat					= 20,
	NM_Obituary				= 21,
} MessageType;

class Message 
{
public:
	const MessageType getMessageType() const
		{	return mMessageType;	}

	uint16_t size() const { return 0; }
	void clear() { }

	uint16_t read(BitStream& stream) { return 0; } 
	uint16_t write(BitStream& stream) const { return 0; }

	Message* clone() const
		{ return new Message(*this); }

private:
	MessageType				mMessageType;	
};



const uint8_t MESSAGE_ID_BITS = 8;
typedef SequenceNumber<MESSAGE_ID_BITS> MessageId;
typedef SequentialIdGenerator<MESSAGE_ID_BITS> OrderedMessageIdGenerator;


// ============================================================================
//
// MessageManager abstract base class interface
//
// MessageManagers handle the serialization of certain families of Message
// objects and implicitly define rules for the reliability of those Message
// objects. A MessageManager keeps track of which messages went into each
// packet and uses this information to resend any data it needs to when it
// receives notification that a packet was lost.
//
// The low-level networking subsystem is responsible for notifying the
// MessageManager of successful delivery or failure of a packet and
// MessageManager is responsible for deciding to resend any lost data according
// to its policy.
//
// ============================================================================

class MessageManager
{
public:
	virtual ~MessageManager() { }

	virtual void addMessage(Message* msg) = 0;
	virtual void process() = 0;

	virtual uint16_t read(BitStream& stream, uint16_t allotted_size) = 0;
	virtual uint16_t write(BitStream& stream, uint16_t allotted_size, uint32_t seq) = 0;
	virtual void notifyReceived(uint32_t seq) = 0;
	virtual void notifyLost(uint32_t seq) = 0;
};


// ============================================================================
//
// UrgentMessageManager abstract base class interface
//
// The UrgentMessageManager uses a "guaranteed fastest" algorithm for sending
// ordered Messages. This means that every time the UrgentMessageManager is
// asked to write into a packet, it will write every Message that the recipient
// has not yet acknowledged receiving. This brute-force method handles
// resending of lost Messages implicitly by assuming that every message is lost
// until its receipt has been acknowledged. As messages will be resent several
// times before acknowledgement is received, this method trades heavier
// bandwidth usage in exchange for more immediate handling of lost messages.
//
// ============================================================================

class UrgentMessageManager : public MessageManager
{
public:
	UrgentMessageManager();
	virtual ~UrgentMessageManager();

	virtual void addMessage(Message* msg);
	virtual void process();

	virtual uint16_t read(BitStream&stream, uint16_t allotted_size);
	virtual uint16_t write(BitStream& stream, uint16_t allotted_size, uint32_t seq);
	virtual void notifyReceived(uint32_t seq);
	virtual void notifyLost(uint32_t seq);

private:
	// define the maximum number of Messages that can be stored (and written
	// into every packet until acknowledged)
	static const uint8_t HISTORY_BITS = 5;
	static const uint8_t HISTORY_SIZE = 1 << HISTORY_BITS;
	static const uint8_t HISTORY_MASK = HISTORY_SIZE - 1;

	typedef std::map<MessageId, Message*> MessageTable;

	// storage for Messages to be written to a stream
	MessageTable			mOutgoingMessages;

	// save a list of MessageIds for each sent sequence number
	// indicates which Messages were written into a particular packet
	typedef std::list<MessageId> RecordList;
	RecordList				mRecords[HISTORY_SIZE];

	uint32_t				mNewestSequence;
	uint32_t				mOldestSequence;

	void deleteOutgoingMessage(const MessageId& id);
	void freeRecordList(UrgentMessageManager::RecordList* recordlist);
};


// ============================================================================
//
// TiccmdManager class interface
//
// Responsible for sending TiccmdMessages to the server, which contain a
// user's controller input for a particular gametic. The TiccmdManager
// uses the same "guaranteed fastest" delivery method as UrgentMessageManager.
//
// ============================================================================

class TiccmdManager : public UrgentMessageManager
{
};


// ============================================================================
//
// ReplicationManager class interface
//
// Responsible for replicating the state of in-game entities to clients such
// as players, projectiles, lifts, and doors.
//
// The ReplicationManager guarantees that eventually the remote host will
// receive the most-recent state of the entity. This is accomplished by
// replicating the data fields of an entity which have changed since the last
// acknowledged replication of that entity. If a replication message for an
// entity is lost, it will not be resent. Instead, the next replication
// message for the entity will include the most recent state of the fields
// that were in the lost message as well as any fields that have changed since.
//
// ============================================================================

class ReplicationManager : public MessageManager
{
public:
	ReplicationManager();
	virtual ~ReplicationManager();

	virtual void addMessage(Message* msg);
	virtual void process();

	virtual uint16_t read(BitStream&stream, uint16_t allotted_size);
	virtual uint16_t write(BitStream& stream, uint16_t allotted_size, uint32_t seq);
	virtual void notifyReceived(uint32_t seq);
	virtual void notifyLost(uint32_t seq);
};


// ============================================================================
//
// UrgentEventManager class interface
//
// Responsible for sending and receiving certain classes of event Messages
// where it is cruical that the Message be received and processed before any
// other subsequent Messages. The UrgentEventManager uses the same "guaranteed
// fastest" delivery method as UrgentMessageManager.
//
// ============================================================================

class UrgentEventManager : public UrgentMessageManager
{
};


// ============================================================================
//
// EventManager class interface
//
// Responsible for sending event messages to clients with no guarantee of
// reliability. Messages sent by EventManager are never resent.
//
// ============================================================================

class EventManager : public MessageManager
{
public:
	EventManager();
	virtual ~EventManager();

	virtual void addMessage(Message* msg);
	virtual void process();

	virtual uint16_t read(BitStream&stream, uint16_t allotted_size);
	virtual uint16_t write(BitStream& stream, uint16_t allotted_size, uint32_t seq);
	virtual void notifyReceived(uint32_t seq);
	virtual void notifyLost(uint32_t seq);

private:
	typedef std::list<Message*> MessageList;
	MessageList		mMessages;
};	
	
#endif	// __NET_MESSAGEMANAGER_H__

