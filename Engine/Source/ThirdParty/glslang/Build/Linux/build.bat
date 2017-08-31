@echo off



rem ## Change relative path into absolute path. We need that for cmake.

set REL_PATH=..\..\..\..\..\Extras\ThirdPartyNotUE\GNU_Make\make-3.81\bin

set MAKEFILE_PATH=

pushd %REL_PATH%

set MAKEFILE_PATH=%CD%

popd



setlocal



rem ## Give a nice message.

echo Cross Compiling glslang ...



rem ## Set the architecture.

set ARCHITECTURE_TRIPLE="x86_64-unknown-linux-gnu"

rem set ARCHITECTURE_TRIPLE="i686-unknown-linux-gnu"

rem set ARCHITECTURE_TRIPLE="arm-unknown-linux-gnueabihf"

rem set ARCHITECTURE_TRIPLE="aarch64-unknown-linux-gnueabi"



rem ## Compile and Build.

cmake -DCMAKE_MAKE_PROGRAM=%MAKEFILE_PATH%\make.exe -DCMAKE_TOOLCHAIN_FILE="LinuxCrossToolchain.multiarch.cmake" -DARCHITECTURE_TRIPLE=%ARCHITECTURE_TRIPLE% -G"Unix Makefiles" ../../../glslang/glslang/src/glslang_lib

%MAKEFILE_PATH%\make.exe



rem ## Copy to destination.

set DESTINATION_DIRECTORY="../../../glslang/glslang/lib/Linux"



if not exist %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE% mkdir %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE%



copy /y glslang\libglslang.a %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE%\libglslang.a

copy /y glslang\OSDependent\Unix\libOSDependent.a %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE%\libOSDependent.a

copy /y hlsl\libHLSL.a %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE%\libHLSL.a

copy /y OGLCompilersDLL\libOGLCompiler.a %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE%\libOGLCompiler.a

copy /y SPIRV\libSPIRV.a %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE%\libSPIRV.a

copy /y SPIRV\libSPVRemapper.a %DESTINATION_DIRECTORY%\%ARCHITECTURE_TRIPLE%\libSPVRemapper.a