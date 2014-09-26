// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "BreakIterator.h"

#if UE_ENABLE_ICU
#include "ICUBreakIterator.h"

class FICUCharacterBoundaryIterator : public FICUBreakIterator
{
public:
	FICUCharacterBoundaryIterator();
};

FICUCharacterBoundaryIterator::FICUCharacterBoundaryIterator()
	: FICUBreakIterator(FICUBreakIteratorManager::Get().CreateCharacterBoundaryIterator())
{
}

TSharedRef<IBreakIterator> FBreakIterator::CreateCharacterBoundaryIterator()
{
	return MakeShareable(new FICUCharacterBoundaryIterator());
}

#endif
