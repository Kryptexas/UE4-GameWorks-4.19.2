// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetsPrivatePCH.h"
#include "MediaTextureResource.h"


/* UMediaTexture structors
 *****************************************************************************/

UMediaTexture::UMediaTexture( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, ClearColor(FLinearColor::Red)
	, MediaAsset(nullptr)
	, CurrentMediaAsset(nullptr)
	, VideoBuffer(MakeShareable(new FMediaSampleBuffer))
{
	NeverStream = true;

	UpdateResource();
}


UMediaTexture::~UMediaTexture( )
{
	if (VideoTrack.IsValid())
	{
		VideoTrack->RemoveSink(VideoBuffer);
	}
}


/* UMediaTexture interface
 *****************************************************************************/

void UMediaTexture::SetMediaAsset( UMediaAsset* InMediaAsset )
{
	MediaAsset = InMediaAsset;

	InitializeTrack();
}


/* UTexture  overrides
 *****************************************************************************/

FTextureResource* UMediaTexture::CreateResource( )
{
	return new FMediaTextureResource(this, VideoBuffer);
}


EMaterialValueType UMediaTexture::GetMaterialType( )
{
	return MCT_Texture2D;
}


float UMediaTexture::GetSurfaceWidth( ) const
{
	return CachedDimensions.X;
}


float UMediaTexture::GetSurfaceHeight( ) const
{
	return CachedDimensions.Y;
}


/* UObject  overrides
 *****************************************************************************/

void UMediaTexture::BeginDestroy( )
{
	Super::BeginDestroy();

	// synchronize with the rendering thread by inserting a fence
 	if (!ReleasePlayerFence)
 	{
 		ReleasePlayerFence = new FRenderCommandFence();
 	}

 	ReleasePlayerFence->BeginFence();
}


void UMediaTexture::FinishDestroy( )
{
	delete ReleasePlayerFence;
	ReleasePlayerFence = nullptr;

	Super::FinishDestroy();
}


FString UMediaTexture::GetDesc( )
{
	TSharedPtr<IMediaPlayer> MediaPlayer = GetMediaPlayer();

	if (!MediaPlayer.IsValid())
	{
		return FString();
	}

	return FString::Printf(TEXT("%dx%d [%s]"), CachedDimensions.X, CachedDimensions.Y, GPixelFormats[GetFormat()].Name);
}


SIZE_T UMediaTexture::GetResourceSize( EResourceSizeMode::Type Mode )
{
	return CachedDimensions.X * CachedDimensions.Y * 4;
}


bool UMediaTexture::IsReadyForFinishDestroy( )
{
	// ready to call FinishDestroy if the flushing fence has been hit
	return (Super::IsReadyForFinishDestroy() && ReleasePlayerFence && ReleasePlayerFence->IsFenceComplete());
}


void UMediaTexture::PostLoad( )
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ClassDefaultObject) && !GIsBuildMachine)
	{
		InitializeTrack();
	}
}


#if WITH_EDITOR

void UMediaTexture::PreEditChange( UProperty* PropertyAboutToChange )
{
	// this will release the FMediaTextureResource
	Super::PreEditChange(PropertyAboutToChange);

	FlushRenderingCommands();
}


void UMediaTexture::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	InitializeTrack();

	// this will recreate the FMediaTextureResource
	Super::PostEditChangeProperty(PropertyChangedEvent);
}


/* UMediaTexture implementation
 *****************************************************************************/

void UMediaTexture::InitializeTrack( )
{
	// assign new media asset
	if (CurrentMediaAsset != MediaAsset)
	{
		if (CurrentMediaAsset != nullptr)
		{
			IMediaPlayerPtr MediaPlayer = CurrentMediaAsset->GetMediaPlayer();

			if (MediaPlayer.IsValid())
			{
				MediaPlayer->OnClosing().RemoveAll(this);
				MediaPlayer->OnOpened().RemoveAll(this);
			}
		}

		CurrentMediaAsset = MediaAsset;

		if (MediaAsset != nullptr)
		{
			IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

			if (MediaPlayer.IsValid())
			{
				MediaPlayer->OnClosing().AddUObject(this, &UMediaTexture::HandleMediaPlayerClosing);
				MediaPlayer->OnOpened().AddUObject(this, &UMediaTexture::HandleMediaPlayerOpened);
			}
		}	
	}

	// disconnect from current track
	if (VideoTrack.IsValid())
	{
		VideoTrack->RemoveSink(VideoBuffer);
	}

	VideoTrack.Reset();

	// initialize from new track
	if (MediaAsset != nullptr)
	{
		IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

		if (MediaPlayer.IsValid())
		{
			VideoTrack = MediaPlayer->GetTrackSafe(VideoTrackIndex, EMediaTrackTypes::Video);
		}
	}

	if (VideoTrack.IsValid())
	{
		CachedDimensions = VideoTrack->GetVideoDetails().GetDimensions();
	}
	else
	{
		CachedDimensions = FIntPoint(ForceInit);
	}

	UpdateResource();

	// connect to new track
	if (VideoTrack.IsValid())
	{
		VideoTrack->AddSink(VideoBuffer);
	}
}


/* UMediaTexture callbacks
 *****************************************************************************/

void UMediaTexture::HandleMediaPlayerClosing( FString ClosingUrl )
{
	InitializeTrack();
}


void UMediaTexture::HandleMediaPlayerOpened( FString OpenedUrl )
{
	InitializeTrack();
}


#endif // WITH_EDITOR
