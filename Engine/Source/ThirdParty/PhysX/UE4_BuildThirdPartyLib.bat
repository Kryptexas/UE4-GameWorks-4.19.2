REM PhysX
setlocal

REM we need to move all the way out to the PhysX branch - we assume it's next to UE4 as it is in p4
pushd ..\..\..\..\..\PhysX\Epic\PhysX_32

	pushd PhysX_3.3
	
		p4 edit %THIRD_PARTY_CHANGELIST% Lib\...
		p4 edit %THIRD_PARTY_CHANGELIST% Bin\...

		REM generate PhysX project files
		pushd Source\compiler\xpj
			call create_projects.cmd
		popd

		REM now build PhysX libs

		msbuild Source\compiler\vc11win32\PhysX.sln /target:Clean,Build,PhysX /p:Platform=Win32;Configuration=debug
		msbuild Source\compiler\vc11win32\PhysX.sln /target:Clean,Build,PhysX /p:Platform=Win32;Configuration=release
		msbuild Source\compiler\vc11win32\PhysX.sln /target:Clean,Build,PhysX /p:Platform=Win32;Configuration=profile

		msbuild Source\compiler\vc11win64\PhysX.sln /target:Clean,Build,PhysX /p:Platform=x86;Configuration=debug
		msbuild Source\compiler\vc11win64\PhysX.sln /target:Clean,Build,PhysX /p:Platform=x86;Configuration=release
		msbuild Source\compiler\vc11win64\PhysX.sln /target:Clean,Build,PhysX /p:Platform=x86;Configuration=profile

		msbuild Source\compiler\vc11xboxone\PhysX.sln /target:Clean,Build /p:Platform=Durango;Configuration=debug
		msbuild Source\compiler\vc11xboxone\PhysX.sln /target:Clean,Build /p:Platform=Durango;Configuration=release
		msbuild Source\compiler\vc11xboxone\PhysX.sln /target:Clean,Build /p:Platform=Durango;Configuration=profile

		msbuild Source\compiler\vc10ps4\PhysX.sln /target:Clean,Build /p:Platform=ORBIS;Configuration=debug
		msbuild Source\compiler\vc10ps4\PhysX.sln /target:Clean,Build /p:Platform=ORBIS;Configuration=release
		msbuild Source\compiler\vc10ps4\PhysX.sln /target:Clean,Build /p:Platform=ORBIS;Configuration=profile

		msbuild Source\compiler\vc10android9\PhysX.sln /target:Clean,Build /p:Platform=Android;Configuration=debug
		msbuild Source\compiler\vc10android9\PhysX.sln /target:Clean,Build /p:Platform=Android;Configuration=release
		msbuild Source\compiler\vc10android9\PhysX.sln /target:Clean,Build /p:Platform=Android;Configuration=profile
			
	 	msbuild Source\compiler\html5win32\PhysX.sln /target:Clean,Build /p:Platform=EMSCRIPTEN;Configuration=debug;PlatformToolset=emcc
	 	msbuild Source\compiler\html5win32\PhysX.sln /target:Clean,Build /p:Platform=EMSCRIPTEN;Configuration=release;PlatformToolset=emcc
	 	msbuild Source\compiler\html5win32\PhysX.sln /target:Clean,Build /p:Platform=EMSCRIPTEN;Configuration=profile;PlatformToolset=emcc

	popd


	pushd APEX_1.3_vs_PhysX_3.3
		
		p4 edit %THIRD_PARTY_CHANGELIST% lib\...
		p4 edit %THIRD_PARTY_CHANGELIST% bin\...

		REM generate Apex project files
		pushd compiler\xpj
			call create_projects.cmd
		popd

		REM now build Apex libs

		msbuild compiler\vc11win32-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=Win32;Configuration=debug
		msbuild compiler\vc11win32-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=Win32;Configuration=release
		msbuild compiler\vc11win32-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=Win32;Configuration=profile

		msbuild compiler\vc11win64-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=x86;Configuration=debug
		msbuild compiler\vc11win64-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=x86;Configuration=release
		msbuild compiler\vc11win64-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=x86;Configuration=profile

		msbuild compiler\vc11xboxone-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=Durango;Configuration=debug
		msbuild compiler\vc11xboxone-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=Durango;Configuration=release
		msbuild compiler\vc11xboxone-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=Durango;Configuration=profile

		msbuild compiler\vc10ps4-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=ORBIS;Configuration=debug
		msbuild compiler\vc10ps4-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=ORBIS;Configuration=release
		msbuild compiler\vc10ps4-PhysX_3.3\APEX.sln /target:Clean,Build /p:Platform=ORBIS;Configuration=profile
		
	popd

popd

