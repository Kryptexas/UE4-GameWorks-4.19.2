//
// Copyright (C) Google Inc. 2017. All rights reserved.
//

#include "ResonanceAudioCommon.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "FileManager.h"
#include "IPluginManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogResonanceAudio);

namespace ResonanceAudio
{

	void* LoadResonanceAudioDynamicLibrary()
	{
		FString LibraryPath = FPaths::EngineDir() / TEXT("Source/ThirdParty/ResonanceAudioApi/lib");
		FString DynamicLibraryToLoad;
		void* DynamicLibraryHandle = nullptr;

#if PLATFORM_WINDOWS
	#if PLATFORM_64BITS
		DynamicLibraryToLoad = LibraryPath / TEXT("win_x64/vraudio.dll");
	#else
		DynamicLibraryToLoad = LibraryPath / TEXT("win_x86/vraudio.dll");
	#endif	// PLATFORM_64BITS
#elif PLATFORM_MAC
		DynamicLibraryToLoad = LibraryPath / TEXT("darwin/libvraudio.dylib");
#elif PLATFORM_ANDROID || PLATFORM_IOS
		 // Not necessary on this platform.
		return nullptr;
#elif PLATFORM_LINUX
		DynamicLibraryToLoad = LibraryPath / TEXT("linux/libvraudio.so");
#else
		UE_LOG(LogResonanceAudio, Error, TEXT("Unsupported Platform. Supported platforms are ANDROID, IOS, LINUX, MAC and WINDOWS"));
		return nullptr;
#endif  // PLATFORM_WINDOWS

		UE_LOG(LogResonanceAudio, Log, TEXT("Attempting to load %s"), *DynamicLibraryToLoad);

		if (FPaths::FileExists(*DynamicLibraryToLoad))
		{
			DynamicLibraryHandle = FPlatformProcess::GetDllHandle(*DynamicLibraryToLoad);
		}
		else
		{
			UE_LOG(LogResonanceAudio, Log, TEXT("File does not exist. %s"), *DynamicLibraryToLoad);
		}

		if (!DynamicLibraryHandle)
		{
			UE_LOG(LogResonanceAudio, Log, TEXT("Unable to load %s."), *FPaths::ConvertRelativePathToFull(DynamicLibraryToLoad));
		}
		else
		{
			UE_LOG(LogResonanceAudio, Log, TEXT("Loaded %s."), *DynamicLibraryToLoad);
		}

		return DynamicLibraryHandle;
	}

	vraudio::VrAudioApi* CreateResonanceAudioApi(void* DynamicLibraryHandle, size_t NumChannels, size_t NumFrames, int SampleRate) {
		vraudio::VrAudioApi* (*create)(size_t, size_t, int);
#if PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_WINDOWS
		if (DynamicLibraryHandle)
		{
			create = reinterpret_cast<vraudio::VrAudioApi* (*)(size_t, size_t, int)>(FPlatformProcess::GetDllExport(DynamicLibraryHandle, TEXT("CreateVrAudioApi")));
		}
		else
		{
			create = nullptr;
		}
#else
		 // For the static case, or for Android.
		return vraudio::CreateVrAudioApi(NumChannels, NumFrames, SampleRate);
#endif

		if (create == nullptr) {
			UE_LOG(LogResonanceAudio, Log, TEXT("Failed to load the Create method from VrAudioApi."));
			return nullptr;
		}
		return create(NumChannels, NumFrames, SampleRate);
	}

	RaMaterialName ConvertToResonanceAudioMaterialName(ERaMaterialName UnrealMaterialName)
	{
		switch (UnrealMaterialName)
		{
		case ERaMaterialName::TRANSPARENT:
			return RaMaterialName::kTransparent;
		case ERaMaterialName::ACOUSTIC_CEILING_TILES:
			return RaMaterialName::kAcousticCeilingTiles;
		case ERaMaterialName::BRICK_BARE:
			return RaMaterialName::kBrickBare;
		case ERaMaterialName::BRICK_PAINTED:
			return RaMaterialName::kBrickPainted;
		case ERaMaterialName::CONCRETE_BLOCK_COARSE:
			return RaMaterialName::kConcreteBlockCoarse;
		case ERaMaterialName::CONCRETE_BLOCK_PAINTED:
			return RaMaterialName::kConcreteBlockPainted;
		case ERaMaterialName::CURTAIN_HEAVY:
			return RaMaterialName::kCurtainHeavy;
		case ERaMaterialName::FIBER_GLASS_INSULATION:
			return RaMaterialName::kFiberGlassInsulation;
		case ERaMaterialName::GLASS_THICK:
			return RaMaterialName::kGlassThick;
		case ERaMaterialName::GLASS_THIN:
			return RaMaterialName::kGlassThin;
		case ERaMaterialName::GRASS:
			return RaMaterialName::kGrass;
		case ERaMaterialName::LINOLEUM_ON_CONCRETE:
			return RaMaterialName::kLinoleumOnConcrete;
		case ERaMaterialName::MARBLE:
			return RaMaterialName::kMarble;
		case ERaMaterialName::METAL:
			return RaMaterialName::kMetal;
		case ERaMaterialName::PARQUET_ONCONCRETE:
			return RaMaterialName::kParquetOnConcrete;
		case ERaMaterialName::PLASTER_ROUGH:
			return RaMaterialName::kPlasterRough;
		case ERaMaterialName::PLASTER_SMOOTH:
			return RaMaterialName::kPlasterSmooth;
		case ERaMaterialName::PLYWOOD_PANEL:
			return RaMaterialName::kPlywoodPanel;
		case ERaMaterialName::POLISHED_CONCRETE_OR_TILE:
			return RaMaterialName::kPolishedConcreteOrTile;
		case ERaMaterialName::SHEETROCK:
			return RaMaterialName::kSheetrock;
		case ERaMaterialName::WATER_OR_ICE_SURFACE:
			return RaMaterialName::kWaterOrIceSurface;
		case ERaMaterialName::WOOD_CEILING:
			return RaMaterialName::kWoodCeiling;
		case ERaMaterialName::WOOD_PANEL:
			return RaMaterialName::kWoodPanel;
		case ERaMaterialName::UNIFORM:
			return RaMaterialName::kUniform;
		default:
			UE_LOG(LogResonanceAudio, Error, TEXT("Acoustic Material does not exist. Returning Transparent Material."))
			return RaMaterialName::kTransparent;
		}
	}

/*       RESONANCE AUDIO                UNREAL
           Y                             Z
           |					         |    X
		   |						     |   /
		   |						     |  /
		   |						     | /
		   |_______________X			 |/_______________Y
		  /
		 /
		/
	   Z
*/
	FVector ConvertToResonanceAudioCoordinates(const FVector& UnrealVector)
	{
		FVector ResonanceAudioVector;
		ResonanceAudioVector.X = UnrealVector.Y;
		ResonanceAudioVector.Y = UnrealVector.Z;
		ResonanceAudioVector.Z = -UnrealVector.X;
		return ResonanceAudioVector * SCALE_FACTOR;
	}


	FQuat ConvertToResonanceAudioRotation(const FQuat& UnrealQuat)
	{
		FQuat ResonanceAudioQuat;
		ResonanceAudioQuat.X = -UnrealQuat.Y;
		ResonanceAudioQuat.Y = -UnrealQuat.Z;
		ResonanceAudioQuat.Z = UnrealQuat.X;
		ResonanceAudioQuat.W = UnrealQuat.W;
		return ResonanceAudioQuat;
	}

}  // namespace ResonanceAudio
