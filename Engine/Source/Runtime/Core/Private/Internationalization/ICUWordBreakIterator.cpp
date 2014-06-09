// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "WordBreakIterator.h"

#if UE_ENABLE_ICU
#include "ICUUtilities.h"
#include <unicode/brkiter.h>

namespace
{
	TSharedRef<icu::BreakIterator> CreateWordBreakIterator()
	{
		UErrorCode ICUStatus = U_ZERO_ERROR;
		return MakeShareable( icu::BreakIterator::createWordInstance( icu::Locale::getDefault(), ICUStatus ) );
	}
}

class FWordBreakIterator::FImplementation
{
public:
	FImplementation(const TCHAR* const String, const int32 StringLength)
		: ICUBreakIterator(CreateWordBreakIterator())
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

FWordBreakIterator::FWordBreakIterator(const FText& Text)
	: Implementation( new FImplementation( *Text.ToString(), Text.ToString().Len() ) )
{
}

FWordBreakIterator::FWordBreakIterator(const FString& String)
	: Implementation( new FImplementation( *String, String.Len() ) )
{
}

FWordBreakIterator::FWordBreakIterator(const TCHAR* const String, const int32 StringLength)
	: Implementation( new FImplementation( String, StringLength ) )
{
}

int32 FWordBreakIterator::GetCurrentPosition() const
{
	return Implementation->GetCurrentPosition();
}

int32 FWordBreakIterator::ResetToBeginning()
{
	return Implementation->ResetToBeginning();
}

int32 FWordBreakIterator::ResetToEnd()
{
	return Implementation->ResetToEnd();
}

int32 FWordBreakIterator::MoveToPrevious()
{
	return Implementation->MoveToPrevious();
}

int32 FWordBreakIterator::MoveToNext()
{
	return Implementation->MoveToNext();
}

int32 FWordBreakIterator::MoveToCandidateBefore(const int32 Index)
{
	return Implementation->MoveToCandidateBefore(Index);
}

int32 FWordBreakIterator::MoveToCandidateAfter(const int32 Index)
{
	return Implementation->MoveToCandidateAfter(Index);
}

#endif