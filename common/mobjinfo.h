#pragma once

#include "info.h" // mobjinfo_t

void D_Initialize_Mobjinfo(mobjinfo_t* source, int count, mobjtype_t start);
void D_EnsureMobjInfoCapacity(int limit);
