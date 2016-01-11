// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "UMGSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneBinding.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSequenceInstance.h"
#include "MovieScene.h"
#include "WidgetAnimation.h"


UUMGSequencePlayer::UUMGSequencePlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerStatus = EMovieScenePlayerStatus::Stopped;
	TimeCursorPosition = 0.0f;
	Animation = nullptr;
}

void UUMGSequencePlayer::InitSequencePlayer( const UWidgetAnimation& InAnimation, UUserWidget& UserWidget )
{
	Animation = &InAnimation;

	UMovieScene* MovieScene = Animation->GetMovieScene();

	// Cache the time range of the sequence to determine when we stop
	// Get the range of all sections combined
	TArray< TRange<float> > Bounds;

	for (int32 TypeIndex = 0; TypeIndex < MovieScene->GetMasterTracks().Num(); ++TypeIndex)
	{
		Bounds.Add(MovieScene->GetMasterTracks()[TypeIndex]->GetSectionBoundaries());
	}

	for (int32 BindingIndex = 0; BindingIndex < MovieScene->GetBindings().Num(); ++BindingIndex)
	{
		Bounds.Add(MovieScene->GetBindings()[BindingIndex].GetTimeRange());
	}

	TimeRange = TRange<float>::Hull(Bounds);

	UWidgetTree* WidgetTree = UserWidget.WidgetTree;

	// Bind to Runtime Objects
	for (const FWidgetAnimationBinding& Binding : InAnimation.GetBindings())
	{
		UObject* FoundObject = Binding.FindRuntimeObject( *WidgetTree );

		if( FoundObject )
		{
			TArray<UObject*>& Objects = GuidToRuntimeObjectMap.FindOrAdd(Binding.AnimationGuid);
			Objects.Add(FoundObject);
		}
	}
}

void UUMGSequencePlayer::Tick(float DeltaTime)
{
	if ( PlayerStatus == EMovieScenePlayerStatus::Playing )
	{
		double LastTimePosition = TimeCursorPosition;

		TimeCursorPosition += bIsPlayingForward ? DeltaTime : -DeltaTime;

		float AnimationLength = TimeRange.GetUpperBoundValue();
		if ( TimeCursorPosition < 0 )
		{
			NumLoopsCompleted++;
			if (NumLoopsToPlay != 0 && NumLoopsCompleted >= NumLoopsToPlay)
			{
				TimeCursorPosition = 0;
				PlayerStatus = EMovieScenePlayerStatus::Stopped;
				OnSequenceFinishedPlayingEvent.Broadcast(*this);
			}
			else
			{
				if (PlayMode == EUMGSequencePlayMode::PingPong)
				{
					bIsPlayingForward = !bIsPlayingForward;
					TimeCursorPosition = FMath::Abs(TimeCursorPosition);
				}
				else
				{
					TimeCursorPosition += AnimationLength;
				}
			}
		}
		else if ( TimeCursorPosition > AnimationLength )
		{
			NumLoopsCompleted++;
			if (NumLoopsToPlay != 0 && NumLoopsCompleted >= NumLoopsToPlay)
			{
				TimeCursorPosition = AnimationLength;
				PlayerStatus = EMovieScenePlayerStatus::Stopped;
				OnSequenceFinishedPlayingEvent.Broadcast(*this);
			}
			else
			{
				if (PlayMode == EUMGSequencePlayMode::PingPong)
				{
					bIsPlayingForward = !bIsPlayingForward;
					TimeCursorPosition = AnimationLength - (TimeCursorPosition - AnimationLength);
				}
				else
				{
					TimeCursorPosition -= AnimationLength;
				}
			}
		}

		if (RootMovieSceneInstance.IsValid())
		{
			RootMovieSceneInstance->Update(TimeCursorPosition, LastTimePosition, *this);
		}
	}
}

void UUMGSequencePlayer::Play(float StartAtTime, int32 InNumLoopsToPlay, EUMGSequencePlayMode::Type InPlayMode)
{
	RootMovieSceneInstance = MakeShareable( new FMovieSceneSequenceInstance( *Animation ) );
	RootMovieSceneInstance->RefreshInstance( *this );

	PlayMode = InPlayMode;

	// Clamp the start time to be between 0 and the upper time bound
	TimeCursorPosition = StaticCast<double>(FMath::Clamp(StartAtTime, 0.0f, TimeRange.GetUpperBoundValue()));

	if (PlayMode == EUMGSequencePlayMode::Reverse)
	{
		// When playing in reverse count substract the start time from the end.
		TimeCursorPosition = TimeRange.GetUpperBoundValue() - TimeCursorPosition;
	}

	if ( PlayMode == EUMGSequencePlayMode::PingPong )
	{
		// When animating in ping-pong mode double the number of loops to play so that a loop is a complete forward/reverse cycle.
		NumLoopsToPlay = 2 * InNumLoopsToPlay;
	}
	else
	{
		NumLoopsToPlay = InNumLoopsToPlay;
	}

	NumLoopsCompleted = 0;
	bIsPlayingForward = InPlayMode != EUMGSequencePlayMode::Reverse;

	PlayerStatus = EMovieScenePlayerStatus::Playing;
}

void UUMGSequencePlayer::Pause()
{
	PlayerStatus = EMovieScenePlayerStatus::Stopped;
}

void UUMGSequencePlayer::Stop()
{
	PlayerStatus = EMovieScenePlayerStatus::Stopped;

	OnSequenceFinishedPlayingEvent.Broadcast(*this);

	TimeCursorPosition = 0;
}

void UUMGSequencePlayer::GetRuntimeObjects( TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray< UObject* >& OutObjects ) const
{
	const TArray<UObject*>* FoundObjects = GuidToRuntimeObjectMap.Find( ObjectHandle );

	if( FoundObjects )
	{
		OutObjects.Append(*FoundObjects);
	}
	else
	{
		UE_LOG( LogUMG, Warning, TEXT("Failed to find runtime objects for %s animation"), Animation ? *Animation->GetPathName() : TEXT("(none)") );
	}
}

EMovieScenePlayerStatus::Type UUMGSequencePlayer::GetPlaybackStatus() const
{
	return PlayerStatus;
}