#pragma once

#include "i_net.h"

#include "SequenceReceiver.h"
#include "SequenceSender.h"

class SequencedMessenger
{
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
				//CL_QuitNetGame(NQ_PROTO);
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
				if (reliableSize < io_rawBuf.BytesLeftToRead())
				{
					const size_t startOfReliableData    = io_rawBuf.Tell();
					const size_t startOfNonReliableData = startOfReliableData + reliableSize;
					const size_t sizeOfNonReliableData  = io_rawBuf.size() - startOfNonReliableData;

					m_receiveBuffer.WriteChunk(reinterpret_cast<char*>(io_rawBuf.ptr()),
					                           sizeOfNonReliableData,
					                           startOfNonReliableData);
#if 0
			io_rawBuf.Seek(reliableSize, buf_t::BT_CURRENT);
			if (CL_AcceptNetMessage() == MessageResultEnum::ABORT)
            {
                return MessageResultEnum::ABORT;
            }
#endif
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
				return MessageResultEnum::DEFER;
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

        buf_t& ReliableBuf() { return m_reliableBuffer; }
        buf_t& NetBuf() { return m_nonreliableBuffer; }

        void SetRetransmitDelay(int i_delayInTics) { m_retransmitDelayInTics = i_delayInTics; }
        void SetPacketsPerRetransmit(int i_maxPackets) { m_maxPacketsPerRetransmit = i_maxPackets; }

    protected:
        SequenceSender   m_sender;
        SequenceReceiver m_receiver;

        // Send buffers
        buf_t m_reliableBuffer    { MAX_UDP_PACKET };
        buf_t m_nonreliableBuffer { MAX_UDP_PACKET };

        // Receive buffer
        buf_t m_receiveBuffer { MAX_UDP_PACKET };

        int m_retransmitDelayInTics  { 0 };
        int m_maxPacketsPerRetransmit{ 5 };
};
