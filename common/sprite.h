#pragma once

void D_Initialize_sprnames(const char** source, int count);
bool D_Backup_sprnames();
bool D_Restore_sprnames();
int D_FindOrgSpriteIndex(const char** src_sprnames, const char* key);
void D_EnsureSprnamesCapacity(int limit);
