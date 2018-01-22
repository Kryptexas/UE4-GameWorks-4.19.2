SET ILMBASE_ROOT=%cd%/AlembicDeploy/VS2015/x64/
SET HDF5_ROOT=%cd%/AlembicDeploy/VS2015/x64/
mkdir build
cd build
cd build
mkdir VS2015
cd VS2015
rmdir /s /q alembic
mkdir alembic
cd alembic
cmake -G "Visual Studio 14 2015 Win64" -DZLIB_INCLUDE_DIR=../../../../../../Source/ThirdParty/zlib/v1.2.8/include/Win64/VS2015/ -DZLIB_LIBRARY=../../../../../../Source/ThirdParty/zlib/v1.2.8/lib/Win64/VS2015/ -DALEMBIC_SHARED_LIBS=OFF -DUSE_TESTS=OFF -DUSE_BINARIES=OFF  -DUSE_HDF5=ON -DALEMBIC_ILMBASE_LINK_STATIC=ON -DUSE_STATIC_HDF5=OFF  -DCMAKE_INSTALL_PREFIX=%cd%/../../../AlembicDeploy/VS2015/x64/ ../../../alembic/ 
cd ../../../


