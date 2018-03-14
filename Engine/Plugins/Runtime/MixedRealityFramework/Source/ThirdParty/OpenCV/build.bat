@ECHO OFF

set opencv_url=https://github.com/opencv/opencv/archive/3.3.1.zip
set opencv_src=opencv-3.3.1

for %%x in ("..\..\Binaries\ThirdParty") do set bin_path=%%~fx
for %%x in ("lib") do set lib_path=%%~fx

if not exist build md build

pushd build

IF NOT EXIST %opencv_src% (
    IF NOT EXIST opencv.zip (
        echo Downloading %opencv_url%...
        powershell -Command "(New-Object Net.WebClient).DownloadFile('%opencv_url%', '%opencv_src%.zip')"
    )
    echo Extracting %opencv_src%.zip...
    powershell -Command "Add-Type -AssemblyName System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory('%opencv_src%.zip', '.')"
)


echo Deleting existing build directories...
if exist x86 rd /s /q x86
if exist x64 rd /s /q x64

md x86
pushd x86

echo Configuring x86 build...
cmake -G "Visual Studio 14 2015" -C %~dp0\cmake_options.txt -DCMAKE_INSTALL_PREFIX=%~dp0 ..\%opencv_src%

echo Building x86 Release build...
cmake.exe --build . --config Release --target INSTALL -- /m:4
echo Building x86 Debug build...
cmake.exe --build . --config Debug --target INSTALL -- /m:4

popd

md x64
pushd x64

echo Configuring x64 build...
cmake -G "Visual Studio 14 2015 Win64" -C %~dp0\cmake_options.txt -DCMAKE_INSTALL_PREFIX=%~dp0 ..\%opencv_src%

echo Building x64 Release build...
cmake.exe --build . --config Release --target INSTALL -- /m:4
echo Building x64 Debug build...
cmake.exe --build . --config Debug --target INSTALL -- /m:4

popd

popd

echo Cleaning up
:: Clean up destination directories
md %bin_path%\Win32
md %bin_path%\Win64

md %lib_path%\Win32
md %lib_path%\Win64

move /y x86\vc14\bin\*.* %bin_path%\Win32
move /y x64\vc14\bin\*.* %bin_path%\Win64
move /y x86\vc14\lib\*.lib %lib_path%\Win32
move /y x64\vc14\lib\*.lib %lib_path%\Win64

rd /s /q x86
rd /s /q x64
del /f OpenCV*.cmake

echo Done. Remember to delete the build directory and submit changed files to p4
