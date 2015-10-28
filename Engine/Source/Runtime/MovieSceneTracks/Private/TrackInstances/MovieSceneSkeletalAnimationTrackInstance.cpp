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


void FMovieSceneSkeletalAnimationTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance ) 
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

				float AnimPlayRate = FMath::IsNearlyZero(AnimSection->GetPlayRate()) ? 1.0f : AnimSection->GetPlayRate();
				float AnimPosition = (Position - AnimSection->GetStartTime()) * AnimPlayRate;
				float SeqLength = AnimSection->GetSequenceLength() - (AnimSection->GetStartOffset() + AnimSection->GetEndOffset());

				AnimPosition = FMath::Fmod(AnimPosition, SeqLength);
				AnimPosition += AnimSection->GetStartOffset();

				// Clamp to end of section
				AnimPosition = FMath::Clamp(AnimPosition, AnimPosition, AnimSection->GetEndTime());

				if (AnimSection->GetReverse())
				{
					AnimPosition = (SeqLength - (AnimPosition - AnimSection->GetStartOffset())) + AnimSection->GetStartOffset();
				}

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
