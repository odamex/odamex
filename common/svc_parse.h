#pragma once

#include "i_net.h"

enum parseError_e
{
	PERR_OK,
	PERR_UNKNOWN_HEADER,
	PERR_UNKNOWN_MESSAGE,
	PERR_BAD_DECODE
};

namespace google
{
	namespace protobuf
	{
		class Message;
	}
}

/**
 * @brief Given a message type, read a message from the MSG_ socket and
 *        return a decoded message in "out".
 *
 * @param out Output message - will not be modified unless successful.
 * @param cmd Command to parse out.
 * @return Error condition, or OK (0) if successful.
 */
parseError_e SVC_ParseMessage(google::protobuf::Message*& out, const byte cmd);
