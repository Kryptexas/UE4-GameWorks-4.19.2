@echo off

for /f "tokens=2,*" %%a in ('reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Personal ^| findstr Personal') do (
	set UE4_MyDocs=%%b
	
	if "%VS120COMNTOOLS%" == "" goto NoVisualStudio2013Environment
	attrib -R "!UE4_MyDocs!\Visual Studio 2013\Visualizers\UE4.natvis" 1>nul 2>nul
	copy /Y "%~dp0..\..\Extras\VisualStudioDebugging\UE4.natvis" "!UE4_MyDocs!\Visual Studio 2013\Visualizers\UE4.natvis" 1>nul 2>nul

:NoVisualStudio2013Environment
	if "%VS110COMNTOOLS%" == "" goto NoVisualStudio2012Environment
	attrib -R "!UE4_MyDocs!\Visual Studio 2012\Visualizers\UE4.natvis" 1>nul 2>nul
	copy /Y "%~dp0..\..\Extras\VisualStudioDebugging\UE4.natvis" "!UE4_MyDocs!\Visual Studio 2012\Visualizers\UE4.natvis" 1>nul 2>nul

:NoVisualStudio2012Environment
	set UE4_MyDocs=
)
