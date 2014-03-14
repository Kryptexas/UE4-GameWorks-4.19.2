// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// GameEngine: The game subsystem.
//=============================================================================

#pragma once
#include "GameEngine.generated.h"

UCLASS(HeaderGroup=GameEngine, config=Engine, transient)
class ENGINE_API UGameEngine : public UEngine
{
	GENERATED_UCLASS_BODY()

	/** The name of the class to spawn as the temporary pending level player controller */
	UPROPERTY(config)
	FString PendingLevelPlayerControllerClassName;

	/** check to see if we need to start a movie capture
	 * (used on the first tick when we want to record a matinee)
	 */
	UPROPERTY(transient)
	uint32 bCheckForMovieCapture:1;

	/** Maximium delta time the engine uses to populate GDeltaTime. If 0, unbound.									*/
	UPROPERTY(config)
	float MaxDeltaTime;

	/** String name for any secondary viewport clients created for secondary screens */
	UPROPERTY(config)
	FString SecondaryViewportClientClassName;

	/** Secondary viewport clients inside of secondary windows (not for split screen) */
	UPROPERTY()
	TArray<class UScriptViewportClient*> SecondaryViewportClients;

public:
	/** Array parallel to SecondaryViewportClients - these are the frames that render the SecondaryViewport clients */
	TArray<FViewportFrame*> SecondaryViewportFrames;

	/** The game viewport window */
	TWeakPtr<class SWindow> GameViewportWindow;
	/** The primary scene viewport */
	TSharedPtr<class FSceneViewport> SceneViewport;
	/** The game viewport widget */
	TSharedPtr<class SViewport> GameViewportWidget;
	
	/**
	 * Creates the viewport widget where the games Slate UI is added to.
	 */
	void CreateGameViewportWidget( UGameViewportClient* GameViewportClient );

	/**
	 * Creates the game viewport
	 *
	 * @param GameViewportClient	The viewport client to use in the game
	 */
	void CreateGameViewport( UGameViewportClient* GameViewportClient );
	
	/**
	 * Creates the game window
	 */
	static TSharedRef<SWindow> CreateGameWindow();

	/**
	 * Modifies the game window resolution settings if any overrides have been specified on the command line
	 *
	 * @param ResolutionX	[in/out] Width of the game window, in pixels
	 * @param ResolutionY	[in/out] Height of the game window, in pixels
	 * @param bIsFullscreen	[in/out] Whether the game should be fullscreen or not
	 */
	static void ConditionallyOverrideSettings( int32& ResolutionX, int32& ResolutionY, bool& bIsFullscreen );
	
	/**
	 * Changes the game window to use the game viewport instead of any loading screen
	 * or movie that might be using it instead
	 */
	void SwitchGameWindowToUseGameViewport();

	/**
	 * Called when the game window closes (ends the game)
	 */
	void OnGameWindowClosed( const TSharedRef<SWindow>& WindowBeingClosed );
	
	/**
	 * Called when the game window is moved
	 */
	void OnGameWindowMoved( const TSharedRef<SWindow>& WindowBeingMoved );

	/**
	 * Redraws all viewports.
	 * @param	bShouldPresent	Whether we want this frame to be presented
	 */
	virtual void RedrawViewports( bool bShouldPresent = true );

	// Begin UObject interface.
	virtual void FinishDestroy() OVERRIDE;
	// End UObject interface.

	// Begin UEngine interface.
	virtual void Init(class IEngineLoop* InEngineLoop) OVERRIDE;
	virtual void PreExit() OVERRIDE;
	virtual void Tick( float DeltaSeconds, bool bIdleMode ) OVERRIDE;
	virtual float GetMaxTickRate( float DeltaTime, bool bAllowFrameRateSmoothing = true ) OVERRIDE;
	virtual void ProcessToggleFreezeCommand( UWorld* InWorld ) OVERRIDE;
	virtual void ProcessToggleFreezeStreamingCommand( UWorld* InWorld ) OVERRIDE;
	// End UEngine interface.

	// Begin FExec Interface
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar=*GLog ) OVERRIDE;
	// End FExec Interface

	/** 
	 * Exec command handlers
	 */

	bool HandleReattachComponentsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleMovieCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleExitCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleGetMaxTickRateCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleCancelCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld );
#if !UE_BUILD_SHIPPING
	bool HandleApplyUserSettingsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
#endif // !UE_BUILD_SHIPPING

	/**
	 * Called from the first Tick after LoadMap() has been called.
	 * Turns off the loading movie if it was started by LoadMap().
	 */
	virtual void PostLoadMap();

	/**
	 * @return true, if the GEngine is a game engine and has any secondary screens active
	 */
	static bool HasSecondaryScreenActive();

	/**
	 * Creates a new FViewportFrame with a viewport client of class SecondaryViewportClientClassName
	 */
	void CreateSecondaryViewport(uint32 SizeX, uint32 SizeY);

	/**
	 * Closes all secondary viewports opened with CreateSecondaryViewport
	 */
	void CloseSecondaryViewports();

	/** Returns the GameViewport widget */
	virtual TSharedPtr<SViewport> GetGameViewportWidget() const
	{
		return GameViewportWidget;
	}
};

