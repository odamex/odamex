//----------------------------------------------------------------------------------------------
// odamex enum definitions with negative indices (similar to id24 specification) are in info.h
// using negative indices prevents overriding during dehacked (though id24 allows this)
//
// id24 allows 0x80000000-0x8FFFFFFF (abbreviated as 0x8000-0x8FFF) for negative indices for source port implementation
// id24 spec no longer uses enum to define the indices for doom objects, but until that is implemented enums will
// continue to be used
//----------------------------------------------------------------------------------------------

#pragma once
#include "info.h" // doom object definitions - including enums with negative indices

extern state_t odastates[]; //statenum_t 
extern mobjinfo_t odathings[]; //mobjtype_t
extern const char* odasprnames[]; // spritenum_t

state_t* D_GetOdaState(statenum_t statenum);
mobjinfo_t* D_GetOdaMobjinfo(mobjtype_t mobjtype);
const char* D_GetOdaSprName(spritenum_t spritenum);
