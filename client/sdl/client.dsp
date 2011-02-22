# Microsoft Developer Studio Project File - Name="client" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=client - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "client.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "client.MAK" CFG="client - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "client - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "client - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "client - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /I "." /I "..\src" /I "..\..\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "NOASM" /D "NOMINMAX" /FAs /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib msvcrt.lib msvcprt.lib oldnames.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib sdl.lib sdlmain.lib sdl_mixer.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /nodefaultlib:"libcmt" /nodefaultlib /out:"..\..\bin\odamex.exe"
# SUBTRACT LINK32 /verbose /profile /pdb:none /map /debug

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /I "." /I "..\src" /I "..\..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "NOASM" /D "NOMINMAX" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib msvcrt.lib msvcprt.lib oldnames.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib sdl.lib sdlmain.lib sdl_mixer.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcmt LIBCMTD" /nodefaultlib /out:"..\..\dbg\odamex.exe" /pdbtype:sept
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "client - Win32 Release"
# Name "client - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "c;cpp"
# Begin Source File

SOURCE=..\src\am_map.cpp
# End Source File
# Begin Source File

SOURCE=..\src\c_bind.cpp
# End Source File
# Begin Source File

SOURCE=..\src\c_console.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\c_cvarlist.cpp
# End Source File
# Begin Source File

SOURCE=..\src\cl_ctf.cpp
# End Source File
# Begin Source File

SOURCE=..\src\cl_cvarlist.cpp
# End Source File
# Begin Source File

SOURCE=..\src\cl_main.cpp
# End Source File
# Begin Source File

SOURCE=..\src\cl_pred.cpp
# End Source File
# Begin Source File

SOURCE=..\src\d_main.cpp
# End Source File
# Begin Source File

SOURCE=..\src\d_net.cpp
# End Source File
# Begin Source File

SOURCE=..\src\d_netinfo.cpp
# End Source File
# Begin Source File

SOURCE=..\src\f_finale.cpp
# End Source File
# Begin Source File

SOURCE=..\src\f_wipe.cpp
# End Source File
# Begin Source File

SOURCE=..\src\g_game.cpp
# End Source File
# Begin Source File

SOURCE=..\src\g_level.cpp
# End Source File
# Begin Source File

SOURCE=..\src\hu_stuff.cpp
# End Source File
# Begin Source File

SOURCE=..\src\m_menu.cpp
# End Source File
# Begin Source File

SOURCE=..\src\m_misc.cpp
# End Source File
# Begin Source File

SOURCE=..\src\m_options.cpp
# End Source File
# Begin Source File

SOURCE=..\src\p_interaction.cpp
# End Source File
# Begin Source File

SOURCE=..\src\p_mobj.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_bsp.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_draw.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_main.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_plane.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_segs.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_sky.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_things.cpp
# End Source File
# Begin Source File

SOURCE=..\src\s_sound.cpp
# End Source File
# Begin Source File

SOURCE=..\src\s_sndseq.cpp
# End Source File
# Begin Source File

SOURCE=..\src\st_lib.cpp
# End Source File
# Begin Source File

SOURCE=..\src\st_new.cpp
# End Source File
# Begin Source File

SOURCE=..\src\st_stuff.cpp
# End Source File
# Begin Source File

SOURCE=..\src\v_draw.cpp
# End Source File
# Begin Source File

SOURCE=..\src\v_palette.cpp
# End Source File
# Begin Source File

SOURCE=..\src\v_text.cpp
# End Source File
# Begin Source File

SOURCE=..\src\v_video.cpp
# End Source File
# Begin Source File

SOURCE=..\src\wi_stuff.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\src\am_map.h
# End Source File
# Begin Source File

SOURCE=..\src\c_bind.h
# End Source File
# Begin Source File

SOURCE=..\src\c_console.h
# End Source File
# Begin Source File

SOURCE=..\src\cl_ctf.h
# End Source File
# Begin Source File

SOURCE=..\src\cl_main.h
# End Source File
# Begin Source File

SOURCE=..\src\d_main.h
# End Source File
# Begin Source File

SOURCE=..\src\d_player.h
# End Source File
# Begin Source File

SOURCE=..\src\doomstat.h
# End Source File
# Begin Source File

SOURCE=..\src\f_finale.h
# End Source File
# Begin Source File

SOURCE=..\src\f_wipe.h
# End Source File
# Begin Source File

SOURCE=..\src\g_game.h
# End Source File
# Begin Source File

SOURCE=..\src\g_level.h
# End Source File
# Begin Source File

SOURCE=..\src\hu_stuff.h
# End Source File
# Begin Source File

SOURCE=..\src\m_menu.h
# End Source File
# Begin Source File

SOURCE=..\src\m_misc.h
# End Source File
# Begin Source File

SOURCE=..\src\memio.h
# End Source File
# Begin Source File

SOURCE=..\src\mus2midi.h
# End Source File
# Begin Source File

SOURCE=..\src\p_interaction.h
# End Source File
# Begin Source File

SOURCE=..\src\p_local.h
# End Source File
# Begin Source File

SOURCE=..\src\r_bsp.h
# End Source File
# Begin Source File

SOURCE=..\src\r_draw.h
# End Source File
# Begin Source File

SOURCE=..\src\r_main.h
# End Source File
# Begin Source File

SOURCE=..\src\r_plane.h
# End Source File
# Begin Source File

SOURCE=..\src\r_segs.h
# End Source File
# Begin Source File

SOURCE=..\src\r_sky.h
# End Source File
# Begin Source File

SOURCE=..\src\s_sound.h
# End Source File
# Begin Source File

SOURCE=..\src\s_sndseq.h
# End Source File
# Begin Source File

SOURCE=..\src\st_lib.h
# End Source File
# Begin Source File

SOURCE=..\src\v_palette.h
# End Source File
# Begin Source File

SOURCE=..\src\v_text.h
# End Source File
# Begin Source File

SOURCE=..\src\wi_stuff.h
# End Source File
# End Group
# Begin Group "Assembly Files"

# PROP Default_Filter "nas; asm"
# Begin Source File

SOURCE=..\..\common\linear.nas
# End Source File
# Begin Source File

SOURCE=..\..\common\misc.nas
# End Source File
# Begin Source File

SOURCE=..\..\common\tmap.nas
# End Source File
# End Group
# Begin Group "SDL Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\client.rc
# End Source File
# Begin Source File

SOURCE=.\hardware.cpp
# End Source File
# Begin Source File

SOURCE=.\hardware.h
# End Source File
# Begin Source File

SOURCE=.\i_input.cpp
# End Source File
# Begin Source File

SOURCE=.\i_input.h
# End Source File
# Begin Source File

SOURCE=.\i_main.cpp
# End Source File
# Begin Source File

SOURCE=.\i_music.cpp
# End Source File
# Begin Source File

SOURCE=.\i_music.h
# End Source File
# Begin Source File

SOURCE=.\i_sdlvideo.cpp
# End Source File
# Begin Source File

SOURCE=.\i_sdlvideo.h
# End Source File
# Begin Source File

SOURCE=.\i_sound.cpp
# End Source File
# Begin Source File

SOURCE=.\i_sound.h
# End Source File
# Begin Source File

SOURCE=.\i_system.cpp
# End Source File
# Begin Source File

SOURCE=.\i_system.h
# End Source File
# Begin Source File

SOURCE=.\i_video.h
# End Source File
# Begin Source File

SOURCE=.\mus2midi.cpp
# End Source File
# Begin Source File

SOURCE=.\mus2midi.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter "c;cpp;h;hpp"
# Begin Source File

SOURCE=..\..\common\actor.h
# End Source File
# Begin Source File

SOURCE=..\..\common\c_cvars.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\c_cvars.h
# End Source File
# Begin Source File

SOURCE=..\..\common\c_dispatch.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\c_dispatch.h
# End Source File
# Begin Source File

SOURCE=..\..\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d_dehacked.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d_dehacked.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d_event.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d_items.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d_items.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d_net.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d_netinf.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d_protocol.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d_protocol.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d_ticcmd.h
# End Source File
# Begin Source File

SOURCE=..\..\common\dobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\dobject.h
# End Source File
# Begin Source File

SOURCE=..\..\common\doomdata.h
# End Source File
# Begin Source File

SOURCE=..\..\common\doomdef.h
# End Source File
# Begin Source File

SOURCE=..\..\common\doomstat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\doomtype.h
# End Source File
# Begin Source File

SOURCE=..\..\common\dsectoreffect.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\dsectoreffect.h
# End Source File
# Begin Source File

SOURCE=..\..\common\dstrings.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\dstrings.h
# End Source File
# Begin Source File

SOURCE=..\..\common\dthinker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\dthinker.h
# End Source File
# Begin Source File

SOURCE=..\..\common\errors.h
# End Source File
# Begin Source File

SOURCE=..\..\common\farchive.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\farchive.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\gi.h
# End Source File
# Begin Source File

SOURCE=..\..\common\huffman.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\huffman.h
# End Source File
# Begin Source File

SOURCE=..\..\common\i_net.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\i_net.h
# End Source File
# Begin Source File

SOURCE=..\..\common\info.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\info.h
# End Source File
# Begin Source File

SOURCE=..\..\common\lzoconf.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_alloc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_argv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_argv.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_bbox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_bbox.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_cheat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_cheat.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_fileio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_fileio.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_fixed.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_fixed.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_memio.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_memio.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_random.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_swap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_swap.h
# End Source File
# Begin Source File

SOURCE=..\..\common\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\common\minilzo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\minilzo.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_ceiling.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_doors.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_enemy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_floor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_inter.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_lights.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_lnspec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_lnspec.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_local.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_map.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_maputl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_pillar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_plats.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_pspr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_pspr.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_quake.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_saveg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_saveg.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_setup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_setup.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_sight.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_spec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_spec.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_switch.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_teleport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_tick.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_tick.h
# End Source File
# Begin Source File

SOURCE=..\..\common\p_user.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\p_xlat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\r_data.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\r_data.h
# End Source File
# Begin Source File

SOURCE=..\..\common\r_defs.h
# End Source File
# Begin Source File

SOURCE=..\src\r_drawt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\r_local.h
# End Source File
# Begin Source File

SOURCE=..\..\common\r_state.h
# End Source File
# Begin Source File

SOURCE=..\..\common\r_things.h
# End Source File
# Begin Source File

SOURCE=..\..\common\sc_man.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\sc_man.h
# End Source File
# Begin Source File

SOURCE=..\..\common\st_stuff.h
# End Source File
# Begin Source File

SOURCE=..\..\common\stats.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\stats.h
# End Source File
# Begin Source File

SOURCE=..\..\common\szp.h
# End Source File
# Begin Source File

SOURCE=..\..\common\tables.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\tables.h
# End Source File
# Begin Source File

SOURCE=..\..\common\tarray.h
# End Source File
# Begin Source File

SOURCE=..\..\common\v_video.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vectors.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\vectors.h
# End Source File
# Begin Source File

SOURCE=..\..\common\version.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\version.h
# End Source File
# Begin Source File

SOURCE=..\..\common\w_wad.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\w_wad.h
# End Source File
# Begin Source File

SOURCE=..\..\common\z_zone.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\z_zone.h
# End Source File
# End Group
# Begin Group "Output"

# PROP Default_Filter "out;txt"
# Begin Source File

SOURCE=..\..\dbg\odamex.out
# End Source File
# End Group
# End Target
# End Project
