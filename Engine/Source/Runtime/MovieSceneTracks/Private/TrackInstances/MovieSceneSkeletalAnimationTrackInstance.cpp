// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSkeletalAnimationTrack.h"
#include "MovieSceneSkeletalAnimationTrackInstance.h"
#include "MovieSceneSkeletalAnimationSection.h"
#include "IMovieScenePlayer.h"
#include "Matinee/MatineeAnimInterface.h"


FMovieSceneSkeletalAnimationTrackInstance::FMovieSceneSkeletalAnimationTrackInstance( UMovieSceneSkeletalAnimationTrack& InAnimationTrack )
{
	AnimationTrack = &InAnimationTrack;
}


FMovieSceneSkeletalAnimationTrackInstance::~FMovieSceneSkeletalAnimationTrackInstance()
{
	// @todo Sequencer Need to find some way to call PreviewFinishAnimControl (needs the runtime objects)
}


void FMovieSceneSkeletalAnimationTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	// @todo Sequencer gameplay update has a different code path than editor update for animation

	for (int32 i = 0; i < RuntimeObjects.Num(); ++i)
	{
		IMatineeAnimInterface* AnimInterface = Cast<IMatineeAnimInterface>(RuntimeObjects[i]);
		if (AnimInterface)
		{
			UMovieSceneSkeletalAnimationSection* AnimSection = Cast<UMovieSceneSkeletalAnimationSection>(AnimationTrack->GetAnimSectionAtTime(Position));
			if (AnimSection && AnimSection->IsActive())
			{
				int32 ChannelIndex = 0;
				FName SlotName = FName("AnimationSlot");
				UAnimSequence* AnimSequence = AnimSection->GetAnimSequence();
				float AnimPosition = FMath::Fmod(Position - AnimSection->GetAnimationStartTime(), AnimSection->GetAnimationDuration()) * AnimSection->GetAnimationDilationFactor();

				AnimInterface->PreviewBeginAnimControl(nullptr);
				AnimInterface->PreviewSetAnimPosition(SlotName, ChannelIndex, AnimSequence, AnimPosition, true, false, 0.f);
			}
		}
	}
}
