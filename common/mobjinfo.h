#pragma once

#include "info.h" // mobjinfo_t

void D_Initialize_mobjinfo(mobjinfo_t* source, int count);
void D_EnsureMobjInfoCapacity(int limit);
