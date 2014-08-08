// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "BreakIterator.h"

#if UE_ENABLE_ICU
#include "ICUBreakIterator.h"

class FICUWordBreakIterator : public FICUBreakIterator
{
public:
	FICUWordBreakIterator();

private:
	static TSharedRef<icu::BreakIterator> CreateInternalWordBreakIterator();
};

FICUWordBreakIterator::FICUWordBreakIterator()
	: FICUBreakIterator(CreateInternalWordBreakIterator())
{
}

TSharedRef<icu::BreakIterator> FICUWordBreakIterator::CreateInternalWordBreakIterator()
{
	UErrorCode ICUStatus = U_ZERO_ERROR;
	return MakeShareable(icu::BreakIterator::createWordInstance(icu::Locale::getDefault(), ICUStatus));
}

TSharedRef<IBreakIterator> FBreakIterator::CreateWordBreakIterator()
{
	return MakeShareable(new FICUWordBreakIterator());
}

#endif
