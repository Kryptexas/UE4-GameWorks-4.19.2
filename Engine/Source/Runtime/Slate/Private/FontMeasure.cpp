// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "FontMeasure.h"
#include "FontCache.h"
#include "FontCacheUtils.h"
#include "WordWrapper.h"

namespace FontMeasureConstants
{
	/** Number of possible elements in each measurement cache */
	const uint32 MeasureCacheSize = 500;
}

TSharedRef< FSlateFontMeasure > FSlateFontMeasure::Create( const TSharedRef<class FSlateFontCache>& FontCache )
{
	return MakeShareable( new FSlateFontMeasure( FontCache ) );
}

FSlateFontMeasure::FSlateFontMeasure( const TSharedRef<class FSlateFontCache>& InFontCache )
	: FontCache( InFontCache )
{

}

FVector2D FSlateFontMeasure::Measure( const FString& Text, const FSlateFontInfo& InFontInfo, float FontScale ) const
{
	int32 DummyLastCharacterIndex;
	return MeasureStringInternal( Text, 0, Text.Len(), InFontInfo, false, FontScale, INDEX_NONE, ELastCharacterIndexFormat::Unused, DummyLastCharacterIndex);
}

FVector2D FSlateFontMeasure::Measure( const FText& Text, const FSlateFontInfo& InFontInfo, float FontScale ) const
{
	int32 DummyLastCharacterIndex;
	const FString String = Text.ToString();
	return MeasureStringInternal( String, 0, String.Len(), InFontInfo, false, FontScale, INDEX_NONE, ELastCharacterIndexFormat::Unused, DummyLastCharacterIndex);
}

FVector2D FSlateFontMeasure::Measure( const FString& Text, int32 StartIndex, int32 EndIndex, const FSlateFontInfo& InFontInfo, bool IncludeKerningWithPrecedingChar, float FontScale ) const
{
	int32 DummyLastCharacterIndex;
	return MeasureStringInternal( Text, StartIndex, EndIndex, InFontInfo, IncludeKerningWithPrecedingChar, FontScale, INDEX_NONE, ELastCharacterIndexFormat::Unused, DummyLastCharacterIndex);
}

int32 FSlateFontMeasure::FindLastWholeCharacterIndexBeforeOffset( const FString& Text, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, float FontScale ) const
{
	return FindLastWholeCharacterIndexBeforeOffset( Text, 0, Text.Len(), InFontInfo, HorizontalOffset, false, FontScale );
}

int32 FSlateFontMeasure::FindLastWholeCharacterIndexBeforeOffset( const FText& Text, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, float FontScale ) const
{
	return FindLastWholeCharacterIndexBeforeOffset( Text.ToString(), InFontInfo, HorizontalOffset, FontScale );
}


int32 FSlateFontMeasure::FindLastWholeCharacterIndexBeforeOffset( const FString& Text, int32 StartIndex, int32 EndIndex, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, bool IncludeKerningWithPrecedingChar, float FontScale ) const
{
	int32 FoundLastCharacterIndex = INDEX_NONE;
	MeasureStringInternal( Text, StartIndex, EndIndex, InFontInfo, IncludeKerningWithPrecedingChar, FontScale, HorizontalOffset, ELastCharacterIndexFormat::LastWholeCharacterBeforeOffset, FoundLastCharacterIndex );
	return FoundLastCharacterIndex;
}


int32 FSlateFontMeasure::FindFirstWholeCharacterIndexAfterOffset( const FText& Text, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, float FontScale ) const
{
	return FindFirstWholeCharacterIndexAfterOffset( Text.ToString(), InFontInfo, HorizontalOffset, FontScale );
}

int32 FSlateFontMeasure::FindFirstWholeCharacterIndexAfterOffset( const FString& Text, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, float FontScale ) const
{
	return FindFirstWholeCharacterIndexAfterOffset( Text, 0, Text.Len(), InFontInfo, HorizontalOffset, false, FontScale );
}

int32 FSlateFontMeasure::FindFirstWholeCharacterIndexAfterOffset( const FString& Text, int32 StartIndex, int32 EndIndex, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, bool IncludeKerningWithPrecedingChar, float FontScale ) const
{
	int32 FoundLastCharacterIndex = FindCharacterIndexAtOffset( Text, StartIndex, EndIndex, InFontInfo, HorizontalOffset, IncludeKerningWithPrecedingChar, FontScale );

	float TextWidth = Measure( Text, StartIndex, EndIndex, InFontInfo, IncludeKerningWithPrecedingChar, FontScale ).X;
	float AvailableWidth = TextWidth - HorizontalOffset;

	float RightStringWidth = Measure( Text, FoundLastCharacterIndex, EndIndex, InFontInfo, IncludeKerningWithPrecedingChar, FontScale ).X;
	if ( AvailableWidth < RightStringWidth )
	{
		++FoundLastCharacterIndex;
	}

	return FoundLastCharacterIndex;
}

int32 FSlateFontMeasure::FindCharacterIndexAtOffset( const FString& Text, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, float FontScale ) const
{
	return FindCharacterIndexAtOffset( Text, 0, Text.Len(), InFontInfo, HorizontalOffset, false, FontScale );
}

int32 FSlateFontMeasure::FindCharacterIndexAtOffset( const FText& Text, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, float FontScale ) const
{
	return FindCharacterIndexAtOffset( Text.ToString(), InFontInfo, HorizontalOffset, FontScale );
}

int32 FSlateFontMeasure::FindCharacterIndexAtOffset( const FString& Text, int32 StartIndex, int32 EndIndex, const FSlateFontInfo& InFontInfo, const int32 HorizontalOffset, bool IncludeKerningWithPrecedingChar, float FontScale ) const
{
	int32 FoundLastCharacterIndex = 0;
	MeasureStringInternal( Text, StartIndex, EndIndex, InFontInfo, IncludeKerningWithPrecedingChar, FontScale, HorizontalOffset, ELastCharacterIndexFormat::CharacterAtOffset, FoundLastCharacterIndex );
	return FoundLastCharacterIndex;
}

FVector2D FSlateFontMeasure::MeasureStringInternal( const FString& Text, int32 StartIndex, int32 EndIndex, const FSlateFontInfo& InFontInfo, bool IncludeKerningWithPrecedingChar, float FontScale, int32 StopAfterHorizontalOffset, ELastCharacterIndexFormat CharIndexFormat, int32& OutLastCharacterIndex ) const
{
	SCOPE_CYCLE_COUNTER(STAT_SlateMeasureStringTime);
	FCharacterList& CharacterList = FontCache->GetCharacterList( InFontInfo, FontScale );
	const uint16 MaxHeight = CharacterList.GetMaxHeight();

	const bool DoesStartAtBeginning = StartIndex == 0;
	const bool DoesFinishAtEnd = EndIndex == Text.Len();
	const int32 TextRangeLength = EndIndex - StartIndex;
	if ( EndIndex - StartIndex <= 0 || EndIndex <= 0 || StartIndex < 0 || EndIndex <= StartIndex )
	{
		return FVector2D( 0, MaxHeight );
	}

#define USE_MEASURE_CACHING 1
#if USE_MEASURE_CACHING
	TSharedPtr< FMeasureCache > CurrentMeasureCache = NULL;
	// Do not cache strings which have small sizes or which have complicated measure requirements
	if( DoesStartAtBeginning && DoesFinishAtEnd && !IncludeKerningWithPrecedingChar && TextRangeLength > 5 && StopAfterHorizontalOffset == INDEX_NONE )
	{
		FSlateFontKey FontKey(InFontInfo,FontScale);
		TSharedPtr< FMeasureCache > FoundMeasureCache = FontToMeasureCache.FindRef( FontKey );

		if ( FoundMeasureCache.IsValid() )
		{
			CurrentMeasureCache = FoundMeasureCache;
			const FVector2D* CachedMeasurement = CurrentMeasureCache->AccessItem( Text );
			if( CachedMeasurement )
			{
				return *CachedMeasurement;
			}
		}
		else
		{
			CurrentMeasureCache = MakeShareable( new FMeasureCache( FontMeasureConstants::MeasureCacheSize ) );
			FontToMeasureCache.Add( FontKey, CurrentMeasureCache );
		}
	}
#endif

	// The size of the string
	FVector2D Size(0,0);
	// Widest line encountered while drawing this text.
	int32 MaxLineWidth = 0;
	// The width of the current line so far.
	int32 CurrentX = 0;
	// Accumulated height of this block of text
	int32 StringSizeY = MaxHeight;
	// The previous char (for kerning)
	TCHAR PreviousChar = 0;

	//If we are measuring a range then we should take into account the kerning with the character before the start of the range
	if ( !DoesStartAtBeginning && IncludeKerningWithPrecedingChar )
	{
		PreviousChar = Text[ StartIndex - 1 ];
	}

	int32 FinalPosX = 0;

	int32 CharIndex;
	for( CharIndex = StartIndex; CharIndex < EndIndex; ++CharIndex )
	{
		TCHAR CurrentChar = Text[CharIndex];

		const bool IsNewline = (CurrentChar == '\n');

		if (IsNewline)
		{
			// New line means
			// 1) we accumulate total height
			StringSizeY += MaxHeight;
			// 2) update the longest line we've encountered
			MaxLineWidth = FMath::Max(CurrentX, MaxLineWidth);
			// 3) the next line starts at the beginning
			CurrentX = 0;
		}
		else
		{
			const FCharacterEntry& Entry = CharacterList[CurrentChar];

			int32 Kerning = 0;
			if( CharIndex > 0 )
			{
				Kerning = CharacterList.GetKerning( PreviousChar, CurrentChar );
			}

			const int32 TotalCharSpacing = 
				Kerning + Entry.HorizontalOffset +		// Width is any kerning plus how much to advance the position when drawing a new character
				Entry.XAdvance;	// How far we advance 

			PreviousChar = CurrentChar;

			CurrentX += Kerning + Entry.XAdvance;

			// Were we asked to stop measuring after the specified horizontal offset in pixels?
			if( StopAfterHorizontalOffset != INDEX_NONE )
			{
				if( CharIndexFormat == ELastCharacterIndexFormat::CharacterAtOffset )
				{
					// Round our test toward the character's center position
					if( StopAfterHorizontalOffset < CurrentX - TotalCharSpacing / 2 )
					{
						// We've reached the stopping point, so bail
						break;
					}
				}
				else if( CharIndexFormat == ELastCharacterIndexFormat::LastWholeCharacterBeforeOffset )
				{
					if( StopAfterHorizontalOffset < CurrentX )
					{
						--CharIndex;
						if ( CharIndex < StartIndex )
						{
							CharIndex = INDEX_NONE;
						}

						// We've reached the stopping point, so bail
						break;
					}
				}
			}
		}
	}

	// We just finished a line, so need to update the longest line encountered.
	MaxLineWidth = FMath::Max(CurrentX, MaxLineWidth);

	Size.X = MaxLineWidth;
	Size.Y = StringSizeY;
	OutLastCharacterIndex = CharIndex;

#if USE_MEASURE_CACHING
	if( StopAfterHorizontalOffset == INDEX_NONE && CurrentMeasureCache.IsValid() )
	{
		CurrentMeasureCache->Add( Text, Size );
	}
#endif
	return Size;
}

uint16 FSlateFontMeasure::GetMaxCharacterHeight( const FSlateFontInfo& InFontInfo, float FontScale ) const
{
	FCharacterList& CharacterList = FontCache->GetCharacterList( InFontInfo, FontScale );

	const uint16 MaxHeight = CharacterList.GetMaxHeight();
	return MaxHeight;
}

uint8 FSlateFontMeasure::GetKerning( const FSlateFontInfo& InFontInfo, float FontScale, TCHAR PreviousCharacter, TCHAR CurrentCharacter ) const
{
	FCharacterList& CharacterList = FontCache->GetCharacterList( InFontInfo, FontScale );
	return CharacterList.GetKerning( PreviousCharacter, CurrentCharacter );
}

uint16 FSlateFontMeasure::GetBaseline( const FSlateFontInfo& InFontInfo, float FontScale ) const
{
	FCharacterList& CharacterList = FontCache->GetCharacterList( InFontInfo, FontScale );

	const uint16 Baseline = CharacterList.GetBaseline();
	return Baseline;
}

void FSlateFontMeasure::FlushCache()
{
	FontToMeasureCache.Empty();
}
