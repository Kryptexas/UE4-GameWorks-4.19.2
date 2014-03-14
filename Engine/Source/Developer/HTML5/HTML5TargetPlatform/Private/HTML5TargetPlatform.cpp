// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HTML5TargetPlatform.cpp: Implements the FHTML5TargetPlatform class.
=============================================================================*/

#include "HTML5TargetPlatformPrivatePCH.h"

#if WITH_EDITOR
#include "Texture.h"
#endif 

 /* FHTML5TargetPlatform structors
 *****************************************************************************/

FHTML5TargetPlatform::FHTML5TargetPlatform( )
{
	LocalDevice = MakeShareable(new FHTML5TargetDevice(*this, FPlatformProcess::ComputerName()));

	// load the final HTML5 engine settings for this game
	FConfigCacheIni::LoadLocalIniFile(HTML5EngineSettings, TEXT("Engine"), true, *PlatformName());

#if WITH_ENGINE
	// load up texture settings from the config file
	HTML5LODSettings.Initialize(HTML5EngineSettings, TEXT("SystemSettings"));
	StaticMeshLODSettings.Initialize(HTML5EngineSettings);
#endif
}


 /* ITargetPlatform interface
 *****************************************************************************/

void FHTML5TargetPlatform::GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const
{
	OutDevices.Reset();
	OutDevices.Add(LocalDevice);
}


ECompressionFlags FHTML5TargetPlatform::GetBaseCompressionMethod( ) const
{
	return COMPRESS_ZLIB;
}


bool FHTML5TargetPlatform::GetBuildArtifacts( const FString& ProjectPath, EBuildTargets::Type BuildTarget, EBuildConfigurations::Type BuildConfiguration, ETargetPlatformBuildArtifacts::Type Artifacts, TMap<FString, FString>& OutFiles, TArray<FString>& OutMissingFiles ) const
{
	if ((BuildTarget != EBuildTargets::Game) || (BuildConfiguration == EBuildConfigurations::Unknown))
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
		const FString EngineBinariesDir = FPaths::EngineDir() / TEXT("Binaries") / PlatformName();
		const FString DeployedBinariesDir = FString(TEXT("Engine/Binaries")) / PlatformName();

		// executable
		{
			FString ExecutableName;
			FString ProjectBinariesDir = ProjectRoot / TEXT("Binaries");

			// content-only projects use game agnostic binaries
			if (IFileManager::Get().DirectoryExists(*ProjectBinariesDir))
			{
				ExecutableName = ProjectName;
			}
			else
			{
				ExecutableName = TEXT("UE4");
				ProjectBinariesDir = EngineBinariesDir;
			}

			if (BuildConfiguration != EBuildConfigurations::Development)
			{
				ExecutableName += FString::Printf(TEXT("-HTML5-%s"), EBuildConfigurations::ToString(BuildConfiguration));
			}

			ExecutableName += TEXT(".js");
			ProjectBinariesDir /= PlatformName();

			// add executable
			{
				const FString ExecutablePath = ProjectBinariesDir / ExecutableName;

				if (FPaths::FileExists(*ExecutablePath))
				{
					OutFiles.Add(ExecutablePath, DeployedBinariesDir / ExecutableName);
				}
				else
				{
					OutMissingFiles.Add(ExecutablePath);
				}

				const FString MemFileName = ExecutableName + TEXT(".mem");
				const FString MemFilePath = ProjectBinariesDir / MemFileName;

				if (FPaths::FileExists(*MemFilePath))
				{
					OutFiles.Add(MemFilePath, DeployedBinariesDir / MemFileName);
				}
				else
				{
					OutMissingFiles.Add(MemFilePath);
				}
			}
		}
	}

	// append engine content
	if (Artifacts & ETargetPlatformBuildArtifacts::Content)
	{
		// @todo: handle Engine configuration files and content
	}

	// append game content
	if (Artifacts & ETargetPlatformBuildArtifacts::Content)
	{
		// @todo: handle game configuration files and content
	}

	return (OutMissingFiles.Num() == 0);
}


ITargetDevicePtr FHTML5TargetPlatform::GetDefaultDevice( ) const
{
	return LocalDevice;
}


ITargetDevicePtr FHTML5TargetPlatform::GetDevice( const FTargetDeviceId& DeviceId )
{
	if (LocalDevice.IsValid() && (DeviceId == LocalDevice->GetId()))
	{
		return LocalDevice;
	}

	return NULL;
}


FString FHTML5TargetPlatform::GetIconPath( ETargetPlatformIcons::IconType IconType ) const
{
	switch (IconType)
	{
	case ETargetPlatformIcons::Normal:
		return FString(TEXT("Launcher/HTML5/Platform_HTML5_24x"));

	case ETargetPlatformIcons::Large:
	case ETargetPlatformIcons::XLarge:
		return FString(TEXT("Launcher/HTML5/Platform_HTML5_128x"));
	}

	return FString();
}


bool FHTML5TargetPlatform::IsRunningPlatform( ) const
{
	return false; // but this will never be called because this platform doesn't run the target platform framework
}


#if WITH_ENGINE

static FName NAME_OPENGL_ES2_WEBGL(TEXT("GLSL_ES2_WEBGL"));

void FHTML5TargetPlatform::GetShaderFormats( TArray<FName>& OutFormats ) const
{
	OutFormats.AddUnique(NAME_OPENGL_ES2_WEBGL);
}


const class FStaticMeshLODSettings& FHTML5TargetPlatform::GetStaticMeshLODSettings( ) const
{
	return StaticMeshLODSettings;
}


void FHTML5TargetPlatform::GetTextureFormats( const UTexture* Texture, TArray<FName>& OutFormats ) const
{
	FName TextureFormatName = NAME_None;

#if WITH_EDITOR
	// Supported texture format names.
	static FName NameDXT1(TEXT("DXT1"));
	static FName NameDXT3(TEXT("DXT3"));
	static FName NameDXT5(TEXT("DXT5"));
	static FName NameDXT5n(TEXT("DXT5n"));
	static FName NameAutoDXT(TEXT("AutoDXT"));
	static FName NameBGRA8(TEXT("BGRA8"));
	static FName NameG8(TEXT("G8"));
	static FName NameRGBA16F(TEXT("RGBA16F"));

	bool bNoCompression = Texture->CompressionNone				// Code wants the texture uncompressed.
		|| (HasEditorOnlyData() && Texture->DeferCompression)	// The user wishes to defer compression, this is ok for the Editor only.
		|| (Texture->CompressionSettings == TC_EditorIcon)
		|| (Texture->LODGroup == TEXTUREGROUP_ColorLookupTable)	// Textures in certain LOD groups should remain uncompressed.
		|| (Texture->LODGroup == TEXTUREGROUP_Bokeh)
		|| (Texture->LODGroup == TEXTUREGROUP_IESLightProfile)
		|| (Texture->Source.GetSizeX() < 4) // Don't compress textures smaller than the DXT block size.
		|| (Texture->Source.GetSizeY() < 4)
		|| (Texture->Source.GetSizeX() % 4 != 0)
		|| (Texture->Source.GetSizeY() % 4 != 0);


	ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();

	// Determine the pixel format of the (un/)compressed texture
	if (bNoCompression)
	{
		TextureFormatName = NameBGRA8;
	}
	else if (Texture->CompressionSettings == TC_HDR)
	{
		TextureFormatName = NameRGBA16F;
	}
	else if (Texture->CompressionSettings == TC_Normalmap)
	{
		TextureFormatName =  NameDXT5;
	}
	else if (Texture->CompressionSettings == TC_Displacementmap)
	{
		TextureFormatName = NameG8;
	}
	else if (Texture->CompressionSettings == TC_VectorDisplacementmap)
	{
		TextureFormatName = NameBGRA8;
	}
	else if (Texture->CompressionSettings == TC_Grayscale)
	{
		TextureFormatName = NameG8;
	}
	else if( Texture->CompressionSettings == TC_Alpha)
	{
		TextureFormatName = NameDXT5;
	}
	else if (Texture->CompressionNoAlpha)
	{
		TextureFormatName = NameDXT1;
	}
	else if (Texture->bDitherMipMapAlpha)
	{
		TextureFormatName = NameDXT5;
	}
	else
	{
		TextureFormatName = NameAutoDXT;
	}

	// Some PC GPUs don't support sRGB read from G8 textures (e.g. AMD DX10 cards on ShaderModel3.0)
	// This solution requires 4x more memory but a lot of PC HW emulate the format anyway
	if ((TextureFormatName == NameG8) && Texture->SRGB && !SupportsFeature(ETargetPlatformFeatures::GrayscaleSRGB))
	{
		TextureFormatName = NameBGRA8;
	}
#endif 

	OutFormats.Add( TextureFormatName); 
}


const struct FTextureLODSettings& FHTML5TargetPlatform::GetTextureLODSettings( ) const
{
	return HTML5LODSettings;
}


FName FHTML5TargetPlatform::GetWaveFormat( USoundWave* Wave ) const
{
	static FName NAME_OGG(TEXT("OGG"));
	return NAME_OGG;
}

#endif // WITH_ENGINE
