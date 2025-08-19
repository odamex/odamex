// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2025 by The Odamex Team.
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
//
// Source code and protocol versioning
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#ifndef ODAMEX_NO_GITVER
#include "git_describe.h"
#endif

#include <unordered_map>
#include <sstream>
#include <memory>
#include <sstream>

#include "c_dispatch.h"
#include "cmdlib.h"

/**
 * @brief Compare two "packed" versions of Odamex to see if they are expected
 *        to be protocol-compatible.
 *
 * @param server Packed version of the server.
 * @param client Packed version of the client.
 * @return 0 if they are compatible, -1 if the server is on the older verison
 *         1 if the client is on the older version.
 */
int VersionCompat(const int server, const int client)
{
	// Early-out if versions are identical.
	if (server == client)
		return 0;

	int sv_maj, sv_min, sv_pat;
	BREAKVER(server, sv_maj, sv_min, sv_pat);
	int cl_maj, cl_min, cl_pat;
	BREAKVER(client, cl_maj, cl_min, cl_pat);

	// Major version must be identical, client is allowed to have a newer
	// minor version, patch doesn't matter.  We don't need to account for
	// 0.x's version selection because it's all incompatible anyway.
	if (sv_maj == cl_maj && sv_min <= cl_min)
	{
		return 0;
	}

	// Compare all members of the patch number, even if it's redundant,
	// because eventually the condition above might change.
	else if (sv_maj < cl_maj)
	{
		return -1;
	}
	else if (sv_maj > cl_maj)
	{
		return 1;
	}
	else if (sv_min < cl_min)
	{
		return -1;
	}
	else if (sv_min > cl_min)
	{
		return 1;
	}
	else if (sv_pat < cl_pat)
	{
		return -1;
	}
	else if (sv_pat > cl_pat)
	{
		return 1;
	}

	return 0;
}

/**
 * @brief Generate a version mismatch message.
 *
 * @param server Packed version of the server.
 * @param client Packed version of the client.
 * @param email E-mail address of server host.
 * @return String message, or blank string if compatible.
 */
std::string VersionMessage(const int server, const int client, const char* email)
{
	std::string rvo;

	int cmp = VersionCompat(server, client);
	if (!cmp)
		return rvo;

	int sv_maj, sv_min, sv_pat;
	BREAKVER(server, sv_maj, sv_min, sv_pat);
	int cl_maj, cl_min, cl_pat;
	BREAKVER(client, cl_maj, cl_min, cl_pat);

	rvo += fmt::sprintf(
	    "Your version of Odamex %d.%d.%d does not match the server version %d.%d.%d.\n",
	    cl_maj, cl_min, cl_pat, sv_maj, sv_min, sv_pat);

	if (cmp > 0)
	{
		rvo += fmt::sprintf(
		          "Please visit https://odamex.net/ to obtain Odamex %d.%d.%d or "
		          "newer.\nIf you do not see this version available for download, "
		          "you are likely attempting to connect to a server running a "
		          "development version of Odamex.\n",
		          sv_maj, sv_min, sv_pat);
	}
	else
	{
		rvo += fmt::sprintf("Please allow the server admin some time to upgrade.");

		if (email != NULL)
		{
			rvo += fmt::sprintf("  If the problem persists, you can contact them at %s.\n",
			          email);
		}
		else
		{
			rvo += "\n";
		}
	}

	return rvo;
}

using source_files_t = std::unordered_map<std::string, std::string>;

source_files_t& get_source_files()
{
	static auto source_files = std::make_unique<source_files_t>();
	return *source_files.get();
}

file_version::file_version(const char *uid, const char *id, const char *pp, int l, const char *t, const char *d)
{
	std::stringstream rs(id), ss;
	std::string p = pp;

	size_t e = p.find_last_of("/\\");
	std::string file = p.substr(e == std::string::npos ? 0 : e + 1);

	ss << id << " " << l << " " << t << " " << d << " "
	   << p.substr(e == std::string::npos ? 0 : e + 1);

	get_source_files()[file] = ss.str();
}

/**
 * @brief Return true we have the bare minimum git information
 */
static bool NoGitVersion()
{
#if defined(ODAMEX_NO_GITVER) || !defined(GIT_SHORT_HASH) || !defined(GIT_REV_COUNT)
	return true;
#else
	return false;
#endif
}

/**
 * @brief Return the current commit hash.
 */
const char* GitHash()
{
#ifdef GIT_HASH
	return GIT_HASH;
#else
	return "";
#endif
}

/**
 * @brief Return the current branch name.
 */
const char* GitBranch()
{
#ifdef GIT_BRANCH
	return GIT_BRANCH;
#else
	return "";
#endif
}

/**
 * @brief Return the number of commits since the first commit.
 *
 * @detail Two branches that are the same distance from the first commit
 *         can have the same number.
 */
const char* GitRevCount()
{
#ifdef GIT_REV_COUNT
	return GIT_REV_COUNT;
#else
	return "";
#endif
}

/**
 * @brief Return a truncated unambiguous hash.
 */
const char* GitShortHash()
{
#ifdef GIT_SHORT_HASH
	return GIT_SHORT_HASH;
#else
	return "";
#endif
}

/**
 * @brief Return version details in a format that's good-enough to display
 *        in most end-user contexts.
 */
const char* NiceVersionDetails()
{
	static std::string version;
	static bool tried = false;

	if (tried)
	{
		return version.c_str();
	}
	tried = true;

	// Debug builds get a special callout.
#if !defined(_DEBUG)
	const char* debug = "";
#else
	const char* debug = ", Debug Build";
#endif

	static constexpr std::string_view RELEASE_PREFIX = "release";


	if (NoGitVersion())
	{
		// Without a git version, the only useful info we know is if
		// this is a debug build.
		if (debug[0] != '\0')
		{
			version = "Debug Build";
		}
	}
	else if (!strncmp(GitBranch(), RELEASE_PREFIX.data(), RELEASE_PREFIX.length()))
	{
		// "Release" branch shows total revisions as a build number
		version = fmt::sprintf("-prerelease.%s%s", GitRevCount(), debug);
	}
	else
	{
		// Other branches are written in.
		version = fmt::sprintf("%s, g%s-%s%s", GitBranch(), GitShortHash(), GitRevCount(),
		          debug);
	}

	return version.c_str();
}

/**
 * @brief Return a "full" version string, starting with the version number
 *        and putting appropriate details in parenthesis.
 */
const char* NiceVersion()
{
	static std::string version;
	static bool tried = false;
	static constexpr std::string_view RELEASE_PREFIX = "release";

	if (tried)
	{
		return version.c_str();
	}
	tried = true;

	// Get the version details.
	const char* details = NiceVersionDetails();
	if (details[0] == '\0')
	{
		// No version details, no parenthesis.
		version = DOTVERSIONSTR;
	}
	else
	{
		// Release candidates show everything together
		if (!strncmp(GitBranch(), RELEASE_PREFIX.data(), RELEASE_PREFIX.length()))
		{
			version = fmt::sprintf("%s%s", DOTVERSIONSTR, details);
		}
		else
		{
			// Put details in parens.
			version = fmt::sprintf("%s (%s)", DOTVERSIONSTR, details);
		}
	}

	return version.c_str();
}

BEGIN_COMMAND(version)
{
	if (argc == 1)
	{
		// distribution
		PrintFmt("Odamex v{} - {}\n", NiceVersion(), COPYRIGHTSTR);
	}
	else
	{
		// specific file version
		const auto it = get_source_files().find(argv[1]);
		if (it == get_source_files().end())
		{
			PrintFmt(PRINT_WARNING, "no such file: {}", argv[1]);
		}
		else
		{
			PrintFmt("{}", it->second);
		}
	}
}
END_COMMAND(version)

BEGIN_COMMAND(listsourcefiles)
{
	for (const auto& file : get_source_files())
	{
		PrintFmt(PRINT_HIGH, "{} {}\n", file.first, file.second);
	}
}
END_COMMAND(listsourcefiles)

VERSION_CONTROL(version_cpp, "$Id$")
