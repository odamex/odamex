

// for sixaxis stuff

#include <cstring>
#include <switch.h>
#include "nx_io.h"
#include "doomkeys.h"
#include "i_input.h"

void NX_InitializeKeyNameTable(void)
{
	key_names[KEY_JOY1] = "(A)";
	key_names[KEY_JOY2] = "(B)";
	key_names[KEY_JOY3] = "(X)";
	key_names[KEY_JOY4] = "(Y)";
	key_names[KEY_JOY5] = "LNUB";
	key_names[KEY_JOY6] = "RNUB";
	key_names[KEY_JOY7] = "LSHOULDER";
	key_names[KEY_JOY8] = "RSHOULDER";
	key_names[KEY_JOY9] = "LTRIGGER";
	key_names[KEY_JOY10] = "RTRIGGER";
	key_names[KEY_JOY11] = "(+)";
	key_names[KEY_JOY12] = "(-)";
	key_names[KEY_JOY13] = "LEFT";
	key_names[KEY_JOY14] = "UP";
	key_names[KEY_JOY15] = "RIGHT";
	key_names[KEY_JOY16] = "DOWN";
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