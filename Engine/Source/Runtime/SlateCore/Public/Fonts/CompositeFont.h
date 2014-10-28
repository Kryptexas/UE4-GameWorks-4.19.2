// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CompositeFont.generated.h"

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

/** Payload data describing an individual font in a typeface */
USTRUCT(BlueprintType)
struct SLATECORE_API FFontData
{
	GENERATED_USTRUCT_BODY()

	/** Default constructor */
	FFontData()
		: FontFilename()
		, FontData()
		, Hinting(EFontHinting::Default)
	{
	}

	/** Construct the raw font data from a filename (it will load the file into FontData) */
	FFontData(FString InFontFilename, const EFontHinting InHinting);

	/** Construct the raw font data from a filename, and the data associated with that file */
	FFontData(FString InFontFilename, TArray<uint8> InFontData, const EFontHinting InHinting)
		: FontFilename(MoveTemp(InFontFilename))
		, FontData(MoveTemp(InFontData))
		, Hinting(InHinting)
	{
	}

	/** Set the new font data from a filename (it will load the file into FontData) */
	bool SetFont(FString InFontFilename);

	/** Set the new font data from a filename, and the data associated with that file */
	void SetFont(FString InFontFilename, TArray<uint8> InFontData);

	/** The filename of the font to use - this may not always exist on disk, as we may have previously loaded and cached the font data inside an asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	FString FontFilename;

	/** The data associated with the font - this should always be filled in providing the source font filename is valid */
	UPROPERTY()
	TArray<uint8> FontData;

	/** The hinting algorithm to use with the font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	EFontHinting Hinting;
};

/** A single entry in a typeface */
USTRUCT(BlueprintType)
struct SLATECORE_API FTypefaceEntry
{
	GENERATED_USTRUCT_BODY()

	/** Default constructor */
	FTypefaceEntry()
	{
	}

	/** Construct the entry from a name */
	FTypefaceEntry(const FName& InFontName)
		: Name(InFontName)
	{
	}

	/** Construct the entry from a filename */
	FTypefaceEntry(const FName& InFontName, FString InFontFilename, const EFontHinting InHinting)
		: Name(InFontName)
		, Font(MoveTemp(InFontFilename), InHinting)
	{
	}

	/** Construct the entry from a filename, and the data associated with that file */
	FTypefaceEntry(const FName& InFontName, FString InFontFilename, TArray<uint8> InFontData, const EFontHinting InHinting)
		: Name(InFontName)
		, Font(MoveTemp(InFontFilename), MoveTemp(InFontData), InHinting)
	{
	}

	/** Name used to identify this font within its typeface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	FName Name;

	/** Raw font data for this font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	FFontData Font;
};

/** Definition for a typeface (a family of fonts) */
USTRUCT(BlueprintType)
struct SLATECORE_API FTypeface
{
	GENERATED_USTRUCT_BODY()

	/** Default constructor */
	FTypeface()
	{
	}

	/** Convenience constructor for when your font family only contains a single font */
	FTypeface(const FName& InFontName, FString InFontFilename, const EFontHinting InHinting)
	{
		Fonts.Emplace(FTypefaceEntry(InFontName, MoveTemp(InFontFilename), InHinting));
	}

	/** Convenience constructor for when your font family only contains a single font */
	FTypeface(const FName& InFontName, FString InFontFilename, TArray<uint8> InFontData, const EFontHinting InHinting)
	{
		Fonts.Emplace(FTypefaceEntry(InFontName, MoveTemp(InFontFilename), MoveTemp(InFontData), InHinting));
	}

	/** Append a new font into this family */
	FTypeface& AppendFont(const FName& InFontName, FString InFontFilename, const EFontHinting InHinting = EFontHinting::Default)
	{
		Fonts.Emplace(FTypefaceEntry(InFontName, MoveTemp(InFontFilename), InHinting));
		return *this;
	}

	/** Append a new font into this family */
	FTypeface& AppendFont(const FName& InFontName, FString InFontFilename, TArray<uint8> InFontData, const EFontHinting InHinting = EFontHinting::Default)
	{
		Fonts.Emplace(FTypefaceEntry(InFontName, MoveTemp(InFontFilename), MoveTemp(InFontData), InHinting));
		return *this;
	}

	/** The fonts contained within this family */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	TArray<FTypefaceEntry> Fonts;
};

USTRUCT(BlueprintType)
struct SLATECORE_API FCompositeSubFont
{
	GENERATED_USTRUCT_BODY()

	/** Default constructor */
	FCompositeSubFont()
		: Typeface()
		, CharacterRanges()
		, ScalingFactor(1.0f)
	{
	}

	/** Typeface data for this sub-font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	FTypeface Typeface;

	/** Array of character ranges for which this sub-font should be used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	TArray<FInt32Range> CharacterRanges;

	/** Amount to scale this sub-font so that it better matches the size of the default font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Font)
	float ScalingFactor;
};

USTRUCT(BlueprintType)
struct SLATECORE_API FCompositeFont
{
	GENERATED_USTRUCT_BODY()

	/** Default constructor */
	FCompositeFont()
		: DefaultTypeface()
		, SubTypefaces()
		, HistoryRevision(0)
	{
	}

	/** Convenience constructor for when your composite font only contains a single font */
	FCompositeFont(const FName& InFontName, FString InFontFilename, const EFontHinting InHinting = EFontHinting::Default)
		: DefaultTypeface(InFontName, MoveTemp(InFontFilename), InHinting)
		, SubTypefaces()
		, HistoryRevision(0)
	{
	}

	/** Convenience constructor for when your composite font only contains a single font */
	FCompositeFont(const FName& InFontName, FString InFontFilename, TArray<uint8> InFontData, const EFontHinting InHinting = EFontHinting::Default)
		: DefaultTypeface(InFontName, MoveTemp(InFontFilename), MoveTemp(InFontData), InHinting)
		, SubTypefaces()
		, HistoryRevision(0)
	{
	}

	/** Call this when the composite font is changed after its initial setup - this allows various caches to update as required */
	void MakeDirty()
	{
		++HistoryRevision;
	}

	/** The default typeface that will be used when not overridden by a sub-typeface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RuntimeFont)
	FTypeface DefaultTypeface;

	/** Sub-typefaces to use for a specific set of characters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RuntimeFont)
	TArray<FCompositeSubFont> SubTypefaces;

	/** 
	 * Transient value containing the current history ID of this composite font
	 * This should be updated when the composite font is changed (which should happen infrequently as composite fonts are assumed to be mostly immutable) once they've been setup
	 */
	int32 HistoryRevision;
};
