// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StreamingPauseRendering.h: Streaming pause definitions.
=============================================================================*/

#ifndef __STREAMINGPAUSERENDERING_H__
#define __STREAMINGPAUSERENDERING_H__

#include "Engine.h"
#include "ModuleInterface.h"

/** Allow the streaming pause only on selected platform. It is required for TRC/TCR. */
#define WITH_STREAMING_PAUSE 1

/** 
 * Module handling default behavior for streaming pause rendering. 
 * Games can override by calling RegisterBegin/EndStreamingPauseDelegate with their own delegates.
 */
class FStreamingPauseRenderingModule : public IModuleInterface
{
public:	
	FStreamingPauseRenderingModule()
		: Viewport(NULL)
		, ViewportWidget(NULL)
		, ViewportClient(NULL) 
	{
	}

	virtual void StartupModule();	
	virtual void ShutdownModule();

	virtual void BeginStreamingPause( class FViewport* Viewport );
	virtual void EndStreamingPause();

	TSharedPtr<FSceneViewport>		Viewport;
	TSharedPtr<SViewport>			ViewportWidget;
	FViewportClient*			ViewportClient;

	/** Delegate providing default functionality for beginning streaming pause. */
	FBeginStreamingPauseDelegate BeginDelegate;
	/** Delegate providing default functionality for ending streaming pause. */
	FEndStreamingPauseDelegate EndDelegate;
};

#endif // __STREAMINGPAUSERENDERING_H__
