# Microsoft Developer Studio Project File - Name="server" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=server - Win32 DebugConsole
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "server.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "server.mak" CFG="server - Win32 DebugConsole"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "server - Win32 DebugConsole" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "server___Win32_DebugConsole"
# PROP BASE Intermediate_Dir "server___Win32_DebugConsole"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Gr /MTd /W3 /GX /ZI /Od /I "." /I "..\src" /I "..\..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "NOASM" /FR /Fp"Debug/client.pch" /YX /FD /c
# ADD CPP /nologo /G5 /Gr /MTd /W3 /GX /ZI /Od /I "." /I "..\src" /I "..\..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "NOASM" /D "_CONSOLE" /FR /Fp"Debug/client.pch" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcmt" /out:"..\..\dbg\odmserv.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcmt" /out:"..\..\dbg\odmserv.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "server - Win32 DebugConsole"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\c_cmds.cpp
# End Source File
# Begin Source File

SOURCE=..\src\c_console.cpp
# End Source File
# Begin Source File

SOURCE=..\src\c_varinit.cpp
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

SOURCE=..\src\doomstat.cpp
# End Source File
# Begin Source File

SOURCE=..\src\g_game.cpp
# End Source File
# Begin Source File

SOURCE=..\src\g_level.cpp
# End Source File
# Begin Source File

SOURCE=..\src\i_main.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\src\i_system.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\src\m_misc.cpp
# End Source File
# Begin Source File

SOURCE=..\src\p_interaction.cpp
# End Source File
# Begin Source File

SOURCE=..\src\p_mobj.cpp
# End Source File
# Begin Source File

SOURCE=..\src\p_pspr.cpp
# End Source File
# Begin Source File

SOURCE=..\src\p_user.cpp
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

SOURCE=..\src\r_sky.cpp
# End Source File
# Begin Source File

SOURCE=..\src\r_things.cpp
# End Source File
# Begin Source File

SOURCE=..\src\s_sound.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sv_ctf.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sv_main.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sv_master.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sv_rproto.cpp
# End Source File
# Begin Source File

SOURCE=..\src\v_palette.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\c_console.h
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

SOURCE=..\src\g_game.h
# End Source File
# Begin Source File

SOURCE=..\src\g_level.h
# End Source File
# Begin Source File

SOURCE=..\src\i_system.h
# End Source File
# Begin Source File

SOURCE=..\src\m_misc.h
# End Source File
# Begin Source File

SOURCE=..\src\p_local.h
# End Source File
# Begin Source File

SOURCE=..\src\p_mobj.h
# End Source File
# Begin Source File

SOURCE=\Projects\odamex2\trunk\server\src\r_main.h
# End Source File
# Begin Source File

SOURCE=..\src\s_sound.h
# End Source File
# Begin Source File

SOURCE=..\src\st_stuff.h
# End Source File
# Begin Source File

SOURCE=..\src\sv_ctf.h
# End Source File
# Begin Source File

SOURCE=..\src\sv_main.h
# End Source File
# Begin Source File

SOURCE=..\src\sv_master.h
# End Source File
# Begin Source File

SOURCE=..\src\v_palette.h
# End Source File
# Begin Source File

SOURCE=..\src\v_text.h
# End Source File
# End Group
# Begin Group "Assembly Files"

# PROP Default_Filter ""
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
# Begin Group "Win32 Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\i_main.cpp
# End Source File
# Begin Source File

SOURCE=.\i_system.cpp
# End Source File
# Begin Source File

SOURCE=.\i_system.h
# End Source File
# Begin Source File

SOURCE=.\odmserv.ico
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\server.rc
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
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

SOURCE=..\..\common\lzodefs.h
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

SOURCE=..\..\common\m_fixed.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_fixed.h
# End Source File
# Begin Source File

SOURCE=..\..\common\m_random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\m_random.h
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

SOURCE=..\..\common\p_pspr.h
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

SOURCE=..\..\common\p_xlat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\r_bsp.h
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

SOURCE=..\..\common\r_local.h
# End Source File
# Begin Source File

SOURCE=..\..\common\r_segs.h
# End Source File
# Begin Source File

SOURCE=..\..\common\r_state.h
# End Source File
# Begin Source File

SOURCE=..\..\common\r_things.h
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
# Begin Source File

SOURCE=.\odaserv.ico
# End Source File
# End Target
# End Project
