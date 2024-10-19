for %%I in (.) do set NAME=%%~nI
set ZIP="%ProgramFiles%\7-Zip\7z.exe" a -tzip -xr!.svn\ "%CD%\_%NAME%.zip" 
del _%NAME%.zip

%ZIP% *.txt
%ZIP% .\Demo\Sample.bat
%ZIP% .\Demo\Sample.nsi
%ZIP% .\Demo\dir1
%ZIP% .\Demo\dir2
%ZIP% ..\bin\%NAME%.dll
%ZIP% ..\bin\%NAME%W.dll
%ZIP% ..\bin\WGenPat.exe
%ZIP% ..\bin\WGenPatDir.exe
_WPatch.zip
