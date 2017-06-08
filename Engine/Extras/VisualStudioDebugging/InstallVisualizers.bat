@echo off
set FoundVSInstance=0

rem Change to directory where batch file is located.  We'll restore this later with "popd"
pushd %~dp0

if "%VS140COMNTOOLS%" neq "" (
  set FoundVSInstance=1
  mkdir "%USERPROFILE%\Documents\Visual Studio 2015\Visualizers"
  copy UE4.natvis "%USERPROFILE%\Documents\Visual Studio 2015\Visualizers"
  copy UE4_Android_Nsight.dat "%VS140COMNTOOLS%\..\IDE\Extensions\NVIDIA\Nsight Tegra\3.4\Debuggers\Visualizers"
  echo Installed visualizers for Visual Studio 2015
)

if "%VS120COMNTOOLS%" neq "" (
  set FoundVSInstance=1
  mkdir "%USERPROFILE%\Documents\Visual Studio 2013\Visualizers"
  copy UE4.natvis "%USERPROFILE%\Documents\Visual Studio 2013\Visualizers"
  echo Installed visualizers for Visual Studio 2013
)

if "%VS110COMNTOOLS%" neq "" (
  set FoundVSInstance=1
  mkdir "%USERPROFILE%\Documents\Visual Studio 2012\Visualizers"
  copy UE4.natvis "%USERPROFILE%\Documents\Visual Studio 2012\Visualizers"
  echo Installed visualizers for Visual Studio 2012
)

if "%SCE_ORBIS_SDK_DIR%" neq "" (
  mkdir "%USERPROFILE%\Documents\SCE\orbis-debugger"
  copy PS4UE4.natvis "%USERPROFILE%\Documents\SCE\orbis-debugger"
  echo Installed Visualizers for Orbis
)

if "%FoundVSInstance%" equ "0" (
  echo ERROR: Could not find a valid version of Visual Studio installed (2012, 2013, 2015)
)

popd
pause
exit /b