//---------------------------------------------------------------------------
// FileFormat1.cpp
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

#include "FileFormat1.h"
#include "GlobalTypes.h"

#define MAGIC_VPAT 'WPAT'
#define FORMAT_VERSION      0x00010000
#define FILEFLAG_SPLIT_MD5  0x00000001

namespace FileFormat1 {
	void writeByte(bostream& patch, TFileOffset dw)
	{
		unsigned char b = dw & 0xFF;
		patch.write(reinterpret_cast<char*>(&b),sizeof(b));
	}
	void writeWord(bostream& patch, TFileOffset dw)
	{
		unsigned char b = dw & 0xFF;
		patch.write(reinterpret_cast<char*>(&b),sizeof(b));
		b = (dw & 0xFF00) >> 8;
		patch.write(reinterpret_cast<char*>(&b),sizeof(b));
	}
	void writeDword(bostream& patch, TFileOffset dw)
	{
		unsigned char b = dw & 0xFF;
		patch.write(reinterpret_cast<char*>(&b),sizeof(b));
		b = (dw & 0xFF00) >> 8;
		patch.write(reinterpret_cast<char*>(&b),sizeof(b));
		b = (dw & 0xFF0000) >> 16;
		patch.write(reinterpret_cast<char*>(&b),sizeof(b));
		b = (dw & 0xFF000000) >> 24;
		patch.write(reinterpret_cast<char*>(&b),sizeof(b));
	}
	
	void writeMD5(bostream& patch, const md5_byte_t digest[16])
	{
		for(int i = 0; i < 16; i++)
			writeByte(patch, digest[i]);
	}
	
	TFileOffset readDword(bistream& patch)
	{
		unsigned char b;
		patch.read(reinterpret_cast<char*>(&b),sizeof(b));
		TFileOffset dw = b;
		patch.read(reinterpret_cast<char*>(&b),sizeof(b));
		dw = dw | (b << 8);
		patch.read(reinterpret_cast<char*>(&b),sizeof(b));
		dw = dw | (b << 16);
		patch.read(reinterpret_cast<char*>(&b),sizeof(b));
		dw = dw | (b << 24);
		return dw;
	}
	
	void checkDword(bistream& patch, TFileOffset cdw)
	{
		TFileOffset dw = readDword(patch);
		if (dw != cdw)
		{
			printf("Patch consistency failed!\n");
			exit(4);
		}
	}
	
	void readMD5(bistream& patch, md5_byte_t digest[16])
	{
		unsigned char b;
		for(int i = 0; i < 16; i++) {
			patch.read(reinterpret_cast<char*>(&b),sizeof(b));
			digest[i] = b;
		}
	}
	
	void updateFileCount(biostream& f, int delta)
	{
		TFileOffset currentCount;
		f.seekp(8,ios::beg);
		currentCount = readDword(f);
		currentCount += delta;
		f.seekp(8,ios::beg);
		writeDword(f,currentCount);
	}
	
	void createEmptyPatch(bostream& patch)
	{
		TFileOffset fileCount = 0;
		writeDword(patch,MAGIC_VPAT);
		writeDword(patch,FORMAT_VERSION);
		writeDword(patch,fileCount);           // noFiles
	}
	
	TFileOffset checkExistingPatch(biostream& patch, const TChecksum& sourceCRC, const TChecksum& targetCRC, bool modePrecise)
	{
		TFileOffset fileCount = 0;
		checkDword(patch, MAGIC_VPAT);
		checkDword(patch, FORMAT_VERSION);
		fileCount = readDword(patch);
		
		TFileOffset tempCount = fileCount;
		for(TFileOffset i = 0; i < tempCount; i++)
		{
			TFileOffset startOffset = patch.tellg();
			TFileOffset flags = readDword(patch);        // fileFlags
			readDword(patch);                            // targetSize
			TChecksum sourceChecksum, targetChecksum;
			if (flags & FILEFLAG_SPLIT_MD5)
			{
				sourceChecksum.mode = TChecksum::SPLIT_MD5;
				targetChecksum.mode = TChecksum::SPLIT_MD5;
			}
			md5_byte_t digest[16];
			readMD5(patch, digest);                      // SourceCRC
			sourceChecksum.loadMD5(digest);
			readMD5(patch, digest);                      // TargetCRC
			targetChecksum.loadMD5(digest);
			TFileOffset bodySize = readDword(patch);     // bodySize
			readDword(patch);                            // FILETIME.dwLowDateTime
			readDword(patch);                            // FILETIME.dwHighDateTime
			readDword(patch);                            // noBlocks
			patch.seekg(bodySize,ios::cur);
			TFileOffset endOffset = patch.tellg();
			if (sourceChecksum == sourceCRC)
			{
				if (targetChecksum == targetCRC)
					return startOffset; // identical patch already inside, return offset
				else if (!modePrecise)
					return 3; // if not precise, it's an error to have the same sourceCRC & different targetCRC
			}
		}
		return 0;
	}
	
	void writePatch(biostream& patch, bistream& target, TFileOffset targetSize, vector<SameBlock*>& sameBlocks,
					const TChecksum& sourceCRC, const TChecksum& targetCRC, POSIX::ALT_FILETIME targetTime)
	{
		TFileOffset bodySize = 0;
		TFileOffset flags = 0;
		if (sourceCRC.mode == TChecksum::SPLIT_MD5)
			flags |= FILEFLAG_SPLIT_MD5;
		writeDword(patch,flags);                    // fileFlags
		writeDword(patch,targetSize);               // targetSize
		writeMD5(patch,sourceCRC.digest);          // sourceCRC
		writeMD5(patch,targetCRC.digest);          // targetCRC
		TFileOffset bodySizeOffset = patch.tellp();
		writeDword(patch,bodySize);                 // bodySize
		writeDword(patch,targetTime.dwLowDateTime); // FILETIME.dwLowDateTime
		writeDword(patch,targetTime.dwHighDateTime);// FILETIME.dwHighDateTime
		TFileOffset noBlocks = 0;
		TFileOffset noBlocksOffset = patch.tellp();
		writeDword(patch,noBlocks);                 // noBlocks
		
		for(vector<SameBlock*>::iterator iter = sameBlocks.begin(); iter != sameBlocks.end(); iter++) {
			SameBlock* current = *iter;
			
			// store current block
			if(current->size > 0) {
				// copy block from sourceFile
				if(current->size < 256) {
					writeByte(patch,1);
					writeByte(patch,current->size);
					bodySize += 2;
				} else if(current->size < 65536) {
					writeByte(patch,2);
					writeWord(patch,current->size);
					bodySize += 3;
				} else {
					writeByte(patch,3);
					writeDword(patch,current->size);
					bodySize += 5;
				}
				writeDword(patch,current->sourceOffset);
				bodySize += 4;
				noBlocks++;
			}
			iter++;
			if(iter == sameBlocks.end()) break;
			SameBlock* next = *iter;
			iter--;
			
			// calculate area inbetween this block and the next
			TFileOffset notFoundStart = current->targetOffset+current->size;
			if(notFoundStart > next->targetOffset) {
				throw _T("makeBinaryPatch input problem: there was overlap");
			}
			TFileOffset notFoundSize = next->targetOffset - notFoundStart;
			if(notFoundSize > 0) {
				// we need to include this area in the patch directly
				if(notFoundSize < 256) {
					writeByte(patch,5);
					writeByte(patch,notFoundSize);
					bodySize += 2;
				} else if(notFoundSize < 65536) {
					writeByte(patch,6);
					writeWord(patch,notFoundSize);
					bodySize += 3;
				} else {
					writeByte(patch,7);
					writeDword(patch,notFoundSize);
					bodySize += 5;
				}
				// copy from target...
				target.seekg(notFoundStart,ios::beg);
#define COPY_BUF_SIZE 4096
				char copyBuffer[COPY_BUF_SIZE];
				for(TFileOffset i = 0; i < notFoundSize; i += COPY_BUF_SIZE) {
					TFileOffset j = notFoundSize - i;
					if(j > COPY_BUF_SIZE) j = COPY_BUF_SIZE;
					target.read(copyBuffer,j);
					patch.write(copyBuffer,j);
				}
				bodySize += notFoundSize;
				noBlocks++;
			}
		}
		TFileOffset curPos = patch.tellp();
		patch.seekp(noBlocksOffset,ios::beg);
		writeDword(patch,noBlocks);
		patch.seekp(bodySizeOffset,ios::beg);
		writeDword(patch,bodySize);
		// do this at the end because it messes up file position
		updateFileCount(patch,+1);
		patch.seekp(curPos,ios::beg);
	}
}
