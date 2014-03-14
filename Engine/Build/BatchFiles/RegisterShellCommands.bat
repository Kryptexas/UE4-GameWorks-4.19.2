@echo off

pushd ..\..\..\
set UE_ROOT_DIR=%CD:\=\\%
set UE_ROOT_DIR_SINGLE_SLASHES=%CD%
popd

reg delete HKEY_CLASSES_ROOT\.uproject /f
reg add HKEY_CLASSES_ROOT\.uproject /d "Unreal.ProjectFile"
reg add HKEY_CLASSES_ROOT\.uproject /v "Content Type" /t REG_SZ /d "text/Unreal"
reg add HKEY_CLASSES_ROOT\.uproject /v "PerceivedType" /t REG_SZ /d "Text"

reg delete HKEY_CLASSES_ROOT\Unreal.ProjectFile /f
reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile /d "Unreal Project File"
reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\DefaultIcon /d "%UE_ROOT_DIR%\\Engine\\Binaries\\Win64\\UE4Editor.exe"

reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\open /d "Open"
reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\open\command /t REG_SZ /d "\"%UE_ROOT_DIR%\\Engine\\Binaries\\Win64\\UE4Editor.exe\" \"%%1\""

reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\run /d "Launch Game"
reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\run\command /t REG_SZ /d "\"%UE_ROOT_DIR%\\Engine\\Binaries\\Win64\\UE4Editor.exe\" \"%%1\" -game"

reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj /d "Generate Visual Studio projects"
reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj\command /t REG_EXPAND_SZ /d "%%comspec%% /c \"%UE_ROOT_DIR_SINGLE_SLASHES%\Engine\Build\BatchFiles\GenerateProjectFiles.bat -project=\"%%1\" -game -engine\""

