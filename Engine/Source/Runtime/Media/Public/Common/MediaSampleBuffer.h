// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** Type definition for shared pointers to instances of FMediaSampleBuffer. */
typedef TSharedPtr<class FMediaSampleBuffer, ESPMode::ThreadSafe> FMediaSampleBufferPtr;

/** Type definition for shared references to instances of FMediaSampleBuffer. */
typedef TSharedRef<class FMediaSampleBuffer, ESPMode::ThreadSafe> FMediaSampleBufferRef;


/**
 * Implements a media sample sink that buffers the last sample.
 */
class FMediaSampleBuffer
	: public TQueue<TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe>>
	, public IMediaSink
{
public:

	/** Default constructor. */
	FMediaSampleBuffer( )
		: CurrentSampleTime(FTimespan::MinValue())
	{ }

public:

	/**
	 * Gets the sample data of the current media sample.
	 *
	 * @return The current sample.
	 * @see GetCurrentSampleTime
	 */
	TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> GetCurrentSample( ) const
	{
		return CurrentSample;
	}
	
	/**
	 * Gets the playback time of the currently available media sample.
	 *
	 * @return Sample playback time.
	 * @see GetCurrentSample
	 */
	FTimespan GetCurrentSampleTime( ) const
	{
		return CurrentSampleTime;
	}

public:

	// IMediaSink interface

	void ProcessMediaSample( const void* Buffer, uint32 BufferSize, FTimespan Duration, FTimespan Time ) override
	{
		TArray<uint8>* Sample = new TArray<uint8>();
		Sample->AddUninitialized(BufferSize);
		FMemory::Memcpy(Sample->GetData(), Buffer, BufferSize);

		CurrentSample = MakeShareable(Sample);
		CurrentSampleTime = Time;
	}
	
private:

	/** Holds the sample data of the current media sample. */
	TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> CurrentSample;

	/** The playback time of the currently available media sample. */
	FTimespan CurrentSampleTime;
};
