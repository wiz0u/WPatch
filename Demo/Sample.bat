@path ..\..\bin
@del WGenPatDir.pat 2> NUL
@mkdir dir1\removedDir\removedNoFiles 2> NUL
@mkdir dir1\removedDirEmpty 2> NUL
@mkdir dir2\newDir\newDirNoFiles 2> NUL
@mkdir dir2\newDirEmpty 2> NUL
WGenPatDir.exe --precise --exclude *.tmp;.svn dir1 dir2
@echo.
@echo You can now compile Sample.nsi with NSIS
@echo.
@pause
