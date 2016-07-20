// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AvfMediaPCH.h"
#include "AvfMediaPlayer.h"


/* FMediaHelper implementation
 *****************************************************************************/

@implementation FMediaHelper
@synthesize bIsPlayerItemReady;
@synthesize MediaPlayer;


-(FMediaHelper*) initWithMediaPlayer:(AVPlayer*)InPlayer
{
	MediaPlayer = InPlayer;
	bIsPlayerItemReady = false;
	
	self = [super init];
	return self;
}


/** Listener for changes in our media classes properties. */
- (void) observeValueForKeyPath:(NSString*)keyPath
		ofObject:	(id)object
		change:		(NSDictionary*)change
		context:	(void*)context
{
	if ([keyPath isEqualToString:@"status"])
	{
		if (object == [MediaPlayer currentItem])
		{
			bIsPlayerItemReady = ([MediaPlayer currentItem].status == AVPlayerItemStatusReadyToPlay);
		}
	}
}


- (void)dealloc
{
	[MediaPlayer release];
	[super dealloc];
}

@end


/* FAvfMediaPlayer structors
 *****************************************************************************/

FAvfMediaPlayer::FAvfMediaPlayer()
{
    CurrentTime = FTimespan::Zero();
    Duration = FTimespan::Zero();
	MediaUrl = FString();
	ShouldLoop = false;
    
	MediaHelper = nil;
    MediaPlayer = nil;
	PlayerItem = nil;
	
	CurrentRate = 0.0f;
}


/* FTickerObjectBase interface
 *****************************************************************************/

bool FAvfMediaPlayer::Tick(float DeltaTime)
{
	return false;
	/*
	if (!IsPlaying() || !IsReady() && !ReachedEnd())
	{
		return true;
	}

	for (IMediaVideoTrackRef& VideoTrack : VideoTracks)
	{
		FAvfMediaVideoTrack& AVFTrack = (FAvfMediaVideoTrack&)VideoTrack.Get();

		if (!AVFTrack.ReadFrameAtTime([[MediaPlayer currentItem] currentTime]))
		{
			if (ReachedEnd())
			{
				MediaEvent.Broadcast(EMediaEvent::PlaybackEndReached);

				if (ShouldLoop)
				{
					Seek(FTimespan(0));
					SetRate(CurrentRate);
				}
			}
			else
			{
				UE_LOG(LogAvfMedia, Display, TEXT("Failed to read video track for "), *MediaUrl);
				
				return false;
			}
		}
		else
		{
			CurrentTime = FTimespan::FromSeconds(CMTimeGetSeconds([[MediaPlayer currentItem] currentTime]));
		}
	}
	
    return true;*/
}


/* IMediaControls interface
 *****************************************************************************/

FTimespan FAvfMediaPlayer::GetDuration() const
{
	return Duration;
}


float FAvfMediaPlayer::GetRate() const
{
	return CurrentRate;
}


EMediaState FAvfMediaPlayer::GetState() const
{
	return EMediaState::Error; // @todo AvfMedia: implement state management
}


TRange<float> FAvfMediaPlayer::GetSupportedRates(EMediaPlaybackDirections Direction, bool Unthinned) const
{
	if (Direction == EMediaPlaybackDirections::Reverse)
	{
		return TRange<float>::Empty();
	}

	return TRange<float>(0.0f, 1.0f);
}


FTimespan FAvfMediaPlayer::GetTime() const
{
	return CurrentTime;
}


bool FAvfMediaPlayer::IsLooping() const
{
	return ShouldLoop;
}


bool FAvfMediaPlayer::Seek(const FTimespan& Time)
{
	return false;
}


bool FAvfMediaPlayer::SetLooping(bool Looping)
{
	ShouldLoop = Looping;

	return true;
}


bool FAvfMediaPlayer::SetRate(float Rate)
{
	CurrentRate = Rate;
	[MediaPlayer setRate : CurrentRate];

	MediaEvent.Broadcast(
		FMath::IsNearlyZero(Rate)
		? EMediaEvent::PlaybackSuspended
		: EMediaEvent::PlaybackResumed
	);

	return true;
}


bool FAvfMediaPlayer::SupportsRate(float Rate, bool Unthinned) const
{
	return GetSupportedRates(EMediaPlaybackDirections::Forward, true).Contains(Rate);
}


bool FAvfMediaPlayer::SupportsScrubbing() const
{
	return false;
}


bool FAvfMediaPlayer::SupportsSeeking() const
{
	return true;
}


/* IMediaPlayer interface
 *****************************************************************************/

void FAvfMediaPlayer::Close()
{
	// @todo trepka: return here if already closed

	CurrentTime = 0;
	MediaUrl = FString();

	if (PlayerItem != nil)
	{
		[PlayerItem removeObserver : MediaHelper forKeyPath : @"status"];
		[PlayerItem release];
		PlayerItem = nil;
	}

	if (MediaHelper)
	{
		[MediaHelper release];
		MediaHelper = nil;
	}

	Tracks.Reset();
	MediaEvent.Broadcast(EMediaEvent::TracksChanged);

	Duration = CurrentTime = FTimespan::Zero();

#if PLATFORM_MAC
	GameThreadCall(^{
#elif PLATFORM_IOS
		// report back to the game thread whether this succeeded
		[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void) {
#endif
		// dispatch on the game thread that we have closed the video
			MediaEvent.Broadcast(EMediaEvent::MediaClosed);
#if PLATFORM_IOS
			return true;
		}];
#elif PLATFORM_MAC
	});
#endif
}


IMediaControls& FAvfMediaPlayer::GetControls()
{
	return *this;
}


FString FAvfMediaPlayer::GetInfo() const
{
	return TEXT("AvfMedia media information not implemented yet");
}


IMediaOutput& FAvfMediaPlayer::GetOutput()
{
	return Tracks;
}


FString FAvfMediaPlayer::GetStats() const
{
	return TEXT("AvfMedia stats information not implemented yet");
}


IMediaTracks& FAvfMediaPlayer::GetTracks()
{
	return Tracks;
}


FString FAvfMediaPlayer::GetUrl() const
{
	return MediaUrl;
}


bool FAvfMediaPlayer::Open(const FString& Url, const IMediaOptions& Options)
{
	Close();

	// open media file
	FString Ext = FPaths::GetExtension(Url);
	FString BFNStr = FPaths::GetBaseFilename(Url);

#if PLATFORM_MAC
	NSURL* nsMediaUrl = [NSURL fileURLWithPath : Url.GetNSString()];
#elif PLATFORM_IOS
	NSURL* nsMediaUrl = [[NSBundle mainBundle] URLForResource:BFNStr.GetNSString() withExtension : Ext.GetNSString()];
#endif

	if (nsMediaUrl == nil)
	{
		UE_LOG(LogAvfMedia, Error, TEXT("Failed to open Media file:"), *Url);

		return false;
	}

	// create player instance
	MediaUrl = FPaths::GetCleanFilename(Url);
	MediaPlayer = [[AVPlayer alloc] init];

	if (!MediaPlayer)
	{
		UE_LOG(LogAvfMedia, Error, TEXT("Failed to create instance of an AVPlayer"));

		return false;
	}

	// create player item
	MediaHelper = [[FMediaHelper alloc] initWithMediaPlayer:MediaPlayer];
	check(MediaHelper != nil);

	PlayerItem = [AVPlayerItem playerItemWithURL : nsMediaUrl];

	if (PlayerItem == nil)
	{
		UE_LOG(LogAvfMedia, Error, TEXT("Failed to open player item with Url:"), *Url);

		return false;
	}

	// load tracks
	[PlayerItem retain];
	[PlayerItem addObserver : MediaHelper forKeyPath : @"status" options : 0 context : nil];

	Duration = FTimespan::FromSeconds(CMTimeGetSeconds(PlayerItem.asset.duration));

	[[PlayerItem asset] loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^
	{
		NSError* Error = nil;

		if ([[PlayerItem asset] statusOfValueForKey:@"tracks" error : &Error] == AVKeyValueStatusLoaded)
		{
//			Tracks.Initialize(PlayerItem); // @todo trepka: fix me

#if PLATFORM_MAC
			GameThreadCall(^{
#elif PLATFORM_IOS
				// report back to the game thread whether this succeeded.
				[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void) {
#endif
					// dispatch on the gamethread that we have opened the video.
					MediaEvent.Broadcast(EMediaEvent::TracksChanged);
					MediaEvent.Broadcast(EMediaEvent::MediaOpened);
#if PLATFORM_IOS
					return true;
				}];
#elif PLATFORM_MAC
			});
#endif
		}
		else if (Error != nullptr)
		{
			NSDictionary *userInfo = [Error userInfo];
			NSString *errstr = [[userInfo objectForKey : NSUnderlyingErrorKey] localizedDescription];

			UE_LOG(LogAvfMedia, Warning, TEXT("Failed to load video tracks. [%s]"), *FString(errstr));
		}
	}];

	[MediaPlayer replaceCurrentItemWithPlayerItem : PlayerItem];
	[[MediaPlayer currentItem] seekToTime:kCMTimeZero];

	MediaPlayer.rate = 0.0;
	CurrentTime = FTimespan::Zero();
		
	return true;
}


bool FAvfMediaPlayer::Open(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, const IMediaOptions& Options)
{
	return false; // not supported
}


/* FAvfMediaPlayer implementation
 *****************************************************************************/

bool FAvfMediaPlayer::ReachedEnd() const
{/*
	for (const IMediaVideoTrackRef& VideoTrack : VideoTracks)
	{
		FAvfMediaVideoTrack& AVFTrack = (FAvfMediaVideoTrack&)VideoTrack.Get();

		if (AVFTrack.ReachedEnd())
		{
			return true;
		}
	}*/

	return false;
}
