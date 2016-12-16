// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IMediaAudioSink;
class IMediaBinarySink;
class IMediaOverlaySink;
class IMediaTextureSink;

/**
 * Interface for access to a media player's output.
 *
 * @see IMediaControls, IMediaTracks
 */
class IMediaOutput
{
public:

	/**
	 * Set the sink to receive data from audio tracks.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetBinarySink, SetOverlaySink, SetVideoSink
	 */
	virtual void SetAudioSink(IMediaAudioSink* Sink) = 0;

	/**
	 * Set the sink to receive data from metadata tracks.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetAudioSink, SetOverlaySink, SetVideoSink
	 */
	virtual void SetMetadataSink(IMediaBinarySink* Sink) = 0;

	/**
	 * Set the sink to receive data from caption, subtitle, and text tracks.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetAudioSink, SetBinarySink, SetVideoSink
	 */
	virtual void SetOverlaySink(IMediaOverlaySink* Sink) = 0;

	/**
	 * Set the sink to receive data from the active video track.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetAudioSink, SetBinarySink, SetOverlaySink
	 */
	virtual void SetVideoSink(IMediaTextureSink* Sink) = 0;

public:

	/** Virtual destructor. */
	virtual ~IMediaOutput() { }
};
