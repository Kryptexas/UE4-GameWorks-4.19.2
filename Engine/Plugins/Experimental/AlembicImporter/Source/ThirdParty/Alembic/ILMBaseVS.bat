SET ILMBASE_ROOT=%cd%/AlembicDeploy/
mkdir build
cd build
mkdir VS2015
cd VS2015
rmdir /s /q ilmbase
mkdir ilmbase
cd ilmbase

cmake -G "Visual Studio 14 2015 Win64" -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=%cd%/../../../AlembicDeploy/VS2015/x64/ ../../../openexr/ilmbase/
cd ../../../
