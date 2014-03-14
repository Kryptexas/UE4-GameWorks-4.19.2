// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MoviePlayer.h"

#include "Slate.h"
#include "Slate/SlateTextures.h"

#import <AVFoundation/AVFoundation.h>


// The actual streamer class
class FAVPlayerMovieStreamer : public IMovieStreamer
{
public:
	FAVPlayerMovieStreamer();
	virtual ~FAVPlayerMovieStreamer();

	virtual void Init(const TArray<FString>& MoviePaths) OVERRIDE;
	virtual void ForceCompletion() OVERRIDE;
	virtual bool Tick(float DeltaTime) OVERRIDE;
	virtual TSharedPtr<class ISlateViewport> GetViewportInterface() OVERRIDE;
	virtual float GetAspectRatio() const OVERRIDE;
	virtual void Cleanup() OVERRIDE;

private:

    enum ESyncStatus
    {
        Default,    // Starting state
        Ahead,      // Frame is ahead of playback cursor. 
        Behind,     // Frame is behind playback cursor. 
        Ready,      // Frame is within tolerance of playback cursor.
    };

    // Holds references to textures until their RHI resources are freed
    TArray<TSharedPtr<FSlateTexture2DRHIRef, ESPMode::ThreadSafe>> TexturesPendingDeletion;

	/** Texture and viewport data for displaying to Slate */
	TSharedPtr<FMovieViewport> MovieViewport;

    TSharedPtr<FSlateTextureData, ESPMode::ThreadSafe> TextureData;
    TSharedPtr<FSlateTexture2DRHIRef, ESPMode::ThreadSafe> Texture;

    // The list of pending movies
    TArray<FString>		        MovieQueue;

    // Current Movie
    AVAudioPlayer*              AudioPlayer;
    AVURLAsset*                 AVMovie;
    AVAssetReader*              AVReader;
    AVAssetReaderTrackOutput*   AVVideoOutput;
    AVAssetTrack*               AVVideoTrack;
    CMSampleBufferRef           LatestSamples;

    // AV Synchronization
    float                       VideoRate;
    int                         SyncStatus;
    double                      StartTime;
    double                      Cursor;

    bool                        bVideoTracksLoaded;
    bool                        bWasActive;

private:

    /**
	 * Sets up and starts playback on the next movie in the queue
	 */
	bool StartNextMovie();

    bool FinishLoadingTracks();

    void TeardownPlayback();

    bool CheckForNextFrameAndCopy();

};
