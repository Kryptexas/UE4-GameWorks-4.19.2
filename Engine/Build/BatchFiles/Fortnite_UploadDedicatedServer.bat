@ECHO OFF

SET BUILD_VERSION=%1
SET BUILD_CLN=%BUILD_VERSION:~-7%
SET BUILD_EPIC_FOLDER=\\epicgames.net\Root\Builds\Fortnite\%BUILD_VERSION%\LinuxServer\
SET HOST_RSYNC_DIRECTORY=//epicgames.net/Root/Builds/Fortnite/%BUILD_VERSION%/LinuxServer
SET DEST=fn-mal-us1f-prod11-1
SET DEST_IP=130.211.185.94
SET DEST_EU=fn-mal-eu1b-prod11-1
SET DEST_IP_EU=130.211.52.202

ECHO.
ECHO ***************************************************************************
ECHO * Rsyncing the build...
ECHO * FROM: %BUILD_EPIC_FOLDER%
ECHO * TO: %DEST%:/opt/fortnite/primer (%DEST_IP%)
ECHO ***************************************************************************
ECHO.

SET /a RSYNC_ERRORLEVEL=1
SET /a RSYNC_INCREMENT=0

:rsync_start
    SET /a RSYNC_INCREMENT=RSYNC_INCREMENT+1
    ECHO ***************************************************************************
    ECHO RSYNC: Attempt %RSYNC_INCREMENT% starting...
    P:\Builds\Utilities\rsync\rsync.exe -acrv --chmod=ug=rwX,o=rxX --no-implied-dirs %HOST_RSYNC_DIRECTORY%/* _fnuser@%DEST_IP%:/opt/fortnite/primer -e "P:\Builds\Utilities\rsync\ssh -l _fnuser -i P:\Builds\Utilities\Google_Compute.pem -o StrictHostKeyChecking=no"
    SET /a RSYNC_ERRORLEVEL = %ERRORLEVEL%
    if %RSYNC_ERRORLEVEL% EQU 0 (goto :rsync_success)
    if %RSYNC_INCREMENT% EQU 10 (goto :rsync_fail)
    goto :rsync_start

:rsync_fail
    ECHO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ECHO !!! The rsync failed after 10 tries !!!
    ECHO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    exit /b 1

:rsync_success
ECHO.
ECHO ***************************************************************************
ECHO * Rsync finished successfully. The build is up on %DEST%.
ECHO ***************************************************************************
ECHO.
ECHO ***************************************************************************
ECHO * Copying build from /opt/fortnite/primer
ECHO *                 to /opt/fortnite/builds/%BUILD_CLN%
ECHO ***************************************************************************
ECHO.

P:\Builds\Utilities\rsync\ssh.exe -o StrictHostKeyChecking=no -i P:\Builds\Utilities\Google_Compute.pem _fnuser@%DEST_IP% "mkdir --parents /opt/fortnite/builds/%BUILD_CLN%"
P:\Builds\Utilities\rsync\ssh.exe -o StrictHostKeyChecking=no -i P:\Builds\Utilities\Google_Compute.pem _fnuser@%DEST_IP% "cp -r /opt/fortnite/primer/* /opt/fortnite/builds/%BUILD_CLN%"
P:\Builds\Utilities\rsync\ssh.exe -o StrictHostKeyChecking=no -i P:\Builds\Utilities\Google_Compute.pem _fnuser@%DEST_IP% "chmod +x /opt/fortnite/builds/%BUILD_CLN%/FortniteGame/Binaries/Linux/* /opt/fortnite/builds/%BUILD_CLN%/Engine/Binaries/Linux/CrashReportClient"

ECHO ***************************************************************************
ECHO * Copy finished. The build should now be available in
ECHO * /opt/fortnite/builds/%BUILD_CLN%
ECHO ***************************************************************************

ECHO.
ECHO ***************************************************************************
ECHO * Rsyncing the build...
ECHO * From: %DEST% (%DEST_IP%)
ECHO * To:   %DEST_EU% (%DEST_IP_EU%)
ECHO *         /opt/fortnite/builds/%BUILD_CLN%
ECHO ***************************************************************************
ECHO.

REM P:\Builds\Utilities\rsync\ssh.exe -o StrictHostKeyChecking=no -i P:\Builds\Utilities\Google_Compute.pem _fnuser@%DEST_IP% "rsync -acrv --chmod=ug=rwX,o=rxX --no-implied-dirs /opt/fortnite/builds/%BUILD_CLN% %DEST_EU%:/opt/fortnite/builds/"

SET /a RSYNC_ERRORLEVEL=1
SET /a RSYNC_INCREMENT=0

:rsync_eu_start
    SET /a RSYNC_INCREMENT=RSYNC_INCREMENT+1
    ECHO ***************************************************************************
    ECHO RSYNC: Attempt %RSYNC_INCREMENT% starting...
    P:\Builds\Utilities\rsync\ssh.exe -o StrictHostKeyChecking=no -i P:\Builds\Utilities\Google_Compute.pem _fnuser@%DEST_IP% "rsync -acrv --chmod=ug=rwX,o=rxX --no-implied-dirs /opt/fortnite/builds/%BUILD_CLN% %DEST_EU%:/opt/fortnite/builds/"
    SET /a RSYNC_ERRORLEVEL = %ERRORLEVEL%
    if %RSYNC_ERRORLEVEL% EQU 0 (goto :rsync_eu_success)
    if %RSYNC_INCREMENT% EQU 10 (goto :rsync_eu_fail)
    goto :rsync_eu_start

:rsync_eu_fail
    ECHO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ECHO !!! The rsync failed to copy to EU's Mal after 10 tries !!!
    ECHO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    exit /b 1

:rsync_eu_success
ECHO.
ECHO ***************************************************************************
ECHO * Rsync finished successfully. The build is up on:
ECHO *    US Mal: %DEST% (%DEST_IP%)
ECHO *    EU Mal: %DEST_EU% (%DEST_IP_EU%)
ECHO ***************************************************************************
ECHO.
