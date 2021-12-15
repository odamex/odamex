// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//  A handle that wraps a resolved file on disk.
//
//-----------------------------------------------------------------------------

#pragma once

enum ofilehash_t
{
	/**
	 * @brief Hash is or wants to look for an MD5 hash.
	 */
	OFHASH_MD5,

	/**
	 * @brief Hash is or wants to look for a CRC32 checksum.
	 */
	OFHASH_CRC,
};

class OFileHash
{
	ofilehash_t m_type;
	std::string m_hash;

  public:
	OFileHash() : m_type(OFHASH_MD5), m_hash("") { }
	bool operator==(const OFileHash& other) const { return m_hash == other.m_hash; }
	bool operator!=(const OFileHash& other) const { return !(operator==(other)); }
	ofilehash_t getType() const { return m_type; }
	const std::string& getHexStr() const { return m_hash; }
	bool empty() const { return m_hash.empty(); }

	static void makeEmpty(OFileHash& out, const ofilehash_t type);
	static bool makeFromHexStr(OFileHash& out, const std::string& hash);
};

enum ofile_t
{
	/**
	 * @brief Unknown file type.
	 */
	OFILE_UNKNOWN,

	/**
	 * @brief Anything that would be passed as a -file or -iwad parameter.
	 */
	OFILE_WAD,

	/**
	 * @brief Anything that would be passed as a -deh parameter.
	 */
	OFILE_DEH,
};

/**
 * @brief A handle that wraps a resolved file on disk.
 */
class OResFile
{
	std::string m_fullpath;
	OFileHash m_hash;
	std::string m_basename;

  public:
	bool operator==(const OResFile& other) const { return m_hash == other.m_hash; }
	bool operator!=(const OResFile& other) const { return !(operator==(other)); }

	/**
	 * @brief Get the full absolute path to the file.
	 */
	const std::string& getFullpath() const { return m_fullpath; }

	/**
	 * @brief Get a unique hash of the file.
	 */
	const OFileHash& getHash() const { return m_hash; }

	/**
	 * @brief Get the base filename, with no path.
	 */
	const std::string& getBasename() const { return m_basename; }

	static bool make(OResFile& out, const std::string& file);
	static bool makeWithHash(OResFile& out, const std::string& file,
	                         const OFileHash& hash);
};
typedef std::vector<OResFile> OResFiles;

/**
 * @brief A handle that represents a "wanted" file that may or may not exist.
 */
class OWantFile
{
	std::string m_wantedpath;
	ofile_t m_wantedtype;
	OFileHash m_wantedhash;
	std::string m_basename;
	std::string m_extension;

  public:
	/**
	 * @brief Get the original "wanted" path.
	 */
	const std::string& getWantedPath() const { return m_wantedpath; }

	/**
	 * @brief Get the original "wanted" path.
	 */
	ofile_t getWantedType() const { return m_wantedtype; }

	/**
	 * @brief Get the assumed hash of the file, or an empty string if there
	 *        is no hash.
	 */
	const OFileHash& getWantedHash() const { return m_wantedhash; }

	/**
	 * @brief Get the base filename of the resource, with no directory.
	 */
	const std::string& getBasename() const { return m_basename; }

	/**
	 * @brief Get the extension of the resource.
	 */
	const std::string& getExt() const { return m_extension; }

	static bool make(OWantFile& out, const std::string& file, const ofile_t type);
	static bool makeWithHash(OWantFile& out, const std::string& file, const ofile_t type,
	                         const OFileHash& hash);
};
typedef std::vector<OWantFile> OWantFiles;

std::string M_ResFilesToString(const OResFiles& files);
const std::vector<std::string>& M_FileTypeExts(ofile_t type);
std::vector<std::string> M_FileSearchDirs();
bool M_ResolveWantedFile(OResFile& out, const OWantFile& wanted);
