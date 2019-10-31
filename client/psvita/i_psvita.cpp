// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// DESCRIPTION:
//	Playstation Vita Support. It includes an unix-like recreation
//	of the network interface.
//
// 	It also has non-basic functions recreation/replacements.
//
//	Credits to Rinnegatamante !
//
// scandir() and alphasort() originally obtained from the viewmol project. They have been slightly modified.
// The original source is located here: 
//		http://viewmol.cvs.sourceforge.net/viewvc/viewmol/source/scandir.c?revision=1.3&view=markup
// The license (GPL) is located here: http://viewmol.sourceforge.net/documentation/node2.html
//
//-----------------------------------------------------------------------------
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <SDL.h>
#include <vitasdk.h>

#include <dirent.h>
#include <unistd.h>

#include "i_psvita.h"
#include "doomtype.h"

#include "debugScreen.h"

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>

#define printf psvDebugScreenPrintf

// External function declarations
extern int I_Main(int argc, char *argv[]); // i_main.cpp

bool vita_pathisrelative(const char *path)
{
	if (path &&
	   (path[0] == PATHSEPCHAR || // /path/to/file
	   (path[0] == '.' && path[1] == PATHSEPCHAR) || // ./file
	   (path[0] == '.' && path[1] == '.' && path[2] == PATHSEPCHAR))) // ../file
		return true;

	return false;
}

//
// vita_scandir - Custom implementation of scandir()
//
int vita_scandir(const char *dir, struct  dirent ***namelist,
									int (*select)(const struct dirent *),
									int (*compar)(const struct dirent **, const struct dirent **))
{
	DIR *d;
	struct dirent *entry;
	register int i=0;
	size_t entrysize;

	if ((d=opendir(dir)) == NULL)
		return(-1);

	*namelist=NULL;
	while ((entry=readdir(d)) != NULL)
	{
		if (select == NULL || (select != NULL && (*select)(entry)))
		{
			*namelist=(struct dirent **)realloc((void *)(*namelist),
								(size_t)((i+1)*sizeof(struct dirent *)));
			if (*namelist == NULL) return(-1);
				entrysize=sizeof(struct dirent)-sizeof(entry->d_name)+strlen(entry->d_name)+1;
			(*namelist)[i]=(struct dirent *)malloc(entrysize);
			if ((*namelist)[i] == NULL) return(-1);
				memcpy((*namelist)[i], entry, entrysize);
			i++;
		}
	}
	if (closedir(d)) return(-1);
	if (i == 0) return(-1);
	if (compar != NULL)
		qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), (int (*)(const void*,const void*))compar);
               
	return(i);
	return 0;
}

//
// vita_alphasort - Custom implementation of alphasort

int vita_alphasort(const struct dirent **a, const struct dirent **b)
{
	//return(strcmp((*a)->d_name, (*b)->d_name));
	return NULL;
}

//-------------------------------

unsigned int vita_sleep(unsigned int seconds)
{
        sceKernelDelayThread(seconds*1000*1000);
        return 0;
}

int vita_usleep(useconds_t usec)
{
        sceKernelDelayThread(usec);
        return 0;
}




int odamex_main (unsigned int argc, void *argv){
	
	// Initializing stuff
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET); 
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	//sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});

	/*sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	SceCommonDialogConfigParam cmnDlgCfgParam;
	sceCommonDialogConfigParamInit(&cmnDlgCfgParam);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&cmnDlgCfgParam.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&cmnDlgCfgParam.enterButtonAssign);
	sceCommonDialogSetConfigParam(&cmnDlgCfgParam);*/

	I_Main(0, NULL); // Does not return

}

int main(int argc, char *argv[])
{
	psvDebugScreenInit();

// We need a bigger stack to run Quake 3, so we create a new thread with a proper stack size
	SceUID main_thread = sceKernelCreateThread("odamex", odamex_main, 0x40, 0x200000, 0, 0, NULL);
	if (main_thread >= 0){
		sceKernelStartThread(main_thread, 0, NULL);
		sceKernelWaitThreadEnd(main_thread, NULL, NULL);
	}

	sceKernelExitProcess(0);
	return 0;
}

