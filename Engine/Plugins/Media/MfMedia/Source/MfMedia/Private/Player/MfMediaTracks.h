// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MfMediaPrivate.h"

#if MFMEDIA_SUPPORTED_PLATFORM

#include "IMediaOutput.h"
#include "IMediaTracks.h"

#if PLATFORM_WINDOWS
	#include "WindowsHWrapper.h"
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif

class IMediaOptions;


/**
 * Implements the track collection for Microsoft Media Framework based media players.
 */
class FMfMediaTracks
	: public IMediaOutput
	, public IMediaTracks
{
	struct FStreamInfo
	{
		FText DisplayName;
		FString Language;
		FString Name;
		float SecondsUntilNextSample;
		int32 StreamIndex;
	};

	struct FAudioTrack : FStreamInfo
	{
		uint32 BitsPerSample;
		uint32 NumChannels;
		uint32 SampleRate;
	};

	struct FCaptionTrack : FStreamInfo
	{
	};

	struct FVideoTrack : FStreamInfo
	{
		uint32 BitRate;
		FIntPoint BufferDim;
		float FrameRate;
		FIntPoint OutputDim;
		EMediaTextureSinkFormat SinkFormat;
	};

public:

	/** Default constructor. */
	FMfMediaTracks();

	/** Destructor. */
	~FMfMediaTracks();

public:

	/**
	 * Initialize this track collection.
	 *
	 * @param InPresentationDescriptor The presentation descriptor object.
	 * @param InMediaSourceObject The media source object.
	 * @param InSourceReader The source reader to use.
	 * @param OutInfo String to append debug information to.
	 * @see Reinitialize
	 */
	void Initialize(IMFPresentationDescriptor* InPresentationDescriptor, IMFMediaSource* InMediaSource, IMFSourceReader* InSourceReader, FString& OutInfo);

	/**
	 * Reinitialize with the previous presentation descriptor and media source.
	 *
	 * @see Initialize
	 */
	void Reinitialize();

	/** Reset this object. */
	void Reset();

	/**
	 * Set whether tracks are enabled.
	 *
	 * @param InEnabled true if tracks are enabled, false otherwise.
	 */
	void SetEnabled(bool InEnabled);

	/** Called each frame. */
	void Tick(float DeltaTime);

public:

	//~ IMediaOutput interface

	virtual void SetAudioSink(IMediaAudioSink* Sink) override;
	virtual void SetMetadataSink(IMediaBinarySink* Sink) override;
	virtual void SetOverlaySink(IMediaOverlaySink* Sink) override;
	virtual void SetVideoSink(IMediaTextureSink* Sink) override;

public:

	//~ IMediaTracks interface

	virtual uint32 GetAudioTrackChannels(int32 TrackIndex) const override;
	virtual uint32 GetAudioTrackSampleRate(int32 TrackIndex) const override;
	virtual int32 GetNumTracks(EMediaTrackType TrackType) const override;
	virtual int32 GetSelectedTrack(EMediaTrackType TrackType) const override;
	virtual FText GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual FString GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual FString GetTrackName(EMediaTrackType TrackType, int32 TrackIndex) const override;
	virtual uint32 GetVideoTrackBitRate(int32 TrackIndex) const override;
	virtual FIntPoint GetVideoTrackDimensions(int32 TrackIndex) const override;
	virtual float GetVideoTrackFrameRate(int32 TrackIndex) const override;
	virtual bool SelectTrack(EMediaTrackType TrackType, int32 TrackIndex) override;

public:

	//~ IUnknown interface

	STDMETHODIMP QueryInterface(REFIID RefID, void** Object);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

protected:

	/**
	 * Adds the specified stream to the track collection.
	 *
	 * @param StreamIndex The index number of the stream in the presentation descriptor.
	 * @param PresentationDescriptor The presentation descriptor object.
	 * @param MediaSource The media source object.
	 * @param SourceReader The source reader to use.
	 * @param OutInfo String to append debug information to.
	 */
	void AddStreamToTracks(uint32 StreamIndex, IMFPresentationDescriptor* PresentationDescriptor, IMFMediaSource* MediaSource, FString& OutInfo);

	/** Initialize the current audio sink. */
	void InitializeAudioSink();

	/** Initialize the current text overlay sink. */
	void InitializeOverlaySink();

	/** Initialize the current video sink. */
	void InitializeVideoSink();

private:

	/** The currently used audio sink. */
	IMediaAudioSink* AudioSink;

	/** The currently used text overlay sink. */
	IMediaOverlaySink* OverlaySink;

	/** The currently used video sink. */
	IMediaTextureSink* VideoSink;

private:

	/** The available audio tracks. */
	TArray<FAudioTrack> AudioTracks;

	/** The available caption tracks. */
	TArray<FCaptionTrack> CaptionTracks;

	/** The available video tracks. */
	TArray<FVideoTrack> VideoTracks;

private:

	/** Index of the selected audio track. */
	int32 SelectedAudioTrack;

	/** Index of the selected caption track. */
	int32 SelectedCaptionTrack;

	/** Index of the selected video track. */
	int32 SelectedVideoTrack;

private:

	/** Whether the audio track has reached the end. */
	bool AudioDone;

	/** Whether the caption track has reached the end. */
	bool CaptionDone;

	/** Synchronizes write access to track arrays, selections & sinks. */
	FCriticalSection CriticalSection;

	/** Whether tracks are enabled. */
	bool Enabled;

	/** Holds a reference counter for this instance. */
	int32 RefCount;

	/** The source reader to use. */
	TComPtr<IMFSourceReader> SourceReader;

	/** Whether the video track has reached the end. */
	bool VideoDone;

	/** Whether the tracks have started playback. Set on first enabled tick. */
	bool bIsStarted;
};


#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif //MFMEDIA_SUPPORTED_PLATFORM
