// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  Odamex RCON protocol
//
//-----------------------------------------------------------------------------

// This is separate from the RCON protocol used by the client
// It's here in common for unit testing purposes
#if defined(SERVER_APP) || defined(TEST_APP)

#include "odamex.h"

#include "rconprotocol.h"

#include <regex>
#include "json/json.h"

namespace rcon
{

static bool check_extra_fields(const Json::Value& val, nonstd::span<const std::string_view> names)
{
	for (auto& name : val.getMemberNames())
	{
		if (std::none_of(names.begin(), names.end(), [&](std::string_view good) { return good == name; }))
			return false;
	}
	return true;
}

namespace messages
{

Json::Value ProtocolVersion::serialize() const
{
	return Json::Value{fmt::format("{}.{}.{}", major, minor, patch)};
}

nonstd::expected<ProtocolVersion, ParseError> ProtocolVersion::deserialize(const Json::Value& root)
{
	// [EB] I know std::regex is awful but this is a simple regex that's not
	// checked all that frequently, so it should be fine
	static const auto VERSION_REGEX = std::regex("^(\\d+)\\.(\\d+)\\.(\\d+)$");

	if (!root.isString())
		return nonstd::make_unexpected(ParseError("Protocol error: version must be a string"));

	std::string versionstr = root.asString();
	std::smatch versionmatch;
	if(!std::regex_search(versionstr, versionmatch, VERSION_REGEX))
		return nonstd::make_unexpected(ParseError(
			fmt::format("Protocol error: invalid protocol version '{}'", versionstr)
		));

	ProtocolVersion version;
	// where are you, my beloved c++23 monadic operations ;-;
	if (auto major = ParseNum<uint8_t>(versionmatch[1].str()))
		version.major = major.value();
	else
		return nonstd::make_unexpected(ParseError(fmt::format("Protocol error: major version out of range {}", versionmatch[1].str())));

	if (auto minor = ParseNum<uint8_t>(versionmatch[2].str()))
		version.minor = minor.value();
	else
		return nonstd::make_unexpected(ParseError(fmt::format("Protocol error: minor version out of range {}", versionmatch[2].str())));

	if (auto patch = ParseNum<uint8_t>(versionmatch[3].str()))
		version.patch = patch.value();
	else
		return nonstd::make_unexpected(ParseError(fmt::format("Protocol error: revision number out of range {}", versionmatch[3].str())));

	return version;
}

namespace client
{

Json::Value LoginRequest::serialize() const
{
	return version.serialize();
}

nonstd::expected<LoginRequest, ParseError> LoginRequest::deserialize(const Json::Value& root)
{
	if (auto version = ProtocolVersion::deserialize(root))
		return LoginRequest { version.value() };
	else
		return nonstd::make_unexpected(version.error());
}

Json::Value LoginPassword::serialize() const
{
	return Json::Value{password};
}

nonstd::expected<LoginPassword, ParseError> LoginPassword::deserialize(const Json::Value& root)
{
	if (!root.isString())
		return nonstd::make_unexpected(ParseError("Protocol error: password must be a string"));

	return LoginPassword { root.asString() };
}

Json::Value Command::serialize() const
{
	return Json::Value{command};
}

nonstd::expected<Command, ParseError> Command::deserialize(const Json::Value& root)
{
	if (root.isString())
		return Command { root.asString() };

	return nonstd::make_unexpected(ParseError("Protocol error: client command must be a string"));
}

Json::Value Maplist::serialize() const
{
	return Json::Value{};
}

nonstd::expected<Maplist, ParseError> Maplist::deserialize(const Json::Value& root)
{
	return nonstd::make_unexpected(ParseError("Unimplemented!!"));
}

} // namespace client

namespace server
{

Json::Value LoginResponse::serialize() const
{
	return Json::Value{nonce};
}

nonstd::expected<LoginResponse, ParseError> LoginResponse::deserialize(const Json::Value& root)
{
	if (!root.isUInt64())
		return nonstd::make_unexpected(ParseError("Protocol error: nonce must be an unsigned 64-bit integer"));

	return LoginResponse { root.asUInt64() };
}

Json::Value LoginFailure::serialize() const
{
	return Json::Value{message};
}

nonstd::expected<LoginFailure, ParseError> LoginFailure::deserialize(const Json::Value& root)
{
	if (!root.isString())
		return nonstd::make_unexpected(ParseError("Protocol error: message must be a string"));

	return LoginFailure { root.asString() };
}

Json::Value LoginSuccess::serialize() const
{
	return Json::Value{};
}

nonstd::expected<LoginSuccess, ParseError> LoginSuccess::deserialize(const Json::Value& root)
{
	if (!root.isNull())
		return nonstd::make_unexpected(ParseError("Protocol error: content must be null"));

	return LoginSuccess {};
}

Json::Value Print::serialize() const
{
	Json::Value root;
	switch (printlevel)
	{
		case PRINT_PICKUP:
			root["printlevel"] = "pickup";
			break;
		case PRINT_OBITUARY:
			root["printlevel"] = "obituary";
			break;
		case PRINT_HIGH:
			root["printlevel"] = "high";
			break;
		case PRINT_CHAT:
			root["printlevel"] = "chat";
			break;
		case PRINT_TEAMCHAT:
			root["printlevel"] = "teamchat";
			break;
		case PRINT_SERVERCHAT:
			root["printlevel"] = "serverchat";
			break;
		case PRINT_WARNING:
			root["printlevel"] = "warning";
			break;
		case PRINT_ERROR:
			root["printlevel"] = "error";
			break;
		default:
			root["printlevel"] = "unknown";
	}

	root["text"] = text;

	return root;
}

nonstd::expected<Print, ParseError> Print::deserialize(const Json::Value& root)
{
	if (!root.isObject() || !root.isMember("text") || !root.isMember("printlevel"))
		return nonstd::make_unexpected(ParseError("Protocol error: content must be an object with printlevel and text fields"));

	static constexpr std::array<std::string_view, 3> field_names = {"text", "printlevel"};
	if (!check_extra_fields(root, field_names))
		return nonstd::make_unexpected(ParseError("Protocol error: 'content' must contain only 'printlevel' and 'text'"));

	const auto& text = root["text"];
	const auto& printlevel = root["printlevel"];

	if (!text.isString())
		return nonstd::make_unexpected(ParseError("Protocol error: text must be a string"));

	if (!printlevel.isString())
		return nonstd::make_unexpected(ParseError("Protocol error: printlevel must be a string"));

	printlevel_t level;
	switch (OUtil::CONST_HASH(printlevel.asString()))
	{
		case OUtil::CONST_HASH("pickup"):
			level = PRINT_PICKUP;
			break;
		case OUtil::CONST_HASH("obituary"):
			level = PRINT_OBITUARY;
			break;
		case OUtil::CONST_HASH("high"):
			level = PRINT_HIGH;
			break;
		case OUtil::CONST_HASH("chat"):
			level = PRINT_CHAT;
			break;
		case OUtil::CONST_HASH("teamchat"):
			level = PRINT_TEAMCHAT;
			break;
		case OUtil::CONST_HASH("serverchat"):
			level = PRINT_SERVERCHAT;
			break;
		case OUtil::CONST_HASH("warning"):
			level = PRINT_WARNING;
			break;
		case OUtil::CONST_HASH("error"):
			level = PRINT_ERROR;
			break;
		default:
			return nonstd::make_unexpected(ParseError(
				fmt::format("Protocol error: invalid printlevel '{}'", printlevel.asString())
			));
	}

	return Print { level, text.asString() };
}

Json::Value Maplist::serialize() const
{
	return Json::Value{};
}

nonstd::expected<Maplist, ParseError> Maplist::deserialize(const Json::Value& root)
{
	return nonstd::make_unexpected(ParseError("Unimplemented!!"));
}

}

template <typename T, typename Enable>
std::string Message<T, Enable>::serialize(bool pretty) const
{
	Json::Value root;
	root["id"] = id;
	std::visit([&](auto&& message){
		// TODO: use the following line instead in C++20
		// root["type"] = std::remove_cvref_t<decltype(message)>::MESSAGE_TYPE;
		root["type"] = std::remove_reference_t<decltype(message)>::MESSAGE_TYPE;
		root["content"] = message.serialize();
	}, content);

	Json::StreamWriterBuilder writer;
	if (!pretty)
		writer["indentation"] = "";
	return Json::writeString(writer, root);
}

template <typename Variant, std::size_t I = 0>
static nonstd::expected<Variant, ParseError> try_deserialize_variant(
	uint32_t type,
	const Json::Value& content
)
{
	if constexpr (I >= std::variant_size_v<Variant>)
	{
		return nonstd::make_unexpected(ParseError(
			fmt::format("Protocol error: Unknown message type '{}'", type)
		));
	}
	else
	{
		using Msg = std::variant_alternative_t<I, Variant>;
		if (type == Msg::MESSAGE_TYPE_HASH)
		{
			auto result = Msg::deserialize(content);
			if (!result)
				return nonstd::make_unexpected(result.error());

			return Variant{*result};
		}

		return try_deserialize_variant<Variant, I + 1>(type, content);
	}
}


template <typename T, typename Enable>
nonstd::expected<Message<T, Enable>, ParseError>
Message<T, Enable>::deserialize(std::string_view json)
{
	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());
	if (!reader->parse(json.data(), json.data() + json.length(), &root, &errs))
	{
		return nonstd::make_unexpected(ParseError(fmt::format("JSON parse error: {}\n", errs)));
	}

	if (!root.isObject())
		return nonstd::make_unexpected(ParseError("Protocol error: root must be JSON object"));

	static constexpr std::array<std::string_view, 3> field_names = {"id", "type", "content"};
	if (!check_extra_fields(root, field_names))
		return nonstd::make_unexpected(ParseError("Protocol error: root must contain only 'id', 'type', and 'content'"));

	const Json::Value& id      = root["id"];
	const Json::Value& type    = root["type"];

	if (id.isNull())
		return nonstd::make_unexpected(ParseError("Protocol error: missing id"));

	if (!id.isUInt64())
		return nonstd::make_unexpected(ParseError("Protocol error: id must be an unsigned integer"));

	if (type.isNull())
		return nonstd::make_unexpected(ParseError("Protocol error: missing type"));

	if (!type.isString())
		return nonstd::make_unexpected(ParseError("Protocol error: type must be a string"));

	if (!root.isMember("content"))
		return nonstd::make_unexpected(ParseError("Protocol error: missing content"));

	const Json::Value& content = root["content"];

	const auto variant_result = try_deserialize_variant<T>(OUtil::CONST_HASH(type.asString()), content);
	if (!variant_result)
		return nonstd::make_unexpected(variant_result.error());

	return Message<T, Enable>{ id.asUInt64(), std::move(*variant_result) };
}

template struct Message<messages::server::Message>;
template struct Message<messages::client::Message>;

} // namespace messages

} // namespace rcon

VERSION_CONTROL (rconprotocol_cpp, "$Id$")

#endif