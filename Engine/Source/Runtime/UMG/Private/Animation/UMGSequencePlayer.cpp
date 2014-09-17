// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "UMGSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneBindings.h"
#include "MovieSceneInstance.h"
#include "MovieScene.h"
#include "WidgetAnimation.h"

UUMGSequencePlayer::UUMGSequencePlayer(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PlayerStatus = EMovieScenePlayerStatus::Stopped;
	TimeCursorPosition = 0.0f;
	Animation = nullptr;
}

void UUMGSequencePlayer::InitSequencePlayer( const UWidgetAnimation& InAnimation, UUserWidget& UserWidget )
{
	Animation = &InAnimation;

	UMovieScene* MovieScene = Animation->MovieScene;

	// Cache the time range of the sequence to determine when we stop
	TimeRange = MovieScene->GetTimeRange();

	RuntimeBindings = ConstructObject<UMovieSceneBindings>( UMovieSceneBindings::StaticClass(), this );
	RuntimeBindings->SetRootMovieScene( MovieScene );

	UWidgetTree* WidgetTree = UserWidget.WidgetTree;

	TMap<FGuid, TArray<UObject*> > GuidToRuntimeObjectMap;
	// Bind to Runtime Objects
	for (const FWidgetAnimationBinding& Binding : InAnimation.AnimationBindings)
	{
		UObject* FoundObject = Binding.FindRuntimeObject( *WidgetTree );

		if( FoundObject )
		{
			TArray<UObject*>& Objects = GuidToRuntimeObjectMap.FindOrAdd(Binding.AnimationGuid);
			Objects.Add(FoundObject);
		}
	}

	for( auto It = GuidToRuntimeObjectMap.CreateConstIterator(); It; ++It )
	{
		RuntimeBindings->AddBinding( It.Key(), It.Value() );
	}

}

void UUMGSequencePlayer::Tick(float DeltaTime)
{
	if ( PlayerStatus == EMovieScenePlayerStatus::Playing )
	{
		double LastTimePosition = TimeCursorPosition;

		TimeCursorPosition += DeltaTime;

		if (RootMovieSceneInstance.IsValid())
		{
			RootMovieSceneInstance->Update(TimeCursorPosition, LastTimePosition, *this);
		}

		bool bIsFinished = false;

		if ( TimeCursorPosition >= TimeRange.GetUpperBoundValue() )
		{
			bIsFinished = true;
		}

		if (bIsFinished)
		{

			PlayerStatus = EMovieScenePlayerStatus::Stopped;

			OnSequenceFinishedPlayingEvent.Broadcast( *this );

		}
	}
}

void UUMGSequencePlayer::Play()
{
	UMovieScene* MovieScene = Animation->MovieScene;

	RootMovieSceneInstance = MakeShareable( new FMovieSceneInstance( *MovieScene ) );

	RootMovieSceneInstance->RefreshInstance( *this );

	// @todo UMG Support user specified start times
	TimeCursorPosition = 0;

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

void UUMGSequencePlayer::GetRuntimeObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray< UObject* >& OutObjects ) const
{
	OutObjects = RuntimeBindings->FindBoundObjects( ObjectHandle );
}

EMovieScenePlayerStatus::Type UUMGSequencePlayer::GetPlaybackStatus() const
{
	return PlayerStatus;
}