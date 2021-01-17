

// for sixaxis stuff

#include <cstring>
#include <switch.h>
#include "nx_io.h"
#include "doomkeys.h"
#include "i_input.h"

void NX_InitializeKeyNameTable(void)
{
	key_names[OKEY_JOY1] = "(B)";
	key_names[OKEY_JOY2] = "(A)";
	key_names[OKEY_JOY3] = "(Y)";
	key_names[OKEY_JOY4] = "(X)";
	key_names[OKEY_JOY5] = "(-)";
	key_names[OKEY_JOY7] = "(+)";
	key_names[OKEY_JOY8] = "LNUB";
	key_names[OKEY_JOY9] = "RNUB";
	key_names[OKEY_JOY10] = "LSHOULDER";
	key_names[OKEY_JOY11] = "RSHOULDER";
	key_names[OKEY_JOY12] = "UP";
	key_names[OKEY_JOY13] = "DOWN";
	key_names[OKEY_JOY14] = "LEFT";
	key_names[OKEY_JOY15] = "RIGHT";
	key_names[OKEY_JOY20] = "LTRIGGER";
	key_names[OKEY_JOY21] = "RTRIGGER";
}

void NX_SetKeyboard(char *out, int out_len)
{
	SwkbdConfig kbd;
	char tmp_out[out_len + 1];
	Result rc;
	tmp_out[0] = 0;
	rc = swkbdCreate(&kbd, 0);
	if (R_SUCCEEDED(rc)) {
		swkbdConfigMakePresetDefault(&kbd);
		swkbdConfigSetInitialText(&kbd, out);
		rc = swkbdShow(&kbd, tmp_out, out_len);
		if (R_SUCCEEDED(rc))
			strncpy(out, tmp_out, out_len); 
		swkbdClose(&kbd);
	}
}