// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxTargetPlatform.h: Declares the FLinuxTargetPlatform class.
=============================================================================*/

#pragma once


/**
 * Template for Linux target platforms
 */
template<bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY>
class TLinuxTargetPlatform
	: public TTargetPlatformBase<FLinuxPlatformProperties<IS_DEDICATED_SERVER, IS_CLIENT_ONLY> >
{
public:

	typedef public TTargetPlatformBase<FLinuxPlatformProperties<IS_DEDICATED_SERVER, IS_CLIENT_ONLY> > TSuper;

	/**
	 * Default constructor.
	 */
	TLinuxTargetPlatform( )
	{
		LocalDevice = MakeShareable(new FLinuxTargetDevice(*this, FTargetDeviceId(PlatformName(), FPlatformProcess::ComputerName())));
	
		#if WITH_ENGINE
			FConfigCacheIni::LoadLocalIniFile(EngineSettings, TEXT("Engine"), true, *PlatformName());
			TextureLODSettings.Initialize(EngineSettings, TEXT("SystemSettings"));
			StaticMeshLODSettings.Initialize(EngineSettings);
		#endif
	}


public:

	// Begin ITargetPlatform interface

	virtual void EnableDeviceCheck(bool OnOff) OVERRIDE {}

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const OVERRIDE
	{
		// TODO: ping all the machines in a local segment and/or try to connect to port 22 of those that respond
		OutDevices.Reset();
		OutDevices.Add(LocalDevice);
	}

	virtual ECompressionFlags GetBaseCompressionMethod( ) const OVERRIDE
	{
		return COMPRESS_ZLIB;
	}

	virtual bool GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFiles, TArray<FString>& OutMissingFiles ) const OVERRIDE
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


	virtual bool GenerateStreamingInstallManifest(const TMultiMap<FString, int32>& ChunkMap, const TSet<int32>& ChunkIDsInUse) const OVERRIDE
	{
		return true;
	}

	virtual ITargetDevicePtr GetDefaultDevice( ) const OVERRIDE
	{
		return LocalDevice;
	}

	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId ) OVERRIDE
	{
		if (LocalDevice.IsValid() && (DeviceId == LocalDevice->GetId()))
		{
			return LocalDevice;
		}

		FTargetDeviceId UATFriendlyId(TEXT("linux"), DeviceId.GetDeviceName());
		return MakeShareable(new FLinuxTargetDevice(*this, UATFriendlyId));
	}

	virtual FString GetIconPath( ETargetPlatformIcons::IconType IconType ) const OVERRIDE
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

	virtual bool IsRunningPlatform( ) const OVERRIDE
	{
		return false;
	}

	bool SupportsFeature(ETargetPlatformFeatures::Type Feature) const OVERRIDE
	{
		if (Feature == ETargetPlatformFeatures::UserCredentials || Feature == ETargetPlatformFeatures::Packaging)
		{
			return true;
		}

		return TTargetPlatformBase<FLinuxPlatformProperties<IS_DEDICATED_SERVER, IS_CLIENT_ONLY>>::SupportsFeature(Feature);
	}

#if WITH_ENGINE
	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		// no shaders needed for dedicated server target
		if (!IS_DEDICATED_SERVER)
		{
			static FName NAME_GLSL_150(TEXT("GLSL_150"));
			static FName NAME_GLSL_430(TEXT("GLSL_430"));

			OutFormats.AddUnique(NAME_GLSL_150);
			OutFormats.AddUnique(NAME_GLSL_430);
		}
	}

	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings( ) const OVERRIDE
	{
		return StaticMeshLODSettings;
	}

	virtual void GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const OVERRIDE
	{
		if (!IS_DEDICATED_SERVER)
		{
			// just use the standard texture format name for this texture
			OutFormats.Add(GetDefaultTextureFormatName(InTexture, EngineSettings));
		}
	}

	virtual const struct FTextureLODSettings& GetTextureLODSettings( ) const OVERRIDE
	{
		return TextureLODSettings;
	}

	virtual FName GetWaveFormat( class USoundWave* Wave ) const OVERRIDE
	{
		static FName NAME_OGG(TEXT("OGG"));

		return NAME_OGG;
	}
#endif //WITH_ENGINE

	DECLARE_DERIVED_EVENT(FAndroidTargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) OVERRIDE
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(FAndroidTargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) OVERRIDE
	{
		return DeviceLostEvent;
	}

	// End ITargetPlatform interface

private:

	// Holds the local device.
	FLinuxTargetDevicePtr LocalDevice;

#if WITH_ENGINE
	// Holds the Engine INI settings for quick use.
	FConfigFile EngineSettings;

	// Holds the texture LOD settings.
	FTextureLODSettings TextureLODSettings;

	// Holds static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;
#endif // WITH_ENGINE

private:

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};
