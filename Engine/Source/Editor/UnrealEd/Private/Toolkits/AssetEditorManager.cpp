// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "Toolkits/AssetEditorManager.h"
#include "ObjectTools.h"
#include "AssetToolsModule.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/SimpleAssetEditor.h"
#include "LevelEditor.h"
#include "EngineAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "ToolkitManager.h"
#include "BlueprintEditorModule.h"

#define LOCTEXT_NAMESPACE "AssetEditorManager"

FAssetEditorManager* FAssetEditorManager::Instance = NULL;


FAssetEditorManager& FAssetEditorManager::Get()
{
	if (Instance == NULL)
	{
		Instance = new FAssetEditorManager();
	}

	return *Instance;
}

FAssetEditorManager::FAssetEditorManager()
	: bSavingOnShutdown(false)
{
	// Message bus to receive requests to load assets
	MessageEndpoint = FMessageEndpoint::Builder("FAssetEditorManager")
		.Handling<FAssetEditorRequestOpenAsset>(this, &FAssetEditorManager::HandleRequestOpenAssetMessage)
		.WithInbox();

	if (MessageEndpoint.IsValid())
	{
		MessageEndpoint->Subscribe<FAssetEditorRequestOpenAsset>();
	}

	TickDelegate = FTickerDelegate::CreateRaw(this, &FAssetEditorManager::HandleTicker);
	FTicker::GetCoreTicker().AddTicker(TickDelegate, 1.f);
}

void FAssetEditorManager::OnExit()
{
	SaveOpenAssetEditors(true);

	TGuardValue<bool> GuardOnShutdown(bSavingOnShutdown, true);

	CloseAllAssetEditors();

	// Don't attempt to report usage stats if analytics isn't available
	if(FEngineAnalytics::IsAvailable())
	{
		TArray<FAnalyticsEventAttribute> EditorUsageAttribs;
		for (auto Iter = EditorDurations.CreateConstIterator(); Iter; ++Iter)
		{
			FString EventName = FString::Printf(TEXT("Editor.Usage.%s"), *Iter.Key().ToString());

			FEngineAnalytics::GetProvider().RecordEvent(EventName, TEXT("Duration.Seconds"), FString::Printf( TEXT( "%.1f" ), Iter.Value().GetTotalSeconds()));
		}	
	}
}

void FAssetEditorManager::AddReferencedObjects( FReferenceCollector& Collector )
{
	for (TMultiMap<UObject*, IAssetEditorInstance*>::TIterator It(OpenedAssets); It; ++It)
	{
		UObject* Asset = It.Key();
		if(Asset != NULL)
		{
			Collector.AddReferencedObject( Asset );
		}
	}
	for (TMultiMap<IAssetEditorInstance*, UObject*>::TIterator It(OpenedEditors); It; ++It)
	{
		UObject* Asset = It.Value();
		if(Asset != NULL)
		{
			Collector.AddReferencedObject( Asset );
		}
	}
}


IAssetEditorInstance* FAssetEditorManager::FindEditorForAsset(UObject* Asset, bool bFocusIfOpen)
{
	IAssetEditorInstance** pAssetEditor = OpenedAssets.Find(Asset);
	const bool bEditorOpen = pAssetEditor != NULL;

	if (bEditorOpen && bFocusIfOpen)
	{
		// @todo toolkit minor: We may need to handle this differently for world-centric vs standalone.  (multiple level editors, etc)
		(*pAssetEditor)->FocusWindow(Asset);
	}

	return bEditorOpen ? *pAssetEditor : NULL;
}


void FAssetEditorManager::CloseOtherEditors( UObject* Asset, IAssetEditorInstance* OnlyEditor)
{
	TArray<UObject*> AllAssets;
	for (TMultiMap<UObject*, IAssetEditorInstance*>::TIterator It(OpenedAssets); It; ++It)
	{
		IAssetEditorInstance* Editor = It.Value();
		if(Asset == It.Key() && Editor != OnlyEditor)
		{
			Editor->CloseWindow();
		}
	}
}


TArray<UObject*> FAssetEditorManager::GetAllEditedAssets()
{
	TArray<UObject*> AllAssets;
	for (TMultiMap<UObject*, IAssetEditorInstance*>::TIterator It(OpenedAssets); It; ++It)
	{
		UObject* Asset = It.Key();
		if(Asset != NULL)
		{
			AllAssets.AddUnique(Asset);
		}
	}
	return AllAssets;
}


void FAssetEditorManager::NotifyAssetOpened(UObject* Asset, IAssetEditorInstance* InInstance)
{
	if (!OpenedEditors.Contains(InInstance))
	{
		FOpenedEditorTime EditorTime;
		EditorTime.EditorName = InInstance->GetEditorName();
		EditorTime.OpenedTime = FDateTime::UtcNow();

		OpenedEditorTimes.Add(InInstance, EditorTime);
	}

	OpenedAssets.Add(Asset, InInstance);
	OpenedEditors.Add(InInstance, Asset);

	SaveOpenAssetEditors(false);
}


void FAssetEditorManager::NotifyAssetsOpened( const TArray< UObject* >& Assets, IAssetEditorInstance* InInstance)
{
	for( auto AssetIter = Assets.CreateConstIterator(); AssetIter; ++AssetIter )
	{
		NotifyAssetOpened(*AssetIter, InInstance);
	}
}


void FAssetEditorManager::NotifyAssetClosed(UObject* Asset, IAssetEditorInstance* InInstance)
{
	OpenedEditors.RemoveSingle(InInstance, Asset);
	OpenedAssets.RemoveSingle(Asset, InInstance);

	SaveOpenAssetEditors(false);
}


void FAssetEditorManager::NotifyEditorClosed(IAssetEditorInstance* InInstance)
{
	// Remove all assets associated with the editor
	TArray<UObject*> Assets;
	OpenedEditors.MultiFind(InInstance, /*out*/ Assets);
	for (int32 AssetIndex = 0; AssetIndex < Assets.Num(); ++AssetIndex)
	{
		OpenedAssets.Remove(Assets[AssetIndex], InInstance);
	}

	// Remove the editor itself
	OpenedEditors.Remove(InInstance);
	FOpenedEditorTime EditorTime = OpenedEditorTimes.FindAndRemoveChecked(InInstance);

	// Record the editor open-close duration
	FTimespan* Duration = EditorDurations.Find(EditorTime.EditorName);
	if (Duration == NULL)
	{
		Duration = &EditorDurations.Add(EditorTime.EditorName, FTimespan::Zero());
	}
	*Duration += FDateTime::UtcNow() - EditorTime.OpenedTime;

	SaveOpenAssetEditors(false);
}


bool FAssetEditorManager::CloseAllAssetEditors()
{
	bool bAllEditorsClosed = true;
	for (TMultiMap<IAssetEditorInstance*, UObject*>::TIterator It(OpenedEditors); It; ++It)
	{
		IAssetEditorInstance* Editor = It.Key();
		if (Editor != NULL)
		{
			if ( !Editor->CloseWindow() )
			{
				bAllEditorsClosed = false;
			}
		}
	}

	return bAllEditorsClosed;
}


bool FAssetEditorManager::OpenEditorForAsset(UObject* Asset, const EToolkitMode::Type ToolkitMode, TSharedPtr< IToolkitHost > OpenedFromLevelEditor )
{
	check(Asset);
	// @todo toolkit minor: When "Edit Here" happens in a different level editor from the one that an asset is already
	//    being edited within, we should decide whether to disallow "Edit Here" in that case, or to close the old asset
	//    editor and summon it in the new level editor, or to just foreground the old level editor (current behavior)


	const bool bBringToFrontIfOpen = true;
	
	AssetEditorRequestOpenEvent.Broadcast(Asset);

	if( FindEditorForAsset(Asset, bBringToFrontIfOpen) != NULL )
	{
		// This asset is already open in an editor! (the call to FindEditorForAsset above will bring it to the front)
		return true;
	}
	else
	{
		GWarn->BeginSlowTask( LOCTEXT("OpenEditor", "Opening Editor..."), true);
	}


	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass( Asset->GetClass() );
	
	auto ActualToolkitMode = ToolkitMode;
	if( AssetTypeActions.IsValid() )
	{
		if( AssetTypeActions.Pin()->ShouldForceWorldCentric() )
		{
			// This asset type prefers a specific toolkit mode
			ActualToolkitMode = EToolkitMode::WorldCentric;

			if( !OpenedFromLevelEditor.IsValid() )
			{
				// We don't have a level editor to spawn in world-centric mode, so we'll find one now
				// @todo sequencer: We should eventually eliminate this code (incl include dependencies) or change it to not make assumptions about a single level editor
				OpenedFromLevelEditor = FModuleManager::LoadModuleChecked< FLevelEditorModule >( "LevelEditor" ).GetFirstLevelEditor();
			}
		}
	}
	
	GWarn->EndSlowTask();
	
	if( ActualToolkitMode != EToolkitMode::WorldCentric && OpenedFromLevelEditor.IsValid() )
	{
		// @todo toolkit minor: Kind of lame use of a static variable here to prime the new asset editor.  This was done to avoid refactoring a few dozen files for a very minor change.
		FAssetEditorToolkit::SetPreviousWorldCentricToolkitHostForNewAssetEditor( OpenedFromLevelEditor.ToSharedRef() );
	}

	// Disallow opening an asset editor for classes
	bool bCanSummonSimpleAssetEditor = !Asset->IsA<UClass>();

	if( AssetTypeActions.IsValid() )
	{
		TArray<UObject*> AssetsToEdit;
		AssetsToEdit.Add(Asset);
		AssetTypeActions.Pin()->OpenAssetEditor(AssetsToEdit, ActualToolkitMode == EToolkitMode::WorldCentric ? OpenedFromLevelEditor : TSharedPtr<IToolkitHost>());
		AssetEditorOpenedEvent.Broadcast(Asset);
	}
	else if( bCanSummonSimpleAssetEditor )
	{
		// No asset type actions for this asset. Just use a properties editor.
		FSimpleAssetEditor::CreateEditor(ActualToolkitMode, ActualToolkitMode == EToolkitMode::WorldCentric ? OpenedFromLevelEditor : TSharedPtr<IToolkitHost>(), Asset);
	}

	return true;
}


bool FAssetEditorManager::OpenEditorForAssets( const TArray< UObject* >& Assets, const EToolkitMode::Type ToolkitMode, TSharedPtr< IToolkitHost > OpenedFromLevelEditor )
{
	//@todo Temporarily whenever someone attempts to open an editor editing multiple assets the editor type will default to FSimpleAssetEditor [8/14/2012 Justin.Sargent]

	if( ToolkitMode != EToolkitMode::WorldCentric && OpenedFromLevelEditor.IsValid() )
	{
		// @todo toolkit minor: Kind of lame use of a static variable here to prime the new asset editor.  This was done to avoid refactoring a few dozen files for a very minor change.
		FAssetEditorToolkit::SetPreviousWorldCentricToolkitHostForNewAssetEditor( OpenedFromLevelEditor.ToSharedRef() );
	}

	// No asset type actions for this asset. Just use a properties editor.
	FSimpleAssetEditor::CreateEditor(ToolkitMode, ToolkitMode == EToolkitMode::WorldCentric ? OpenedFromLevelEditor : TSharedPtr<IToolkitHost>(), Assets );

	return true;
}


FArchive& operator<<(FArchive& Ar,IAssetEditorInstance*& TypeRef)
{
	return Ar;
}


void FAssetEditorManager::HandleRequestOpenAssetMessage( const FAssetEditorRequestOpenAsset& Message, const IMessageContextRef& Context )
{
	OpenEditorForAsset(Message.AssetName);
}

void FAssetEditorManager::OpenEditorForAsset(const FString& AssetPathName)
{
	FString FileType = FPaths::GetExtension(AssetPathName, /*bIncludeDot=*/true);

	// Check if a map needs loading
	if (FileType.ToLower() == FPackageName::GetMapPackageExtension().ToLower())
	{
		bool bLoadAsTemplate = false;
		bool bShowProgress = false;

		FEditorFileUtils::LoadMap(AssetPathName, bLoadAsTemplate, bShowProgress);
	}
	else
	{
		// An asset needs loading
		UPackage* Package = LoadPackage(NULL, *AssetPathName, LOAD_NoRedirects);

		if (Package)
		{
			Package->FullyLoad();

			FString AssetName = FPaths::GetBaseFilename(AssetPathName);
			UObject* Object = FindObject<UObject>(Package, *AssetName);

			if (Object != NULL)
			{
				OpenEditorForAsset(Object);
			}
		}
	}
}

bool FAssetEditorManager::HandleTicker( float DeltaTime )
{
	MessageEndpoint->ProcessInbox();

	return true;
}

void FAssetEditorManager::RestorePreviouslyOpenAssets()
{
	TArray<FString> OpenAssets;
	GConfig->GetArray(TEXT("AssetEditorManager"), TEXT("OpenAssetsAtExit"), OpenAssets, GEditorUserSettingsIni);

	bool bCleanShutdown =  false;
	GConfig->GetBool(TEXT("AssetEditorManager"), TEXT("CleanShutdown"), bCleanShutdown, GEditorUserSettingsIni);

	SaveOpenAssetEditors(false);

	if(OpenAssets.Num() > 0)
	{
		if(bCleanShutdown)
		{
			// Do we have permission to automatically re-open the assets, or should we ask?
			const bool bAutoRestore = GetDefault<UEditorLoadingSavingSettings>()->bRestoreOpenAssetTabsOnRestart;

			if(bAutoRestore)
			{
				// Pretend that we showed the notification and that the user clicked "Restore Now"
				OnConfirmRestorePreviouslyOpenAssets(OpenAssets);
			}
			else
			{
				// Has this notification previously been suppressed by the user?
				bool bSuppressNotification = false;
				GConfig->GetBool(TEXT("AssetEditorManager"), TEXT("SuppressRestorePreviouslyOpenAssetsNotification"), bSuppressNotification, GEditorUserSettingsIni);

				if(!bSuppressNotification)
				{
					// Ask the user; this doesn't block so will reopen the assets later
					SpawnRestorePreviouslyOpenAssetsNotification(bCleanShutdown, OpenAssets);
				}
			}
		}
		else
		{
			// If we crashed, we always ask regardless of what the user previously said
			SpawnRestorePreviouslyOpenAssetsNotification(bCleanShutdown, OpenAssets);
 		}
	}
}

void FAssetEditorManager::SpawnRestorePreviouslyOpenAssetsNotification(const bool bCleanShutdown, const TArray<FString>& AssetsToOpen)
{
	/** Utility functions for the notification which don't rely on the state from FAssetEditorManager */
	struct Local
	{
		static ESlateCheckBoxState::Type GetDontAskAgainCheckBoxState()
		{
			bool bSuppressNotification = false;
			GConfig->GetBool(TEXT("AssetEditorManager"), TEXT("SuppressRestorePreviouslyOpenAssetsNotification"), bSuppressNotification, GEditorUserSettingsIni);
			return bSuppressNotification ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
		}

		static void OnDontAskAgainCheckBoxStateChanged(ESlateCheckBoxState::Type NewState)
		{
			const bool bSuppressNotification = (NewState == ESlateCheckBoxState::Checked);
			GConfig->SetBool(TEXT("AssetEditorManager"), TEXT("SuppressRestorePreviouslyOpenAssetsNotification"), bSuppressNotification, GEditorUserSettingsIni);
		}
	};

	FNotificationInfo Info(bCleanShutdown 
		? LOCTEXT("RestoreOpenAssetsAfterClose_Message", "Assets were open when the Editor was last closed, would you like to restore them now?")
		: LOCTEXT("RestoreOpenAssetsAfterCrash", "The Editor did not shut down cleanly, would you like to attempt to restore previously open assets now?")
		);

	// Add the buttons
	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("RestoreOpenAssetsAfterClose_Confirm", "Restore Now"), 
		FText(), 
		FSimpleDelegate::CreateRaw(this, &FAssetEditorManager::OnConfirmRestorePreviouslyOpenAssets, AssetsToOpen), 
		SNotificationItem::CS_None
		));
	Info.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("RestoreOpenAssetsAfterClose_Cancel", "Don't Restore"), 
		FText(), 
		FSimpleDelegate::CreateRaw(this, &FAssetEditorManager::OnCancelRestorePreviouslyOpenAssets), 
		SNotificationItem::CS_None
		));

	// We will be keeping track of this ourselves
	Info.bFireAndForget = false;

	// We want the auto-save to be subtle
	Info.bUseLargeFont = false;
	Info.bUseThrobber = false;
	Info.bUseSuccessFailIcons = false;

	// Only let the user suppress the non-crash version
	if(bCleanShutdown)
	{
		Info.CheckBoxState = TAttribute<ESlateCheckBoxState::Type>::Create(&Local::GetDontAskAgainCheckBoxState);
		Info.CheckBoxStateChanged = FOnCheckStateChanged::CreateStatic(&Local::OnDontAskAgainCheckBoxStateChanged);
		Info.CheckBoxText = NSLOCTEXT("ModalDialogs", "DefaultCheckBoxMessage", "Don't show this again");
	}

	// Close any existing notification
	if(RestorePreviouslyOpenAssetsNotification.IsValid())
	{
		RestorePreviouslyOpenAssetsNotification->ExpireAndFadeout();
		RestorePreviouslyOpenAssetsNotification.Reset();
	}

	RestorePreviouslyOpenAssetsNotification = FSlateNotificationManager::Get().AddNotification(Info);
}

void FAssetEditorManager::OnConfirmRestorePreviouslyOpenAssets(TArray<FString> AssetsToOpen)
{
	// Close any existing notification
	if(RestorePreviouslyOpenAssetsNotification.IsValid())
	{
		RestorePreviouslyOpenAssetsNotification->ExpireAndFadeout();
		RestorePreviouslyOpenAssetsNotification.Reset();

		// If the user suppressed the notification for future sessions, make sure this is reflected in their settings
		// Note: We do that inside this if statement so that we only do it if we were showing a UI they could interact with
		bool bSuppressNotification = false;
		GConfig->GetBool(TEXT("AssetEditorManager"), TEXT("SuppressRestorePreviouslyOpenAssetsNotification"), bSuppressNotification, GEditorUserSettingsIni);
		UEditorLoadingSavingSettings& Settings = *GetMutableDefault<UEditorLoadingSavingSettings>();
		Settings.bRestoreOpenAssetTabsOnRestart = bSuppressNotification;
		Settings.PostEditChange();
	}

	for( const FString& AssetName : AssetsToOpen )
	{
		OpenEditorForAsset(AssetName);
	}
}

void FAssetEditorManager::OnCancelRestorePreviouslyOpenAssets()
{
	// Close any existing notification
	if(RestorePreviouslyOpenAssetsNotification.IsValid())
	{
		RestorePreviouslyOpenAssetsNotification->ExpireAndFadeout();
		RestorePreviouslyOpenAssetsNotification.Reset();
	}
}

void FAssetEditorManager::SaveOpenAssetEditors(bool bOnShutdown)
{
	if(!bSavingOnShutdown)
	{
		TArray<FString> OpenAssets;

		// Don't save a list of assets to restore if we are running under a debugger
		if(!FPlatformMisc::IsDebuggerPresent())
		{
			for (auto EditorPair : OpenedEditors)
			{
				IAssetEditorInstance* Editor = EditorPair.Key;
				if (Editor != NULL)
				{
					UObject* EditedObject = EditorPair.Value;
					if(EditedObject != NULL)
					{
						// only record assets that have a valid saved package
						UPackage* Package = EditedObject->GetOutermost();
						if(Package != NULL && Package->GetFileSize() != 0)
						{
							OpenAssets.Add(EditedObject->GetPathName());
						}
					}
				}
			}
		}

		GConfig->SetArray(TEXT("AssetEditorManager"), TEXT("OpenAssetsAtExit"), OpenAssets, GEditorUserSettingsIni);
		GConfig->SetBool(TEXT("AssetEditorManager"), TEXT("CleanShutdown"), bOnShutdown, GEditorUserSettingsIni);
		GConfig->Flush(false, GEditorUserSettingsIni);
	}
}

#undef LOCTEXT_NAMESPACE