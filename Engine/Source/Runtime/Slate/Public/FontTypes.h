// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

struct SLATE_API FSlateFontKey
{
	FSlateFontInfo FontInfo;
	float Scale;

	FSlateFontKey( const FSlateFontInfo& InInfo, float InScale )
		: FontInfo( InInfo )
		, Scale( InScale )
	{

	}

	bool operator==(const FSlateFontKey& Other ) const
	{
		return FontInfo == Other.FontInfo && Scale == Other.Scale;
	}

	friend inline uint32 GetTypeHash( const FSlateFontKey& Key )
	{
		return GetTypeHash(Key.FontInfo) + FCrc::MemCrc32( &Key.Scale, sizeof(float) );
	}
};

/** Measurement details for a specific character */
struct FCharacterMeasureInfo
{
	/** Width of the character in pixels */
	int16 SizeX;
	/** Height of the character in pixels */
	int16 SizeY;
	/** How many pixels to advance in X after drawing this character (when its part of a string)*/
	int16 XAdvance;
	/** The largest vertical distance below the baseline for any character in the font */
	int16 GlobalDescender;
	/** The vertical distance from the baseline to the topmost border of the glyph bitmap */
	int16 VerticalOffset;
	/** The horizontal distance from the origin to the leftmost border of the character */
	int16 HorizontalOffset;
	FCharacterMeasureInfo( int16 InSizeX = 0, int16 InSizeY = 0, int16 InXAdvance = 0, int16 InVerticalOffset = 0, int16 InHorizontalOffset = 0 )
		: SizeX(InSizeX),SizeY(InSizeY),XAdvance(InXAdvance),VerticalOffset(InVerticalOffset), HorizontalOffset(InHorizontalOffset)
	{
	}
};

/** Contains pixel data for a character rendered from freetype as well as measurement info */
struct FCharacterRenderData
{
	FCharacterMeasureInfo MeasureInfo;
	/** Measurement data for this character */
	/** Raw pixels created by freetype */
	TArray<uint8> RawPixels;
	/** @todo Doesnt belong here. */
	uint16 MaxHeight;
	/** The character that was rendered */
	TCHAR Char;	
};

/** 
 * Representation of a texture for fonts in which characters are packed tightly based on their bounding rectangle 
 */
class SLATE_API FSlateFontAtlas : public FSlateTextureAtlas
{
public:
	FSlateFontAtlas( uint32 InWidth, uint32 InHeight );
	virtual ~FSlateFontAtlas();

	/**
	 * Returns the texture resource used by slate
	 */
	virtual class FSlateShaderResource* GetTexture() = 0;
	
	/**
	 * Releases rendering resources of this cache
	 */
	virtual void ReleaseResources(){}

	/**
	 * Flushes all cached data.
	 */
	void Flush();

	/** 
	 * Adds a character to the texture.
	 *
	 * @param CharInfo	Information about the size of the character
	 */
	const struct FAtlasedTextureSlot* AddCharacter( const FCharacterRenderData& CharInfo );
};

class SLATE_API ISlateFontAtlasFactory
{
public:
	virtual TSharedRef<class FSlateFontAtlas> CreateFontAtlas() const = 0;
};