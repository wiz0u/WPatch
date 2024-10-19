WGenPatDir - Automate Directory Updater based on NSIS & WPatch
=========================================================================
Web site: http://wizou.fr

The WGenPatDir utility recursively compares two directory structures
looking for changes to the files and subdirectories. It produces files
containing instructions that will make your NSIS installer perform a patch 
upgrade from the original structure to the new.

WGenPatDir uses the WPatch "genpat.exe" utility to generate the patch file.
This binary file contain the delta between the old and new version of an
individual file. When applied to the old file, the file differences are 
applied and the file is converted to its new contents.

WGenPatDir is an original creation, even if the command-line interface and the
functionnalities have been inspired by NsisPatchGen by Vibration Technology Ltd.
WGenPatDir is supposedly faster and more efficient


Using WGenPatDir
------------------

WGenPatDir recursively diffs two sets of directories to calculate the delta from
one to the other, and to produce a patch installer that will upgrade from one to 
the other.

- "directory1" is the top level directory containing the old version of the 
software. This should contain the files in the state they would be installed on 
a target machine. 

- "directory2" is the equivalent top level directory containing the update version
of the same software. 

Make sure WGenPat.exe is available in the current directory or the PATH environment variable.
Run: 

  WGenPatDir <directory1> <directory2>

This will produce in the current directory a "WGenPatDir.pat" binary file and
an NSIS script file "WGenPatDir.nsh" containing a Section to execute to apply the patch

Notes:
- to keep running fast, WGenPatDir won't notice files which have exactly the
	same size and date/time but a different content
- you can override (predefine) the ADDEDSOURCE define if the source directory
	changes	between the running of WGenPatDir and the building of the installer
- your installer script must include WGenPatDir.nsh where you want the patch Section to
    execute :
		* call InitPluginsDir in a Section prior to the patch Section
		* include WGenPatDir.nsh (outside your Sections)
		* check for IfErrors in the Section immediately after, in order to be certain
		   the update was successful
	See Sample.nsi
- WGenPatDir has several command-line options. Run program without arguments to learn about them.


LICENSE
=======

WGenPatDir
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
