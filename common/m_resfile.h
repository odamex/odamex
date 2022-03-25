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

#include "ohash.h"
//
//enum ofile_t
//{
//	/**
//	 * @brief Unknown file type.
//	 */
//	OFILE_UNKNOWN,
//
//	/**
//	 * @brief Anything that would be passed as a -file or -iwad parameter.
//	 */
//	OFILE_WAD,
//
//	/**
//	 * @brief Anything that would be passed as a -deh parameter.
//	 */
//	OFILE_DEH,
//};
//
///**
// * @brief A handle that wraps a resolved file on disk.
// */
//class OResFile
//{
//	std::string m_fullpath;
//	OMD5Hash m_md5;
//	std::string m_basename;
//
//  public:
//	bool operator==(const OResFile& other) const { return m_md5 == other.m_md5; }
//	bool operator!=(const OResFile& other) const { return !(operator==(other)); }
//
//	/**
//	 * @brief Get the full absolute path to the file.
//	 */
//	const std::string& getFullpath() const { return m_fullpath; }
//
//	/**
//	 * @brief Get the MD5 hash of the file.
//	 */
//	const OMD5Hash& getMD5() const { return m_md5; }
//
//	/**
//	 * @brief Get the base filename, with no path.
//	 */
//	const std::string& getBasename() const { return m_basename; }
//
//	static bool make(OResFile& out, const std::string& file);
//	static bool makeWithHash(OResFile& out, const std::string& file,
//	                         const OMD5Hash& hash);
//};
//typedef std::vector<OResFile> OResFiles;
//
///**
// * @brief A handle that represents a "wanted" file that may or may not exist.
// */
//class OWantFile
//{
//	std::string m_wantedpath;
//	ofile_t m_wantedtype;
//	OMD5Hash m_wantedMD5;
//	OCRC32Sum m_wantedCRC32;
//	std::string m_basename;
//	std::string m_extension;
//
//  public:
//	/**
//	 * @brief Get the original "wanted" path.
//	 */
//	const std::string& getWantedPath() const { return m_wantedpath; }
//
//	/**
//	 * @brief Get the original "wanted" path.
//	 */
//	ofile_t getWantedType() const { return m_wantedtype; }
//
//	/**
//	 * @brief Get the assumed hash of the file, or an empty string if there
//	 *        is no hash.
//	 */
//	const OMD5Hash& getWantedMD5() const { return m_wantedMD5; }
//
//	/**
//	 * @brief Get the base filename of the resource, with no directory.
//	 */
//	const std::string& getBasename() const { return m_basename; }
//
//	/**
//	 * @brief Get the extension of the resource.
//	 */
//	const std::string& getExt() const { return m_extension; }
//
//	static bool make(OWantFile& out, const std::string& file, const ofile_t type);
//	static bool makeWithHash(OWantFile& out, const std::string& file, const ofile_t type,
//	                         const OMD5Hash& hash);
//};
//typedef std::vector<OWantFile> OWantFiles;
//
//struct fileIdentifier_t;
//
//struct scannedIWAD_t
//{
//	std::string path;
//	const fileIdentifier_t* id;
//};
//
//std::string M_ResFilesToString(const OResFiles& files);
//const std::vector<std::string>& M_FileTypeExts(ofile_t type);
//std::vector<std::string> M_FileSearchDirs();
//bool M_ResolveWantedFile(OResFile& out, const OWantFile& wanted);
//std::vector<scannedIWAD_t> M_ScanIWADs();
