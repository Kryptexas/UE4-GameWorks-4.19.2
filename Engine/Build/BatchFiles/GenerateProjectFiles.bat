@echo off

echo Setting up Unreal Engine 4 project files...

rem ## Unreal Engine 4 Visual Studio project setup script
rem ## Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

rem ## This script is expecting to exist in the UE4 root directory.  It will not work correctly
rem ## if you copy it to a different location and run it.

rem ## First, make sure the batch file exists in the folder we expect it to.  This is necessary in order to
rem ## verify that our relative path to the /Engine/Source directory is correct
if not exist "%~dp0..\..\Source" goto Error_BatchFileInWrongLocation


rem ## Change the CWD to /Engine/Source.  We always need to run UnrealBuildTool from /Engine/Source!
pushd "%~dp0..\..\Source"
if not exist ..\Build\BatchFiles\GenerateProjectFiles.bat goto Error_BatchFileInWrongLocation


rem ## Check to make sure that we have a Binaries directory with at least one dependency that we know that UnrealBuildTool will need
rem ## in order to run.  It's possible the user acquired source but did not download and unpack the other prerequiste binaries.
if not exist ..\Binaries\DotNET\RPCUtility.exe goto Error_MissingBinaryPrerequisites


rem ## Check to see if we're already running under a Visual Studio environment shell
if not "%INCLUDE%" == "" if not "%LIB%" == "" goto ReadyToCompile


rem ## Check for Visual Studio 2013
if "%VS120COMNTOOLS%" == "" goto NoVisualStudio2013Environment
call "%VS120COMNTOOLS%/../../VC/bin/x86_amd64/vcvarsx86_amd64.bat" >NUL
goto ReadyToCompile


rem ## Check for Visual Studio 2012
:NoVisualStudio2013Environment
if "%VS110COMNTOOLS%" == "" goto NoVisualStudioEnvironment
call "%VS110COMNTOOLS%/../../VC/bin/x86_amd64/vcvarsx86_amd64.bat" >NUL
goto ReadyToCompile


rem ## User has no version of Visual Studio installed?
goto Error_NoVisualStudioEnvironment


:ReadyToCompile
msbuild /nologo /verbosity:quiet Programs\UnrealBuildTool\UnrealBuildTool.csproj /property:Configuration=Development /property:Platform=AnyCPU
if not %ERRORLEVEL% == 0 goto Error_UBTCompileFailed

rem ## Run UnrealBuildTool to generate Visual Studio solution and project files
rem ## NOTE: We also pass along any arguments to the GenerateProjectFiles.bat here
..\Binaries\DotNET\UnrealBuildTool.exe -ProjectFiles %*
if not %ERRORLEVEL% == 0 goto Error_ProjectGenerationFailed

rem ## Success!
goto Exit


:Error_BatchFileInWrongLocation
echo.
echo GenerateProjectFiles ERROR: The batch file does not appear to be located in the /Engine/Build/BatchFiles directory.  This script must be run from within that directory.
echo.
pause
goto Exit


:Error_MissingBinaryPrerequisites
echo.
echo GenerateProjectFiles ERROR: It looks like you're missing some files that are required in order to generate projects.  Please check that you've downloaded and unpacked the engine source code, binaries, content and third-party dependencies before running this script.
echo.
pause
goto Exit


:Error_NoVisualStudioEnvironment
echo.
echo GenerateProjectFiles ERROR: We couldn't find a valid installation of Visual Studio.  This program requires either Visual Studio 2013 or Visual Studio 2012.  Please check that you have Visual Studio installed, then verify that the VS120COMNTOOLS environment variable is set.  Visual Studio configures this environment variable when it is installed, and this program expects it to be set to the '\Common7\Tools\' sub-folder under a valid Visual Studio installation directory.
echo.
pause
goto Exit


:Error_UBTCompileFailed
echo.
echo GenerateProjectFiles ERROR: UnrealBuildTool failed to compile.
echo.
pause
goto Exit


:Error_ProjectGenerationFailed
echo.
echo GenerateProjectFiles ERROR: UnrealBuildTool was unable to generate project files.
echo.
pause
goto Exit


:Exit
rem ## Restore original CWD in case we change it
popd

