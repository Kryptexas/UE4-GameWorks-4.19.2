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

				float AnimDilationFactor = FMath::IsNearlyZero(AnimSection->GetAnimationDilationFactor()) ? 1.0f : AnimSection->GetAnimationDilationFactor();

				float AnimPosition = (Position - AnimSection->GetAnimationStartTime()) * AnimDilationFactor;

				// Looping if greater than first section
				const bool bLooping = Position > (AnimSection->GetAnimationStartTime() + (AnimSection->GetAnimationSequenceLength() / AnimDilationFactor));

				if (bLooping)
				{
					AnimPosition = FMath::Fmod(Position - AnimSection->GetAnimationStartTime(), AnimSection->GetAnimationSequenceLength() / AnimDilationFactor);
					AnimPosition *= AnimDilationFactor;
				}

				// Clamp to end of section
				AnimPosition = FMath::Clamp(AnimPosition, AnimPosition, AnimSection->GetEndTime());

				if (GIsEditor && !GWorld->HasBegunPlay())
				{
					AnimInterface->PreviewBeginAnimControl(nullptr);
					AnimInterface->PreviewSetAnimPosition(SlotName, ChannelIndex, AnimSequence, AnimPosition, true, false, 0.f);
				}
				else
				{
					AnimInterface->BeginAnimControl(nullptr);
					AnimInterface->SetAnimPosition(SlotName, ChannelIndex, AnimSequence, AnimPosition, true, false);
				}
			}
		}
	}
}
