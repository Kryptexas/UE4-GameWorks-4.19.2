// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "ICUBreakIterator.h"

#if UE_ENABLE_ICU
#include "ICUTextCharacterIterator.h"

FICUBreakIterator::FICUBreakIterator(TSharedRef<icu::BreakIterator>&& InICUBreakIterator)
	: ICUBreakIterator(InICUBreakIterator)
{
}

void FICUBreakIterator::SetString(const FText& InText)
{
	ICUBreakIterator->adoptText(new FICUTextCharacterIterator(InText)); // ICUBreakIterator takes ownership of this instance
	ResetToBeginning();
}

void FICUBreakIterator::SetString(const FString& InString)
{
	ICUBreakIterator->adoptText(new FICUTextCharacterIterator(InString)); // ICUBreakIterator takes ownership of this instance
	ResetToBeginning();
}

void FICUBreakIterator::SetString(const TCHAR* const InString, const int32 InStringLength) 
{
	ICUBreakIterator->adoptText(new FICUTextCharacterIterator(InString, InStringLength)); // ICUBreakIterator takes ownership of this instance
	ResetToBeginning();
}

void FICUBreakIterator::ClearString()
{
	ICUBreakIterator->setText(icu::UnicodeString());
	ResetToBeginning();
}

int32 FICUBreakIterator::GetCurrentPosition() const
{
	return ICUBreakIterator->current();
}

int32 FICUBreakIterator::ResetToBeginning()
{
	return ICUBreakIterator->first();
}

int32 FICUBreakIterator::ResetToEnd()
{
	return ICUBreakIterator->last();
}

int32 FICUBreakIterator::MoveToPrevious()
{
	return ICUBreakIterator->previous();
}

int32 FICUBreakIterator::MoveToNext()
{
	return ICUBreakIterator->next();
}

int32 FICUBreakIterator::MoveToCandidateBefore(const int32 InIndex)
{
	return ICUBreakIterator->preceding(InIndex);
}

int32 FICUBreakIterator::MoveToCandidateAfter(const int32 InIndex)
{
	return ICUBreakIterator->following(InIndex);
}

#endif
