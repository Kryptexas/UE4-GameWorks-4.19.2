// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneMediaData.h"

#include "IMediaEventSink.h"
#include "MediaPlayer.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"


/* FMediaSectionData structors
 *****************************************************************************/

FMovieSceneMediaData::FMovieSceneMediaData()
	: MediaPlayer(nullptr)
	, SeekOnOpenTime(FTimespan::MinValue())
{ }


FMovieSceneMediaData::~FMovieSceneMediaData()
{
	if (MediaPlayer != nullptr)
	{
		MediaPlayer->OnMediaEvent().RemoveAll(this);
		MediaPlayer->Close();
		MediaPlayer->RemoveFromRoot();
	}
}


/* FMediaSectionData interface
 *****************************************************************************/

UMediaPlayer* FMovieSceneMediaData::GetMediaPlayer()
{
	return MediaPlayer;
}


void FMovieSceneMediaData::SeekOnOpen(FTimespan Time)
{
	SeekOnOpenTime = Time;
}


void FMovieSceneMediaData::Setup()
{
	if (MediaPlayer == nullptr)
	{
		MediaPlayer = NewObject<UMediaPlayer>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UMediaPlayer::StaticClass()));
		MediaPlayer->OnMediaEvent().AddRaw(this, &FMovieSceneMediaData::HandleMediaPlayerEvent);
		MediaPlayer->PlayOnOpen = false;
		MediaPlayer->AddToRoot();
	}
}


/* FMediaSectionData callbacks
 *****************************************************************************/

void FMovieSceneMediaData::HandleMediaPlayerEvent(EMediaEvent Event)
{
	if ((Event != EMediaEvent::MediaOpened) || (SeekOnOpenTime < FTimespan::Zero()))
	{
		return;
	}

	const FTimespan MediaTime = SeekOnOpenTime % MediaPlayer->GetDuration();

	MediaPlayer->SetRate(0.0f);
	MediaPlayer->Seek(MediaTime);

	SeekOnOpenTime = FTimespan::MinValue();
}
