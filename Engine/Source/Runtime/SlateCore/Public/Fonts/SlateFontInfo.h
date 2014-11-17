// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateFontInfo.generated.h"

UENUM()
enum class EFontHinting : uint8
{
	/** Use the default hinting specified in the font */
	Default,
	/** Force the use of an automatic hinting algorithm */
	Auto,
	/** Force the use of an automatic light hinting algorithm, optimized for non-monochrome displays */
	AutoLight,
	/** Force the use of an automatic hinting algorithm optimized for monochrome displays */
	Monochrome,
	/** Do not use hinting */
	None,
};

/**
 * A representation of a font in Slate.
 */
USTRUCT(BlueprintType)
struct SLATECORE_API FSlateFontInfo
{
	GENERATED_USTRUCT_BODY()

	/** The name of the font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SlateStyleRules)
	FName FontName;

	/** The size of the font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SlateStyleRules)
	int32 Size;

	/** The hinting algorithm to use with the font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SlateStyleRules)
	EFontHinting Hinting;

public:

	/** Default constructor. */
	FSlateFontInfo();

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 * @param InHinting The type of hinting to use for the font.
	 */
	FSlateFontInfo( const FString& InFontName, uint16 InSize, EFontHinting InHinting = EFontHinting::Default );

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 * @param InHinting The type of hinting to use for the font.
	 */
	FSlateFontInfo( const FName& InFontName, uint16 InSize, EFontHinting InHinting = EFontHinting::Default );

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 * @param InHinting The type of hinting to use for the font.
	 */
	FSlateFontInfo( const ANSICHAR* InFontName, uint16 InSize, EFontHinting InHinting = EFontHinting::Default );

	/**
	 * Creates and initializes a new instance with the specified font name and size.
	 *
	 * @param InFontName The name of the font.
	 * @param InSize The size of the font.
	 * @param InHinting The type of hinting to use for the font.
	 */
	FSlateFontInfo( const WIDECHAR* InFontName, uint16 InSize, EFontHinting InHinting = EFontHinting::Default );

public:

	/**
	 * Compares this font info with another for equality.
	 *
	 * @param Other The other font info.
	 * @return true if the two font infos are equal, false otherwise.
	 */
	bool operator==( const FSlateFontInfo& Other ) const 
	{
		return FontName == Other.FontName && Size == Other.Size && Hinting == Other.Hinting;
	}

	/**
	 * Compares this font info with another for inequality.
	 *
	 * @param Other The other font info.
	 *
	 * @return false if the two font infos are equal, true otherwise.
	 */
	bool operator!=( const FSlateFontInfo& Other ) const 
	{
		return !(*this == Other);
	}

	/**
	 * Calculates a type hash value for a font info.
	 *
	 * Type hashes are used in certain collection types, such as TMap.
	 *
	 * @param FontInfo The font info to calculate the hash for.
	 * @return The hash value.
	 */
	friend inline uint32 GetTypeHash( const FSlateFontInfo& FontInfo )
	{
		uint32 Hash = 0;
		Hash = HashCombine(Hash, GetTypeHash(FontInfo.FontName));
		Hash = HashCombine(Hash, FontInfo.Size);
		return HashCombine(Hash, uint32(FontInfo.Hinting));
	}
};
