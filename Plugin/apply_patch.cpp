//---------------------------------------------------------------------------
// apply_patch.c
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
// Reviewed for Unicode support by Jim Park -- 08/29/2007
// Adapted for WPatch by Olivier Marcoux http://wizou.fr

#include "apply_patch.h"
#include "checksum.h"

#define MAGIC_VPAT 'WPAT'
#define FORMAT_VERSION      0x00010000
#define FILEFLAG_SPLIT_MD5  0x00000001

/* ------------------------ patch application ----------------- */
extern HWND g_hwndStatic;
extern WCHAR g_staticText[512];
extern LPWSTR g_staticSuffix;


#ifdef _DEBUG
size_t debugMem = 0;
size_t maxDebugMem = 0;
void ReportDebugMem(size_t delta)
{
    debugMem += delta;
    maxDebugMem = max(maxDebugMem, debugMem);
    //printf("Memory Used: %d K\t\tmax: %d K\r\n", debugMem/1024, maxDebugMem/1024);
}
#define DEBUGMEM(delta) ReportDebugMem(delta);
#else
#define DEBUGMEM(delta)
#endif

namespace ViewManager
{
    static HANDLE _hFileMappingObject;
    static DWORD _dwAllocationGranularity;

    static DWORD   _mapSize;
    static DWORD   _idealStart;
    static DWORD   _idealEnd;
    static LPBYTE  _idealBaseAddress;
    static DWORD   _dstStart;
    static DWORD   _dstEnd;
    static LPBYTE  _dstBaseAddress;
    static DWORD   _srcStart;
    static DWORD   _srcEnd;
    static LPBYTE  _srcBaseAddress;

    void Initialize(HANDLE hFileMappingObject, DWORD mapSize)
    {
        _hFileMappingObject = hFileMappingObject;
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        _dwAllocationGranularity = systemInfo.dwAllocationGranularity;

        _mapSize = mapSize;

        // try to map whole file
        _idealBaseAddress = (LPBYTE) MapViewOfFile(_hFileMappingObject, FILE_MAP_WRITE, 0, 0, mapSize);
        _idealStart = 0;
        _idealEnd = (_idealBaseAddress == NULL) ? 0 : mapSize;

        _dstBaseAddress = NULL;
        _dstStart = _dstEnd = 0;
        _srcBaseAddress = NULL;
        _srcStart = _srcEnd = 0;
    }
    void Terminate()
    {
        if (_srcBaseAddress)   UnmapViewOfFile(_srcBaseAddress);
        if (_dstBaseAddress)   UnmapViewOfFile(_dstBaseAddress);
        if (_idealBaseAddress) UnmapViewOfFile(_idealBaseAddress);
    }

    bool IdealMapping(size_t start, size_t end, bool resetSrcDst = true)
    {
        if (resetSrcDst && _srcBaseAddress)
        {
            UnmapViewOfFile(_srcBaseAddress);
            _srcBaseAddress = NULL;
            _srcStart = _srcEnd = 0;
        }
        if (resetSrcDst && _dstBaseAddress)
        {
            UnmapViewOfFile(_dstBaseAddress);
            _dstBaseAddress = NULL;
            _dstStart = _dstEnd = 0;
        }
        if ((start >= _idealStart) && (end <= _idealEnd))
            return true;
        if (_idealBaseAddress) UnmapViewOfFile(_idealBaseAddress);
        _idealStart = DWORD(start) / _dwAllocationGranularity * _dwAllocationGranularity;
        _idealEnd = DWORD(end+_dwAllocationGranularity-1) / _dwAllocationGranularity * _dwAllocationGranularity;
        if (_idealEnd > _mapSize) _idealEnd = _mapSize;
        _idealBaseAddress = (LPBYTE) MapViewOfFile(_hFileMappingObject, FILE_MAP_WRITE, 0, _idealStart, _idealEnd-_idealStart);
        if (_idealBaseAddress == NULL)
        {
            _idealStart = _idealEnd = 0;
            return false;
        }
        else
            return true;
    }

    void ClearIdeal()
    {
        if (_idealBaseAddress) UnmapViewOfFile(_idealBaseAddress);
        _idealBaseAddress = NULL;
        _idealStart = _idealEnd = 0;
    }
    
    LPBYTE Destination(size_t start, size_t size)
    {
        size_t end = start+size;
        if ((start >= _idealStart) && (end <= _idealEnd))
        {
            if (_dstBaseAddress)
            {
                UnmapViewOfFile(_dstBaseAddress);
                _dstBaseAddress = NULL;
                _dstStart = _dstEnd = 0;
            }
            return _idealBaseAddress+(start-_idealStart);
        }
        else if ((start >= _dstStart) && (end <= _dstEnd))
        {
            return _dstBaseAddress+(start-_dstStart);
        }
        else if ((start >= _srcStart) && (end <= _srcEnd))
        {
            if (_dstBaseAddress) UnmapViewOfFile(_dstBaseAddress);
            // move source info to dest info
            _dstBaseAddress = _srcBaseAddress;
            _dstStart       = _srcStart;
            _dstEnd         = _srcEnd;
            _srcBaseAddress = NULL;
            _srcStart = _srcEnd = 0;
            return _dstBaseAddress+(start-_dstStart);
        }
        else
        {
            if (_dstBaseAddress) UnmapViewOfFile(_dstBaseAddress);
            _dstStart = DWORD(start) / _dwAllocationGranularity * _dwAllocationGranularity;
            _dstEnd = DWORD(end+_dwAllocationGranularity-1) / _dwAllocationGranularity * _dwAllocationGranularity;
            if (_dstEnd > _mapSize) _dstEnd = _mapSize;
            if ((_srcStart >= _dstStart) && (_srcEnd <= _dstEnd)) // we are allocating a dest zone that will contain the source zone
            {
                UnmapViewOfFile(_srcBaseAddress);
                _srcBaseAddress = NULL;
                _srcStart = _srcEnd = 0;
            }
            _dstBaseAddress = (LPBYTE) MapViewOfFile(_hFileMappingObject, FILE_MAP_WRITE, 0, _dstStart, _dstEnd-_dstStart);
            if (_dstBaseAddress == NULL)
            {
                // we need more room
                UnmapViewOfFile(_srcBaseAddress);
                _srcBaseAddress = NULL;
                _srcStart = _srcEnd = 0;
                _dstStart = _dstEnd = 0;
                return NULL;
            }
            return _dstBaseAddress+(start-_dstStart);
        }
    }
    
    LPBYTE Source(size_t start, size_t size)
    {
        size_t end = start+size;
        if ((start >= _idealStart) && (end <= _idealEnd))
        {
            if (_srcBaseAddress) 
            {
                UnmapViewOfFile(_srcBaseAddress);
                _srcBaseAddress = NULL;
                _srcStart = _srcEnd = 0;
            }
            return _idealBaseAddress+(start-_idealStart);
        }
        else if ((start >= _dstStart) && (end <= _dstEnd))
        {
            if (_srcBaseAddress) 
            {
                UnmapViewOfFile(_srcBaseAddress);
                _srcBaseAddress = NULL;
                _srcStart = _srcEnd = 0;
            }
            return _dstBaseAddress+(start-_dstStart);
        }
        else if ((start >= _srcStart) && (end <= _srcEnd))
        {
            return _srcBaseAddress+(start-_srcStart);
        }
        else
        {
            if (_srcBaseAddress) UnmapViewOfFile(_srcBaseAddress);
            _srcStart = DWORD(start) / _dwAllocationGranularity * _dwAllocationGranularity;
            _srcEnd = DWORD(end+_dwAllocationGranularity-1) / _dwAllocationGranularity * _dwAllocationGranularity;
            if (_srcEnd > _mapSize) _srcEnd = _mapSize;
            _srcBaseAddress = (LPBYTE) MapViewOfFile(_hFileMappingObject, FILE_MAP_WRITE, 0, _srcStart, _srcEnd-_srcStart);
            if (_srcBaseAddress == NULL)
            {
                // we need more room
                UnmapViewOfFile(_dstBaseAddress);
                _dstBaseAddress = NULL;
                _dstStart = _dstEnd = 0;
                _srcStart = _srcEnd = 0;
                return NULL;
            }
            return _srcBaseAddress+(start-_srcStart);
        }
    }
}

class MappingEntry
{
public:
    size_t  offset;
    size_t  size;
private:
    bool    fromMemory;
    union
    {
        LPBYTE  memory;
        size_t  mappingOffset;
    };
public:
    ~MappingEntry()
    {
        if (fromMemory) 
            free(memory);
    }
    bool MakeBackup()
    {
        DEBUGMEM(size);
        fromMemory = true;
        memory = (LPBYTE) malloc(size);
        if (memory == NULL) // out of memory !
        { 
            ViewManager::ClearIdeal(); // free up memory used by ideal mapping
            memory = (LPBYTE) malloc(size);
            if (memory == NULL) // it was not enough
                return false;
        }
        LPBYTE lpDst   = memory;
        size_t loffset = offset;
        size_t lsize   = size;
        size_t chunkSize = size;
        while (lsize > 0)
        {
            if (chunkSize > lsize) chunkSize = lsize;
            LPBYTE lpSrc = ViewManager::Source(loffset, chunkSize);
            if (lpSrc == NULL) { chunkSize /= 2; continue; }
#ifndef NO_CHANGES
            memcpy(lpDst, lpSrc, chunkSize);
#endif
            lpDst       += chunkSize;
            loffset     += chunkSize;
            lsize       -= chunkSize;
        }
        return true;
    }
    void Map(size_t outputOffset)
    {
        fromMemory = false;
        mappingOffset = outputOffset;
    }
    void Shorten(size_t moveSize)
    {
        size -= moveSize;
        if (fromMemory)
        {
            DEBUGMEM(0-moveSize);
            memory = (LPBYTE) realloc(memory, size);
        }
    }
    LPBYTE GetSource(size_t accessOffset, size_t accessSize)
    {
        if (fromMemory)
            return memory+(accessOffset-offset);
        else
            return ViewManager::Source(mappingOffset+(accessOffset-offset), accessSize);
    }
};

void memcpy_down(void *dst, const void *src, size_t count)
{
    __asm {
        mov     esi, src
        mov     edi, dst
        mov     ebx, count
        cmp     ebx, 8
        jb      ByteCopy
        lea     esi, [esi+ebx-1]
        lea     ecx, [edi+ebx]
        lea     edi, [ecx-1]
        and     ecx, 11b        ; check the alignment of destination
        sub     ebx, ecx
        std
        rep     movsb           ; copy 0..3 leading bytes to align edi on dwords
        sub     esi, 3
        sub     edi, 3
        mov     ecx, ebx
        shr     ecx, 2
        rep     movsd           ; copy dwords
        mov     ecx, ebx
        and     ecx, 11b
        add     esi, 3
        add     edi, 3
        rep     movsb           ; copy remaining trailing bytes
        cld
        jmp     Return

ByteCopy:
        mov     ecx, ebx
        lea     esi, [esi+ecx-1]
        lea     edi, [edi+ecx-1]
        std
        rep     movsb
        cld

Return:
    }
}

void UpdatePercent(size_t outputOffset, DWORD finalSize, UINT &old_percent)
{
    if (g_hwndStatic && (finalSize >= 100))
    {
        UINT percent = (UINT) (outputOffset/(finalSize/100));
        if (percent != old_percent)
        {
            old_percent = percent;
            wsprintfW(g_staticSuffix, L"%d%%", percent);
            SendMessageW(g_hwndStatic, WM_SETTEXT, NULL, (LPARAM) g_staticText);
#ifdef _DEBUG
            OutputDebugStringW(g_staticSuffix);
            OutputDebugStringW(L"\r\n");
#endif
        }
    }
}

// patch must have the same format as a single-file WPatch file, with the following differences
// first 2 DWORDs represent a FILETIME structure, instead of "VPAT" signature + number of patches
//          (this FILETIME structure must match the VFS time of creation)
// the DWORD used for first patch size is now the final size of the patched file

int DoPatchFile(HANDLE hPatch, HANDLE hFile, DWORD finalSize)
{
    FILETIME filetime;
    DWORD read;
    ReadFile(hPatch, &filetime, sizeof(filetime), &read, NULL);

    long patch_blocks;
    ReadFile(hPatch, &patch_blocks, sizeof(patch_blocks), &read, NULL);
    size_t outputOffset = 0;
    DEBUGMEM((patch_blocks*2+1)*sizeof(MappingEntry));
    MappingEntry *mappings = new MappingEntry[patch_blocks*2+1]; // each added mapping overrides previous ones on a given zone
    if (mappings == NULL)
        return PATCH_ERROR;
    
    int result = PATCH_CORRUPT; // default return value

    memset(mappings, 0, (patch_blocks*2+1)*sizeof(MappingEntry));

    DWORD sourceSize = GetFileSize(hFile, NULL);
#ifdef NO_CHANGES
    DWORD mapSize = sourceSize;
#else
    DWORD mapSize = max(sourceSize, finalSize);
#endif
    HANDLE hFileMappingObject = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, mapSize, NULL);
    ViewManager::Initialize(hFileMappingObject, mapSize);
    //LPBYTE lpBaseAddress = static_cast<LPBYTE>(MapViewOfFile(hFileMappingObject, FILE_MAP_WRITE, 0, 0, 0));
    //if (lpBaseAddress == NULL)
    //{
    //    CloseHandle(hFileMappingObject);
    //    return PATCH_ERROR;
    //}

    MappingEntry *lastMapping = mappings;
    lastMapping->offset = 0;
    lastMapping->size = sourceSize;
    lastMapping->Map(0);

    size_t offset;
    UINT old_percent = -1;

    while (patch_blocks--)
    {
        unsigned char blocktype = 0;
        size_t blocksize = 0;
        if (!ReadFile(hPatch, &blocktype, 1, &read, NULL))
             goto abort;
        switch (blocktype) {
        case 1: // block de data à recopier depuis le fichier en cours de patch
        case 2:
        case 3:
            if (blocktype == 1)
            { unsigned char x; blocksize = ReadFile(hPatch,&x,1,&read,NULL)? x: 0; }
            else if (blocktype == 2)
            { unsigned short x; blocksize = ReadFile(hPatch,&x,2,&read,NULL)? x: 0; }
            else
            { unsigned long x; blocksize = ReadFile(hPatch,&x,4,&read,NULL)? x:0; }

            if (!blocksize || !ReadFile(hPatch, &offset, 4, &read, NULL) || read != 4)
                 goto abort;

            if (offset == outputOffset)
            {
                outputOffset += blocksize;
                break; // perfect match, no bytes to move around
            }

            // prepare zone-backup mapping
            if (offset > outputOffset)
            {
                ViewManager::IdealMapping(outputOffset, offset+blocksize);
                lastMapping[1].offset = outputOffset;
                if (offset >= outputOffset+blocksize)
                    lastMapping[1].size = blocksize;
                else
                    lastMapping[1].size = offset-outputOffset;
            }
            else
            {
                ViewManager::IdealMapping(offset, outputOffset+blocksize);
                if (offset+blocksize < outputOffset)
                {
                    lastMapping[1].offset = outputOffset;
                    lastMapping[1].size = blocksize;
                }
                else
                {
                    lastMapping[1].offset = offset+blocksize;
                    lastMapping[1].size = outputOffset-offset;
                }
            }
            if (!lastMapping[1].MakeBackup())
            {
                result = PATCH_OUTOFMEMORY;
                goto abort;
            }
            
            // prepare a new mapping (that will become true after the copy)
            lastMapping[2].offset = offset;
            lastMapping[2].size = blocksize;
            lastMapping[2].Map(outputOffset);

            // copy old data to the new place
            if (offset > outputOffset)
            {
                size_t chunkSize = blocksize;
                while (blocksize > 0)
                {
                    if (chunkSize > blocksize) chunkSize = blocksize;
                    ViewManager::IdealMapping(outputOffset, offset+chunkSize, false);
                    LPBYTE lpDst = ViewManager::Destination(outputOffset, chunkSize);
                    if (lpDst == NULL) { chunkSize /= 2; continue; }
                    // if we are copying data from further in the file, this is unaltered data so we can copy it straight
                    LPBYTE lpSrc = ViewManager::Source(offset, chunkSize);
                    if (lpSrc == NULL) { chunkSize /= 2; continue; }
#ifndef NO_CHANGES
                    memcpy(lpDst, lpSrc, chunkSize);
#endif
                    offset       += chunkSize;
                    outputOffset += chunkSize;
                    blocksize    -= chunkSize;
                    UpdatePercent(outputOffset, finalSize, old_percent);
                }
            }
            else
            {
                // we are copying data from sooner in the file, so we need to go through the mappings to check where the data resides now
                // we are going to copy from right to left to prevent overwriting ourselves some parts we might need
                offset += blocksize;        // starting from now, offset is the ending offset of the block to copy
                outputOffset += blocksize;  // same for outputOffset
                while (blocksize != 0)
                {
                    size_t moveSize = blocksize;
                    for (MappingEntry *mapping = lastMapping; mapping >= mappings ; mapping-- )
                    {
                        if (offset > mapping->offset)
                        {
                            if (offset <= mapping->offset+mapping->size)
                            {
                                // this mapping has the data which compose the end (or maybe whole) of our needed block
                                if (moveSize > offset-mapping->offset)
                                    moveSize = offset-mapping->offset;

                                size_t chunkSize = moveSize;
                                while (moveSize > 0)
                                {
                                    if (chunkSize > moveSize) chunkSize = moveSize;
                                    ViewManager::IdealMapping(offset-chunkSize, outputOffset, false);
                                    LPBYTE lpDst = ViewManager::Destination(outputOffset-chunkSize, chunkSize);
                                    if (lpDst == NULL) { chunkSize /= 2; continue; }
                                    LPBYTE lpSrc = mapping->GetSource(offset-chunkSize, chunkSize);
                                    if (lpSrc == NULL) { chunkSize /= 2; continue; }
#ifndef NO_CHANGES
                                    // note: cannot use memmove here because the memory block can overlap while being mapped at different adresses
                                    if ((outputOffset <= offset) || (outputOffset >= offset+chunkSize))
                                        memcpy(lpDst, lpSrc, chunkSize);
                                    else
                                        memcpy_down(lpDst, lpSrc, chunkSize); // downwards copy to avoid propagation
#endif

                                    if (offset == mapping->offset+mapping->size) // end of this mapping was copied to current output
                                    {
                                        // so we can shorten this mapping (and maybe release some memory)
                                        mapping->Shorten(chunkSize);
                                    }

                                    offset       -= chunkSize;
                                    outputOffset -= chunkSize;
                                    blocksize    -= chunkSize;
                                    moveSize     -= chunkSize;
                                    UpdatePercent(outputOffset+lastMapping[2].size-2*blocksize, finalSize, old_percent);
                                }

                                break;
                            }
                            else
                            {
                                // this mapping comes before the end of our needed block, remember to not copy too much data from elsewhere
                                // (some of which data might reside in this mapping)
                                if (moveSize > offset-(mapping->offset+mapping->size))
                                    moveSize = offset-(mapping->offset+mapping->size);
                            }
                        }
                    }
                }
                // as we copied from right to left, outputOffset is now at the beginning, so move it back to end of output block
                outputOffset += lastMapping[2].size;
            }

            lastMapping += 2;
            break;

        case 5: // insertion d'un block de data depuis le fichier de patch
        case 6:
        case 7:
            if (blocktype == 5)
            { unsigned char x; blocksize = ReadFile(hPatch,&x,1,&read,NULL)? x:0; }
            else if (blocktype == 6)
            { unsigned short x; blocksize = ReadFile(hPatch,&x,2,&read,NULL)? x:0; }
            else
            { unsigned long x; blocksize = ReadFile(hPatch,&x,4,&read,NULL)? x:0; }

            if (!blocksize) goto abort;

            {
                ViewManager::IdealMapping(outputOffset, outputOffset+blocksize);
                lastMapping++;
                lastMapping->offset = outputOffset;
                lastMapping->size = blocksize;
                // note: MakeBackup can ClearIdeal so all ViewManager pointers could become
                //  invalid if they were pointing to ideal zone
                if (!lastMapping->MakeBackup())
                {
                    result = PATCH_OUTOFMEMORY;
                    goto abort;
                }

                size_t chunkSize = blocksize;
                while (blocksize > 0)
                {
                    if (chunkSize > blocksize) chunkSize = blocksize;
                    LPBYTE lpDst = ViewManager::Destination(outputOffset, chunkSize);
                    if (lpDst == NULL) { chunkSize /= 2; continue; }
#ifdef NO_CHANGES
                    SetFilePointer(hPatch, (DWORD) chunkSize, NULL, FILE_CURRENT);
#else
                    ReadFile(hPatch, lpDst, (DWORD) chunkSize, &read, NULL);
#endif
                    outputOffset    += chunkSize;
                    blocksize       -= chunkSize;
                }
            }
            break;

        default:
            goto abort;
        }

        UpdatePercent(outputOffset, finalSize, old_percent);
    }
    result = PATCH_SUCCESS;

abort:
    delete [] mappings;
    ViewManager::Terminate();
    CloseHandle(hFileMappingObject);
    if (result == PATCH_SUCCESS)
    {
        SetFilePointer(hFile, finalSize, NULL, SEEK_SET);
#ifndef NO_CHANGES
        SetEndOfFile(hFile);
#endif
        SetFileTime(hFile, NULL, NULL, &filetime);
    }
    return result;
}

int DoPatch(HANDLE hPatch, HANDLE hFile, bool hugeFile, bool check, DWORD preciseOffset) {

    unsigned long temp = 0;
    unsigned long read;
    unsigned long source_crc = 0;
    md5_byte_t source_md5[16];
    unsigned long patch_dest_crc = 0;
    md5_byte_t patch_dest_md5[16];
    int MD5Mode = 0;
    unsigned long patches = 0;
    int already_uptodate = 0;

    FILETIME targetModifiedTime;

    // target file date is by default the current system time
    GetSystemTimeAsFileTime(&targetModifiedTime);

    SetFilePointer(hPatch, preciseOffset, NULL, FILE_BEGIN);
	if (preciseOffset == 0)
	{
		ReadFile(hPatch, &temp, 4, &read, NULL);
		if (temp != MAGIC_VPAT)
			return PATCH_CORRUPT;
		ReadFile(hPatch, &temp, 4, &read, NULL);
		if (temp != FORMAT_VERSION)
			return PATCH_ERROR;
		// read the number of patches in the file
	    ReadFile(hPatch, &patches, 4, &read, NULL);
	}
	else
		patches = 1;

    if (!(hugeFile ? FileSplitMD5 : FileMD5)(hFile, source_md5))
        return PATCH_ERROR;

    while (patches--)
    {
        long patch_blocks = 0, bodySize = 0;

        // flag which needs to be set by one of the checksum checks
        int currentPatchMatchesChecksum = 0;

        long file_flags, target_size;
        if(!ReadFile(hPatch, &file_flags, 4, &read, NULL))
            return PATCH_CORRUPT;
        if(!ReadFile(hPatch, &target_size, 4, &read, NULL))
            return PATCH_CORRUPT;

        // read checksums

        int md5index;
        md5_byte_t patch_source_md5[16];
        if (!ReadFile(hPatch, patch_source_md5, 16, &read, NULL))
            return PATCH_CORRUPT;
        if (!ReadFile(hPatch, patch_dest_md5, 16, &read, NULL))
            return PATCH_CORRUPT;

        if (((file_flags & FILEFLAG_SPLIT_MD5) != 0) == hugeFile)
        {
            // check to see if it's already up-to-date for some patch
            for (md5index = 0; md5index < 16; md5index++) {
                if(source_md5[md5index] != patch_dest_md5[md5index]) break;
                if(md5index == 15) already_uptodate = 1;
            }
            for (md5index = 0; md5index < 16; md5index++) {
                if(source_md5[md5index] != patch_source_md5[md5index]) break;
                if(md5index == 15) currentPatchMatchesChecksum = 1;
            }
        }
        // read the size of the patch, we can use this to skip over it
        if (!ReadFile(hPatch, &bodySize, 4, &read, NULL))
            return PATCH_CORRUPT;

        if (currentPatchMatchesChecksum)
        {
            if (!check)
            {
                DoPatchFile(hPatch, hFile, target_size);
                int md5index;
                md5_byte_t dest_md5[16];
                if (!(hugeFile ? FileSplitMD5 : FileMD5)(hFile, dest_md5))
                    return PATCH_ERROR;
                for (md5index = 0; md5index < 16; md5index++)
                    if(dest_md5[md5index] != patch_dest_md5[md5index])
                        return PATCH_ERROR;
            }
            return PATCH_SUCCESS;
        }
        else
        {
            SetFilePointer(hPatch, sizeof(FILETIME)+sizeof(DWORD)+bodySize, NULL, FILE_CURRENT);
        }
    }

    // if already up to date, it doesn't matter that we didn't match
    if (already_uptodate)
        return PATCH_UPTODATE;  
    else
        return PATCH_NOMATCH;
}
