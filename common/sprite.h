#pragma once

void D_Initialize_sprnames(const char** source, int count);
int D_FindOrgSpriteIndex(const char** src_sprnames, const char* key);
void D_EnsureSprnamesCapacity(int limit);
