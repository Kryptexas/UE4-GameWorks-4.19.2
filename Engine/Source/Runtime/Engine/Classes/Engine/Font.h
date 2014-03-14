// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Font.generated.h"

/** this struct is serialized using native serialization so any changes to it require a package version bump */
USTRUCT()
struct FFontCharacter
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=FontCharacter)
	int32 StartU;

	UPROPERTY(EditAnywhere, Category=FontCharacter)
	int32 StartV;

	UPROPERTY(EditAnywhere, Category=FontCharacter)
	int32 USize;

	UPROPERTY(EditAnywhere, Category=FontCharacter)
	int32 VSize;

	UPROPERTY(EditAnywhere, Category=FontCharacter)
	uint8 TextureIndex;

	UPROPERTY(EditAnywhere, Category=FontCharacter)
	int32 VerticalOffset;


	FFontCharacter()
		: StartU(0)
		, StartV(0)
		, USize(0)
		, VSize(0)
		, TextureIndex(0)
		, VerticalOffset(0)
	{
	}

	/**
	 * Serialization.
	 * @param Ar - The archive with which to serialize.
	 * @returns true if serialization was successful.
	 */
	bool Serialize( FArchive& Ar )
	{
		Ar << *this;
		return true;
	}

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FFontCharacter& Ch )
	{
		Ar << Ch.StartU << Ch.StartV << Ch.USize << Ch.VSize << Ch.TextureIndex;
		Ar << Ch.VerticalOffset;
		return Ar;
	}
};

template<>
struct TStructOpsTypeTraits<FFontCharacter> : public TStructOpsTypeTraitsBase
{
	enum 
	{
		WithSerializer = true,
	};
};

/**
 * A font object, containing information about a set of glyphs.
 * The glyph bitmaps are stored in the contained textures, while
 * the font database only contains the coordinates of the individual
 * glyph.
 */
UCLASS(hidecategories=Object, dependson=UFontImportOptions, autoexpandcategories=Font, MinimalAPI, BlueprintType)
class UFont : public UObject
{
	GENERATED_UCLASS_BODY()

	/** List of characters in the font.  For a MultiFont, this will include all characters in all sub-fonts!  Thus,
		the number of characters in this array isn't necessary the number of characters available in the font */
	UPROPERTY(EditAnywhere, editinline, Category=Font)
	TArray<struct FFontCharacter> Characters;

	/** Textures that store this font's glyph image data */
	//NOTE: Do not expose this to the editor as it has nasty crash potential
	UPROPERTY()
	TArray<class UTexture2D*> Textures;

	/** True if font is 'remapped'.  That is, the character array is not a direct mapping to unicode values.  Instead,
		all characters are indexed indirectly through the CharRemap array */
	UPROPERTY()
	int32 IsRemapped;

	/** Font metrics. */
	UPROPERTY(EditAnywhere, Category=Font)
	float EmScale;

	/** @todo document */
	UPROPERTY(EditAnywhere, Category=Font)
	float Ascent;

	/** @todo document */
	UPROPERTY(EditAnywhere, Category=Font)
	float Descent;

	/** @todo document */
	UPROPERTY(EditAnywhere, Category=Font)
	float Leading;

	/** Default horizontal spacing between characters when rendering text with this font */
	UPROPERTY(EditAnywhere, Category=Font)
	int32 Kerning;

	/** Options used when importing this font */
	UPROPERTY(EditAnywhere, Category=Font)
	struct FFontImportOptionsData ImportOptions;

	/** Number of characters in the font, not including multiple instances of the same character (for multi-fonts).
		This is cached at load-time or creation time, and is never serialized. */
	UPROPERTY(transient)
	int32 NumCharacters;

	/** The maximum height of a character in this font.  For multi-fonts, this array will contain a maximum
		character height for each multi-font, otherwise the array will contain only a single element.  This is
		cached at load-time or creation time, and is never serialized. */
	UPROPERTY(transient)
	TArray<int32> MaxCharHeight;

	/** Scale to apply to the font. */
	UPROPERTY(EditAnywhere, Category=Font)
	float ScalingFactor;


public:
	/** This is the character that RemapChar will return if the specified character doesn't exist in the font */
	static const TCHAR NULLCHARACTER = 127;

	/** When IsRemapped is true, this array maps unicode values to entries in the Characters array */
	TMap<uint16,uint16> CharRemap;

	/**
	 * Returns the size of the object/ resource for display to artists/ LDs in the Editor.
	 *
	 * @return		Size of resource as to be displayed to artists/ LDs in the Editor.
	 */
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;

	// UFont interface
	FORCEINLINE TCHAR RemapChar(TCHAR CharCode) const
	{
		const uint16 UCode = CharCast<UCS2CHAR>(CharCode);
		if ( IsRemapped )
		{
			// currently, fonts are only remapped if they contain Unicode characters.
			// For remapped fonts, all characters in the CharRemap map are valid, so
			// if the characters exists in the map, it's safe to use - otherwise, return
			// the null character (an empty square on windows)
			const uint16* FontChar = CharRemap.Find(UCode);
			if ( FontChar == NULL )
				return NULLCHARACTER;

			return (TCHAR)*FontChar;
		}

		// Otherwise, our Characters array will contains 256 members, and is
		// a one-to-one mapping of character codes to array indexes, though
		// not every character is a valid character.
		if ( UCode >= NumCharacters )
		{
			return NULLCHARACTER;
		}

		// If the character's size is 0, it's non-printable or otherwise unsupported by
		// the font.  Return the default null character (an empty square on windows).
		if ( !Characters.IsValidIndex(UCode) || (Characters[UCode].VSize == 0 && UCode >= TEXT(' ')) )
		{
			return NULLCHARACTER;
		}

		return CharCode;
	}

	FORCEINLINE void GetCharSize(TCHAR InCh, float& Width, float& Height) const
	{
		Width = Height = 0.f;

		const int32 Ch = (int32)RemapChar(InCh);
		if( Ch < Characters.Num() )
		{
			const FFontCharacter& Char = Characters[Ch];
			if( Char.TextureIndex < Textures.Num() && Textures[Char.TextureIndex] != NULL )
			{
				Width = Char.USize;

				// The height of the character will always be the maximum height of any character in this
				// font.  This ensures consistent vertical alignment of text.  For example, we don't want
				// vertically centered text to visually shift up and down as characters are added to a string.
				// NOTE: This also gives us consistent alignment with fonts generated by the legacy importer.
				const int32 MultiFontIndex = Ch / NumCharacters;
				Height = MaxCharHeight[ MultiFontIndex ];
			}
		}
	}

	/**
	 * Calculate the width of the string using this font's default size and scale.
	 *
	 * @param	Text					the string to size
	 *
	 * @return	the width (in pixels) of the specified text, or 0 if Text was NULL.
	 */
	FORCEINLINE int32 GetStringSize( const TCHAR *Text ) const
	{
		float	Width, Height, Total;

		Total = 0.0f;
		while( *Text )
		{
			GetCharSize( *Text++, Width, Height );
			Total += Width;
		}

		return( FMath::Ceil( Total ) );
	}

	/**
	 * Calculate the width of the string using this font's default size and scale.
	 *
	 * @param	Text					the string to size
	 *
	 * @return	the width (in pixels) of the specified text, or 0 if Text was NULL.
	 */
	FORCEINLINE int32 GetStringHeightSize( const TCHAR *Text ) const
	{
		float	Width, Height, Total;

		Total = 0.0f;
		while( *Text )
		{
			GetCharSize( *Text++, Width, Height );
			Total = FMath::Max( Total, Height );
		}

		return( FMath::Ceil( Total ) );
	}

	// Begin UObject interface
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual bool IsLocalizedResource() OVERRIDE;
	// End UObject interface

	/**
	 * Caches the character count and maximum character height for this font (as well as sub-fonts, in the multi-font case)
	 */
	virtual void CacheCharacterCountAndMaxCharHeight();


	/**
	 *	Set the scaling factor
	 *
	 *	@param	InScalingFactor		The scaling factor to set
	 */
	virtual void SetFontScalingFactor(float InScalingFactor)
	{
		ScalingFactor = InScalingFactor;
	}

	/**
	 *	Get the scaling factor
	 *
	 *	@return	float		The scaling factor currently set
	 */
	virtual float GetFontScalingFactor()
	{
		return ScalingFactor;
	}

	/** Returns the maximum height for any character in this font */
	virtual float GetMaxCharHeight() const;
	
	/** Determines the height and width for the passed in string. */
	virtual void GetStringHeightAndWidth( const FString& InString, int32& Height, int32& Width ) const;
};



