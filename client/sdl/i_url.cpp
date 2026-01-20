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

std::string I_ParseOdamexUrl(std::string_view url)
{
	static constexpr std::string_view protocol = "odamex://";
	std::string_view uri;
	size_t term;

	if (url.empty())
		return {};

	uri = url;
	if (uri.size() < protocol.size() || uri.compare(0, protocol.size(), protocol) != 0)
		return {};

	uri.remove_prefix(protocol.size());
	term = uri.find('/');
	if (term == std::string_view::npos)
		term = uri.size();
	if (term == 0)
		return {};

	return std::string(uri.substr(0, term));
}

char* I_ParseOdamexUrlC(const char *url)
{
	if (!url)
		return nullptr;

	std::string hostport = I_ParseOdamexUrl(url);
	if (hostport.empty())
		return nullptr;

	char *out = static_cast<char*>(std::malloc(hostport.size() + 1));
	if (!out)
		return nullptr;

	std::memcpy(out, hostport.c_str(), hostport.size() + 1);
	return out;
}
