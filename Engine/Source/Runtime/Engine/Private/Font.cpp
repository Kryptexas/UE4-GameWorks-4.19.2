// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Font.cpp: Unreal font code.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"


UFontImportOptions::UFontImportOptions(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UFont::UFont(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ScalingFactor = 1.0f;
}

void UFont::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	Ar << CharRemap;
}

void UFont::PostLoad()
{
	Super::PostLoad();


	// Cache the character count and the maximum character height for this font
	CacheCharacterCountAndMaxCharHeight();


	for( int32 TextureIndex = 0 ; TextureIndex < Textures.Num() ; ++TextureIndex )
	{
		UTexture2D* Texture = Textures[TextureIndex];
		if( Texture )
		{	
			Texture->SetFlags(RF_Public);
			Texture->LODGroup = TEXTUREGROUP_UI;	
		}
	}
}



void UFont::CacheCharacterCountAndMaxCharHeight()
{
	// Cache the number of characters in the font.  Obviously this is pretty simple, but note that it will be
	// computed differently for MultiFonts.  We need to cache it so that we have it available in inline functions
	NumCharacters = Characters.Num();

	// Cache maximum character height
	MaxCharHeight.Reset();
	int32 MaxCharHeightForThisFont = 1;
	for( int32 CurCharNum = 0; CurCharNum < NumCharacters; ++CurCharNum )
	{
		MaxCharHeightForThisFont = FMath::Max( MaxCharHeightForThisFont, Characters[ CurCharNum ].VSize );
	}

	// Add to the array
	MaxCharHeight.Add( MaxCharHeightForThisFont );
}


bool UFont::IsLocalizedResource()
{
	//@todo: maybe this should be a flag?
	return true;
}

float UFont::GetMaxCharHeight() const
{
	// @todo: Provide a version of this function that supports multi-fonts properly.  It should take a
	//    HeightTest parameter and report the appropriate multi-font MaxCharHeight value back.
	int32 MaxCharHeightForAllMultiFonts = 1;
	for( int32 CurMultiFontIndex = 0; CurMultiFontIndex < MaxCharHeight.Num(); ++CurMultiFontIndex )
	{
		MaxCharHeightForAllMultiFonts = FMath::Max( MaxCharHeightForAllMultiFonts, MaxCharHeight[ CurMultiFontIndex ] );
	}
	return MaxCharHeightForAllMultiFonts;
}



void UFont::GetStringHeightAndWidth( const FString& InString, int32& Height, int32& Width ) const
{
	Height = GetStringHeightSize( *InString );
	Width = GetStringSize( *InString );
}



SIZE_T UFont::GetResourceSize(EResourceSizeMode::Type Mode)
{
	if (Mode == EResourceSizeMode::Exclusive)
	{
		return 0;
	}
	else
	{
		int32 ResourceSize = 0;
		for( int32 TextureIndex = 0 ; TextureIndex < Textures.Num() ; ++TextureIndex )
		{
			if ( Textures[TextureIndex] )
			{
				ResourceSize += Textures[TextureIndex]->GetResourceSize(Mode);
			}
		}
		return ResourceSize;
	}
}
