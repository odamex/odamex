#pragma once

#include <string>

#include "i_net.h"

namespace google
{
    namespace protobuf
    {
        class Message;
    }
}

std::optional<msg_t> MSG_Pack(std::string& serializationBuffer, const google::protobuf::Message& msg);
