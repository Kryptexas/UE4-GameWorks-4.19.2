// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSkeletalAnimationSection.h"


UMovieSceneSkeletalAnimationSection::UMovieSceneSkeletalAnimationSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	AnimSequence = nullptr;
	StartOffset = 0.f;
	EndOffset = 0.f;
	PlayRate = 1.f;
	bReverse = false;
}


void UMovieSceneSkeletalAnimationSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection(DeltaTime, KeyHandles);
}


void UMovieSceneSkeletalAnimationSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	PlayRate /= DilationFactor;

	Super::DilateSection(DilationFactor, Origin, KeyHandles);
}

UMovieSceneSection* UMovieSceneSkeletalAnimationSection::SplitSection(float SplitTime)
{
	float AnimPlayRate = FMath::IsNearlyZero(GetPlayRate()) ? 1.0f : GetPlayRate();
	float AnimPosition = (SplitTime - GetStartTime()) * AnimPlayRate;
	float SeqLength = GetSequenceLength() - (GetStartOffset() + GetEndOffset());

	float NewOffset = FMath::Fmod(AnimPosition, SeqLength);
	NewOffset += GetStartOffset();

	UMovieSceneSection* NewSection = Super::SplitSection(SplitTime);
	if (NewSection != nullptr)
	{
		UMovieSceneSkeletalAnimationSection* NewSkeletalSection = Cast<UMovieSceneSkeletalAnimationSection>(NewSection);
		NewSkeletalSection->SetStartOffset(NewOffset);
	}
	return NewSection;
}


void UMovieSceneSkeletalAnimationSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	// do nothing
}


void UMovieSceneSkeletalAnimationSection::GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const
{
	Super::GetSnapTimes(OutSnapTimes, bGetSectionBorders);

	float CurrentTime = GetStartTime();

	while (CurrentTime <= GetEndTime() && !FMath::IsNearlyZero(GetDuration()))
	{
		if (CurrentTime >= GetStartTime())
		{
			OutSnapTimes.Add(CurrentTime);
		}

		CurrentTime += GetDuration() - (StartOffset + EndOffset);
	}
}
