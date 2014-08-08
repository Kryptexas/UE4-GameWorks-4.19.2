// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "BreakIterator.h"

#if UE_ENABLE_ICU
#include "ICUBreakIterator.h"

class FICUCharacterBoundaryIterator : public FICUBreakIterator
{
public:
	FICUCharacterBoundaryIterator();

private:
	static TSharedRef<icu::BreakIterator> CreateInternalGraphemeBreakIterator();
};

FICUCharacterBoundaryIterator::FICUCharacterBoundaryIterator()
	: FICUBreakIterator(CreateInternalGraphemeBreakIterator())
{
}

TSharedRef<icu::BreakIterator> FICUCharacterBoundaryIterator::CreateInternalGraphemeBreakIterator()
{
	UErrorCode ICUStatus = U_ZERO_ERROR;
	return MakeShareable(icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), ICUStatus));
}

TSharedRef<IBreakIterator> FBreakIterator::CreateCharacterBoundaryIterator()
{
	return MakeShareable(new FICUCharacterBoundaryIterator());
}

#endif
