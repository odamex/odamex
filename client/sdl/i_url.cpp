// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// URL parsing helpers for platform-specific launch paths.
//
//-----------------------------------------------------------------------------

#include "i_url.h"

#include <cstring>
#include <cstdlib>
#include <string_view>

OdamexUrlParts I_ParseOdamexUrlParts(std::string_view url)
{
	static constexpr std::string_view protocol = "odamex://";
	std::string_view uri;
	size_t term;
	OdamexUrlParts parts;

	if (url.empty())
		return parts;

	uri = url;
	if (uri.size() < protocol.size() || uri.compare(0, protocol.size(), protocol) != 0)
		return parts;

	uri.remove_prefix(protocol.size());
	term = uri.find('/');
	if (term == std::string_view::npos)
		term = uri.size();
	if (term == 0)
		return parts;

	uri = uri.substr(0, term);

	const size_t at = uri.find('@');
	if (at != std::string_view::npos)
	{
		if (at == 0 || at + 1 >= uri.size())
			return parts;

		parts.hostport = std::string(uri.substr(at + 1));

		const std::string_view userinfo = uri.substr(0, at);
		const size_t colon = userinfo.find(':');
		if (colon != std::string::npos)
		{
			// username:password form
			parts.username = std::string(userinfo.substr(0, colon));
			parts.password = std::string(userinfo.substr(colon + 1));
		}
		else
		{
			// password-only form
			parts.password = std::string(userinfo);
		}
	}
	else
	{
		parts.hostport = std::string(uri);
	}

	if (parts.hostport.empty())
		return {};

	return parts;
}

std::string I_ParseOdamexUrl(std::string_view url)
{
	// Only return the host:port for now.
	return I_ParseOdamexUrlParts(url).hostport;
}

static char* ToCString(std::string_view input)
{
	// Allocate a NUL-terminated C string with malloc.
	if (input.empty())
		return nullptr;

	char *out = static_cast<char*>(std::malloc(input.size() + 1));
	if (!out)
		return nullptr;

	std::memcpy(out, input.data(), input.size());
	out[input.size()] = '\0';
	return out;
}

char* I_ParseOdamexUrlC(const char *url)
{
	if (!url)
		return nullptr;

	return ToCString(I_ParseOdamexUrl(url));
}

char* I_ParseOdamexUrlUsernameC(const char *url)
{
	if (!url)
		return nullptr;

	OdamexUrlParts parts = I_ParseOdamexUrlParts(url);
	return ToCString(parts.username);
}

char* I_ParseOdamexUrlPasswordC(const char *url)
{
	if (!url)
		return nullptr;

	OdamexUrlParts parts = I_ParseOdamexUrlParts(url);
	return ToCString(parts.password);
}
