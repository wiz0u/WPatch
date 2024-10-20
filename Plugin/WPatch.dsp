# Microsoft Developer Studio Project File - Name="WPatch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WPatch - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WPatch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WPatch.mak" CFG="WPatch - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WPatch - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WPatch - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WPatch - Win32 Release UNICODE" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WPatch - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NSIS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib shlwapi.lib /nologo /entry:"DllMain" /dll /debug /machine:I386 /out:"../../bin/WPatch.dll" /OPT:REF /OPT:NOWIN98
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
IntDir=.\Release
TargetDir=\Prog\shared\bin
SOURCE="$(InputPath)"
PostBuild_Desc=Building Sample Installer...
PostBuild_Cmds="%ProgramFiles%\NSIS\makensis" /V2 "/DTARGETDIR=$(TargetDir)" "/DINTDIR=$(IntDir)" "..\Demo\Sample.nsi"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "WPatch - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NSIS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib shlwapi.lib /nologo /entry:"DllMain" /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
IntDir=.\Debug
TargetDir=\Prog\shared\WPatch\Debug
SOURCE="$(InputPath)"
PostBuild_Desc=Building Sample Installer...
PostBuild_Cmds="%ProgramFiles%\NSIS\makensis" /V4 /DTARGETDIR=$(TargetDir) /DINTDIR=$(IntDir) "..\Demo\Sample.nsi"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "WPatch - Win32 Release UNICODE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WPatch___Win32_Release_UNICODE"
# PROP BASE Intermediate_Dir "WPatch___Win32_Release_UNICODE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_UNICODE"
# PROP Intermediate_Dir "Release_UNICODE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NSIS" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "NSIS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib shlwapi.lib /nologo /entry:"DllMain" /dll /machine:I386 /out:"../../bin/WPatch.dll" /OPT:REF /OPT:NOWIN98
# SUBTRACT BASE LINK32 /pdb:none /debug /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib shlwapi.lib /nologo /entry:"DllMain" /dll /debug /machine:I386 /out:"../../bin/WPatchW.dll" /OPT:REF /OPT:NOWIN98
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
IntDir=.\Release_UNICODE
TargetDir=\Prog\shared\bin
SOURCE="$(InputPath)"
PostBuild_Desc=Building Sample Installer...
PostBuild_Cmds="%ProgramFiles%\NSIS\Unicode\makensis" /V2 "/DTARGETDIR=$(TargetDir)" "/DINTDIR=$(IntDir)" "..\Demo\Sample.nsi"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "WPatch - Win32 Release"
# Name "WPatch - Win32 Debug"
# Name "WPatch - Win32 Release UNICODE"
# Begin Source File

SOURCE=.\apply_patch.cpp
# End Source File
# Begin Source File

SOURCE=.\apply_patch.h
# End Source File
# Begin Source File

SOURCE=.\checksum.c
# End Source File
# Begin Source File

SOURCE=.\checksum.h
# End Source File
# Begin Source File

SOURCE=.\exdll.h
# End Source File
# Begin Source File

SOURCE=..\License.txt
# End Source File
# Begin Source File

SOURCE=.\md5.c
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=..\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\WPatch.cpp
# End Source File
# End Target
# End Project
