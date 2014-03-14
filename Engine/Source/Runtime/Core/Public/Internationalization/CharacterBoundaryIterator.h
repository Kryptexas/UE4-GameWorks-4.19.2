// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class CORE_API FCharacterBoundaryIterator
{
public:
	FCharacterBoundaryIterator(const FText& Text);
	FCharacterBoundaryIterator(const FString& String);
	FCharacterBoundaryIterator(const TCHAR* const String, const int32 StringLength);

	int32 GetCurrentPosition() const;

	int32 ResetToBeginning();
	int32 ResetToEnd();

	int32 MoveToPrevious();
	int32 MoveToNext();
	int32 MoveToCandidateBefore(const int32 Index);
	int32 MoveToCandidateAfter(const int32 Index);
private:
#if UE_ENABLE_ICU
	class FImplementation;
	TSharedRef<FImplementation> Implementation;
#else
	const FString String;
	int32 CurrentPosition;
#endif
};