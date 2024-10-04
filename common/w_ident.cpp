// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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


#include "odamex.h"

#include "hashtable.h"
#include "sarray.h"
#include "m_ostring.h"

#include "w_wad.h"
#include "m_fileio.h"
#include "cmdlib.h"

#include "gi.h"

#include "w_ident.h"

//static const uint32_t IDENT_NONE = 0; // unused
static const uint32_t IDENT_COMMERCIAL = BIT(0);
static const uint32_t IDENT_IWAD = BIT(1);
static const uint32_t IDENT_DEPRECATED = BIT(2);

struct identData_t
{
	const char* idName;
	const char* filename;
	const char* crc32Sum;
	const char* md5Sum;
	const char* groupName;
	uint32_t flags;
	int weight;
};

#define DOOMSW_PREFIX "DOOM Shareware"
#define DOOM_PREFIX "DOOM"
#define UDOOM_PREFIX "The Ultimate DOOM"
#define DOOM2_PREFIX "DOOM II: Hell on Earth"
#define PLUTONIA_PREFIX "The Plutonia Experiment"
#define TNT_PREFIX "TNT: Evilution"
#define FREEDOOM1_PREFIX "Freedoom: Phase 1"
#define FREEDOOM2_PREFIX "Freedoom: Phase 2"
#define FREEDM_PREFIX "FreeDM"
#define NERVE_PREFIX "No Rest for the Living"
#define MASTERLV_PREFIX "Master Levels"

#define PWAD_NO_WEIGHT 0

static const identData_t identdata[] = {
    // ------------------------------------------------------------------------
    // DOOM2.WAD
    // ------------------------------------------------------------------------
    {
        DOOM2_PREFIX " v1.9",               // idName
        "DOOM2.WAD",                        // filename
        "EC8725DB",                         // crc32Sum
        "25E1459CA71D321525F84628F45CA8CD", // md5Sum
        DOOM2_PREFIX " v1.9",               // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        100,                                // weight
    },
    {
        // version from October 3, 2024
        DOOM2_PREFIX " (DOOM + DOOM II)",   // idName
        "DOOM2.WAD",                        // filename
        "151B8A96",                         // crc32Sum
        "64A4C88A871DA67492AAA2020A068CD8", // md5Sum
        DOOM2_PREFIX " v1.9",               // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        140,                                // weight
    },
    {
        // version on release
        DOOM2_PREFIX " (DOOM + DOOM II)",   // idName
        "DOOM2.WAD",                        // filename
        "09B8A6AE",                         // crc32Sum
        "9AA3CBF65B961D0BDAC98EC403B832E1", // md5Sum
        DOOM2_PREFIX " v1.9",               // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        140,                                // weight
    },
    {
        DOOM2_PREFIX " Classic Unity v1.3", // idName
        "DOOM2.WAD",                        // filename
        "F1D1AD55",                         // crc32Sum
        "8AB6D0527A29EFDC1EF200E5687B5CAE", // md5Sum
        DOOM2_PREFIX " v1.9",               // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        145,                                // weight
    },
    {
        DOOM2_PREFIX " Classic Unity v1.1", // idName
        "DOOM2.WAD",                        // filename
        "22C291C8",                         // crc32Sum
        "7895D10C281305C45A7E5F01B3F7B1D8", // md5Sum
        DOOM2_PREFIX " v1.9",               // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        145,                                // weight
    },
    {
        DOOM2_PREFIX " Classic Unity v1.0", // idName
        "DOOM2.WAD",                        // filename
        "897339A7",                         // crc32Sum
        "E7395BD5E838D58627BD028871EFBC14", // md5Sum
        DOOM2_PREFIX " v1.9",               // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        145,                                // weight
    },
    {
        DOOM2_PREFIX " BFG Edition",        // idName
        "DOOM2BFG.WAD",                     // filename
        "927A778A",                         // crc32Sum
        "C3BEA40570C23E511A7ED3EBCD9865F7", // md5Sum
        DOOM2_PREFIX " v1.9",               // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        150,                                // weight
    },
    {
        DOOM2_PREFIX " v1.8",                             // idName
        "DOOM2.WAD",                                      // filename
        "31BD3BC0",                                       // crc32Sum
        "C236745BB01D89BBB866C8FED81B6F8C",               // md5Sum
        DOOM2_PREFIX " v1.8",                             // groupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1100,                                             // weight
    },
    {
        DOOM2_PREFIX " v1.8 French",                      // idName
        "DOOM2F.WAD",                                     // filename
        "27EAAE69",                                       // crc32Sum
        "3CB02349B3DF649C86290907EED64E7B",               // md5Sum
        DOOM2_PREFIX " v1.8 French",                      // groupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1100,                                             // weight
    },
    {
        DOOM2_PREFIX " v1.7a",                            // mIdName
        "DOOM2.WAD",                                      // filename
        "952F6BAA",                                       // crc32Sum
        "D7A07E5D3F4625074312BC299D7ED33F",               // md5Sum
        DOOM2_PREFIX " v1.7a",                            // groupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1100,                                             // weight
    },
    {
        DOOM2_PREFIX " v1.7",                             // idName
        "DOOM2.WAD",                                      // filename
        "47DAEB2E",                                       // crc32Sum
        "EA74A47A791FDEF2E9F2EA8B8A9DA13B",               // md5Sum
        DOOM2_PREFIX " v1.7",                             // groupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1100,                                             // weight
    },
    {
        DOOM2_PREFIX " v1.666",                           // idName
        "DOOM2.WAD",                                      // filename
        "E2A683BD",                                       // crc32Sum
        "30E3C2D0350B67BFBF47271970B74B2F",               // md5Sum
        DOOM2_PREFIX " v1.666",                           // groupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1100,                                             // weight
    },
    {
        DOOM2_PREFIX " v1.666 German",                    // idName
        "DOOM2.WAD",                                      // filename
        "C08005F7",                                       // crc32Sum
        "D9153CED9FD5B898B36CC5844E35B520",               // md5Sum
        DOOM2_PREFIX " v1.666 German",                    // groupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1100,                                             // weight
    },

    // ------------------------------------------------------------------------
    // PLUTONIA.WAD
    // ------------------------------------------------------------------------
    {
        PLUTONIA_PREFIX " v1.9",            // mIdName
        "PLUTONIA.WAD",                     // mFilename
        "48D1453C",                         // mCRC32Sum
        "75C8CF89566741FA9D22447604053BD7", // mMd5Sum
        PLUTONIA_PREFIX " v1.9",            // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        300,                                // weight
    },
    {
        // version from October 3, 2024
        PLUTONIA_PREFIX " (DOOM + DOOM II)", // mIdName
        "PLUTONIA.WAD",                      // mFilename
        "E82FBFD2",                          // mCRC32Sum
        "E47CF6D82A0CCEDF8C1C16A284BB5937",  // mMd5Sum
        PLUTONIA_PREFIX " v1.9",             // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,       // flags
        320,                                 // weight
    },
    {
        // version on release
        PLUTONIA_PREFIX " (DOOM + DOOM II)", // mIdName
        "PLUTONIA.WAD",                      // mFilename
        "650B998D",                          // mCRC32Sum
        "24037397056E919961005E08611623F4",  // mMd5Sum
        PLUTONIA_PREFIX " v1.9",             // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,       // flags
        320,                                 // weight
    },
    {
        PLUTONIA_PREFIX " v1.9 Anthology",  // mIdName
        "PLUTONIA.WAD",                     // mFilename
        "15CD1448",                         // mCRC32Sum
        "3493BE7E1E2588BC9C8B31EAB2587A04", // mMd5Sum
        PLUTONIA_PREFIX " v1.9",            // groupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        325,                                // weight
    },

    // ------------------------------------------------------------------------
    // TNT.WAD
    // ------------------------------------------------------------------------
    {
        TNT_PREFIX " v1.9",                 // mIdName
        "TNT.WAD",                          // mFilename
        "903DCC27",                         // mCRC32Sum
        "4E158D9953C79CCF97BD0663244CC6B6", // mMd5Sum
        TNT_PREFIX " v1.9",                 // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        350,                                // weight
    },
    {
        // version from October 3, 2024
        TNT_PREFIX " (DOOM + DOOM II)",     // mIdName
        "TNT.WAD",                          // mFilename
        "6D7A8EEC",                         // mCRC32Sum
        "AD7885C17A6B9B79B09D7A7634DD7E2C", // mMd5Sum
        TNT_PREFIX " v1.9",                 // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        370,                                // weight
    },
    {
        // version on release
        TNT_PREFIX " (DOOM + DOOM II)",     // mIdName
        "TNT.WAD",                          // mFilename
        "15F18DDB",                         // mCRC32Sum
        "8974E3117ED4A1839C752D5E11AB1B7B", // mMd5Sum
        TNT_PREFIX " v1.9",                 // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        370,                                // weight
    },
    {
        TNT_PREFIX " v1.9 Anthology",       // mIdName
        "TNT.WAD",                          // mFilename
        "D4BB05C0",                         // mCRC32Sum
        "1D39E405BF6EE3DF69A8D2646C8D5C49", // mMd5Sum
        TNT_PREFIX " v1.9",                 // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        375,                                // weight
    },

    // ------------------------------------------------------------------------
    // DOOM.WAD
    // ------------------------------------------------------------------------
    {
        UDOOM_PREFIX " v1.9",               // mIdName
        "DOOMU.WAD",                        // mFilename
        "BF0EAAC0",                         // mCRC32Sum
        "C4FE9FD920207691A9F493668E0A2083", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        200,                                // weight
    },
    {
        // version from October 3, 2024
        UDOOM_PREFIX " (DOOM + DOOM II)",   // mIdName
        "DOOM.WAD",                         // mFilename
        "D5F8C089",                         // mCRC32Sum
        "3B37188F6337F15718B617C16E6E7A9C", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        240,                                // weight
    },
    {
        // version on release
        UDOOM_PREFIX " (DOOM + DOOM II)",   // mIdName
        "DOOM.WAD",                         // mFilename
        "CFF03D9F",                         // mCRC32Sum
        "4461D4511386518E784C647E3128E7BC", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        240,                                // weight
    },
    {
        UDOOM_PREFIX " Classic Unity v1.3", // mIdName
        "DOOM.WAD",                         // mFilename
        "75C3B7BF",                         // mCRC32Sum
        "8517C4E8F0EEF90B82852667D345EB86", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        245,                                // weight
    },
    {
        UDOOM_PREFIX " Classic Unity v1.1", // mIdName
        "DOOM.WAD",                         // mFilename
        "346A4BFD",                         // mCRC32Sum
        "21B200688D0FA7C1B6F63703D2BDD455", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        245,                                // weight
    },
    {
        UDOOM_PREFIX " Classic Unity v1.0", // mIdName
        "DOOM.WAD",                         // mFilename
        "46359DFB",                         // mCRC32Sum
        "232A79F7121B22D7401905EE0EE1E487", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        245,                                // weight
    },
    {
        UDOOM_PREFIX " BFG Edition",        // mIdName
        "DOOMBFG.WAD",                      // mFilename
        "5EFA677E",                         // mCRC32Sum
        "FB35C4A5A9FD49EC29AB6E900572C524", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD,      // flags
        250,                                // weight
    },
    {
        DOOM_PREFIX " v1.9",                              // mIdName
        "DOOM.WAD",                                       // mFilename
        "723E60F9",                                       // mCRC32Sum
        "1CD63C5DDFF1BF8CE844237F580E9CF3",               // mMd5Sum
        DOOM_PREFIX " v1.9",                              // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1200,                                             // weight
    },
    {
        DOOM_PREFIX " v1.8",                              // mIdName
        "DOOM.WAD",                                       // mFilename
        "8D242DF9",                                       // mCRC32Sum
        "11E1CD216801EA2657723ABC86ECB01F",               // mMd5Sum
        DOOM_PREFIX " v1.8",                              // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1200,                                             // weight
    },
    {
        DOOM_PREFIX " v1.666",                            // mIdName
        "DOOM.WAD",                                       // mFilename
        "F756AAB5",                                       // mCRC32Sum
        "54978D12DE87F162B9BCC011676CB3C0",               // mMd5Sum
        DOOM_PREFIX " v1.666",                            // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1200,                                             // weight
    },
    {
        DOOM_PREFIX " v1.2",                              // mIdName
        "DOOM.WAD",                                       // mFilename
        "A5DA8930",                                       // mCRC32Sum
        "792FD1FEA023D61210857089A7C1E351",               // mMd5Sum
        DOOM_PREFIX " v1.2",                              // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1200,                                             // weight
    },
    {
        DOOM_PREFIX " v1.1",                              // mIdName
        "DOOM.WAD",                                       // mFilename
        "66457AB9",                                       // mCRC32Sum
        "981B03E6D1DC033301AA3095ACC437CE",               // mMd5Sum
        DOOM_PREFIX " v1.1",                              // mGroupName
        IDENT_COMMERCIAL | IDENT_IWAD | IDENT_DEPRECATED, // flags
        1200,                                             // weight
    },

    // ------------------------------------------------------------------------
    // DOOM1.WAD
    // ------------------------------------------------------------------------
    {
        DOOMSW_PREFIX " v1.9",              // mIdName
        "DOOM1.WAD",                        // mFilename
        "162B696A",                         // mCRC32Sum
        "F0CEFCA49926D00903CF57551D901ABE", // mMd5Sum
        DOOMSW_PREFIX " v1.9",              // mGroupName
        IDENT_IWAD,                         // flags
        400,                                // weight
    },
    {
        DOOMSW_PREFIX " v1.8",              // mIdName
        "DOOM1.WAD",                        // mFilename
        "331EBF07",                         // mCRC32Sum
        "5F4EB849B1AF12887DEC04A2A12E5E62", // mMd5Sum
        DOOMSW_PREFIX " v1.8",              // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.666",            // mIdName
        "DOOM1.WAD",                        // mFilename
        "505FB740",                         // mCRC32Sum
        "C428EA394DC52835F2580D5BFD50D76F", // mMd5Sum
        DOOMSW_PREFIX " v1.666",            // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.6",              // mIdName
        "DOOM1.WAD",                        // mFilename
        "F26DCAD8",                         // mCRC32Sum
        "762FD6D4B960D4B759730F01387A50A1", // mMd5Sum
        DOOMSW_PREFIX " v1.6",              // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.5",              // mIdName
        "DOOM1.WAD",                        // mFilename
        "8653B0EB",                         // mCRC32Sum
        "E280233D533DCC28C1ACD6CCDC7742D4", // mMd5Sum
        DOOMSW_PREFIX " v1.5",              // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.4",              // mIdName
        "DOOM1.WAD",                        // mFilename
        "F5C2708D",                         // mCRC32Sum
        "A21AE40C388CB6F2C3CC1B95589EE693", // mMd5Sum
        DOOMSW_PREFIX " v1.4",              // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.25",             // mIdName
        "DOOM1.WAD",                        // mFilename
        "225D7FB1",                         // mCRC32Sum
        "17AEBD6B5F2ED8CE07AA526A32AF8D99", // mMd5Sum
        DOOMSW_PREFIX " v1.25",             // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.2",              // mIdName
        "DOOM1.WAD",                        // mFilename
        "BC842626",                         // mCRC32Sum
        "30AA5BEB9E5EBFBBE1E1765561C08F38", // mMd5Sum
        DOOMSW_PREFIX " v1.2",              // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.1",              // mIdName
        "DOOM1.WAD",                        // mFilename
        "981DCEBB",                         // mCRC32Sum
        "52CBC8882F445573CE421FA5453513C1", // mMd5Sum
        DOOMSW_PREFIX " v1.1",              // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v1.1 (Broken)",     // mIdName
        "DOOM1.WAD",                        // mFilename
        "289F4D3F",                         // mCRC32Sum
        "CEA4989DF52B65F4D481B706234A3DCA", // mMd5Sum
        DOOMSW_PREFIX " v1.1",              // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },
    {
        DOOMSW_PREFIX " v0.99",             // mIdName
        "DOOM1.WAD",                        // mFilename
        "EEDAE672",                         // mCRC32Sum
        "90FACAB21EEDE7981BE10790E3F82DA2", // mMd5Sum
        DOOMSW_PREFIX " v0.99",             // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1400,                               // weight
    },

    // ------------------------------------------------------------------------
    // NERVE.WAD
    // ------------------------------------------------------------------------
    {
        NERVE_PREFIX " BFG Edition",        // mIdName
        "NERVE.WAD",                        // mFilename
        "ad7f9292",                         // mCRC32Sum
        "967d5ae23daf45196212ae1b605da3b0", // mMd5Sum
        NERVE_PREFIX,                       // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },
    {
        NERVE_PREFIX " (DOOM + DOOM II)",   // mIdName
        "NERVE.WAD",                        // mFilename
        "07d9faab",                         // mCRC32Sum
        "23422eb42833ac7b0dd59c0c7ae18a6f", // mMd5Sum
        NERVE_PREFIX,                       // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },

    // ------------------------------------------------------------------------
    // MASTERLEVELS.WAD
    // ------------------------------------------------------------------------
    {
        MASTERLV_PREFIX " (PSN)",           // mIdName
        "MASTERLEVELS.WAD",                 // mFilename
        "6baec89f",                         // mCRC32Sum
        "84cb8640f599c4a17c8eb526f90d2b7a", // mMd5Sum
        MASTERLV_PREFIX " (PSN)",           // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },
    {
        // version from October 3, 2024
        MASTERLV_PREFIX " (DOOM + DOOM II)",// mIdName
        "MASTERLEVELS.WAD",                 // mFilename
        "D7053E8A",                         // mCRC32Sum
        "AB3CE78E085E50A61F6DFF46AABBFAEB", // mMd5Sum
        MASTERLV_PREFIX " (DOOM + DOOM II)",// mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },
    {
        // version on release
        MASTERLV_PREFIX " (DOOM + DOOM II)",// mIdName
        "MASTERLEVELS.WAD",                 // mFilename
        "07312a30",                         // mCRC32Sum
        "2d0e4fde4c83d90476f3f439bb5f3eea", // mMd5Sum
        MASTERLV_PREFIX " (DOOM + DOOM II)",// mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },

    // ------------------------------------------------------------------------
    // ID1.WAD
    // ------------------------------------------------------------------------
    {
        "Legacy of Rust v1.2",              // mIdName
        "ID1.WAD",                          // mFilename
        "3A495080",                         // mCRC32Sum
        "95F21547BE5E0BFF38D412017440F656", // mMd5Sum
        "Legacy of Rust",                   // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },
    {
        "Legacy of Rust v1.1",              // mIdName
        "ID1.WAD",                          // mFilename
        "e2e73e06",                         // mCRC32Sum
        "681bcea18c1286e8b9986c335034bdd1", // mMd5Sum
        "Legacy of Rust",                   // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },

    // ------------------------------------------------------------------------
    // IDDM1.WAD
    // ------------------------------------------------------------------------
    {
        "ID Deathmatch Pack #1",            // mIdName
        "IDDM1.WAD",                        // mFilename
        "11fe5048",                         // mCRC32Sum
        "5670fd8fe8eb6910ec28f9e27969d84f", // mMd5Sum
        "ID Deathmatch Pack #1",            // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },

    // ------------------------------------------------------------------------
    // ID1-RES.WAD
    // ------------------------------------------------------------------------
    {
        "id1-res v1.2",                     // mIdName
        "ID1-RES.WAD",                      // mFilename
        "5B7DEA04",                         // mCRC32Sum
        "F8FBAB472230BFA090D6A9234D65FAE6", // mMd5Sum
        "id1-res",                          // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },
    {
        "id1-res v1.1",                     // mIdName
        "ID1-RES.WAD",                      // mFilename
        "0b9a1202",                         // mCRC32Sum
        "b6b2370ae8733aaf1377b0ef12351572", // mMd5Sum
        "id1-res",                          // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },

    // ------------------------------------------------------------------------
    // ID1-WEAP.WAD
    // ------------------------------------------------------------------------
    {
        "id1-weap v1.2",                    // mIdName
        "ID1-WEAP.WAD",                     // mFilename
        "EC52FDB9",                         // mCRC32Sum
        "85D25C8C3D06A05A1283AE4AFE749C9F", // mMd5Sum
        "id1-weap",                         // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },
    {
        "id1-weap v1.1",                    // mIdName
        "ID1-WEAP.WAD",                     // mFilename
        "ec81d166",                         // mCRC32Sum
        "b3247939c60f6a819c625036b52a5f53", // mMd5Sum
        "id1-weap",                         // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },

    // ------------------------------------------------------------------------
    // ID24RES.WAD
    // ------------------------------------------------------------------------
    {
        "id24res",                          // mIdName
        "ID24RES.WAD",                      // mFilename
        "6875903f",                         // mCRC32Sum
        "4f0651accebc007b853943ac12aa95b8", // mMd5Sum
        "id24res",                          // mGroupName
        IDENT_COMMERCIAL,                   // flags
        PWAD_NO_WEIGHT                      // weight
    },

    // ------------------------------------------------------------------------
    // FREEDOOM1.WAD
    // ------------------------------------------------------------------------
    {
        FREEDOOM1_PREFIX " v0.13.0",        // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "5B55E156",                         // mCRC32Sum
        "B93BE13D05148DD01614BC205A03648E", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD,                         // flags
        525,                                // weight
    },
    {
        FREEDOOM1_PREFIX " v0.12.1",        // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "DE6DDB27",                         // mCRC32Sum
        "B36AA44A23045E503C19AF4B4C438A78", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.12.0",        // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "0C5F8FF45CC3538D368A0F8D8FC11CE3", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.11.3",        // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "EA471A3D38FCEE0FB3A69BCD3221E335", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.11.2",        // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "6D00C49520BE26F08A6BD001814A32AB", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.11.1",        // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "35312E99D2473297AABE0602700BEE8A", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.11",          // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "21A4707FC25D29EDF4B098BD400C5C42", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.10.1",        // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "91DE79621A393A08C39A9AB2C034B766", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.10",          // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "9B8D72B59FD93B2B3E116149BAA1B142", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.9",           // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "ACA90CF5AC36E996EDC58BD0329B979A", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },
    {
        FREEDOOM1_PREFIX " v0.8",           // mIdName
        "FREEDOOM1.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "30095B256DD3A1566BBC30286F72BC47", // mMd5Sum
        UDOOM_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1525,                               // weight
    },

    // ------------------------------------------------------------------------
    // FREEDOOM2.WAD
    // ------------------------------------------------------------------------
    {
        FREEDOOM2_PREFIX " v0.13.0",        // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "68A76EB5",                         // mCRC32Sum
        "CD666466759B5E5F63AF93C5F0FFD0A1", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD,                         // flags
        500,                                // weight
    },
    {
        FREEDOOM2_PREFIX " v0.12.1",        // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "212E1CF9",                         // mCRC32Sum
        "CA9A4159A7833544A89144C7F5053412", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.12.0",        // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "83560B2963424FA4A2EB971194428BF8", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.11.3",        // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "984F99AF08F085E38070F51095AB7C31", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.11.2",        // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "90832A872B5BB0ACA4CA0B20419AAD5D", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.11.1",        // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "EC5B38B30BA2B70E278205776AF3FBB5", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.11",          // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "B1018017C61B06E33C11102D8BAFAAD0", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.10.1",        // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "DD9C9E73F5F50D3778C85573CD08D9A4", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.10",          // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "C5A4F2D38D78B251D8557CB2D93E40EE", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.9",           // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "8FA57DBC7687F84528EBA39DDE3A20E0", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },
    {
        FREEDOOM2_PREFIX " v0.8",           // mIdName
        "FREEDOOM2.WAD",                    // mFilename
        "",                                 // mCRC32Sum
        "E3668912FC37C479B2840516C887018B", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1500,                               // weight
    },

    // ------------------------------------------------------------------------
    // FREEDM.WAD
    // ------------------------------------------------------------------------
    {
        FREEDM_PREFIX " v0.13.0",           // mIdName
        "FREEDM.WAD",                       // mFilename
        "E5636B13",                         // mCRC32Sum
        "908DFD77A14CC490C4CEA94B62D13449", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD,                         // flags
        550,                                // weight
    },
    {
        FREEDM_PREFIX " v0.12.1",           // mIdName
        "FREEDM.WAD",                       // mFilename
        "BD680D11",                         // mCRC32Sum
        "D40C932A9183DED919AFA89F4A729668", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.12.0",           // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "3250AAD8B1D40FB7B25B7DF6573EB29F", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.11.3",           // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "87EE2494D921633420CE9BDB418127C4", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.11.2",           // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "9352B09AE878DC52C6C18AA38ACDA6EB", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.11.1",           // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "77BA9C0F75C32E4A729490688BB99241", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.11",             // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "D76D3973C075B069ECB4E16DC9EACBB4", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.10.1",           // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "BD4F359F1963E388BEDA014C5548B420", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.10",             // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "F37B8B70E1394289A7EC404F67CDEC1A", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.9",              // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "CBB27C5F3C2C44D34843CF63DAA627F6", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },
    {
        FREEDM_PREFIX " v0.8",              // mIdName
        "FREEDM.WAD",                       // mFilename
        "",                                 // mCRC32Sum
        "05859098BF191899903EF343AFBA369D", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD | IDENT_DEPRECATED,      // flags
        1550,                               // weight
    },

    // ------------------------------------------------------------------------
    // CHEX.WAD
    // ------------------------------------------------------------------------
    {
        "Chex Quest",                       // mIdName
        "CHEX.WAD",                         // mFilename
        "298DD5B5",                         // mCRC32Sum
        "25485721882B050AFA96A56E5758DD52", // mMd5Sum
        "Chex Quest",                       // mGroupName
        IDENT_IWAD,                         // flags
        600,                                // weight
    },

    // ------------------------------------------------------------------------
    // HACX.WAD
    // ------------------------------------------------------------------------
    {
        "HACX v1.2",                        // mIdName
        "HACX.WAD",                         // mFilename
        "72E3B8AC",                         // mCRC32Sum
        "65ED74D522BDF6649C2831B13B9E02B4", // mMd5Sum
        DOOM2_PREFIX " v1.9",               // mGroupName
        IDENT_IWAD,                         // flags
        600,                                // weight
    },

    // ------------------------------------------------------------------------
    // REKKRSA.WAD
    // ------------------------------------------------------------------------
    {
        "REKKR v1.16a",                     // mIdName
        "REKKRSA.WAD",                      // mFilename
        "0D294CA5",                         // mCRC32Sum
        "B6F4BB3A80F096B6045CFAEB57D4CF29", // mMd5Sum
        "REKKR",                            // mGroupName
        IDENT_IWAD,                         // flags
        600,                                // weight
    },
};


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
	void addFile(const OString& idname, const OString& filename, const OString& crc32,
	             const OString& md5, const OString& group, bool commercial,
	             const bool iwad, const bool deprecated, const int weight)
	{
		OCRC32Sum crc32Hash;
		OCRC32Sum::makeFromHexStr(crc32Hash, crc32);
		OMD5Hash md5Hash;
		OMD5Hash::makeFromHexStr(md5Hash, md5);

		IdType id = mIdentifiers.insert();
		fileIdentifier_t* file = &mIdentifiers.get(id);
		file->mIdName = OStringToUpper(idname);
		file->mNiceName = idname;
		file->mFilename = OStringToUpper(filename);
		file->mCRC32Sum = crc32Hash;
		file->mMd5Sum = md5Hash;
		file->mGroupName = OStringToUpper(group);
		file->mIsCommercial = commercial;
		file->mIsIWAD = iwad;
		file->mIsDeprecated = deprecated;
		file->weight = weight;

		if (!crc32Hash.empty())
		{
			mCRC32SumLookup.insert(std::make_pair(file->mCRC32Sum, id));
		}

		if (!md5Hash.empty())
		{
			mMd5SumLookup.insert(std::make_pair(file->mMd5Sum, id));
		}

        if (iwad)
        {
		    // add the filename to the IWAD search list if it's not already in there
		    if (std::find(mIWADSearchOrder.begin(), mIWADSearchOrder.end(), file->mFilename) == mIWADSearchOrder.end())
		    	mIWADSearchOrder.push_back(file->mFilename);
        }
	}

	std::vector<OString> getFilenames() const
	{
		return mIWADSearchOrder;
	}

	bool isCommercialFilename(const std::string& filename) const
	{
		OString upper = StdStringToUpper(filename);
		for (IdentifierTable::const_iterator it = mIdentifiers.begin(); it != mIdentifiers.end(); ++it)
		{
			if (it->mIsCommercial && it->mFilename == upper)
				return true;
		}
		return false;
	}

	bool isKnownIWADFilename(const std::string& filename) const
	{
		OString upper = StdStringToUpper(filename);
		for (IdentifierTable::const_iterator it = mIdentifiers.begin();
		     it != mIdentifiers.end(); ++it)
		{
			if (it->mIsIWAD && it->mFilename == upper)
				return true;
		}
		return false;
	}

	bool isCommercial(const OMD5Hash& hash) const
	{
		const fileIdentifier_t* file = lookupByMd5Sum(hash);
		return file && file->mIsCommercial;
	}

	bool isKnownIWAD(const OMD5Hash& hash) const
	{
		const fileIdentifier_t* file = lookupByMd5Sum(hash);
		return file && file->mIsIWAD;
	}

	bool isDeprecated(const OMD5Hash& hash) const
	{
		const fileIdentifier_t* file = lookupByMd5Sum(hash);
		return file && file->mIsDeprecated;
	}

	bool isIWAD(const OResFile& file) const
	{
		const OMD5Hash& md5sum(file.getMD5());
		const fileIdentifier_t* ident = lookupByMd5Sum(md5sum);
		if (ident)
			return ident->mIsIWAD;

		// [SL] not an offical IWAD.
		// Check for lumps that are required by vanilla Doom.
		static const int NUM_CHECKLUMPS = 6;
		static const char checklumps[NUM_CHECKLUMPS][8] = {
		    {'P', 'L', 'A', 'Y', 'P', 'A', 'L'},      // 0
		    {'C', 'O', 'L', 'O', 'R', 'M', 'A', 'P'}, // 1
		    {'F', '_', 'S', 'T', 'A', 'R', 'T'},      // 2
		    {'S', '_', 'S', 'T', 'A', 'R', 'T'},      // 3
		    {'T', 'E', 'X', 'T', 'U', 'R', 'E', '1'}, // 4
		    {'S', 'T', 'D', 'I', 'S', 'K'}            // 5
		};

		WadFileLumpFinder lumps(file.getFullpath());
		for (int i = 0; i < NUM_CHECKLUMPS; i++)
			if (!lumps.exists(std::string(checklumps[i], 8)))
				return false;
		return true;
	}

	bool areCompatible(const OMD5Hash& hash1, const OMD5Hash& hash2) const
	{
		const fileIdentifier_t* file1 = lookupByMd5Sum(hash1);
		const fileIdentifier_t* file2 = lookupByMd5Sum(hash2);

		if (!file1 || !file2)
			return false;

		return file1->mGroupName == file2->mGroupName;
	}

	const OString identify(const OResFile& file)
	{
		const fileIdentifier_t* fileid = lookupByMd5Sum(file.getMD5());

		if (fileid != NULL)
			return fileid->mIdName;

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

		WadFileLumpFinder lumps(file.getFullpath());
		for (int i = 0; i < NUM_CHECKLUMPS; i++)
			if (lumps.exists(std::string(checklumps[i], 8)))
				lumpsfound[i] = true;

		// [ML] Check for HACX 1.2
		if (lumpsfound[11])
		{
			return "HACX UNKNOWN";
		}

		// [SL] Check for FreeDoom / Freedoom: Phase 1
		if (lumpsfound[10])
		{
			if (lumpsfound[0])
				return OStringToUpper(OString(FREEDOOM1_PREFIX " Unknown"));
			else
				return "FREEDOOM UNKNOWN";
		}

		// Check for Doom II: Hell on Earth or TNT / Plutonia
		if (lumpsfound[3])
		{
			if (lumpsfound[6])
				return OStringToUpper(OString(TNT_PREFIX " Unknown"));
			if (lumpsfound[7])
				return OStringToUpper(OString(PLUTONIA_PREFIX " Unknown"));
			if (lumpsfound[9])
				return OStringToUpper(OString(DOOM2_PREFIX " BFG Edition Unknown"));
			else
				return OStringToUpper(OString(DOOM2_PREFIX " Unknown"));
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
					if (iequals(file.getBasename(), "chex.wad"))
					{
						return "CHEX QUEST UNKNOWN";
					}
					else
					{
						if (lumpsfound[9])
							return UDOOM_PREFIX " BFG UNKNOWN";
						else
							return UDOOM_PREFIX " UNKNOWN";
					}
				}
				else
				{
					return DOOM_PREFIX " UNKNOWN";
				}
			}
			else
			{
				return DOOMSW_PREFIX " UNKNOWN";
			}
		}

		return "UNKNOWN";
	}

	void dump() const
	{
		for (IdentifierTable::const_iterator it = mIdentifiers.begin();
		     it != mIdentifiers.end(); ++it)
		{
			Printf(PRINT_HIGH, "%s %s %s\n", it->mGroupName.c_str(),
			       it->mFilename.c_str(), it->mMd5Sum.getHexCStr());
		}
	}

	const fileIdentifier_t* lookupByCRC32Sum(const OCRC32Sum& crc32sum) const
	{
		CRC32SumLookupTable::const_iterator it = mCRC32SumLookup.find(crc32sum);
		if (it != mCRC32SumLookup.end())
			return &mIdentifiers.get(it->second);
		return NULL;
	}

	const fileIdentifier_t* lookupByMd5Sum(const OMD5Hash& md5sum) const
	{
		Md5SumLookupTable::const_iterator it = mMd5SumLookup.find(md5sum);
		if (it != mMd5SumLookup.end())
			return &mIdentifiers.get(it->second);
		return NULL;
	}

  private:
	typedef unsigned int IdType;

	typedef SArray<fileIdentifier_t> IdentifierTable;
	IdentifierTable			mIdentifiers;

	typedef OHashTable<OCRC32Sum, IdType> CRC32SumLookupTable;
	CRC32SumLookupTable		mCRC32SumLookup;

	typedef OHashTable<OMD5Hash, IdType> Md5SumLookupTable;
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
	for (size_t i = 0; i < ARRAY_LENGTH(::identdata); i++)
	{
		const identData_t& data = ::identdata[i];
		::identtab.addFile(data.idName, data.filename, data.crc32Sum, data.md5Sum,
		                   data.groupName, data.flags & IDENT_COMMERCIAL,
		                   data.flags & IDENT_IWAD, data.flags & IDENT_DEPRECATED,
		                   data.weight);
	}
}

/**
 * @brief Return the gameinfo associated with the given CRC32.
 */
const fileIdentifier_t* W_GameInfo(const OCRC32Sum& crc32)
{
	return ::identtab.lookupByCRC32Sum(crc32);
}

/**
 * @brief Return the gameinfo associated with the given MD5.
 */
const fileIdentifier_t* W_GameInfo(const OMD5Hash& md5)
{
	return ::identtab.lookupByMd5Sum(md5);
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
void W_ConfigureGameInfo(const OResFile& iwad)
{
	extern gameinfo_t SharewareGameInfo;
	extern gameinfo_t RegisteredGameInfo;
	extern gameinfo_t RetailGameInfo;
	extern gameinfo_t CommercialGameInfo;
	extern gameinfo_t RetailBFGGameInfo;
	extern gameinfo_t CommercialBFGGameInfo;

	const OString idname = identtab.identify(iwad);

	if (idname.find("REKKR") == 0)
    {
		gamemode = retail;
		gameinfo = RetailGameInfo;
		gamemission = doom;
	}
	else if (idname.find("HACX") == 0)
	{
		gameinfo = CommercialGameInfo;
		gamemode = commercial;
		gamemission = commercial_hacx;
	}
	else if (idname.find(OStringToUpper(OString(PLUTONIA_PREFIX))) == 0)
	{
		gameinfo = CommercialGameInfo;
		gamemode = commercial;
		gamemission = pack_plut;
	}
	else if (idname.find(OStringToUpper(OString(TNT_PREFIX))) == 0)
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
	else if (idname.find(OStringToUpper(OString(FREEDOOM1_PREFIX))) == 0)
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
	else if (idname.find(OStringToUpper(OString(FREEDOOM2_PREFIX))) == 0)
	{
		gamemode = commercial;
		gameinfo = CommercialGameInfo;
		gamemission = commercial_freedoom;
	}

	else if (idname.find(OStringToUpper(OString(FREEDM_PREFIX))) == 0)
	{
		gamemode = commercial;
		gameinfo = CommercialGameInfo;
		gamemission = commercial_freedoom;
	}
	else if (idname.find(OStringToUpper(OString(DOOMSW_PREFIX))) == 0)
	{
		gamemode = shareware;
		gameinfo = SharewareGameInfo;
		gamemission = doom;
	}
	else if (idname.find(OStringToUpper(OString(UDOOM_PREFIX " BFG"))) == 0)
	{
		gamemode = retail_bfg;
		gameinfo = RetailBFGGameInfo;
		gamemission = doom;
	}
	else if (idname.find(OStringToUpper(OString(UDOOM_PREFIX))) == 0)
	{
		gamemode = retail;
		gameinfo = RetailGameInfo;
		gamemission = doom;
	}
	else if (idname.find(OStringToUpper(OString(DOOM2_PREFIX " BFG"))) == 0)
	{
		gameinfo = CommercialBFGGameInfo;
		gamemode = commercial_bfg;
		gamemission = doom2;
	}
	else if (idname.find(OStringToUpper(OString(DOOM2_PREFIX))) == 0)
	{
		gameinfo = CommercialGameInfo;
		gamemode = commercial;
		gamemission = doom2;
	}
	else if (idname.find(OStringToUpper(OString(DOOM_PREFIX))) == 0)
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
// W_IsKnownIWAD
//
// Returns true if the given file is a known IWAD file.
//
bool W_IsKnownIWAD(const OWantFile& file)
{
	if (::identtab.isKnownIWAD(file.getWantedMD5()))
		return true;

	if (::identtab.isKnownIWADFilename(file.getBasename()))
		return true;

	return false;
}


//
// W_IsIWAD
//
// Returns true if the given file is an IWAD file.
//
bool W_IsIWAD(const OResFile& file)
{
	return ::identtab.isIWAD(file);
}


//
// W_IsFilenameCommercialWAD
//
// Checks to see whether a given filename is an WAD flagged as "commercial"
//
bool W_IsFilenameCommercialWAD(const std::string& filename)
{
	return identtab.isCommercialFilename(filename);
}


//
// W_IsFilehashCommercialWAD
//
// Checks to see whether a given hash belongs to an WAD flagged as "commercial"
//
bool W_IsFilehashCommercialWAD(const OMD5Hash& fileHash)
{
	return identtab.isCommercial(fileHash);
}


//
// W_IsFileCommercialWAD
//
// Checks to see whether a given file on disk is an WAD flagged as "commercial"
//
bool W_IsFileCommercialWAD(const std::string& filename)
{
	const OMD5Hash md5sum = W_MD5(filename);
	return identtab.isCommercial(md5sum);
}


//
// W_IsIWADDeprecated
//
// Checks to see whether a given file is an IWAD flagged as "deprecated"
//
bool W_IsIWADDeprecated(const OResFile& file)
{
	return identtab.isDeprecated(file.getMD5());
}


std::vector<OString> W_GetIWADFilenames()
{
	return identtab.getFilenames();
}

VERSION_CONTROL (w_ident_cpp, "$Id$")
