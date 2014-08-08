// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IBreakIterator.h"

#if UE_ENABLE_ICU

#include <unicode/brkiter.h>

/**
 * Wraps an ICU break iterator instance inside our own break iterator API
 */
class FICUBreakIterator : public IBreakIterator
{
public:
	FICUBreakIterator(TSharedRef<icu::BreakIterator>&& InICUBreakIterator);

	virtual void SetString(const FText& InText) override;
	virtual void SetString(const FString& InString) override;
	virtual void SetString(const TCHAR* const InString, const int32 InStringLength) override;
	virtual void ClearString() override;

	virtual int32 GetCurrentPosition() const override;

	virtual int32 ResetToBeginning() override;
	virtual int32 ResetToEnd() override;

	virtual int32 MoveToPrevious() override;
	virtual int32 MoveToNext() override;
	virtual int32 MoveToCandidateBefore(const int32 InIndex) override;
	virtual int32 MoveToCandidateAfter(const int32 InIndex) override;

protected:
	TSharedRef<icu::BreakIterator> ICUBreakIterator;
};

#endif
