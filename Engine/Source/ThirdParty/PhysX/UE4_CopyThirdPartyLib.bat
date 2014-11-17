rem Parse args
@echo off
setlocal
set changeList=
set compile=
set VS=VS2013
set vc=vc12win64
set local=
:ArgsLoop
REM parse args
if "%1"=="" goto Main
if "%1"=="-c" set changeList=-c %2
if "%1"=="-compile" set compile=1
if "%1"=="-local" set local=1
if "%1"=="-2012" (
    set VS=VS2012
    set vc=vc11win64
)
shift
goto :ArgsLoop
:Main

rem Find UE4_ROOT by searching backwards from cwd for PhysXSDK
set UE4_ROOT=%~dp0
:rootloop
if exist %UE4_ROOT%Engine\Build\BatchFiles goto :haverootpath
set UE4_ROOT=%UE4_ROOT%..\
goto :rootloop

:haverootpath


rem Find Physx by searching backwards from cwd for Physx folder
set PhysxRoot=%UE4_ROOT%
:physxRootLoop
if exist %PhysxRoot%PhysX\Epic goto :havePhysxRoot
set PhysxRoot=%PhysxRoot%..\
goto :physxRootLoop

:havePhysxRoot
set PhysxRoot=%PhysxRoot%Physx\

set VS=VS2013

if "%compile%" == "1" (
    set local=1
    set compile=-config debug -platformPhysx %vc%;x64 -platformAPEX %vc%-PhysX_3.3;x64
)

REM checkout libs and bins
pushd %UE4_ROOT%
rem checkout binaries from main
pushd %PhysxRoot%
call UE4_BuildThirdPartyLib.bat %changeList% %compile%
popd

echo "back in copy script"

set Physx33=%PhysxRoot%Epic\PhysX_32\
set PhysxVersion=%PhysxRoot%Epic\Current\

set LocalSource=Engine\Source\ThirdParty\PhysX\
set LocalBin=Engine\Binaries\ThirdParty\PhysX\
set LocalAPEXLib=%LocalSource%APEX-1.3\lib\Win64\%VS%
set LocalAPEXBin=%LocalBin%APEX-1.3\Win64\%VS%
set LocalPhysXLib=%LocalSource%PhysX-3.3\lib\Win64\%VS%
set LocalPhysXBin=%LocalBin%PhysX-3.3\Win64\%VS%

if "%local%"=="1" (
    set BranchAPEXLib=%Physx33%APEX_1.3_vs_PhysX_3.3\lib\%vc%-PhysX_3.3
    set BranchAPEXBin=%Physx33%APEX_1.3_vs_PhysX_3.3\bin\%vc%-PhysX_3.3
    set BranchPhysXLib=%Physx33%PhysX_3.3\lib\%vc%
    set BranchPhysXBin=%Physx33%PhysX_3.3\bin\%vc%
) else (
    set BranchAPEXLib=%PhysxVersion%APEXLib\%vc%-PhysX_3.3
    set BranchAPEXBin=%PhysxVersion%APEXBin\%vc%-PhysX_3.3
    set BranchPhysXLib=%PhysxVersion%PhysxLib\%vc%
    set BranchPhysXBin=%PhysxVersion%PhysxBin\%vc%
)

p4 edit %changeList% %LocalAPEXLib%\...
p4 edit %changeList% %LocalAPEXBin%\...
p4 edit %changeList% %LocalPhysXLib%\...
p4 edit %changeList% %LocalPhysXBin%\...

xcopy %BranchAPEXLib% %LocalAPEXLib% /i /S /Y
xcopy %BranchAPEXBin% %LocalAPEXBin% /i /S /Y
xcopy %BranchPhysXLib% %LocalPhysXLib% /i /S /Y
xcopy %BranchPhysXBin% %LocalPhysXBin% /i /S /Y

p4 revert -a %changeList%

popd

endlocal