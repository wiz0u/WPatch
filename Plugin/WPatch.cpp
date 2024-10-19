//---------------------------------------------------------------------------
// WPatch.c: NSIS plug-in version of the WPatch runtime
//---------------------------------------------------------------------------
//                           -=* WPatch *=-
//---------------------------------------------------------------------------
// Copyright (C) 2001-2005 Koen van de Sande / Van de Sande Productions
//---------------------------------------------------------------------------
// Website: http://www.tibed.net/vpatch
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Unicode support by Jim Park -- 08/29/2007
// Adapted for WPatch by Olivier Marcoux http://wizou.fr

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <shlwapi.h>
#include <tchar.h>
#include "apply_patch.h"
#include "checksum.h"
#include "exdll.h"

HINSTANCE g_hInstance;
HWND g_hwndParent;
HWND g_hwndStatic;
WCHAR g_staticText[512];
LPWSTR g_staticSuffix;

#define _countof(array) (sizeof(array)/sizeof((array)[0]))

const char version[] = "WPatch 4.00 by Olivier Marcoux";

/* ------------------------ Plug-in code ------------------------- */

int wpatch(const TCHAR* patchPath, const TCHAR* filePath, bool hugeFile, bool check, DWORD preciseOffset, HWND hwndParent)
{
	version;
    HANDLE hPatch, hFile;
    int result;

    g_hwndParent = hwndParent;
    if (hwndParent)
    {
        g_hwndStatic = GetDlgItem(FindWindowEx(hwndParent, NULL, _T("#32770"), NULL), 1006);
#ifdef _DEBUG
        if (!g_hwndStatic) g_hwndStatic = FindWindow(_T("wndclass_desked_gsk"), NULL); // Visual Studio Window
#endif
        if (g_hwndStatic)
        {
            GetWindowTextW(g_hwndStatic, g_staticText, _countof(g_staticText));
            lstrcatW(g_staticText, L" ");
            g_staticSuffix = g_staticText+lstrlenW(g_staticText);
        }
    }

    hPatch = CreateFile(patchPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hPatch == INVALID_HANDLE_VALUE)
        return PATCH_ERROR;

    hFile = CreateFile(filePath, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPatch);
        return PATCH_ERROR;
    }

    result = DoPatch(hPatch, hFile, hugeFile, check, preciseOffset);

    CloseHandle(hFile);
    CloseHandle(hPatch);

    return result;
}

#ifdef NSIS
extern "C"
void __declspec(dllexport) PatchFile(HWND hwndParent, int string_size, 
                                      TCHAR *variables, stack_t **stacktop)
{
    EXDLL_INIT();
    LPCTSTR filePath    = getuservariable(INST_0);
    LPTSTR  var1        = getuservariable(INST_1); // in:options, out:result code
    LPCTSTR patchPath   = getuservariable(INST_2);
    int result;
    if (lstrcmp(var1, _T("/UNLOAD")) == 0)
        result = 0;
    else
    {
		bool checkMode = false;
		bool hugeMode = false;
		DWORD preciseOffset = 0;
		lstrcat(var1, _T(" "));
		LPCTSTR options = var1;
		for (;;)
		{
			while (isspace(*options))
				options++;
			if (*options != '/')
				break;
			if (StrCmpNI(options, _T("/CHECK "), 7) == 0)
				options += 7, checkMode = true;
			else if (StrCmpNI(options, _T("/HUGE "), 6) == 0)
				options += 6, hugeMode = true;
			else if (StrCmpNI(options, _T("/PRECISE "), 9) == 0)
			{
				options += 9;
				preciseOffset = StrToInt(options);
				while (!isspace(*options))
					options++;
			}
		}
		if (*options)
			result = PATCH_BADARGS;
        else
			result = wpatch(patchPath, filePath, hugeMode, checkMode, preciseOffset, hwndParent);
    }
    wsprintf(var1, _T("%d"), result);
}

BOOL WINAPI DllMain(HINSTANCE hInst, ULONG fdwReason, LPVOID lpReserved)
{
    g_hInstance=hInst;
    return TRUE;
}
#else
BOOL WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,  LPSTR lpCmdLine, int nCmdShow)
{
    void *miam_la_ram = malloc(0x30000000);
    g_hInstance=hInst;
    int result = wpatch(__argv[1], __argv[2], true, false, NULL);
    if (result != PATCH_SUCCESS)
        __asm int 3;
    return TRUE;
}
#endif
