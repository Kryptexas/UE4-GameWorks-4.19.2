@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

SET python_dest_name=Win64
SET python_src_name=Python27
SET python_src_dir=C:\%python_src_name%

IF NOT EXIST %python_src_dir% (
	ECHO Python Source Directory Missing: %python_src_dir%
	GOTO End
)

IF EXIST %python_dest_name% (
	ECHO Removing Existing Target Directory: %python_dest_name%
	RMDIR "%python_dest_name%" /s /q
)

ECHO Copying Python: %python_src_dir%
XCOPY /s /i /q "%python_src_dir%" "%python_dest_name%"
XCOPY /s /i /q "NoRedist\%python_dest_name%\Microsoft.VC90.CRT" "%python_dest_name%"
XCOPY /q "NoRedist\TPS\PythonWinBin.tps" "%python_dest_name%"
XCOPY /q "NoRedist\TPS\VisualStudio2008.tps" "%python_dest_name%"
XCOPY /q "%WINDIR%\System32\python27.dll" "%python_dest_name%"
RMDIR "%python_dest_name%\Doc" /s /q
DEL "%python_dest_name%\w9xpopen.exe"

:End

PAUSE
