// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidTargetPlatform.inl: Implements the FAndroidTargetPlatform class.
=============================================================================*/

/* FAndroidTargetPlatform structors
 *****************************************************************************/

inline FAndroidTargetPlatform::FAndroidTargetPlatform( )
{
	#if WITH_ENGINE
		FConfigCacheIni::LoadLocalIniFile(EngineSettings, TEXT("Engine"), true, *PlatformName());
		TextureLODSettings.Initialize(EngineSettings, TEXT("SystemSettings"));
		StaticMeshLODSettings.Initialize(EngineSettings);
	#endif

	TickDelegate = FTickerDelegate::CreateRaw(this, &FAndroidTargetPlatform::HandleTicker);
	FTicker::GetCoreTicker().AddTicker(TickDelegate, 5.0f);
}


inline FAndroidTargetPlatform::~FAndroidTargetPlatform()
{ 
	 FTicker::GetCoreTicker().RemoveTicker(TickDelegate);
}


/* ITargetPlatform overrides
 *****************************************************************************/

inline void FAndroidTargetPlatform::GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const
{
	OutDevices.Reset();

	for (auto Iter = Devices.CreateConstIterator(); Iter; ++Iter)
	{
		OutDevices.Add(Iter.Value());
	}
}


inline ECompressionFlags FAndroidTargetPlatform::GetBaseCompressionMethod( ) const
{
	return COMPRESS_ZLIB;
}


inline ITargetDevicePtr FAndroidTargetPlatform::GetDefaultDevice( ) const
{
	// return the first device in the list
	if (Devices.Num() > 0)
	{
		auto Iter = Devices.CreateConstIterator();
		if (Iter)
		{
			return Iter.Value();
		}
	}

	return nullptr;
}


inline bool FAndroidTargetPlatform::GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFiles, TArray<FString>& OutMissingFiles ) const
{
	//@todo android: implement build artifacts gathering
	return false;
}


inline ITargetDevicePtr FAndroidTargetPlatform::GetDevice( const FTargetDeviceId& DeviceId )
{
	if (DeviceId.GetPlatformName() == PlatformName())
	{
		return Devices.FindRef(DeviceId.GetDeviceName());
	}

	return nullptr;
}


inline FString FAndroidTargetPlatform::GetIconPath( ETargetPlatformIcons::IconType IconType ) const
{
	switch (IconType)
	{
	case ETargetPlatformIcons::Normal:
		return FString::Printf(TEXT("Launcher/Android/Platform_%s_24x"), *PlatformName());

	case ETargetPlatformIcons::Large:
	case ETargetPlatformIcons::XLarge:
		return FString(TEXT("Launcher/Android/Platform_Android_128x"));
	}

	return FString();
}


inline bool FAndroidTargetPlatform::IsRunningPlatform( ) const
{
	return false; // This platform never runs the target platform framework
}

bool FAndroidTargetPlatform::IsSdkInstalled(bool bProjectHasCode, FString& OutDocumentationPath) const
{
	OutDocumentationPath = FString("Shared/Tutorials/SettingUpAndroidTutorial");

	TCHAR ANDROID_HOME[MAX_PATH];
	TCHAR JAVA_HOME[MAX_PATH];
	TCHAR ANT_HOME[MAX_PATH];
	TCHAR NDKROOT[MAX_PATH];
	FPlatformMisc::GetEnvironmentVariable(TEXT("ANDROID_HOME"), ANDROID_HOME, MAX_PATH);
	FPlatformMisc::GetEnvironmentVariable(TEXT("JAVA_HOME"), JAVA_HOME, MAX_PATH);
	FPlatformMisc::GetEnvironmentVariable(TEXT("ANT_HOME"), ANT_HOME, MAX_PATH);
	FPlatformMisc::GetEnvironmentVariable(TEXT("NDKROOT"), NDKROOT, MAX_PATH);

	// make sure ANDROID_HOME points to the right thing
	if (ANDROID_HOME[0] == 0 || 
		IFileManager::Get().FileSize(*(FString(ANDROID_HOME) / TEXT("platform-tools/adb.exe"))) < 0)
	{
		return false;
	}

	// make sure that JAVA_HOME points to the right thing
	if (JAVA_HOME[0] == 0 ||
		IFileManager::Get().FileSize(*(FString(JAVA_HOME) / TEXT("bin/javac.exe"))) < 0)
	{
		return false;
	}

	// now look for ANT_HOME, or the ADT workaround of looking for a plugin
	if (ANT_HOME[0] == 0)
	{
		// look for plugins in eclipse (this is enough to assume we have an ant plugin)
		if (!IFileManager::Get().DirectoryExists(*(FString(ANDROID_HOME) / TEXT("../eclipse/plugins"))))
		{
			return false;
		}
	}

	// we need NDKROOT if the game has code
	if (bProjectHasCode)
	{
		if (NDKROOT[0] == 0 ||
			IFileManager::Get().FileSize(*(FString(NDKROOT) / TEXT("ndk-build.cmd"))) < 0)
		{
			return false;
		}
	}

	return true;
}

#if WITH_ENGINE

inline void FAndroidTargetPlatform::GetShaderFormats( TArray<FName>& OutFormats ) const
{
	static FName NAME_OPENGL_ES2(TEXT("GLSL_ES2"));
	OutFormats.AddUnique(NAME_OPENGL_ES2);
}


inline const FStaticMeshLODSettings& FAndroidTargetPlatform::GetStaticMeshLODSettings( ) const
{
	return StaticMeshLODSettings;
}


inline void FAndroidTargetPlatform::GetTextureFormats( const UTexture* InTexture, TArray<FName>& OutFormats ) const
{
	check(InTexture);

	// The order we add texture formats to OutFormats is important. When multiple formats are cooked
	// and supported by the device, the first supported format listed will be used. 
	// eg, ETC1/uncompressed should always be last

	bool bNoCompression = InTexture->CompressionNone								// Code wants the texture uncompressed.
		|| (InTexture->LODGroup == TEXTUREGROUP_ColorLookupTable)	// Textures in certain LOD groups should remain uncompressed.
		|| (InTexture->LODGroup == TEXTUREGROUP_Bokeh)
		|| (InTexture->CompressionSettings == TC_EditorIcon)
		|| (InTexture->Source.GetSizeX() < 4)						// Don't compress textures smaller than the DXT block size.
		|| (InTexture->Source.GetSizeY() < 4)
		|| (InTexture->Source.GetSizeX() % 4 != 0)
		|| (InTexture->Source.GetSizeY() % 4 != 0);

	// Determine the pixel format of the compressed texture.
	if (bNoCompression && InTexture->HasHDRSource())
	{
		OutFormats.Add(AndroidTexFormat::NameRGBA16F);
	}
	else if (bNoCompression)
	{
		OutFormats.Add(AndroidTexFormat::NameBGRA8);
	}
	else if (InTexture->CompressionSettings == TC_HDR)
	{
		OutFormats.Add(AndroidTexFormat::NameRGBA16F);
	}
	else if (InTexture->CompressionSettings == TC_Normalmap)
	{
		AddTextureFormatIfSupports(AndroidTexFormat::NamePVRTC4, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameDXT5, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameATC_RGBA_I, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC2, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC1, OutFormats);
	}
	else if (InTexture->CompressionSettings == TC_Displacementmap)
	{
		OutFormats.Add(AndroidTexFormat::NameRGBA16F);
	}
	else if (InTexture->CompressionSettings == TC_VectorDisplacementmap)
	{
		OutFormats.Add(AndroidTexFormat::NameBGRA8);
	}
	else if (InTexture->CompressionSettings == TC_Grayscale)
	{
		OutFormats.Add(AndroidTexFormat::NameG8);
	}
	else if (InTexture->CompressionSettings == TC_Alpha)
	{
		OutFormats.Add(AndroidTexFormat::NameG8);
	}
	else if (InTexture->bForcePVRTC4)
	{
		AddTextureFormatIfSupports(AndroidTexFormat::NamePVRTC4, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameDXT5, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameATC_RGBA_I, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC2, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC1, OutFormats);
	}
	else if (InTexture->CompressionNoAlpha)
	{
		AddTextureFormatIfSupports(AndroidTexFormat::NamePVRTC2, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameDXT1, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameATC_RGB, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameETC2_RGB, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameETC1, OutFormats);
	}
	else if (InTexture->bDitherMipMapAlpha)
	{
		AddTextureFormatIfSupports(AndroidTexFormat::NamePVRTC4, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameDXT5, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameATC_RGBA_I, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC2, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC1, OutFormats);
	}
	else
	{
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoPVRTC, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoDXT, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoATC, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC2, OutFormats);
		AddTextureFormatIfSupports(AndroidTexFormat::NameAutoETC1, OutFormats);
	}
}


inline const struct FTextureLODSettings& FAndroidTargetPlatform::GetTextureLODSettings( ) const
{
	return TextureLODSettings;
}


inline FName FAndroidTargetPlatform::GetWaveFormat( class USoundWave* Wave ) const
{
	static FName NAME_OGG(TEXT("OGG"));		//@todo android: probably not ogg

	return NAME_OGG;
}

#endif //WITH_ENGINE


/* FAndroidTargetPlatform implementation
 *****************************************************************************/

inline void FAndroidTargetPlatform::AddTextureFormatIfSupports( FName Format, TArray<FName>& OutFormats ) const
{
	if (SupportsTextureFormat(Format))
	{
		OutFormats.Add(Format);
	}
}


inline bool FAndroidTargetPlatform::ExecuteAdbCommand( const FString& CommandLine, FString* OutStdOut, FString* OutStdErr ) const
{
	// get the SDK binaries folder
	TCHAR AndroidDirectory[32768] = { 0 };
	FPlatformMisc::GetEnvironmentVariable(TEXT("ANDROID_HOME"), AndroidDirectory, 32768);

	if (AndroidDirectory[0] == 0)
	{
		return false;
	}

	FString Filename = FString::Printf(TEXT("%s\\platform-tools\\adb.exe"), AndroidDirectory);

	// execute the command
	int32 ReturnCode;
	FString DefaultError;

	// make sure there's a place for error output to go if the caller specified nullptr
	if (!OutStdErr)
	{
		OutStdErr = &DefaultError;
	}

	FPlatformProcess::ExecProcess(*Filename, *CommandLine, &ReturnCode, OutStdOut, OutStdErr);

	if (ReturnCode != 0)
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("The Android SDK command '%s' failed to run. Return code: %d, Error: %s\n"), *CommandLine, ReturnCode, **OutStdErr);

		return false;
	}

	return true;
}


inline void FAndroidTargetPlatform::QueryConnectedDevices( )
{
	FString StdOut;

	// grab the list of devices via adb
	if (!ExecuteAdbCommand(TEXT("devices -l"), &StdOut, nullptr))
	{
		return;
	}

	// separate out each line
	TArray<FString> DeviceStrings;
	StdOut = StdOut.Replace(TEXT("\r"), TEXT("\n"));
	StdOut.ParseIntoArray(&DeviceStrings, TEXT("\n"), true);

	TArray<FString> ConnectedDeviceIds;

	for (int32 StringIndex = 0; StringIndex < DeviceStrings.Num(); ++StringIndex)
	{
		const FString& DeviceString = DeviceStrings[StringIndex];

		// skip over non-device lines

		if (DeviceString.StartsWith("* ") || DeviceString.StartsWith("List "))
		{
			continue;
		}

		// grab the device serial number
		int32 TabIndex;

		if (!DeviceString.FindChar(TCHAR(' '), TabIndex))
		{
			continue;
		}

		FString SerialNumber = DeviceString.Left(TabIndex);
		ConnectedDeviceIds.Add(DeviceString.Left(TabIndex));

		// create target device if needed
		FAndroidTargetDevicePtr& Device = Devices.FindOrAdd(SerialNumber);

		if (!Device.IsValid())
		{
			Device = MakeShareable(new FAndroidTargetDevice(*this, SerialNumber, GetAndroidVariantName()));
			DeviceDiscoveredEvent.Broadcast(Device.ToSharedRef());
		}

		Device->SetConnected(true);

		// parse device model
		FString Model;

		if (FParse::Value(*DeviceString, TEXT("model:"), Model))
		{
			Device->SetModel(Model);
		}

		// parse device name
		FString DeviceName;

		if (FParse::Value(*DeviceString, TEXT("device:"), DeviceName))
		{
			Device->SetDeviceName(DeviceName);
		}
	}

	// remove disconnected devices
	for (auto Iter = Devices.CreateIterator(); Iter; ++Iter)
	{
		if (!ConnectedDeviceIds.Contains(Iter.Key()))
		{
			FAndroidTargetDevicePtr RemovedDevice = Iter.Value();
			RemovedDevice->SetConnected(false);

			Iter.RemoveCurrent();

			DeviceLostEvent.Broadcast(RemovedDevice.ToSharedRef());
		}
	}
}


/* FAndroidTargetPlatform callbacks
 *****************************************************************************/

inline bool FAndroidTargetPlatform::HandleTicker( float DeltaTime )
{
	QueryConnectedDevices();

	return true;
}
