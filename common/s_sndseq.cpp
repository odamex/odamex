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
//  Sound Sequences
//
//-----------------------------------------------------------------------------

#include <cstring>
#include <stdio.h>

#include "doomtype.h"
#include "doomstat.h"
#include "sc_man.h"
#include "m_alloc.h"
#include "m_random.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "cmdlib.h"
#include "p_local.h"

// MACROS ------------------------------------------------------------------

#define MAX_SEQSIZE			256
#define GetCommand(a)		((a)>>24)
#define GetData(a)			( ((a) & 0xffffff) == 0xffffff ? ~0 : ((a) & 0xffffff) )
#define MakeCommand(a,b)	(((a) << 24)|((b) & 0xffffff))
#define HexenPlatSeq(a)		(a)
#define HexenDoorSeq(a)		((a) | 0x40)
#define HexenEnvSeq(a)		((a) | 0x80)
#define HexenLastSeq		(0xff)

// TYPES -------------------------------------------------------------------

typedef enum
{
	SS_STRING_PLAY,
	SS_STRING_PLAYUNTILDONE,
	SS_STRING_PLAYTIME,
	SS_STRING_PLAYREPEAT,
	SS_STRING_PLAYLOOP,
	SS_STRING_DELAY,
	SS_STRING_DELAYRAND,
	SS_STRING_VOLUME,
	SS_STRING_END,
	SS_STRING_STOPSOUND,
	SS_STRING_ATTENUATION,
	SS_STRING_DOOR,
	SS_STRING_PLATFORM,
	SS_STRING_ENVIRONMENT,
	SS_STRING_NOSTOPCUTOFF
} ssstrings_t;

typedef enum
{
	SS_CMD_NONE,
	SS_CMD_PLAY,
	SS_CMD_WAITUNTILDONE, // used by PLAYUNTILDONE
	SS_CMD_PLAYTIME,
	SS_CMD_PLAYREPEAT,
	SS_CMD_PLAYLOOP,
	SS_CMD_DELAY,
	SS_CMD_DELAYRAND,
	SS_CMD_VOLUME,
	SS_CMD_STOPSOUND,
	SS_CMD_ATTENUATION,
	SS_CMD_END
} sscmds_t;

typedef struct {
	const char		*name;
	byte			seqs[4];
} hexenseq_t;

class DSeqActorNode : public DSeqNode
{
	DECLARE_SERIAL (DSeqActorNode, DSeqNode)
public:
	DSeqActorNode (AActor *actor, int sequence);
	~DSeqActorNode ();
	void MakeSound () { S_SoundID (m_Actor, CHAN_BODY, m_CurrentSoundID, m_Volume, m_Atten); }
	void MakeLoopedSound () { S_LoopedSoundID (m_Actor, CHAN_BODY, m_CurrentSoundID, m_Volume, m_Atten); }
	bool IsPlaying () { return S_GetSoundPlayingInfo (m_Actor, m_CurrentSoundID); }
	void *Source () { return m_Actor; }
	virtual void DestroyedPointer(DObject *obj);
private:
	DSeqActorNode () {}
	AActor *m_Actor;
};

class DSeqPolyNode : public DSeqNode
{
	DECLARE_SERIAL (DSeqPolyNode, DSeqNode)
public:
	DSeqPolyNode (polyobj_t *poly, int sequence);
	~DSeqPolyNode ();
	void MakeSound () { S_SoundID (&m_Poly->startSpot[0], CHAN_BODY, m_CurrentSoundID, m_Volume, m_Atten); }
	void MakeLoopedSound () { S_LoopedSoundID (&m_Poly->startSpot[0], CHAN_BODY, m_CurrentSoundID, m_Volume, m_Atten); }
	bool IsPlaying () { return S_GetSoundPlayingInfo (&m_Poly->startSpot[0], m_CurrentSoundID); }
	void *Source () { return m_Poly; }
private:
	DSeqPolyNode () {}
	polyobj_t *m_Poly;
};

class DSeqSectorNode : public DSeqNode
{
	DECLARE_SERIAL (DSeqSectorNode, DSeqNode)
public:
	DSeqSectorNode (sector_t *sec, int sequence);
	~DSeqSectorNode ();
	void MakeSound () { S_SoundID (&m_Sector->soundorg[0], CHAN_BODY, m_CurrentSoundID, m_Volume, m_Atten); }
	void MakeLoopedSound () { S_LoopedSoundID (&m_Sector->soundorg[0], CHAN_BODY, m_CurrentSoundID, m_Volume, m_Atten); }
	bool IsPlaying () { return S_GetSoundPlayingInfo (m_Sector->soundorg, m_CurrentSoundID); }
	void *Source () { return m_Sector; }
private:
	DSeqSectorNode() {}
	sector_t *m_Sector;
};

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void VerifySeqPtr (int pos, int need);
static void AssignTranslations (int seq, seqtype_t type);
static void AssignHexenTranslations (void);

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static const char *SSStrings[] = {
	"play",
	"playuntildone",
	"playtime",
	"playrepeat",
	"playloop",
	"delay",
	"delayrand",
	"volume",
	"end",
	"stopsound",
	"attenuation",
	"door",
	"platform",
	"environment",
	"nostopcutoff",
	NULL
};
static const char *Attenuations[] = {
	"none",
	"normal",
	"idle",
	"static",
	"surround",
	NULL
};
static const hexenseq_t HexenSequences[] = {
	{ "Platform",		{ HexenPlatSeq(0), HexenPlatSeq(1), HexenPlatSeq(3), HexenLastSeq } },
	{ "PlatformMetal",	{ HexenPlatSeq(2), HexenLastSeq } },
	{ "Silence",		{ HexenPlatSeq(4), HexenDoorSeq(4), HexenLastSeq } },
	{ "Lava",			{ HexenPlatSeq(5), HexenDoorSeq(5), HexenLastSeq } },
	{ "Water",			{ HexenPlatSeq(6), HexenDoorSeq(6), HexenLastSeq } },
	{ "Ice",			{ HexenPlatSeq(7), HexenDoorSeq(7), HexenLastSeq } },
	{ "Earth",			{ HexenPlatSeq(8), HexenDoorSeq(8), HexenLastSeq } },
	{ "PlatformMetal2",	{ HexenPlatSeq(9), HexenLastSeq } },
	{ "DoorNormal",		{ HexenDoorSeq(0), HexenLastSeq } },
	{ "DoorHeavy",		{ HexenDoorSeq(1), HexenLastSeq } },
	{ "DoorMetal",		{ HexenDoorSeq(2), HexenLastSeq } },
	{ "DoorCreak",		{ HexenDoorSeq(3), HexenLastSeq } },
	{ "DoorMetal2",		{ HexenDoorSeq(9), HexenLastSeq } },
	{ "Wind",			{ HexenEnvSeq(10), HexenLastSeq } },
	{ NULL, }
};

static int SeqTrans[64*3];
static unsigned int *ScriptTemp;
static int ScriptTempSize;

sndseq_t **Sequences;
int NumSequences;
int MaxSequences;
int ActiveSequences;
DSeqNode *DSeqNode::SequenceListHead;


// CODE --------------------------------------------------------------------

void DSeqNode::SerializeSequences (FArchive &arc)
{
	if (arc.IsStoring ())
	{
		arc << SequenceListHead;
	}
	else
	{
		SN_StopAllSequences ();
		arc >> SequenceListHead;
	}
}

IMPLEMENT_SERIAL (DSeqNode, DObject)

DSeqNode::DSeqNode ()
{
	m_Next = m_Prev = NULL;
}

void DSeqNode::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << (DWORD)SN_GetSequenceOffset (m_Sequence, m_SequencePtr)
			<< m_DelayTics
			<< m_Volume
			<< m_Atten
			<< S_sfx[m_CurrentSoundID].name
			<< Sequences[m_Sequence]->name
			<< m_Next
			<< m_Prev;
	}
	else
	{
		std::string seqName, soundName;
		int seqOffset = 0, delayTics = 0;
		float volume;
		int atten = ATTN_NORM;

		arc >> seqOffset
			>> delayTics
			>> volume
			>> atten
			>> soundName
			>> seqName
			>> m_Next
			>> m_Prev;

		int i;

		for (i = 0; i < NumSequences; i++)
		{
			if (!stricmp (seqName.c_str(), Sequences[i]->name))
			{
				ActivateSequence (i);
				break;
			}
		}
		if (i == NumSequences)
			I_Error ("Unknown sound sequence '%s'\n", seqName.c_str());

		ChangeData (seqOffset, delayTics, volume, S_FindSound (soundName.c_str()));
	}
}

IMPLEMENT_SERIAL (DSeqActorNode, DSeqNode)

void DSeqActorNode::DestroyedPointer(DObject *obj)
{
	if(obj == m_Actor)
		m_Actor = NULL;
}

void DSeqActorNode::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
		arc << m_Actor;
	else
		arc >> m_Actor;
}

IMPLEMENT_SERIAL (DSeqPolyNode, DSeqNode)

void DSeqPolyNode::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
		arc << (WORD)(m_Poly - polyobjs);
	else
	{
		WORD ofs;
		arc >> ofs;
		m_Poly = polyobjs + ofs;
	}
}

IMPLEMENT_SERIAL (DSeqSectorNode, DSeqNode)

void DSeqSectorNode::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
		arc << m_Sector;
	else
		arc >> m_Sector;
}

//==========================================================================
//
// VerifySequencePtr
//
//==========================================================================

static void VerifySeqPtr (int pos, int need)
{
	if (pos + need > ScriptTempSize)
	{
		ScriptTempSize *= 2;
		ScriptTemp = (unsigned int *)Realloc (ScriptTemp, ScriptTempSize * sizeof(*ScriptTemp));
	}
}

//==========================================================================
//
// AssignTranslations
//
//==========================================================================

static void AssignTranslations (int seq, seqtype_t type)
{
	sc.Crossed = false;

	while (sc.GetString () && !sc.Crossed)
	{
		if (IsNum (sc.String))
		{
			SeqTrans[(atoi(sc.String) & 63) + type * 64] = seq;
		}
	}

	sc.UnGet ();
}

//==========================================================================
//
// AssignHexenTranslations
//
//==========================================================================

static void AssignHexenTranslations (void)
{
	int i, j, seq;

	for (i = 0; HexenSequences[i].name; i++)
	{
		for (seq = 0; seq < NumSequences; seq++)
		{
			if (!stricmp (HexenSequences[i].name, Sequences[seq]->name))
				break;
		}
		if (seq == NumSequences)
			continue;

		for (j = 0; j < 4 && HexenSequences[i].seqs[j] != HexenLastSeq; j++)
		{
			int trans;

			if (HexenSequences[i].seqs[j] & 0x40)
				trans = 64;
			else if (HexenSequences[i].seqs[j] & 0x80)
				trans = 64*2;
			else
				trans = 0;

			SeqTrans[trans + (HexenSequences[i].seqs[j] & 0x3f)] = seq;
		}
	}
}

//==========================================================================
//
// S_ParseSndSeq
//
//==========================================================================

void S_ParseSndSeq (void)
{
	char name[MAX_SNDNAME+1];
	int stopsound;
	int cursize;
	int curseq = -1;

	// denis - reboot safe
	if(Sequences)
	{
		free(Sequences);
		Sequences = 0;
		MaxSequences = 0;
	}

	// be gone, compiler warnings
	cursize = 0;
	stopsound = -1;

	memset (SeqTrans, -1, sizeof(SeqTrans));
	name[MAX_SNDNAME] = 0;
	ScriptTemp = (unsigned int *)Malloc (MAX_SEQSIZE * sizeof(*ScriptTemp));
	ScriptTempSize = MAX_SEQSIZE;

	int lump = -1;
	while ((lump = wads.FindLump("SNDSEQ", lump)) != -1)
	{
		sc.OpenLumpNum (lump, "SNDSEQ");
		while (sc.GetString ())
		{
			if (*sc.String == ':')
			{
				if (curseq != -1)
				{
					sc.ScriptError ("S_ParseSndSeq: Nested Script Error");
				}
				strncpy (name, sc.String + 1, MAX_SNDNAME);
				for (curseq = 0; curseq < NumSequences; curseq++)
				{
					if (stricmp (Sequences[curseq]->name, name) == 0)
					{
						Z_Free (Sequences[curseq]);
						Sequences[curseq] = NULL;
						break;
					}
				}
				if (curseq == NumSequences)
					NumSequences++;
				if (NumSequences > MaxSequences)
				{
					MaxSequences = MaxSequences ? MaxSequences * 2 : 64;
					Sequences = (sndseq_t **)Realloc (Sequences, MaxSequences * sizeof(*Sequences));
				}
				memset (ScriptTemp, 0, sizeof(*ScriptTemp) * ScriptTempSize);
				stopsound = -1;
				cursize = 0;
				continue;
			}
			if (curseq == -1)
			{
				continue;
			}
			switch (sc.MustMatchString (SSStrings))
			{
				case SS_STRING_PLAYUNTILDONE:
					VerifySeqPtr (cursize, 2);
					sc.MustGetString ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_PLAY, S_FindSound (sc.String));
					ScriptTemp[cursize++] = SS_CMD_WAITUNTILDONE << 24;
					break;

				case SS_STRING_PLAY:
					VerifySeqPtr (cursize, 1);
					sc.MustGetString ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_PLAY, S_FindSound (sc.String));
					break;

				case SS_STRING_PLAYTIME:
					VerifySeqPtr (cursize, 2);
					sc.MustGetString ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_PLAY, S_FindSound (sc.String));
					sc.MustGetNumber ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_DELAY, sc.Number);
					break;

				case SS_STRING_PLAYREPEAT:
					VerifySeqPtr (cursize, 1);
					sc.MustGetString ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_PLAYREPEAT, S_FindSound (sc.String));
					break;

				case SS_STRING_PLAYLOOP:
					VerifySeqPtr (cursize, 2);
					sc.MustGetString ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_PLAYLOOP, S_FindSound (sc.String));
					sc.MustGetNumber ();
					ScriptTemp[cursize++] = sc.Number;
					break;

				case SS_STRING_DELAY:
					VerifySeqPtr (cursize, 1);
					sc.MustGetNumber ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_DELAY, sc.Number);
					break;

				case SS_STRING_DELAYRAND:
					VerifySeqPtr (cursize, 2);
					sc.MustGetNumber ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_DELAYRAND, sc.Number);
					sc.MustGetNumber ();
					ScriptTemp[cursize++] = sc.Number;
					break;

				case SS_STRING_VOLUME:
					VerifySeqPtr (cursize, 1);
					sc.MustGetNumber ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_VOLUME, sc.Number);
					break;

				case SS_STRING_STOPSOUND:
					VerifySeqPtr (cursize, 1);
					sc.MustGetString ();
					stopsound = S_FindSound (sc.String);
					ScriptTemp[cursize++] = SS_CMD_STOPSOUND << 24;
					break;

				case SS_STRING_NOSTOPCUTOFF:
					VerifySeqPtr (cursize, 1);
					stopsound = -2;
					ScriptTemp[cursize++] = SS_CMD_STOPSOUND << 24;
					break;

				case SS_STRING_ATTENUATION:
					VerifySeqPtr (cursize, 1);
					sc.MustGetString ();
					ScriptTemp[cursize++] = MakeCommand (SS_CMD_ATTENUATION,
						sc.MustMatchString (Attenuations));
					break;

				case SS_STRING_END:
					Sequences[curseq] = (sndseq_t *)Z_Malloc (sizeof(sndseq_t) + sizeof(int)*cursize, PU_STATIC, 0);
					strcpy (Sequences[curseq]->name, name);
					memcpy (Sequences[curseq]->script, ScriptTemp, sizeof(int)*cursize);
					Sequences[curseq]->script[cursize] = SS_CMD_END;
					Sequences[curseq]->stopsound = stopsound;
					curseq = -1;
					break;

				case SS_STRING_PLATFORM:
					AssignTranslations (curseq, SEQ_PLATFORM);
					break;

				case SS_STRING_DOOR:
					AssignTranslations (curseq, SEQ_DOOR);
					break;

				case SS_STRING_ENVIRONMENT:
					AssignTranslations (curseq, SEQ_ENVIRONMENT);
					break;
			}
		}
		sc.Close ();
	}

	free (ScriptTemp);

	if (HexenHack)
		AssignHexenTranslations ();
}

DSeqNode::~DSeqNode ()
{
	if (SequenceListHead == this)
		SequenceListHead = m_Next;
	if (m_Prev)
		m_Prev->m_Next = m_Next;
	if (m_Next)
		m_Next->m_Prev = m_Prev;
	ActiveSequences--;
}

DSeqNode::DSeqNode (int sequence)
{
	ActivateSequence (sequence);
	if (!SequenceListHead)
	{
		SequenceListHead = this;
		m_Next = m_Prev = NULL;
	}
	else
	{
		SequenceListHead->m_Prev = this;
		m_Next = SequenceListHead;
		SequenceListHead = this;
		m_Prev = NULL;
	}
}

void DSeqNode::ActivateSequence (int sequence)
{
	m_SequencePtr = Sequences[sequence]->script;
	m_Sequence = sequence;
	m_DelayTics = 0;
	m_StopSound = Sequences[sequence]->stopsound;
	m_CurrentSoundID = -1;
	m_Volume = 1;			// Start at max volume...
	m_Atten = ATTN_IDLE;	// ...and idle attenuation

	ActiveSequences++;
}

DSeqActorNode::DSeqActorNode (AActor *actor, int sequence)
	: DSeqNode (sequence)
{
	m_Actor = actor;
}

DSeqPolyNode::DSeqPolyNode (polyobj_t *poly, int sequence)
	: DSeqNode (sequence)
{
	m_Poly = poly;
}

DSeqSectorNode::DSeqSectorNode (sector_t *sec, int sequence)
	: DSeqNode (sequence)
{
	m_Sector = sec;
}

//==========================================================================
//
//  SN_StartSequence
//
//==========================================================================

static bool TwiddleSeqNum (int &sequence, seqtype_t type)
{
	if (type < SEQ_NUMSEQTYPES)
		sequence = SeqTrans[sequence + type * 64];

	// [SL] 2011-05-29 - Prevent a segfault if passed a bad sequence number
	// perhaps this should produce an error msg instead
	if (sequence >= NumSequences || sequence < 0)
		return false;

	if (sequence == -1 || Sequences[sequence] == NULL)
		return false;
	else
		return true;
}

void SN_StartSequence (AActor *actor, int sequence, seqtype_t type)
{
	SN_StopSequence (actor); // Stop any previous sequence
	if (TwiddleSeqNum (sequence, type))
		new DSeqActorNode (actor, sequence);
}

void SN_StartSequence (sector_t *sector, int sequence, seqtype_t type)
{
	SN_StopSequence (sector);
	if (TwiddleSeqNum (sequence, type))
		new DSeqSectorNode (sector, sequence);
}

void SN_StartSequence (polyobj_t *poly, int sequence, seqtype_t type)
{
	SN_StopSequence (poly);
	if (TwiddleSeqNum (sequence, type))
		new DSeqPolyNode (poly, sequence);
}

//==========================================================================
//
//  SN_StartSequence (named)
//
//==========================================================================

void SN_StartSequence (AActor *actor, const char *name)
{
	int i;

	for (i = 0; i < NumSequences; i++)
	{
		if (!stricmp (name, Sequences[i]->name))
		{
			SN_StartSequence (actor, i, SEQ_NOTRANS);
			return;
		}
	}
}

void SN_StartSequence (sector_t *sec, const char *name)
{
	int i;

	for (i = 0; i < NumSequences; i++)
	{
		if (!stricmp (name, Sequences[i]->name))
		{
			SN_StartSequence (sec, i, SEQ_NOTRANS);
			return;
		}
	}
}

void SN_StartSequence (polyobj_t *poly, const char *name)
{
	int i;

	for (i = 0; i < NumSequences; i++)
	{
		if (!stricmp (name, Sequences[i]->name))
		{
			SN_StartSequence (poly, i, SEQ_NOTRANS);
			return;
		}
	}
}

//==========================================================================
//
//  SN_StopSequence
//
//==========================================================================

void SN_StopSequence (AActor *actor)
{
	SN_DoStop (actor);
}

void SN_StopSequence (sector_t *sector)
{
	SN_DoStop (sector);
}

void SN_StopSequence (polyobj_t *poly)
{
	SN_DoStop (poly);
}

void SN_DoStop (void *source)
{
	DSeqNode *node;

	for (node = DSeqNode::FirstSequence (); node; )
	{
		DSeqNode *next = node->NextSequence();
		if (node->Source() == source)
		{
			node->Destroy ();
		}
		node = next;
	}
}

DSeqActorNode::~DSeqActorNode ()
{
	if (m_StopSound >= -1)
		S_StopSound (m_Actor, CHAN_BODY);
	if (m_StopSound >= 0)
		S_SoundID (m_Actor, CHAN_BODY, m_StopSound, m_Volume, m_Atten);
}

DSeqSectorNode::~DSeqSectorNode ()
{
	if (m_StopSound >= -1)
		S_StopSound (m_Sector->soundorg, CHAN_BODY);
	if (m_StopSound >= 0)
		S_SoundID (m_Sector->soundorg, CHAN_BODY, m_StopSound, m_Volume, m_Atten);
}

DSeqPolyNode::~DSeqPolyNode ()
{
	if (m_StopSound >= -1)
		S_StopSound (m_Poly->startSpot, CHAN_BODY);
	if (m_StopSound >= 0)
		S_SoundID (m_Poly->startSpot, CHAN_BODY, m_StopSound, m_Volume, m_Atten);
}

//==========================================================================
//
//  SN_UpdateActiveSequences
//
//==========================================================================

void DSeqNode::RunThink ()
{
	if (m_DelayTics > 0)
	{
		m_DelayTics--;
		return;
	}
	bool sndPlaying = IsPlaying ();
	if (m_DelayTics < 0 && sndPlaying)
	{
		m_DelayTics++;
		return;
	}
	switch (GetCommand(*m_SequencePtr))
	{
	case SS_CMD_PLAY:
		if (!sndPlaying)
		{
			m_CurrentSoundID = GetData(*m_SequencePtr);
			MakeSound ();
		}
		m_SequencePtr++;
		break;

	case SS_CMD_WAITUNTILDONE:
		if (!sndPlaying)
		{
			m_SequencePtr++;
			m_CurrentSoundID = -1;
		}
		break;

	case SS_CMD_PLAYREPEAT:
		if (!sndPlaying)
		{
			// Does not advance sequencePtr, so it will repeat
			// as necessary
			m_CurrentSoundID = GetData(*m_SequencePtr);
			MakeLoopedSound ();
		}
		break;

	case SS_CMD_PLAYLOOP:
		m_CurrentSoundID = GetData(*m_SequencePtr);
		MakeLoopedSound ();
		m_DelayTics = -(signed)GetData(*(m_SequencePtr+1));
		break;

	case SS_CMD_DELAY:
		m_DelayTics = GetData(*m_SequencePtr);
		m_SequencePtr++;
		m_CurrentSoundID = -1;
		break;

	case SS_CMD_DELAYRAND:
		m_DelayTics = GetData(*m_SequencePtr)+
			M_Random()%(*(m_SequencePtr+1)-GetData(*m_SequencePtr));
		m_SequencePtr += 2;
		m_CurrentSoundID = -1;
		break;

	case SS_CMD_VOLUME:
		// volume is in range 0..100
		m_Volume = GetData(*m_SequencePtr)/100;
		m_SequencePtr++;
		break;

	case SS_CMD_STOPSOUND:
		// Wait until something else stops the sequence
		break;

	case SS_CMD_ATTENUATION:
		m_Atten = GetData(*m_SequencePtr);
		m_SequencePtr++;
		break;

	case SS_CMD_END:
		Destroy ();
		break;

	default:	
		break;
	}
}

void SN_UpdateActiveSequences (void)
{
	DSeqNode *node;

	if (!ActiveSequences || paused)
	{ // No sequences currently playing/game is paused
		return;
	}
	for (node = DSeqNode::FirstSequence(); node; node = node->NextSequence())
	{
		node->RunThink ();
	}
}

//==========================================================================
//
//  SN_StopAllSequences
//
//==========================================================================

void SN_StopAllSequences (void)
{
	DSeqNode *node;

	for (node = DSeqNode::FirstSequence(); node; )
	{
		DSeqNode *next = node->NextSequence();
		node->m_StopSound = -1; // don't play any stop sounds
		node->Destroy ();
		node = next;
	}
}
	
//==========================================================================
//
//  SN_GetSequenceOffset
//
//==========================================================================

ptrdiff_t SN_GetSequenceOffset (int sequence, unsigned int *sequencePtr)
{
	return sequencePtr - Sequences[sequence]->script;
}

//==========================================================================
//
//  SN_ChangeNodeData
//
// 	nodeNum zero is the first node
//==========================================================================

void SN_ChangeNodeData (int nodeNum, int seqOffset, int delayTics, float volume,
	int currentSoundID)
{
	int i;
	DSeqNode *node;

	i = 0;
	node = DSeqNode::FirstSequence();
	while (node && i < nodeNum)
	{
		node = node->NextSequence();
		i++;
	}
	if (!node)
	{ // reached the end of the list before finding the nodeNum-th node
		return;
	}
	node->ChangeData (seqOffset, delayTics, volume, currentSoundID);
}

void DSeqNode::ChangeData (int seqOffset, int delayTics, float volume, int currentSoundID)
{
	m_DelayTics = delayTics;
	m_Volume = volume;
	m_SequencePtr += seqOffset;
	m_CurrentSoundID = currentSoundID;
}

VERSION_CONTROL (s_sndseq_cpp, "$Id$")
