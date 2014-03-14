// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StreamingPauseRendering.h: Streaming pause definitions.
=============================================================================*/

#ifndef __STREAMINGPAUSERENDERING_H__
#define __STREAMINGPAUSERENDERING_H__

/** Allow the streaming pause only on selected platform. It is required for TRC/TCR. */
#define WITH_STREAMING_PAUSE 0

extern bool GUseStreamingPause;

/** Streaming pause interface. */
struct FStreamingPause
{
	/** Initializes the streaming pause object. */
	static void Init();

	/** Renders the streaming pause icon. */
	static void Render();

	/** Suspends the title rendering and enables the streaming pause rendering. */
	static void SuspendRendering();

	/** Resumes the title rendering and deletes the streaming pause rendering. */
	static void ResumeRendering();

	/** Returns true if the title rendering is suspended. Otherwise, returns false. */
	static bool IsRenderingSuspended();

	/** Updates the streaming pause rendering. */
	static void Tick( float DeltaTime );

	/** Enqueue the streaming pause to suspend rendering during blocking load. */
	static void GameThreadWantsToSuspendRendering( class FViewport* Viewport );

	/** Enqueue the streaming pause to resume rendering after blocking load is completed. */
	static void GameThreadWantsToResumeRendering();
};

#endif // __STREAMINGPAUSERENDERING_H__