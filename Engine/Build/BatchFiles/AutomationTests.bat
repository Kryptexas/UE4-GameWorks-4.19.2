
set uebp_testfail=0

rem editor tests ***************************************************************

call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=QAGame
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=FortniteGame
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=OrionGame
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=Soul
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=ShooterGame
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=Shadow
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=StrategyGame
call runuattest.bat BuildCookRun -build -run -editortest -RunAutomationTests -unattended -nullrhi -NoP4 -project=VehicleGame

rem game tests ***************************************************************

call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=QAGame
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=FortniteGame
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=OrionGame
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=Soul
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=ShooterGame
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=Shadow
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=StrategyGame
call runuattest.bat BuildCookRun -build -run -RunAutomationTests -unattended -nullrhi -NoP4 -project=VehicleGame

rem cooked game tests ***************************************************************

call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4
call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=QAGame
rem call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=FortniteGame
call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=OrionGame
call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=Soul
call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=ShooterGame
call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=Shadow
call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=StrategyGame
call runuattest.bat BuildCookRun -build -run -cook -stage -pak -RunAutomationTests -unattended -nullrhi -NoP4 -project=VehicleGame



if "%uebp_testfail%" == "1" goto fail

ECHO ****************** All tests Succeeded
exit /B 0

:fail
ECHO ****************** A test failed
exit /B 1



