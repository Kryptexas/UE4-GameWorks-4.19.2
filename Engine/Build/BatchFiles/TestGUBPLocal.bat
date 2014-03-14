set uebp_testfail=0


call runuattest.bat gubp -ListOnly -cleanlocal -Allplatforms -Fake -FakeEC %*

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=Rocket_WaitToMakeBuild

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=Shared_WaitForPromotable

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=Shared_WaitForTesting

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=Shared_WaitForPromotion

pause

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=FortniteGame_WaitForPromotable

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=FortniteGame_WaitForPromotion

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=OrionGame_WaitForPromotable

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=OrionGame_WaitToMakeBuild

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=OrionGame_WaitForPromotion

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=Soul_WaitForPromotable

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=Soul_WaitForPromotion


call runuattest.bat gubp -ListOnly -cleanlocal -Allplatforms -Fake -FakeEC %*

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -Node=OrionGame_MakeBuild

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=OrionGame_WaitForPromotable -Node=OrionGame_MakeBuild

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=OrionGame_WaitToMakeBuild -Node=OrionGame_MakeBuild



call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -SkipTriggers -Node=OrionGame_MakeBuild -CleanLocal


call runuattest.bat gubp -ListOnly -cleanlocal -Allplatforms -Fake -FakeEC %*

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -Node=FortniteGame_MakeBuild

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=FortniteGame_WaitForPromotable -Node=FortniteGame_MakeBuild

call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -TriggerNode=FortniteGame_WaitToMakeBuild -Node=FortniteGame_MakeBuild



call runuattest.bat gubp -Allplatforms -Fake -FakeEC %* -CommanderJobSetupOnly -SkipTriggers -Node=FortniteGame_MakeBuild -CleanLocal



if "%uebp_testfail%" == "1" goto fail

ECHO ****************** All tests Succeeded
exit /B 0

:fail
ECHO ****************** A test failed
exit /B 1



	