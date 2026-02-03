#pragma once

#include "i_net.h"

#include "SequenceReceiver.h"
#include "SequenceSender.h"

dtime_t I_MSTime (void);
EXTERN_CVAR (log_packetdebug)

enum class MessageResultEnum
{
    ACCEPT,
    DEFER,
    ABORT
};

class SequencedMessenger
{
    const static size_t PACKET_SEQUENCE_INDEX      = 0;
    const static size_t PACKET_RELIABLE_SIZE_INDEX = 4;
    const static size_t PACKET_FLAG_INDEX          = 6;
    const static size_t PACKET_MESSAGE_INDEX       = 7;
    const static size_t PACKET_HEADER_SIZE         = PACKET_MESSAGE_INDEX;

    public:
        buf_t& Update(int i_currentTic)
        {
//            if
        }

        // DEFER  - Check
        // ACCEPT - unreliable data is awaiting right now.  Process it immediately or lose it forever.
        MessageResultEnum Receive(buf_t& io_rawBuf)
        {
            const int  sequence     = io_rawBuf.ReadLong();    // Packet sequence number.
            const int  reliableSize = io_rawBuf.ReadShort();   // Reliable size / Start of Unreliable data
            const byte flags        = io_rawBuf.ReadByte();    // Flag bits.
			if (flags & SVF_UNUSED_MASK)
			{
				PrintFmt(PRINT_WARNING, "Protocol flag bits ({}) were not understood", flags);
				return MessageResultEnum::ABORT;
			}
			else if (flags & SVF_COMPRESSED)
			{
				MSG_DecompressMinilzo(io_rawBuf);
			}

			m_receiveBuffer.clear();
			if (sequence >= 0)
			{
				// If this packet has both reliable and unreliable data, receive the unreliable
				// portion immediately, then truncate the packet and defer the rest for ordered
				// processing.
                const bool alsoHasNonReliableData = reliableSize < io_rawBuf.BytesLeftToRead();
				if (alsoHasNonReliableData)
				{
					const size_t startOfReliableData    = io_rawBuf.Tell();
					const size_t startOfNonReliableData = startOfReliableData + reliableSize;
					const size_t sizeOfNonReliableData  = io_rawBuf.size() - startOfNonReliableData;

					m_receiveBuffer.WriteChunk(reinterpret_cast<char*>(io_rawBuf.ptr()),
					                           sizeOfNonReliableData,
					                           startOfNonReliableData);

					const size_t sizeOfMessageWithNonReliableTruncated = startOfNonReliableData;

					io_rawBuf.setcursize(sizeOfMessageWithNonReliableTruncated);
				}

				m_receiver.RegisterReceivedPacket(sequence, io_rawBuf);

				// Send an ACK to the server only if it contained reliable data.
				if (not simulated_connection)
				{
					m_nonreliableBuffer.WriteByte(clc_ack);
					m_nonreliableBuffer.WriteLong(sequence);
				}
				return alsoHasNonReliableData ? MessageResultEnum::ACCEPT : MessageResultEnum::DEFER;
			}
			else
			{
				m_receiveBuffer.swap(io_rawBuf);
			}
			return MessageResultEnum::ACCEPT;
		}

        bool NextReceivedPacket(buf_t& io_rawBuf)
        {
            if (m_receiveBuffer.size() > 0)
            {
                io_rawBuf.swap(m_receiveBuffer);
                m_receiveBuffer.clear();
                return true;
            }
            if (SequenceQueueEntryType* queueEntryPtr = m_receiver.NextPacket())
            {
                io_rawBuf.swap(queueEntryPtr->buf);
                return true;
            }
            return false;
        }

        // Sending functions

        MessageResultEnum Send(int i_currentTic, const netadr_t& i_dest)
        {
            int bps = 0; // bytes per second, not bits per second

            m_lastSendSize = 0;

            if (m_reliableBuffer.overflowed)
            {
                SZ_Clear(&m_nonreliableBuffer);
                SZ_Clear(&m_reliableBuffer);
                //SV_DropClient(pl);
                return MessageResultEnum::ABORT;
            }
            else
                if (m_nonreliableBuffer.overflowed)
                    SZ_Clear(&m_nonreliableBuffer);

            // [SL] 2012-05-04 - Don't send empty packets - they still have overhead
            if (m_reliableBuffer.cursize + m_nonreliableBuffer.cursize == 0)
            {
                return MessageResultEnum::DEFER;
            }

            m_outgoingPacketBuffer.clear();

            if (m_reliableBuffer.cursize)
            {
                // Save the reliable portion of the message for ack checking and retransmission if necessary.
                auto saveMessage = m_sender.ObtainSendPacket(i_currentTic);

                if (saveMessage.buffer)
                {
                    // copy the reliable portion into the buffer.
                    SZ_Write(saveMessage.buffer, m_reliableBuffer.data.get(), m_reliableBuffer.cursize);

                    // Insert Reliable sequence number first thing.
                    MSG_WriteLong(&m_outgoingPacketBuffer, saveMessage.sequence);
                }
            }
            else
            {
                // Messages without reliability are non-sequenced.
                MSG_WriteLong(&m_outgoingPacketBuffer, -1);
            }

            MSG_WriteShort(&m_outgoingPacketBuffer, static_cast<short>(m_reliableBuffer.cursize));  // Reliable size
            MSG_WriteByte (&m_outgoingPacketBuffer, 0);                                             // Flags, filled out later.

            // copy the reliable message to the packet first
            if (m_reliableBuffer.cursize)
            {
                SZ_Write (&m_outgoingPacketBuffer, m_reliableBuffer.data.get(), m_reliableBuffer.cursize);
                m_reliableBps += m_reliableBuffer.cursize;
            }

            // add the unreliable part if space is available and rate value
            // allows it
            const int ticPhase = i_currentTic % TICRATE;
            if (ticPhase != 0)
            {
                bps = (int)((double)( (m_unreliableBps + m_reliableBps) * TICRATE)/(double)(i_currentTic%TICRATE));
            }

            if (bps < m_maxRate * 1000)
            {
                if (m_nonreliableBuffer.cursize && (m_outgoingPacketBuffer.maxsize() - m_outgoingPacketBuffer.cursize > m_nonreliableBuffer.cursize) )
                {
                    SZ_Write (&m_outgoingPacketBuffer, m_nonreliableBuffer.data.get(), m_nonreliableBuffer.cursize);
                    m_unreliableBps += m_nonreliableBuffer.cursize;
                }
            }

            SZ_Clear(&m_nonreliableBuffer);
            SZ_Clear(&m_reliableBuffer);

            if (m_outgoingPacketBuffer.size() > PACKET_HEADER_SIZE and not MustThrottleTransmission())
            {
                // compress the packet, but not the sequence id
                CompressPacket(m_outgoingPacketBuffer, PACKET_HEADER_SIZE);

                if (log_packetdebug)
                {
                    // FIXME: Have the player ID handy for debug messages.
                    //PrintFmt(PRINT_HIGH, "ply {:03}, size {:04}, tic {:07}, time {:011}\n",
                    //        pl.id, m_outgoingPacketBuffer.cursize, i_currentTic, I_MSTime());
                }

#ifdef SIMULATE_LATENCY
                SV_SendPacketDelayed(m_outgoingPacketBuffer, pl);
#else

                m_lastSendSize = NET_SendPacket(m_outgoingPacketBuffer, i_dest);
#endif
            }

            if (ticPhase == 0)
            {
                m_unreliableBps = 0;
                m_reliableBps   = 0;
            }

            return MessageResultEnum::ACCEPT;
        }

        int HandleRetransmissions(int i_currentTic, const netadr_t& i_dest)
        {
            auto                    iter = m_sender.IterateUnackedPackets();
            SequenceQueueEntryType* sendQueueEntry;
            int                     retransmissionsSent = 0;
            int                     bytesSent = 0;

            while ((sendQueueEntry = iter.Next()) != nullptr)
            {
                if (i_currentTic >= (m_retransmitDelayInTics + sendQueueEntry->originatingTic))
                {
                    bytesSent += SendOldPacket(*sendQueueEntry, i_dest);

                    if (++retransmissionsSent > m_sender.GetMaxPacketsPerRetransmission())
                    {
                        break;
                    }
                }
            }
            return bytesSent;
        }

        // Returns true if this is the first acknowledgement of the given sequence.  False otherwise.
        bool Acknowledge(int sequence)
        {
            return m_sender.Acknowledge(sequence);
        }

        buf_t& ReliableBuf() { return m_reliableBuffer; }
        buf_t& NetBuf() { return m_nonreliableBuffer; }

        bool MustThrottleTransmission() const { return m_sender.GetMode() == SequenceSender::RECOVERY; }

        void SetRetransmitDelay(int i_delayInTics) { m_retransmitDelayInTics = i_delayInTics; }
        void SetPacketsPerRetransmit(int i_maxPackets) { m_sender.SetMaxPacketsPerRetransmission(i_maxPackets); }
        void SetMaxRate(int i_maxRate) { m_maxRate  = i_maxRate; }

        int GetLastSendSize() const { return m_lastSendSize; }

    protected:

        static void CompressPacket(buf_t& send, const size_t reserved)
        {
#if 0
            static buf_t plain { MAX_UDP_PACKET };

            if (plain.maxsize() < send.maxsize())
            {
                plain.resize(send.maxsize());
            }

            plain.setcursize(send.size());
            memcpy(plain.ptr(), send.ptr(), send.size());
#endif
            byte method = 0;
            if (MSG_CompressMinilzo(send, reserved, 0))
            {
                // Successful compression, set the compression flag bit.
                method |= SVF_COMPRESSED;
            }

            send.ptr()[PACKET_FLAG_INDEX] |= method;
            //DPrintFmt("CompressPacket {} {}\n", method, send.size());
        }

        int SendOldPacket(const SequenceQueueEntryType& queueEntry, const netadr_t& i_dest)
        {
            m_outgoingPacketBuffer.clear();

            // This is a lot simpler than a fresh send.  Just send the data we have
            // have saved out.

            MSG_WriteLong (&m_outgoingPacketBuffer, queueEntry.sequence);
            MSG_WriteShort(&m_outgoingPacketBuffer, static_cast<short>(queueEntry.buf.cursize));
            MSG_WriteByte (&m_outgoingPacketBuffer, 0); // Flags, filled out later.

            // copy the reliable message to the packet
            if (queueEntry.buf.cursize)
            {
                SZ_Write(&m_outgoingPacketBuffer, queueEntry.buf.data.get(), queueEntry.buf.cursize);
                m_reliableBps += queueEntry.buf.cursize;
            }

            // compress the data portion of the packet
            if (m_outgoingPacketBuffer.size() > PACKET_HEADER_SIZE)
            {
                CompressPacket(m_outgoingPacketBuffer, PACKET_HEADER_SIZE);
            }

            return NET_SendPacket(m_outgoingPacketBuffer, i_dest);
        }


        SequenceSender   m_sender;
        SequenceReceiver m_receiver;

        // Send buffers
        buf_t m_reliableBuffer    { MAX_UDP_PACKET };
        buf_t m_nonreliableBuffer { MAX_UDP_PACKET };

        buf_t m_outgoingPacketBuffer { MAX_UDP_PACKET };

        // Receive buffer
        buf_t m_receiveBuffer { MAX_UDP_PACKET };

        int m_retransmitDelayInTics  { 0 };

        size_t m_unreliableBps { 0 };
        size_t m_reliableBps   { 0 };
        int    m_maxRate       { 0 };
        int    m_lastSendSize  { 0 };
};
