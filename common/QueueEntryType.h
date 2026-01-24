#pragma once

#include "i_net.h"

const size_t DEFAULT_RELIABILITY_QUEUE_SIZE = 256;

struct QueueEntryType
{
    buf_t buf;
    int   sequence;
    int   originatingTic;
    bool  isAwaiting;

    QueueEntryType() :
        buf           (MAX_UDP_PACKET),
        sequence      (-1),
        originatingTic(-1),
        isAwaiting    (false)
    {}
};
