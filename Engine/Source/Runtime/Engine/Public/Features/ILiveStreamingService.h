// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Core/Public/Features/IModularFeature.h"


/**
 * Contains information about the status of the live streaming service.  This is passed along to a user's
 * function through the FLiveStreamingStatusCallback delegate
 */
struct FLiveStreamingStatus
{
	/** Types of statuses */
	enum class EStatusType
	{
		/** An error of some sort */
		Failure,

		/** Progress message while we're getting setup to stream, such as login state */
		Progress,

		/** Success message that streaming is now live */
		BroadcastStarted,

		/** Sent when broadcasting stops */
		BroadcastStopped,

		/** Web cam is now active */
		WebCamStarted,

		/** Web cam is stopped */
		WebCamStopped,

		/** Web cam texture is ready (now safe to call GetWebCamTexture()) */
		WebCamTextureReady,

		/** Web cam texture is no longer valid (GetWebCamTexture() will return null) */
		WebCamTextureNotReady,
	};

	/** Status type */
	EStatusType StatusType;

	/** Customized description of the current status from the live streaming service.  This string may include
	    extra detail about the service's status, specific to that streaming system.  Unless the status is
		an error condition, this text should be brief enough that it can be displayed as an editor notification.*/
	FText CustomStatusDescription;
};



/**
 * Defines parameters for initiating a new broadcast.  Keep in mind that different live streaming services have
 * different restrictions on streaming buffer size and formats
 */
struct FBroadcastConfig
{
	/** Width of the streaming buffer.  If this changes, you'll need to stop and restart broadcasting.  Note that some streaming services
	    require the Width to be a multiple of 32 (for SSE compression).  Call MakeValidVideoBufferResolution() first to make sure it's valid. */
	int VideoBufferWidth;

	/** Height of the streaming buffer.  If this changes, you'll need to stop and restart broadcasting.  Note that some streaming services
	    require the height to be a multiple of 16 (for SSE compression).  Call MakeValidVideoBufferResolution() first to make sure it's valid. */
	int VideoBufferHeight;

	/** Target frame rate of this broadcast */
	int FramesPerSecond;

	/** Defines the possible pixel formats the output streaming buffer can be in */
	enum class EBroadcastPixelFormat
	{
		/** Four bytes per pixel, red channel first, alpha last */
		R8G8B8A8,

		/** Four bytes per pixel, blue channel first, alpha last */
		B8G8R8A8,

		/** Four bytes per pixel, alpha first, blue channel last */
		A8R8G8B8,

		/** Four bytes per pixel, alpha first, red channel last */
		A8B8G8R8,
	};

	/** Desired pixel format for the buffer. */
	EBroadcastPixelFormat PixelFormat;

	/** True to capture output audio from the local computer */
	bool bCaptureAudioFromComputer;

	/** True to capture audio from the default microphone */
	bool bCaptureAudioFromMicrophone;


	/** Default constructor */
	FBroadcastConfig()
		: VideoBufferWidth( 1280 ),
		  VideoBufferHeight( 720 ),
		  FramesPerSecond( 30 ),
		  PixelFormat( EBroadcastPixelFormat::B8G8R8A8 ),
		  bCaptureAudioFromComputer( true ),
		  bCaptureAudioFromMicrophone( true )
	{
	}
};



/**
 * Defines parameters for displaying a live web cam over your broadcast
 */
struct FWebCamConfig
{
	/** Desired web cam capture resolution width.  The web cam may only support a limited number of resolutions, so we'll
		choose one that matches as closely to this as possible */
	int DesiredWebCamWidth;

	/** Desired web cam capture resolution height. */
	int DesiredWebCamHeight;


	/** Default constructor */
	FWebCamConfig()
		: DesiredWebCamWidth( 320 ),
		  DesiredWebCamHeight( 240 )
	{
	}
};


/**
 * Generic interface to an implementation of a live streaming system, usually capable of capturing video and audio
 * and streaming live content to the internet
 */
class ILiveStreamingService : public IModularFeature
{

public:

	DECLARE_MULTICAST_DELEGATE_OneParam( FOnStatusChanged, const FLiveStreamingStatus& );

	/** Event to register with to find out when the status of the streaming system changes, such as when transitioning between
		login phases, or a web cam becomes active */
	virtual FOnStatusChanged& OnStatusChanged() = 0;


	/**
	 * Called to start broadcasting video and audio.  Note that the actual streaming may not begin immediately.
	 *
	 * @param	Config		Configuration for this broadcast.  If you want to change this later, you'll need to stop and restart the broadcast.
	 */
	virtual void StartBroadcasting( const FBroadcastConfig& Config ) = 0;

	/** Called to stop broadcasting video and audio.  There may be a delay before streaming actually stops. */
	virtual void StopBroadcasting() = 0;

	/**
	 * Checks to see if we've currently initiated a broadcast.  Note that even though we've asked to broadcast, the live stream
	 * may not have actually started yet.  For example, we could still be waiting for the login authentication to complete
	 *
	 * @return	True if we're currently broadcasting
	 */
	virtual bool IsBroadcasting() const = 0;

	/**
	 * Checks to see if this streaming service is currently broadcasting and ready to accept new video frames.
	 *
	 * @return	True if we're broadcasting right now!
	 */
	virtual bool IsReadyForVideoFrames() const = 0;

	/**
	 * Given a desired streaming resolution, allows the streaming service to change the resolution to be valid for
	 * broadcasting on this service.  Some streaming systems have specific requirements about resolutions, such as
	 * multiples of 32 or 16.  Note that image aspect ratio can be affected by this call (that is, aspect ratio is
	 * not always preserved, so it will be up to you to compensate.)
	 *
	 * @param	VideoBufferWidth	(In/Out) Set this to the width that you want, and you'll get back a width that it actually valid.
	 * @param	VideoBufferHeight	(In/Out) Set this to the height that you want, and you'll get back a height that it actually valid.
	 */
	virtual void MakeValidVideoBufferResolution( int& VideoBufferWidth, int& VideoBufferHeight ) const = 0;

	/**
	 * Queries information about the current broadcast.  Should only be called while IsBroadcasting() is true.
	 *
	 * @param	BroadcastConfig		(Out) Filled with information about the current broadcast.
	 */
	virtual void QueryBroadcastConfig( FBroadcastConfig& OutBroadcastConfig ) const = 0;

	/**
	 * Broadcasts a new video frame.  Should only be called while IsReadyForVideoFrames() is true.  The frame buffer data must match
	 * the format and resolution that the live streaming service expects.  You can call QueryBroadcastConfig() to find that out.
	 *
	 * @param	VideoFrameBuffer		The new video data to push to the live stream.  The size of the buffer must match the expected buffer resolution returned from QueryBroadcastConfig()!
	 */
	virtual void PushVideoFrame( const FColor* VideoFrameBuffer ) = 0;

	/**
	 * Enables broadcasting from your web camera device
	 *
	 * @param	Config	Settings for the web cam 
	 */
	virtual void StartWebCam( const FWebCamConfig& Config ) = 0;

	/**
	 * Turns off the web cam
	 */
	virtual void StopWebCam() = 0;

	/**
	 * Checks to see if the web cam is currently enabled
	 *
	 * @return	True if the web camera is active
	 */
	virtual bool IsWebCamEnabled() const = 0;

	/**
	 * For services that support web cams, this function will return the current picture on the web cam as a GPU texture
	 *
	 * @return	Current web cam frame as a 2D texture
	 */
	virtual class UTexture2D* GetWebCamTexture() = 0;
};