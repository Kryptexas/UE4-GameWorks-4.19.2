// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneCinematicShotTrack.h"
#include "MovieSceneCinematicShotTrackInstance.h"
#include "MovieSceneSubTrackInstance.h"
#include "MovieSceneSubSection.h"


/* FMovieSceneCinematicShotTrackInstance structors
 *****************************************************************************/

FMovieSceneCinematicShotTrackInstance::FMovieSceneCinematicShotTrackInstance(UMovieSceneCinematicShotTrack& InTrack)
	: FMovieSceneSubTrackInstance(InTrack)
{ }

/* IMovieSceneTrackInstance interface
 *****************************************************************************/

void FMovieSceneCinematicShotTrackInstance::ClearInstance(IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	FMovieSceneSubTrackInstance::ClearInstance(Player, SequenceInstance);

	Player.UpdateCameraCut(nullptr, nullptr);
}


void FMovieSceneCinematicShotTrackInstance::RefreshInstance(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	FMovieSceneSubTrackInstance::RefreshInstance(RuntimeObjects, Player, SequenceInstance);
}

	
void FMovieSceneCinematicShotTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	FMovieSceneSubTrackInstance::RestoreState(RuntimeObjects, Player, SequenceInstance);

	Player.UpdateCameraCut(nullptr, nullptr);

	//@todo Need to manage how this interacts with the fade track
	/*
	// Reset editor preview/fade
	EMovieSceneViewportParams ViewportParams;
	ViewportParams.SetWhichViewportParam = (EMovieSceneViewportParams::SetViewportParam)(EMovieSceneViewportParams::SVP_FadeAmount | EMovieSceneViewportParams::SVP_FadeColor);
	ViewportParams.FadeAmount = 0.f;
	ViewportParams.FadeColor = FLinearColor::Black;

	TMap<FViewportClient*, EMovieSceneViewportParams> ViewportParamsMap;
	Player.GetViewportSettings(ViewportParamsMap);
	for (auto ViewportParamsPair : ViewportParamsMap)
	{
		ViewportParamsMap[ViewportParamsPair.Key] = ViewportParams;
	}
	Player.SetViewportSettings(ViewportParamsMap);
	*/
}
	
void FMovieSceneCinematicShotTrackInstance::Update(EMovieSceneUpdateData& UpdateData, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance) 
{
	FMovieSceneSubTrackInstance::Update(UpdateData, RuntimeObjects, Player, SequenceInstance);

	const TArray<UMovieSceneSection*>& AllSections = SubTrack->GetAllSections();
	TArray<UMovieSceneSection*> TraversedSections = GetTraversedSectionsWithPreroll(AllSections, UpdateData.Position, UpdateData.LastPosition);

	if (TraversedSections.Num() == 0)
	{
		Player.UpdateCameraCut(nullptr, nullptr);
	}
	else
	{
		//@todo Need to manage how this interacts with the fade track
		/*
		for (auto Section : TraversedSections)
		{
			UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
			if (SubSection->GetSequence() == nullptr)
			{
				// Set editor preview/fade
				EMovieSceneViewportParams ViewportParams;
				ViewportParams.SetWhichViewportParam = (EMovieSceneViewportParams::SetViewportParam)(EMovieSceneViewportParams::SVP_FadeAmount | EMovieSceneViewportParams::SVP_FadeColor);
				ViewportParams.FadeAmount = 1.f;
				ViewportParams.FadeColor = FLinearColor::Black;

				TMap<FViewportClient*, EMovieSceneViewportParams> ViewportParamsMap;
				Player.GetViewportSettings(ViewportParamsMap);
				for (auto ViewportParamsPair : ViewportParamsMap)
				{
					ViewportParamsMap[ViewportParamsPair.Key] = ViewportParams;
				}
				Player.SetViewportSettings(ViewportParamsMap);

				// Set runtime fade
				for (const FWorldContext& Context : GEngine->GetWorldContexts())
				{
					if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
					{
						UWorld* World = Context.World();
						if (World != nullptr)
						{
							APlayerController* PlayerController = World->GetGameInstance()->GetFirstLocalPlayerController();
							if (PlayerController != nullptr && PlayerController->PlayerCameraManager && !PlayerController->PlayerCameraManager->IsPendingKill())
							{
								PlayerController->PlayerCameraManager->SetManualCameraFade(1.f, FLinearColor::Black, false);
							}
						}
					}
				}

				break;
			}
		}
		*/
	}
}