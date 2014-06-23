cscript /nologo FindToolchainPaths.vbs > Makefile.ToRun
type Makefile.Epic >> Makefile.ToRun

cd ..\..\src
rem nmake -f ..\Builds\Linux\Makefile.ToRun
nmake -f..\Builds\Linux\Makefile.ToRun clean
nmake -f ..\Builds\Linux\Makefile.ToRun install
