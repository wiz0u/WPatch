//---------------------------------------------------------------------------
// main.cpp
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

#ifdef __BORLANDC__
#pragma argsused
#endif

#if !defined(__BORLANDC__) && !defined(_MSC_VER)
#include <unistd.h>
#endif

#include "GlobalTypes.h"
#include "POSIXUtil.h"
#include "Checksums.h"
#include "PatchGenerator.h"
#include "FileFormat1.h"

#include <fstream>
#include <sstream>

int _tmain( int argc, TCHAR * argv[] )
{
	cout << _T("WGenPat v4.00\n");
	_T("============\n")
		_T("\n")
		_T("(c) 2001-2005 Van de Sande Productions http://www.tibed.net/vpatch\n")
		_T("Adapted for WPatch by Olivier Marcoux  http://wizou.fr\n")
		_T("\n");
	
	tstring sourceFileName;
	tstring targetFileName;
	tstring patchFileName;
	
	bool showHelp = true;
	
	int blockSize = 64;
	int maxMatches = 500;
	bool beVerbose = false;
	bool beOptimal = false;
	bool useSplitMD5 = false;
	bool modePrecise = false;
	int fileNameArgument = 0;
	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			tstring s(argv[i]);
			if (s.size() > 0)
			{
				if ((s[0] == '-')
#ifdef __WIN32__
				 || (s[0] == '/')
#endif
					)
				{
					if(s.size() > 1)
					{
						if (toupper(s[1]) == _T('V'))
						{
							beVerbose = true;
						}
						if (toupper(s[1]) == _T('O'))
						{
							beOptimal = true;
						}
						if (toupper(s[1]) == _T('H'))
						{
							useSplitMD5 = true;
						}
						if (toupper(s[1]) == _T('P'))
						{
							modePrecise = true;
						}
					}
					if (s.size() > 2)
					{
						if (toupper(s[1]) == _T('B'))
						{
							if (s[2] == _T('='))
							{
								tistringstream ss(s.substr(3));
								ss >> blockSize;
							}
						}
						if (toupper(s[1]) == _T('A'))
						{
							if (s[2] == _T('='))
							{
								tistringstream ss(s.substr(3));
								ss >> maxMatches;
							}
						}
					}
				}
				else
				{
					switch (fileNameArgument)
					{
					case 0:
						sourceFileName = s;
						break;
					case 1:
						targetFileName = s;
						break;
					case 2:
						patchFileName = s;
						showHelp = false;
						break;
					default:
						terr << _T("WARNING: extra filename argument not used: ") << s << _T("\n");
					}
					fileNameArgument++;
				}
			}
		}
	}
	if (beOptimal)
		maxMatches = 0;
	if (showHelp)
	{
		tout << _T("This program will take (sourcefile) as input and create a (patchfile).\n");
		tout << _T("With this patchfile, you can convert a (sourcefile) into (targetfile).\n\n");
		tout << _T("Command line info:\n");
		tout << _T("  WGENPAT (sourcefile) (targetfile) (patchfile)\n\n");
		
		tout << _T("Command line option (optional):\n");
	    tout << _T("-P        Use fast and precise update mode. See exit code\n");
		tout << _T("-H        Huge file: Use MD5 checksum over first and last 64K only\n");
		tout << _T("-B=64     Set blocksize (default=64), multiple of 2 is required.\n");
		tout << _T("-V        More verbose information during patch creation.\n");
		tout << _T("-O        Deactivate match limit of the -A switch (sometimes smaller patches).\n");
		tout << _T("-A=500    Maximum number of block matches per block (improves performance).\n");
		tout << _T("          Default is 500, larger is slower. Use -V to see the cut-off aborts.\n\n");
		tout << _T("Note: filenames should never start with - character!\n\n");
		tout << _T("Possible exit codes:\n");
		tout << _T("  0  Success\n");
		tout << _T("  1  Arguments missing\n");
		tout << _T("  2  Other error\n");
		tout << _T("  3  Source file already has a patch in specified patch file (=error)\n");
		tout << _T(" 12+ Not an error in precise mode: Offset to the patch information\n");
		return 1;
	}
	
	tout << _T("[Source] ") << sourceFileName.c_str() << _T("\n");
	tout << _T("[Target] ") << targetFileName.c_str() << _T("\n[PatchFile] ") << patchFileName.c_str() << _T("\n");
	
	// get the file sizes
	TFileOffset sourceSize = 0;
	try
	{
		sourceSize = POSIX::getFileSize(sourceFileName.c_str());
	}
	catch(TCHAR* s)
	{
		terr << _T("Source file size reading failed: ") << s << _T("\n");
		return 2;
	}
	TFileOffset targetSize;
	try {
		targetSize = POSIX::getFileSize(targetFileName.c_str());
	}
	catch(const TCHAR* s)
	{
		terr << _T("Target file size reading failed: ") << s << _T("\n");
		return 2;
	}
	
	// calculate CRCs
	TChecksum sourceCRC(useSplitMD5 ? TChecksum::SPLIT_MD5 : TChecksum::MD5);
	sourceCRC.LoadFile(sourceFileName);
	TChecksum targetCRC(useSplitMD5 ? TChecksum::SPLIT_MD5 : TChecksum::MD5);
	targetCRC.LoadFile(targetFileName);
	
	if(sourceCRC == targetCRC)
	{
		terr << _T("Source and target file have equal CRCs!");
		return 2;
	}
	
	// open the files
	bifstream source;
	source.open(sourceFileName.c_str(), std::ios_base::binary | std::ios_base::in);
	bifstream target;
	target.open(targetFileName.c_str(), std::ios_base::binary | std::ios_base::in);
	bfstream patch;
	patch.open(patchFileName.c_str(), std::ios_base::binary | std::ios_base::in | std::ios_base::out);
	if (patch.fail())
	{
		patch.clear();
		patch.open(patchFileName.c_str(), std::ios_base::binary | std::ios_base::out);
		patch.close();
		patch.open(patchFileName.c_str(), std::ios_base::binary | std::ios_base::in | std::ios_base::out);
		if (patch.good())
			FileFormat1::createEmptyPatch(patch);
	}
	else
	{
		// this will remove previous patch for "sourceCRC" if already inside
		TFileOffset patchOffset = FileFormat1::checkExistingPatch(patch, sourceCRC, targetCRC, modePrecise);
		if (patchOffset >= 12)
		{
			tout << _T("Skipping: Same source/target patch already included!\n");
			return modePrecise ? patchOffset : 0;
		}
		else switch (patchOffset)
		{
		case 0: // no problem.. ready to generate patch
			break;
		case 3:
		default:
			terr << _T("ERROR: Source file with the exact same contents already exists in patch!\nThis is only possible to patch with precise mode enabled.\n");
			return 3;
		}
	}
	
	if (source.good() && target.good() && patch.good())
	{
		TFileOffset patchOffset = patch.tellg();
		PatchGenerator* gen = new PatchGenerator(source, sourceSize, target, targetSize);
		try
		{
			// clean up the blocksize to be a multiple of 2
			int orgBlockSize = blockSize;
			int bs_counter = 0;
			while(blockSize != 0)
			{
				bs_counter++;
				blockSize >>= 1;
			}
			blockSize = 1;
			while(bs_counter != 0)
			{
				blockSize <<= 1;
				bs_counter--;
			}
			if((blockSize >> 1) == orgBlockSize) blockSize = orgBlockSize;
			if(blockSize != orgBlockSize)
				tout << _T("[BlockSizeFix] Your blocksize had to be fixed since it is not a multiple of 2\n");
			if(blockSize < 16)
			{
				blockSize = 16;
				tout << _T("[BlockSizeFix] Your blocksize had to be fixed since it is smaller than 16\n");
			}
			
			gen->blockSize = blockSize;
			tout << _T("[BlockSize] ") << static_cast<unsigned int>(gen->blockSize) << _T(" bytes\n");
			
			gen->maxMatches = maxMatches;
			if(gen->maxMatches == 0)
				tout << _T("[FindBlockMatchLimit] Unlimited matches\n");
			else
				tout << _T("[FindBlockMatchLimit] ") << gen->maxMatches << _T(" matches\n");
			
			gen->beVerbose = beVerbose;
			if(beVerbose)
				tout << _T("[Debug] Verbose output during patch generation activated.\n");
			
			// create sameBlock storage
			vector<SameBlock*> sameBlocks;
			// run the patch generator to find similar blocks
			gen->execute(sameBlocks);
			// construct the actual patch in FileFormat1
			FileFormat1::writePatch(patch, target, targetSize, sameBlocks, sourceCRC, targetCRC, POSIX::getFileTime(targetFileName.c_str()));
			// cleanup sameblocks
			for(vector<SameBlock*>::iterator iter = sameBlocks.begin(); iter != sameBlocks.end(); iter++)
			{
				delete *iter;
				*iter = NULL;
			}
		}
		catch(tstring s)
		{
			terr << _T("Error thrown: ") << s.c_str();
			return 2;
		}
		catch(const TCHAR* s)
		{
			terr << _T("Error thrown: ") << s;
			return 2;
		}
		return modePrecise ? patchOffset : 0;
	}
	else
	{
		terr << _T("There was a problem opening the files.\n");
		return 2;
	}
}
