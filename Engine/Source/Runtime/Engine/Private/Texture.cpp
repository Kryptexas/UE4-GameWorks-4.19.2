// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Texture.cpp: Implementation of UTexture.
=============================================================================*/

#include "EnginePrivate.h"
#include "TargetPlatform.h"
#include "ImageWrapper.h"

#if WITH_EDITORONLY_DATA
#include "DDSLoader.h"
#endif

DEFINE_LOG_CATEGORY(LogTexture);

#if STATS
DECLARE_STATS_GROUP(TEXT("TextureGroup"),STATGROUP_TextureGroup);

// Declare the stats for each Texture Group.
#define DECLARETEXTUREGROUPSTAT(Group) DECLARE_MEMORY_STAT(TEXT(#Group),STAT_##Group,STATGROUP_TextureGroup);
FOREACH_ENUM_TEXTUREGROUP(DECLARETEXTUREGROUPSTAT)
#undef DECLARETEXTUREGROUPSTAT

// Initialize TextureGroupStatFNames array with the FNames for each stats.
FName FTextureResource::TextureGroupStatFNames[TEXTUREGROUP_MAX] =
	{
		#define ASSIGNTEXTUREGROUPSTATNAME(Group) GET_STATFNAME(STAT_##Group),
		FOREACH_ENUM_TEXTUREGROUP(ASSIGNTEXTUREGROUPSTATNAME)
		#undef ASSIGNTEXTUREGROUPSTATNAME
	};
#endif

UTexture::FOnTextureSaved UTexture::PreSaveEvent;

UTexture::UTexture(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SRGB = true;
	Filter = TF_Linear;
#if WITH_EDITORONLY_DATA
	AdjustBrightness = 1.0f;
	AdjustBrightnessCurve = 1.0f;
	AdjustVibrance = 0.0f;
	AdjustSaturation = 1.0f;
	AdjustRGBCurve = 1.0f;
	AdjustHue = 0.0f;
	AdjustMinAlpha = 0.0f;
	AdjustMaxAlpha = 1.0f;
	MipGenSettings = TMGS_FromTextureGroup;
	SourceArtType_DEPRECATED = TSAT_PNGCompressed;
	CompositeTextureMode = CTM_NormalRoughnessToAlpha;
	CompositePower = 1.0f;
#endif // #if WITH_EDITORONLY_DATA
}

void UTexture::ReleaseResource()
{
	if (Resource)
	{
		UTexture2D* Texture2D = Cast<UTexture2D>(this);
		checkf( Texture2D == NULL || Texture2D->PendingMipChangeRequestStatus.GetValue() <= TexState_ReadyFor_Requests, TEXT("PendingMipChangeRequestStates = %d"), Texture2D->PendingMipChangeRequestStatus.GetValue() );

		// Free the resource.
		ReleaseResourceAndFlush(Resource);
		delete Resource;
		Resource = NULL;
	}
}

void UTexture::UpdateResource()
{
	// Release the existing texture resource.
	ReleaseResource();

	//Dedicated servers have no texture internals
	if( FApp::CanEverRender() && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		// Create a new texture resource.
		Resource = CreateResource();
		if( Resource )
		{
			BeginInitResource(Resource);
		}
	}
}


int32 UTexture::GetCachedLODBias() const
{
	return CachedCombinedLODBias;
}

#if WITH_EDITOR
void UTexture::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SetLightingGuid();

	// Determine whether any property that requires recompression of the texture, or notification to Materials has changed.
	bool RequiresNotifyMaterials = false;

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if( PropertyThatChanged )
	{
		FString PropertyName = *PropertyThatChanged->GetName();
		if (FCString::Stricmp(*PropertyName, TEXT("CompressionSettings")) == 0)
		{
			RequiresNotifyMaterials = true;
		}

		bool bPreventSRGB = (CompressionSettings == TC_Alpha || CompressionSettings == TC_Normalmap || CompressionSettings == TC_Masks || CompressionSettings == TC_HDR);		
		if(bPreventSRGB && SRGB == true)
		{
			SRGB = false;
		}
	}

	NumCinematicMipLevels = FMath::Max<int32>( NumCinematicMipLevels, 0 );

	if( (PropertyChangedEvent.ChangeType & EPropertyChangeType::Interactive) == 0 )
	{
		// Update the texture resource. This will recache derived data if necessary
		// which may involve recompressing the texture.
		UpdateResource();
	}

	// Notify any loaded material instances if changed our compression format
	if (RequiresNotifyMaterials)
	{
		TArray<UMaterialInterface*> MaterialsThatUseThisTexture;

		// Create a material update context to safely update materials.
		{
			FMaterialUpdateContext UpdateContext;

			// Notify any material that uses this texture
			TSet<UMaterial*> BaseMaterialsThatUseThisTexture;
			for (TObjectIterator<UMaterialInterface> It; It; ++It)
			{
				UMaterialInterface* MaterialInterface = *It;
				if (DoesMaterialUseTexture(MaterialInterface,this))
				{
					MaterialsThatUseThisTexture.Add(MaterialInterface);

					// This is a bit tricky. We want to make sure all materials using this texture are
					// updated. Materials are always updated. Material instances may also have to be
					// updated and if they have static permutations their children must be updated
					// whether they use the texture or not! The safe thing to do is to add the instance's
					// base material to the update context causing all materials in the tree to update.
					BaseMaterialsThatUseThisTexture.Add(MaterialInterface->GetMaterial());
				}
			}

			// Go ahead and update any base materials that need to be.
			for (TSet<UMaterial*>::TConstIterator It(BaseMaterialsThatUseThisTexture); It; ++It)
			{
				UpdateContext.AddMaterial(*It);
				(*It)->PostEditChange();
			}
		}

		// Now that all materials and instances have updated send necessary callbacks.
		for (int32 i = 0; i < MaterialsThatUseThisTexture.Num(); ++i)
		{
			FEditorSupportDelegates::MaterialTextureSettingsChanged.Broadcast(MaterialsThatUseThisTexture[i]);
		}
	}
		
#if WITH_EDITORONLY_DATA
	// any texture that is referencing this texture as AssociatedNormalMap needs to be informed
	{
		TArray<UTexture*> TexturesThatUseThisTexture;

		for (TObjectIterator<UTexture> It; It; ++It)
		{
			UTexture* Tex = *It;

			if(Tex != this && Tex->CompositeTexture == this && Tex->CompositeTextureMode != CTM_Disabled)
			{
				TexturesThatUseThisTexture.Add(Tex);
			}
		}
		for (int32 i = 0; i < TexturesThatUseThisTexture.Num(); ++i)
		{
			TexturesThatUseThisTexture[i]->PostEditChange();
		}
	}
#endif
}
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
void UTexture::LegacySerialize(FArchive& Ar, FStripDataFlags& StripFlags)
{
	check(Ar.UE4Ver() < VER_UE4_TEXTURE_SOURCE_ART_REFACTOR);

	// Serialize the old source art directly in to the source's bulk data.
	if (!StripFlags.IsEditorDataStripped())
	{
		Source.BulkData.Serialize(Ar, this);
	}

	switch (SourceArtType_DEPRECATED)
	{
	case TSAT_Uncompressed:
		Source.Format = TSF_BGRA8;
		break;
	case TSAT_PNGCompressed:
		Source.Format = TSF_BGRA8;
		Source.bPNGCompressed = true;
		break;
	case TSAT_DDSFile:
		Source.ConvertFromLegacyDDS();
		break;
	}

	if (Ar.UE4Ver() < VER_UE4_TEXTURE_DERIVED_DATA2)
	{
		// PostEditChange used to set CompressionNone for TC_EditorIcon. This is
		// now handled by the texture derived data logic automatically. Repair
		// this on load so that the correct textures are cached and built.
		if (CompressionSettings == TC_EditorIcon)
		{
			CompressionNone = false;
		}
	}

	// to be backwards compatible, the flag was removed
	if(CompressionNoMipmaps_DEPRECATED)
	{
		MipGenSettings = TMGS_NoMipmaps;
	}

	if ( CompressionSettings >= TC_MAX )
	{
		CompressionSettings = TC_Default;
	}
}
#endif

void UTexture::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	FStripDataFlags StripFlags(Ar);

	/** Legacy serialization. */
#if WITH_EDITORONLY_DATA
	if (Ar.UE4Ver() < VER_UE4_TEXTURE_SOURCE_ART_REFACTOR)
	{
		UTexture::LegacySerialize(Ar, StripFlags);
		return;
	}

	if (Ar.UE4Ver() < VER_UE4_TEXTURE_FORMAT_RGBA_SWIZZLE)
	{
		switch (Source.Format)
		{
		case TSF_RGBA8:	Source.Format = TSF_BGRA8; break;
		case TSF_RGBE8:	Source.Format = TSF_BGRE8; break;
		}
	}

	if (!StripFlags.IsEditorDataStripped())
	{
		Source.BulkData.Serialize(Ar, this);
	}
#endif // #if WITH_EDITORONLY_DATA
}

void UTexture::PostLoad()
{
	Super::PostLoad();

	if( !IsTemplate() )
	{
		// Update cached LOD bias.
		CachedCombinedLODBias = GSystemSettings.TextureLODSettings.CalculateLODBias( this );

		// The texture will be cached by the cubemap it is contained within on consoles.
		UTextureCube* CubeMap = Cast<UTextureCube>(GetOuter());
		if (CubeMap == NULL)
		{
			// Recreate the texture's resource.
			UpdateResource();
		}
	}
}

void UTexture::BeginDestroy()
{
	Super::BeginDestroy();
	if( !UpdateStreamingStatus() && Resource )
	{
		// Send the rendering thread a release message for the texture's resource.
		BeginReleaseResource(Resource);
		Resource->ReleaseFence.BeginFence();
		// Keep track that we alrady kicked off the async release.
		bAsyncResourceReleaseHasBeenStarted = true;
	}
}

bool UTexture::IsReadyForFinishDestroy()
{
	bool bReadyForFinishDestroy = false;
	// Check whether super class is ready and whether we have any pending streaming requests in flight.
	if( Super::IsReadyForFinishDestroy() && !UpdateStreamingStatus() )
	{
		// Kick off async resource release if we haven't already.
		if( !bAsyncResourceReleaseHasBeenStarted && Resource )
		{
			// Send the rendering thread a release message for the texture's resource.
			BeginReleaseResource(Resource);
			Resource->ReleaseFence.BeginFence();
			// Keep track that we alrady kicked off the async release.
			bAsyncResourceReleaseHasBeenStarted = true;
		}
		// Only allow FinishDestroy to be called once the texture resource has finished its rendering thread cleanup.
		else if( !Resource || Resource->ReleaseFence.IsFenceComplete() )
		{
			bReadyForFinishDestroy = true;
		}
	}
	return bReadyForFinishDestroy;
}

void UTexture::FinishDestroy()
{
	Super::FinishDestroy();

	if(Resource)
	{
		check(Resource->ReleaseFence.IsFenceComplete());

		// Free the resource.
		delete Resource;
		Resource = NULL;
	}
}

void UTexture::PreSave()
{
	PreSaveEvent.Broadcast(this);

	Super::PreSave();

#if WITH_EDITORONLY_DATA
	if (DeferCompression)
	{
		GWarn->StatusUpdate( 0, 0, FText::Format( NSLOCTEXT("UnrealEd", "SavingPackage_CompressingTexture", "Compressing texture:  {0}"), FText::FromString(GetName()) ) );
		DeferCompression = false;
		UpdateResource();
	}

	GWarn->StatusUpdate( 0, 0, FText::Format( NSLOCTEXT("UnrealEd", "SavingPackage_CompressingSourceArt", "Compressing source art for texture:  {0}"), FText::FromString(GetName()) ) );
	Source.Compress();

#endif // #if WITH_EDITORONLY_DATA
}


float UTexture::GetAverageBrightness(bool bIgnoreTrueBlack, bool bUseGrayscale)
{
	// Indicate the action was not performed...
	return -1.0f;
}

/** Helper functions for text output of texture properties... */
#ifndef CASE_ENUM_TO_TEXT
#define CASE_ENUM_TO_TEXT(txt) case txt: return TEXT(#txt);
#endif

#ifndef TEXT_TO_ENUM
#define TEXT_TO_ENUM(eVal, txt)		if (FCString::Stricmp(TEXT(#eVal), txt) == 0)	return eVal;
#endif

const TCHAR* UTexture::GetTextureGroupString(TextureGroup InGroup)
{
	switch (InGroup)
	{
		FOREACH_ENUM_TEXTUREGROUP(CASE_ENUM_TO_TEXT)
	}

	return TEXT("TEXTUREGROUP_World");
}

const TCHAR* UTexture::GetMipGenSettingsString(TextureMipGenSettings InEnum)
{
	switch(InEnum)
	{
		default:
		FOREACH_ENUM_TEXTUREMIPGENSETTINGS(CASE_ENUM_TO_TEXT)
	}
}

TextureMipGenSettings UTexture::GetMipGenSettingsFromString(const TCHAR* InStr, bool bTextureGroup)
{
#define TEXT_TO_MIPGENSETTINGS(m) TEXT_TO_ENUM(m, InStr);
	FOREACH_ENUM_TEXTUREMIPGENSETTINGS(TEXT_TO_MIPGENSETTINGS)
#undef TEXT_TO_MIPGENSETTINGS

	// default for TextureGroup and Texture is different
	return bTextureGroup ? TMGS_SimpleAverage : TMGS_FromTextureGroup;
}

UEnum* UTexture::GetPixelFormatEnum()
{
	// Lookup the pixel format enum so that the pixel format can be serialized by name.
	static FName PixelFormatUnknownName(TEXT("PF_Unknown"));
	static UEnum* PixelFormatEnum = NULL;
	if (PixelFormatEnum == NULL)
	{
		UEnum::LookupEnumName(PixelFormatUnknownName, &PixelFormatEnum);
		check(PixelFormatEnum);
	}
	return PixelFormatEnum;
}

bool UTexture::ForceUpdateTextureStreaming()
{
	if ( GStreamingManager )
	{
		// Make sure textures can be streamed out so that we can unload current mips.
		static auto CVarOnlyStreamInTextures = IConsoleManager::Get().FindConsoleVariable(TEXT("r.OnlyStreamInTextures"));
		const bool bOldOnlyStreamInTextures = CVarOnlyStreamInTextures->GetInt() != 0;
		CVarOnlyStreamInTextures->Set(false);

		for( TObjectIterator<UTexture2D> It; It; ++It )
		{
			UTexture* Texture = *It;

			// Update cached LOD bias.
			Texture->CachedCombinedLODBias = GSystemSettings.TextureLODSettings.CalculateLODBias( Texture );
		}

		// Make sure we iterate over all textures by setting it to high value.
		GStreamingManager->SetNumIterationsForNextFrame( 100 );
		// Update resource streaming with updated texture LOD bias/ max texture mip count.
		GStreamingManager->UpdateResourceStreaming( 0 );
		// Block till requests are finished.
		GStreamingManager->BlockTillAllRequestsFinished();

		// Restore streaming out of textures.
		CVarOnlyStreamInTextures->Set(bOldOnlyStreamInTextures);
	}

	return true;
}

void FTextureLODSettings::Initialize( const FString& IniFilename, const TCHAR* IniSection )
{
	// look up the file object
	FConfigFile* ConfigFile = GConfig->FindConfigFile(IniFilename);
	if (ConfigFile)
	{
		// pass to the other initialize function
		Initialize(*ConfigFile, IniSection);
	}
}

void FTextureLODSettings::Initialize(const FConfigFile& IniFile, const TCHAR* IniSection)
{
	// Read individual entries from a config file.
#define GROUPREADENTRY(g) ReadEntry( g, TEXT(#g), IniFile, IniSection );
	FOREACH_ENUM_TEXTUREGROUP(GROUPREADENTRY)
#undef GROUPREADENTRY
}

/**
 * Returns the texture group names, sorted like enum.
 *
 * @return array of texture group names
 */
TArray<FString> FTextureLODSettings::GetTextureGroupNames()
{
	TArray<FString> TextureGroupNames;

#define GROUPNAMES(g) new(TextureGroupNames) FString(TEXT(#g));
	FOREACH_ENUM_TEXTUREGROUP(GROUPNAMES)
#undef GROUPNAMES

	return TextureGroupNames;
}

void FTextureLODSettings::ReadEntry( int32 GroupId, const TCHAR* GroupName, const FConfigFile& IniFile, const TCHAR* IniSection )
{
	// Look for string in filename/ section.
	FString Entry;
	if (IniFile.GetString(IniSection, GroupName, Entry))
	{
		// Trim whitespace at the beginning.
		Entry = Entry.Trim();
		// Remove brackets.
		Entry = Entry.Replace( TEXT("("), TEXT("") );
		Entry = Entry.Replace( TEXT(")"), TEXT("") );
		
		// Parse minimum LOD mip count.
		int32	MinLODSize = 0;
		if( FParse::Value( *Entry, TEXT("MinLODSize="), MinLODSize ) )
		{
			TextureLODGroups[GroupId].MinLODMipCount = FMath::CeilLogTwo( MinLODSize );
		}

		// Parse maximum LOD mip count.
		int32 MaxLODSize = 0;
		if( FParse::Value( *Entry, TEXT("MaxLODSize="), MaxLODSize ) )
		{
			TextureLODGroups[GroupId].MaxLODMipCount = FMath::CeilLogTwo( MaxLODSize );
		}

		// Parse LOD bias.
		int32 LODBias = 0;
		if( FParse::Value( *Entry, TEXT("LODBias="), LODBias ) )
		{
			TextureLODGroups[GroupId].LODBias = LODBias;
		}

		// Parse min/map/mip filter names.
		FName MinMagFilter = NAME_Aniso;
		FParse::Value( *Entry, TEXT("MinMagFilter="), MinMagFilter );
		FName MipFilter = NAME_Point;
		FParse::Value( *Entry, TEXT("MipFilter="), MipFilter );

		{
			FString MipGenSettings;
			FParse::Value( *Entry, TEXT("MipGenSettings="), MipGenSettings );
			TextureLODGroups[GroupId].MipGenSettings = UTexture::GetMipGenSettingsFromString(*MipGenSettings, true);
		}

		// Convert into single filter enum. The code is layed out such that invalid input will 
		// map to the default state of highest quality filtering.

		// Linear filtering
		if( MinMagFilter == NAME_Linear )
		{
			if( MipFilter == NAME_Point )
			{
				TextureLODGroups[GroupId].Filter = SF_Bilinear;
			}
			else
			{
				TextureLODGroups[GroupId].Filter = SF_Trilinear;
			}
		}
		// Point. Don't even care about mip filter.
		else if( MinMagFilter == NAME_Point )
		{
			TextureLODGroups[GroupId].Filter = SF_Point;
		}
		// Aniso or unknown.
		else
		{
			if( MipFilter == NAME_Point )
			{
				TextureLODGroups[GroupId].Filter = SF_AnisotropicPoint;
			}
			else
			{
				TextureLODGroups[GroupId].Filter = SF_AnisotropicLinear;
			}
		}

		// Parse NumStreamedMips
		int32 NumStreamedMips = -1;
		if( FParse::Value( *Entry, TEXT("NumStreamedMips="), NumStreamedMips ) )
		{
			TextureLODGroups[GroupId].NumStreamedMips = NumStreamedMips;
		}
	}
}

int32 FTextureLODSettings::CalculateLODBias( UTexture* Texture, bool bIncTextureBias ) const
{	
	// Find LOD group.
	check( Texture );
	const FTextureLODGroup& LODGroup = TextureLODGroups[Texture->LODGroup];

	// Calculate maximum number of miplevels.
	int32 TextureMaxLOD	= FMath::CeilLogTwo( FMath::Trunc( FMath::Max( Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight() ) ) );

	// Calculate LOD bias.
	int32 UsedLODBias		= Texture->NumCinematicMipLevels + LODGroup.LODBias + (bIncTextureBias ? Texture->LODBias : 0);
	int32 MinLOD			= LODGroup.MinLODMipCount;
	int32 MaxLOD			= LODGroup.MaxLODMipCount;
	int32 WantedMaxLOD	= FMath::Clamp( TextureMaxLOD - UsedLODBias, MinLOD, MaxLOD );
	WantedMaxLOD		= FMath::Clamp( WantedMaxLOD, 0, TextureMaxLOD );
	UsedLODBias			= TextureMaxLOD - WantedMaxLOD;

	return UsedLODBias;
}

/**
	* Calculates and returns the LOD bias based on the information provided.
	*
	* @param	Width						Width of the texture
	* @param	Height						Height of the texture
	* @param	LODGroup					Which LOD group the texture belongs to
	* @param	LODBias						LOD bias to include in the calculation
	* @param	NumCinematicMipLevels		The texture cinematic mip levels to include in the calculation
	* @return	LOD bias
	*/
int32 FTextureLODSettings::CalculateLODBias( int32 Width, int32 Height, int32 LODGroup, int32 LODBias, int32 NumCinematicMipLevels, TextureMipGenSettings InMipGenSetting ) const
{	
	// Find LOD group.
	const FTextureLODGroup& LODGroupInfo = TextureLODGroups[LODGroup];

	// Test to see if we have no mip generation as in which case the LOD bias will be ignored

	const TextureMipGenSettings FinalMipGenSetting = (InMipGenSetting == TMGS_FromTextureGroup) ? LODGroupInfo.MipGenSettings : InMipGenSetting;

	if ( FinalMipGenSetting == TMGS_NoMipmaps )
	{
		return 0;
	}

	// Calculate maximum number of miplevels.
	int32 TextureMaxLOD	= FMath::CeilLogTwo( FMath::Trunc( FMath::Max( Width, Height ) ) );

	// Calculate LOD bias.
	int32 UsedLODBias		= LODGroupInfo.LODBias + LODBias + NumCinematicMipLevels;
	int32 MinLOD			= LODGroupInfo.MinLODMipCount;
	int32 MaxLOD			= LODGroupInfo.MaxLODMipCount;
	int32 WantedMaxLOD	= FMath::Clamp( TextureMaxLOD - UsedLODBias, MinLOD, MaxLOD );
	WantedMaxLOD		= FMath::Clamp( WantedMaxLOD, 0, TextureMaxLOD );
	UsedLODBias			= TextureMaxLOD - WantedMaxLOD;

	return UsedLODBias;
}

/** 
* Useful for stats in the editor.
*/
void FTextureLODSettings::ComputeInGameMaxResolution(int32 LODBias, UTexture &Texture, uint32 &OutSizeX, uint32 &OutSizeY) const
{
	uint32 ImportedSizeX = FMath::Trunc(Texture.GetSurfaceWidth());
	uint32 ImportedSizeY = FMath::Trunc(Texture.GetSurfaceHeight());
	
	const FTextureLODGroup& LODGroup = GetTextureLODGroup((TextureGroup)Texture.LODGroup);

	uint32 SourceLOD = FMath::Max(FMath::CeilLogTwo(ImportedSizeX), FMath::CeilLogTwo(ImportedSizeY));
	uint32 MinLOD = FMath::Max(uint32(UTexture2D::GetMinTextureResidentMipCount() - 1), (uint32)LODGroup.MinLODMipCount);
	uint32 MaxLOD = FMath::Min(uint32(GMaxTextureMipCount - 1), (uint32)LODGroup.MaxLODMipCount);
	uint32 GameLOD = FMath::Min(SourceLOD, FMath::Clamp(SourceLOD - LODBias, MinLOD, MaxLOD));

	uint32 DeltaLOD = SourceLOD - GameLOD;

	OutSizeX = ImportedSizeX >> DeltaLOD;
	OutSizeY = ImportedSizeY >> DeltaLOD;
}

/**
* TextureLODGroups access with bounds check
*
* @param   GroupIndex      usually from Texture.LODGroup
* @return                  A handle to the indexed LOD group. 
*/
const FTextureLODSettings::FTextureLODGroup& FTextureLODSettings::GetTextureLODGroup(TextureGroup GroupIndex) const
{
	if((uint32)GroupIndex >= TEXTUREGROUP_MAX)
	{
		// to prevent crash
		GroupIndex = (TextureGroup)0;
	}
//	check((uint32)GroupIndex < TEXTUREGROUP_MAX);
	return TextureLODGroups[GroupIndex];
}

#if WITH_EDITORONLY_DATA
void FTextureLODSettings::GetMipGenSettings( const UTexture& Texture, TextureMipGenSettings& OutMipGenSettings, float& OutSharpen, uint32& OutKernelSize, bool& bOutDownsampleWithAverage, bool& bOutSharpenWithoutColorShift, bool &bOutBorderColorBlack ) const
{
	TextureMipGenSettings Setting = (TextureMipGenSettings)Texture.MipGenSettings;

	bOutBorderColorBlack = false;

	// avoiding the color shift assumes we deal with colors which is not true for normalmaps
	// or we blur where it's good to blur the color as well
	bOutSharpenWithoutColorShift = !Texture.IsNormalMap();

	bOutDownsampleWithAverage = true;

	// inherit from texture group
	if(Setting == TMGS_FromTextureGroup)
	{
		const FTextureLODGroup& LODGroup = TextureLODGroups[Texture.LODGroup];

		Setting = LODGroup.MipGenSettings;
	}
	OutMipGenSettings = Setting;

	// ------------

	// default:
	OutSharpen = 0;
	OutKernelSize = 2;

	if(Setting >= TMGS_Sharpen0 && Setting <= TMGS_Sharpen10)
	{
		// 0 .. 2.0f
		OutSharpen = ((int32)Setting - (int32)TMGS_Sharpen0) * 0.2f;
		OutKernelSize = 8;
	}
	else if(Setting >= TMGS_Blur1 && Setting <= TMGS_Blur5)
	{
		int32 BlurFactor = ((int32)Setting + 1 - (int32)TMGS_Blur1);
		OutSharpen = -BlurFactor * 2;
		OutKernelSize = 2 + 2 * BlurFactor;
		bOutDownsampleWithAverage = false;
		bOutSharpenWithoutColorShift = false;
		bOutBorderColorBlack = true;
	}
}
#endif // #if WITH_EDITORONLY_DATA

/**
 * Will return the LODBias for a passed in LODGroup
 *
 * @param	InLODGroup		The LOD Group ID 
 * @return	LODBias
 */
int32 FTextureLODSettings::GetTextureLODGroupLODBias( int32 InLODGroup ) const
{
	int32 Retval = 0;

	const FTextureLODGroup& LODGroup = TextureLODGroups[InLODGroup]; 

	Retval = LODGroup.LODBias;

	return Retval;
}

/**
 * Returns the LODGroup setting for number of streaming mip-levels.
 * -1 means that all mip-levels are allowed to stream.
 *
 * @param	InLODGroup		The LOD Group ID 
 * @return	Number of streaming mip-levels for textures in the specified LODGroup
 */
int32 FTextureLODSettings::GetNumStreamedMips( int32 InLODGroup ) const
{
	int32 Retval = 0;

	const FTextureLODGroup& LODGroup = TextureLODGroups[InLODGroup]; 

	Retval = LODGroup.NumStreamedMips;

	return Retval;
}

/**
 * Returns the filter state that should be used for the passed in texture, taking
 * into account other system settings.
 *
 * @param	Texture		Texture to retrieve filter state for
 * @return	Filter sampler state for passed in texture
 */
ESamplerFilter FTextureLODSettings::GetSamplerFilter( const UTexture* Texture ) const
{
	// Default to point filtering.
	ESamplerFilter Filter = SF_Point;

	// Only diverge from default for valid textures that don't use point filtering.
	if( !Texture || Texture->Filter != TF_Nearest )
	{
		// Use LOD group value to find proper filter setting.
		Filter = TextureLODGroups[Texture->LODGroup].Filter;
	}

	return Filter;
}

/*------------------------------------------------------------------------------
	Texture source data.
------------------------------------------------------------------------------*/

FTextureSource::FTextureSource()
	: LockedMipData(NULL)
	, LockedMips(0)
#if WITH_EDITORONLY_DATA
	, SizeX(0)
	, SizeY(0)
	, NumSlices(0)
	, NumMips(0)
	, bPNGCompressed(false)
	, bGuidIsHash(false)
	, Format(TSF_Invalid)
#endif // WITH_EDITORONLY_DATA
{
}

#if WITH_EDITORONLY_DATA

void FTextureSource::Init(
		int32 NewSizeX,
		int32 NewSizeY,
		int32 NewNumSlices,
		int32 NewNumMips,
		ETextureSourceFormat NewFormat,
		const uint8* NewData
		)
{
	RemoveSourceData();
	SizeX = NewSizeX;
	SizeY = NewSizeY;
	NumSlices = NewNumSlices;
	NumMips = NewNumMips;
	Format = NewFormat;

	int32 TotalBytes = 0;
	int32 BytesPerPixel = GetBytesPerPixel();
	int32 MipSizeX = SizeX;
	int32 MipSizeY = SizeY;

	while (NewNumMips-- > 0)
	{
		TotalBytes += MipSizeX * MipSizeY * NumSlices * BytesPerPixel;
		MipSizeX = FMath::Max(MipSizeX >> 1, 1);
		MipSizeY = FMath::Max(MipSizeY >> 1, 1);
	}

	BulkData.Lock(LOCK_READ_WRITE);
	uint8* DestData = (uint8*)BulkData.Realloc(TotalBytes);
	if (NewData)
	{
		FMemory::Memcpy(DestData, NewData, TotalBytes);
	}
	BulkData.Unlock();
}

void FTextureSource::Init2DWithMipChain(
	int32 NewSizeX,
	int32 NewSizeY,
	ETextureSourceFormat NewFormat
	)
{
	int32 NumMips = FMath::Max(FMath::CeilLogTwo(NewSizeX),FMath::CeilLogTwo(NewSizeY)) + 1;
	Init(NewSizeX, NewSizeY, 1, NumMips, NewFormat);
}

void FTextureSource::InitCubeWithMipChain(
	int32 NewSizeX,
	int32 NewSizeY,
	ETextureSourceFormat NewFormat
	)
{
	int32 NumMips = FMath::Max(FMath::CeilLogTwo(NewSizeX),FMath::CeilLogTwo(NewSizeY)) + 1;
	Init(NewSizeX, NewSizeY, 6, NumMips, NewFormat);
}

void FTextureSource::Compress()
{
	if (CanPNGCompress())
	{
		uint8* BulkDataPtr = (uint8*)BulkData.Lock(LOCK_READ_WRITE);
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>( FName("ImageWrapper") );
		IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper( EImageFormat::PNG );
		// TODO: TSF_BGRA8 is stored as RGBA, so the R and B channels are swapped in the internal png. Should we fix this?
		ERGBFormat::Type RawFormat = (Format == TSF_G8) ? ERGBFormat::Gray : ERGBFormat::RGBA;
		if ( ImageWrapper.IsValid() && ImageWrapper->SetRaw( BulkDataPtr, BulkData.GetBulkDataSize(), SizeX, SizeY, RawFormat, Format == TSF_RGBA16 ? 16 : 8 ) )
		{
			const TArray<uint8>& CompressedData = ImageWrapper->GetCompressed();
			if ( CompressedData.Num() > 0 )
			{
				BulkDataPtr = (uint8*)BulkData.Realloc(CompressedData.Num());
				FMemory::Memcpy(BulkDataPtr, CompressedData.GetTypedData(), CompressedData.Num());
				BulkData.Unlock();
				bPNGCompressed = true;

				BulkData.StoreCompressedOnDisk(ECompressionFlags::COMPRESS_None);
			}
		}
	}
	else
	{
		//Can't PNG compress so just zlib compress the lot when its serialized out to disk. 
		BulkData.StoreCompressedOnDisk(ECompressionFlags::COMPRESS_ZLIB);
	}
}

uint8* FTextureSource::LockMip(int32 MipIndex)
{
	uint8* MipData = NULL;
	if (MipIndex < NumMips)
	{
		if (LockedMipData == NULL)
		{
			LockedMipData = (uint8*)BulkData.Lock(LOCK_READ_WRITE);
			if (bPNGCompressed)
			{
				bool bCanPngCompressFormat = (Format == TSF_G8 || Format == TSF_RGBA8 || Format == TSF_BGRA8 || Format == TSF_RGBA16);
				check(NumSlices == 1 && bCanPngCompressFormat);
				if (MipIndex != 0)
				{
					return NULL;
				}

				IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>( FName("ImageWrapper") );
				IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper( EImageFormat::PNG );
				if ( ImageWrapper.IsValid() && ImageWrapper->SetCompressed( LockedMipData, BulkData.GetBulkDataSize() ) )
				{
					check( ImageWrapper->GetWidth() == SizeX );
					check( ImageWrapper->GetHeight() == SizeY );
					const TArray<uint8>* RawData = NULL;
					// TODO: TSF_BGRA8 is stored as RGBA, so the R and B channels are swapped in the internal png. Should we fix this?
					ERGBFormat::Type RawFormat = (Format == TSF_G8) ? ERGBFormat::Gray : ERGBFormat::RGBA;
					if (ImageWrapper->GetRaw( RawFormat, Format == TSF_RGBA16 ? 16 : 8, RawData ))
					{
						if (RawData->Num() > 0)
						{
							LockedMipData = (uint8*)FMemory::Malloc(RawData->Num());
							FMemory::Memcpy(LockedMipData, RawData->GetTypedData(), RawData->Num());
						}
					}
					if (RawData == NULL || RawData->Num() == 0)
					{
						UE_LOG(LogTexture, Warning, TEXT("PNG decompression of source art failed"));
					}
				}
				else
				{
					UE_LOG(LogTexture, Log, TEXT("Only pngs are supported"));
				}
			}
		}

		MipData = LockedMipData + CalcMipOffset(MipIndex);
		LockedMips |= (1 << MipIndex);
	}
	return MipData;
}

void FTextureSource::UnlockMip(int32 MipIndex)
{
	check(MipIndex < MAX_TEXTURE_MIP_COUNT);

	uint32 LockBit = 1 << MipIndex;
	if (LockedMips & LockBit)
	{
		LockedMips &= (~LockBit);
		if (LockedMips == 0)
		{
			if (bPNGCompressed)
			{
				check(MipIndex == 0);
				int32 MipSize = CalcMipSize(0);
				uint8* UncompressedData = (uint8*)BulkData.Realloc(MipSize);
				FMemory::Memcpy(UncompressedData, LockedMipData, MipSize);
				FMemory::Free(LockedMipData);
				bPNGCompressed = false;
			}
			LockedMipData = NULL;
			BulkData.Unlock();
			ForceGenerateGuid();
		}
	}
}

bool FTextureSource::GetMipData(TArray<uint8>& OutMipData, int32 MipIndex)
{
	bool bSuccess = false;
	if (MipIndex < NumMips && BulkData.GetBulkDataSize() > 0)
	{
		void* RawSourceData = BulkData.Lock(LOCK_READ_ONLY);
		if (bPNGCompressed)
		{
			bool bCanPngCompressFormat = (Format == TSF_G8 || Format == TSF_RGBA8 || Format == TSF_BGRA8 || Format == TSF_RGBA16);
			if (MipIndex == 0 && NumSlices == 1 && bCanPngCompressFormat)
			{
				IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>( FName("ImageWrapper") );
				IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper( EImageFormat::PNG );
				if ( ImageWrapper.IsValid() && ImageWrapper->SetCompressed( RawSourceData, BulkData.GetBulkDataSize() ) )
				{
					if (ImageWrapper->GetWidth() == SizeX
						&& ImageWrapper->GetHeight() == SizeY)
					{
						const TArray<uint8>* RawData = NULL;
						// TODO: TSF_BGRA8 is stored as RGBA, so the R and B channels are swapped in the internal png. Should we fix this?
						ERGBFormat::Type RawFormat = (Format == TSF_G8) ? ERGBFormat::Gray : ERGBFormat::RGBA;
						if (ImageWrapper->GetRaw( RawFormat, Format == TSF_RGBA16 ? 16 : 8, RawData ))
						{
							OutMipData = *RawData;
							bSuccess = true;
						}
						else
						{
							UE_LOG(LogTexture, Warning, TEXT("PNG decompression of source art failed"));
							OutMipData.Empty();
						}
					}
					else
					{
						UE_LOG(LogTexture, Warning,
							TEXT("PNG decompression of source art failed. ")
							TEXT("Source image should be %dx%d but is %dx%d"),
							SizeX, SizeY,
							ImageWrapper->GetWidth(), ImageWrapper->GetHeight()
							);
					}
				}
				else
				{
					UE_LOG(LogTexture, Log, TEXT("Only pngs are supported"));
				}
			}
		}
		else
		{
			int32 MipOffset = CalcMipOffset(MipIndex);
			int32 MipSize = CalcMipSize(MipIndex);
			if (BulkData.GetBulkDataSize() >= MipOffset + MipSize)
			{
				OutMipData.Empty(MipSize);
				OutMipData.AddUninitialized(MipSize);
				FMemory::Memcpy(
					OutMipData.GetTypedData(),
					(uint8*)RawSourceData + MipOffset,
					MipSize
					);
			}
			bSuccess = true;
		}
		BulkData.Unlock();
	}
	return bSuccess;
}

int32 FTextureSource::CalcMipSize(int32 MipIndex) const
{
	int32 MipSizeX = FMath::Max(SizeX >> MipIndex, 1);
	int32 MipSizeY = FMath::Max(SizeY >> MipIndex, 1);
	int32 BytesPerPixel = GetBytesPerPixel();
	return MipSizeX * MipSizeY * NumSlices * BytesPerPixel;
}

int32 FTextureSource::GetBytesPerPixel() const
{
	int32 BytesPerPixel = 0;
	switch (Format)
	{
	case TSF_G8:		BytesPerPixel = 1; break;
	case TSF_BGRA8:		BytesPerPixel = 4; break;
	case TSF_BGRE8:		BytesPerPixel = 4; break;
	case TSF_RGBA16:	BytesPerPixel = 8; break;
	case TSF_RGBA16F:	BytesPerPixel = 8; break;
	default:			BytesPerPixel = 0; break;
	}
	return BytesPerPixel;
}

bool FTextureSource::IsPowerOfTwo() const
{
	return FMath::IsPowerOfTwo(SizeX) && FMath::IsPowerOfTwo(SizeY);
}

bool FTextureSource::IsValid() const
{
	return SizeX > 0 && SizeY > 0 && NumSlices > 0 && NumMips > 0 &&
		Format != TSF_Invalid && BulkData.GetBulkDataSize() > 0;
}

FString FTextureSource::GetIdString() const
{
	FString GuidString = Id.ToString();
	if (bGuidIsHash)
	{
		GuidString += TEXT("X");
	}
	return GuidString;
}

bool FTextureSource::CanPNGCompress() const
{
	bool bCanPngCompressFormat = (Format == TSF_G8 || Format == TSF_RGBA8 || Format == TSF_BGRA8 || Format == TSF_RGBA16);

	if (!bPNGCompressed &&
		NumMips == 1 &&
		NumSlices == 1 &&
		SizeX > 4 &&
		SizeY > 4 &&
		BulkData.GetBulkDataSize() > 0 &&
		bCanPngCompressFormat)
	{
		return true;
	}
	return false;
}

void FTextureSource::ForceGenerateGuid()
{
	Id = FGuid::NewGuid();
	bGuidIsHash = false;
}

void FTextureSource::RemoveSourceData()
{
	SizeX = 0;
	SizeY = 0;
	NumSlices = 0;
	NumMips = 0;
	Format = TSF_Invalid;
	bPNGCompressed = false;
	LockedMipData = NULL;
	LockedMips = 0;
	if (BulkData.IsLocked())
	{
		BulkData.Unlock();
	}
	BulkData.RemoveBulkData();
	ForceGenerateGuid();
}

int32 FTextureSource::CalcMipOffset(int32 MipIndex) const
{
	int32 MipOffset = 0;
	int32 BytesPerPixel = GetBytesPerPixel();
	int32 MipSizeX = SizeX;
	int32 MipSizeY = SizeY;

	while (MipIndex-- > 0)
	{
		MipOffset += MipSizeX * MipSizeY * BytesPerPixel * NumSlices;
		MipSizeX = FMath::Max(MipSizeX >> 1, 1);
		MipSizeY = FMath::Max(MipSizeY >> 1, 1);
	}

	return MipOffset;
}

void FTextureSource::ConvertFromLegacyDDS()
{
	TArray<uint8> RawDDS;
	RawDDS.Empty(BulkData.GetBulkDataSize());
	RawDDS.AddUninitialized(BulkData.GetBulkDataSize());
	{
		void* LockedDDSData = NULL;
		BulkData.GetCopy(&LockedDDSData);
		FMemory::Memcpy(RawDDS.GetTypedData(), LockedDDSData, RawDDS.Num());
		FMemory::Free(LockedDDSData);
		BulkData.RemoveBulkData();
	}

	FDDSLoadHelper DDS(RawDDS.GetTypedData(), RawDDS.Num());
	if ((DDS.IsValid2DTexture() || DDS.IsValidCubemapTexture()) && DDS.ComputeSourceFormat() != TSF_Invalid)
	{
		Init(
			DDS.DDSHeader->dwWidth, 
			DDS.DDSHeader->dwHeight,
			/*NumSlices=*/ DDS.IsValidCubemapTexture() ? 6 : 1,
			DDS.ComputeMipMapCount(),
			DDS.ComputeSourceFormat()
			);

		uint8* DestMipData[MAX_TEXTURE_MIP_COUNT] = {0};
		int32 MipSize[MAX_TEXTURE_MIP_COUNT] = {0};
		for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
		{
			DestMipData[MipIndex] = LockMip(MipIndex);
			MipSize[MipIndex] = CalcMipSize(MipIndex) / NumSlices;
		}

		for (int32 SliceIndex = 0; SliceIndex < NumSlices; ++SliceIndex)
		{
			const uint8* SrcMipData = DDS.GetDDSDataPointer((ECubeFace)SliceIndex);
			for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
			{
				FMemory::Memcpy(DestMipData[MipIndex] + MipSize[MipIndex] * SliceIndex, SrcMipData, MipSize[MipIndex]);
				SrcMipData += MipSize[MipIndex];
			}
		}

		for (int32 MipIndex = 0; MipIndex < NumMips; ++MipIndex)
		{
			UnlockMip(MipIndex);
		}
	}
}

void FTextureSource::UseHashAsGuid()
{
	uint32 Hash[5];

	if (BulkData.GetBulkDataSize() > 0)
	{
		bGuidIsHash = true;
		void* Buffer = BulkData.Lock(LOCK_READ_ONLY);
		FSHA1::HashBuffer(Buffer, BulkData.GetBulkDataSize(), (uint8*)Hash);
		BulkData.Unlock();
		Id = FGuid(Hash[0] ^ Hash[4], Hash[1], Hash[2], Hash[3]);
	}
}

#endif // #if WITH_EDITORONLY_DATA
