#pragma once

#include <cstdint>

#include "i_net.h"

struct PacketHeaderType
{
    const static size_t PACKET_SEQUENCE_INDEX      = 0;
    const static size_t PACKET_RELIABLE_SIZE_INDEX = 4;
    const static size_t PACKET_ACK_SIZE_INDEX      = 6;
    const static size_t PACKET_FLAG_INDEX          = 8;
    const static size_t PACKET_MESSAGE_INDEX       = 9;
    const static size_t PACKET_HEADER_SIZE         = PACKET_MESSAGE_INDEX;

    int32_t  sequence;
    uint16_t reliableSize;
    uint16_t ackSize;
    uint8_t  flags;

    explicit PacketHeaderType(int32_t i_sequence) :
        sequence    (i_sequence),
        reliableSize(0),
        ackSize     (0),
        flags       (0)
    {
    }

    PacketHeaderType() :
        PacketHeaderType(-1)
    {
    }

    explicit PacketHeaderType(buf_t& io_buf) :
        sequence    (io_buf.ReadLong()),
        reliableSize(io_buf.ReadShort()),
        ackSize     (io_buf.ReadShort()),
        flags       (io_buf.ReadByte())
    {
    }

    void Unpack(buf_t& io_buf)
    {
        sequence     = io_buf.ReadLong();
        reliableSize = io_buf.ReadShort();
        ackSize      = io_buf.ReadShort();
        flags        = io_buf.ReadByte();
    }

    void Pack(buf_t& io_buf)
    {
        io_buf.WriteLong (sequence);
        io_buf.WriteShort(reliableSize);
        io_buf.WriteShort(ackSize);
        io_buf.WriteByte (flags);
    }
};
