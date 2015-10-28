// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSkeletalAnimationSection.h"
#include "MovieSceneSkeletalAnimationTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneSkeletalAnimationTrackInstance.h"


#define LOCTEXT_NAMESPACE "MovieSceneSkeletalAnimationTrack"


UMovieSceneSkeletalAnimationTrack::UMovieSceneSkeletalAnimationTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


FName UMovieSceneSkeletalAnimationTrack::GetTrackName() const
{
	return FName("Animation");
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneSkeletalAnimationTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneSkeletalAnimationTrackInstance( *this ) ); 
}


const TArray<UMovieSceneSection*>& UMovieSceneSkeletalAnimationTrack::GetAllSections() const
{
	return AnimationSections;
}


void UMovieSceneSkeletalAnimationTrack::RemoveAllAnimationData()
{
	// do nothing
}


bool UMovieSceneSkeletalAnimationTrack::HasSection(const UMovieSceneSection& Section ) const
{
	return AnimationSections.Contains(&Section);
}


void UMovieSceneSkeletalAnimationTrack::AddSection(UMovieSceneSection& Section)
{
	AnimationSections.Add(&Section);
}


void UMovieSceneSkeletalAnimationTrack::RemoveSection(UMovieSceneSection& Section)
{
	AnimationSections.Remove(&Section);
}


bool UMovieSceneSkeletalAnimationTrack::IsEmpty() const
{
	return AnimationSections.Num() == 0;
}


TRange<float> UMovieSceneSkeletalAnimationTrack::GetSectionBoundaries() const
{
	TArray<TRange<float>> Bounds;

	for (auto Section : AnimationSections)
	{
		Bounds.Add(Section->GetRange());
	}

	return TRange<float>::Hull(Bounds);
}


void UMovieSceneSkeletalAnimationTrack::AddNewAnimation(float KeyTime, UAnimSequence* AnimSequence)
{
	UMovieSceneSkeletalAnimationSection* NewSection = NewObject<UMovieSceneSkeletalAnimationSection>(this);
	{
		NewSection->InitialPlacement(AnimationSections, KeyTime, KeyTime + AnimSequence->SequenceLength, SupportsMultipleRows());
		NewSection->SetAnimSequence(AnimSequence);
	}

	AddSection(*NewSection);
}


UMovieSceneSection* UMovieSceneSkeletalAnimationTrack::GetAnimSectionAtTime(float Time)
{
	for (auto Section : AnimationSections)
	{
		if (Section->IsTimeWithinSection(Time))
		{
			return Section;
		}
	}

	return nullptr;
}


#undef LOCTEXT_NAMESPACE
