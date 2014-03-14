// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TTargetPlatformBase.h: Declares the TTargetPlatformBase template class.
=============================================================================*/

#pragma once


/**
 * Base class for target platforms.
 */
class FTargetPlatformBase
	: public ITargetPlatform
{
public:

	// Begin ITargetPlatform interface

	virtual bool AddDevice( const FString& DeviceName, bool bDefault ) OVERRIDE
	{
		return false;
	}

#if WITH_ENGINE
	virtual void GetReflectionCaptureFormats( TArray<FName>& OutFormats ) const
	{
		OutFormats.Add(FName(TEXT("FullHDR")));
	}


#ifdef TEXTURE_H_INCLUDED // defined in Texture.h, this way we know if UTexture is available, needed for Clang
	FName GetDefaultTextureFormatName( const UTexture* Texture, const FConfigFile& EngineSettings ) const
	{
		FName TextureFormatName = NAME_None;

#if WITH_EDITOR
		// Supported texture format names.
		static FName NameDXT1(TEXT("DXT1"));
		static FName NameDXT3(TEXT("DXT3"));
		static FName NameDXT5(TEXT("DXT5"));
		static FName NameDXT5n(TEXT("DXT5n"));
		static FName NameAutoDXT(TEXT("AutoDXT"));
		static FName NameBC4(TEXT("BC4"));
		static FName NameBC5(TEXT("BC5"));
		static FName NameBGRA8(TEXT("BGRA8"));
		static FName NameXGXR8(TEXT("XGXR8"));
		static FName NameG8(TEXT("G8"));
		static FName NameVU8(TEXT("VU8"));
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

		bool bUseDXT5NormalMap = false;

		FString UseDXT5NormalMapsString;

		if (EngineSettings.GetString(TEXT("SystemSettings"), TEXT("Compat.UseDXT5NormalMaps"), UseDXT5NormalMapsString))
		{
			bUseDXT5NormalMap = FCString::ToBool(*UseDXT5NormalMapsString);
		}

		ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();

		// Determine the pixel format of the (un/)compressed texture
		if (bNoCompression)
		{
			if (Texture->HasHDRSource())
			{
				TextureFormatName = NameRGBA16F;
			}
			else if (SourceFormat == TSF_G8 || Texture->CompressionSettings == TC_Grayscale)
			{
				TextureFormatName = NameG8;
			}
			else if (Texture->CompressionSettings == TC_Normalmap && bUseDXT5NormalMap)
			{
				TextureFormatName = NameXGXR8;
			}
			else
			{
				TextureFormatName = NameBGRA8;
			}
		}
		else if (Texture->CompressionSettings == TC_HDR)
		{
			TextureFormatName = NameRGBA16F;
		}
		else if (Texture->CompressionSettings == TC_Normalmap)
		{
			TextureFormatName = bUseDXT5NormalMap ? NameDXT5n : NameBC5;
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
			TextureFormatName = NameBC4;
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
#endif //WITH_EDITOR

		return TextureFormatName;
	}
#else //TEXTURE_H_INCLUDED

	FName GetDefaultTextureFormatName( const UTexture* Texture, const FConfigFile& EngineSettings ) const;

#endif //TEXTURE_H_INCLUDED
#endif //WITH_ENGINE

	virtual bool PackageBuild( const FString& InPackgeDirectory ) OVERRIDE
	{
		return true;
	}

	virtual bool IsSdkInstalled(bool bProjectHasCode, FString& OutDocumentationPath) const OVERRIDE
	{
		return true;
	}

	// End ITargetPlatform interface
};


/**
 * Template for target platforms.
 *
 * @param TPlatformProperties - Type of platform properties.
 */
template<typename TPlatformProperties>
class TTargetPlatformBase
	: public FTargetPlatformBase
{
public:

	/**
	 * Default constructor.
	 */
	TTargetPlatformBase( )
	{
		// HasEditorOnlyData and RequiresCookedData are mutually exclusive.
		check(TPlatformProperties::HasEditorOnlyData() != TPlatformProperties::RequiresCookedData());
	}

public:

	// Begin ITargetPlatform interface

	virtual FText DisplayName( ) const OVERRIDE
	{
		return FText::FromString(TPlatformProperties::DisplayName());
	}

	virtual bool HasEditorOnlyData( ) const OVERRIDE
	{
		return TPlatformProperties::HasEditorOnlyData();
	}

	virtual bool IsLittleEndian( ) const OVERRIDE
	{
		return TPlatformProperties::IsLittleEndian();
	}

	virtual bool IsServerOnly( ) const OVERRIDE
	{
		return TPlatformProperties::IsServerOnly();
	}

	virtual bool IsClientOnly( ) const OVERRIDE
	{
		return TPlatformProperties::IsClientOnly();
	}

	virtual FString PlatformName( ) const OVERRIDE
	{
		return FString(TPlatformProperties::PlatformName());
	}

	virtual FString IniPlatformName( ) const OVERRIDE
	{
		return FString(TPlatformProperties::IniPlatformName());
	}

	virtual bool RequiresCookedData( ) const OVERRIDE
	{
		return TPlatformProperties::RequiresCookedData();
	}

	virtual bool RequiresUserCredentials() const OVERRIDE
	{
		return TPlatformProperties::RequiresUserCredentials();
	}

	virtual bool SupportsBuildTarget( EBuildTargets::Type BuildTarget ) const OVERRIDE
	{
		return TPlatformProperties::SupportsBuildTarget(BuildTarget);
	}

	virtual bool SupportsFeature( ETargetPlatformFeatures::Type Feature ) const OVERRIDE
	{
		switch (Feature)
		{
		case ETargetPlatformFeatures::DistanceFieldShadows:
			return TPlatformProperties::SupportsDistanceFieldShadows();

		case ETargetPlatformFeatures::GrayscaleSRGB:
			return TPlatformProperties::SupportsGrayscaleSRGB();

		case ETargetPlatformFeatures::HighQualityLightmaps:
			return TPlatformProperties::SupportsHighQualityLightmaps();

		case ETargetPlatformFeatures::LowQualityLightmaps:
			return TPlatformProperties::SupportsLowQualityLightmaps();

		case ETargetPlatformFeatures::MultipleGameInstances:
			return TPlatformProperties::SupportsMultipleGameInstances();

		case ETargetPlatformFeatures::Packaging:
			return false;

		case ETargetPlatformFeatures::Tessellation:
			return TPlatformProperties::SupportsTessellation();

		case ETargetPlatformFeatures::TextureStreaming:
			return TPlatformProperties::SupportsTextureStreaming();

		case ETargetPlatformFeatures::VertexShaderTextureSampling:
			return TPlatformProperties::SupportsVertexShaderTextureSampling();
		}

		return false;
	}
	
	virtual uint32 MaxGpuSkinBones( ) const OVERRIDE
	{
		return TPlatformProperties::MaxGpuSkinBones();
	}

#if WITH_ENGINE
	virtual FName GetPhysicsFormat( class UBodySetup* Body ) const OVERRIDE
	{
		return FName(TPlatformProperties::GetPhysicsFormat());
	}
#endif // WITH_ENGINE

	// End ITargetPlatform interface
};
