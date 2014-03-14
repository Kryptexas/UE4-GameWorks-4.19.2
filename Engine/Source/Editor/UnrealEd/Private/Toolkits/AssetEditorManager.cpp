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

	if(OpenAssets.Num() > 0)
	{
		bool bRestore = false;
		if(!bCleanShutdown)
		{
			if(FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("RestoreOpenAssetsAfterCrash", "The Editor did not shut down cleanly, would you like to attempt to restore previously open assets now?")) == EAppReturnType::Yes)
			{
				bRestore = true;
			}
		}
		else
		{
			bRestore = GetDefault<UEditorLoadingSavingSettings>()->bRestoreOpenAssetTabsOnRestart;

			if(!bRestore)
			{
				// assets to restore, but we haven't been explicitly told not to restore them, so check now
				FSuppressableWarningDialog::FSetupInfo Info(LOCTEXT("RestoreOpenAssetsAfterClose_Message", "Assets were open when the Editor was last closed, would you like to restore them now?"), LOCTEXT("RestoreOpenAssetsAfterClose_Title", "Reopen assets"), TEXT("SuppressPreviouslyOpenAssetsDialog"));
				Info.ConfirmText = LOCTEXT("RestoreOpenAssetsAfterClose_Confirm", "Restore Open Assets");
				Info.CancelText = LOCTEXT("RestoreOpenAssetsAfterClose_Cancel", "Don't Open Assets");
				FSuppressableWarningDialog RestoreAssetsDialog( Info );

				if(RestoreAssetsDialog.ShowModal() == FSuppressableWarningDialog::EResult::Confirm)
				{
					GetMutableDefault<UEditorLoadingSavingSettings>()->bRestoreOpenAssetTabsOnRestart = true;
					bRestore = true;
				}
			}
		}

		if(bRestore)
		{
			for( const FString& AssetName : OpenAssets )
			{
				OpenEditorForAsset(AssetName);
			}
		}
	}

	SaveOpenAssetEditors(false);
}

void FAssetEditorManager::SaveOpenAssetEditors(bool bOnShutdown)
{
	if(!bSavingOnShutdown)
	{
		TArray<FString> OpenAssets;

		// Dont save a list of assets to restore if we are running under a debugger
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