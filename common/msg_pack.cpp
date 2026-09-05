#include "msg_pack.h"

#include "msg_map.h"

#include <google/protobuf/message.h>

std::optional<msg_t> MSG_Pack(std::string& serializationBuffer, const google::protobuf::Message& msg)
{
	if (not msg.SerializeToString(& serializationBuffer))
	{
		PrintFmt(
		    PRINT_WARNING,
		    "WARNING: Could not serialize message \"{}\".  This is most likely a bug.\n",
		    msg.GetDescriptor()->full_name());
		return std::nullopt;
	}

	const msg_t header = MSG_ResolveDescriptor(msg.GetDescriptor());
	if (header == msg_noop)
	{
		PrintFmt(PRINT_WARNING,
		         "WARNING: Could not find svc header for message \"{}\".  This is most "
		         "likely a bug.\n",
		         msg.GetDescriptor()->full_name());
		return std::nullopt;
	}

	return header;
}
