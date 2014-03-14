// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxTargetPlatform.cpp: Implements the FLinuxTargetPlatform class.
=============================================================================*/


#include "LinuxTargetPlatformPrivatePCH.h"


/* FAndroidTargetPlatform structors
 *****************************************************************************/

FLinuxTargetPlatform::FLinuxTargetPlatform( )
{
	LocalDevice = MakeShareable(new FLinuxTargetDevice(*this, FTargetDeviceId(PlatformName(), FPlatformProcess::ComputerName())));
	
	#if WITH_ENGINE
		FConfigCacheIni::LoadLocalIniFile(EngineSettings, TEXT("Engine"), true, *PlatformName());
		TextureLODSettings.Initialize(EngineSettings, TEXT("SystemSettings"));
		StaticMeshLODSettings.Initialize(EngineSettings);
	#endif
}


/* ITargetPlatform overrides
 *****************************************************************************/

void FLinuxTargetPlatform::GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const
{
	// TODO: ping all the machines in a local segment and/or try to connect to port 22 of those that respond
	OutDevices.Reset();
	OutDevices.Add(LocalDevice);
}


ECompressionFlags FLinuxTargetPlatform::GetBaseCompressionMethod( ) const
{
	return COMPRESS_ZLIB;
}


bool FLinuxTargetPlatform::GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFiles, TArray<FString>& OutMissingFiles ) const
{
	if ((BuildTarget == EBuildTargets::Unknown) || (BuildConfiguration == EBuildConfigurations::Unknown))
	{
		return false;
	}

	const FString ProjectName = FPaths::GetBaseFilename(ProjectPath);
	const FString ProjectRoot = FPaths::GetPath(ProjectPath);

	if (!ProjectName.IsEmpty() || !ProjectRoot.IsEmpty())
	{
		return false;
	}

	// append binaries
	if (Artifacts & ETargetPlatformBuildArtifacts::Engine)
	{
		const FString EngineBinariesDir = FPaths::EngineDir() / TEXT("Binaries") / TEXT("Linux");
		const FString DeployedBinariesDir = FString(TEXT("Engine")) / TEXT("Binaries") / TEXT("Linux");

		// executable
		{
			FString ExecutableBaseName;
			FString ProjectBinariesDir = ProjectRoot / TEXT("Binaries");

			// content-only projects use game agnostic binaries
			if (IFileManager::Get().DirectoryExists(*ProjectBinariesDir))
			{
				ExecutableBaseName = ProjectName;
			}
			else
			{
				ExecutableBaseName = TEXT("UE4");
				ProjectBinariesDir = EngineBinariesDir;
			}
	
			if (BuildConfiguration != EBuildConfigurations::Development)
			{
				ExecutableBaseName += FString::Printf(TEXT("-%s-%s"), *PlatformName(), EBuildConfigurations::ToString(BuildConfiguration));
			}

			ProjectBinariesDir /= TEXT("Linux");

			// add executable
			{
				FString ExecutablePath = ProjectBinariesDir / ExecutableBaseName;

				if (FPaths::FileExists(*ExecutablePath))
				{
					OutFiles.Add(ExecutablePath, DeployedBinariesDir / ExecutableBaseName);
				}
				else
				{
					OutMissingFiles.Add(ExecutablePath);
				}
			}
		}

		// dynamic link libraries
		{
			// @todo platforms: this needs some major improvement, so that only required files are included
			TArray<FString> FileNames;

			IFileManager::Get().FindFiles(FileNames, *(EngineBinariesDir / TEXT("*.so")), true, false);

			for (int32 FileIndex = 0; FileIndex < FileNames.Num(); ++FileIndex)
			{
				// add library
				const FString& FileName = FileNames[FileIndex];
				OutFiles.Add(EngineBinariesDir / FileName, DeployedBinariesDir / FileName);
			}
		}

		// append third party binaries
		{
			FString ThirdPartyBinariesDir = FPaths::EngineDir() / TEXT("Binaries") / TEXT("ThirdParty");
			TArray<FString> FileNames;

			IFileManager::Get().FindFilesRecursive(FileNames, *ThirdPartyBinariesDir, TEXT("*.*"), true, false);

			for (int32 FileIndex = 0; FileIndex < FileNames.Num(); ++FileIndex)
			{
				FString FileName = FileNames[FileIndex];

				if (FileName.EndsWith(TEXT(".so")) && !FileName.Contains(TEXT("Mcpp")) && !FileName.Contains(TEXT("NoRedist")) )
				{
					OutFiles.Add(FileName, FileName.RightChop(9));
				}
			}
		}
	}

	// append engine content
	if (Artifacts & ETargetPlatformBuildArtifacts::Content)
	{
		// @todo platforms: handle Engine configuration files and content
	}

	// append game content
	if (Artifacts & ETargetPlatformBuildArtifacts::Content)
	{
		// @todo platforms: handle game configuration files and content
	}

	// append tools
	if (Artifacts & ETargetPlatformBuildArtifacts::Tools)
	{
		// @todo platforms: add tools deployment support
		return false;
	}

	return (OutMissingFiles.Num() == 0);
}



ITargetDevicePtr FLinuxTargetPlatform::GetDefaultDevice( ) const
{
	return LocalDevice;
}


ITargetDevicePtr FLinuxTargetPlatform::GetDevice( const FTargetDeviceId& DeviceId )
{
	if (LocalDevice.IsValid() && (DeviceId == LocalDevice->GetId()))
	{
		return LocalDevice;
	}

	FTargetDeviceId UATFriendlyId(TEXT("linux"), DeviceId.GetDeviceName());
	return MakeShareable(new FLinuxTargetDevice(*this, UATFriendlyId));
}


FString FLinuxTargetPlatform::GetIconPath( ETargetPlatformIcons::IconType IconType ) const
{
	switch (IconType)
	{
	case ETargetPlatformIcons::Normal:
		return FString(TEXT("Launcher/Linux/Platform_Linux_24x"));

	case ETargetPlatformIcons::Large:
	case ETargetPlatformIcons::XLarge:
		return FString(TEXT("Launcher/Linux/Platform_Linux_128x"));
	}

	return FString();
}


bool FLinuxTargetPlatform::IsRunningPlatform( ) const
{
	return false;
}


#if WITH_ENGINE

void FLinuxTargetPlatform::GetShaderFormats( TArray<FName>& OutFormats ) const
{
	// no shaders needed for dedicated server target
}

const FStaticMeshLODSettings& FLinuxTargetPlatform::GetStaticMeshLODSettings( ) const
{
	return StaticMeshLODSettings;
}


void FLinuxTargetPlatform::GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const
{
	// no textures needed for dedicated server target
	if (IsServerOnly())
	{
		return;
	}

	OutFormats.Add(NAME_None);
}


const struct FTextureLODSettings& FLinuxTargetPlatform::GetTextureLODSettings( ) const
{
	return TextureLODSettings;
}


FName FLinuxTargetPlatform::GetWaveFormat( class USoundWave* Wave ) const
{
	static FName NAME_OGG(TEXT("OGG"));

	return NAME_OGG;
}

#endif //WITH_ENGINE