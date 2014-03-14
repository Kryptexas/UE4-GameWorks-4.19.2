// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MainFrameModule.h: Declares the FMainFrameModule class.
=============================================================================*/

#pragma once


/**
 * Editor main frame module
 */
class FMainFrameModule
	: public IMainFrameModule
{
public:

	// Begin IMainFrameModule interface

	virtual void CreateDefaultMainFrame( const bool bStartImmersivePIE ) OVERRIDE;

	virtual TSharedRef<SWidget> MakeMainMenu( const TSharedPtr<FTabManager>& TabManager, const TSharedRef< FExtender > Extender ) const OVERRIDE;

	virtual TSharedRef<SWidget> MakeMainTabMenu( const TSharedPtr<FTabManager>& TabManager, const TSharedRef< FExtender > Extender ) const OVERRIDE;

	virtual TSharedRef<SWidget> MakeDeveloperTools( ) const OVERRIDE;

	virtual bool IsWindowInitialized( ) const OVERRIDE
	{
		return MainFrameHandler->GetParentWindow().IsValid();
	}
	
	virtual TSharedPtr<SWindow> GetParentWindow( ) const OVERRIDE
	{
		return MainFrameHandler->GetParentWindow();
	}

	virtual void SetMainTab(const TSharedRef<SDockTab>& MainTab) OVERRIDE
	{
		MainFrameHandler->SetMainTab(MainTab);
	}

	virtual void EnableTabClosedDelegate( ) OVERRIDE
	{
		MainFrameHandler->EnableTabClosedDelegate();
	}

	virtual void DisableTabClosedDelegate( ) OVERRIDE
	{
		MainFrameHandler->DisableTabClosedDelegate();
	}

	virtual void RequestCloseEditor( ) OVERRIDE
	{
		if ( MainFrameHandler->CanCloseEditor() )
		{
			MainFrameHandler->ShutDownEditor();
		}
		else
		{
			FUnrealEdMisc::Get().ClearPendingProjectName();
		}
	}

	virtual void SetLevelNameForWindowTitle( const FString& InLevelFileName ) OVERRIDE;
	
	virtual FString GetLoadedLevelName( ) const OVERRIDE
	{ 
		return LoadedLevelName; 
	}

	virtual const TSharedRef<FUICommandList>& GetMainFrameCommandBindings( ) OVERRIDE
	{
		return FMainFrameCommands::ActionList;
	}

	virtual class FMainMRUFavoritesList* GetMRUFavoritesList() const OVERRIDE
	{
		return MRUFavoritesList;
	}

	virtual const FText GetApplicationTitle( const bool bIncludeGameName ) const OVERRIDE
	{
		return StaticGetApplicationTitle( bIncludeGameName );
	}

	virtual void ShowAboutWindow( ) const OVERRIDE
	{
		FMainFrameActionCallbacks::AboutUnrealEd_Execute();
	}
	
	DECLARE_DERIVED_EVENT(FMainFrameModule, IMainFrameModule::FMainFrameCreationFinishedEvent, FMainFrameCreationFinishedEvent);
	virtual FMainFrameCreationFinishedEvent& OnMainFrameCreationFinished( ) OVERRIDE
	{
		return MainFrameCreationFinishedEvent;
	}

	DECLARE_DERIVED_EVENT(FMainFrameModule, IMainFrameModule::FMainFrameSDKNotInstalled, FMainFrameSDKNotInstalled);
	virtual FMainFrameSDKNotInstalled& OnMainFrameSDKNotInstalled( ) OVERRIDE
	{
		return MainFrameSDKNotInstalled;
	}
	void BroadcastMainFrameSDKNotInstalled(const FString& PlatformName, const FString& DocLink) OVERRIDE
	{
		return MainFrameSDKNotInstalled.Broadcast(PlatformName, DocLink);
	}

	// End IMainFrameModule interface


public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE;

	virtual void ShutdownModule( ) OVERRIDE;

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true; // @todo: Eventually, this should probably not be allowed.
	}

	// EndIModuleInterface interface


protected:

	/**
	 * Checks whether the project dialog should be shown at startup.
	 *
	 * The project dialog should be shown if the Editor was started without a game specified.
	 *
	 * @return true if the project dialog should be shown, false otherwise.
	 */
	bool ShouldShowProjectDialogAtStartup( ) const;


private:

	// Handles the level editor module starting to recompile.
	void HandleLevelEditorModuleCompileStarted( );

	// Handles the user requesting the current compilation to be canceled
	void OnCancelCodeCompilationClicked();

	// Handles the level editor module finishing to recompile.
	void HandleLevelEditorModuleCompileFinished( const FString& LogDump, bool bCompileSucceeded, bool bShowLog );

	// Handles the VSHandler having finished launching Visual Studio.
	void HandleVSAccessorLaunched( const bool WasSuccessful );

	// Handle an VS open file operation failing
	void HandleVSAccessorOpenFileFailed(const FString& Filename);

	// Handles VSHandler launching Visual Studio.
	void HandleVSAccessorLaunching( );


private:

	// Weak pointer to the level editor's compile notification item.
	TWeakPtr<SNotificationItem> CompileNotificationPtr;

	// Friendly name for persistently level name currently loaded.  Used for window and tab titles.
	FString LoadedLevelName;

	/// Event to be called when the mainframe is fully created.
	FMainFrameCreationFinishedEvent MainFrameCreationFinishedEvent;

	/// Event to be called when the editor tried to use a platform, but it wasn't installed
	FMainFrameSDKNotInstalled MainFrameSDKNotInstalled;

	// Commands used by main frame in menus and key bindings.
	TSharedPtr<class FMainFrameCommands> MainFrameActions;

	// Holds the main frame handler.
	TSharedPtr<class FMainFrameHandler> MainFrameHandler;

	// Absolute real time that we started compiling modules. Used for stats tracking.
	double ModuleCompileStartTime;

	// Holds the collection of most recently used favorites.
	class FMainMRUFavoritesList* MRUFavoritesList;

	// Weak pointer to the VSAccessor's notification item.
	TWeakPtr<class SNotificationItem> VSAccessorNotificationPtr;

	// Sounds used for compilation.
	class USoundBase* CompileStartSound;
	class USoundBase* CompileSuccessSound;
	class USoundBase* CompileFailSound;
};
