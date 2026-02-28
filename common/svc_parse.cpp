#include "svc_parse.h"

#include "svc_map.h"
#include "svc_message.h"

parseError_e SVC_ParseMessage(google::protobuf::Message*& out, const byte cmd)
{
	// A message factory + Descriptor gives us the proper message.
	google::protobuf::MessageFactory* factory =
	    google::protobuf::MessageFactory::generated_factory();
	const google::protobuf::Descriptor* desc = SVC_ResolveHeader(static_cast<svc_t>(cmd));
	if (desc == NULL)
	{
		return PERR_UNKNOWN_HEADER;
	}

	// Can we get the mssage prototype from the descriptor?
	const google::protobuf::Message* defmsg = factory->GetPrototype(desc);
	if (defmsg == NULL)
	{
		return PERR_UNKNOWN_MESSAGE;
	}

	const size_t size   = MSG_ReadUnVarint();
	const void*  buffer = MSG_ReadChunk(size);

	// Allocated with "new" - can't be null, and we own it.
	google::protobuf::Message* msg = defmsg->New();
	if (!msg->ParseFromArray(buffer, size))
	{
		return PERR_BAD_DECODE;
	}

	out = msg;
	return PERR_OK;
}
