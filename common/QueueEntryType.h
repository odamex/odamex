#pragma once

#include "i_net.h"

const size_t DEFAULT_RELIABILITY_QUEUE_SIZE = 256;

struct QueueEntryType
{
    buf_t buf;
    int   sequence;
    bool  isAwaiting;

    QueueEntryType() :
        buf         (MAX_UDP_PACKET),
        sequence    (-1),
        isAwaiting  (false)
    {}
};
