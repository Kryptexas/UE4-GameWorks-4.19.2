// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*=============================================================================
	StreamingPauseRendering.h: Streaming pause definitions.
=============================================================================*/

#ifndef __STREAMINGPAUSERENDERING_H__
#define __STREAMINGPAUSERENDERING_H__

#include "Engine.h"
#include "ModuleInterface.h"

/** 
 * Module handling default behavior for streaming pause rendering. 
 * Games can override by calling RegisterBegin/EndStreamingPauseDelegate with their own delegates.
 */
class FStreamingPauseRenderingModule : public IModuleInterface
{
public:	
	FStreamingPauseRenderingModule();

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
