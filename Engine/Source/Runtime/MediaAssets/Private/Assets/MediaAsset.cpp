// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetsPrivatePCH.h"


/* UMediaAsset structors
 *****************************************************************************/

UMediaAsset::UMediaAsset( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, AutoPlay(false)
	, AutoPlayRate(1.0f)
	, Looping(true)
	, StreamMode(MASM_FromUrl)
	, MediaPlayer(nullptr)
{ }


/* UMediaAsset interface
 *****************************************************************************/

bool UMediaAsset::CanPause( ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->IsPlaying();
}


bool UMediaAsset::CanPlay( ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->IsReady();
}


FTimespan UMediaAsset::GetDuration( ) const
{
	if (MediaPlayer.IsValid())
	{
		return MediaPlayer->GetMediaInfo().GetDuration();
	}

	return FTimespan::Zero();
}


float UMediaAsset::GetRate( ) const
{
	if (MediaPlayer.IsValid())
	{
		return MediaPlayer->GetRate();
	}

	return 0.0f;
}


FTimespan UMediaAsset::GetTime( ) const
{
	if (MediaPlayer.IsValid())
	{
		return MediaPlayer->GetTime();
	}

	return FTimespan::Zero();
}


const FString& UMediaAsset::GetUrl( ) const
{
	return CurrentUrl;
}


bool UMediaAsset::IsLooping( ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->IsLooping();
}


bool UMediaAsset::IsPaused( ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->IsPaused();
}


bool UMediaAsset::IsPlaying( ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->IsPlaying();
}


bool UMediaAsset::IsStopped( ) const
{
	return !MediaPlayer.IsValid() || !MediaPlayer->IsReady();
}


bool UMediaAsset::OpenUrl( const FString& NewUrl )
{
	URL = NewUrl;
	InitializePlayer();

	return (CurrentUrl == NewUrl);
}


bool UMediaAsset::Pause( )
{
	return SetRate(0.0f);
}


bool UMediaAsset::Play( )
{
	return SetRate(1.0f);
}


bool UMediaAsset::Rewind( )
{
	return Seek(FTimespan::Zero());
}


bool UMediaAsset::SetLooping( bool InLooping )
{
	return MediaPlayer.IsValid() && MediaPlayer->SetLooping(InLooping);
}


bool UMediaAsset::SetRate( float Rate )
{
	return MediaPlayer.IsValid() && MediaPlayer->SetRate(Rate);
}


bool UMediaAsset::Seek( const FTimespan& InTime )
{
	return MediaPlayer.IsValid() && MediaPlayer->Seek(InTime);
}


bool UMediaAsset::SupportsRate( float Rate, bool Unthinned ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->GetMediaInfo().SupportsRate(Rate, Unthinned);
}


bool UMediaAsset::SupportsScrubbing( ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->GetMediaInfo().SupportsScrubbing();
}


bool UMediaAsset::SupportsSeeking( ) const
{
	return MediaPlayer.IsValid() && MediaPlayer->GetMediaInfo().SupportsSeeking();
}


/* UObject  overrides
 *****************************************************************************/

void UMediaAsset::BeginDestroy( )
{
	Super::BeginDestroy();

	if (MediaPlayer.IsValid())
	{
		MediaPlayer->Close();
		MediaPlayer.Reset();
	}
}


FString UMediaAsset::GetDesc( )
{
	if (MediaPlayer.IsValid())
	{
		return TEXT("UMediaAsset");
	}

	return FString();
}


void UMediaAsset::PostLoad( )
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ClassDefaultObject) && !GIsBuildMachine)
	{
		InitializePlayer();
	}
}


#if WITH_EDITOR

void UMediaAsset::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	InitializePlayer();
}

#endif


/* UMediaAsset implementation
 *****************************************************************************/

void UMediaAsset::InitializePlayer( )
{
	if (URL != CurrentUrl)
	{
		// close previous player
		CurrentUrl = FString();

		if (MediaPlayer.IsValid())
		{
			MediaPlayer->Close();
			MediaPlayer->OnClosing().RemoveAll(this);
			MediaPlayer->OnOpened().RemoveAll(this);
			MediaPlayer.Reset();
		}

		if (URL.IsEmpty())
		{
			return;
		}

		// create new player
		MediaPlayer = IMediaModule::Get().CreatePlayer(URL);

		if (!MediaPlayer.IsValid())
		{
			return;
		}

		MediaPlayer->OnClosing().AddUObject(this, &UMediaAsset::HandleMediaPlayerMediaClosing);
		MediaPlayer->OnOpened().AddUObject(this, &UMediaAsset::HandleMediaPlayerMediaOpened);

		// open the new media file
		bool OpenedSuccessfully = false;

		if (StreamMode == EMediaAssetStreamModes::MASM_FromUrl)
		{
			OpenedSuccessfully = MediaPlayer->Open(FPaths::ConvertRelativePathToFull(URL));
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

				OpenedSuccessfully = MediaPlayer->Open(MakeShareable(FileData), URL);
			}

			delete FileReader;
		}

		// finish initialization
		if (OpenedSuccessfully)
		{
			CurrentUrl = URL;
		}
	}

	if (!MediaPlayer.IsValid())
	{
		return;
	}

	// start playback, if desired
	MediaPlayer->SetLooping(Looping);
    
	if (AutoPlay)
	{
		MediaPlayer->SetRate(AutoPlayRate);
	}
}


/* UMediaAsset callbacks
 *****************************************************************************/

void UMediaAsset::HandleMediaPlayerMediaClosing( FString ClosedUrl )
{
	OnMediaClosing.Broadcast(ClosedUrl);
}


void UMediaAsset::HandleMediaPlayerMediaOpened( FString OpenedUrl )
{
	OnMediaOpened.Broadcast(OpenedUrl);
}
