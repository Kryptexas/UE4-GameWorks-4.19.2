
set uebp_testfail=0


call runuattest.bat BuildCookRun -build -run -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -cook -pak -stage -run -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -cook -run -allmaps -project=Samples\Showcases\Effects\Effects.uproject -stage -pak -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -cookonthefly -stage -run -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -cook -fileserver -stage -run -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -project=ShooterGame -build -cook -server -pak -stage -run -platform=win32 -serverplatform=win64 -map=/Game/Maps/Sanctuary -unattended -nullrhi -NoP4 -addcmdline="-nosteam"
call runuattest.bat BuildCookRun -project=OrionGame -build -cook -server -pak -stage -run -platform=win32 -serverplatform=win64 -map=/Game/PROTO/Gametypes/TDM/Maps/TDM_Arena -unattended -nullrhi -NoP4


if "%uebp_testfail%" == "1" goto fail

ECHO ****************** All tests Succeeded
exit /B 0

:fail
ECHO ****************** A test failed
exit /B 1



