#pragma once

//
// CHEAT SEQUENCE PACKAGE
//

struct cheatseq_t
{
	unsigned char*  Sequence;
	unsigned char*  Pos;
	bool            DontCheck;
	bool            AllowInNetdemoPlayback;
	unsigned char   CurrentArg;
	unsigned char   Args[2];
	bool (*Handler)(cheatseq_t*);
};

namespace cheat
{
    // keycheat handlers
    bool AddKey(cheatseq_t* cheat, unsigned char key, bool* eat);

    bool AutoMap    (cheatseq_t* cheat);
    bool ChangeLevel(cheatseq_t* cheat);
    bool IdMyPos    (cheatseq_t* cheat);
    bool BeholdMenu (cheatseq_t* cheat);
    bool ChangeMusic(cheatseq_t* cheat);
    bool SetGeneric (cheatseq_t* cheat);
}
