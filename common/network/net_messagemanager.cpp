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
//
//-----------------------------------------------------------------------------

#include "network/net_messagemanager.h"


// ============================================================================
//
// UrgentMessageManager class implementation
//
// ============================================================================

// ----------------------------------------------------------------------------
// Public UrgentMessageManager functions
// ----------------------------------------------------------------------------

UrgentMessageManager::UrgentMessageManager()
{ }

UrgentMessageManager::~UrgentMessageManager()
{
	// free all of the stored Messages
	for (MessageTable::const_iterator itr = mOutgoingMessages.begin(); itr != mOutgoingMessages.end(); ++itr)
	{
		Message* msg = itr->second;
		delete msg;
	}
}

//
// UrgentMessageManager::addMessage
//
// Adds a Message to the list of outgoing Messages, which will be sent with the
// next call to UrgentMessageManager::write.
//
// Note that the UrgentMessageManager takes over responsibility for freeing the
// Message object passed to this funciton.
// 
void UrgentMessageManager::addMessage(Message* msg)
{
	if (msg == NULL)
		return;

	// TODO: generate message id
	MessageId id = 0;

	mOutgoingMessages.insert(std::pair<MessageId, Message*>(id, msg));
}

//
// UrgentMessageManager::process
//
void UrgentMessageManager::process()
{

}

//
// UrgentMessageManager::read
//
uint16_t UrgentMessageManager::read(BitStream& stream, uint16_t allotted_size)
{
	uint16_t read_size = 0;
	const uint16_t message_type_id_size = 8;

	// TODO: Fix this
/*
	while (allotted_size - read_size >= message_type_id_size && isTiccmdMessage(stream))
	{
		TiccmdMessage message;
		message.read(stream);
		read_size += message.size();
		
		// process this message
		// create a new ticcmd_t based on the message data and insert it into
		// the ticcmd queue
		message.process();
	}
*/

	return read_size;
}

//
// UrgentMessageManager::write
//
// Writes any unacknowledged Messages to the stream. A list of the
// IDs of the Messages written to the stream are recorded so that
// the appropriate Messages may be freed when notification for the
// packet with sequence number seq arrives.
//
uint16_t UrgentMessageManager::write(BitStream& stream, uint16_t allotted_size, uint32_t seq)
{
	uint16_t written = 0;

	if (seq <= mNewestSequence)
		return written;

	if (stream.writeSize() < allotted_size)
		allotted_size = stream.writeSize();

	const size_t seqidx = seq & HISTORY_MASK;
	mNewestSequence = seq;

	// Is the sliding window of Messages full? It's too late to deliver
	// these old messages, however they need to be freed to prevent memory leaks.
	freeRecordList(&mRecords[seqidx]);

	// gather all unacknowledged Messages and write them to the BitStream
	for (MessageTable::iterator itr = mOutgoingMessages.begin(); itr != mOutgoingMessages.end(); ++itr)
	{
		Message* msg = itr->second;

		// is there room for this message?
		if (written + msg->size() > allotted_size)
			break;

		msg->write(stream);
		written += msg->size();

		// record that this Message was written into this packet
		mRecords[seqidx].push_back(itr->first);
	}

	return written;
}

//
// UrgentMessageManager::notifyReceived
//
// Informs the UrgentMessageManager that the packet with sequence number seq
// (and any Messages contained in it) was successfully received. All
// Messages that were received can be freed and the list of records for
// the received sequence number and all older sequence numbers can be cleared.
//
void UrgentMessageManager::notifyReceived(uint32_t seq)
{
	// Check if seq is too old and the records have already been deleted
	if (seq + HISTORY_SIZE < mNewestSequence || seq < mOldestSequence)
		return;

	// Free all Messages in the record for sequence number seq
	freeRecordList(&mRecords[seq & HISTORY_MASK]);

	// Clear out all message record lists that have been acknowledged. This includes
	// seq and all sequence numbers older than seq.
	for (uint32_t i = mOldestSequence; i <= seq; ++i)
		mRecords[i & HISTORY_MASK].clear();

	if (mOldestSequence < mNewestSequence)
		mOldestSequence = seq + 1;
}

//
// UrgentMessageManager::notifyLost
//
// Informs the UrgentMessageManager that the packet with sequence number seq
// (and any Messages contained in it) was lost. Since the UrgentMessageManager
// assumes every message is lost until it is acknowledged, this function does
// nothing.
//
void UrgentMessageManager::notifyLost(uint32_t seq)
{
	// do nothing
}

// ----------------------------------------------------------------------------
// Private UrgentMessageManager helper functions
// ----------------------------------------------------------------------------

//
// UrgentMessageManager::deleteMessage
//
// Removes a Message identified by id from the lookup table and frees
// the Message object.
//
void UrgentMessageManager::deleteOutgoingMessage(const MessageId& id)
{
	MessageTable::iterator itr = mOutgoingMessages.find(id);
	if (itr != mOutgoingMessages.end())
	{
		delete itr->second;
		mOutgoingMessages.erase(itr);
	}
}

//
// UrgentMessageManager::freeRecordList
//
// Frees all Messages that have MessageIds in recordlist and then
// clears recordlist.
//
void UrgentMessageManager::freeRecordList(UrgentMessageManager::RecordList* recordlist)
{
	for (RecordList::const_iterator itr = recordlist->begin(); itr != recordlist->end(); ++itr)
		deleteOutgoingMessage(*itr);

	recordlist->clear();
}

// ============================================================================
//
// ReplicationManager class implementation
//
// ============================================================================

// ----------------------------------------------------------------------------
// Public ReplicationManager functions
// ----------------------------------------------------------------------------

ReplicationManager::ReplicationManager()
{ }

ReplicationManager::~ReplicationManager()
{ }

void ReplicationManager::addMessage(Message* msg)
{
}

void ReplicationManager::process()
{
}


//
// ReplicationManager::read
//
//
uint16_t ReplicationManager::read(BitStream& stream, uint16_t allotted_size)
{
	return 0;
}

//
// ReplicationManager::write
//
//
uint16_t ReplicationManager::write(BitStream& stream, uint16_t allotted_size, uint32_t seq)
{
	return 0;
}


//
// ReplicationManager::notifyReceived
//
// Informs the ReplicationManager that the packet with sequence number seq
// (and any ReplicationMessages contained in it) was successfully received.
//
void ReplicationManager::notifyReceived(uint32_t seq)
{
}


//
// ReplicationManager::notifyLost
//
// Informs the ReplicationManager that the packet with sequence number seq
// (and any ReplicationMessages contained in it) was lost. Since the
// ReplicationManager never resends Messages, this function does nothing.
//
void ReplicationManager::notifyLost(uint32_t seq)
{
	// do nothing
}


// ============================================================================
//
// EventManager class implementation
//
// ============================================================================

// ----------------------------------------------------------------------------
// Public EventManager functions
// ----------------------------------------------------------------------------

EventManager::EventManager()
{ }


EventManager::~EventManager()
{
	// free all stored Message objects
	for (MessageList::const_iterator itr = mMessages.begin(); itr != mMessages.end(); ++itr)
	{
		Message* msg = *itr;
		delete msg;
	}
}

//
// EventManager::addMessage
//
// Adds a Message to the list of outgoing Messages, which will be sent with the
// next call to EventManager::write.
//
// Note that the EventManager takes over responsibility for freeing the Message
// object passed to this funciton.
// 
void EventManager::addMessage(Message* msg)
{
	mMessages.push_back(msg);
}


//
// EventManager::process
//
// Processes any recently received Messages. The EventManager processes
// Messages as soon as they are received instead of queing them so this
// function does nothing.
//
void EventManager::process()
{
	// do nothing
}

//
// EventManager::read
//
//
uint16_t EventManager::read(BitStream& stream, uint16_t allotted_size)
{
	uint16_t read_size = 0;
	const uint16_t message_type_id_size = 8;

//	while (allotted_size - read_size >= message_type_id_size && isEventMessage(stream))
	while (allotted_size - read_size >= message_type_id_size)
	{
		uint32_t msgtype = stream.readBits(message_type_id_size);

		Message* msg;
		// TODO: instantiate the correct Message type!

		msg->read(stream);
		read_size += msg->size();
		
		// process this message
		// create a new ticcmd_t based on the message data and insert it into
		// the ticcmd queue
//		msg->process();
	}

	return read_size;
}


//
// EventManager::write
//
// Writes upto allotted_size bits of Messages to the stream. Message objects
// are freed immediately after being written. Returns the number of bits
// written to the stream.
//
uint16_t EventManager::write(BitStream& stream, uint16_t allotted_size, uint32_t seq)
{
	uint16_t written = 0;

	if (stream.writeSize() < allotted_size)
		allotted_size = stream.writeSize();

	// write all Messages to the BitStream
	for (MessageList::iterator itr = mMessages.begin(); itr != mMessages.end(); ++itr)
	{
		Message* msg = *itr;

		// is there room for this message?
		if (written + msg->size() > allotted_size)
			break;

		msg->write(stream);
		written += msg->size();

		// free the Message since it will never need to be resent
		delete msg;
		mMessages.erase(itr);
	}

	return written;
}


//
// EventManager::notifyReceived
//
// Informs the EventManager that the packet with sequence number seq
// (and any EventMessages contained in it) was successfully received.
// Since Messages in EventManager are never resent, this function does nothing.
//
void EventManager::notifyReceived(uint32_t seq)
{
	// do nothing
}


//
// EventManager::notifyLost
//
// Informs the EventManager that the packet with sequence number seq
// (and any EventMessages contained in it) was lost. Since Messages in
// EventManager are never resent, this function does nothing.
//
void EventManager::notifyLost(uint32_t seq)
{
	// do nothing
}