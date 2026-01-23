#pragma once

#include "i_net.h"

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
