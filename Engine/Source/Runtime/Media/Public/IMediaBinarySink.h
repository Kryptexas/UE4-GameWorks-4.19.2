// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Interface for media sinks that receive binary data.
 *
 * Implementations of this interface must be thread-safe as these
 * methods will be called from some random media decoder thread.
 */
class IMediaBinarySink
{
public:

	/**
	 * Initialize the sink.
	 *
	 * @return true on success, false otherwise.
	 * @see ShutdownBinarySink
	 */
	virtual bool InitializeBinarySink() = 0;

	/**
	 * Process the given binary data.
	 *
	 * @param Data The data process.
	 * @param Size The size of the data (in bytes).
	 * @param Time The timestamp of the data (relative to the beginning of the media).
	 * @param Duration The duration of the data.
	 */
	virtual void ProcessBinarySinkData(const uint8* Data, uint32 Size, FTimespan Time, FTimespan Duration) = 0;

	/**
	 * Shut down the sink.
	 *
	 * @see InitializeBinarySink
	 */
	virtual void ShutdownBinarySink() = 0;

public:

	/** Virtual destructor. */
	virtual ~IMediaBinarySink() { }
};
