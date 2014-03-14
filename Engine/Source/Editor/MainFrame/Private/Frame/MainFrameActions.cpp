// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "MainFramePrivatePCH.h"


#define LOCTEXT_NAMESPACE "MainFrameActions"

DEFINE_LOG_CATEGORY_STATIC(MainFrameActions, Log, All);

TSharedRef< FUICommandList > FMainFrameCommands::ActionList( new FUICommandList() );

TWeakPtr<SNotificationItem> FMainFrameActionCallbacks::ChoosePackagesToCheckInNotification;

FMainFrameCommands::FMainFrameCommands()
	: TCommands<FMainFrameCommands>(
		TEXT("MainFrame"), // Context name for fast lookup
		LOCTEXT( "MainFrame", "Main Frame" ), // Localized context name for displaying
		NAME_None,	 // No parent context
		FEditorStyle::GetStyleSetName() ), // Icon Style Set
	  ToggleFullscreenConsoleCommand(
		TEXT( "MainFrame.ToggleFullscreen" ),
		TEXT( "Toggles the editor between \"full screen\" mode and \"normal\" mode.  In full screen mode, the task bar and window title area are hidden." ),
		FConsoleCommandDelegate::CreateStatic( &FMainFrameActionCallbacks::ToggleFullscreen_Execute ) )
{
}


void FMainFrameCommands::RegisterCommands()
{
	// The global action list was created at static initialization time. Create a handler for otherwise unhandled keyboard input to route key commands through this list.
	FSlateApplication::Get().SetUnhandledKeyDownEventHandler( FOnKeyboardEvent::CreateStatic( &FMainFrameActionCallbacks::OnUnhandledKeyDownEvent ) );

	// Make a default can execute action that disables input when in debug mode
	FCanExecuteAction DefaultExecuteAction = FCanExecuteAction::CreateStatic( &FMainFrameActionCallbacks::DefaultCanExecuteAction );

	UI_COMMAND( SaveAll, "Save All", "Saves all unsaved levels and assets to disk", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::S ) );
	ActionList->MapAction( SaveAll, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::SaveAll ), DefaultExecuteAction );

	UI_COMMAND( ChooseFilesToSave, "Choose Files to Save...", "Opens a dialog with save options for content and levels", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( ChooseFilesToSave, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::ChoosePackagesToSave ), DefaultExecuteAction );

	UI_COMMAND( ChooseFilesToCheckIn, "Submit to Source Control...", "Opens a dialog with check in options for content and levels", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( ChooseFilesToCheckIn, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::ChoosePackagesToCheckIn ), FCanExecuteAction::CreateStatic( &FMainFrameActionCallbacks::CanChoosePackagesToCheckIn ) );

	UI_COMMAND( ConnectToSourceControl, "Connect To Source Control...", "Connect to source control to allow source control operations to be performed on content and levels.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( ConnectToSourceControl, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::ConnectToSourceControl ), DefaultExecuteAction );

	UI_COMMAND( NewProject, "New Project...", "Opens a dialog to create a new game project", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( NewProject, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::NewProject, false, true), DefaultExecuteAction );

	UI_COMMAND( OpenProject, "Open Project...", "Opens a dialog to choose a game project to open", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( OpenProject, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::NewProject, true, false), DefaultExecuteAction );

	UI_COMMAND( AddCodeToProject, "Add Code to Project...", "Adds C++ code to the project. The code can only be compiled if you have an appropriate C++ compiler installed.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( AddCodeToProject, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::AddCodeToProject ), FCanExecuteAction::CreateStatic( &FSourceCodeNavigation::IsCompilerAvailable ) );

	UI_COMMAND( RefreshCodeProject, "Refresh code project", "Refreshes your C++ code project.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( RefreshCodeProject, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::RefreshCodeProject ), DefaultExecuteAction );

	UI_COMMAND( OpenIDE, "Open IDE", "Opens your C++ code in an integrated development environment.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( OpenIDE, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::OpenIDE ), DefaultExecuteAction );

	UI_COMMAND( PackagingSettings, "Packaging Settings...", "Opens the settings for project packaging", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( PackagingSettings, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::PackagingSettings ), DefaultExecuteAction );

	const int32 MaxProjects = 20;
	for( int32 CurProjectIndex = 0; CurProjectIndex < MaxProjects; ++CurProjectIndex )
	{
		// NOTE: The actual label and tool-tip will be overridden at runtime when the command is bound to a menu item, however
		// we still need to set one here so that the key bindings UI can function properly
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("CurrentProjectIndex"), CurProjectIndex);
		const FText Message = FText::Format( LOCTEXT( "SwitchProject", "Switch Project {CurrentProjectIndex}" ), Arguments ); 
		TSharedRef< FUICommandInfo > SwitchProject =
			FUICommandInfoDecl(
				this->AsShared(),
				FName( *FString::Printf( TEXT( "SwitchProject%i" ), CurProjectIndex ) ),
				Message,
				LOCTEXT( "SwitchProjectToolTip", "Restarts the editor and switches to selected project" ) )
			.UserInterfaceType( EUserInterfaceActionType::Button )
			.DefaultGesture( FInputGesture() );
		SwitchProjectCommands.Add( SwitchProject );

		ActionList->MapAction( SwitchProjectCommands[ CurProjectIndex ], FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::SwitchProjectByIndex, CurProjectIndex ),
			FCanExecuteAction::CreateStatic( &FMainFrameActionCallbacks::CanSwitchToProject, CurProjectIndex ),
			FIsActionChecked::CreateStatic( &FMainFrameActionCallbacks::IsSwitchProjectChecked, CurProjectIndex ) );
	}

	UI_COMMAND( Exit, "Exit", "Exits the application", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( Exit, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::Exit ), DefaultExecuteAction );

	ActionList->MapAction( FGenericCommands::Get().Undo, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::ExecuteExecCommand, FString( TEXT("TRANSACTION UNDO") ) ), FCanExecuteAction::CreateStatic( &FMainFrameActionCallbacks::Undo_CanExecute ) );

	ActionList->MapAction( FGenericCommands::Get().Redo, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::ExecuteExecCommand, FString( TEXT("TRANSACTION REDO") ) ), FCanExecuteAction::CreateStatic( &FMainFrameActionCallbacks::Redo_CanExecute ) );

	UI_COMMAND( OpenDeviceManager, "Device Manager", "Opens up the device manager app", EUserInterfaceActionType::Check, FInputGesture() );
	ActionList->MapAction( OpenDeviceManager, 
												FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::OpenSlateApp, FName( TEXT( "DeviceManager" ) ) ),
												FCanExecuteAction(),
												FIsActionChecked::CreateStatic( &FMainFrameActionCallbacks::OpenSlateApp_IsChecked, FName( TEXT( "DeviceManager" ) ) ) );

	UI_COMMAND( OpenSessionManager, "Session Manager", "Opens up the session manager app", EUserInterfaceActionType::Check, FInputGesture() );
	ActionList->MapAction( OpenSessionManager, 
												FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::OpenSlateApp, FName( "SessionFrontend" ) ),
												FCanExecuteAction(),
												FIsActionChecked::CreateStatic( &FMainFrameActionCallbacks::OpenSlateApp_IsChecked, FName("SessionFrontend" ) ) );

	UI_COMMAND(VisitWiki, "Wiki...", "Go to the Unreal Engine Wiki page", EUserInterfaceActionType::Button, FInputGesture());
	ActionList->MapAction(VisitWiki, FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::VisitWiki));

	UI_COMMAND(VisitForums, "Forums...", "Go to the Unreal Engine forums", EUserInterfaceActionType::Button, FInputGesture());
	ActionList->MapAction(VisitForums, FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::VisitForums));

	UI_COMMAND( VisitAskAQuestionPage, "Ask a Question...", "Have a question?  Go here to ask about anything and everything related to Unreal.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( VisitAskAQuestionPage, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::VisitAskAQuestionPage ) );

	UI_COMMAND( VisitSearchForAnswersPage, "Answer Hub...", "Searches for useful answers on UDN provided by other users and experts.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( VisitSearchForAnswersPage, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::VisitSearchForAnswersPage ) );

	UI_COMMAND( VisitSupportWebSite, "Unreal Engine Support Web Site...", "Navigates to the Unreal Engine Support web site's main page.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( VisitSupportWebSite, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::VisitSupportWebSite ) );

	UI_COMMAND( VisitEpicGamesDotCom, "Visit UnrealEngine.com...", "Navigates to UnrealEngine.com where you can learn more about Unreal Technology.", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( VisitEpicGamesDotCom, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::VisitEpicGamesDotCom ) );

	UI_COMMAND( AboutUnrealEd, "About Editor...", "Displays application credits and copyright information", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( AboutUnrealEd, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::AboutUnrealEd_Execute ) );

	UI_COMMAND( SaveLayout, "Save Layout", "Save the layout customizations", EUserInterfaceActionType::Button, FInputGesture() );
	ActionList->MapAction( SaveLayout, FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::SaveLayout) );

	UI_COMMAND( ToggleFullscreen, "Enable Fullscreen", "Enables fullscreen mode for the application, expanding across the entire monitor", EUserInterfaceActionType::ToggleButton, FInputGesture(EModifierKey::Shift, EKeys::F11) );
	ActionList->MapAction( ToggleFullscreen,
		FExecuteAction::CreateStatic( &FMainFrameActionCallbacks::ToggleFullscreen_Execute ),
		FCanExecuteAction(),
		FIsActionChecked::CreateStatic( &FMainFrameActionCallbacks::FullScreen_IsChecked )
	);

	FGlobalEditorCommonCommands::MapActions(ActionList);
}

FReply FMainFrameActionCallbacks::OnUnhandledKeyDownEvent(const FKeyboardEvent& InKeyboardEvent)
{
	if ( FMainFrameCommands::ActionList->ProcessCommandBindings( InKeyboardEvent ) )
	{
		return FReply::Handled();
	}
	else if( GEditor->PlayWorld && InKeyboardEvent.GetKey() == EKeys::Escape )
	{
		FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked< FLevelEditorModule >(TEXT("LevelEditor"));
		if( LevelEditor.GetLevelEditorTab()->IsForeground()  )
		{
			// Escape that gets this far should end the play map if we have one
			// We have to do this here because nothing else has focus and escape should always end the active PIE session.
			GEditor->RequestEndPlayMap();
		}
	}
	return FReply::Unhandled();
}

bool FMainFrameActionCallbacks::DefaultCanExecuteAction()
{
	return FSlateApplication::Get().IsNormalExecution();
}

void FMainFrameActionCallbacks::ChoosePackagesToSave()
{
	const bool bPromptUserToSave = true;
	const bool bSaveMapPackages = true;
	const bool bSaveContentPackages = true;
	const bool bFastSave = false;
	const bool bClosingEditor = false;
	const bool bNotifyNoPackagesSaved = true;
	FEditorFileUtils::SaveDirtyPackages( bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved );
}

void FMainFrameActionCallbacks::ChoosePackagesToCheckInCompleted(TArray<UPackage*> LoadedPackages, TArray<FString> PackageNames)
{
	if (ChoosePackagesToCheckInNotification.IsValid())
	{
		ChoosePackagesToCheckInNotification.Pin()->ExpireAndFadeout();
	}
	ChoosePackagesToCheckInNotification.Reset();
	
	check(PackageNames.Num() > 0);

	// Prompt the user to ask if they would like to first save any dirty packages they are trying to check-in
	const FEditorFileUtils::EPromptReturnCode UserResponse = FEditorFileUtils::PromptForCheckoutAndSave( LoadedPackages, true, true );

	// If the user elected to save dirty packages, but one or more of the packages failed to save properly OR if the user
	// canceled out of the prompt, don't follow through on the check-in process
	const bool bShouldProceed = ( UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Success || UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Declined );
	if ( bShouldProceed )
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		FSourceControlWindows::PromptForCheckin(PackageNames);
	}
	else
	{
		// If a failure occurred, alert the user that the check-in was aborted. This warning shouldn't be necessary if the user cancelled
		// from the dialog, because they obviously intended to cancel the whole operation.
		if ( UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Failure )
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "SCC_Checkin_Aborted", "Check-in aborted as a result of save failure.") );
		}
	}
}

void FMainFrameActionCallbacks::ChoosePackagesToCheckInCancelled(FSourceControlOperationRef InOperation)
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	SourceControlProvider.CancelOperation(InOperation);

	if (ChoosePackagesToCheckInNotification.IsValid())
	{
		ChoosePackagesToCheckInNotification.Pin()->ExpireAndFadeout();
	}
	ChoosePackagesToCheckInNotification.Reset();
}

void FMainFrameActionCallbacks::ChoosePackagesToCheckInCallback(const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult)
{
	if (ChoosePackagesToCheckInNotification.IsValid())
	{
		ChoosePackagesToCheckInNotification.Pin()->ExpireAndFadeout();
	}
	ChoosePackagesToCheckInNotification.Reset();

	if(InResult == ECommandResult::Succeeded)
	{
		// Get a list of all the checked out packages
		int32 NumSelectedNames = 0;
		int32 NumSelectedPackages = 0;
		TArray<FString> PackageNames;
		TArray<UPackage*> LoadedPackages;
		TMap<FString, FSourceControlStatePtr> PackageStates;
		FEditorFileUtils::FindAllSubmittablePackageFiles( PackageStates, true );
		for (TMap<FString, FSourceControlStatePtr>::TConstIterator PackageIter(PackageStates); PackageIter; ++PackageIter)
		{
			const FString Filename = *PackageIter.Key();
			const FString PackageName = FPackageName::FilenameToLongPackageName(Filename);
			const FSourceControlStatePtr CurPackageSCCState = PackageIter.Value();
				
			UPackage* Package = FindPackage(NULL, *PackageName);

			// Put pre-selected items at the start of the list
			if (CurPackageSCCState.IsValid() && CurPackageSCCState->IsSourceControlled())
			{
				if (Package != NULL)
				{
					LoadedPackages.Insert(Package, NumSelectedPackages++);
				}
				PackageNames.Insert(PackageName, NumSelectedNames++);
			}
			else
			{
				if (Package != NULL)
				{
					LoadedPackages.Add(Package);
				}
				PackageNames.Add(PackageName);
			}
		}

		if ( PackageNames.Num() > 0 )
		{
			ChoosePackagesToCheckInCompleted(LoadedPackages, PackageNames);
		}
		else
		{
			FMessageLog EditorErrors("EditorErrors");
			EditorErrors.Warning(LOCTEXT( "NoAssetsToCheckIn", "No assets to check in!"));
			EditorErrors.Notify();
		}
	}
	else if(InResult == ECommandResult::Failed)
	{
		FMessageLog EditorErrors("EditorErrors");
		EditorErrors.Warning(LOCTEXT( "CheckInOperationFailed", "Failed checking source control status!"));
		EditorErrors.Notify();
	}
}

void FMainFrameActionCallbacks::ChoosePackagesToCheckIn()
{
	if ( ISourceControlModule::Get().IsEnabled() )
	{
		if( ISourceControlModule::Get().GetProvider().IsAvailable() )
		{
			// make sure we update the SCC status of all packages (this could take a long time, so we will run it as a background task)
			TArray<FString> Packages;
			FEditorFileUtils::FindAllPackageFiles(Packages);
			
			ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
			FSourceControlOperationRef Operation = ISourceControlOperation::Create<FUpdateStatus>();
			SourceControlProvider.Execute(Operation, SourceControlHelpers::PackageFilenames(Packages), EConcurrency::Asynchronous, FSourceControlOperationComplete::CreateStatic(&FMainFrameActionCallbacks::ChoosePackagesToCheckInCallback));

			if (ChoosePackagesToCheckInNotification.IsValid())
			{
				ChoosePackagesToCheckInNotification.Pin()->ExpireAndFadeout();
			}

			FNotificationInfo Info(LOCTEXT("ChooseAssetsToCheckInIndicator", "Checking for assets to check in..."));
			Info.bFireAndForget = false;
			Info.ExpireDuration = 0.0f;
			Info.FadeOutDuration = 1.0f;

			if(SourceControlProvider.CanCancelOperation(Operation))
			{
				Info.ButtonDetails.Add(FNotificationButtonInfo(
					LOCTEXT("ChoosePackagesToCheckIn_CancelButton", "Cancel"), 
					LOCTEXT("ChoosePackagesToCheckIn_CancelButtonTooltip", "Cancel the check in operation."), 
					FSimpleDelegate::CreateStatic(&FMainFrameActionCallbacks::ChoosePackagesToCheckInCancelled, Operation)
					));
			}

			ChoosePackagesToCheckInNotification = FSlateNotificationManager::Get().AddNotification(Info);

			if (ChoosePackagesToCheckInNotification.IsValid())
			{
				ChoosePackagesToCheckInNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
			}
		}
		else
		{
			FMessageLog EditorErrors("EditorErrors");
			EditorErrors.Warning(LOCTEXT( "NoSCCConnection", "No connection to source control available!"));
			EditorErrors.Notify();
		}
	}
}

bool FMainFrameActionCallbacks::CanChoosePackagesToCheckIn()
{
	return !ChoosePackagesToCheckInNotification.IsValid();
}

void FMainFrameActionCallbacks::ConnectToSourceControl()
{
	ISourceControlModule::Get().ShowLoginDialog(FSourceControlLoginClosed(), ELoginWindowMode::Modeless);
}

void FMainFrameActionCallbacks::SaveAll()
{
	const bool bPromptUserToSave = false;
	const bool bSaveMapPackages = true;
	const bool bSaveContentPackages = true;
	const bool bFastSave = false;
	FEditorFileUtils::SaveDirtyPackages( bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave );
}

TArray<FString> FMainFrameActionCallbacks::ProjectNames;

void FMainFrameActionCallbacks::CacheProjectNames()
{
	ProjectNames.Empty();

	// The switch project menu is filled with recently opened project files
	ProjectNames = GEditor->GetGameAgnosticSettings().RecentlyOpenedProjectFiles;
}

void FMainFrameActionCallbacks::NewProject( bool bAllowProjectOpening, bool bAllowProjectCreate )
{
	if (GUnrealEd->WarnIfLightingBuildIsCurrentlyRunning())
	{
		return;
	}

	TSharedRef<SWindow> NewProjectWindow =
		SNew(SWindow)
		.Title(LOCTEXT( "SelectProjectWindowHeader", "Select Project"))
		.ClientSize( FVector2D(1280, 720) )
		.SizingRule( ESizingRule::FixedSize )
		.SupportsMinimize(false) .SupportsMaximize(false);

	NewProjectWindow->SetContent( FGameProjectGenerationModule::Get().CreateGameProjectDialog(bAllowProjectOpening, bAllowProjectCreate) );

	IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	if (MainFrameModule.GetParentWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(NewProjectWindow, MainFrameModule.GetParentWindow().ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(NewProjectWindow);
	}
}

void FMainFrameActionCallbacks::AddCodeToProject()
{
	FGameProjectGenerationModule::Get().OpenAddCodeToProjectDialog();
}

void FMainFrameActionCallbacks::CookContent( const FString InPlatformName, const FText PlatformDisplayName )
{
	FString PlatformName = InPlatformName;
	FString OptionalParams;

	if (PlatformName == "MacNoEditor")
	{
		OptionalParams += TEXT(" -targetplatform=Mac");
	}

	FString ExecutableName = FPlatformProcess::ExecutableName(false);

	// for things like Android_ATC, pull it apart to be Android, with cook flavor ATC
	int32 UnderscoreLoc;
	if (PlatformName.FindChar(TEXT('_'), UnderscoreLoc))
	{
		OptionalParams += FString(TEXT(" -cookflavor=")) + PlatformName.Mid(UnderscoreLoc + 1);
		PlatformName = PlatformName.Mid(0, UnderscoreLoc);
	}

#if PLATFORM_WINDOWS
	// turn UE4editor into UE4editor-cmd
	if(ExecutableName.EndsWith(".exe", ESearchCase::IgnoreCase) && !FPaths::GetBaseFilename(ExecutableName).EndsWith("-cmd", ESearchCase::IgnoreCase))
	{
		FString NewExeName = ExecutableName.Left(ExecutableName.Len() - 4) + "-Cmd.exe";
		if (FPaths::FileExists(NewExeName))
		{
			ExecutableName = NewExeName;
		}
	}
#endif
	const bool bRunningDebug = FParse::Param(FCommandLine::Get(), TEXT("debug"));

	if (bRunningDebug)
	{
		OptionalParams += TEXT(" -UseDebugParamForEditorExe");
	}

	FString ProjectPath = FPaths::IsProjectFilePathSet() ? FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()) : FPaths::RootDir() / FApp::GetGameName() / FApp::GetGameName() + TEXT(".uproject");
	FString CommandLine = FString::Printf(TEXT("BuildCookRun %s%s -nop4 -project=\"%s\" -cook -allmaps -%s -ue4exe=%s %s"),
		FRocketSupport::IsRocket() ? TEXT("-rocket -nocompile") : TEXT("-nocompileeditor"),
		FApp::IsEngineInstalled() ? TEXT(" -installed") : TEXT(""),
		*ProjectPath,
		*PlatformName,
		*ExecutableName,
		*OptionalParams
	);

	CreateUatTask(CommandLine, PlatformDisplayName, LOCTEXT("CookingContentTaskName", "Cooking content"), LOCTEXT("CookingTaskName", "Cooking"), FEditorStyle::GetBrush(TEXT("MainFrame.CookContent")));
}

bool FMainFrameActionCallbacks::CookContentCanExecute( const FString PlatformName )
{
	return true;
}

void FMainFrameActionCallbacks::PackageBuildConfiguration( EProjectPackagingBuildConfigurations BuildConfiguration )
{
	UProjectPackagingSettings* PackagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
	PackagingSettings->BuildConfiguration = BuildConfiguration;
}

bool FMainFrameActionCallbacks::PackageBuildConfigurationIsChecked( EProjectPackagingBuildConfigurations BuildConfiguration )
{
	return (GetDefault<UProjectPackagingSettings>()->BuildConfiguration == BuildConfiguration);
}

void FMainFrameActionCallbacks::PackageProject( const FString InPlatformName, const FText PlatformDisplayName )
{
	FString PlatformName = InPlatformName;

	// does the project have any code?
	FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	bool bProjectHasCode = GameProjectModule.Get().GetProjectCodeFileCount() > 0;

	const ITargetPlatform* Platform = GetTargetPlatformManager()->FindTargetPlatform(PlatformName);
	if (Platform)
	{
		FString NotInstalledDocLink;
		if (!Platform->IsSdkInstalled(bProjectHasCode, NotInstalledDocLink))
		{
			// broadcast this, and assume someone will pick it up
			IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			MainFrameModule.BroadcastMainFrameSDKNotInstalled(PlatformName, NotInstalledDocLink);
			return;
		}
	}


	UProjectPackagingSettings* PackagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());

	// let the user pick a target directory
	if (PackagingSettings->StagingDirectory.Path.IsEmpty())
	{
		PackagingSettings->StagingDirectory.Path = FPaths::GameDir();
	}

	FString OutFolderName;

	if (!FDesktopPlatformModule::Get()->OpenDirectoryDialog(NULL, LOCTEXT("PackageDirectoryDialogTitle", "Package project...").ToString(), PackagingSettings->StagingDirectory.Path, OutFolderName))
	{
		return;
	}

	PackagingSettings->StagingDirectory.Path = OutFolderName;
	PackagingSettings->SaveConfig();

	// create the packager process
	FString OptionalParams;
	
	if (PackagingSettings->FullRebuild)
	{
		OptionalParams += TEXT(" -clean");
	}

	if (PackagingSettings->UsePakFile)
	{
		OptionalParams += TEXT(" -pak");
	}

	if (PackagingSettings->ForDistribution)
	{
		OptionalParams += TEXT(" -distribution");
	}

	if (PlatformName == "WindowsNoEditor")
	{
		if (PackagingSettings->BuildConfiguration == PPBC_Shipping)
		{
			OptionalParams += TEXT(" -targetplatform=Win32");
		}
		else
		{
			OptionalParams += TEXT(" -targetplatform=Win64");
		}
	}
	else if (PlatformName == "MacNoEditor")
	{
		OptionalParams += TEXT(" -targetplatform=Mac");
	}

	// only build if the project has code that might need to be built
	if (bProjectHasCode && FSourceCodeNavigation::IsCompilerAvailable())
	{
		OptionalParams += TEXT(" -build");
	}

	// for things like Android_ATC, pull it apart to be Android, with cook flavor ATC
	int32 UnderscoreLoc;
	if (PlatformName.FindChar(TEXT('_'), UnderscoreLoc))
	{
		OptionalParams += FString(TEXT(" -cookflavor=")) + PlatformName.Mid(UnderscoreLoc + 1);
		PlatformName = PlatformName.Mid(0, UnderscoreLoc);
	}

	const int32 DirCount = PackagingSettings->DirectoriesToAlwaysCook.Num();
	if(DirCount> 0)
	{
		FString Dirs = TEXT(" -cookdir=");
		for(int32 DirIndex = 0; DirIndex < DirCount; DirIndex++)
		{
			Dirs += FString::Printf(TEXT("\"%s\""), *PackagingSettings->DirectoriesToAlwaysCook[DirIndex].Path);
			if(DirCount > 1 && DirIndex != DirCount - 1)
			{
				Dirs += TEXT("+");
			}
		}

		OptionalParams += Dirs;
	}

	FString ExecutableName = FPlatformProcess::ExecutableName(false);
#if PLATFORM_WINDOWS
	// turn UE4editor into UE4editor.cmd
	if(ExecutableName.EndsWith(".exe", ESearchCase::IgnoreCase) && !FPaths::GetBaseFilename(ExecutableName).EndsWith("-cmd", ESearchCase::IgnoreCase))
	{
		FString NewExeName = ExecutableName.Left(ExecutableName.Len() - 4) + "-Cmd.exe";
		if (FPaths::FileExists(NewExeName))
		{
			ExecutableName = NewExeName;
		}
	}
#endif

	const bool bRunningDebug = FParse::Param(FCommandLine::Get(), TEXT("debug"));

	if (bRunningDebug)
	{
		OptionalParams += TEXT(" -UseDebugParamForEditorExe");
	}

	FString ProjectPath = FPaths::IsProjectFilePathSet() ? FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()) : FPaths::RootDir() / FApp::GetGameName() / FApp::GetGameName() + TEXT(".uproject");
	FString CommandLine = FString::Printf(TEXT("BuildCookRun %s%s -nop4 -project=\"%s\" -cook -allmaps -stage -archive -archivedirectory=\"%s\" -package -%s -clientconfig=%s -ue4exe=%s %s"),
		FRocketSupport::IsRocket() ? TEXT( "-rocket -nocompile" ) : TEXT( "-nocompileeditor" ),
		FApp::IsEngineInstalled() ? TEXT(" -installed") : TEXT(""),
		*ProjectPath,
		*PackagingSettings->StagingDirectory.Path,
		*PlatformName,
		*FindObject<UEnum>(ANY_PACKAGE, TEXT("EProjectPackagingBuildConfigurations"))->GetEnumString(PackagingSettings->BuildConfiguration),
		*ExecutableName,
		*OptionalParams
	);

	CreateUatTask(CommandLine, PlatformDisplayName, LOCTEXT("PackagingProjectTaskName", "Packaging project"), LOCTEXT("PackagingTaskName", "Packaging"), FEditorStyle::GetBrush(TEXT("MainFrame.PackageProject")));
}

bool FMainFrameActionCallbacks::PackageProjectCanExecute( const FString PlatformName, bool IsImplemented )
{
	return IsImplemented;
}

void FMainFrameActionCallbacks::RefreshCodeProject()
{
	if ( !FSourceCodeNavigation::IsCompilerAvailable() )
	{
		// Attempt to trigger the tutorial if the user doesn't have a compiler installed for the project.
		FSourceCodeNavigation::AccessOnCompilerNotFound().Broadcast();
	}

	FText FailReason;
	if(!FGameProjectGenerationModule::Get().UpdateCodeProject(FailReason))
	{
		FSlateNotificationManager::Get().AddNotification(FNotificationInfo(FailReason));
	}
}

void FMainFrameActionCallbacks::OpenIDE()
{
	if ( !FSourceCodeNavigation::IsCompilerAvailable() )
	{
		// Attempt to trigger the tutorial if the user doesn't have a compiler installed for the project.
		FSourceCodeNavigation::AccessOnCompilerNotFound().Broadcast();
	}
	else
	{
		if ( !FSourceCodeNavigation::OpenModuleSolution() )
		{
			const FString FullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead( *FModuleManager::Get().GetSolutionFilepath() );
			const FText Message = FText::Format( LOCTEXT( "OpenIDEFailed_MissingFile", "Could not open {0} for project {1}" ), FSourceCodeNavigation::GetSuggestedSourceCodeIDE(), FText::FromString( FullPath ) );
			FMessageDialog::Open( EAppMsgType::Ok, Message );
		}
	}
}

void FMainFrameActionCallbacks::PackagingSettings()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Game", "Packaging");
}

void FMainFrameActionCallbacks::SwitchProjectByIndex( int32 ProjectIndex )
{
	FUnrealEdMisc::Get().SwitchProject( ProjectNames[ ProjectIndex ] );
}

void FMainFrameActionCallbacks::SwitchProject(const FString& GameOrProjectFileName)
{
	FUnrealEdMisc::Get().SwitchProject( GameOrProjectFileName );
}

void FMainFrameActionCallbacks::SaveLayout()
{
	FGlobalTabmanager::Get()->SaveAllVisualState();

	// Write the saved state's config to disk
	GConfig->Flush( false, GEditorUserSettingsIni );
}

void FMainFrameActionCallbacks::ToggleFullscreen_Execute()
{
	if ( GIsEditor && FApp::HasGameName() )
	{
		static TWeakPtr<SDockTab> LevelEditorTabPtr = FGlobalTabmanager::Get()->InvokeTab(FTabId("LevelEditor"));
		const TSharedPtr<SWindow> LevelEditorWindow = FSlateApplication::Get().FindWidgetWindow( LevelEditorTabPtr.Pin().ToSharedRef() );

		if (LevelEditorWindow->GetWindowMode() == EWindowMode::Windowed)
		{
			LevelEditorWindow->SetWindowMode(EWindowMode::WindowedFullscreen);
		}
		else
		{
			LevelEditorWindow->SetWindowMode(EWindowMode::Windowed);
		}
	}	
}

bool FMainFrameActionCallbacks::FullScreen_IsChecked()
{
	const TWeakPtr<SDockTab> LevelEditorTabPtr = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>( "LevelEditor" ).GetLevelEditorTab();

	const TSharedPtr<SWindow> LevelEditorWindow = LevelEditorTabPtr.IsValid()
		? LevelEditorTabPtr.Pin()->GetParentWindow()
		: TSharedPtr<SWindow>();

	return (LevelEditorWindow.IsValid())
		? (LevelEditorWindow->GetWindowMode() != EWindowMode::Windowed)
		: false;
}


bool FMainFrameActionCallbacks::CanSwitchToProject( int32 InProjectIndex )
{
	if ( FApp::HasGameName() && ProjectNames[ InProjectIndex ].StartsWith( GGameName ) )
	{
		return false;
	}

	if ( FPaths::IsProjectFilePathSet() && ProjectNames[ InProjectIndex ] == FPaths::GetProjectFilePath() )
	{
		return false;
	}

	return true;
}

bool FMainFrameActionCallbacks::IsSwitchProjectChecked( int32 InProjectIndex )
{
	return CanSwitchToProject( InProjectIndex ) == false;
}


void FMainFrameActionCallbacks::Exit()
{
	FSlateApplication::Get().LeaveDebuggingMode();
	// Shut down the editor
	// NOTE: We can't close the editor from within this stack frame as it will cause various DLLs
	//       (such as MainFrame) to become unloaded out from underneath the code pointer.  We'll shut down
	//       as soon as it's safe to do so.
	GEngine->DeferredCommands.Add( TEXT("CLOSE_SLATE_MAINFRAME"));
}


bool FMainFrameActionCallbacks::Undo_CanExecute()
{
	return GUnrealEd->Trans->CanUndo() && FSlateApplication::Get().IsNormalExecution();
}

bool FMainFrameActionCallbacks::Redo_CanExecute()
{
	return GUnrealEd->Trans->CanRedo() && FSlateApplication::Get().IsNormalExecution();
}


void FMainFrameActionCallbacks::ExecuteExecCommand( FString Command )
{
	GUnrealEd->Exec( GEditor->GetEditorWorldContext(true).World(), *Command );
}


void FMainFrameActionCallbacks::OpenSlateApp_ViaModule( FName AppName, FName ModuleName )
{
	FModuleManager::Get().LoadModule( ModuleName );
	OpenSlateApp( AppName );
}

void FMainFrameActionCallbacks::OpenSlateApp( FName AppName )
{
	FGlobalTabmanager::Get()->InvokeTab(FTabId(AppName));
}

bool FMainFrameActionCallbacks::OpenSlateApp_IsChecked( FName AppName )
{
	return false;
}

void FMainFrameActionCallbacks::VisitAskAQuestionPage()
{
	FString AskAQuestionURL;
	if(FUnrealEdMisc::Get().GetURL( TEXT("AskAQuestionURL"), AskAQuestionURL, true ))
	{
		FPlatformProcess::LaunchURL( *AskAQuestionURL, NULL, NULL );
	}
}


void FMainFrameActionCallbacks::VisitSearchForAnswersPage()
{
	FString SearchForAnswersURL;
	if(FUnrealEdMisc::Get().GetURL( TEXT("SearchForAnswersURL"), SearchForAnswersURL, true ))
	{
		FPlatformProcess::LaunchURL( *SearchForAnswersURL, NULL, NULL );
	}
}


void FMainFrameActionCallbacks::VisitSupportWebSite()
{
	FString SupportWebsiteURL;
	if(FUnrealEdMisc::Get().GetURL( TEXT("SupportWebsiteURL"), SupportWebsiteURL, true ))
	{
		FPlatformProcess::LaunchURL( *SupportWebsiteURL, NULL, NULL );
	}
}


void FMainFrameActionCallbacks::VisitEpicGamesDotCom()
{
	FString EpicGamesURL;
	if(FUnrealEdMisc::Get().GetURL( TEXT("EpicGamesURL"), EpicGamesURL ))
	{
		FPlatformProcess::LaunchURL( *EpicGamesURL, NULL, NULL );
	}
}

void FMainFrameActionCallbacks::VisitWiki()
{
	FString URL;
	if (FUnrealEdMisc::Get().GetURL(TEXT("WikiURL"), URL))
	{
		FPlatformProcess::LaunchURL(*URL, NULL, NULL);
	}
}

void FMainFrameActionCallbacks::VisitForums()
{
	FString URL;
	if (FUnrealEdMisc::Get().GetURL(TEXT("ForumsURL"), URL))
	{
		FPlatformProcess::LaunchURL(*URL, NULL, NULL);
	}
}

void FMainFrameActionCallbacks::AboutUnrealEd_Execute()
{
	const FText AboutWindowTitle = LOCTEXT( "AboutUnrealEditor", "About Unreal Editor" );

	TSharedPtr<SWindow> AboutWindow = 
		SNew(SWindow)
		.Title( AboutWindowTitle )
		.ClientSize(FVector2D(600.f, 200.f))
		.SupportsMaximize(false) .SupportsMinimize(false)
		.SizingRule( ESizingRule::FixedSize )
		[
			SNew(SAboutScreen)
		];

	IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>( "MainFrame" );
	TSharedPtr<SWindow> ParentWindow = MainFrame.GetParentWindow();

	if ( ParentWindow.IsValid() )
	{
		FSlateApplication::Get().AddWindowAsNativeChild(AboutWindow.ToSharedRef(), ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(AboutWindow.ToSharedRef());
	}
}


/* FMainFrameActionCallbacks implementation
 *****************************************************************************/

void FMainFrameActionCallbacks::CreateUatTask( const FString& CommandLine, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon )
{
	// make sure that the UAT batch file is in place
#if PLATFORM_WINDOWS
	FString RunUATScriptName = TEXT("RunUAT.bat");
	FString CmdExe = TEXT("cmd.exe");
#else
	FString RunUATScriptName = TEXT("RunUAT.command");
	FString CmdExe = TEXT("/bin/sh");
#endif

	FString UatPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Build/BatchFiles") / RunUATScriptName);

	if (!FPaths::FileExists(UatPath))
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("File"), FText::FromString(UatPath));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("RequiredFileNotFoundMessage", "A required file could not be found:\n{File}"), Arguments));
		
		return;
	}

#if PLATFORM_WINDOWS
	FString FullCommandLine = FString::Printf(TEXT("/c \"\"%s\" %s\""), *UatPath, *CommandLine);
#else
	FString FullCommandLine = FString::Printf(TEXT("\"%s\" %s"), *UatPath, *CommandLine);
#endif

	TSharedPtr<FMonitoredProcess> UatProcess = MakeShareable(new FMonitoredProcess(CmdExe, FullCommandLine, true));

	// create notification item
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);
	FNotificationInfo Info( FText::Format( LOCTEXT("UatTaskInProgressNotification", "{TaskName} for {Platform}..."), Arguments) );
	
	Info.Image = TaskIcon;
	Info.bFireAndForget = false;
	Info.ExpireDuration = 3.0f;
	Info.Hyperlink = FSimpleDelegate::CreateStatic(&FMainFrameActionCallbacks::HandleUatHyperlinkNavigate);
	Info.ButtonDetails.Add(
		FNotificationButtonInfo(
			LOCTEXT("UatTaskCancel", "Cancel"),
			LOCTEXT("UatTaskCancelToolTip", "Cancels execution of this task."),
			FSimpleDelegate::CreateStatic(&FMainFrameActionCallbacks::HandleUatCancelButtonClicked, UatProcess)
		)
	);

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

	if (!NotificationItem.IsValid())
	{
		return;
	}

	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

	// launch the packager
	TWeakPtr<SNotificationItem> NotificationItemPtr(NotificationItem);

	UatProcess->OnCanceled().BindStatic(&FMainFrameActionCallbacks::HandleUatProcessCanceled, NotificationItemPtr, PlatformDisplayName, TaskShortName);
	UatProcess->OnCompleted().BindStatic(&FMainFrameActionCallbacks::HandleUatProcessCompleted, NotificationItemPtr, PlatformDisplayName, TaskShortName);
	UatProcess->OnOutput().BindStatic(&FMainFrameActionCallbacks::HandleUatProcessOutput, NotificationItemPtr, PlatformDisplayName, TaskShortName);

	if (UatProcess->Launch())
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileStart_Cue.CompileStart_Cue"));
	}
	else
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));

		NotificationItem->SetText(LOCTEXT("UatLaunchFailedNotification", "Failed to launch Unreal Automation Tool (UAT)!"));
		NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
		NotificationItem->ExpireAndFadeout();
	}
}


/* FMainFrameActionCallbacks callbacks
 *****************************************************************************/

class FMainFrameActionsNotificationTask
{
public:

	FMainFrameActionsNotificationTask( TWeakPtr<SNotificationItem> InNotificationItemPtr, SNotificationItem::ECompletionState InCompletionState, const FText& InText )
		: CompletionState(InCompletionState)
		, NotificationItemPtr(InNotificationItemPtr)
		, Text(InText)
	{ }

	void DoTask( ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent )
	{
		if (NotificationItemPtr.IsValid())
		{
			if (CompletionState == SNotificationItem::CS_Fail)
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
			}
			else
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
			}
			
			TSharedPtr<SNotificationItem> NotificationItem = NotificationItemPtr.Pin();
			NotificationItem->SetText(Text);
			NotificationItem->SetCompletionState(CompletionState);
			NotificationItem->ExpireAndFadeout();
		}
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
	ENamedThreads::Type GetDesiredThread( ) { return ENamedThreads::GameThread; }
	static const TCHAR* GetTaskName( ) { return TEXT("FMainFrameActionsNotificationTask"); }
	FORCEINLINE static TStatId GetStatId()
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMainFrameActionsNotificationTask, STATGROUP_TaskGraphTasks);
	}

private:

	SNotificationItem::ECompletionState CompletionState;
	TWeakPtr<SNotificationItem> NotificationItemPtr;
	FText Text;
};


void FMainFrameActionCallbacks::HandleUatHyperlinkNavigate( )
{
	FGlobalTabmanager::Get()->InvokeTab(FName("OutputLog"));
}


void FMainFrameActionCallbacks::HandleUatCancelButtonClicked( TSharedPtr<FMonitoredProcess> PackagerProcess )
{
	if (PackagerProcess.IsValid())
	{
		PackagerProcess->Cancel(true);
	}
}


void FMainFrameActionCallbacks::HandleUatProcessCanceled( TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName )
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);

	TGraphTask<FMainFrameActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
		NotificationItemPtr,
		SNotificationItem::CS_Fail,
		FText::Format(LOCTEXT("UatProcessFailedNotification", "{TaskName} canceled!"), Arguments)
	);

//	FMessageLog("PackagingResults").Warning(FText::Format(LOCTEXT("UatProcessCanceledMessageLog", "{TaskName} for {Platform} canceled by user"), Arguments));
}


void FMainFrameActionCallbacks::HandleUatProcessCompleted( int32 ReturnCode, TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName )
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);

	if (ReturnCode == 0)
	{
		TGraphTask<FMainFrameActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
			NotificationItemPtr,
			SNotificationItem::CS_Success,
			FText::Format(LOCTEXT("UatProcessSucceededNotification", "{TaskName} complete!"), Arguments)
		);

//		FMessageLog("PackagingResults").Info(FText::Format(LOCTEXT("UatProcessSuccessMessageLog", "{TaskName} for {Platform} completed successfully"), Arguments));
	}
	else
	{
		TGraphTask<FMainFrameActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
			NotificationItemPtr,
			SNotificationItem::CS_Fail,
			FText::Format(LOCTEXT("PackagerFailedNotification", "{TaskName} failed!"), Arguments)
		);

//		FMessageLog("PackagingResults").Info(FText::Format(LOCTEXT("UatProcessFailedMessageLog", "{TaskName} for {Platform} failed"), Arguments));
	}
}


void FMainFrameActionCallbacks::HandleUatProcessOutput( FString Output, TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName )
{
	if (!Output.IsEmpty())
	{
		UE_LOG(MainFrameActions, Log, TEXT("%s (%s): %s"), *TaskName.ToString(), *PlatformDisplayName.ToString(), *Output);
	}	
}


#undef LOCTEXT_NAMESPACE
