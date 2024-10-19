/*
LICENSE
=======

WGenPatDir/GenPatDir
Copyright (C) 2007-2012  Olivier Marcoux  ( http://wizou.fr )

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

// GenPatDir.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#ifdef WPATCH
#define GENPATDIR "WGenPatDir"
#define GENPATEXE "WGENPAT.EXE"
#else
#define GENPATDIR "GenPatDir"
#define GENPATEXE "GENPAT.EXE"
#endif

string dir1, dir2;
string excludeSpec, nsisExclude;
string lastSpec;
HANDLE nsiHandle;
DWORD dwDummy;
#define BLOCKSIZE 128*1024
void *block1, *block2;
bool forcediff = false;
bool modePrecise = false;
bool include_system = false;
bool include_hidden = false;

/*
 NOTES ON NSIS:
    Delete, RMDir, SetOutPath requires a full path
    File /r should almost always be used with a "path\*" as input (see doc)
    a $ variable/macro specified alone can represent only one parameter, not a list of parameters

 */

#if _MSC_VER <= 1200
#define _tcscpy_s _tcscpy
#endif 

struct FileInfo
{
public:
	FileInfo(CONST CHAR *a_name, bool a_isdir, DWORD a_size, FILETIME a_time)
		: name(a_name), isdir(a_isdir), size(a_size), time(a_time) {}
    string   name;
    bool     isdir;
    DWORD    size;
    FILETIME time;
    bool operator < (const FileInfo &info2) const { return _stricmp(name.c_str(), info2.name.c_str()) < 0; }
};

typedef set<FileInfo> Filenames;

struct ChangeInfo
{

    string   pathname;
    enum Action { REMOVED, REMOVED_DIR, ADDED, ADDED_EMPTYDIR, ADDED_DIR, MODIFIED, MODIFIED_HUGE } action;
	DWORD	patchOffset;
    ChangeInfo(const string& a_pathname, enum Action a_action) : pathname(a_pathname), action(a_action) {};
    ChangeInfo(const string& a_pathname, enum Action a_action, DWORD a_patchOffset) : pathname(a_pathname), action(a_action), patchOffset(a_patchOffset) {};
};
typedef list<ChangeInfo> Changelist;


inline void Output(const string &s) { WriteFile(nsiHandle, s.c_str(), DWORD(s.length()), &dwDummy, NULL); }
inline void Output(LPCTSTR s) { WriteFile(nsiHandle, s, DWORD(strlen(s)), &dwDummy, NULL); }

///
/// Replace all occurences of a substring in a string with another string, in-place.
///
/// @param s String to replace in. Will be modified.
/// @param sub Substring to replace.
/// @param other String to replace substring with.
///
/// @return The string after replacements.
int replace_all(std::string &s, const std::string& from, const std::string& to)
{
    const std::string::size_type from_len = from.size();
    const std::string::size_type to_len = to.size();
    if ((from_len == 0) || ((from_len == to_len) && (from == to))) 
        return 0;
    std::string::size_type pos = 0;
    int counter = 0;
    while ((pos = s.find(from, pos)) != std::string::npos)
    {
        s.replace(pos, from_len, to);
        pos += to_len;
        ++counter;
    }
    return counter;
}

DWORD ExecWait(LPCTSTR cmdline)
{
    TCHAR commandLine[1024];
    _tcscpy_s(commandLine, cmdline);
	DWORD exitCode;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if (!CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		FatalAppExit(0, "Cannot launch subprogram");
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	return exitCode;
}

void LoadDir(const string &dir, Filenames &names)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile((dir+"*").c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do
    {
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (FindFileData.cFileName[0] == '.'))
            continue;
        if (!excludeSpec.empty() && PathMatchSpec(FindFileData.cFileName, excludeSpec.c_str()))
            continue;
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !include_hidden)
            continue;
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) && !include_system)
            continue;
        FileInfo info(	FindFileData.cFileName,
						(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0, 
						FindFileData.nFileSizeLow,
						FindFileData.ftLastWriteTime);
        names.insert(info);
    } while (FindNextFile(hFind, &FindFileData));

    FindClose(hFind);
}

bool FileDiff(const string &file1, const string &file2)
{
    bool diff = false;
    HANDLE hFile1, hFile2;
    hFile1 = CreateFile(file1.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    hFile2 = CreateFile(file2.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if ((hFile1 != INVALID_HANDLE_VALUE) && (hFile2 != INVALID_HANDLE_VALUE))
    {
        DWORD read1, read2;
        do
        {
            ReadFile(hFile1, block1, BLOCKSIZE, &read1, NULL);
            ReadFile(hFile2, block2, BLOCKSIZE, &read2, NULL);
            if (read1 != read2)
            {
                printf("An error occured diffing files !");
				exit(128);
            }
            if (memcmp(block1, block2, read1) != 0)
                diff = true;
        } while (!diff && (read1 == BLOCKSIZE));
    }
    CloseHandle(hFile1);
    CloseHandle(hFile2);
    return diff;
}

bool IsEmptyDir(const string &dir)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile((dir+"\\*").c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) return true;
    do
    {
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (FindFileData.cFileName[0] == '.'))
            continue;
        if (!excludeSpec.empty() && PathMatchSpec(FindFileData.cFileName, excludeSpec.c_str()))
            continue;
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !include_hidden)
            continue;
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) && !include_system)
            continue;
        return false;
    } while (FindNextFile(hFind, &FindFileData));
    return true;
}

void Removed(const string &path, const string &name, bool isdir, Changelist& changelist)
{
    if (isdir)
        changelist.push_back(ChangeInfo(path+name, ChangeInfo::REMOVED_DIR));
    else
        changelist.push_back(ChangeInfo(path+name, ChangeInfo::REMOVED));
}

void Added(const string &path, const string &name, bool isdir, Changelist& changelist)
{
    if (isdir)
        if (IsEmptyDir((dir2+path+name).c_str()))
            changelist.push_back(ChangeInfo(path+name, ChangeInfo::ADDED_EMPTYDIR));
        else
            changelist.push_back(ChangeInfo(path+name, ChangeInfo::ADDED_DIR));
    else
        changelist.push_back(ChangeInfo(path+name, ChangeInfo::ADDED));
}

void Modified(const string &pathname, DWORD size, Changelist& changelist)
{
	DWORD exitCode;
	string cmdline = GENPATEXE;
	bool hugeFile = false;
#ifdef WPATCH
    if (size > 20*1024*1024) // 20 MB => huge file
		hugeFile = true;
#endif
    if (hugeFile) cmdline += " /H";
	if (modePrecise) cmdline += " /P";
	cmdline += " \"" + dir1 + pathname + "\" \"" + dir2 + pathname + "\" "GENPATDIR".pat";
    exitCode = ExecWait(cmdline.c_str());
	if ((exitCode > 0) && (exitCode < 12))
	{
        printf("%s\n returned an error %d\n", cmdline.c_str(), exitCode);
		exit(1);
	}
    changelist.push_back(ChangeInfo(pathname, hugeFile ? ChangeInfo::MODIFIED_HUGE : ChangeInfo::MODIFIED, exitCode));
}

void ScanDir(const string &path, Changelist& changelist)
{
    Filenames names1, names2;
    // load filename lists of both directories in alphabetically-sorted sets
    LoadDir(dir1+path, names1);
    LoadDir(dir2+path, names2);
    Filenames::const_iterator it1 = names1.begin();
    Filenames::const_iterator it2 = names2.begin();
    // compare file lists
    while ((it1 != names1.end()) && (it2 != names2.end()))
    {
        int res = _stricmp(it1->name.c_str(), it2->name.c_str());
        if (res < 0)
        {
            Removed(path, it1->name, it1->isdir, changelist);
            it1++;
        }
        else if (res > 0)
        {
            Added(path, it2->name, it2->isdir, changelist);
            it2++;
        }
        else // we have a name match
        {
            if (it1->isdir != it2->isdir) // type has changed from directory to file, or vice-versa => force a remove+add
            {
                Removed(path, it1->name, it1->isdir, changelist);
                Added(path, it2->name, it2->isdir, changelist);
            }
            else if (it1->isdir) // both are directories => recurse
            {
                ScanDir(path+it1->name+'\\', changelist);
            }
            else if (it1->size != it2->size) // size has changed => modified
            {
                Modified(path+it1->name, max(it1->size, it2->size), changelist);
            }
            else if (forcediff || (CompareFileTime(&it1->time, &it2->time) != 0)) // size is same but filetime different
            {
                string pathname = path+it1->name;
                if (FileDiff(dir1+pathname, dir2+pathname)) // check for content diff, if yes => modified
                    Modified(pathname, it1->size, changelist);
            }
            it1++;
            it2++;
        }
    }
    // take care of extra files left after loop
    while (it1 != names1.end())
    {
        Removed(path, it1->name, it1->isdir, changelist);
        it1++;
    }
    while (it2 != names2.end())
    {
        Added(path, it2->name, it2->isdir, changelist);
        it2++;
    }
}

void CheckPatched(const Changelist& changelist)
{
    char tempBuf[64];
    for (Changelist::const_iterator it = changelist.begin(); it != changelist.end(); it++)
    {
        switch (it->action) {
        case ChangeInfo::MODIFIED_HUGE:
            Output( "StrCpy $0   '" );
            Output( it->pathname );
            Output( "' \t; Check modified huge file\r\n"
                    "StrCpy $1 '/CHECK /HUGE" );
			if (modePrecise)
			{
				sprintf(tempBuf, " /PRECISE %d", it->patchOffset);
				Output( tempBuf );
			}
			Output( "'\r\n"
                    "Call Patch\r\n" );
            break;
        case ChangeInfo::MODIFIED:
            Output( "StrCpy $0   '" );
            Output( it->pathname );
            Output( "' \t; Check modified file\r\n"
                    "StrCpy $1 '/CHECK" );
			if (modePrecise)
			{
				sprintf(tempBuf, " /PRECISE %d", it->patchOffset);
				Output( tempBuf );
			}
			Output( "'\r\n"
					"Call Patch\r\n" );
            break;
        }
    }
}

void OutputPatch(const Changelist& changelist, BOOL last)
{
	char tempBuf[64];
    for (Changelist::const_iterator it = changelist.begin(); it != changelist.end(); it++)
    {
        if (last ^ PathMatchSpec(it->pathname.c_str(), lastSpec.c_str()))
            continue;
        switch (it->action) {
        case ChangeInfo::REMOVED:
            Output("Delete      '$INSTDIR\\");				// Delete requires a full path
            Output(it->pathname);
            Output("' \t; Removed file\r\n");
            break;
        case ChangeInfo::REMOVED_DIR:
            Output("RMDir /r    '$INSTDIR\\");				// RMDir requires a fully qualified path
            Output(it->pathname);
            Output("' \t; Removed directory\r\n");
            break;
        case ChangeInfo::ADDED:
            Output( "File '/oname=$INSTDIR\\" );
            Output( it->pathname );
            Output( "' \t'${ADDEDSOURCE}" );
            Output( it->pathname );
            Output( "' \t; Added file\r\n" );
            break;
        case ChangeInfo::ADDED_EMPTYDIR:
            Output( "CreateDirectory '$INSTDIR\\" );		// CreateDirectory requires an absolute path
            Output( it->pathname );
            Output( "' \t; Added empty directory\r\n" );
            break;
        case ChangeInfo::ADDED_DIR:
            Output( "SetOutPath  '$INSTDIR\\" );			// SetOutPath requires a full pathname
            Output( it->pathname );
            Output( "' \t; Added directory\r\n"
                    "File /r" );
            Output( nsisExclude );
            Output( " \t'${ADDEDSOURCE}" );
            Output( it->pathname );
            Output( "\\*'\r\n" );
            break;
        case ChangeInfo::MODIFIED_HUGE:
            Output( "StrCpy $0   '" );
            Output( it->pathname );
            Output( "' \t; Modified huge file\r\n"
                    "StrCpy $1 '/HUGE" );
			if (modePrecise)
			{
				sprintf(tempBuf, " /PRECISE %d", it->patchOffset);
				Output( tempBuf );
			}
			Output( "'\r\n"
                    "Call Patch\r\n" );
            break;
        case ChangeInfo::MODIFIED:
            Output( "StrCpy $0   '" );
            Output( it->pathname );
            Output( "' \t; Modified file\r\n"
					"StrCpy $1 '" );
			if (modePrecise)
			{
				sprintf(tempBuf, "/PRECISE %d", it->patchOffset);
				Output( tempBuf );
			}
			Output( "'\r\n"
                    "Call Patch\r\n" );
            break;
        }
    }
}

void ShowUsage()
{
    printf( GENPATDIR" [--options] directory1 directory2\r\n"
            "options are:\r\n"
            "   --forcediff             : force to check differences between files having same name & date\r\n"
			"   --precise               : enable fast & precise updates\r\n"
            "   --hidden                : include hidden files in comparison\r\n"
            "   --system                : include system files in comparison\r\n"
            "   --exclude wildcard-list : match filenames to exclude from comparison\r\n"
            "   --last wildcard-list    : match filenames to be patched at the end\r\n"
            "\r\n"
            "wildcard-list is a list of semicolons separated DOS wildcards\r\n"
        );
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf( GENPATDIR" 4.03 - http://wizou.fr\n\n");
    int argi;
    for (argi = 1; argi < argc; argi++)
    {
        if ((argv[argi][0] != '-') || (argv[argi][1] != '-'))
            break;
        if (_stricmp(argv[argi], "--forcediff") == 0)
            forcediff = true;
        else if (_stricmp(argv[argi], "--precise") == 0)
            modePrecise = true;
        else if (_stricmp(argv[argi], "--hidden") == 0)
            include_hidden = true;
        else if (_stricmp(argv[argi], "--system") == 0)
            include_system = true;
        else if (_stricmp(argv[argi], "--exclude") == 0)
        {
            excludeSpec = argv[++argi]; // exclusion list : DOS wildcards separated by semicolons
            if (!excludeSpec.empty())
            {
                nsisExclude = excludeSpec;
                replace_all(nsisExclude, ";", " /x ");
                nsisExclude.insert(0, " /x ");
            }
        }
        else if (_stricmp(argv[argi], "--last") == 0)
        {
            lastSpec = argv[++argi]; // list of files to be updated last: DOS wildcards separated by semicolons
        }
        else
        {
            ShowUsage();
            return 1;
        }
    }
    if (argi != argc-2)
    {
        ShowUsage();
        return 1;
    }

    dir1 = string(argv[argi])+'\\';
    dir2 = string(argv[argi+1])+'\\';

    block1 = LocalAlloc(LMEM_FIXED, BLOCKSIZE);
    block2 = LocalAlloc(LMEM_FIXED, BLOCKSIZE);
    if (block1 && block2)
    {
#ifndef WPATCH
        // initialize empty GenPatDir.pat
        nsiHandle = CreateFile(GENPATDIR".pat", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        CloseHandle(nsiHandle);
#endif

        Changelist changelist;
        ScanDir(string(), changelist);


        nsiHandle = CreateFile(GENPATDIR".nsh", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		Output( "!ifndef ADDEDSOURCE\r\n"
				"!define ADDEDSOURCE '" );
		Output( dir2 );
		Output( "'\r\n"
				"!endif\r\n"
				"\r\n" );
#ifdef WPATCH
        Output( "Function Patch\r\n"
                "	DetailPrint 'Patch: $0'\r\n"
				"	StrCpy $0 '$INSTDIR\\$0'\r\n"
                "retry:\r\n"
                "	WPatch::PatchFile /NOUNLOAD	; expects $0:file path, $1:options, $2:patch path\r\n"
                "	IntCmp $1 0 continue can_skip 0\r\n"
                "		SetErrors\r\n"
                "can_skip:\r\n"
                "		SetDetailsPrint listonly\r\n"
                "		DetailPrint '=> Error $1'\r\n"
                "		SetDetailsPrint both\r\n"
                "		IntCmp $1 1 0 continue continue\r\n"
                "			MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION $(^FileError_NoIgnore) /SD IDCANCEL IDRETRY retry\r\n"
                "			Abort\r\n"
                "continue:\r\n"
                "FunctionEnd\r\n"
              );
#else
        Output( "Function Patch\r\n"
                "	DetailPrint \"Patch: $0\"\r\n"
                "	vpatch::vpatchfile \"$2\" \"$INSTDIR\\$0\" \"$$INSTDIR\\0.patched\"\r\n"
                "	Pop $1\r\n"
                "	StrCmp $1 \"OK\" +4\r\n"
                "		DetailPrint \"=> $1\"\r\n"
                "		SetErrors\r\n"
                "		Return\r\n"
                "	SetDetailsPrint none\r\n"
                "	Delete \"$INSTDIR\\$0\"\r\n"
                "	Rename \"$INSTDIR\\$0.patched\" \"$INSTDIR\\$0\"\r\n"
                "	SetDetailsPrint lastused\r\n"
                "FunctionEnd\r\n" );
#endif // WPATCH
		
		Output( "\r\n"
				"Section 'ApplyPatch'\r\n"
				"ClearErrors\r\n"
				"SetOutPath '$PLUGINSDIR'\r\n"
				"File "GENPATDIR".pat\r\n"
				"StrCpy $2 '$PLUGINSDIR\\"GENPATDIR".pat'\r\n"
				"DetailPrint 'Checking before patch...'\r\n"
                "\r\n");
        CheckPatched(changelist);
        Output( "\r\n"
                "IfErrors 0 +3\r\n"
                "\tSetErrors\r\n"
                "\tGoto end_of_patch\r\n"
                "DetailPrint 'Beginning real patch...'\r\n"
                "\r\n");
        OutputPatch(changelist, FALSE);
        OutputPatch(changelist, TRUE);
		Output( "\r\n" 
				"end_of_patch:\r\n"
#ifdef WPATCH
				"StrCpy $1 '/UNLOAD'\r\n"
				"WPatch::PatchFile\r\n"
#endif
				"Delete $2\r\n"
				"; Now you should check for IfErrors ...\r\n"
				"SectionEnd\r\n"
				"\r\n");

        CloseHandle(nsiHandle);
        LocalFree(block1);
        LocalFree(block2);
        printf("success !\n");
    }
	return 0;
}
