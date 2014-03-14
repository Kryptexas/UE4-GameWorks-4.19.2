// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "LineBreakIterator.h"

#if !UE_ENABLE_ICU

FLineBreakIterator::FLineBreakIterator(const FText& Text)
	: String( Text.ToString() ), CurrentPosition(0)
{
}

FLineBreakIterator::FLineBreakIterator(const FString& InString)
	: String( InString ), CurrentPosition(0)
{
}

FLineBreakIterator::FLineBreakIterator(const TCHAR* const InString, const int32 StringLength)
	: String( StringLength, InString ), CurrentPosition(0)
{
}

int32 FLineBreakIterator::GetCurrentPosition() const
{
	return CurrentPosition;
}

int32 FLineBreakIterator::ResetToBeginning()
{
	return CurrentPosition = 0;
}

int32 FLineBreakIterator::ResetToEnd()
{
	return CurrentPosition = String.Len();
}

int32 FLineBreakIterator::MoveToPrevious()
{
	return MoveToCandidateBefore(CurrentPosition);
}

int32 FLineBreakIterator::MoveToNext()
{
	return MoveToCandidateAfter(CurrentPosition);
}

int32 FLineBreakIterator::MoveToCandidateBefore(const int32 Index)
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

int32 FLineBreakIterator::MoveToCandidateAfter(const int32 Index)
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