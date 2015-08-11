@echo off
setlocal

:args
if "%1" == "-c" set THIRD_PARTY_CHANGELIST=-c %2
if "%1" == "" goto start
shift
goto args

:start

p4 edit %THIRD_PARTY_CHANGELIST% PhysX-3.3/*
p4 edit %THIRD_PARTY_CHANGELIST% PhysX-3.3/Source/compiler/...
p4 edit %THIRD_PARTY_CHANGELIST% APEX-1.3/*
p4 edit %THIRD_PARTY_CHANGELIST% APEX-1.3/compiler/...

xcopy /I /Y /S /D ".\..\NotForLicensees\PhysX\PhysX_3.3\buildtools\*" ".\PhysX-3.3\buildtools"
xcopy /I /Y /S /D ".\..\NotForLicensees\PhysX\PhysX_3.3\source\compiler\xpj\*" ".\PhysX-3.3\source\compiler\xpj"
xcopy /I /Y /S /D ".\..\NotForLicensees\PhysX\APEX_1.3_vs_PhysX_3.3\compiler\xpj\*" ".\APEX-1.3\compiler\xpj"
xcopy /Y /D ".\..\NotForLicensees\PhysX\PhysX_3.3\*" ".\PhysX-3.3\"
xcopy /Y /D ".\..\NotForLicensees\PhysX\APEX_1.3_vs_PhysX_3.3\*" ".\APEX-1.3\"

set GENERATE_IN_MAIN=1

pushd PhysX-3.3\source\compiler\xpj
call create_projects.cmd
popd

pushd APEX-1.3\compiler\xpj
call create_projects.cmd
popd

p4 revert -a %THIRD_PARTY_CHANGELIST%

endlocal