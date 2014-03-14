// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericWindowsTargetPlatform.h: Declares the TGenericWindowsTargetPlatform class template.
=============================================================================*/

#pragma once


/**
 * Template for Windows target platforms
 */
template<bool HAS_EDITOR_DATA, bool IS_DEDICATED_SERVER, bool IS_CLIENT_ONLY>
class TGenericWindowsTargetPlatform
	: public TTargetPlatformBase<FWindowsPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> >
{
public:

	typedef public TTargetPlatformBase<FWindowsPlatformProperties<HAS_EDITOR_DATA, IS_DEDICATED_SERVER, IS_CLIENT_ONLY> > TSuper;

	/**
	 * Default constructor.
	 */
	TGenericWindowsTargetPlatform( )
	{
		LocalDevice = MakeShareable(new TLocalPcTargetDevice<HAS_EDITOR_DATA>(*this));
		
		#if WITH_ENGINE
			FConfigCacheIni::LoadLocalIniFile(EngineSettings, TEXT("Engine"), true, *PlatformName());
			TextureLODSettings.Initialize(EngineSettings, TEXT("SystemSettings"));
			StaticMeshLODSettings.Initialize(EngineSettings);
		#endif
	}

public:

	// Begin ITargetPlatform interface

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const OVERRIDE
	{
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

		if (ProjectName.IsEmpty() || ProjectRoot.IsEmpty())
		{
			return false;
		}

		// append binaries
		if (Artifacts & ETargetPlatformBuildArtifacts::Engine)
		{
			const FString EngineBinariesDir = FPaths::EngineDir() / TEXT("Binaries") / GetBinariesSubDir();
			const FString DeployedBinariesDir = FString(TEXT("Engine")) / TEXT("Binaries") / GetBinariesSubDir();

			// executable
			{
				FString ExecutableBaseName;
				FString ProjectBinariesDir = ProjectRoot / TEXT("Binaries") / GetBinariesSubDir();

				// content-only projects use game agnostic binaries
				if (IFileManager::Get().DirectoryExists(*ProjectBinariesDir))
				{
					ExecutableBaseName = ProjectName;
				}
				else
				{
					if (BuildTarget == EBuildTargets::Game)
					{
						ExecutableBaseName = TEXT("UE4Game");
					}
					else if (BuildTarget == EBuildTargets::Server)
					{
						ExecutableBaseName = TEXT("UE4Game");
					}
					else if (BuildTarget == EBuildTargets::Editor)
					{
						ExecutableBaseName = TEXT("UE4Editor");
					}
					ProjectBinariesDir = EngineBinariesDir;
				}
		
				if (BuildConfiguration != EBuildConfigurations::Development)
				{
					ExecutableBaseName += FString::Printf(TEXT("-%s-%s.exe"), *PlatformName(), EBuildConfigurations::ToString(BuildConfiguration));
				}

				// add executable
				{
					FString ExecutablePath = ProjectBinariesDir / ExecutableBaseName + TEXT(".exe");

					if (FPaths::FileExists(*ExecutablePath))
					{
						OutFiles.Add(ExecutablePath, DeployedBinariesDir / ExecutableBaseName + TEXT(".exe"));
					}
					else
					{
						OutMissingFiles.Add(ExecutablePath);
					}
				}

				// add debug symbols
				if (Artifacts & ETargetPlatformBuildArtifacts::DebugSymbols)
				{
					FString DebugSymbolsPath = ProjectBinariesDir / ExecutableBaseName + TEXT(".pdb");

					if (FPaths::FileExists(*DebugSymbolsPath))
					{
						OutFiles.Add(DebugSymbolsPath, DeployedBinariesDir / ExecutableBaseName + TEXT(".pdb"));
					}
					else
					{
						OutMissingFiles.Add(DebugSymbolsPath);
					}
				}
			}

			// dynamic link libraries
			{
				// @todo platforms: this needs some major improvement, so that only required files are included
				TArray<FString> FileNames;

				IFileManager::Get().FindFiles(FileNames, *(EngineBinariesDir / TEXT("*.dll")), true, false);

				for (int32 FileIndex = 0; FileIndex < FileNames.Num(); ++FileIndex)
				{
					// add library
					const FString& FileName = FileNames[FileIndex];
					OutFiles.Add(EngineBinariesDir / FileName, DeployedBinariesDir / FileName);

					// add debug symbols
					if (Artifacts & ETargetPlatformBuildArtifacts::DebugSymbols)
					{
						FString DebugSymbolFile = FPaths::GetBaseFilename(FileName) + TEXT(".pdb");
						FString DebugySymbolPath = EngineBinariesDir / DebugSymbolFile;

						if (FPaths::FileExists(DebugySymbolPath))
						{
							OutFiles.Add(DebugySymbolPath, DeployedBinariesDir / DebugSymbolFile);
						}
						else
						{
							OutMissingFiles.Add(DebugySymbolPath);
						}
					}
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

					if (FileName.EndsWith(TEXT(".dll")) && !FileName.Contains(TEXT("Mcpp")) && !FileName.Contains(TEXT("NoRedist")) )
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

	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId )
	{
		if (LocalDevice.IsValid() && (DeviceId == LocalDevice->GetId()))
		{
			return LocalDevice;
		}

		return NULL;
	}

	virtual FString GetIconPath( ETargetPlatformIcons::IconType IconType ) const OVERRIDE
	{
		if (IS_DEDICATED_SERVER)
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
				return FString(TEXT("Launcher/Windows/Platform_WindowsServer_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
				return FString(TEXT("Launcher/Windows/Platform_WindowsServer_128x"));
			}
		}
		else if (UE_GAME)
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
				return FString(TEXT("Launcher/Windows/Platform_WindowsNoEditor_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
				return FString(TEXT("Launcher/Windows/Platform_WindowsNoEditor_128x"));
			}
		}
		else
		{
			switch (IconType)
			{
			case ETargetPlatformIcons::Normal:
				return FString(TEXT("Launcher/Windows/Platform_Windows_24x"));

			case ETargetPlatformIcons::Large:
			case ETargetPlatformIcons::XLarge:
				return FString(TEXT("Launcher/Windows/Platform_Windows_128x"));
			}
		}

		return FString();
	}

	virtual bool IsRunningPlatform( ) const OVERRIDE
	{
		// Must be Windows platform as editor for this to be considered a running platform
		return PLATFORM_WINDOWS && !UE_SERVER && !UE_GAME && WITH_EDITOR && HAS_EDITOR_DATA;
	}

	virtual bool SupportsFeature( ETargetPlatformFeatures::Type Feature ) const OVERRIDE
	{
		// we currently do not have a build target for WindowsServer
		if (Feature == ETargetPlatformFeatures::Packaging)
		{
			return (HAS_EDITOR_DATA || !IS_DEDICATED_SERVER);
		}

		return TSuper::SupportsFeature(Feature);
	}

#if WITH_ENGINE
	virtual void GetShaderFormats( TArray<FName>& OutFormats ) const OVERRIDE
	{
		// no shaders needed for dedicated server target
		if (!IS_DEDICATED_SERVER)
		{
			// Only shader formats needed for cooked builds should be added here.
			// TODO: this should be configurable per-project, e.g. you may or
			//       may not wish to support SM4 or OpenGL.
			static FName NAME_PCD3D_SM5(TEXT("PCD3D_SM5"));
			static FName NAME_PCD3D_SM4(TEXT("PCD3D_SM4"));

			OutFormats.AddUnique(NAME_PCD3D_SM5);
			OutFormats.AddUnique(NAME_PCD3D_SM4);
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

	DECLARE_DERIVED_EVENT(TGenericWindowsTargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) OVERRIDE
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(TGenericWindowsTargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) OVERRIDE
	{
		return DeviceLostEvent;
	}

	// End ITargetPlatform interface

protected:

	/**
	 * Temporary helper until we refactor Windows build targets.
	 * Basically maps WindowsNoEditor to Win32 and Windows to Win64.
	 */
	FString GetBinariesSubDir( ) const
	{
		if (HAS_EDITOR_DATA)
		{
			return TEXT("Win64");
		}

		return TEXT("Win32");
	}

private:

	// Holds the local device.
	ITargetDevicePtr LocalDevice;

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
