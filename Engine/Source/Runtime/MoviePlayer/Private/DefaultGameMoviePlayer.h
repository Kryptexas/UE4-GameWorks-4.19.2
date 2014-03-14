// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TickableObjectRenderThread.h"


/** An implementation of the movie player/loading screen we will use */
class FDefaultGameMoviePlayer : public FTickableObjectRenderThread, public IGameMoviePlayer,
	public TSharedFromThis<FDefaultGameMoviePlayer>
{
public:
	static TSharedPtr<FDefaultGameMoviePlayer> Get();
	~FDefaultGameMoviePlayer();

	/** IGameMoviePlayer Interface */
	virtual void RegisterMovieStreamer(TSharedPtr<IMovieStreamer> InMovieStreamer) OVERRIDE;
	virtual void SetSlateRenderer(TSharedPtr<FSlateRenderer> InSlateRenderer) OVERRIDE;
	virtual void Initialize() OVERRIDE;
	virtual void PassLoadingScreenWindowBackToGame() const OVERRIDE;
	virtual void SetupLoadingScreen(const FLoadingScreenAttributes& LoadingScreenAttributes) OVERRIDE;
	virtual bool PlayMovie() OVERRIDE;
	virtual void WaitForMovieToFinish() OVERRIDE;
	virtual bool IsLoadingFinished() const OVERRIDE;
	virtual bool IsMovieCurrentlyPlaying() const OVERRIDE;
	virtual bool LoadingScreenIsPrepared() const OVERRIDE;
	virtual void SetupLoadingScreenFromIni() OVERRIDE;

	/** FTickableObjectRenderThread interface */
	virtual void Tick( float DeltaTime ) OVERRIDE;
	virtual TStatId GetStatId() const OVERRIDE;
	virtual bool IsTickable() const OVERRIDE;
	
private:

	/** True if we have both a registered movie streamer and movies to stream */
	bool MovieStreamingIsPrepared() const;

	/** True if movie streamer has finished streaming all the movies it wanted to */
	bool IsMovieStreamingFinished() const;

	/** Callbacks for movie sizing for the movie viewport */
	FVector2D GetMovieSize() const;
	FOptionalSize GetMovieWidth() const;
	FOptionalSize GetMovieHeight() const;
	EVisibility GetSlateBackgroundVisibility() const;
	EVisibility GetViewportVisibility() const;

	/** Callback for clicking on the viewport */
	FReply OnLoadingScreenMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	
	/** Called via a delegate in the engine when maps start to load */
	void OnPreLoadMap();
	
	/** Called via a delegate in the engine when maps finish loading */
	void OnPostLoadMap();
private:
	FDefaultGameMoviePlayer();
	
	/** The movie streaming system that will be used by us */
	TSharedPtr<IMovieStreamer> MovieStreamer;
	
	/** The window that the loading screen resides in */
	TWeakPtr<class SWindow> LoadingScreenWindowPtr;
	/** Holds a handle to the slate renderer for our threaded drawing */
	TWeakPtr<class FSlateRenderer> RendererPtr;
	/** The widget which includes all contents of the loading screen, widgets and movie player and all */
	TSharedPtr<class SWidget> LoadingScreenContents;
	/** The widget which holds the loading screen widget passed in via the FLoadingScreenAttributes */
	TSharedPtr<class SBorder> LoadingScreenWidgetHolder;

	/** The threading mechanism with which we handle running slate on another thread */
	class FSlateLoadingSynchronizationMechanism* SyncMechanism;

	/** True if all movies have successfully streamed and completed */
	FThreadSafeCounter MovieStreamingIsDone;

	/** True if the game thread has finished loading */
	FThreadSafeCounter LoadingIsDone;

	/** User has called finish (needed if LoadingScreenAttributes.bAutoCompleteWhenLoadingCompletes is off) */
	bool bUserCalledFinish;
	
	/** Attributes of the loading screen we are currently displaying */
	FLoadingScreenAttributes LoadingScreenAttributes;
	
private:
	/** Singleton handle */
	static TSharedPtr<FDefaultGameMoviePlayer> MoviePlayer;
};
