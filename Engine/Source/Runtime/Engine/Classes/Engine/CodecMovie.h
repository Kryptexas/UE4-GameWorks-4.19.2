// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "CodecMovie.generated.h"

UCLASS(abstract, transient, MinimalAPI)
class UCodecMovie : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Cached script accessible playback duration of movie. */
	UPROPERTY(transient)
	float PlaybackDuration;


	// Can't have pure virtual functions in classes declared in *Classes.h due to DECLARE_CLASS macro being used.

	// CodecMovie interface

	/**
	* Not all codec implementations are available
	*
	* @return true if the current codec is supported
	*/
	virtual bool IsSupported() { return false; }

	/**
	 * Returns the movie width.
	 *
	 * @return width of movie.
	 */
	virtual uint32 GetSizeX() { return 0; }

	/**
	 * Returns the movie height.
	 *
	 * @return height of movie.
	 */
	virtual uint32 GetSizeY()	{ return 0; }

	/** 
	 * Returns the movie format.
	 *
	 * @return format of movie.
	 */
	virtual EPixelFormat GetFormat();

	/**
	 * Returns the framerate the movie was encoded at.
	 *
	 * @return framerate the movie was encoded at.
	 */
	virtual float GetFrameRate() { return 0; }
	
	/**
	 * Initializes the decoder to stream from disk.
	 *
	 * @param	Filename	Filename of compressed media.
	 * @param	Offset		Offset into file to look for the beginning of the compressed data.
	 * @param	Size		Size of compressed data.
	 *
	 * @return	true if initialization was successful, false otherwise.
	 */
	virtual bool Open( const FString& Filename, uint32 Offset, uint32 Size ) { return false; }

	/**
	 * Initializes the decoder to stream from memory.
	 *
	 * @param	Source		Beginning of memory block holding compressed data.
	 * @param	Size		Size of memory block.
	 *
	 * @return	true if initialization was successful, false otherwise.
	 */
	virtual bool Open( void* Source, uint32 Size ) { return false; }	

	/**
	 * Tears down stream.
	 */	
	virtual void Close() {}

	/**
	 * Resets the stream to its initial state so it can be played again from the beginning.
	 */
	virtual void ResetStream() {}

	/**
	 * Queues the request to retrieve the next frame.
	 *
	 * @param InTextureMovieResource - output from movie decoding is written to this resource
	 */
	virtual void GetFrame( class FTextureMovieResource* InTextureMovieResource ) {}

	/**
	 * Returns the playback time of the movie.
	 *
	 * @return playback duration of movie.
	 */
	virtual float GetDuration() { return PlaybackDuration; }

	/** 
	* Begin playback of the movie stream 
	*
	* @param bLooping - if true then the movie loops back to the start when finished
	* @param bOneFrameOnly - if true then the decoding is paused after the first frame is processed 
	* @param bResetOnLastFrame - if true then the movie frame is set to 0 when playback finishes
	*/
	virtual void Play(bool bLooping, bool bOneFrameOnly, bool bResetOnLastFrame) {}

	/** 
	* Pause or resume the movie playback.
	*
	* @param bPause - if true then decoding will be paused otherwise it resumes
	*/
	virtual void Pause(bool bPause) {}

	/**
	* Stop playback from the movie stream 
	*/ 
	virtual void Stop() {}
	
	/**
	* Release any dynamic rendering resources created by this codec
	*/
	virtual void ReleaseDynamicResources() {}
};

