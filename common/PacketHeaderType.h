#pragma once

#include <cstdint>

#include "i_net.h"

struct PacketHeaderType
{
    const static size_t PACKET_SEQUENCE_INDEX      = 0;
    const static size_t PACKET_RELIABLE_SIZE_INDEX = 4;
    const static size_t PACKET_FLAG_INDEX          = 6;
    const static size_t PACKET_MESSAGE_INDEX       = 7;
    const static size_t PACKET_HEADER_SIZE         = PACKET_MESSAGE_INDEX;

    int32_t  sequence     {-1};
    uint16_t reliableSize { 0};
    uint8_t  flags        { 0};

    void Pack(buf_t& io_buf)
    {
        MSG_WriteLong (&io_buf, sequence);
        MSG_WriteShort(&io_buf, reliableSize);
        MSG_WriteByte (&io_buf, flags);
    }
};
