//---------------------------------------------------------------------------
// checksum.c
//---------------------------------------------------------------------------
//                           -=* VPatch *=-
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
// Reviewed for Unicode support by Jim Park -- 08/29/2007
// Adapted for WPatch by Olivier Marcoux http://wizou.fr

#include "checksum.h"

#ifdef _MSC_VER
#  define FORCE_INLINE __forceinline
#else
#  ifdef __GNUC__
#    if __GNUC__ < 3
#      define FORCE_INLINE inline
#    else
#      define FORCE_INLINE inline __attribute__ ((always_inline))
#    endif
#  else
#    define FORCE_INLINE inline
#  endif
#endif

/* ------------------------ MD5 checksum calculation ----------------- */

#define MD5BLOCKSIZE    65536
static BYTE md5block[MD5BLOCKSIZE];

BOOL FileMD5(HANDLE hFile, md5_byte_t digest[16]) {
  DWORD read;
  md5_state_t state;
  md5_init(&state);
 
  SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
  do {
    if (ReadFile(hFile, md5block, MD5BLOCKSIZE, &read, NULL) == FALSE)
      return FALSE;
    md5_append(&state, md5block, read);
  } while (read);

  md5_finish(&state, digest);
  return TRUE;
}

BOOL FileSplitMD5(HANDLE hFile, md5_byte_t digest[16]) {
  DWORD read;
  md5_state_t state;
  md5_init(&state);
 
  SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
  if (ReadFile(hFile, md5block, MD5BLOCKSIZE, &read, NULL) == FALSE) return FALSE;
  md5_append(&state, md5block, read);
  SetFilePointer(hFile, -MD5BLOCKSIZE, NULL, FILE_END);
  if (ReadFile(hFile, md5block, MD5BLOCKSIZE, &read, NULL) == FALSE) return FALSE;
  md5_append(&state, md5block, read);

  md5_finish(&state, digest);
  return TRUE;
}
