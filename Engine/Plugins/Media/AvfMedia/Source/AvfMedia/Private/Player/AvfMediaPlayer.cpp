// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AvfMediaPrivatePCH.h"


/* FAvfMediaPlayer structors
 *****************************************************************************/

FAvfMediaPlayer::FAvfMediaPlayer()
{
    CurrentTime = FTimespan::Zero();
    Duration = FTimespan::Zero();
	MediaUrl = FString();
    
    PlayerItem = nil;
    MediaPlayer = nil;
}


FAvfMediaPlayer::~FAvfMediaPlayer( )
{
}


/* IMediaInfo interface
 *****************************************************************************/

FTimespan FAvfMediaPlayer::GetDuration( ) const
{
	return Duration;
}


TRange<float> FAvfMediaPlayer::GetSupportedRates( EMediaPlaybackDirections Direction, bool Unthinned ) const
{
	float MaxRate = 1.0f;
	float MinRate = 0.0f;
    
	return TRange<float>(MinRate, MaxRate);
}


FString FAvfMediaPlayer::GetUrl( ) const
{
	return MediaUrl;
}


bool FAvfMediaPlayer::SupportsRate( float Rate, bool Unthinned ) const
{
    return GetSupportedRates(EMediaPlaybackDirections::Forward, true).Contains( Rate );
}


bool FAvfMediaPlayer::SupportsScrubbing( ) const
{
	return false;
}


bool FAvfMediaPlayer::SupportsSeeking( ) const
{
	return false;
}


/* IMediaPlayer interface
 *****************************************************************************/

void FAvfMediaPlayer::Close( )
{
    CurrentTime = 0;
	MediaUrl = FString();
    if( PlayerItem != nil )
    {
        [PlayerItem release];
        PlayerItem = nil;
    }

    if( MediaPlayer != nil )
    {
        [MediaPlayer release];
        MediaPlayer = nil;
    }
	Tracks.Reset();
    
    Duration = CurrentTime = FTimespan::Zero();
    
    ClosedEvent.Broadcast();
}


const IMediaInfo& FAvfMediaPlayer::GetMediaInfo( ) const 
{
	return *this;
}


float FAvfMediaPlayer::GetRate( ) const
{
    return CurrentRate;
}


FTimespan FAvfMediaPlayer::GetTime( ) const 
{
    return CurrentTime;
}


const TArray<IMediaTrackRef>& FAvfMediaPlayer::GetTracks( ) const
{
	return Tracks;
}


bool FAvfMediaPlayer::IsLooping( ) const 
{
    return false;
}


bool FAvfMediaPlayer::IsPaused( ) const
{
    return FMath::IsNearlyZero( [MediaPlayer rate] );
}


bool FAvfMediaPlayer::IsPlaying( ) const
{
    return (MediaPlayer != nil) && (1.0f == MediaPlayer.rate) && (CurrentTime <= Duration);
}


bool FAvfMediaPlayer::IsReady( ) const
{
    // To be ready, we need the AVPlayer setup
    bool bIsReady = (MediaPlayer != nil) && ([MediaPlayer status] == AVPlayerStatusReadyToPlay);
    // A player item setup,
    bIsReady &= (PlayerItem != nil) && ([PlayerItem status] == AVPlayerItemStatusReadyToPlay);
    
    // and all tracks to be setup and ready
    for( const IMediaTrackRef& Track : Tracks )
    {
        FAvfMediaVideoTrack* AVFTrack = (FAvfMediaVideoTrack*)&Track.Get();
        if( AVFTrack != nil )
        {
            bIsReady &= AVFTrack->IsReady();
        }
    }
    
    return bIsReady;
}


/* FAvfMediaPlayer implementation
 *****************************************************************************/


bool FAvfMediaPlayer::Open( const FString& Url )
{
    Close();
    
    bool bSuccessfullyOpenedMovie = false;

    MediaUrl = Url;
    
    MediaPlayer = [[AVPlayer alloc] init];
    NSURL* nsMediaUrl = [NSURL fileURLWithPath:MediaUrl.GetNSString()];
    if( MediaPlayer )
    {
        PlayerItem = [AVPlayerItem playerItemWithURL: nsMediaUrl];
        if( PlayerItem != nil )
        {
            Duration = FTimespan::FromSeconds( CMTimeGetSeconds( PlayerItem.asset.duration ) );

            [[PlayerItem asset] loadValuesAsynchronouslyForKeys: @[@"tracks"] completionHandler:^
            {
                if( [[PlayerItem asset] statusOfValueForKey:@"tracks" error:nil] == AVKeyValueStatusLoaded )
                {
                    NSArray* VideoTracks = [[PlayerItem asset] tracksWithMediaType:AVMediaTypeVideo];
                    if( VideoTracks.count > 0 )
                    {
                        AVAssetTrack* VideoTrack = [VideoTracks objectAtIndex: 0];
                        Tracks.Add( MakeShareable( new FAvfMediaVideoTrack(VideoTrack) ) );
     
                        OpenedEvent.Broadcast( MediaUrl );
                    }
                }
            }];
            
            [MediaPlayer replaceCurrentItemWithPlayerItem: PlayerItem];
            
            [[MediaPlayer currentItem] seekToTime:kCMTimeZero];
            MediaPlayer.rate = 0.0;
            
            bSuccessfullyOpenedMovie = true;
        }
        else
        {
            UE_LOG(LogAvfMedia, Warning, TEXT("Failed to open player item with Url:"), *Url);
        }
    }
    else
    {
        UE_LOG(LogAvfMedia, Error, TEXT("Failed to create instance of an AVPlayer"));
    }
    
    
    return bSuccessfullyOpenedMovie;
}


bool FAvfMediaPlayer::Open( const TSharedRef<TArray<uint8>>& Buffer, const FString& OriginalUrl )
{
    return false;
}


bool FAvfMediaPlayer::Seek( const FTimespan& Time )
{
    // We need to find a suitable way to seek using avplayer and AVAssetReader,
/*
    CurrentTime = Time;

    [[MediaPlayer currentItem] seekToTime:CMTimeMakeWithSeconds(CurrentTime.GetSeconds(), 1000)];

    for( IMediaTrackRef& Track : Tracks )
    {
        FAvfMediaVideoTrack* AVFTrack = (FAvfMediaVideoTrack*)&Track.Get();
        if( AVFTrack != nil )
        {
            AVFTrack->SeekToTime( [[MediaPlayer currentItem] currentTime] );
        }
    }
*/
    return false;
}


bool FAvfMediaPlayer::SetLooping( bool Looping )
{
	return false;
}


bool FAvfMediaPlayer::SetRate( float Rate )
{
    CurrentRate = Rate;
    [MediaPlayer setRate: CurrentRate];
    return true;
}


/* FTickerBase implementation
 *****************************************************************************/


bool FAvfMediaPlayer::Tick(float DeltaTime)
{
    CurrentTime = FTimespan::FromSeconds( CMTimeGetSeconds([[MediaPlayer currentItem] currentTime]) );
    
    if( IsPlaying() )
    {
        for( IMediaTrackRef& Track : Tracks )
        {
            FAvfMediaVideoTrack* AVFTrack = (FAvfMediaVideoTrack*)&Track.Get();
            if( AVFTrack != nil )
            {
                AVFTrack->ReadFrameAtTime( [[MediaPlayer currentItem] currentTime] );
            }
        }
    }
    
    return true;
}