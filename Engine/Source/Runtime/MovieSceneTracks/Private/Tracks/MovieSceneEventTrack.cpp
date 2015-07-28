// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneEventSection.h"
#include "MovieSceneEventTrack.h"
#include "MovieSceneEventTrackInstance.h"


/* UMovieSceneEventTrack interface
 *****************************************************************************/

bool UMovieSceneEventTrack::AddKeyToSection(float Time, FName EventName)
{
	// @todo sequencer: gmp: add key to event section
	// should be inserted into sorted array
	// can sections overlap?
	return false;
}


void UMovieSceneEventTrack::TriggerEvents(TRange<float> TimeRange, bool Backwards)
{
	if ((!Backwards && !bFireEventsWhenForwards) ||
		(Backwards && !bFireEventsWhenBackwards))
	{
		return;
	}

	// @todo sequencer: gmp: implement event track triggering
	/*
	ULevel* Level = GetLevel();
	ALevelScriptActor* LevelScriptActor = Level->LevelScriptActor;

	if (LevelScriptActor == nullptr)
	{
		return;
	}

	// for each key...
	for ()
	{
		UFunction* EventFunction = LevelScriptActor->FindFunction(EventName);

		if (EventFunction != nullptr)
		{
			if(EventFunction->NumParms == 0)
			{
				LevelScriptActor->ProcessEvent(EventFunction, nullptr);
			}
			else
			{
				// @todo sequencer: gmp: add external log category for MovieScene
				//UE_LOG(LogMovieScene, Log, TEXT("NotifyEventTriggered: Function '%s' does not have zero parameters."), *EventFuncName.ToString());
			}
		}
		else
		{
			// @todo sequencer: gmp: add external log category for MovieScene
			//UE_LOG(LogMovieScene, Log, TEXT("NotifyEventTriggered: Unable to find function '%s'"), *EventFuncName.ToString());
		}
	}*/
}


/* UMovieSceneTrack interface
 *****************************************************************************/

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneEventTrack::CreateInstance()
{
	return MakeShareable(new FMovieSceneEventTrackInstance(*this));
}


UMovieSceneSection* UMovieSceneEventTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneEventSection::StaticClass(), NAME_None, RF_Transactional);
}


const TArray<UMovieSceneSection*>& UMovieSceneEventTrack::GetAllSections() const
{
	return Sections;
}


TRange<float> UMovieSceneEventTrack::GetSectionBoundaries() const
{
	TRange<float> SectionBoundaries = TRange<float>::Empty();

	for (auto& Section : Sections)
	{
		SectionBoundaries = TRange<float>::Hull(SectionBoundaries, Section->GetRange());
	}

	return SectionBoundaries;
}


FName UMovieSceneEventTrack::GetTrackName() const
{
	return TrackName;
}


bool UMovieSceneEventTrack::HasSection(UMovieSceneSection* Section) const
{
	return Sections.Contains(Section);
}


bool UMovieSceneEventTrack::IsEmpty() const
{
	return (Sections.Num() == 0);
}


void UMovieSceneEventTrack::RemoveAllAnimationData()
{
	Sections.Empty();
}


void UMovieSceneEventTrack::RemoveSection(UMovieSceneSection* Section)
{
	Sections.Remove(Section);
}
