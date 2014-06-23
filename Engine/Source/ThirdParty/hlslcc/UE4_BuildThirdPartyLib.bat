@echo off
REM hlslcc
pushd hlslcc\projects

	p4 edit %THIRD_PARTY_CHANGELIST% ..\lib\...

	REM vs2013
	pushd vs2013
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=x64;Configuration="Release"
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=x64;Configuration="Debug"
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=Win32;Configuration="Debug"
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=Win32;Configuration="Release"
	popd

	REM vs2012
	pushd vs2012
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=Win32;Configuration="Debug"
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=Win32;Configuration="Release"
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=x64;Configuration="Debug"
	msbuild hlslcc.sln /target:Clean,hlslcc_lib /p:Platform=x64;Configuration="Release"
	popd
popd
