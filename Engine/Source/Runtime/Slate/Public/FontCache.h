// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FCharacterRenderData;
class FFreeTypeInterface;


/** Information for rendering one character */
struct FCharacterEntry
{
	/** Start X location of the character in the texture */
	float StartU;
	/** Start Y location of the character in the texture */
	float StartV;
	/** X Size of the character in the texture */
	float USize;
	/** Y Size of the character in the texture */
	float VSize;
	/** The vertical distance from the baseline to the topmost border of the character */
	int16 VerticalOffset;
	/** The vertical distance from the origin to the left most border of the character */
	int16 HorizontalOffset;
	/** The largest vertical distance below the baseline for any character in the font */
	int16 GlobalDescender;
	/** The amount to advance in X before drawing the next character in a string */
	int16 XAdvance;
	/** Index to a specific texture in the font cache. */
	uint8 TextureIndex;
	/** 1 if this entry is valid, 0 otherwize. */
	uint8 Valid;

	FCharacterEntry()
	{
		FMemory::Memzero( this, sizeof(FCharacterEntry) );
	}

	/**
	 * Checks if the character entry is valid (I.E has been cached)
	 * 
	 * @param InEntry The entry to check
	 */
	FORCEINLINE bool IsValidEntry() const
	{
		return Valid == 1;
	}
};

struct FKerningPair
{
	TCHAR First;
	TCHAR Second;

	FKerningPair( TCHAR InFirst, TCHAR InSecond )
		: First(InFirst),Second(InSecond)
	{
	}

	bool operator==(const FKerningPair& Other ) const
	{
		return First == Other.First && Second == Other.Second;
	}

	friend inline uint32 GetTypeHash( const FKerningPair& Key )
	{
		return FCrc::MemCrc32( &Key, sizeof(Key) );
	}
};

/**
 * A Kerning table for a single font key
 */
class FKerningTable
{
public:
	FKerningTable( const FSlateFontKey& InFont, const FSlateFontCache& InFontCache );
	~FKerningTable();

	/**
	 * Gets a kerning value for a pair of characters
	 *
	 * @param FirstChar		The first character in the pair
	 * @param SecondChar	The second character in the pair
	 * @return The kerning value
	 */
	int8 GetKerning( TCHAR FirstChar, TCHAR SecondChar );
private:
	/**
	 * Allocated memory for the directly indexed kerning table
	 */
	void CreateDirectTable();
private:
	/** Extended kerning Pairs which are mapped to save memory */
	TMap<FKerningPair,int8> MappedKerningPairs;
	/** Direct access kerning table for ascii chars.  Note its very important that this stays small.  Kerni */
	int8* DirectAccessTable;
	/** Interface to freetype for accessing new kerning values */
	const class FSlateFontCache& FontCache;
	/** Font this kerning table is for */
	FSlateFontKey FontKey;
	/** Whether the font has kerning at all */
	bool bHasKerning;
};


/**
 * Manages a potentially large list of font characters
 * Uses a directly indexed by TCHAR array until space runs out and then maps the rest to conserve memory
 * Every character indexed by TCHAR could potentially cost a lot of memory of a lot of empty entries are created
 * because characters being used are far apart
 */
class FCharacterList
{
public:
	FCharacterList( const FSlateFontKey& InFontKey, const FSlateFontCache& InFontCache );

	/* @return Is the character in this list */
	bool IsValidIndex( TCHAR Character ) const
	{
		return DirectIndexEntries.IsValidIndex( Character ) || ( Character >= MaxDirectIndexedEntries && MappedEntries.Contains( Character ) );
	}

	/**
	 * Gets data about how to render and measure a character 
	 * Caching and atlasing it if needed
	 *
	 * @param Character	The character to get
	 * @return Data about the character
	 */
	const FCharacterEntry& operator[]( TCHAR Character )
	{
		if( Character < MaxDirectIndexedEntries )
		{
			// The character can be indexed directly

			int32 NumToAdd = (Character - DirectIndexEntries.Num()) + 1;
			if( NumToAdd > 0 )
			{
				// The character doesnt exist yet in the index and there is not enough space
				// Resize the array now
				DirectIndexEntries.AddZeroed( NumToAdd );
			}
			
			FCharacterEntry& CharacterEntry = DirectIndexEntries[ Character ];
			if( !CharacterEntry.IsValidEntry() )
			{
				// Character has not been cached yet
				CharacterEntry = CacheCharacter( Character );
			}

			return CharacterEntry;
		}
		else
		{
			FCharacterEntry& CharacterEntry = MappedEntries.FindOrAdd( Character );
			if( !CharacterEntry.IsValidEntry() )
			{
				// Character has not been cached yet
				CharacterEntry = CacheCharacter( Character );
			}

			return CharacterEntry;
		}
	}

	/**
	 * Gets a kerning value for a pair of characters
	 *
	 * @param FirstChar		The first character in the pair
	 * @param SecondChar	The second character in the pair
	 * @return The kerning value
	 */
	int8 GetKerning( TCHAR First, TCHAR Second );

	/**
	 * @return The global max height for any character in this font
	 */
	uint16 GetMaxHeight() const;

	/** 
	 * Returns the baseline for the font used by this character 
	 *
	 * @return The offset from the bottom of the max character height to the baseline. Be aware that the value will be negative.
	 */
	int16 GetBaseline() const;

private:

	/**
	 * Caches a new character
	 * 
	 * @param Character	The character to cache
	 */
	FCharacterEntry& CacheCharacter( TCHAR Character );


private:

	/** Entries for larger character sets to conserve memory */
	TMap<TCHAR, FCharacterEntry> MappedEntries; 
	/** Directly indexed entries for fast lookup */
	TArray<FCharacterEntry> DirectIndexEntries;
	/** Table of kerning values for this font */
	FKerningTable KerningTable;
	/** Font for this character list */
	FSlateFontKey FontKey;
	/** Reference to the font cache for accessing new unseen characters */
	const class FSlateFontCache& FontCache;
	/** Number of directly indexed entries */
	int32 MaxDirectIndexedEntries;
	/** The global max height for any character in this font */
	mutable uint16 MaxHeight;
	/** The offset from the bottom of the max character height to the baseline. */
	mutable uint16 Baseline;
};

/*
 * Font caching implementation
 * Caches characters into textures as needed
 */
class SLATE_API FSlateFontCache
{
public:
	/**
	 * Constructor
	 *
	 * @param InTextureSize The size of the atlas texture
	 * @param InFontAlas	Platform specific font atlas resource
	 */
	FSlateFontCache( TSharedRef<ISlateFontAtlasFactory> InFontAtlasFactory );
	virtual ~FSlateFontCache();

	/** 
	 * Gets information for how to draw all characters in the specified string. Caches characters as they are found
	 * 
	 * @param Text			The string to get character information for
	 * @param InFontInfo	Information about the font that the string is drawn with
	 * @param FontScale	The scale to apply to the font
	 * @param OutCharacterEntries	Populated array of character entries. Indices of characters in Text match indices in this array
	 */
	class FCharacterList& GetCharacterList( const FSlateFontInfo &InFontInfo, float FontScale ) const;

	/** 
	 * Add a new entries into a cache atlas
	 *
	 * @param InFontInfo	Information about the font being used for the characters
	 * @param Characters	The characters to cache
	 * @param FontScale		The font scale to use
	 * @return true if the characters could be cached. false if the cache is full
	 */
	bool AddNewEntry( TCHAR Character, const FSlateFontKey& InKey, FCharacterEntry& OutCharacterEntry ) const;

	/** 
	 * Flush the cache if needed
	 */
	void ConditionalFlushCache();

	/**
	 * Updates the texture used for rendering
	 */
	void UpdateCache();

	/**
	 * Releases rendering resources
	 */
	void ReleaseResources();

	/**
	 * Get the texture resource for a font atlas at a given index
	 * 
	 * @param Index	The index of the texture 
	 * @return Handle to the texture resource
	 */
	FSlateShaderResource* GetTextureResource( uint32 Index ) { return FontAtlases[Index]->GetTexture(); }

	/**
	 * Calculates the kerning amount for a pair of characters
	 *
	 * @param First		The first character in the pair
	 * @param Second	The second character in the pair
	 * @oaran FontKey	The font to get kerning from
	 * @return The kerning amount, 0 if no kerning
	 */
	int8 GetKerning( TCHAR First, TCHAR Second, const FSlateFontKey& FontKey ) const;

	/**
	 * @return Whether or not a font has kerning information
	 */
	bool HasKerning( const FSlateFontInfo& InFontInfo ) const;

	/**
	 * Returns the height of the largest character in the font. 
	 *
	 * @param InFontInfo	A descriptor of the font to get character size for 
	 * @param FontScale		The scale to apply to the font
	 * 
	 * @return The largest character height
	 */
	uint16 GetMaxCharacterHeight( const FSlateFontInfo& InFontInfo, float FontScale ) const;

	/**
	 * Returns the baseline for the specified font.
	 *
	 * @param InFontInfo	A descriptor of the font to get character size for 
	 * @param FontScale		The scale to apply to the font
	 * 
	 * @return The offset from the bottom of the max character height to the baseline.
	 */
	uint16 GetBaseline( const FSlateFontInfo& InFontInfo, float FontScale ) const;

	/**
	 * Clears all cached data from the cache
	 */
	void FlushCache() const;


private:

	/** Mapping Font keys to cached data */
	mutable TMap<FSlateFontKey, TSharedRef< class FCharacterList > > FontToCharacterListCache;

	/** Interface to the freetype library */
	mutable TSharedRef<FFreeTypeInterface> FTInterface;

	/** Array of all font atlases */
	mutable TArray< TSharedRef<FSlateFontAtlas> > FontAtlases;

	/** Factory for creating new font atlases */
	TSharedRef<ISlateFontAtlasFactory> FontAtlasFactory;

	/** Whether or not we have a pending request to flush the cache when it is safe to do so */
	mutable bool bFlushRequested;
};
