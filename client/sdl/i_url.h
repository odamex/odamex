#pragma once

#include <stddef.h>

#ifdef __cplusplus
#include <string>
#include <string_view>

struct OdamexUrlParts
{
	// Optional credentials parsed from userinfo before '@'.
	std::string username;
	std::string password;
	// Host[:port] portion used for -connect.
	std::string hostport;
};

// Parse an odamex:// URL into username/password and host:port.
OdamexUrlParts I_ParseOdamexUrlParts(std::string_view url);
// Extract host[:port] from an odamex:// URL, or return empty on failure.
std::string I_ParseOdamexUrl(std::string_view url);

#endif
