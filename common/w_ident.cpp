// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
// Resource file identification
//
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include "hashtable.h"
#include "sarray.h"
#include "m_ostring.h"

#include "w_wad.h"
#include "m_fileio.h"
#include "cmdlib.h"

#include "doomstat.h"
#include "gi.h"

#include <string>
#include <vector>
#include <stdio.h>


// ============================================================================
//
// WadFileLumpFinder
//
// Opens a WAD file and checks for the existence of specified lumps.
//
// ============================================================================

class WadFileLumpFinder
{
public:
	WadFileLumpFinder(const std::string& filename) :
		mNumLumps(0), mLumps(NULL)
	{
		FILE* fp = fopen(filename.c_str(), "rb");
		if (fp)
		{
			wadinfo_t header;
			if (fread(&header, sizeof(header), 1, fp) == 1)
			{
				header.identification = LELONG(header.identification);
				header.infotableofs = LELONG(header.infotableofs);

				if (header.identification == IWAD_ID || header.identification == PWAD_ID)
				{
					if (fseek(fp, header.infotableofs, SEEK_SET) == 0)
					{
						mNumLumps = LELONG(header.numlumps);
						mLumps = new filelump_t[mNumLumps];

						if (fread(mLumps, mNumLumps * sizeof(*mLumps), 1, fp) != 1)
							mNumLumps = 0;
					}
				}
			}

			fclose(fp);
		}
	}

	~WadFileLumpFinder()
	{
		if (mLumps)
			delete [] mLumps;
	}

	bool exists(const std::string& lumpname)
	{
		for (size_t i = 0; i < mNumLumps; i++)
			if (iequals(lumpname, std::string(mLumps[i].name, 8)))
				return true;
		return false;
	}

private:
	size_t		mNumLumps;
	filelump_t*	mLumps;
};



// ============================================================================
//
// FileIdentificationManager
//
// Class to identify known IWAD/PWAD resource files
//
// ============================================================================

class FileIdentificationManager
{
public:
	FileIdentificationManager() : mIdentifiers(64)
	{ }

	//
	// FileIdentificationManager::addFile
	//
	// Adds identification information for a known file.
	//
	void addFile(
		const OString& idname, const OString& filename,
		const OString& hash, const OString& group, bool commercial, bool iwad = true, bool deprecated = false)
	{
		IdType id = mIdentifiers.insert();
		FileIdentifier* file = &mIdentifiers.get(id);

		file->mIdName = OStringToUpper(idname);
		file->mFilename = OStringToUpper(filename);
		file->mMd5Sum = OStringToUpper(hash);
		file->mGroupName = OStringToUpper(group);
		file->mIsCommercial = commercial;
		file->mIsIWAD = iwad;
		file->mIsDeprecated = deprecated;

		mMd5SumLookup.insert(std::make_pair(OStringToUpper(file->mMd5Sum), id));

		// add the filename to the IWAD search list if it's not already in there
		if (std::find(mIWADSearchOrder.begin(), mIWADSearchOrder.end(), file->mFilename) == mIWADSearchOrder.end())
			mIWADSearchOrder.push_back(file->mFilename);
	}

	std::vector<OString> getFilenames() const
	{
		return mIWADSearchOrder;
	}

	bool isCommercial(const OString& hash) const
	{
		const FileIdentifier* file = lookupByMd5Sum(hash);
		return file && file->mIsCommercial;
	}

	bool isDeprecated(const OString& hash) const
	{
		const FileIdentifier* file = lookupByMd5Sum(hash);
		return file && file->mIsDeprecated;
	}

	bool isIWAD(const OString& filename) const
	{
		const OString md5sum = wads.GetMD5Hash(filename);
		const FileIdentifier* file = lookupByMd5Sum(md5sum);
		if (file)
			return file->mIsIWAD;

		// [SL] not an offical IWAD.
		// Check for lumps that are required by vanilla Doom.
		static const int NUM_CHECKLUMPS = 5;
		static const char checklumps[NUM_CHECKLUMPS][8] = {
			{ 'P','L','A','Y','P','A','L' },		// 0
			{ 'C','O','L','O','R','M','A','P' },	// 1
			{ 'F','_','S','T','A','R','T' },		// 2
			{ 'S','_','S','T','A','R','T' },		// 3
			{ 'T','E','X','T','U','R','E','1' }		// 4
		};

		WadFileLumpFinder lumps(filename);
		for (int i = 0; i < NUM_CHECKLUMPS; i++)
			if (!lumps.exists(std::string(checklumps[i], 8)))
				return false;
		return true;
	}

	bool areCompatible(const OString& hash1, const OString& hash2) const
	{
		const FileIdentifier* file1 = lookupByMd5Sum(hash1);
		const FileIdentifier* file2 = lookupByMd5Sum(hash2);

		if (!file1 || !file2)
			return false;

		return file1->mGroupName == file2->mGroupName;
	}

	const OString identify(const OString& filename)
	{
		const OString md5sum = wads.GetMD5Hash(filename);
		const FileIdentifier* file = lookupByMd5Sum(md5sum);

		if (file != NULL)
			return file->mIdName;

		// Not a registered file.
		// Try to identify if it's compatible with known IWADs.

		static const int NUM_CHECKLUMPS = 12;
		static const char checklumps[NUM_CHECKLUMPS][8] = {
			{ 'E','1','M','1' },					// 0
			{ 'E','2','M','1' },					// 1
			{ 'E','4','M','1' },					// 2
			{ 'M','A','P','0','1' },				// 3
			{ 'A','N','I','M','D','E','F','S' },	// 4
			{ 'F','I','N','A','L','2' },			// 5
			{ 'R','E','D','T','N','T','2' },		// 6
			{ 'C','A','M','O','1' },				// 7
			{ 'E','X','T','E','N','D','E','D' },	// 8
			{ 'D','M','E','N','U','P','I','C' },	// 9
			{ 'F','R','E','E','D','O','O','M' },	// 10
			{ 'H','A','C','X','-','R'}				// 11
		};

		bool lumpsfound[NUM_CHECKLUMPS] = { 0 };

		WadFileLumpFinder lumps(filename);
		for (int i = 0; i < NUM_CHECKLUMPS; i++)
			if (lumps.exists(std::string(checklumps[i], 8)))
				lumpsfound[i] = true;
				
		// [ML] Check for HACX 1.2
		if (lumpsfound[11])
		{
			return "HACX UNKNOWN";
		}

		// [SL] Check for FreeDoom / Ultimate FreeDoom
		if (lumpsfound[10])
		{
			if (lumpsfound[0])
				return "ULTIMATE FREEDOOM UNKNOWN";
			else
				return "FREEDOOM UNKNOWN";
		}

		// Check for Doom 2 or TNT / Plutonia
		if (lumpsfound[3])
		{
			if (lumpsfound[6])
				return "TNT EVILUTION UNKNOWN";
			if (lumpsfound[7])
				return "PLUTONIA UNKNOWN";
			if (lumpsfound[9])
				return "DOOM 2 BFG UNKNOWN";
			else
				return "DOOM 2 UNKNOWN";
		}

		// Check for Registered Doom / Ultimate Doom / Chex Quest / Shareware Doom
		if (lumpsfound[0])
		{
			if (lumpsfound[1])
			{
				if (lumpsfound[2])
				{
					// [ML] 1/7/10: HACK - There's no unique lumps in the chex quest
					// iwad.  It's ultimate doom with their stuff replacing most things.
					std::string base_filename;
					M_ExtractFileName(filename, base_filename);
					if (iequals(base_filename, "chex.wad"))
						return "CHEX QUEST UNKNOWN";
					else
					{
						if (lumpsfound[9])
							return "ULTIMATE DOOM BFG UNKNOWN";
						else
							return "ULTIMATE DOOM UNKNOWN";
					}
				}
				else
				{
					return "DOOM UNKNOWN";
				}
			}
			else
			{
				return "DOOM SHAREWARE UNKNOWN";
			}
		}

		return "UNKNOWN";
	}

	void dump() const
	{
		for (IdentifierTable::const_iterator it = mIdentifiers.begin(); it != mIdentifiers.end(); ++it)
			Printf(PRINT_HIGH, "%s %s %s\n", it->mGroupName.c_str(), it->mFilename.c_str(), it->mMd5Sum.c_str());
	}

private:
	struct FileIdentifier
	{
		OString				mIdName;
		OString				mFilename;
		OString				mMd5Sum;
		OString				mGroupName;
		bool				mIsCommercial;
		bool				mIsIWAD;
		bool				mIsDeprecated;
	};

	const FileIdentifier* lookupByMd5Sum(const OString& md5sum) const
	{
		Md5SumLookupTable::const_iterator it = mMd5SumLookup.find(OStringToUpper(md5sum));
		if (it != mMd5SumLookup.end())
			return &mIdentifiers.get(it->second);
		return NULL;
	}

	typedef unsigned int IdType;

	typedef SArray<FileIdentifier> IdentifierTable;
	IdentifierTable			mIdentifiers;

	typedef OHashTable<OString, IdType> Md5SumLookupTable;
	Md5SumLookupTable		mMd5SumLookup;

	typedef std::vector<OString> FilenameArray;
	FilenameArray			mIWADSearchOrder;
};


static FileIdentificationManager identtab;


//
// W_SetupFileIdentifiers
//
// Initializes the list of file identifiers with a set of known IWAD files.
// Based on information from http://doomwiki.org/wiki/Doom_files
//
void W_SetupFileIdentifiers()
{
	// ------------------------------------------------------------------------
	// DOOM2.WAD
	// ------------------------------------------------------------------------
	{
		identtab.addFile(
			"Doom 2 v1.9",						// mIdName
			"DOOM2.WAD",						// mFilename
			"25E1459CA71D321525F84628F45CA8CD",	// mMd5Sum
			"Doom2 v1.9",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 BFG",						// mIdName
			"DOOM2BFG.WAD",						// mFilename
			"C3BEA40570C23E511A7ED3EBCD9865F7",	// mMd5Sum
			"Doom2 v1.9",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 BFG",						// mIdName
			"BFGDOOM2.WAD",						// mFilename
			"C3BEA40570C23E511A7ED3EBCD9865F7",	// mMd5Sum
			"Doom2 v1.9",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 v1.8",						// mIdName
			"DOOM2.WAD",						// mFilename
			"C236745BB01D89BBB866C8FED81B6F8C",	// mMd5Sum
			"Doom2 v1.8",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 v1.8 French",				// mIdName
			"DOOM2F.WAD",						// mFilename
			"3CB02349B3DF649C86290907EED64E7B",	// mMd5Sum
			"Doom2 v1.8",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 v1.7a",						// mIdName
			"DOOM2.WAD",						// mFilename
			"D7A07E5D3F4625074312BC299D7ED33F",	// mMd5Sum
			"Doom2 v1.7a",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 v1.7",						// mIdName
			"DOOM2.WAD",						// mFilename
			"EA74A47A791FDEF2E9F2EA8B8A9DA13B",	// mMd5Sum
			"Doom2 v1.7",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 v1.666",					// mIdName
			"DOOM2.WAD",						// mFilename
			"30E3C2D0350B67BFBF47271970B74B2F",	// mMd5Sum
			"Doom2 v1.666",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom 2 v1.666 German",				// mIdName
			"DOOM2.WAD",						// mFilename
			"D9153CED9FD5B898B36CC5844E35B520",	// mMd5Sum
			"Doom2 v1.666",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated
	}

	// ------------------------------------------------------------------------
	// PLUTONIA.WAD
	// ------------------------------------------------------------------------

	identtab.addFile(
		"Plutonia v1.9",					// mIdName
		"PLUTONIA.WAD",						// mFilename
		"75C8CF89566741FA9D22447604053BD7",	// mMd5Sum
		"Plutonia v1.9",					// mGroupName
		true,								// mIsCommercial
		true,								// mIsIWAD
		false);								// mIsDeprecated


	// ------------------------------------------------------------------------
	// TNT.WAD
	// ------------------------------------------------------------------------

	identtab.addFile(
		"TNT Evilution v1.9",				// mIdName
		"TNT.WAD",							// mFilename
		"4E158D9953C79CCF97BD0663244CC6B6",	// mMd5Sum
		"TNT Evilution v1.9",				// mGroupName
		true,								// mIsCommercial
		true,								// mIsIWAD
		false);								// mIsDeprecated


	// ------------------------------------------------------------------------
	// DOOM.WAD
	// ------------------------------------------------------------------------
	{
		identtab.addFile(
			"Ultimate Doom v1.9",				// mIdName
			"DOOMU.WAD",						// mFilename
			"C4FE9FD920207691A9F493668E0A2083",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Doom v1.9",				// mIdName
			"DOOM.WAD",							// mFilename
			"C4FE9FD920207691A9F493668E0A2083",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Doom v1.9",						// mIdName
			"DOOM.WAD",							// mFilename
			"1CD63C5DDFF1BF8CE844237F580E9CF3",	// mMd5Sum
			"Doom v1.9",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Doom BFG",				// mIdName
			"DOOMBFG.WAD",						// mFilename
			"FB35C4A5A9FD49EC29AB6E900572C524",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Doom BFG",				// mIdName
			"BFGDOOM.WAD",						// mFilename
			"FB35C4A5A9FD49EC29AB6E900572C524",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Doom v1.8",						// mIdName
			"DOOM.WAD",							// mFilename
			"11E1CD216801EA2657723ABC86ECB01F",	// mMd5Sum
			"Doom v1.8",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom v1.666",						// mIdName
			"DOOM.WAD",							// mFilename
			"54978D12DE87F162B9BCC011676CB3C0",	// mMd5Sum
			"Doom v1.666",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom v1.2",						// mIdName
			"DOOM.WAD",							// mFilename
			"792FD1FEA023D61210857089A7C1E351",	// mMd5Sum
			"Doom v1.2",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom v1.1",						// mIdName
			"DOOM.WAD",							// mFilename
			"981B03E6D1DC033301AA3095ACC437CE",	// mMd5Sum
			"Doom v1.1",						// mGroupName
			true,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated
	}

	// ------------------------------------------------------------------------
	// DOOM1.WAD
	// ------------------------------------------------------------------------
	{
		identtab.addFile(
			"Doom Shareware v1.9",				// mIdName
			"DOOM1.WAD",						// mFilename
			"F0CEFCA49926D00903CF57551D901ABE",	// mMd5Sum
			"Doom Shareware v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Doom Shareware v1.8",				// mIdName
			"DOOM1.WAD",						// mFilename
			"5F4EB849B1AF12887DEC04A2A12E5E62",	// mMd5Sum
			"Doom Shareware v1.8",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom Shareware v1.6",				// mIdName
			"DOOM1.WAD",						// mFilename
			"762FD6D4B960D4B759730F01387A50A1",	// mMd5Sum
			"Doom Shareware v1.6",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom Shareware v1.5",				// mIdName
			"DOOM1.WAD",						// mFilename
			"E280233D533DCC28C1ACD6CCDC7742D4",	// mMd5Sum
			"Doom Shareware v1.5",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom Shareware v1.4",				// mIdName
			"DOOM1.WAD",						// mFilename
			"A21AE40C388CB6F2C3CC1B95589EE693",	// mMd5Sum
			"Doom Shareware v1.4",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom Shareware v1.2",				// mIdName
			"DOOM1.WAD",						// mFilename
			"30AA5BEB9E5EBFBBE1E1765561C08F38",	// mMd5Sum
			"Doom Shareware v1.2",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom Shareware v1.1",				// mIdName
			"DOOM1.WAD",						// mFilename
			"52CBC8882F445573CE421FA5453513C1",	// mMd5Sum
			"Doom Shareware v1.1",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated

		identtab.addFile(
			"Doom Shareware v1.0",				// mIdName
			"DOOM1.WAD",						// mFilename
			"90FACAB21EEDE7981BE10790E3F82DA2",	// mMd5Sum
			"Doom Shareware v1.0",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			true);								// mIsDeprecated
	}

	// ------------------------------------------------------------------------
	// FREEDOOM1.WAD
	// ------------------------------------------------------------------------
	{
		identtab.addFile(
			"Ultimate Freedoom v0.11.3",		// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"EA471A3D38FCEE0FB3A69BCD3221E335",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Freedoom v0.11.2",		// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"6D00C49520BE26F08A6BD001814A32AB",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated		

		identtab.addFile(
			"Ultimate Freedoom v0.11.1",		// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"35312E99D2473297AABE0602700BEE8A",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated		
		
		identtab.addFile(
			"Ultimate Freedoom v0.11",			// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"21A4707FC25D29EDF4B098BD400C5C42",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Freedoom v0.10.1",		// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"91DE79621A393A08C39A9AB2C034B766",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Freedoom v0.10",		// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"9B8D72B59FD93B2B3E116149BAA1B142",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Freedoom v0.9",			// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"ACA90CF5AC36E996EDC58BD0329B979A",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Ultimate Freedoom v0.8",			// mIdName
			"FREEDOOM1.WAD",					// mFilename
			"30095B256DD3A1566BBC30286F72BC47",	// mMd5Sum
			"Ultimate Doom v1.9",				// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated
	}

	// ------------------------------------------------------------------------
	// FREEDOOM2.WAD
	// ------------------------------------------------------------------------
	{
		identtab.addFile(
			"Freedoom v0.11.3",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"984F99AF08F085E38070F51095AB7C31",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated


		identtab.addFile(
			"Freedoom v0.11.2",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"90832A872B5BB0ACA4CA0B20419AAD5D",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Freedoom v0.11.1",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"EC5B38B30BA2B70E278205776AF3FBB5",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Freedoom v0.11",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"B1018017C61B06E33C11102D8BAFAAD0",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated


		identtab.addFile(
			"Freedoom v0.10.1",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"DD9C9E73F5F50D3778C85573CD08D9A4",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Freedoom v0.10",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"C5A4F2D38D78B251D8557CB2D93E40EE",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Freedoom v0.9",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"8FA57DBC7687F84528EBA39DDE3A20E0",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"Freedoom v0.8",					// mIdName
			"FREEDOOM2.WAD",					// mFilename
			"E3668912FC37C479B2840516C887018B",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated
	}

	// ------------------------------------------------------------------------
	// FREEDM.WAD
	// ------------------------------------------------------------------------
	{
		identtab.addFile(
			"FreeDM v0.11.3",					// mIdName
			"FREEDM.WAD",						// mFilename
			"87EE2494D921633420CE9BDB418127C4",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"FreeDM v0.11.2",					// mIdName
			"FREEDM.WAD",						// mFilename
			"9352B09AE878DC52C6C18AA38ACDA6EB",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"FreeDM v0.11.1",					// mIdName
			"FREEDM.WAD",						// mFilename
			"77BA9C0F75C32E4A729490688BB99241",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"FreeDM v0.11",						// mIdName
			"FREEDM.WAD",						// mFilename
			"D76D3973C075B069ECB4E16DC9EACBB4",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"FreeDM v0.10.1",					// mIdName
			"FREEDM.WAD",						// mFilename
			"BD4F359F1963E388BEDA014C5548B420",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"FreeDM v0.10",						// mIdName
			"FREEDM.WAD",						// mFilename
			"F37B8B70E1394289A7EC404F67CDEC1A",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		//--------------------------------

		identtab.addFile(
			"FreeDM v0.9",						// mIdName
			"FREEDM.WAD",						// mFilename
			"CBB27C5F3C2C44D34843CF63DAA627F6",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated

		identtab.addFile(
			"FreeDM v0.8",						// mIdName
			"FREEDM.WAD",						// mFilename
			"05859098BF191899903EF343AFBA369D",	// mMd5Sum
			"Doom 2 v1.9",						// mGroupName
			false,								// mIsCommercial
			true,								// mIsIWAD
			false);								// mIsDeprecated
	}

	// ------------------------------------------------------------------------
	// CHEX.WAD
	// ------------------------------------------------------------------------

	identtab.addFile(
		"Chex Quest",						// mIdName
		"CHEX.WAD",							// mFilename
		"25485721882B050AFA96A56E5758DD52",	// mMd5Sum
		"Chex Quest",						// mGroupName
		true,								// mIsCommercial
		true,								// mIsIWAD
		false);								// mIsDeprecated


	// ------------------------------------------------------------------------
	// HACX.WAD
	// ------------------------------------------------------------------------

	identtab.addFile(
		"HACX 1.2",						// mIdName
		"HACX.WAD",							// mFilename
		"65ED74D522BDF6649C2831B13B9E02B4",	// mMd5Sum
		"Doom 2 v1.9",						// mGroupName
		true,								// mIsCommercial
		true,								// mIsIWAD
		false);								// mIsDeprecated

}


//
// W_ConfigureGameInfo
//
// Queries the FileIdentityManager to identify the given IWAD file based
// on its MD5Sum. The appropriate values are then set for the globals
// gameinfo, gamemode, and gamemission.
//
// gamemode will be set to undetermined if the file is not a valid IWAD.
//
//
void W_ConfigureGameInfo(const std::string& iwad_filename)
{
	extern gameinfo_t SharewareGameInfo;
	extern gameinfo_t RegisteredGameInfo;
	extern gameinfo_t RetailGameInfo;
	extern gameinfo_t CommercialGameInfo;
	extern gameinfo_t RetailBFGGameInfo;
	extern gameinfo_t CommercialBFGGameInfo;

	const OString idname = identtab.identify(iwad_filename);


	if (idname.find("HACX") == 0)
	{
		gameinfo = CommercialGameInfo;
		gamemode = commercial;
		gamemission = commercial_hacx;
	}
	else if (idname.find("PLUTONIA") == 0)
	{
		gameinfo = CommercialGameInfo;
		gamemode = commercial;
		gamemission = pack_plut;
	}
	else if (idname.find("TNT EVILUTION") == 0)
	{
		gameinfo = CommercialGameInfo;
		gamemode = commercial;
		gamemission = pack_tnt;
	}
	else if (idname.find("CHEX QUEST") == 0)
	{
		gamemission = chex;
		gamemode = retail_chex;
		gameinfo = RetailGameInfo;
	}
	else if (idname.find("ULTIMATE FREEDOOM") == 0)
	{
		gamemode = retail;
		gameinfo = RetailGameInfo;
		gamemission = retail_freedoom;
	}
	else if (idname.find("FREEDOOM") == 0)
	{
		gamemode = commercial;
		gameinfo = CommercialGameInfo;
		gamemission = commercial_freedoom;
	}
	else if (idname.find("FREEDOOM2") == 0)
	{
		gamemode = commercial;
		gameinfo = CommercialGameInfo;
		gamemission = commercial_freedoom;
	}

	else if (idname.find("FREEDM") == 0)
	{
		gamemode = commercial;
		gameinfo = CommercialGameInfo;
		gamemission = commercial_freedoom;
	}	
	else if (idname.find("DOOM SHAREWARE") == 0)
	{
		gamemode = shareware;
		gameinfo = SharewareGameInfo;
		gamemission = doom;
	}
	else if (idname.find("ULTIMATE DOOM BFG") == 0)
	{
		gamemode = retail_bfg;
		gameinfo = RetailBFGGameInfo;
		gamemission = doom;
	}
	else if (idname.find("ULTIMATE DOOM") == 0)
	{
		gamemode = retail;
		gameinfo = RetailGameInfo;
		gamemission = doom;
	}
	else if (idname.find("DOOM 2 BFG") == 0)
	{
		gameinfo = CommercialBFGGameInfo;
		gamemode = commercial_bfg;
		gamemission = doom2;
	}
	else if (idname.find("DOOM 2") == 0)
	{
		gameinfo = CommercialGameInfo;
		gamemode = commercial;
		gamemission = doom2;
	}
	else if (idname.find("DOOM") == 0)
	{
		gamemode = registered;
		gameinfo = RegisteredGameInfo;
		gamemission = doom;
	}
	else
	{
		gamemode = undetermined;
		gameinfo = SharewareGameInfo;
		gamemission = doom;
	}
}


//
// W_IsIWAD
//
// Returns true if the given file is a known IWAD file.
//
bool W_IsIWAD(const std::string& filename)
{
	return identtab.isIWAD(filename);
}


//
// W_IsIWADCommercial
//
// Checks to see whether a given file is an IWAD flagged as "commercial"
//
bool W_IsIWADCommercial(const std::string& filename)
{
	const OString md5sum = wads.GetMD5Hash(filename);
	return identtab.isCommercial(md5sum);
}


//
// W_IsIWADDeprecated
//
// Checks to see whether a given file is an IWAD flagged as "deprecated"
//
bool W_IsIWADDeprecated(const std::string& filename)
{
	const OString md5sum = wads.GetMD5Hash(filename);
	return identtab.isDeprecated(md5sum);
}


std::vector<OString> W_GetIWADFilenames()
{
	return identtab.getFilenames();
}

VERSION_CONTROL (w_ident_cpp, "$Id$")
