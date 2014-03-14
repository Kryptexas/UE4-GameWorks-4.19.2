// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "CharacterBoundaryIterator.h"

#if !UE_ENABLE_ICU

FCharacterBoundaryIterator::FCharacterBoundaryIterator(const FText& Text)
	: String( Text.ToString() ), CurrentPosition(0)
{
}

FCharacterBoundaryIterator::FCharacterBoundaryIterator(const FString& InString)
	: String( InString ), CurrentPosition(0)
{
}

FCharacterBoundaryIterator::FCharacterBoundaryIterator(const TCHAR* const InString, const int32 StringLength)
	: String( StringLength, InString ), CurrentPosition(0)
{
}

int32 FCharacterBoundaryIterator::GetCurrentPosition() const
{
	return CurrentPosition;
}

int32 FCharacterBoundaryIterator::ResetToBeginning()
{
	return CurrentPosition = 0;
}

int32 FCharacterBoundaryIterator::ResetToEnd()
{
	return CurrentPosition = String.Len();
}

int32 FCharacterBoundaryIterator::MoveToPrevious()
{
	return MoveToCandidateBefore(CurrentPosition);
}

int32 FCharacterBoundaryIterator::MoveToNext()
{
	return MoveToCandidateAfter(CurrentPosition);
}

int32 FCharacterBoundaryIterator::MoveToCandidateBefore(const int32 Index)
{
	CurrentPosition = FMath::Clamp( Index - 1, 0, String.Len() );
	return CurrentPosition >= Index ? -1 : CurrentPosition;
}

int32 FCharacterBoundaryIterator::MoveToCandidateAfter(const int32 Index)
{
	CurrentPosition = FMath::Clamp( Index + 1, 0, String.Len() );
	return CurrentPosition <= Index ? -1 : CurrentPosition;
}

#endif