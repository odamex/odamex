

// for sixaxis stuff

#include <cstring>
#include <switch.h>
#include "nx_io.h"

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