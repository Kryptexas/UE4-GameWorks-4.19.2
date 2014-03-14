// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealEd.cpp: UnrealEd package file
=============================================================================*/

#include "UnrealEd.h"
#include "AssetSelection.h"
#include "Factories.h"

#include "BlueprintUtilities.h"
#include "EngineLevelScriptClasses.h"
#include "BlueprintGraphDefinitions.h"
#include "EdGraphUtilities.h"
#include "DebugToolExec.h"
#include "MainFrame.h"
#include "ISourceControlModule.h"
#include "EngineAnalytics.h"
#include "IAnalyticsProvider.h"

#include "GameProjectGenerationModule.h"
#include "AssetEditorManager.h"

UUnrealEdEngine* GUnrealEd;

DEFINE_LOG_CATEGORY_STATIC(LogUnrealEd, Log, All);

/**
 * Provides access to the FEditorModeTools singleton.
 */
class FEditorModeTools& GEditorModeTools()
{
	static FEditorModeTools* EditorModeToolsSingleton = new FEditorModeTools;
	return *EditorModeToolsSingleton;
}

FLevelEditorViewportClient* GCurrentLevelEditingViewportClient = NULL;
/** Tracks the last level editing viewport client that received a key press. */
FLevelEditorViewportClient* GLastKeyLevelEditingViewportClient = NULL;

/**
 * Returns the path to the engine's editor resources directory (e.g. "/../../Engine/Content/Editor/")
 */
const FString GetEditorResourcesDir()
{
	return FPaths::Combine( FPlatformProcess::BaseDir(), *FPaths::EngineContentDir(), TEXT("Editor/") );
}

int32 EditorInit( IEngineLoop& EngineLoop )
{
	// Create debug exec.	
	GDebugToolExec = new FDebugToolExec;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Editor Initialized"), STAT_EditorStartup, STATGROUP_LoadTime);

	int32 ErrorLevel = EngineLoop.Init();
	if( ErrorLevel != 0 )
	{
		FPlatformSplash::Hide();
		return 0;
	}

	// Initialize the misc editor
	FUnrealEdMisc::Get().OnInit();

	// Prime our array of default directories for loading and saving content files to
	FEditorDirectories::Get().LoadLastDirectories();


	// =================== CORE EDITOR INIT FINISHED ===================

	// Hide the splash screen now that everything is ready to go
	FPlatformSplash::Hide();

	// Are we in immersive mode?
	const bool bIsImmersive = FParse::Param( FCommandLine::Get(), TEXT( "immersive" ) );

	// Do final set up on the editor frame and show it
	{
		// Tear down rendering thread once instead of doing it for every window being resized.
		SCOPED_SUSPEND_RENDERING_THREAD(true);

		// Startup Slate main frame and other editor windows
		{
			// Always start maximized if -immersive was specified.  NOTE: Currently, if no layout data
			// exists, the main frame will always start up maximized.
			const bool bForceMaximizeMainFrame = bIsImmersive;
			const bool bStartImmersivePIE = bIsImmersive;
			
			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			MainFrameModule.CreateDefaultMainFrame( bStartImmersivePIE );

		}
	}

	// Check for automated build/submit option
	const bool bDoAutomatedMapBuild = FParse::Param( FCommandLine::Get(), TEXT("AutomatedMapBuild") );

	// Prompt to update the game project file to the current version, if necessary
	if ( FRocketSupport::IsRocket() && FPaths::IsProjectFilePathSet() )
	{
		FGameProjectGenerationModule::Get().CheckForOutOfDateGameProjectFile();
	}

	// =================== EDITOR STARTUP FINISHED ===================
	
	// Don't show welcome screen when in immersive preview mode
	if( !bDoAutomatedMapBuild && !bIsImmersive ) 
	{
		bool bShowWelcomeScreen = false;

		// See if the welcome screen should be displayed based on user settings, and if so, display it
		// @todo: Check user preferences
		if ( bShowWelcomeScreen )
		{
			// @todo: Display the welcome screen!
		}
	}

	// Stat tracking
	{
		const float StartupTime = (float)( FPlatformTime::Seconds() - GStartTime );

		if( FEngineAnalytics::IsAvailable() )
		{
			FEngineAnalytics::GetProvider().RecordEvent( 
				TEXT( "Editor.Performance.Startup" ), 
				TEXT( "Duration" ), FString::Printf( TEXT( "%.3f" ), StartupTime ) );
		}
	}

	return 1;
}

void EditorExit()
{
	if( GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_MeshPaint) ||
		GEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit) )
	{
		GEditorModeTools().ActivateMode( FBuiltinEditorModes::EM_Default );
	}

	// Save out any config settings for the editor so they don't get lost
	GEditor->SaveConfig();
	GEditorModeTools().SaveConfig();

	// Save out default file directories
	FEditorDirectories::Get().SaveLastDirectories();

	// Cleanup the misc editor
	FUnrealEdMisc::Get().OnExit();

	if( GLogConsole )
	{
		GLogConsole->Show( false );
	}


	delete GDebugToolExec;
	GDebugToolExec = NULL;

}

IMPLEMENT_MODULE( FDefaultModuleImpl, UnrealEd );

#include "UnrealEd.generated.inl"

