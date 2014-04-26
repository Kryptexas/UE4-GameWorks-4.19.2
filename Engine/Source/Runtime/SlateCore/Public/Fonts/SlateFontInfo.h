// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateFontInfo.h: Declares the FSlateFontInfo structure.
=============================================================================*/

#pragma once

#include "SlateFontInfo.generated.h"


/**
 * A representation of a font in Slate.
 */
USTRUCT()
struct SLATECORE_API FSlateFontInfo
{
	GENERATED_USTRUCT_BODY()

	/** The name of the font */
	UPROPERTY(EditAnywhere, Category=SlateStyleRules)
	FName FontName;

	/** The size of the font */
	UPROPERTY(EditAnywhere, Category=SlateStyleRules)
	int32 Size;

public:

	/**
	 * Default constructor.
	 */
	FSlateFontInfo();

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 */
	FSlateFontInfo( const FString& InFontName, uint16 InSize );

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 */
	FSlateFontInfo( const FName& InFontName, uint16 InSize );

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 */
	FSlateFontInfo( const ANSICHAR* InFontName, uint16 InSize );

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 */
	FSlateFontInfo( const WIDECHAR* InFontName, uint16 InSize );

public:

	/**
	 * Compares this font info with another for equality.
	 *
	 * @param Other The other font info.
	 *
	 * @return true if the two font infos are equal, false otherwise.
	 */
	bool operator==( const FSlateFontInfo& Other ) const 
	{
		return FontName == Other.FontName && Size == Other.Size;
	}

	/**
	 * Calculates a type hash value for a font info.
	 *
	 * Type hashes are used in certain collection types, such as TMap.
	 *
	 * @param FontInfo The font info to calculate the hash for.
	 *
	 * @return The hash value.
	 */
	friend inline uint32 GetTypeHash( const FSlateFontInfo& FontInfo )
	{
		return GetTypeHash(FontInfo.FontName) + (FontInfo.Size << 17);
	}
};
