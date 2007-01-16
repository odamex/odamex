!define INSTALLPATH $R0 ;

Name "Odamex Installer"

OutFile "odamex-installer.exe"

Page custom ChooseDir

Page instfiles

Section "AskPath"

ReadINIStr ${INSTALLPATH} "$PLUGINSDIR\installpath.ini" "Field 3" "State"

SectionEnd

Function .onInit

InitPluginsDir

File /oname=$PLUGINSDIR\installpath.ini "installpath.ini"
  
FunctionEnd

Function ChooseDir

Push ${INSTALLPATH}

InstallOptions::dialog "$PLUGINSDIR\installpath.ini"

Pop ${INSTALLPATH}
  
Pop ${INSTALLPATH}

FunctionEnd

ShowInstDetails show

Section "Install"

SetOutPath ${INSTALLPATH}

File ..\..\bin\odalaunch.exe

File ..\..\bin\odamex.exe

File ..\..\bin\odasrv.exe

File ..\..\odamex.wad

File ..\..\SDL.dll

File ..\..\SDL_mixer.dll

WriteUninstaller ${INSTALLPATH}\uninstaller.exe

SectionEnd

ShowUninstDetails show

Section "Uninstall"

DeleteRegKey HKEY_CLASSES_ROOT "ods"

Delete $INSTDIR\uninstaller.exe

Delete $INSTDIR\odalaunch.exe

Delete $INSTDIR\odamex.exe

Delete $INSTDIR\odasrv.exe

Delete $INSTDIR\odamex.wad

Delete $INSTDIR\SDL.dll

Delete $INSTDIR\SDL_mixer.dll

SectionEnd