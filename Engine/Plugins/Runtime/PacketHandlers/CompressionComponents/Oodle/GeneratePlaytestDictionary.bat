@echo off

set /p GameName=Type the name of the game you are working with: 
echo.

set /p Directory=Type the root directory of the zipped captures: 
echo.

set /p ChangelistFilter=Type a changelist number to filter by, or all for all files: 
echo.

echo %Directory%| call ExtractAllArchives.bat

set DictionaryOutput=Input
set FileFilter=Input
set DirectoryRoot=%Directory%

(echo %GameName%
echo %DictionaryOutput%
echo %FileFilter%
echo %ChangelistFilter%
echo %Directory%) | call BuildDictionary_Generic.bat

set DictionaryOutput=Output
set FileFilter=Output

(echo %GameName%
echo %OutputString%
echo %OutputString%
echo %ChangelistFilter%
echo %DirectoryRoot%)| call BuildDictionary_Generic.bat
