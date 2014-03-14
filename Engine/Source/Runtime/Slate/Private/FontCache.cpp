// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "FontCache.h"
#include "FontCacheUtils.h"
#include "WordWrapper.h"

#ifndef WITH_FREETYPE
	#define WITH_FREETYPE	0
#endif // WITH_FREETYPE

#if PLATFORM_COMPILER_HAS_GENERIC_KEYWORD
	#define generic __identifier(generic)
#endif	//PLATFORM_COMPILER_HAS_GENERIC_KEYWORD

#if WITH_FREETYPE
	#include "ft2build.h"

	// Freetype style include
	#include FT_FREETYPE_H
	#include FT_GLYPH_H
	#include FT_MODULE_H

#endif // WITH_FREETYPE

namespace FontCacheConstants
{
	/** The horizontal dpi we render at */
	const uint32 HorizontalDPI = 96;
	/** The vertical dpi we render at */
	const uint32 VerticalDPI = 96;

	/** Number of characters that can be indexed directly in the cache */
	const int32 DirectAccessSize = 256;

	/** Number of possible elements in each measurement cache */
	const uint32 MeasureCacheSize = 500;
}

#if WITH_FREETYPE
const uint32 GlyphFlags = FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_BITMAP;

/**
 * Memory allocation functions to be used only by freetype
 */
static void* FreetypeAlloc( FT_Memory Memory, long size )
{
	return FMemory::Malloc( size );
}

static void* FreetypeRealloc( FT_Memory Memory, long CurSize, long NewSize, void* Block )
{
	return FMemory::Realloc( Block, NewSize );
}

static void FreetypeFree( FT_Memory Memory, void* Block )
{
	return FMemory::Free( Block );
}

#endif // WITH_FREETYPE

/**
 * An interface to the freetype API.                     
 */
class FFreeTypeInterface
{
public:
	FFreeTypeInterface()
	{
#if WITH_FREETYPE
		CustomMemory = (FT_Memory)FMemory::Malloc( sizeof(*CustomMemory) );
		// Init freetype
		CustomMemory->alloc = FreetypeAlloc;
		CustomMemory->realloc = FreetypeRealloc;
		CustomMemory->free = FreetypeFree;
		CustomMemory->user = NULL;

		int32 Error = FT_New_Library( CustomMemory, &FTLibrary );
		
		if ( Error )
		{
			checkf(0, TEXT("Could not init Freetype"));
		}
		
		FT_Add_Default_Modules( FTLibrary );
#endif // WITH_FREETYPE
	}

	~FFreeTypeInterface()
	{
#if WITH_FREETYPE
		// Clear before releasing freetype resources
		Flush();

		FT_Done_Library( FTLibrary );
		FMemory::Free( CustomMemory );
#endif // WITH_FREETYPE
	}

	/**
	 * Flushes stored data.
	 */ 
	void Flush()
	{
#if WITH_FREETYPE
		FontToKerningPairMap.Empty();
		// toss memory
		for (TMap<FName, FFontFaceAndMemory>::TIterator It(FontFaceMap); It; ++It)
		{
			FMemory::Free(It.Value().Memory);
		}
		FontFaceMap.Empty();
#endif // WITH_FREETYPE
	}

	/** 
	 * Creates render data for a specific character 
	 * 
	 * @param InFontInfo	Information about the font to render the character with
	 * @param Char			The character to render
	 * @param OutCharInfo	Will contain the created render data
	 */
	void GetRenderData( const FSlateFontInfo& InFontInfo, TCHAR Char, FCharacterRenderData& OutRenderData, float Scale )
	{
#if WITH_FREETYPE
		// Find or load the face if needed
		FT_UInt GlyphIndex = 0;
		FT_Face FontFace = GetFontFace( InFontInfo.FontName );

		if ( FontFace != NULL ) 
		{
			// Get the index to the glyph in the font face
			GlyphIndex = FT_Get_Char_Index( FontFace, Char );
		}

		uint32 LocalGlyphFlags = GlyphFlags;

		// If the requested glyph doesn't exist, use the localization fallback font.
		if( GlyphIndex == 0 )
		{
			static FName FallbackFontName( NAME_None );
			if( FallbackFontName == NAME_None )
			{
				FText FallbackFontFilename;
				FallbackFontName = FName( *( FPaths::EngineContentDir() / TEXT("Slate/Fonts/") / ( NSLOCTEXT("Slate", "FallbackFont", "DroidSansFallback").ToString() + TEXT(".ttf") ) ) );
			}
			if( FallbackFontName.IsValid() && FallbackFontName != NAME_None )
			{
				FontFace = GetFontFace( FallbackFontName );
				if (FontFace != NULL)
				{					
					GlyphIndex = FT_Get_Char_Index( FontFace, Char );
					LocalGlyphFlags |= FT_LOAD_FORCE_AUTOHINT;
				}
				else
				{
					FallbackFontName = FName(NAME_Inactive);
				}
			}
		}

		// If the requested glyph doesn't exist, use the last resort fallback font.
		if( GlyphIndex == 0 )
		{
			static FName LastResortFontName( NAME_None );
			if( LastResortFontName == NAME_None )
			{
				LastResortFontName = FName( *( FPaths::EngineContentDir() / TEXT("Slate/Fonts/LastResort.ttf") ) );
			}
			if( LastResortFontName.IsValid() && LastResortFontName != NAME_None )
			{
				FontFace = GetFontFace( LastResortFontName );
				check( FontFace );
				GlyphIndex = FT_Get_Char_Index( FontFace, Char );
				LocalGlyphFlags |= FT_LOAD_FORCE_AUTOHINT;
			}
			else
			{
				LastResortFontName = FName(NAME_Inactive);
			}
		}

		// Set the character size to render at (needs to be in 1/64 of a "point")
		FT_Error Error = FT_Set_Char_Size( FontFace, 0, InFontInfo.Size*64, FontCacheConstants::HorizontalDPI, FontCacheConstants::VerticalDPI );
		check(Error==0);

		if( Scale != 1.0f )
		{
			FT_Matrix ScaleMatrix;
			ScaleMatrix.xy = 0;
			ScaleMatrix.xx = (FT_Fixed)(Scale * 65536);
			ScaleMatrix.yy = (FT_Fixed)(Scale * 65536);
			ScaleMatrix.yx = 0;
			FT_Set_Transform( FontFace, &ScaleMatrix, NULL );
		}
		else
		{
			FT_Set_Transform( FontFace, NULL, NULL );
		}


		// Load the glyph.  Force using the freetype hinter because not all true type fonts have their own hinting
		Error = FT_Load_Glyph( FontFace, GlyphIndex, LocalGlyphFlags );
		check(Error==0);

		// Get the slot for the glyph.  This contains measurement info
		FT_GlyphSlot Slot = FontFace->glyph;

		FT_Render_Glyph( Slot, FT_RENDER_MODE_NORMAL );

		// one byte per pixel 
		const uint32 GlyphPixelSize = 1;

		FT_Bitmap Bitmap = Slot->bitmap;

		OutRenderData.RawPixels.Reset();
		OutRenderData.RawPixels.AddUninitialized( Bitmap.rows * Bitmap.width );

		if( Bitmap.pixel_mode == FT_PIXEL_MODE_MONO )
		{
			uint32 BytesPerLine = Bitmap.pitch;

			// For each texel generate a corresponding color for a 32 bit image
			for( int32 Height = 0; Height < Bitmap.rows; ++Height )
			{
				for( uint32 ByteIdx = 0; ByteIdx < BytesPerLine; ++ByteIdx )
				{
					// The current byte
					uint8 Byte = Bitmap.buffer[ Height*BytesPerLine + ByteIdx ];

					// We may not need to use all the bits in a byte.  If there are more than 8 pixels left to check, check all 8
					// otherwise check whats left
					uint8 NumBitsToCheck = FMath::Min<uint8>(Bitmap.width-ByteIdx*8, 8);

					// Iterate through each bit in this byte.
					// Since each byte has up to 8 pixels, we need to generate an uint8 for each bit
					for( int32 Bit = 0; Bit < NumBitsToCheck; ++Bit )
					{
						// Most significant bit is left most bit
						if( ( Byte & ( 1 << (7-Bit) ) ) == 0 )
						{
							// Bit not set, pixel is black
							OutRenderData.RawPixels[ Height * Bitmap.width + Bit + 8*ByteIdx ] = 0;
						}
						else
						{
							// Bit set, pixel is white
							OutRenderData.RawPixels[ Height * Bitmap.width + Bit + 8*ByteIdx ] = 255;
						}
					}
				}
			}
		}
		else
		{
			// Copy the rendered bitmap to our raw pixels array
			for( int32 Row = 0; Row < Bitmap.rows; ++Row )
			{
				// Copy a single row. Note Bitmap.pitch contains the offset (in bytes) between rows.  Not always equal to Bitmap.width!
				FMemory::Memcpy( &OutRenderData.RawPixels[Row*Bitmap.width], &Bitmap.buffer[Row*Bitmap.pitch], Bitmap.width*GlyphPixelSize );
			}
		}

		FT_BBox GlyphBox;
		FT_Glyph Glyph;
		FT_Get_Glyph( Slot, &Glyph );
		FT_Glyph_Get_CBox( Glyph, FT_GLYPH_BBOX_PIXELS, &GlyphBox );

		int32 Height = (FT_MulFix( FontFace->height, FontFace->size->metrics.y_scale ) / 64) * Scale;

		// Set measurement info for this character
		OutRenderData.Char = Char;
		OutRenderData.MeasureInfo.SizeX = Bitmap.width;
		OutRenderData.MeasureInfo.SizeY = Bitmap.rows;
		OutRenderData.MaxHeight = Height;

		// Need to divide by 64 to get pixels;
		// Descender is not scaled by freetype.  Scale it now. 
		OutRenderData.MeasureInfo.GlobalDescender = (FontFace->size->metrics.descender / 64) * Scale;
		// Note we use Slot->advance instead of Slot->metrics.horiAdvance because Slot->Advance contains transformed position (needed if we scale)
		OutRenderData.MeasureInfo.XAdvance =  Slot->advance.x / 64;
		OutRenderData.MeasureInfo.HorizontalOffset = Slot->bitmap_left;
		OutRenderData.MeasureInfo.VerticalOffset = Slot->bitmap_top;

		FT_Done_Glyph( Glyph );
#endif // WITH_FREETYPE
	}

	/**
	 * @param Whether or not the font has kerning
	 */
	bool HasKerning( const FSlateFontInfo& InFontInfo )
	{
#if WITH_FREETYPE
		FT_Face FontFace = GetFontFace( InFontInfo.FontName );

		if ( FontFace == NULL )
		{
			return false;
		}

		return FT_HAS_KERNING( FontFace ) != 0;
#else
		return false;
#endif // WITH_FREETYPE
	}

	/**
	 * Calculates the kerning amount for a pair of characters
	 *
	 * @param First			The first character in the pair
	 * @param Second		The second character in the pair
 	 * @param InFontInfo	Information about the font that used to draw the string with the first and second characters
	 * @return The kerning amount, 0 if no kerning
	 */
	int8 GetKerning( TCHAR First, TCHAR Second, const FSlateFontKey& FontKey )
	{
#if WITH_FREETYPE
		int32 Kerning = 0;
		int32* FoundKerning = NULL;

		FT_Face FontFace = GetFontFace( FontKey.FontInfo.FontName );

		// Check if this font has kerning.  Not all fonts do.
		if( FontFace != NULL && FT_HAS_KERNING( FontFace ) )
		{
			FT_Error Error = FT_Set_Char_Size( FontFace, 0, FontKey.FontInfo.Size*64, FontCacheConstants::HorizontalDPI, FontCacheConstants::VerticalDPI  );

			if( FontKey.Scale != 1.0f )
			{
				FT_Matrix ScaleMatrix;
				ScaleMatrix.xy = 0;
				ScaleMatrix.xx = (FT_Fixed)(FontKey.Scale * 65536);
				ScaleMatrix.yy = (FT_Fixed)(FontKey.Scale * 65536);
				ScaleMatrix.yx = 0;
				FT_Set_Transform( FontFace, &ScaleMatrix, NULL );
			}
			else
			{
				FT_Set_Transform( FontFace, NULL, NULL );
			}

			check(Error==0);
		
			int32 KernValue = 0;
		
			FT_UInt FirstIndex = FT_Get_Char_Index( FontFace, First );
			FT_UInt SecondIndex = FT_Get_Char_Index( FontFace, Second );

			FT_Vector KerningVec;
			FT_Get_Kerning( FontFace, FirstIndex, SecondIndex, FT_KERNING_DEFAULT, &KerningVec );

			// Return pixel sizes
			int32 NewKerning = KerningVec.x / 64;

			// Set this pair's kerning value
			//KeringPairMap->Add( Pair, NewKerning );

			Kerning = NewKerning;
		}

		return Kerning;
#else
		return 0;
#endif // WITH_FREETYPE
	}

private:

#if WITH_FREETYPE
	/**
	 * Gets or loads a freetype font face
	 *
	 * @param The name of the font to load
	 */
	FT_Face GetFontFace( const FName& FontName )
	{
		FFontFaceAndMemory* FaceAndMemory = FontFaceMap.Find( FontName );
		if (!FaceAndMemory)
		{
			// make a new entry
			FaceAndMemory = &FontFaceMap.Add(FontName, FFontFaceAndMemory());

			// load the font via UE4 methods, so that it will route through all proper file management (network file loading, etc)
			FString FontPath = FontName.ToString();

			// default to error condition
			bool Error = true;

			int64 FileSize = IFileManager::Get().FileSize(*FontPath);
			if ( FileSize > 0 )
			{
				// allocate space for the font
				FaceAndMemory->Memory = (uint8*)FMemory::Malloc((uint32)FileSize);
				FArchive* Ar = IFileManager::Get().CreateFileReader(*FontPath);
				if (Ar != NULL)
				{
					// read in the file
					Ar->Serialize( FaceAndMemory->Memory, FileSize );
					delete Ar;

					// initialize the font, setting the error code
					Error = FT_New_Memory_Face( FTLibrary, FaceAndMemory->Memory, (FT_Long)FileSize, 0, &FaceAndMemory->Face ) != 0;
				}

				// if it failed, we don't want to keep the memory around
				if ( Error )
				{
					FMemory::Free(FaceAndMemory->Memory);
					FontFaceMap.Remove(FontName);
					FaceAndMemory = NULL;
				}
			}

			if ( Error )
			{
				UE_LOG( LogSlate, Warning, TEXT("GetFontFace failed to load or process '%s'"), *FontPath);
				FontFaceMap.Remove(FontName);
				FaceAndMemory = NULL;
			}
		}

		if ( FaceAndMemory == NULL )
		{
			return NULL;
		}

		return FaceAndMemory->Face;
	}

private:
	struct FFontFaceAndMemory
	{
		// the FT2 object
		FT_Face Face;

		// the memory for the face (can't be a TArray as the FontFaceMap could be reallocated and copy it's members around)
		uint8* Memory;

		FFontFaceAndMemory()
			: Face(NULL)
			, Memory(NULL)
		{
		}
	};

	/** Mapping of font names to freetype faces */
	TMap<FName,FFontFaceAndMemory> FontFaceMap;
	/** Mapping of fonts to maps of kerning pairs */
	TMap<FSlateFontInfo, TMap<FKerningPair,int32> > FontToKerningPairMap; 
	/** Free type library interface */
	FT_Library FTLibrary;
	FT_Memory CustomMemory;
#endif // WITH_FREETYPE
};

FKerningTable::FKerningTable( const FSlateFontKey& InFont, const FSlateFontCache& InFontCache )
	: DirectAccessTable( NULL )
	, FontCache( InFontCache )
	, FontKey( InFont )
{
	bHasKerning = InFontCache.HasKerning( InFont.FontInfo );
}

FKerningTable::~FKerningTable()
{
	if( DirectAccessTable )
	{
		const uint32 DirectAccessSizeBytes = FontCacheConstants::DirectAccessSize*FontCacheConstants::DirectAccessSize*sizeof(int8);

		FMemory::Free( DirectAccessTable );

		DEC_MEMORY_STAT_BY( STAT_SlateFontKerningTableMemory, DirectAccessSizeBytes );
	}
	DEC_MEMORY_STAT_BY( STAT_SlateFontKerningTableMemory, MappedKerningPairs.GetAllocatedSize() );
}

int8 FKerningTable::GetKerning( TCHAR FirstChar, TCHAR SecondChar )
{
	int8 OutKerning = 0;

	if( bHasKerning && FirstChar < FontCacheConstants::DirectAccessSize && SecondChar < FontCacheConstants::DirectAccessSize )
	{
		// This character can be directly indexed

		if( !DirectAccessTable )
		{
			// Create the table now
			CreateDirectTable();
		}

		// Determine the index into the kerning table
		const uint32 Index = FirstChar*FontCacheConstants::DirectAccessSize+SecondChar;

		OutKerning = DirectAccessTable[Index];
		// If the kerning value hasn't been accessed yet, get the value from the cache now 
		if( OutKerning == MAX_int8 )
		{
			OutKerning = FontCache.GetKerning( FirstChar, SecondChar, FontKey );
			DirectAccessTable[Index] = OutKerning;
		}
	}
	else if( bHasKerning )
	{
		// Kerning is mapped
		FKerningPair KerningPair( FirstChar, SecondChar );

		int8* FoundKerning = MappedKerningPairs.Find( KerningPair );
		if( !FoundKerning )
		{
			OutKerning = FontCache.GetKerning( FirstChar, SecondChar, FontKey );

#if STATS
			const uint32 CurrentMemoryUsage = MappedKerningPairs.GetAllocatedSize();
#endif
			MappedKerningPairs.Add( KerningPair, OutKerning );
		
#if STATS
			uint32 NewMemoryUsage = MappedKerningPairs.GetAllocatedSize();
			if( NewMemoryUsage > CurrentMemoryUsage )
			{
				INC_MEMORY_STAT_BY( STAT_SlateFontKerningTableMemory, NewMemoryUsage-CurrentMemoryUsage );
			}
#endif
		}
	}

	return OutKerning;
}

void FKerningTable::CreateDirectTable()
{
	const uint32 DirectAccessSizeBytes = FontCacheConstants::DirectAccessSize*FontCacheConstants::DirectAccessSize*sizeof(int8);

	check( !DirectAccessTable )
	DirectAccessTable = (int8*)FMemory::Malloc(DirectAccessSizeBytes);
	// Invalidate all entries so we know when to get a new kerning value from the implementation
	FMemory::Memset( DirectAccessTable, MAX_int8, DirectAccessSizeBytes );
	INC_MEMORY_STAT_BY( STAT_SlateFontKerningTableMemory, DirectAccessSizeBytes );
}

FCharacterList::FCharacterList( const FSlateFontKey& InFontKey, const FSlateFontCache& InFontCache )
	: KerningTable( InFontKey, InFontCache )
	, FontKey( InFontKey )
	, FontCache( InFontCache )
	, MaxDirectIndexedEntries( FontCacheConstants::DirectAccessSize )
	, MaxHeight( 0 )
	, Baseline( 0 )
{


}

int8 FCharacterList::GetKerning( TCHAR First, TCHAR Second )
{
	return KerningTable.GetKerning( First, Second );
}

uint16 FCharacterList::GetMaxHeight() const
{
	if( MaxHeight == 0 )
	{
		MaxHeight = FontCache.GetMaxCharacterHeight( FontKey.FontInfo, FontKey.Scale );
	}

	return MaxHeight;
}

int16 FCharacterList::GetBaseline() const
{
	if( Baseline == 0 )
	{
		Baseline = FontCache.GetBaseline( FontKey.FontInfo, FontKey.Scale );
	}

	return Baseline;
}

FCharacterEntry& FCharacterList::CacheCharacter( TCHAR Character )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateFontCachingTime );

	FCharacterEntry NewEntry;
	bool bSuccess = FontCache.AddNewEntry( Character, FontKey, NewEntry );

	check( bSuccess );

	if( Character < MaxDirectIndexedEntries && NewEntry.IsValidEntry() )
	{
		DirectIndexEntries[ Character ] = NewEntry;
		return DirectIndexEntries[ Character ];
	}
	else
	{
		return MappedEntries.Add( Character, NewEntry );
	}
}

FSlateFontCache::FSlateFontCache( TSharedRef<ISlateFontAtlasFactory> InFontAtlasFactory )
	: FTInterface( new FFreeTypeInterface )
	, FontAtlasFactory( InFontAtlasFactory )
	, bFlushRequested( false )
{

}

FSlateFontCache::~FSlateFontCache()
{	

}

bool FSlateFontCache::AddNewEntry( TCHAR Character, const FSlateFontKey& InKey, FCharacterEntry& OutCharacterEntry ) const
{
	FCharacterRenderData RenderData;
	// Render the character 
	FTInterface->GetRenderData( InKey.FontInfo, Character, RenderData, InKey.Scale );	

	// the index of the atlas where the character is stored
	int32 AtlasIndex = 0;
	const FAtlasedTextureSlot* NewSlot = NULL;

	for( AtlasIndex = 0; AtlasIndex < FontAtlases.Num(); ++AtlasIndex ) 
	{
		// Add the character to the texture
		NewSlot = FontAtlases[AtlasIndex]->AddCharacter( RenderData );
		if( NewSlot )
		{
			break;
		}
	}

	if( !NewSlot )
	{
		TSharedRef<FSlateFontAtlas> FontAtlas = FontAtlasFactory->CreateFontAtlas();

		// Add the character to the texture
		NewSlot = FontAtlas->AddCharacter( RenderData );

		AtlasIndex = FontAtlases.Add( FontAtlas );

		INC_DWORD_STAT_BY( STAT_SlateNumFontAtlases, 1 );

		if( FontAtlases.Num() > 1 )
		{
			// There is more than one font atlas which means there is a lot of font data being cached
			// try to shrink it next time
			bFlushRequested = true;
		}
	}

	// If the new slot is null and we are not just measuring, then the atlas is full
	bool bSuccess = NewSlot != NULL;

	if( bSuccess )
	{
		OutCharacterEntry.StartU = NewSlot->X;
		OutCharacterEntry.StartV = NewSlot->Y;
		OutCharacterEntry.USize = NewSlot->Width;
		OutCharacterEntry.VSize = NewSlot->Height;
		OutCharacterEntry.TextureIndex = 0;
		OutCharacterEntry.XAdvance = RenderData.MeasureInfo.XAdvance;
		OutCharacterEntry.VerticalOffset = RenderData.MeasureInfo.VerticalOffset;
		OutCharacterEntry.GlobalDescender = RenderData.MeasureInfo.GlobalDescender;
		OutCharacterEntry.HorizontalOffset = RenderData.MeasureInfo.HorizontalOffset;
		OutCharacterEntry.TextureIndex = AtlasIndex;
		OutCharacterEntry.Valid = 1;
	}

	return bSuccess;
}

FCharacterList& FSlateFontCache::GetCharacterList( const FSlateFontInfo &InFontInfo, float FontScale ) const
{
	// Create a key for looking up each character
	FSlateFontKey FontKey( InFontInfo, FontScale );

	TSharedRef< class FCharacterList >* CachedCharacterList = FontToCharacterListCache.Find( FontKey );

	// Additional setup
	if( CachedCharacterList == NULL )
	{
		return FontToCharacterListCache.Add( FontKey, MakeShareable( new FCharacterList( FontKey, *this ) ) ).Get();
	}

	return CachedCharacterList->Get();
}

uint16 FSlateFontCache::GetMaxCharacterHeight( const FSlateFontInfo& InFontInfo, float FontScale ) const
{
	FCharacterRenderData NewRenderData;

	// Just render the null character 
	TCHAR Char = 0;
	// Render the character 
	FTInterface->GetRenderData( InFontInfo, Char, NewRenderData, FontScale );	

	return NewRenderData.MaxHeight;
}

uint16 FSlateFontCache::GetBaseline( const FSlateFontInfo& InFontInfo, float FontScale ) const
{
	FCharacterRenderData NewRenderData;

	// Just render the null character 
	TCHAR Char = 0;
	// Render the character 
	FTInterface->GetRenderData( InFontInfo, Char, NewRenderData, FontScale );	

	return NewRenderData.MeasureInfo.GlobalDescender;
}

int8 FSlateFontCache::GetKerning( TCHAR First, TCHAR Second, const FSlateFontKey& FontKey ) const 
{
	return FTInterface->GetKerning( First, Second, FontKey );
}

bool FSlateFontCache::HasKerning( const FSlateFontInfo& InFontInfo ) const
{
	return FTInterface->HasKerning( InFontInfo );
}

void FSlateFontCache::ConditionalFlushCache()
{
	if( bFlushRequested )
	{
		FlushCache();
		bFlushRequested = false;
	}
}

void FSlateFontCache::UpdateCache()
{
	for( int32 AtlasIndex = 0; AtlasIndex < FontAtlases.Num(); ++AtlasIndex ) 
	{
		FontAtlases[AtlasIndex]->ConditionalUpdateTexture();
	}
}

void FSlateFontCache::ReleaseResources()
{
	for( int32 AtlasIndex = 0; AtlasIndex < FontAtlases.Num(); ++AtlasIndex ) 
	{
		FontAtlases[AtlasIndex]->ReleaseResources();
	}
}

void FSlateFontCache::FlushCache() const
{
	FontToCharacterListCache.Empty();
	FTInterface->Flush();

	for( int32 AtlasIndex = 0; AtlasIndex < FontAtlases.Num(); ++AtlasIndex ) 
	{
		FontAtlases[AtlasIndex]->ReleaseResources();
	}

	// hack
	FSlateApplication::Get().GetRenderer()->FlushCommands();

	SET_DWORD_STAT( STAT_SlateNumFontAtlases, 0 );

	FontAtlases.Empty();

	UE_LOG( LogSlate, Verbose, TEXT("Slate font cache was flushed") );
}

