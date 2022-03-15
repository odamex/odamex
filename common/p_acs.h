// p_acs.h: ACS Script Stuff

#pragma once

#include "dobject.h"
#include "r_defs.h"

#define LOCAL_SIZE	20
#define STACK_SIZE 4096

struct ScriptPtr
{
	WORD Number;
	BYTE Type;
	BYTE ArgCount;
	DWORD Address;
};

struct ScriptPtr1
{
	WORD Number;
	WORD Type;
	DWORD Address;
	DWORD ArgCount;
};

struct ScriptPtr2
{
	DWORD Number;	// Type is Number / 1000
	DWORD Address;
	DWORD ArgCount;
};

struct ScriptFunction
{
	BYTE ArgCount;
	BYTE LocalCount;
	BYTE HasReturnValue;
	BYTE Pad;
	DWORD Address;
};

enum
{
	SCRIPT_Closed		= 0,
	SCRIPT_Open			= 1,
	SCRIPT_Respawn		= 2,
	SCRIPT_Death		= 3,
	SCRIPT_Enter		= 4,
	SCRIPT_Pickup		= 5,
	SCRIPT_T1Return		= 6,
	SCRIPT_T2Return		= 7,
	SCRIPT_Lightning	= 12,
	SCRIPT_Disconnect	= 14,
};

enum ACSFormat { ACS_Old, ACS_Enhanced, ACS_LittleEnhanced, ACS_Unknown };


class FBehavior
{
public:
	FBehavior (BYTE *object, int len);
	~FBehavior ();

	bool IsGood ();
	BYTE *FindChunk (DWORD id) const;
	BYTE *NextChunk (BYTE *chunk) const;
	int *FindScript (int number) const;
	void PrepLocale (DWORD userpref, DWORD userdef, DWORD syspref, DWORD sysdef);
	const char *LookupString (DWORD index, DWORD ofs=0) const;
	const char *LocalizeString (DWORD index) const;
	void StartTypedScripts (WORD type, AActor *activator, int arg0=0, int arg1=0, int arg2=0, bool always = true) const;
	DWORD PC2Ofs (int *pc) const { return (BYTE *)pc - Data; }
	int *Ofs2PC (DWORD ofs) const { return (int *)(Data + ofs); }
	ACSFormat GetFormat() const { return Format; }
	ScriptFunction *GetFunction (int funcnum) const;
	int GetArrayVal (int arraynum, int index) const;
	void SetArrayVal (int arraynum, int index, int value);

private:
	struct ArrayInfo;

	ACSFormat Format;

	BYTE *Data;
	int DataSize;
	BYTE *Chunks;
	BYTE *Scripts;
	int NumScripts;
	BYTE *Functions;
	int NumFunctions;
	ArrayInfo *Arrays;
	int NumArrays;
	DWORD LanguageNeutral;
	DWORD Localized;

	static int STACK_ARGS SortScripts (const void *a, const void *b);
	void AddLanguage (DWORD lang);
	DWORD FindLanguage (DWORD lang, bool ignoreregion) const;
	DWORD *CheckIfInList (DWORD lang);
};

class DLevelScript : public DObject
{
	DECLARE_SERIAL (DLevelScript, DObject)
public:

	// P-codes for ACS scripts
	enum
	{
/*  0*/	PCD_NOP,
		PCD_TERMINATE,
		PCD_SUSPEND,
		PCD_PUSHNUMBER,
		PCD_LSPEC1,
		PCD_LSPEC2,
		PCD_LSPEC3,
		PCD_LSPEC4,
		PCD_LSPEC5,
		PCD_LSPEC1DIRECT,
/* 10*/	PCD_LSPEC2DIRECT,
		PCD_LSPEC3DIRECT,
		PCD_LSPEC4DIRECT,
		PCD_LSPEC5DIRECT,
		PCD_ADD,
		PCD_SUBTRACT,
		PCD_MULTIPLY,
		PCD_DIVIDE,
		PCD_MODULUS,
		PCD_EQ,
/* 20*/ PCD_NE,
		PCD_LT,
		PCD_GT,
		PCD_LE,
		PCD_GE,
		PCD_ASSIGNSCRIPTVAR,
		PCD_ASSIGNMAPVAR,
		PCD_ASSIGNWORLDVAR,
		PCD_PUSHSCRIPTVAR,
		PCD_PUSHMAPVAR,
/* 30*/	PCD_PUSHWORLDVAR,
		PCD_ADDSCRIPTVAR,
		PCD_ADDMAPVAR,
		PCD_ADDWORLDVAR,
		PCD_SUBSCRIPTVAR,
		PCD_SUBMAPVAR,
		PCD_SUBWORLDVAR,
		PCD_MULSCRIPTVAR,
		PCD_MULMAPVAR,
		PCD_MULWORLDVAR,
/* 40*/	PCD_DIVSCRIPTVAR,
		PCD_DIVMAPVAR,
		PCD_DIVWORLDVAR,
		PCD_MODSCRIPTVAR,
		PCD_MODMAPVAR,
		PCD_MODWORLDVAR,
		PCD_INCSCRIPTVAR,
		PCD_INCMAPVAR,
		PCD_INCWORLDVAR,
		PCD_DECSCRIPTVAR,
/* 50*/	PCD_DECMAPVAR,
		PCD_DECWORLDVAR,
		PCD_GOTO,
		PCD_IFGOTO,
		PCD_DROP,
		PCD_DELAY,
		PCD_DELAYDIRECT,
		PCD_RANDOM,
		PCD_RANDOMDIRECT,
		PCD_THINGCOUNT,
/* 60*/	PCD_THINGCOUNTDIRECT,
		PCD_TAGWAIT,
		PCD_TAGWAITDIRECT,
		PCD_POLYWAIT,
		PCD_POLYWAITDIRECT,
		PCD_CHANGEFLOOR,
		PCD_CHANGEFLOORDIRECT,
		PCD_CHANGECEILING,
		PCD_CHANGECEILINGDIRECT,
		PCD_RESTART,
/* 70*/	PCD_ANDLOGICAL,
		PCD_ORLOGICAL,
		PCD_ANDBITWISE,
		PCD_ORBITWISE,
		PCD_EORBITWISE,
		PCD_NEGATELOGICAL,
		PCD_LSHIFT,
		PCD_RSHIFT,
		PCD_UNARYMINUS,
		PCD_IFNOTGOTO,
/* 80*/	PCD_LINESIDE,
		PCD_SCRIPTWAIT,
		PCD_SCRIPTWAITDIRECT,
		PCD_CLEARLINESPECIAL,
		PCD_CASEGOTO,
		PCD_BEGINPRINT,
		PCD_ENDPRINT,
		PCD_PRINTSTRING,
		PCD_PRINTNUMBER,
		PCD_PRINTCHARACTER,
/* 90*/	PCD_PLAYERCOUNT,
		PCD_GAMETYPE,
		PCD_GAMESKILL,
		PCD_TIMER,
		PCD_SECTORSOUND,
		PCD_AMBIENTSOUND,
		PCD_SOUNDSEQUENCE,
		PCD_SETLINETEXTURE,
		PCD_SETLINEBLOCKING,
		PCD_SETLINESPECIAL,
/*100*/	PCD_THINGSOUND,
		PCD_ENDPRINTBOLD,		// [RH] End of Hexen p-codes
		PCD_ACTIVATORSOUND,
		PCD_LOCALAMBIENTSOUND,
		PCD_SETLINEMONSTERBLOCKING,
		PCD_PLAYERBLUESKULL,	// [BC] Start of new [Skull Tag] pcodes
		PCD_PLAYERREDSKULL,
		PCD_PLAYERYELLOWSKULL,
		PCD_PLAYERMASTERSKULL,
		PCD_PLAYERBLUECARD,
/*110*/	PCD_PLAYERREDCARD,
		PCD_PLAYERYELLOWCARD,
		PCD_PLAYERMASTERCARD,
		PCD_PLAYERBLACKSKULL,
		PCD_PLAYERSILVERSKULL,
		PCD_PLAYERGOLDSKULL,
		PCD_PLAYERBLACKCARD,
		PCD_PLAYERSILVERCARD,
		PCD_PLAYERGOLDCARD,
		PCD_PLAYERTEAM1,
/*120*/	PCD_PLAYERHEALTH,
		PCD_PLAYERARMORPOINTS,
		PCD_PLAYERFRAGS,
		PCD_PLAYEREXPERT,
		PCD_TEAM1COUNT,
		PCD_TEAM2COUNT,
		PCD_TEAM1SCORE,
		PCD_TEAM2SCORE,
		PCD_TEAM1FRAGPOINTS,
		PCD_LSPEC6,				// These are never used. They should probably
/*130*/	PCD_LSPEC6DIRECT,		// be given names like PCD_DUMMY.
		PCD_PRINTNAME,
		PCD_MUSICCHANGE,
		PCD_TEAM2FRAGPOINTS,
		PCD_CONSOLECOMMAND,
		PCD_SINGLEPLAYER,		// [RH] End of Skull Tag p-codes
		PCD_FIXEDMUL,
		PCD_FIXEDDIV,
		PCD_SETGRAVITY,
		PCD_SETGRAVITYDIRECT,
/*140*/	PCD_SETAIRCONTROL,
		PCD_SETAIRCONTROLDIRECT,
		PCD_CLEARINVENTORY,
		PCD_GIVEINVENTORY,
		PCD_GIVEINVENTORYDIRECT,
		PCD_TAKEINVENTORY,
		PCD_TAKEINVENTORYDIRECT,
		PCD_CHECKINVENTORY,
		PCD_CHECKINVENTORYDIRECT,
		PCD_SPAWN,
/*150*/	PCD_SPAWNDIRECT,
		PCD_SPAWNSPOT,
		PCD_SPAWNSPOTDIRECT,
		PCD_SETMUSIC,
		PCD_SETMUSICDIRECT,
		PCD_LOCALSETMUSIC,
		PCD_LOCALSETMUSICDIRECT,
		PCD_PRINTFIXED,
		PCD_PRINTLOCALIZED,
		PCD_MOREHUDMESSAGE,
/*160*/	PCD_OPTHUDMESSAGE,
		PCD_ENDHUDMESSAGE,
		PCD_ENDHUDMESSAGEBOLD,
		PCD_SETSTYLE,
		PCD_SETSTYLEDIRECT,
		PCD_SETFONT,
		PCD_SETFONTDIRECT,
		PCD_PUSHBYTE,
		PCD_LSPEC1DIRECTB,
		PCD_LSPEC2DIRECTB,
/*170*/	PCD_LSPEC3DIRECTB,
		PCD_LSPEC4DIRECTB,
		PCD_LSPEC5DIRECTB,
		PCD_DELAYDIRECTB,
		PCD_RANDOMDIRECTB,
		PCD_PUSHBYTES,
		PCD_PUSH2BYTES,
		PCD_PUSH3BYTES,
		PCD_PUSH4BYTES,
		PCD_PUSH5BYTES,
/*180*/	PCD_SETTHINGSPECIAL,
		PCD_ASSIGNGLOBALVAR,
		PCD_PUSHGLOBALVAR,
		PCD_ADDGLOBALVAR,
		PCD_SUBGLOBALVAR,
		PCD_MULGLOBALVAR,
		PCD_DIVGLOBALVAR,
		PCD_MODGLOBALVAR,
		PCD_INCGLOBALVAR,
		PCD_DECGLOBALVAR,
/*190*/	PCD_FADETO,
		PCD_FADERANGE,
		PCD_CANCELFADE,
		PCD_PLAYMOVIE,
		PCD_SETFLOORTRIGGER,
		PCD_SETCEILINGTRIGGER,
		PCD_GETACTORX,
		PCD_GETACTORY,
		PCD_GETACTORZ,
		PCD_STARTTRANSLATION,
/*200*/	PCD_TRANSLATIONRANGE1,
		PCD_TRANSLATIONRANGE2,
		PCD_ENDTRANSLATION,
		PCD_CALL,
		PCD_CALLDISCARD,
		PCD_RETURNVOID,
		PCD_RETURNVAL,
		PCD_PUSHMAPARRAY,
		PCD_ASSIGNMAPARRAY,
		PCD_ADDMAPARRAY,
/*210*/	PCD_SUBMAPARRAY,
		PCD_MULMAPARRAY,
		PCD_DIVMAPARRAY,
		PCD_MODMAPARRAY,
		PCD_INCMAPARRAY,
		PCD_DECMAPARRAY,
		PCD_DUP,
		PCD_SWAP,
		PCD_WRITETOINI,
		PCD_GETFROMINI,
/*220*/ PCD_SIN,
		PCD_COS,
		PCD_VECTORANGLE,
			
// v THESE ARE UNUSED YET
		PCD_CHECKWEAPON,
		PCD_SETWEAPON,
		PCD_TAGSTRING,
		PCD_PUSHWORLDARRAY,
		PCD_ASSIGNWORLDARRAY,
		PCD_ADDWORLDARRAY,
		PCD_SUBWORLDARRAY,
/*230*/	PCD_MULWORLDARRAY,
		PCD_DIVWORLDARRAY,
		PCD_MODWORLDARRAY,
		PCD_INCWORLDARRAY,
		PCD_DECWORLDARRAY,
		PCD_PUSHGLOBALARRAY,
		PCD_ASSIGNGLOBALARRAY,
		PCD_ADDGLOBALARRAY,
		PCD_SUBGLOBALARRAY,
		PCD_MULGLOBALARRAY,
/*240*/	PCD_DIVGLOBALARRAY,
		PCD_MODGLOBALARRAY,
		PCD_INCGLOBALARRAY,
		PCD_DECGLOBALARRAY,
		PCD_SETMARINEWEAPON,
		PCD_SETACTORPROPERTY,
		PCD_GETACTORPROPERTY,
	  // ^ THESE ARE UNUSED YET

		PCD_PLAYERNUMBER,
		PCD_ACTIVATORTID,
		PCD_GETCVAR = 255,
	  /*260*/ PCD_GETACTORANGLE = 260,
	  PCD_GETLEVELINFO = 265,

		PCODE_COMMAND_COUNT
	};


	static void ACS_SetLineTexture(int* args, byte argCount);
	static void ACS_ClearInventory(AActor* actor);
	static void ACS_Print(byte pcd, AActor* actor, const char* print);
	static void ACS_ChangeMusic(byte pcd, AActor* activator, int* args, byte argCount);
	static void ACS_StartSound(byte pcd, AActor* activator, int* args, byte argCount);
	static void ACS_SetLineBlocking(int* args, byte argCount);
	static void ACS_SetLineMonsterBlocking(int* args, byte argCount);
	static void ACS_SetLineSpecial(int* args, byte argCount);
	static void ACS_SetThingSpecial(int* args, byte argCount);
	static void ACS_FadeRange(AActor* activator, int* args, byte argCount);
	static void ACS_CancelFade(AActor* activator);
	static void ACS_ChangeFlat(byte pcd, int* args, byte argCount);
	static void ACS_SoundSequence(int* args, byte argCount);

	// Some constants used by ACS scripts
	enum {
		LINE_FRONT =			0,
		LINE_BACK =				1
	};
	enum {
		SIDE_FRONT =			0,
		SIDE_BACK =				1
	};
	enum {
		TEXTURE_TOP =			0,
		TEXTURE_MIDDLE =		1,
		TEXTURE_BOTTOM =		2
	};
	enum {
		GAME_SINGLE_PLAYER =	0,
		GAME_NET_COOPERATIVE =	1,
		GAME_NET_DEATHMATCH =	2,
		GAME_NET_TEAMDEATHMATCH = 3,
		GAME_NET_CTF = 4
	};
	enum {
		CLASS_FIGHTER =			0,
		CLASS_CLERIC =			1,
		CLASS_MAGE =			2
	};
	enum {
		SKILL_VERY_EASY =		0,
		SKILL_EASY =			1,
		SKILL_NORMAL =			2,
		SKILL_HARD =			3,
		SKILL_VERY_HARD =		4
	};
	enum {
		BLOCK_NOTHING =			0,
		BLOCK_CREATURES =		1,
		BLOCK_EVERYTHING =		2
	};

	enum
	{
		LEVELINFO_PAR_TIME,
		LEVELINFO_CLUSTERNUM,
		LEVELINFO_LEVELNUM,
		LEVELINFO_TOTAL_SECRETS,
		LEVELINFO_FOUND_SECRETS,
		LEVELINFO_TOTAL_ITEMS,
		LEVELINFO_FOUND_ITEMS,
		LEVELINFO_TOTAL_MONSTERS,
		LEVELINFO_KILLED_MONSTERS,
		LEVELINFO_SUCK_TIME
	};

	enum EScriptState
	{
		SCRIPT_Running,
		SCRIPT_Suspended,
		SCRIPT_Delayed,
		SCRIPT_TagWait,
		SCRIPT_PolyWait,
		SCRIPT_ScriptWaitPre,
		SCRIPT_ScriptWait,
		SCRIPT_PleaseRemove,
		SCRIPT_DivideBy0,
		SCRIPT_ModulusBy0,
	};

	DLevelScript (AActor *who, line_t *where, int num, int *code,
		int lineSide, int arg0, int arg1, int arg2, int always, bool delay);

	void RunScript ();

	inline void SetState (EScriptState newstate) { state = newstate; }
	inline EScriptState GetState () { return state; }

	void *operator new (size_t size);
	void operator delete (void *block);

protected:
	DLevelScript	*next, *prev;
	int				script;
	int				sp;
	int				localvars[LOCAL_SIZE];
	int				*pc;
	EScriptState	state;
	int				statedata;
	AActor			*activator;
	line_t			*activationline;
	int				lineSide;
	int				stringstart;

	inline void PushToStack (int val);

	void Link ();
	void Unlink ();
	void PutLast ();
	void PutFirst ();
	static int Random (int min, int max);
	static int ThingCount (int type, int tid);
	static void ChangeFlat (int tag, int name, bool floorOrCeiling);
	static int CountPlayers ();
	static void SetLineTexture (int lineid, int side, int position, int name);

	static int DoSpawn (int type, fixed_t x, fixed_t y, fixed_t z, int tid, int angle);
	static int DoSpawnSpot (int type, int spot, int tid, int angle);

	static void SetLineBlocking(int lineid, int flags);
	static void SetLineMonsterBlocking(int lineid, int toggle);
	static void SetLineSpecial(int lineid, int special, int arg1, int arg2, int arg3, int arg4, int arg5);
	static void ActivateLineSpecial(byte special, line_t* line, AActor* activator,
	                                int arg0, int arg1, int arg2, int arg3, int arg4);
	static void ChangeMusic(byte pcd, AActor* activator, int index, int loop);
	static void StartSound(byte pcd, AActor* activator, int channel, int index, int volume, int attenuation);
	static void StartSectorSound(byte pcd, sector_t* sector, int channel, int index, int volume, int attenuation);
	static void StartThingSound(byte pcd, AActor* actor, int channel, int index, int volume, int attenuation);
	static void SetThingSpecial(AActor* actor, int special, int arg1, int arg2, int arg3, int arg4, int arg5);
	static void CancelFade(AActor* actor);
	static void StartSoundSequence(sector_t* sec, int index);

	static void DoFadeTo (AActor* who, int r, int g, int b, int a, fixed_t time);
	static void DoFadeRange (AActor* who, int r1, int g1, int b1, int a1,
		int r2, int g2, int b2, int a2, fixed_t time);

private:
	DLevelScript ();

	friend class DACSThinker;
};

inline FArchive &operator<< (FArchive &arc, DLevelScript::EScriptState state)
{
	return arc << (BYTE)state;
}
inline FArchive &operator>> (FArchive &arc, DLevelScript::EScriptState &state)
{
	BYTE in; arc >> in; state = (DLevelScript::EScriptState)in; return arc;
}

class DACSThinker : public DThinker
{
	DECLARE_SERIAL (DACSThinker, DThinker)
public:
	DACSThinker ();
	~DACSThinker ();

	void RunThink ();

	DLevelScript *RunningScripts[1000];	// Array of all synchronous scripts
	static DACSThinker *ActiveThinker;

    void DumpScriptStatus();

private:
	DLevelScript *LastScript;
	DLevelScript *Scripts;				// List of all running scripts

	friend class DLevelScript;
};

// The structure used to control scripts between maps
struct acsdefered_s
{
	struct acsdefered_s *next;

	enum EType
	{
		defexecute,
		defexealways,
		defsuspend,
		defterminate
	} type;
	int script;
	int arg0, arg1, arg2;
	int playernum;
};
typedef struct acsdefered_s acsdefered_t;


FArchive &operator<< (FArchive &arc, acsdefered_s *defer);
FArchive &operator>> (FArchive &arc, acsdefered_s* &defer);
