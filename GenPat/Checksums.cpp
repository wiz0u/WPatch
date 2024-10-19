//---------------------------------------------------------------------------
// Checksums.cpp
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
// Unicode support by Jim Park -- 08/29/2007
// Adapted for WPatch by Olivier Marcoux http://wizou.fr

#include "Checksums.h"

// Jim Park: string -> tstring.
void TChecksum::LoadFile(tstring& fileName) {
  bifstream data;
  data.open(fileName.c_str(), ios::binary | ios::in);
  data.seekg(0, ios::beg);
  const int MD5BLOCKSIZE = 65536;
  uint8_t md5block[MD5BLOCKSIZE]; // ok maybe it's not nice to allocate 64K off the stack.. convert to heap alloc?
  md5_state_t state;

  md5_init(&state);
  if (mode == SPLIT_MD5)
  {
      data.read(reinterpret_cast<char*>(md5block), MD5BLOCKSIZE);
      md5_append(&state, md5block, data.gcount());
      data.seekg(-MD5BLOCKSIZE, ios::end);
      data.read(reinterpret_cast<char*>(md5block), MD5BLOCKSIZE);
      md5_append(&state, md5block, data.gcount());
  }
  else
  {
    while(data.good()) {
      data.read(reinterpret_cast<char*>(md5block), MD5BLOCKSIZE);
      md5_append(&state, md5block, data.gcount());
    }
  }
  md5_finish(&state, digest);
  data.close();
}

void TChecksum::loadMD5(md5_byte_t newdigest[16]) {
  for(int i = 0; i < 16; i++) {
    digest[i] = newdigest[i];
  }
}
bool TChecksum::operator==(const TChecksum& b) {
  if(mode != b.mode) 
      return false;
  for(int md5index = 0; md5index < 16; md5index++) {
    if(digest[md5index] != b.digest[md5index]) break;
    if(md5index == 15) return true;
  }
  return false;
}
