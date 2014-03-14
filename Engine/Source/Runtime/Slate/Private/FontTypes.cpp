// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "Slate.h"
#include "FontTypes.h"

FSlateFontAtlas::FSlateFontAtlas( uint32 InWidth, uint32 InHeight )
	: FSlateTextureAtlas( InWidth, InHeight, sizeof(uint8), 0 )
{
}

FSlateFontAtlas::~FSlateFontAtlas()
{

}	

/** 
 * Adds a character to the texture.
 *
 * @param CharInfo	Information about the size of the character
 */
const FAtlasedTextureSlot* FSlateFontAtlas::AddCharacter( const FCharacterRenderData& RenderData )
{
	return AddTexture( RenderData.MeasureInfo.SizeX, RenderData.MeasureInfo.SizeY, RenderData.RawPixels );
}

void FSlateFontAtlas::Flush()
{
	// Empty the atlas
	Empty();

	// Recreate the data
	InitAtlasData();

	bNeedsUpdate = true;
	ConditionalUpdateTexture();
}