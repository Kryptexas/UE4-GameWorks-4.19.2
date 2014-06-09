// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "WordBreakIterator.h"

#if !UE_ENABLE_ICU

FWordBreakIterator::FWordBreakIterator(const FText& Text)
	: String( Text.ToString() ), CurrentPosition(0)
{
}

FWordBreakIterator::FWordBreakIterator(const FString& InString)
	: String( InString ), CurrentPosition(0)
{
}

FWordBreakIterator::FWordBreakIterator(const TCHAR* const InString, const int32 StringLength)
	: String( StringLength, InString ), CurrentPosition(0)
{
}

int32 FWordBreakIterator::GetCurrentPosition() const
{
	return CurrentPosition;
}

int32 FWordBreakIterator::ResetToBeginning()
{
	return CurrentPosition = 0;
}

int32 FWordBreakIterator::ResetToEnd()
{
	return CurrentPosition = String.Len();
}

int32 FWordBreakIterator::MoveToPrevious()
{
	return MoveToCandidateBefore(CurrentPosition);
}

int32 FWordBreakIterator::MoveToNext()
{
	return MoveToCandidateAfter(CurrentPosition);
}

int32 FWordBreakIterator::MoveToCandidateBefore(const int32 Index)
{
	// Can break between char and whitespace.
	for(CurrentPosition = FMath::Clamp( Index, 0, String.Len() ) - 1; CurrentPosition >= 0 + 1; --CurrentPosition)
	{
		bool bPreviousCharWasWhitespace = FChar::IsWhitespace(String[CurrentPosition - 1]);
		bool bCurrentCharIsWhiteSpace = FChar::IsWhitespace(String[CurrentPosition]);
		if(bPreviousCharWasWhitespace != bCurrentCharIsWhiteSpace)
		{
			break;
		}
	}

	return CurrentPosition >= Index ? -1 : CurrentPosition;
}

int32 FWordBreakIterator::MoveToCandidateAfter(const int32 Index)
{
	// Can break between char and whitespace.
	for(CurrentPosition = FMath::Clamp( Index, 0, String.Len() ) + 1; CurrentPosition <= String.Len() - 1; ++CurrentPosition)
	{
		bool bPreviousCharWasWhitespace = FChar::IsWhitespace(String[CurrentPosition - 1]);
		bool bCurrentCharIsWhiteSpace = FChar::IsWhitespace(String[CurrentPosition]);
		if(bPreviousCharWasWhitespace != bCurrentCharIsWhiteSpace)
		{
			break;
		}
	}

	return CurrentPosition <= Index ? -1 : CurrentPosition;
}

#endif