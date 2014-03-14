// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MainFrameHandler.h: Declares the FMainFrameHandler class.
=============================================================================*/

#pragma once


/**
 * Helper class that handles talking to the Slate App Manager.
 */
class FMainFrameHandler : public TSharedFromThis<FMainFrameHandler>
{
public:

	/**
	 * Shuts down the Editor.
	 */
	void ShutDownEditor( TSharedRef<SDockTab> TabBeingClosed )
	{
		ShutDownEditor();
	}

	/**
	 * Shuts down the Editor.
	 */
	void ShutDownEditor( )
	{
		FEditorDelegates::OnShutdownPostPackagesSaved.Broadcast();

		// Any pending autosaves should not happen.  A tick will go by before the editor shuts down and we want to avoid auto-saving during this time.
		GUnrealEd->GetPackageAutoSaver().ResetAutoSaveTimer();
		GEditor->RequestEndPlayMap();

		// End any play on console/PC games still happening
		GEditor->EndPlayOnLocalPc();

		TSharedPtr<SWindow> RootWindow = RootWindowPtr.Pin();

		// Save root window placement so we can restore it.
		if( RootWindow.IsValid() )
		{
			FSlateRect WindowRect = RootWindow->GetNonMaximizedRectInScreen();
			FRootWindowLocation RootWindowLocation( FVector2D(WindowRect.Left, WindowRect.Top), WindowRect.GetSize(), RootWindow->IsWindowMaximized() );
			RootWindowLocation.SaveToIni();
		}

		// Save the visual state of the editor before we even
		// ask whether we can shut down.
		TSharedRef<FGlobalTabmanager> GlobalTabManager = FGlobalTabmanager::Get();
		if(FUnrealEdMisc::Get().IsSavingLayoutOnClosedAllowed())
		{
			GlobalTabManager->SaveAllVisualState();
		}
		else
		{
			GConfig->EmptySection(TEXT("EditorLayouts"), *GEditorUserSettingsIni);
		}


		// Clear the callback for destructionfrom the main tab; otherwise it will re-enter this shutdown function.
		if ( MainTabPtr.IsValid() )
		{
			MainTabPtr.Pin()->SetOnTabClosed( SDockTab::FOnTabClosedCallback() );
		}

		// Inform the AssetEditorManager that the editor is exiting so that it may save open assets
		// and report usage stats
		FAssetEditorManager::Get().OnExit();
		
		if (RootWindow.IsValid())
		{
			RootWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride());
			RootWindow->RequestDestroyWindow();
		}

		// Save out any config settings for the editor so they don't get lost
		GEditor->SaveConfig();
		GEditorModeTools().SaveConfig();

		// Delete user settings, if requested
		if (FUnrealEdMisc::Get().IsDeletePreferences())
		{
			IFileManager::Get().Delete(*GEditorUserSettingsIni);
		}

		// Take a screenshot of this project for the project browser
		if ( FApp::HasGameName() )
		{
			const FString ExistingBaseFilename = FString(FApp::GetGameName()) + TEXT(".png");
			const FString ExistingScreenshotFilename = FPaths::Combine(*FPaths::GameDir(), *ExistingBaseFilename);

			// If there is already a screenshot, no need to take an auto screenshot
			if ( !FPaths::FileExists(ExistingScreenshotFilename) )
			{
				const FString ScreenShotFilename = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("AutoScreenshot.png"));
				FViewport* Viewport = GEditor->GetActiveViewport();
				if ( Viewport )
				{
					UThumbnailManager::CaptureProjectThumbnail(Viewport, ScreenShotFilename, false);
				}
			}
		}

		// Shut down the editor
		// NOTE: We can't close the editor from within this stack frame as it will cause various DLLs
		//       (such as MainFrame) to become unloaded out from underneath the code pointer.  We'll shut down
		//       as soon as it's safe to do so.
		// Note this is the only place in slate that should be calling QUIT_EDITOR
		GEngine->DeferredCommands.Add( TEXT( "QUIT_EDITOR" ) );
	}
	
	/**
	 * Checks whether the main frame tab can be closed.
	 *
	 * @return true if the tab can be closed, false otherwise.
	 */
	bool CanCloseTab()
	{
		if ( GIsRequestingExit )
		{
			UE_LOG(LogMainFrame, Warning, TEXT("MainFrame: Shutdown already in progress when CanCloseTab was queried, approve tab for closure."));
			return true;
		}
		return CanCloseEditor();
	}

	/**
	 * Checks whether the Editor tab can be closed.
	 *
	 * @return true if the Editor can be closed, false otherwise.
	 */
	bool CanCloseEditor()
	{
		if ( FSlateApplication::IsInitialized() && !FSlateApplication::Get().IsNormalExecution() )
		{
			// DEBUGGER EXIT PATH

			// The debugger is running we cannot actually close now.
			// We will stop the debugger and enque a request to close the editor on the next frame.

			// Stop debugging.
			FSlateApplication::Get().LeaveDebuggingMode();

			// Defer the call RequestCloseEditor() till next tick.
			GEngine->DeferredCommands.Add( TEXT("CLOSE_SLATE_MAINFRAME") );

			// Cannot exit right now.
			return false;
		}
		else if (GIsSavingPackage || GIsGarbageCollecting)
		{
			// SAVING/GC PATH

			// We're currently saving or garbage collecting and can't close the editor just yet.
			// We will have to wait and try to request to close the editor on the next frame.

			// Defer the call RequestCloseEditor() till next tick.
			GEngine->DeferredCommands.Add( TEXT("CLOSE_SLATE_MAINFRAME") );

			// Cannot exit right now.
			return false;
		}
		else
		{
			// NORMAL EXIT PATH

			// Unattented mode can always exit.
			if (FApp::IsUnattended())
			{
				return true;
			}
			
			// We can't close if lightmass is currently building
			if (GUnrealEd->WarnIfLightingBuildIsCurrentlyRunning()) {return false;}

			bool bOkToExit = true;

			// Check if level Mode is open this does PostEditMove processing on actors when it closes so need to do this first before save dialog
			if( GEditorModeTools().IsModeActive( FBuiltinEditorModes::EM_Level ) || 
				GEditorModeTools().IsModeActive( FBuiltinEditorModes::EM_StreamingLevel) )
			{
				GEditorModeTools().ActivateMode( FBuiltinEditorModes::EM_Default );
				bOkToExit = false;
			}

			// Can we close all the major tabs? They have sub-editors in them that might want to not close
			{
				// Ignore the LevelEditor tab; it invoked this function in the first place.
				TSet< TSharedRef<SDockTab> > TabsToIgnore;
				if ( MainTabPtr.IsValid() )
				{
					TabsToIgnore.Add( MainTabPtr.Pin().ToSharedRef() );
				}

				bOkToExit = bOkToExit && FGlobalTabmanager::Get()->CanCloseManager(TabsToIgnore);
			}

			// Prompt for save and quit only if we did not launch a gameless rocket exe or are in demo mode
			if ( FApp::HasGameName() && !GIsDemoMode )
			{
				// Prompt the user to save packages/maps.
				bool bHadPackagesToSave = false;
				{
					const bool bPromptUserToSave = true;
					const bool bSaveMapPackages = true;
					const bool bSaveContentPackages = true;
					const bool bFastSave = false;
					const bool bNotifyNoPackagesSaved = false;
					bOkToExit = bOkToExit && FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved, &bHadPackagesToSave);
				}

				// If there were packages to save, or switching project, then the user already had a chance to bail out of exiting.
				if ( !bOkToExit && !bHadPackagesToSave && FUnrealEdMisc::Get().GetPendingProjectName().IsEmpty() )
				{
					FUnrealEdMisc::Get().ClearPendingProjectName();
					FUnrealEdMisc::Get().AllowSavingLayoutOnClose(true);
					FUnrealEdMisc::Get().ForceDeletePreferences(false);
					FUnrealEdMisc::Get().ClearConfigRestoreFilenames();
				}
			}
			
			return bOkToExit;
		}
	}

	void CloseRootWindowOverride( const TSharedRef<SWindow>& WindowBeingClosed )
	{
		if (CanCloseEditor())
		{
			ShutDownEditor();
		}
	}

	/**
	 * Method that handles the generation of the mainframe, given the window it resides in,
	 * and a string which determines the initial layout of its primary dock area.
	 */
	void OnMainFrameGenerated( const TSharedPtr<SDockTab>& MainTab, const TSharedRef<SWindow>& InRootWindow )
	{
		TSharedRef<FGlobalTabmanager> GlobalTabManager = FGlobalTabmanager::Get();
		
		// Persistent layouts should get stored using the specified method.
		GlobalTabManager->SetOnPersistLayout( FTabManager::FOnPersistLayout::CreateStatic( &FLayoutSaveRestore::SaveTheLayout ) );
		
		const bool bIncludeGameName = true;
		GlobalTabManager->SetApplicationTitle( StaticGetApplicationTitle( bIncludeGameName ) );
		
		InRootWindow->SetRequestDestroyWindowOverride( FRequestDestroyWindowOverride::CreateRaw( this, &FMainFrameHandler::CloseRootWindowOverride ) );

		MainTabPtr = MainTab;
		RootWindowPtr = InRootWindow;

		EnableTabClosedDelegate();
	}

	/**
	 * Shows the main frame window.  Call this after you've setup initial layouts to reveal the window 
	 *
	 * @param bStartImmersivePIE True to force a main frame viewport into immersive mode PIE session before shown
	 */
	void ShowMainFrameWindow(TSharedRef<SWindow> Window, const bool bStartImmersivePIE) const
	{
		// Make sure viewport windows are maximized/immersed if they need to be
		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked< FLevelEditorModule >( TEXT( "LevelEditor" ) );

		if( bStartImmersivePIE )
		{
			// Kick off an immersive PIE session immediately!
			LevelEditor.StartImmersivePlayInEditorSession();
			Window->ShowWindow();
			// Ensure the window is at the front or else we could end up capturing and locking the mouse to a window that isnt visible
			bool bForceWindowToFront = true;
			Window->BringToFront( bForceWindowToFront );

			// Need to register after the window is shown or else we cant capture the mouse
			TSharedPtr<ILevelViewport> Viewport = LevelEditor.GetFirstActiveViewport();
			Viewport->RegisterGameViewportIfPIE();
		}
		else
		{
			// Show the window!
			Window->ShowWindow();

			// Focus the level editor viewport
			LevelEditor.FocusViewport();

			// Restore any assets we had open. Note we don't do this on immersive PIE as its annoying to the user.
			FAssetEditorManager::Get().RestorePreviouslyOpenAssets();
		}
	}

	/** Gets the parent window of the mainframe */
	TSharedPtr<SWindow> GetParentWindow() const
	{
		return RootWindowPtr.Pin();
	}

	/** Sets the reference to the main tab */
	void SetMainTab(const TSharedRef<SDockTab>& MainTab)
	{
		MainTabPtr = MainTab;
	}
	
	/** Enables the delegate responsible for shutting down the editor when the main tab is closed */
	void EnableTabClosedDelegate()
	{
		if ( MainTabPtr.IsValid() )
		{
			MainTabPtr.Pin()->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FMainFrameHandler::ShutDownEditor));
			MainTabPtr.Pin()->SetCanCloseTab(SDockTab::FCanCloseTab::CreateRaw(this, &FMainFrameHandler::CanCloseTab));
		}
	}

	/** Disables the delegate responsible for shutting down the editor when the main tab is closed */
	void DisableTabClosedDelegate()
	{
		if ( MainTabPtr.IsValid() )
		{
			MainTabPtr.Pin()->SetOnTabClosed(SDockTab::FOnTabClosedCallback());
			MainTabPtr.Pin()->SetCanCloseTab(SDockTab::FCanCloseTab());
		}
	}


private:

	/** Editor main frame window */
	TWeakPtr<SDockTab> MainTabPtr;

	/** The window that all of the editor is parented to. */
	TWeakPtr<SWindow> RootWindowPtr;
};
