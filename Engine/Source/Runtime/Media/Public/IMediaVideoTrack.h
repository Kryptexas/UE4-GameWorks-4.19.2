// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


class IMediaStream;


/**
 * Interface for video tracks.
 */
class IMediaVideoTrack
{
public:

	/**
	 * Gets the average data rate of the video track in bits per second.
	 *
	 * @return The data rate.
	 * @see GetFrameRate
	 */
	virtual uint32 GetBitRate() const = 0;

	/**
	 * Gets the video's dimensions in pixels.
	 *
	 * @return The width and height of the video.
	 * @see GetAspectRatio
	 */
	virtual FIntPoint GetDimensions() const = 0;

	/**
	 * Gets the video's frame rate in frames per second.
	 *
	 * @return The frame rate.
	 * @see GetBitRate
	 */
	virtual float GetFrameRate() const = 0;

	/**
	 * Get the underlying media stream.
	 *
	 * @return Media stream object.
	 */
	virtual IMediaStream& GetStream() = 0;

#if WITH_ENGINE
	/**
	 * Bind the given TextureResource to the track.
	 *
	 * The MediaPlayer/Track implementation can use this to bypass the normal CPU
	 * copy deferred sinks and directly update the resource with the GPU if desired.
	 *
	 * @see RemoveBoundTexture
	 */
	virtual void AddBoundTexture(class FRHITexture* BoundResource) { };

	/**
	 * Remove the given texture from the track binding.  Texture will no longer be updated via BoundTexture path.
	 * @see AddBoundTexture
	 */
	virtual void RemoveBoundTexture(class FRHITexture* BoundResource) { };
#endif

public:

	/**
	 * Gets the media's aspect ratio.
	 *
	 * @return Aspect ratio.
	 * @see GetDimensions
	 */
	float GetAspectRatio() const
	{
		const FVector2D Dimensions = GetDimensions();

		if (FMath::IsNearlyZero(Dimensions.Y))
		{
			return 0.0f;
		}

		return Dimensions.X / Dimensions.Y;
	}

public:

	/** Virtual destructor. */
	~IMediaVideoTrack() { }
};


/** Type definition for shared pointers to instances of IMediaVideoTrack. */
typedef TSharedPtr<IMediaVideoTrack, ESPMode::ThreadSafe> IMediaVideoTrackPtr;

/** Type definition for shared references to instances of IMediaVideoTrack. */
typedef TSharedRef<IMediaVideoTrack, ESPMode::ThreadSafe> IMediaVideoTrackRef;
