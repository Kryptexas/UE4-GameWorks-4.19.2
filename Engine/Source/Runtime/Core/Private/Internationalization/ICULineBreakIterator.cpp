// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "BreakIterator.h"

#if UE_ENABLE_ICU
#include "ICUBreakIterator.h"

class FICULineBreakIterator : public FICUBreakIterator
{
public:
	FICULineBreakIterator();

private:
	static TSharedRef<icu::BreakIterator> CreateInternalLineBreakIterator();
};

FICULineBreakIterator::FICULineBreakIterator()
	: FICUBreakIterator(CreateInternalLineBreakIterator())
{
}

TSharedRef<icu::BreakIterator> FICULineBreakIterator::CreateInternalLineBreakIterator()
{
	UErrorCode ICUStatus = U_ZERO_ERROR;
	return MakeShareable( icu::BreakIterator::createLineInstance(icu::Locale::getDefault(), ICUStatus));
}

TSharedRef<IBreakIterator> FBreakIterator::CreateLineBreakIterator()
{
	return MakeShareable(new FICULineBreakIterator());
}

#endif
