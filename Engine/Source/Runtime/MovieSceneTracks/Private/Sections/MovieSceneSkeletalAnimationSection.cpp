// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSkeletalAnimationSection.h"


UMovieSceneSkeletalAnimationSection::UMovieSceneSkeletalAnimationSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	AnimSequence = nullptr;
	AnimationStartTime = 0.f;
	AnimationDilationFactor = 1.f;
}


void UMovieSceneSkeletalAnimationSection::MoveSection( float DeltaTime, TSet<FKeyHandle>& KeyHandles )
{
	Super::MoveSection(DeltaTime, KeyHandles);

	AnimationStartTime += DeltaTime;
}


void UMovieSceneSkeletalAnimationSection::DilateSection( float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles )
{
	AnimationStartTime = (AnimationStartTime - Origin) * DilationFactor + Origin;
	AnimationDilationFactor *= DilationFactor;

	Super::DilateSection(DilationFactor, Origin, KeyHandles);
}


void UMovieSceneSkeletalAnimationSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	// do nothing
}


void UMovieSceneSkeletalAnimationSection::GetSnapTimes(TArray<float>& OutSnapTimes, bool bGetSectionBorders) const
{
	Super::GetSnapTimes(OutSnapTimes, bGetSectionBorders);

	float CurrentTime = GetAnimationStartTime();
	while (CurrentTime <= GetEndTime() && !FMath::IsNearlyZero(GetAnimationDuration()))
	{
		if (CurrentTime >= GetStartTime())
		{
			OutSnapTimes.Add(CurrentTime);
		}

		CurrentTime += GetAnimationDuration();
	}
}
