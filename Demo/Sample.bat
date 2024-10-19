@path ..\..\bin
@del WGenPatDir.pat 2> NUL
WGenPatDir.exe --precise --exclude *.tmp;.svn dir1 dir2
@echo.
@echo You can now compile Sample.nsi with NSIS
@echo.
@pause
