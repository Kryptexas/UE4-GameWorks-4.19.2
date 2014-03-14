// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "CharacterBoundaryIterator.h"

#if UE_ENABLE_ICU
#include "ICUUtilities.h"
#include <unicode/brkiter.h>

namespace
{
	TSharedRef<icu::BreakIterator> CreateGraphemeBreakIterator()
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( icu::BreakIterator::createCharacterInstance( icu::Locale::getDefault(), ICUStatus ) );
	}
}

class FCharacterBoundaryIterator::FImplementation
{
public:
	FImplementation(const TCHAR* const String, const int32 StringLength)
		: ICUBreakIterator( CreateGraphemeBreakIterator() )
	{
		ICUUtilities::Convert(FString(String, StringLength), ICUString);
		ICUBreakIterator->setText(ICUString);
	}

	int32 GetCurrentPosition() const
	{
		return ICUBreakIterator->current();
	}

	int32 ResetToBeginning()
	{
		return ICUBreakIterator->first();
	}

	int32 ResetToEnd()
	{
		return ICUBreakIterator->last();
	}

	int32 MoveToPrevious()
	{
		return ICUBreakIterator->previous();
	}

	int32 MoveToNext()
	{
		return ICUBreakIterator->next();
	}

	int32 MoveToCandidateBefore(const int32 Index)
	{
		return ICUBreakIterator->preceding(Index);
	}

	int32 MoveToCandidateAfter(const int32 Index)
	{
		return ICUBreakIterator->following(Index);
	}

private:
	TSharedRef<icu::BreakIterator> ICUBreakIterator;
	icu::UnicodeString ICUString;
};

FCharacterBoundaryIterator::FCharacterBoundaryIterator(const FText& Text)
	: Implementation( new FImplementation( *Text.ToString(), Text.ToString().Len() ) )
{
}

FCharacterBoundaryIterator::FCharacterBoundaryIterator(const FString& String)
	: Implementation( new FImplementation( *String, String.Len() ) )
{
}

FCharacterBoundaryIterator::FCharacterBoundaryIterator(const TCHAR* const String, const int32 StringLength)
	: Implementation( new FImplementation( String, StringLength ) )
{
}

int32 FCharacterBoundaryIterator::GetCurrentPosition() const
{
	return Implementation->GetCurrentPosition();
}

int32 FCharacterBoundaryIterator::ResetToBeginning()
{
	return Implementation->ResetToBeginning();
}

int32 FCharacterBoundaryIterator::ResetToEnd()
{
	return Implementation->ResetToEnd();
}

int32 FCharacterBoundaryIterator::MoveToPrevious()
{
	return Implementation->MoveToPrevious();
}

int32 FCharacterBoundaryIterator::MoveToNext()
{
	return Implementation->MoveToNext();
}

int32 FCharacterBoundaryIterator::MoveToCandidateBefore(const int32 Index)
{
	return Implementation->MoveToCandidateBefore(Index);
}

int32 FCharacterBoundaryIterator::MoveToCandidateAfter(const int32 Index)
{
	return Implementation->MoveToCandidateAfter(Index);
}

#endif