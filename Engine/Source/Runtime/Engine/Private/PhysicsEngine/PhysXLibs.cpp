// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PhysXLibs.cpp: PhysX library imports
=============================================================================*/
#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "EngineDefines.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "EngineLogs.h"

#if WITH_PHYSX 

// PhysX library imports
    


#if PLATFORM_WINDOWS || PLATFORM_MAC
	void* PxFoundationHandle = nullptr;
	void* PhysX3CommonHandle = nullptr;
	void* PhysX3Handle = nullptr;
	void* PxPvdSDKHandle = nullptr;
	void* PhysX3CookingHandle = nullptr;
	void* nvToolsExtHandle = nullptr;
	#if WITH_APEX
		void* APEXFrameworkHandle = nullptr;
		void* APEX_DestructibleHandle = nullptr;
		void* APEX_LegacyHandle = nullptr;
		#if WITH_APEX_CLOTHING
			void* APEX_ClothingHandle = nullptr;
		#endif  //WITH_APEX_CLOTHING
	#endif	//WITH_APEX
#endif

/**
 *	Load the required modules for PhysX
 */
ENGINE_API void LoadPhysXModules(bool bLoadCookingModule)
{
#if PLATFORM_WINDOWS
	FString PhysXBinariesRoot = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/PhysX/");
	FString APEXBinariesRoot = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/PhysX/");
	FString SharedBinariesRoot = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/PhysX/");

	#if _MSC_VER >= 1900
		FString VSDirectory(TEXT("VS2015/"));
	#else
		#error "Unrecognized Visual Studio version."
	#endif

	#if PLATFORM_64BITS
		FString RootPhysXPath(PhysXBinariesRoot + TEXT("Win64/") + VSDirectory);
		FString RootAPEXPath(APEXBinariesRoot + TEXT("Win64/") + VSDirectory);
		FString RootSharedPath(SharedBinariesRoot + TEXT("Win64/") + VSDirectory);
		FString ArchName(TEXT("_x64"));
		FString ArchBits(TEXT("64"));
	#else
		FString RootPhysXPath(PhysXBinariesRoot + TEXT("Win32/") + VSDirectory);
		FString RootAPEXPath(APEXBinariesRoot + TEXT("Win32/") + VSDirectory);
		FString RootSharedPath(SharedBinariesRoot + TEXT("Win32/") + VSDirectory);
		FString ArchName(TEXT("_x86"));
		FString ArchBits(TEXT("32"));
	#endif

#ifdef UE_PHYSX_SUFFIX
	FString PhysXSuffix(TEXT(PREPROCESSOR_TO_STRING(UE_PHYSX_SUFFIX)) + ArchName + TEXT(".dll"));
#else
	FString PhysXSuffix(ArchName + TEXT(".dll"));
#endif

#ifdef UE_APEX_SUFFIX
	FString APEXSuffix(TEXT(PREPROCESSOR_TO_STRING(UE_APEX_SUFFIX)) + ArchName + TEXT(".dll"));
#else
	FString APEXSuffix(ArchName + TEXT(".dll"));
#endif

	auto LoadPhysicsLibrary([](const FString& Path) -> void*
	{
		void* Handle = FPlatformProcess::GetDllHandle(*Path);
		if (Handle == nullptr)
		{
			UE_LOG(LogPhysics, Fatal, TEXT("Failed to load module '%s'."), *Path);
		}
		return Handle;
	});

	PxFoundationHandle = LoadPhysicsLibrary(RootSharedPath + "PxFoundation" + PhysXSuffix);
	PhysX3CommonHandle = LoadPhysicsLibrary(RootPhysXPath + "PhysX3Common" + PhysXSuffix);
	const FString nvToolsExtPath = RootPhysXPath + "nvToolsExt" + ArchBits + "_1.dll";
	if (IFileManager::Get().FileExists(*nvToolsExtPath))
	{
		nvToolsExtHandle = LoadPhysicsLibrary(nvToolsExtPath);
	}
	PxPvdSDKHandle = LoadPhysicsLibrary(RootSharedPath + "PxPvdSDK" + PhysXSuffix);
	PhysX3Handle = LoadPhysicsLibrary(RootPhysXPath + "PhysX3" + PhysXSuffix);

	if(bLoadCookingModule)
	{
		PhysX3CookingHandle = LoadPhysicsLibrary(RootPhysXPath + "PhysX3Cooking" + PhysXSuffix);
	}

	#if WITH_APEX
		APEXFrameworkHandle = LoadPhysicsLibrary(RootAPEXPath + "APEXFramework" + APEXSuffix);
		APEX_DestructibleHandle = LoadPhysicsLibrary(RootAPEXPath + "APEX_Destructible" + APEXSuffix);
		#if WITH_APEX_LEGACY
			APEX_LegacyHandle = LoadPhysicsLibrary(RootAPEXPath + "APEX_Legacy" + APEXSuffix);
		#endif //WITH_APEX_LEGACY
		#if WITH_APEX_CLOTHING
			APEX_ClothingHandle = LoadPhysicsLibrary(RootAPEXPath + "APEX_Clothing" + APEXSuffix);
		#endif //WITH_APEX_CLOTHING
	#endif	//WITH_APEX
#elif PLATFORM_MAC
	FString PhysXBinariesRoot = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/PhysX/Mac/");

	auto LoadPhysicsLibrary([](const FString& Path) -> void*
	{
		void* Handle = FPlatformProcess::GetDllHandle(*Path);
		if (Handle == nullptr)
		{
			UE_LOG(LogPhysics, Fatal, TEXT("Failed to load module '%s'."), *Path);
		}
		return Handle;
	});

#ifdef UE_PHYSX_SUFFIX
	FString PhysXSuffix = FString(TEXT(PREPROCESSOR_TO_STRING(UE_PHYSX_SUFFIX))) + TEXT(".dylib");
#else
	FString PhysXSuffix(TEXT(".dylib"));
#endif

#ifdef UE_APEX_SUFFIX
	FString APEXSuffix = FString(TEXT(PREPROCESSOR_TO_STRING(UE_APEX_SUFFIX))) + TEXT(".dylib");
#else
	FString APEXSuffix(TEXT(".dylib"));
#endif

	const FString PxFoundationLibName = FString::Printf(TEXT("%slibPxFoundation%s"), *PhysXBinariesRoot, *PhysXSuffix);
	PxFoundationHandle = LoadPhysicsLibrary(PxFoundationLibName);

	const FString PhysX3CommonLibName = FString::Printf(TEXT("%slibPhysX3Common%s"), *PhysXBinariesRoot, *PhysXSuffix);
	PhysX3CommonHandle = LoadPhysicsLibrary(PhysX3CommonLibName);

	const FString PxPvdSDKLibName = FString::Printf(TEXT("%slibPxPvdSDK%s"), *PhysXBinariesRoot, *PhysXSuffix);
	PxPvdSDKHandle = LoadPhysicsLibrary(PxPvdSDKLibName);

	const FString PhysX3LibName = FString::Printf(TEXT("%slibPhysX3%s"), *PhysXBinariesRoot, *PhysXSuffix);
	PhysX3Handle = LoadPhysicsLibrary(PhysX3LibName);

	if(bLoadCookingModule)
	{
		const FString PhysX3CookinLibName = FString::Printf(TEXT("%slibPhysX3Cooking%s"), *PhysXBinariesRoot, *PhysXSuffix);
		PhysX3CookingHandle = LoadPhysicsLibrary(PhysX3CookinLibName);
	}

	#if WITH_APEX
		const FString APEXFrameworkLibName = FString::Printf(TEXT("%slibAPEXFramework%s"), *PhysXBinariesRoot, *APEXSuffix);
		APEXFrameworkHandle = LoadPhysicsLibrary(APEXFrameworkLibName);
		const FString APEX_DestructibleHandleLibName = FString::Printf(TEXT("%slibAPEX_Destructible%s"), *PhysXBinariesRoot, *APEXSuffix);
		APEX_DestructibleHandle = LoadPhysicsLibrary(APEX_DestructibleHandleLibName);
		#if WITH_APEX_LEGACY
			const FString APEX_LegacyHandleLibName = FString::Printf(TEXT("%slibAPEX_Legacy%s"), *PhysXBinariesRoot, *APEXSuffix);
			APEX_LegacyHandle = LoadPhysicsLibrary(APEX_LegacyHandleLibName);
		#endif //WITH_APEX_LEGACY
		#if WITH_APEX_CLOTHING
			const FString APEX_ClothingHandleLibName = FString::Printf(TEXT("%slibAPEX_Clothing%s"), *PhysXBinariesRoot, *APEXSuffix);
			APEX_ClothingHandle = LoadPhysicsLibrary(APEX_ClothingHandleLibName);
		#endif //WITH_APEX_CLOTHING
	#endif	//WITH_APEX
#endif	//PLATFORM_WINDOWS
}

/** 
 *	Unload the required modules for PhysX
 */
void UnloadPhysXModules()
{
#if PLATFORM_WINDOWS || PLATFORM_MAC
	FPlatformProcess::FreeDllHandle(PxPvdSDKHandle);
	FPlatformProcess::FreeDllHandle(PhysX3Handle);
	if(PhysX3CookingHandle)
	{
		FPlatformProcess::FreeDllHandle(PhysX3CookingHandle);
	}
	FPlatformProcess::FreeDllHandle(PhysX3CommonHandle);
	FPlatformProcess::FreeDllHandle(PxFoundationHandle);
	#if WITH_APEX
		FPlatformProcess::FreeDllHandle(APEXFrameworkHandle);
		FPlatformProcess::FreeDllHandle(APEX_DestructibleHandle);
		FPlatformProcess::FreeDllHandle(APEX_LegacyHandle);
		#if WITH_APEX_CLOTHING
			FPlatformProcess::FreeDllHandle(APEX_ClothingHandle);
		#endif //WITH_APEX_CLOTHING
	#endif	//WITH_APEX
#endif
}

#endif // WITH_PHYSX

