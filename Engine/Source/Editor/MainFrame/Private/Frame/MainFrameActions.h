// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __MainFrameActions_h__
#define __MainFrameActions_h__

#pragma once

#include "ISourceControlProvider.h"

/**
 * Unreal editor main frame actions
 */
class FMainFrameCommands : public TCommands<FMainFrameCommands>
{
public:
	/** FMainFrameCommands constructor */
	FMainFrameCommands();

	/** List of all of the main frame commands */
	static TSharedRef< FUICommandList > ActionList;

	TSharedPtr< FUICommandInfo > SaveAll;
	TSharedPtr< FUICommandInfo > Exit;
	TSharedPtr< FUICommandInfo > ChooseFilesToSave;
	TSharedPtr< FUICommandInfo > ChooseFilesToCheckIn;
	TSharedPtr< FUICommandInfo > ConnectToSourceControl;
	TSharedPtr< FUICommandInfo > NewProject;
	TSharedPtr< FUICommandInfo > OpenProject;
	TSharedPtr< FUICommandInfo > AddCodeToProject;
	TSharedPtr< FUICommandInfo > OpenIDE;
	TSharedPtr< FUICommandInfo > RefreshCodeProject;
	TSharedPtr< FUICommandInfo > PackagingSettings;
	TArray< TSharedPtr< FUICommandInfo > > SwitchProjectCommands;

	TSharedPtr< FUICommandInfo > OpenContentBrowser;
	TSharedPtr< FUICommandInfo > OpenLevelEditor;
	TSharedPtr< FUICommandInfo > OpenOutputLog;
	TSharedPtr< FUICommandInfo > OpenMessageLog;
	TSharedPtr< FUICommandInfo > OpenKeybindings;
	TSharedPtr< FUICommandInfo > OpenSessionManager;
	TSharedPtr< FUICommandInfo > OpenDeviceManager;
	TSharedPtr< FUICommandInfo > OpenToolbox;
	TSharedPtr< FUICommandInfo > OpenDebugView;
	TSharedPtr< FUICommandInfo > OpenClassViewer;

	TSharedPtr< FUICommandInfo > VisitWiki;
	TSharedPtr< FUICommandInfo > VisitForums;
	TSharedPtr< FUICommandInfo > VisitAskAQuestionPage;
	TSharedPtr< FUICommandInfo > VisitSearchForAnswersPage;
	TSharedPtr< FUICommandInfo > VisitSupportWebSite;
	TSharedPtr< FUICommandInfo > VisitEpicGamesDotCom;
	TSharedPtr< FUICommandInfo > AboutUnrealEd;

	TSharedPtr< FUICommandInfo > SaveLayout;
	TSharedPtr< FUICommandInfo > ToggleFullscreen;

	virtual void RegisterCommands() OVERRIDE;


private:

	/** Console command for toggling full screen.  We need this to expose the full screen toggle action to
	    the game UI system for play-in-editor view ports */
	FAutoConsoleCommand ToggleFullscreenConsoleCommand;
};


/**
 * Implementation of various main frame action callback functions
 */
class FMainFrameActionCallbacks
{

public:
	/** Global handler for unhandled key-down events in the editor. */
	static FReply OnUnhandledKeyDownEvent(const FKeyboardEvent& InKeyboardEvent);

	/**
	 * The default can execute action for all commands unless they override it
	 * By default commands cannot be executed if the application is in K2 debug mode.
	 */
	static bool DefaultCanExecuteAction();

	/** Saves all levels and asset packages */
	static void SaveAll();
	
	/** Opens a dialog to choose packages to save */
	static void ChoosePackagesToSave();

	/** Opens a dialog to choose packages to submit */
	static void ChoosePackagesToCheckIn();

	/** Determines whether we can choose packages to check in (we cant if an operation is already in progress) */
	static bool CanChoosePackagesToCheckIn();

	/** Enable source control features */
	static void ConnectToSourceControl();

	/** Quits the application */
	static void Exit();

	/** Edit menu commands */
	static bool Undo_CanExecute();
	static bool Redo_CanExecute();

	/**
	 * Called when many of the menu items in the main frame context menu are clicked
	 *
	 * @param Command	The command to execute
	 */
	static void ExecuteExecCommand( FString Command );

	/** Opens up the specified slate window by name after loading the module */
	static void OpenSlateApp_ViaModule( FName AppName, FName ModuleName );

	/** Opens up the specified slate window by name */
	static void OpenSlateApp( FName AppName );

	/** Checks if a slate window is already open */
	static bool OpenSlateApp_IsChecked( FName AppName );

	/** Visits the "ask a question" page on UDN */
	static void VisitAskAQuestionPage();

	/** Visits the "search for answers" page on UDN */
	static void VisitSearchForAnswersPage();

	/** Visits the UDN support web site */
	static void VisitSupportWebSite();

	/** Visits EpicGames.com */
	static void VisitEpicGamesDotCom();

	static void VisitWiki();

	static void VisitForums();

	static void AboutUnrealEd_Execute();

	/** Opens the new project dialog */
	static void NewProject( bool bAllowProjectOpening, bool bAllowProjectCreate );

	/** Adds code to the current project if it does not already have any */
	static void AddCodeToProject();

	/** Cooks the project's content for the specified platform. */
	static void CookContent( const FString PlatformName, const FText PlatformDisplayName );

	/** Checks whether a menu action for cooking the project's content can execute. */
	static bool CookContentCanExecute( const FString PlatformName );

	/** Sets the project packaging build configuration. */
	static void PackageBuildConfiguration( EProjectPackagingBuildConfigurations BuildConfiguration );

	/** Determines whether the specified build configuration option is checked. */
	static bool PackageBuildConfigurationIsChecked( EProjectPackagingBuildConfigurations BuildConfiguration );

	/** Packages the project for the specified platform. */
	static void PackageProject( const FString PlatformName, const FText PlatformDisplayName );

	/** Checks whether a menu action for packaging the project can execute. */
	static bool PackageProjectCanExecute( const FString PlatformName, bool IsImplemented );

	/** Refresh the project in the current IDE */
	static void RefreshCodeProject();

	/** Opens an IDE to edit c++ code */
	static void OpenIDE();

	/** Opens the Packaging settings tab */
	static void PackagingSettings();

	/** Restarts the editor and switches projects */
	static void SwitchProjectByIndex( int32 ProjectIndex );

	/** Opens the specified project file or game. Restarts the editor */
	static void SwitchProject(const FString& GameOrProjectFileName);

	/** Save the visual state of the editor */
	static void SaveLayout();

	/** Toggle the level editor's fullscreen mode */
	static void ToggleFullscreen_Execute();

	/** Is the level editor fullscreen? */
	static bool FullScreen_IsChecked();

	/** 
	 * Checks if the selected project can be switched to
	 *
	 * @param	InProjectIndex  Index from the available projects
	 *
	 * @return true if the selected project can be switched to (i.e. it's not the current project)
	 */
	static bool CanSwitchToProject( int32 InProjectIndex );

	/** 
	 * Checks which Switch Project sub menu entry should be checked
	 *
	 * @param	InProjectIndex  Index from the available projects
	 *
	 * @return true if the menu entry should be checked
	 */
	static bool IsSwitchProjectChecked( int32 InProjectIndex );

	/** Gathers all available projects the user can switch to from main menu */
	static void CacheProjectNames();


public:

	// List of projects that the user can switch to.
	static TArray<FString> ProjectNames;


protected:

	/**
	 * Creates an asynchronous UAT task.
	 *
	 * @param CommandLine - The command line for the UAT task.
	 * @param PlatformDisplayName - The display name of the platform that the task is running for.
	 * @param TaskName - The human readable name of the task.
	 * @param TaskIcon - The icon for the task (to show in the notification item).
	 */
	static void CreateUatTask( const FString& CommandLine, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon );

	/** Callback for ChoosePackagesToCheckIn(), continues to bring up UI once source control operations are complete */
	static void ChoosePackagesToCheckInCallback(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult);

	/** Called when the process has completed and we have packages to check in */
	static void ChoosePackagesToCheckInCompleted(TArray<UPackage*> LoadedPackages, TArray<FString> PackageNames);

	/** Delegate called when the user has decided to cancel the check in process */
	static void ChoosePackagesToCheckInCancelled(FSourceControlOperationRef InOperation);

private:

	// Handles clicking the packager notification item's Cancel button.
	static void HandleUatCancelButtonClicked( TSharedPtr<FMonitoredProcess> PackagerProcess );

	// Handles clicking the hyper link on a packager notification item.
	static void HandleUatHyperlinkNavigate( );

	// Handles canceled packager processes.
	static void HandleUatProcessCanceled( TWeakPtr<class SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName );

	// Handles the completion of a packager process.
	static void HandleUatProcessCompleted( int32 ReturnCode, TWeakPtr<class SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName );

	// Handles packager process output.
	static void HandleUatProcessOutput( FString Output, TWeakPtr<class SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName );

private:

	/** The notification in place while we choose packages to check in */
	static TWeakPtr<SNotificationItem> ChoosePackagesToCheckInNotification;
};


#endif		// __MainFrameActions_h__
