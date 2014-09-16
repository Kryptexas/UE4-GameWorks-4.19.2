// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AvfMediaPrivatePCH.h"


FAvfMediaVideoTrack::FAvfMediaVideoTrack(AVAssetTrack* InVideoTrack)
    : FAvfMediaTrack()
    , SyncStatus(Default)
    , BitRate(0)
{
    NSError* nsError = nil;
    AVReader = [[AVAssetReader alloc] initWithAsset: [InVideoTrack asset] error:&nsError];
    if( nsError != nil )
    {
        FString ErrorStr( [nsError localizedDescription] );
		UE_LOG(LogAvfMedia, Error, TEXT("Failed to create asset reader: %s"), *ErrorStr);
    }
    
    // Initialize our video output to match the format of the texture we'll be streaming to.
    NSMutableDictionary* OutputSettings = [NSMutableDictionary dictionary];
    [OutputSettings setObject: [NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(NSString*)kCVPixelBufferPixelFormatTypeKey];
        
    AVVideoOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:InVideoTrack outputSettings:OutputSettings];
    AVVideoOutput.alwaysCopiesSampleData = NO;
        
    [AVReader addOutput:AVVideoOutput];
    [AVReader startReading];
    
    bVideoTracksLoaded = true;
    
    GetVideoDetails( InVideoTrack );
}


bool FAvfMediaVideoTrack::SeekToTime( const CMTime& SeekTime )
{
    bool bSeekComplete = false;
//    if( AVReader != nil )
//    {
//        CMTimeRange TimeRange = CMTimeRangeMake(SeekTime, kCMTimePositiveInfinity);
//        AVReader.timeRange = TimeRange;
        
//        bSeekComplete = true;
//    }
    return bSeekComplete;
}


void FAvfMediaVideoTrack::GetVideoDetails(AVAssetTrack* InVideoTrack)
{
    // Setup the asset reader to ascertain some further video details.
    NSError* nsError = nil;
    AVAssetReader* DetailsAssetReader = [[AVAssetReader alloc] initWithAsset: [InVideoTrack asset] error:&nsError];
    if( nsError != nil )
    {
        FString ErrorStr( [nsError localizedDescription] );
		UE_LOG(LogAvfMedia, Error, TEXT("Failed to create asset reader: %s"), *ErrorStr);
    }
    
    // Initialize our video output so we can get some details
    NSMutableDictionary* OutputSettings = [NSMutableDictionary dictionary];
    [OutputSettings setObject: [NSNumber numberWithInt:kCVPixelFormatType_32BGRA] forKey:(NSString*)kCVPixelBufferPixelFormatTypeKey];
    
    AVAssetReaderTrackOutput* DetailsTrackOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:InVideoTrack outputSettings:OutputSettings];
    DetailsTrackOutput.alwaysCopiesSampleData = NO;
    
    
    // Read Frame rate
    FrameRate = 1.0f / [[DetailsTrackOutput track] nominalFrameRate];
    
    
    // Let's start reading the buffer to get some further details.
    [DetailsAssetReader addOutput:DetailsTrackOutput];
    [DetailsAssetReader startReading];
    
    // Grab the pixel buffer and lock it for reading.
    CMSampleBufferRef LatestSamples = [DetailsTrackOutput copyNextSampleBuffer];
    CVImageBufferRef PixelBuffer = CMSampleBufferGetImageBuffer(LatestSamples);
    CVPixelBufferLockBaseAddress( PixelBuffer, kCVPixelBufferLock_ReadOnly );
    
    // Grab the width and height
    Width  = (uint32)CVPixelBufferGetWidth(PixelBuffer);
    Height = (uint32)CVPixelBufferGetHeight(PixelBuffer);
    
    CVPixelBufferUnlockBaseAddress( PixelBuffer, kCVPixelBufferLock_ReadOnly );
    CFRelease(LatestSamples);
    LatestSamples= nil;
    
    [DetailsAssetReader cancelReading];
    [DetailsAssetReader release];
}


FAvfMediaVideoTrack::~FAvfMediaVideoTrack()
{
    // Destroyed.
    [AVReader release];
    [AVVideoOutput release];
}

void FAvfMediaVideoTrack::ResetAssetReader()
{
    [AVReader cancelReading];
    [AVReader release];
}

bool FAvfMediaVideoTrack::IsReady() const
{
    return AVReader != nil && AVReader.status == AVAssetReaderStatusReading;
}


void FAvfMediaVideoTrack::ReadFrameAtTime( const CMTime& AVPlayerTime )
{
    check( bVideoTracksLoaded );
    
    if( AVReader.status == AVAssetReaderStatusReading )
    {
        double CurrentAVTime = CMTimeGetSeconds( AVPlayerTime );
    
        CMSampleBufferRef LatestSamples = nil;
    
        // We need to synchronize the video playback with the audio.
        // If the video frame is within tolerance (Ready), update the Texture Data.
        // If the video is Behind, throw it away and get the next one until we catch up with the ref time.
        // If the video is Ahead, update the TextureData but don't retrieve more frames until time catches up.
        while( SyncStatus != Ready )
        {
            double Delta;
            if( SyncStatus != Ahead )
            {
                LatestSamples = [AVVideoOutput copyNextSampleBuffer];
            
                if( LatestSamples == NULL )
                {
                    // Failed to get next sample buffer.
                    break;
                }
            }
        
            // Get the time stamp of the video frame
            CMTime FrameTimeStamp = CMSampleBufferGetPresentationTimeStamp(LatestSamples);
    
            // Compute delta of video frame and current playback times
            Delta = CurrentAVTime - CMTimeGetSeconds(FrameTimeStamp);
        
            if( Delta < 0 )
            {
                Delta *= -1;
                SyncStatus = Ahead;
            }
            else
            {
                SyncStatus = Behind;
            }
        
            if( Delta < FrameRate )
            {
                // Video in sync with audio
                SyncStatus = Ready;
            }
            else if(SyncStatus == Ahead)
            {
                // Video ahead of audio: stay in Ahead state, exit loop
                break;
            }
            else if( LatestSamples != nil )
            {
                // Video behind audio (Behind): stay in loop
                CFRelease(LatestSamples);
                LatestSamples = NULL;
            }
        }
    
        // Process this frame.
        if( SyncStatus == Ready )
        {
            check(LatestSamples != NULL);
    
            // Grab the pixel buffer and lock it for reading.
            CVImageBufferRef PixelBuffer = CMSampleBufferGetImageBuffer(LatestSamples);
            CVPixelBufferLockBaseAddress( PixelBuffer, kCVPixelBufferLock_ReadOnly );
    
            Width  = (uint32)CVPixelBufferGetWidth(PixelBuffer);
            Height = (uint32)CVPixelBufferGetHeight(PixelBuffer);
    
            uint8* VideoDataBuffer = (uint8*)CVPixelBufferGetBaseAddress( PixelBuffer );
            uint32 BufferSize = Width * Height * 4;
    
            // Push the pixel data for the frame to the sinks which are listening for it.
            for( IMediaSinkWeakPtr& SinkPtr : Sinks )
            {
                IMediaSinkPtr NextSink = SinkPtr.Pin();
                if( NextSink.IsValid() )
                {
                    NextSink->ProcessMediaSample( VideoDataBuffer, BufferSize, FTimespan::MaxValue(), FTimespan::FromSeconds(CurrentAVTime));
                }
            }
    
            CVPixelBufferUnlockBaseAddress( PixelBuffer, kCVPixelBufferLock_ReadOnly );
            CFRelease(LatestSamples);
            LatestSamples= nil;
        }
    
        if( SyncStatus != Ahead )
        {
            // Reset status;
            SyncStatus = Default;
        }
    }
}