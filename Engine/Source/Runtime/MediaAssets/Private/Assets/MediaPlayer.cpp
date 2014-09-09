// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetsPrivatePCH.h"


/* UMediaPlayer structors
 *****************************************************************************/

UMediaPlayer::UMediaPlayer( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, AutoPlay(false)
	, AutoPlayRate(1.0f)
	, Looping(true)
	, StreamMode(MASM_FromUrl)
	, Player(nullptr)
{ }


/* UMediaPlayer interface
 *****************************************************************************/

bool UMediaPlayer::CanPause( ) const
{
	return Player.IsValid() && Player->IsPlaying();
}


bool UMediaPlayer::CanPlay( ) const
{
	return Player.IsValid() && Player->IsReady();
}


FTimespan UMediaPlayer::GetDuration( ) const
{
	if (Player.IsValid())
	{
		return Player->GetMediaInfo().GetDuration();
	}

	return FTimespan::Zero();
}


float UMediaPlayer::GetRate( ) const
{
	if (Player.IsValid())
	{
		return Player->GetRate();
	}

	return 0.0f;
}


FTimespan UMediaPlayer::GetTime( ) const
{
	if (Player.IsValid())
	{
		return Player->GetTime();
	}

	return FTimespan::Zero();
}


const FString& UMediaPlayer::GetUrl( ) const
{
	return CurrentUrl;
}


bool UMediaPlayer::IsLooping( ) const
{
	return Player.IsValid() && Player->IsLooping();
}


bool UMediaPlayer::IsPaused( ) const
{
	return Player.IsValid() && Player->IsPaused();
}


bool UMediaPlayer::IsPlaying( ) const
{
	return Player.IsValid() && Player->IsPlaying();
}


bool UMediaPlayer::IsStopped( ) const
{
	return !Player.IsValid() || !Player->IsReady();
}


bool UMediaPlayer::OpenUrl( const FString& NewUrl )
{
	URL = NewUrl;
	InitializePlayer();

	return (CurrentUrl == NewUrl);
}


bool UMediaPlayer::Pause( )
{
	return SetRate(0.0f);
}


bool UMediaPlayer::Play( )
{
	return SetRate(1.0f);
}


bool UMediaPlayer::Rewind( )
{
	return Seek(FTimespan::Zero());
}


bool UMediaPlayer::SetLooping( bool InLooping )
{
	return Player.IsValid() && Player->SetLooping(InLooping);
}


bool UMediaPlayer::SetRate( float Rate )
{
	return Player.IsValid() && Player->SetRate(Rate);
}


bool UMediaPlayer::Seek( const FTimespan& InTime )
{
	return Player.IsValid() && Player->Seek(InTime);
}


bool UMediaPlayer::SupportsRate( float Rate, bool Unthinned ) const
{
	return Player.IsValid() && Player->GetMediaInfo().SupportsRate(Rate, Unthinned);
}


bool UMediaPlayer::SupportsScrubbing( ) const
{
	return Player.IsValid() && Player->GetMediaInfo().SupportsScrubbing();
}


bool UMediaPlayer::SupportsSeeking( ) const
{
	return Player.IsValid() && Player->GetMediaInfo().SupportsSeeking();
}


/* UObject  overrides
 *****************************************************************************/

void UMediaPlayer::BeginDestroy( )
{
	Super::BeginDestroy();

	if (Player.IsValid())
	{
		Player->Close();
		Player.Reset();
	}
}


FString UMediaPlayer::GetDesc( )
{
	if (Player.IsValid())
	{
		return TEXT("UMediaPlayer");
	}

	return FString();
}


void UMediaPlayer::PostLoad( )
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ClassDefaultObject) && !GIsBuildMachine)
	{
		InitializePlayer();
	}
}


#if WITH_EDITOR

void UMediaPlayer::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	InitializePlayer();
}

#endif


/* UMediaPlayer implementation
 *****************************************************************************/

void UMediaPlayer::InitializePlayer( )
{
	if (URL != CurrentUrl)
	{
		// close previous player
		CurrentUrl = FString();

		if (Player.IsValid())
		{
			Player->Close();
			Player->OnClosed().RemoveAll(this);
			Player->OnOpened().RemoveAll(this);
			Player.Reset();
		}

		if (URL.IsEmpty())
		{
			return;
		}

		// create new player
		Player = IMediaModule::Get().CreatePlayer(URL);

		if (!Player.IsValid())
		{
			return;
		}

		Player->OnClosed().AddUObject(this, &UMediaPlayer::HandleMediaPlayerMediaClosed);
		Player->OnOpened().AddUObject(this, &UMediaPlayer::HandleMediaPlayerMediaOpened);

		// open the new media file
		bool OpenedSuccessfully = false;

		if (StreamMode == EMediaPlayerStreamModes::MASM_FromUrl)
		{
			OpenedSuccessfully = Player->Open(FPaths::ConvertRelativePathToFull(URL));
		}
		else if (FPaths::FileExists(URL))
		{
			FArchive* FileReader = IFileManager::Get().CreateFileReader(*URL);
		
			if (FileReader == nullptr)
			{
				return;
			}

			if (FileReader->TotalSize() > 0)
			{
				TArray<uint8>* FileData = new TArray<uint8>();

				FileData->AddUninitialized(FileReader->TotalSize());
				FileReader->Serialize(FileData->GetData(), FileReader->TotalSize());

				OpenedSuccessfully = Player->Open(MakeShareable(FileData), URL);
			}

			delete FileReader;
		}

		// finish initialization
		if (OpenedSuccessfully)
		{
			CurrentUrl = URL;
		}
	}

	if (!Player.IsValid())
	{
		return;
	}

	// start playback, if desired
	Player->SetLooping(Looping);
    
	if (AutoPlay)
	{
		Player->SetRate(AutoPlayRate);
	}
}


/* UMediaPlayer callbacks
 *****************************************************************************/

void UMediaPlayer::HandleMediaPlayerMediaClosed( )
{
	MediaChangedEvent.Broadcast();
	OnMediaClosed.Broadcast();
}


void UMediaPlayer::HandleMediaPlayerMediaOpened( FString OpenedUrl )
{
	MediaChangedEvent.Broadcast();
	OnMediaOpened.Broadcast(OpenedUrl);
}
