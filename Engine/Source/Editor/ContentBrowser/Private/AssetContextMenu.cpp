// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"
#include "AssetViewTypes.h"
#include "AssetContextMenu.h"
#include "ContentBrowserModule.h"

#include "Editor/UnrealEd/Public/LinkedObjEditor.h"
#include "Editor/UnrealEd/Public/ObjectTools.h"
#include "Editor/UnrealEd/Public/PackageTools.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Toolkits/AssetEditorManager.h"
#include "ConsolidateWindow.h"
#include "ReferenceViewer.h"

#include "ReferencedAssetsUtils.h"

#include "ISourceControlModule.h"
#include "ISourceControlRevision.h"
#include "SourceControlWindows.h"
#include "KismetEditorUtilities.h"
#include "AssetToolsModule.h"
#include "ComponentAssetBroker.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

FAssetContextMenu::FAssetContextMenu(const TWeakPtr<SAssetView>& InAssetView)
	: bAtLeastOneNonRedirectorSelected(false)
	, bCanExecuteSCCCheckOut(false)
	, bCanExecuteSCCOpenForAdd(false)
	, bCanExecuteSCCCheckIn(false)
	, bCanExecuteSCCHistory(false)
	, bCanExecuteSCCRevert(false)
	, bCanExecuteSCCSync(false)
	, AssetView(InAssetView)
{

}

void FAssetContextMenu::BindCommands(TSharedPtr< FUICommandList > InCommandList)
{
	InCommandList->MapAction( FGenericCommands::Get().Rename, FUIAction(
		FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteRename ),
		FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteRename )
		));

	InCommandList->MapAction( FGenericCommands::Get().Delete, FUIAction(
		FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteDelete ),
		FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteDelete )
		));
}

TSharedRef<SWidget> FAssetContextMenu::MakeContextMenu(const TArray<FAssetData>& InSelectedAssets, const FSourcesData& InSourcesData, TSharedPtr< FUICommandList > InCommandList)
{
	SelectedAssets = InSelectedAssets;
	SourcesData = InSourcesData;

	// Cache any vars that are used in determining if you can execute any actions.
	// Useful for actions whose "CanExecute" will not change or is expensive to calculate.
	CacheCanExecuteVars();

	// Get all menu extenders for this context menu from the content browser module
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	TArray<FContentBrowserMenuExtender_SelectedAssets> MenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(SelectedAssets));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, InCommandList, MenuExtender);

	// Only add something if at least one asset is selected
	if ( SelectedAssets.Num() )
	{
		// Add any type-specific context menu options
		AddAssetTypeMenuOptions(MenuBuilder);

		// Add quick access to common commands.
		AddCommonMenuOptions(MenuBuilder);

		// Add reference options
		AddReferenceMenuOptions(MenuBuilder);

		// Add source control options
		AddSourceControlMenuOptions(MenuBuilder);

		// Add collection options
		AddCollectionMenuOptions(MenuBuilder);
	}
	
	return MenuBuilder.MakeWidget();
}

void FAssetContextMenu::SetOnFindInAssetTreeRequested(const FOnFindInAssetTreeRequested& InOnFindInAssetTreeRequested)
{
	OnFindInAssetTreeRequested = InOnFindInAssetTreeRequested;
}

void FAssetContextMenu::SetOnRenameRequested(const FOnRenameRequested& InOnRenameRequested)
{
	OnRenameRequested = InOnRenameRequested;
}

void FAssetContextMenu::SetOnRenameFolderRequested(const FOnRenameFolderRequested& InOnRenameFolderRequested)
{
	OnRenameFolderRequested = InOnRenameFolderRequested;
}

void FAssetContextMenu::SetOnDuplicateRequested(const FOnDuplicateRequested& InOnDuplicateRequested)
{
	OnDuplicateRequested = InOnDuplicateRequested;
}

void FAssetContextMenu::SetOnAssetViewRefreshRequested(const FOnAssetViewRefreshRequested& InOnAssetViewRefreshRequested)
{
	OnAssetViewRefreshRequested = InOnAssetViewRefreshRequested;
}

bool FAssetContextMenu::AddCommonMenuOptions(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("AssetContextActions", LOCTEXT("AssetActionsMenuHeading", "Asset Actions"));
	{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SyncToAssetTree", "Find in Asset Tree"),
		LOCTEXT("SyncToAssetTreeTooltip", "Selects the folder in the asset tree containing this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSyncToAssetTree ),
			FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSyncToAssetTree )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("FindAssetInWorld", "Select Actors Using This Asset"),
		LOCTEXT("FindAssetInWorldTooltip", "Selects all actors referencing this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteFindAssetInWorld ),
			FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteFindAssetInWorld )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Properties", "Details..."),
		LOCTEXT("PropertiesTooltip", "Opens the details for the selected assets."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteProperties ),
			FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteProperties )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("PropertyMatrix", "Property Matrix..."),
		LOCTEXT("PropertyMatrixTooltip", "Opens the property matrix editor for the selected assets."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecutePropertyMatrix ),
		FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteProperties )
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Duplicate", "Create Copy"),
		LOCTEXT("DuplicateTooltip", "Create a copy of the selected assets."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteDuplicate ),
			FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteDuplicate )
			)
		);

	if ( SelectedAssets.Num() == 1 )
	{
		MenuBuilder.AddMenuEntry( FGenericCommands::Get().Rename, NAME_None, 
			LOCTEXT("Rename", "Rename"),
			LOCTEXT("RenameTooltip", "Rename the selected asset.")
			);
	}

	MenuBuilder.AddMenuEntry( FGenericCommands::Get().Delete, NAME_None, 
		LOCTEXT("Delete", "Delete"),
		LOCTEXT("DeleteTooltip", "Delete the selected assets.")
		);

	if ( CanExecuteConsolidate() )
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Consolidate", "Consolidate"),
			LOCTEXT("ConsolidateTooltip", "Consolidate the selected assets into one."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteConsolidate )
				)
			);
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Export", "Export..."),
		LOCTEXT("ExportTooltip", "Export the selected assets to file."),
		FSlateIcon(),
		FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteExport ) )
		);

	if (SelectedAssets.Num() > 1)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("BulkExport", "Bulk Export..."),
			LOCTEXT("BulkExportTooltip", "Export the selected assets to file in the selected directory"),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteBulkExport ) )
			);
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MigrateAsset", "Migrate..."),
		LOCTEXT("MigrateAssetTooltip", "Copies all selected assets and their dependencies to another game"),
		FSlateIcon(),
		FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteMigrateAsset ) )
		);

	MenuBuilder.AddMenuEntry(
		ContentBrowserUtils::GetExploreFolderText(),
		LOCTEXT("FindInExplorerTooltip", "Finds this asset on disk"),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteFindInExplorer ),
		FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteFindInExplorer )
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CreateBlueprintUsing", "Create Blueprint Using..."),
		LOCTEXT("CreateBlueprintUsingTooltip", "Create a new Blueprint and add this asset to it"),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteCreateBlueprintUsing),
		FCanExecuteAction::CreateSP(this, &FAssetContextMenu::CanExecuteCreateBlueprintUsing)
		)
		);


	MenuBuilder.AddMenuEntry(
		LOCTEXT("SaveAsset", "Save"),
		LOCTEXT("SaveAssetTooltip", "Saves the asset to file."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSaveAsset ),
			FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSaveAsset )
			)
		);

	if (CanExecuteDiffSelected())
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("DiffSelected", "Diff Selected"),
			LOCTEXT("DiffSelectedTooltip", "Diff the two assets that you have selected."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetContextMenu::ExecuteDiffSelected)
			)
		);
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if ( SelectedAssets.Num() == 1 && AssetToolsModule.Get().AssetUsesGenericThumbnail(SelectedAssets[0]) )
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CaptureThumbnail", "Capture Thumbnail"),
			LOCTEXT("CaptureThumbnailTooltip", "Captures a thumbnail from the active viewport."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteCaptureThumbnail ),
				FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteCaptureThumbnail )
				)
			);
	}

	if ( CanClearCustomThumbnails() )
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearCustomThumbnail", "Clear Thumbnail"),
			LOCTEXT("ClearCustomThumbnailTooltip", "Clears all custom thumbnails for selected assets."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteClearThumbnail )
				)
			);
	}
	}
	MenuBuilder.EndSection();

	return true;
}

bool FAssetContextMenu::AddReferenceMenuOptions(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("AssetContextReferences", LOCTEXT("ReferencesMenuHeading", "References"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CopyReference", "Copy Reference"),
			LOCTEXT("CopyReferenceTooltip", "Copies reference paths for the selected assets to the clipboard."),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteCopyReference ) )
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ReferenceViewer", "Reference Viewer..."),
			LOCTEXT("ReferenceViewerTooltip", "Shows a graph of references for this asset."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteShowReferenceViewer )
				)
			);
	}
	MenuBuilder.EndSection();

	return true;
}

bool FAssetContextMenu::AddAssetTypeMenuOptions(FMenuBuilder& MenuBuilder)
{
	bool bAnyTypeOptions = false;

	// Objects must be loaded for this operation... for now
	TArray<FString> ObjectPaths;
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		ObjectPaths.Add(SelectedAssets[AssetIdx].ObjectPath.ToString());
	}

	TArray<UObject*> SelectedObjects;
	if ( ContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, SelectedObjects) )
	{
		// Load the asset tools module
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		bAnyTypeOptions = AssetToolsModule.Get().GetAssetActions(SelectedObjects, MenuBuilder, /*bIncludeHeading=*/true);
	}

	return bAnyTypeOptions;
}

bool FAssetContextMenu::AddSourceControlMenuOptions(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("AssetContextSourceControl", LOCTEXT("AssetSCCOptionsMenuHeading", "Source Control"));

	if ( ISourceControlModule::Get().IsEnabled() )
	{
		if( CanExecuteSCCSync() )
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCSync", "Sync"),
				LOCTEXT("SCCSyncTooltip", "Updates the item to the latest version in source control."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCSync ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCSync )
					)
				);
		}

		if ( CanExecuteSCCCheckOut() )
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCCheckOut", "Check Out"),
				LOCTEXT("SCCCheckOutTooltip", "Checks out the selected asset from source control."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCCheckOut ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCCheckOut )
					)
				);
		}

		if ( CanExecuteSCCOpenForAdd() )
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCOpenForAdd", "Mark For Add"),
				LOCTEXT("SCCOpenForAddTooltip", "Adds the selected asset to source control."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCOpenForAdd ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCOpenForAdd )
					)
				);
		}

		if ( CanExecuteSCCCheckIn() )
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCCheckIn", "Check In"),
				LOCTEXT("SCCCheckInTooltip", "Checks in the selected asset to source control."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCCheckIn ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCCheckIn )
					)
				);
		}

		MenuBuilder.AddMenuEntry(
			LOCTEXT("SCCRefresh", "Refresh"),
			LOCTEXT("SCCRefreshTooltip", "Updates the source control status of the asset."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCRefresh ),
				FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCRefresh )
				)
			);

		if( CanExecuteSCCHistory() )
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCHistory", "History"),
				LOCTEXT("SCCHistoryTooltip", "Displays the source control revision history of the selected asset."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCHistory ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCHistory )
					)
				);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCDiffAgainstDepot", "Diff Against Depot"),
				LOCTEXT("SCCDiffAgainstDepotTooltip", "Look at differences between your version of the asset and that in source control."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCDiffAgainstDepot ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCDiffAgainstDepot )
					)
				);	
		}

		if( CanExecuteSCCRevert() )
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("SCCRevert", "Revert"),
				LOCTEXT("SCCRevertTooltip", "Reverts the asset to the state it was before it was checked out."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteSCCRevert ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteSCCRevert )
					)
				);
		}

	}
	else
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SCCConnectToSourceControl", "Connect To Source Control"),
			LOCTEXT("SCCConnectToSourceControlTooltip", "Connect to source control to allow source control operations to be performed on content and levels."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteEnableSourceControl ),
				FCanExecuteAction()
				)
			);		

	}

	MenuBuilder.EndSection();

	return true;
}

bool FAssetContextMenu::AddCollectionMenuOptions(FMenuBuilder& MenuBuilder)
{
	// "Remove from collection" (only display option if exactly one collection is selected)
	if ( SourcesData.Collections.Num() == 1 )
	{
		MenuBuilder.BeginSection("AssetContextCollections", LOCTEXT("AssetCollectionOptionsMenuHeading", "Collections"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RemoveFromCollection", "Remove From Collection"),
				LOCTEXT("RemoveFromCollection_ToolTip", "Removes the selected asset from the current collection."),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP( this, &FAssetContextMenu::ExecuteRemoveFromCollection ),
					FCanExecuteAction::CreateSP( this, &FAssetContextMenu::CanExecuteRemoveFromCollection )
					)
				);
		}
		MenuBuilder.EndSection();

		return true;
	}

	return false;
}

void FAssetContextMenu::ExecuteSyncToAssetTree()
{
	OnFindInAssetTreeRequested.ExecuteIfBound(SelectedAssets);
}

void FAssetContextMenu::ExecuteFindInExplorer()
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		const UObject* Asset = SelectedAssets[AssetIdx].GetAsset();
		if (Asset)
		{
			FAssetData AssetData(Asset);

			const FString FilePath = FPackageName::LongPackageNameToFilename(AssetData.PackageName.ToString());
			const FString FullFilePath = FPaths::ConvertRelativePathToFull(FilePath);
			const FString Filename = FullFilePath + FPackageName::GetAssetPackageExtension();
			FPlatformProcess::ExploreFolder(*Filename);
		}
	}
}

void FAssetContextMenu::ExecuteCreateBlueprintUsing()
{
	if(SelectedAssets.Num() == 1)
	{
		UObject* Asset = SelectedAssets[0].GetAsset();
		FKismetEditorUtilities::CreateBlueprintUsingAsset(Asset, true);
	}
}

void FAssetContextMenu::GetSelectedAssets(TArray<UObject*>& Assets, bool SkipRedirectors)
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		if (!SelectedAssets[AssetIdx].IsAssetLoaded() && FEditorFileUtils::IsMapPackageAsset(SelectedAssets[AssetIdx].ObjectPath.ToString()))
		{
			// Don't load assets in map packages
			continue;
		}

		if (SkipRedirectors && (SelectedAssets[AssetIdx].AssetClass == UObjectRedirector::StaticClass()->GetFName()))
		{
			// Don't operate on Redirectors
			continue;
		}

		UObject* Object = SelectedAssets[AssetIdx].GetAsset();

		if (Object)
		{
			Assets.Add(Object);
		}
	}
}

/** Generates a reference graph of the world and can then find actors referencing specified objects */
struct WorldReferenceGenerator : public FFindReferencedAssets
{
	void BuildReferencingData()
	{
		MarkAllObjects();

		FReferencedAssets* Referencer = new(Referencers) FReferencedAssets(GWorld);
		const int32 MaxRecursionDepth = 0;
		const bool bIncludeClasses = true;
		const bool bIncludeDefaults = false;
		const bool bReverseReferenceGraph = true;

		// Generate the reference graph for GWorld
		FFindAssetsArchive(GWorld, Referencer->AssetList, &ReferenceGraph, MaxRecursionDepth, bIncludeClasses, bIncludeDefaults, bReverseReferenceGraph);
	}

	void MarkAllObjects()
	{
		// Mark all objects so we don't get into an endless recursion
		for (FObjectIterator It; It; ++It)
		{
			It->Mark(OBJECTMARK_TagExp);
		}
	}

	void Generate( const UObject* AssetToFind, TArray< TWeakObjectPtr<UObject> >& OutObjects )
	{
		// Don't examine visited objects
		if (!AssetToFind->HasAnyMarks(OBJECTMARK_TagExp))
		{
			return;
		}

		AssetToFind->UnMark(OBJECTMARK_TagExp);

		// Return once we find a parent object that is an actor
		if (AssetToFind->IsA(AActor::StaticClass()))
		{
			OutObjects.Add(AssetToFind);
			return;
		}

		// Transverse the reference graph looking for actor objects
		TSet<UObject*>* Referencers = ReferenceGraph.Find(AssetToFind);
		if (Referencers)
		{
			for(TSet<UObject*>::TConstIterator SetIt(*Referencers); SetIt; ++SetIt)
			{
				Generate(*SetIt, OutObjects);
			}
		}
	}
};

void FAssetContextMenu::ExecuteFindAssetInWorld()
{
	TArray<UObject*> AssetsToFind;
	const bool SkipRedirectors = true;
	GetSelectedAssets(AssetsToFind, SkipRedirectors);

	const bool NoteSelectionChange = true;
	const bool DeselectBSPSurfs = true;
	const bool WarnAboutManyActors = false;
	GEditor->SelectNone(NoteSelectionChange, DeselectBSPSurfs, WarnAboutManyActors);

	if (AssetsToFind.Num() > 0)
	{
		const bool ShowProgressDialog = true;
		GWarn->BeginSlowTask(NSLOCTEXT("AssetContextMenu", "FindAssetInWorld", "Finding actors that use this asset..."), ShowProgressDialog);

 		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		TArray< TWeakObjectPtr<UObject> > OutObjects;
		WorldReferenceGenerator ObjRefGenerator;
		ObjRefGenerator.BuildReferencingData();

		for (int32 AssetIdx = 0; AssetIdx < AssetsToFind.Num(); ++AssetIdx)
		{
			ObjRefGenerator.MarkAllObjects();
			ObjRefGenerator.Generate(AssetsToFind[AssetIdx], OutObjects);
		}

		if (OutObjects.Num() > 0)
		{
			const bool InSelected = true;
			const bool Notify = false;

			// Select referencing actors
			for (int32 ActorIdx = 0; ActorIdx < OutObjects.Num(); ++ActorIdx)
			{
				GEditor->SelectActor(CastChecked<AActor>(OutObjects[ActorIdx].Get()), InSelected, Notify);
			}

			GEditor->NoteSelectionChange();
		}
		else
		{
			FNotificationInfo Info(LOCTEXT("NoReferencingActorsFound", "No actors found."));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}

		GWarn->EndSlowTask();
	}
}

void FAssetContextMenu::ExecuteProperties()
{
	TArray<UObject*> ObjectsForPropertiesMenu;
	const bool SkipRedirectors = true;
	GetSelectedAssets(ObjectsForPropertiesMenu, SkipRedirectors);

	if ( ObjectsForPropertiesMenu.Num() > 0 )
	{
		FAssetEditorManager::Get().OpenEditorForAssets( ObjectsForPropertiesMenu );
	}
}

void FAssetContextMenu::ExecutePropertyMatrix()
{
	TArray<UObject*> ObjectsForPropertiesMenu;
	const bool SkipRedirectors = true;
	GetSelectedAssets(ObjectsForPropertiesMenu, SkipRedirectors);

	if ( ObjectsForPropertiesMenu.Num() > 0 )
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
		PropertyEditorModule.CreatePropertyEditorToolkit( EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), ObjectsForPropertiesMenu );
	}
}

void FAssetContextMenu::ExecuteSaveAsset()
{
	TArray<UPackage*> PackagesToSave;
	GetSelectedPackages(PackagesToSave);

	TArray< UPackage* > PackagesWithExternalRefs;
	FString PackageNames;
	if( PackageTools::CheckForReferencesToExternalPackages( &PackagesToSave, &PackagesWithExternalRefs ) )
	{
		for(int32 PkgIdx = 0; PkgIdx < PackagesWithExternalRefs.Num(); ++PkgIdx)
		{
			PackageNames += FString::Printf(TEXT("%s\n"), *PackagesWithExternalRefs[ PkgIdx ]->GetName());
		}
		bool bProceed = EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, FText::Format( NSLOCTEXT("UnrealEd", "Warning_ExternalPackageRef", "The following assets have references to external assets: \n{0}\nExternal assets won't be found when in a game and all references will be broken.  Proceed?"), FText::FromString(PackageNames) ) );
		if(!bProceed)
		{
			return;
		}
	}

	const bool bCheckDirty = false;
	const bool bPromptToSave = false;
	const FEditorFileUtils::EPromptReturnCode Return = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);

	//return Return == FEditorFileUtils::EPromptReturnCode::PR_Success;
}

void FAssetContextMenu::ExecuteDiffSelected() const
{
	if (SelectedAssets.Num() >= 2)
	{
		UObject* FirstObjectSelected = SelectedAssets[0].GetAsset();
		UObject* SecondObjectSelected = SelectedAssets[1].GetAsset();

		if ((FirstObjectSelected != NULL) && (SecondObjectSelected != NULL))
		{
			// Load the asset registry module
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

			FRevisionInfo CurrentRevision; 
			CurrentRevision.Revision = -1;

			AssetToolsModule.Get().DiffAssets(FirstObjectSelected, SecondObjectSelected, CurrentRevision, CurrentRevision);
		}
	}
}

void FAssetContextMenu::ExecuteDuplicate() 
{
	TArray<UObject*> ObjectsToDuplicate;
	const bool SkipRedirectors = true;
	GetSelectedAssets(ObjectsToDuplicate, SkipRedirectors);

	if ( ObjectsToDuplicate.Num() == 1 )
	{
		OnDuplicateRequested.ExecuteIfBound(ObjectsToDuplicate[0]);
	}
	else if ( ObjectsToDuplicate.Num() > 1 )
	{
		TArray<UObject*> NewObjects;
		ObjectTools::DuplicateObjects(ObjectsToDuplicate, TEXT(""), TEXT(""), /*bOpenDialog=*/false, &NewObjects);

		TArray<FAssetData> AssetsToSync;
		for ( auto ObjIt = NewObjects.CreateConstIterator(); ObjIt; ++ObjIt )
		{
			new(AssetsToSync) FAssetData(*ObjIt);
		}

		// Sync to asset tree
		if ( NewObjects.Num() > 0 )
		{
			OnFindInAssetTreeRequested.ExecuteIfBound(AssetsToSync);
		}
	}
}

void FAssetContextMenu::ExecuteRename() 
{
	TArray< FAssetData > AssetViewSelectedAssets = AssetView.Pin()->GetSelectedAssets();
	TArray< FString > SelectedFolders = AssetView.Pin()->GetSelectedFolders();

	if ( AssetViewSelectedAssets.Num() == 1 && SelectedFolders.Num() == 0 )
	{
		// Don't operate on Redirectors
		if ( AssetViewSelectedAssets[0].AssetClass != UObjectRedirector::StaticClass()->GetFName() )
		{
			OnRenameRequested.ExecuteIfBound(AssetViewSelectedAssets[0]);
		}
	}

	if ( AssetViewSelectedAssets.Num() == 0 && SelectedFolders.Num() == 1 )
	{
		OnRenameFolderRequested.ExecuteIfBound(SelectedFolders[0]);
	}
}

void FAssetContextMenu::ExecuteDelete() 
{
	TArray< FAssetData > AssetViewSelectedAssets = AssetView.Pin()->GetSelectedAssets();
	if(AssetViewSelectedAssets.Num() > 0)
	{
		TArray<UObject*> ObjectsToDelete;

		for( auto AssetIt = AssetViewSelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			const FAssetData& AssetData = *AssetIt;

			if( !AssetData.IsAssetLoaded() && FEditorFileUtils::IsMapPackageAsset( AssetData.ObjectPath.ToString() ) )
			{
				// Don't load assets in map packages
				continue;
			}

			if( AssetData.AssetClass == UObjectRedirector::StaticClass()->GetFName() )
			{
				// Don't operate on Redirectors
				continue;
			}

			UObject* Object = AssetData.GetAsset();
			if( Object )
			{
				ObjectsToDelete.Add( Object );
			}
		}

		if ( ObjectsToDelete.Num() > 0 )
		{
			ObjectTools::DeleteObjects(ObjectsToDelete);
		}
	}

	TArray< FString > SelectedFolders = AssetView.Pin()->GetSelectedFolders();
	if(SelectedFolders.Num() > 0)
	{
		FText Prompt;
		if ( SelectedFolders.Num() == 1 )
		{
			Prompt = FText::Format(LOCTEXT("FolderDeleteConfirm_Single", "Delete folder '{0}'?"), FText::FromString(SelectedFolders[0]));
		}
		else
		{
			Prompt = FText::Format(LOCTEXT("FolderDeleteConfirm_Multiple", "Delete {0} folders?"), FText::AsNumber(SelectedFolders.Num()));
		}

		// Spawn a confirmation dialog since this is potentially a highly destructive operation
 		ContentBrowserUtils::DisplayConfirmationPopup(
			Prompt,
			LOCTEXT("FolderDeleteConfirm_Yes", "Delete"),
			LOCTEXT("FolderDeleteConfirm_No", "Cancel"),
			AssetView.Pin().ToSharedRef(),
			FOnClicked::CreateSP( this, &FAssetContextMenu::ExecuteDeleteFolderConfirmed ));
	}
}

FReply FAssetContextMenu::ExecuteDeleteFolderConfirmed()
{
	TArray< FString > SelectedFolders = AssetView.Pin()->GetSelectedFolders();
	if(SelectedFolders.Num() > 0)
	{
		ContentBrowserUtils::DeleteFolders(SelectedFolders);
	}

	return FReply::Handled();
}

void FAssetContextMenu::ExecuteConsolidate()
{
	TArray<UObject*> ObjectsToConsolidate;
	const bool SkipRedirectors = true;
	GetSelectedAssets(ObjectsToConsolidate, SkipRedirectors);

	if ( ObjectsToConsolidate.Num() >  0 )
	{
		FConsolidateToolWindow::AddConsolidationObjects( ObjectsToConsolidate );
	}
}

void FAssetContextMenu::ExecuteCaptureThumbnail()
{
	FViewport* Viewport = GEditor->GetActiveViewport();

	if ( ensure(GCurrentLevelEditingViewportClient) && ensure(Viewport) )
	{
		//have to re-render the requested viewport
		FLevelEditorViewportClient* OldViewportClient = GCurrentLevelEditingViewportClient;
		//remove selection box around client during render
		GCurrentLevelEditingViewportClient = NULL;
		Viewport->Draw();

		ContentBrowserUtils::CaptureThumbnailFromViewport(Viewport, SelectedAssets);

		//redraw viewport to have the yellow highlight again
		GCurrentLevelEditingViewportClient = OldViewportClient;
		Viewport->Draw();
	}
}

void FAssetContextMenu::ExecuteClearThumbnail()
{
	ContentBrowserUtils::ClearCustomThumbnails(SelectedAssets);
}

void FAssetContextMenu::ExecuteMigrateAsset()
{
	// Get a list of package names for input into MigratePackages
	TArray<FName> PackageNames;
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		PackageNames.Add(SelectedAssets[AssetIdx].PackageName);
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().MigratePackages( PackageNames );
}

void FAssetContextMenu::ExecuteShowReferenceViewer()
{
	TArray<FName> PackageNames;
	for ( auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		PackageNames.Add(AssetIt->PackageName);
	}

	if ( PackageNames.Num() > 0 )
	{
		IReferenceViewerModule::Get().InvokeReferenceViewerTab(PackageNames);
	}
}

void FAssetContextMenu::ExecuteCopyReference()
{
	ContentBrowserUtils::CopyAssetReferencesToClipboard(SelectedAssets);
}

void FAssetContextMenu::ExecuteExport()
{
	TArray<UObject*> ObjectsToExport;
	const bool SkipRedirectors = false;
	GetSelectedAssets(ObjectsToExport, SkipRedirectors);

	if ( ObjectsToExport.Num() > 0 )
	{
		ObjectTools::ExportObjects(ObjectsToExport, /*bPromptForEachFileName=*/true);
	}
}

void FAssetContextMenu::ExecuteBulkExport()
{
	TArray<UObject*> ObjectsToExport;
	const bool SkipRedirectors = false;
	GetSelectedAssets(ObjectsToExport, SkipRedirectors);

	if ( ObjectsToExport.Num() > 0 )
	{
		ObjectTools::ExportObjects(ObjectsToExport, /*bPromptForEachFileName=*/false);
	}
}

void FAssetContextMenu::ExecuteRemoveFromCollection()
{
	if ( ensure(SourcesData.Collections.Num() == 1) )
	{
		TArray<FName> AssetsToRemove;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			AssetsToRemove.Add((*AssetIt).ObjectPath);
		}

		if ( AssetsToRemove.Num() > 0 )
		{
			FCollectionManagerModule& CollectionManagerModule = FModuleManager::LoadModuleChecked<FCollectionManagerModule>("CollectionManager");

			FName CollectionName = SourcesData.Collections[0].Name;
			ECollectionShareType::Type CollectionType = SourcesData.Collections[0].Type;
			CollectionManagerModule.Get().RemoveFromCollection(CollectionName, CollectionType, AssetsToRemove);
			OnAssetViewRefreshRequested.ExecuteIfBound();
		}
	}
}

void FAssetContextMenu::ExecuteSCCRefresh()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);

	ISourceControlModule::Get().GetProvider().Execute(ISourceControlOperation::Create<FUpdateStatus>(), SourceControlHelpers::PackageFilenames(PackageNames), EConcurrency::Asynchronous);
}

void FAssetContextMenu::ExecuteSCCCheckOut()
{
	TArray<UPackage*> PackagesToCheckOut;
	GetSelectedPackages(PackagesToCheckOut);

	if ( PackagesToCheckOut.Num() > 0 )
	{
		// Update the source control status of all potentially relevant packages
		ISourceControlModule::Get().GetProvider().Execute(ISourceControlOperation::Create<FUpdateStatus>(), PackagesToCheckOut);

		// Now check them out
		FEditorFileUtils::CheckoutPackages(PackagesToCheckOut);
	}
}

void FAssetContextMenu::ExecuteSCCOpenForAdd()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

	TArray<FString> PackagesToAdd;
	TArray<UPackage*> PackagesToSave;
	for ( auto PackageIt = PackageNames.CreateConstIterator(); PackageIt; ++PackageIt )
	{
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(*PackageIt), EStateCacheUsage::Use);
		if ( SourceControlState.IsValid() && !SourceControlState->IsSourceControlled() )
		{
			PackagesToAdd.Add(*PackageIt);

			// Make sure the file actually exists on disk before adding it
			FString Filename;
			if ( !FPackageName::DoesPackageExist(*PackageIt, NULL, &Filename) )
			{
				UPackage* Package = FindPackage(NULL, **PackageIt);
				if ( Package )
				{
					PackagesToSave.Add(Package);
				}
			}
		}
	}

	if ( PackagesToAdd.Num() > 0 )
	{
		// If any of the packages are new, save them now
		if ( PackagesToSave.Num() > 0 )
		{
			const bool bCheckDirty = false;
			const bool bPromptToSave = false;
			TArray<UPackage*> FailedPackages;
			const FEditorFileUtils::EPromptReturnCode Return = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave, &FailedPackages);
			if(FailedPackages.Num() > 0)
			{
				// don't try and add files that failed to save - remove them from the list
				for(auto FailedPackageIt = FailedPackages.CreateConstIterator(); FailedPackageIt; FailedPackageIt++)
				{
					PackagesToAdd.Remove((*FailedPackageIt)->GetName());
				}
			}
		}

		SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), SourceControlHelpers::PackageFilenames(PackagesToAdd));
	}
}

void FAssetContextMenu::ExecuteSCCCheckIn()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);

	TArray<UPackage*> Packages;
	GetSelectedPackages(Packages);

	// Prompt the user to ask if they would like to first save any dirty packages they are trying to check-in
	const FEditorFileUtils::EPromptReturnCode UserResponse = FEditorFileUtils::PromptForCheckoutAndSave( Packages, true, true );

	// If the user elected to save dirty packages, but one or more of the packages failed to save properly OR if the user
	// canceled out of the prompt, don't follow through on the check-in process
	const bool bShouldProceed = ( UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Success || UserResponse == FEditorFileUtils::EPromptReturnCode::PR_Declined );
	if ( bShouldProceed )
	{
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

void FAssetContextMenu::ExecuteSCCHistory()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);
	FSourceControlWindows::DisplayRevisionHistory(SourceControlHelpers::PackageFilenames(PackageNames));
}

void FAssetContextMenu::ExecuteSCCDiffAgainstDepot() const
{
	// Load the asset registry module
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	// Iterate over each selected asset
	for(int32 AssetIdx=0; AssetIdx<SelectedAssets.Num(); AssetIdx++)
	{
		// Get the actual asset (will load it)
		const FAssetData& AssetData = SelectedAssets[AssetIdx];

		UObject* CurrentObject = AssetData.GetAsset();
		if( CurrentObject )
		{
			const FString PackagePath = AssetData.PackageName.ToString();
			const FString PackageName = AssetData.AssetName.ToString();
			AssetToolsModule.Get().DiffAgainstDepot( CurrentObject, PackagePath, PackageName );
		}
	}
}

void FAssetContextMenu::ExecuteSCCRevert()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);
	FSourceControlWindows::PromptForRevert(PackageNames);
}

void FAssetContextMenu::ExecuteSCCSync()
{
	TArray<FString> PackageNames;
	GetSelectedPackageNames(PackageNames);
	TArray<FString> PackageFileNames = SourceControlHelpers::PackageFilenames(PackageNames);

	TArray<UPackage*> Packages;
	GetSelectedPackages(Packages);
	if(PackageTools::UnloadPackages(Packages))
	{
		ISourceControlModule::Get().GetProvider().Execute(ISourceControlOperation::Create<FSync>(), PackageFileNames);
		for( TArray<FString>::TConstIterator PackageIter( PackageNames ); PackageIter; ++PackageIter )
		{
			PackageTools::LoadPackage(*PackageIter);
		}
	}
	else
	{
		FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("SCC_Sync_PackageUnloadFailed", "Could not unload assets when syncing from source control") );
	}
}

void FAssetContextMenu::ExecuteEnableSourceControl()
{
	ISourceControlModule::Get().ShowLoginDialog(FSourceControlLoginClosed(), ELoginWindowMode::Modeless);
}

bool FAssetContextMenu::CanExecuteSyncToAssetTree() const
{
	return SelectedAssets.Num() > 0;
}

bool FAssetContextMenu::CanExecuteFindInExplorer() const
{
	return SelectedAssets.Num() > 0;
}

bool FAssetContextMenu::CanExecuteCreateBlueprintUsing() const
{
	// Only work if you have a single asset selected
	if(SelectedAssets.Num() == 1)
	{
		UObject* Asset = SelectedAssets[0].GetAsset();
		// See if we know how to make a component from this asset
		TArray< TSubclassOf<UActorComponent> > ComponentClassList = FComponentAssetBrokerage::GetComponentsForAsset(Asset);
		return (ComponentClassList.Num() > 0);
	}

	return false;
}

bool FAssetContextMenu::CanExecuteFindAssetInWorld() const
{
	return bAtLeastOneNonRedirectorSelected;
}

bool FAssetContextMenu::CanExecuteProperties() const
{
	return bAtLeastOneNonRedirectorSelected;
}

bool FAssetContextMenu::CanExecutePropertyMatrix() const
{
	return bAtLeastOneNonRedirectorSelected;
}

bool FAssetContextMenu::CanExecuteDuplicate() const
{
	return bAtLeastOneNonRedirectorSelected;
}

bool FAssetContextMenu::CanExecuteRename() const
{
	TArray< FAssetData > AssetViewSelectedAssets = AssetView.Pin()->GetSelectedAssets();
	TArray< FString > SelectedFolders = AssetView.Pin()->GetSelectedFolders();
	const bool bOneAssetSelected = AssetViewSelectedAssets.Num() == 1 && SelectedFolders.Num() == 0 && AssetViewSelectedAssets[0].AssetClass != UObjectRedirector::StaticClass()->GetFName();
	const bool bOneFolderSelected = AssetViewSelectedAssets.Num() == 0 && SelectedFolders.Num() == 1;
	return (bOneAssetSelected || bOneFolderSelected) && !AssetView.Pin()->IsThumbnailEditMode();
}

bool FAssetContextMenu::CanExecuteDelete() const
{
	TArray< FAssetData > AssetViewSelectedAssets = AssetView.Pin()->GetSelectedAssets();
	TArray< FString > SelectedFolders = AssetView.Pin()->GetSelectedFolders();
	return AssetViewSelectedAssets.Num() > 0 || SelectedFolders.Num() > 0;
}

bool FAssetContextMenu::CanExecuteRemoveFromCollection() const 
{
	return SourcesData.Collections.Num() == 1;
}

bool FAssetContextMenu::CanExecuteSCCRefresh() const
{
	return ISourceControlModule::Get().IsEnabled();
}

bool FAssetContextMenu::CanExecuteSCCCheckOut() const
{
	return bCanExecuteSCCCheckOut;
}

bool FAssetContextMenu::CanExecuteSCCOpenForAdd() const
{
	return bCanExecuteSCCOpenForAdd;
}

bool FAssetContextMenu::CanExecuteSCCCheckIn() const
{
	return bCanExecuteSCCCheckIn;
}

bool FAssetContextMenu::CanExecuteSCCHistory() const
{
	return bCanExecuteSCCHistory;
}

bool FAssetContextMenu::CanExecuteSCCDiffAgainstDepot() const
{
	return bCanExecuteSCCHistory;
}

bool FAssetContextMenu::CanExecuteSCCRevert() const
{
	return bCanExecuteSCCRevert;
}

bool FAssetContextMenu::CanExecuteSCCSync() const
{
	return bCanExecuteSCCSync;
}

bool FAssetContextMenu::CanExecuteConsolidate() const
{
	TArray<UObject*> ProposedObjects;
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		// Don't load assets here. Only operate on already loaded assets.
		if ( SelectedAssets[AssetIdx].IsAssetLoaded() )
		{
			UObject* Object = SelectedAssets[AssetIdx].GetAsset();

			if ( Object )
			{
				ProposedObjects.Add(Object);
			}
		}
	}
	
	if ( ProposedObjects.Num() > 0 )
	{
		TArray<UObject*> CompatibleObjects;
		return FConsolidateToolWindow::DetermineAssetCompatibility(ProposedObjects, CompatibleObjects);
	}

	return false;
}

bool FAssetContextMenu::CanExecuteSaveAsset() const
{
	TArray<UPackage*> Packages;
	GetSelectedPackages(Packages);

	// only enabled if at least one selected package is loaded at all
	for (int32 PackageIdx = 0; PackageIdx < Packages.Num(); ++PackageIdx)
	{
		if ( Packages[PackageIdx] != NULL )
		{
			return true;
		}
	}

	return false;
}

bool FAssetContextMenu::CanExecuteDiffSelected() const
{
	bool bCanDiffSelected = false;
	if (SelectedAssets.Num() == 2)
	{
		FAssetData const& FirstSelection = SelectedAssets[0];
		FAssetData const& SecondSelection = SelectedAssets[1];

		if (FirstSelection.AssetClass != SecondSelection.AssetClass)
		{
			bCanDiffSelected = false;
		}
		else 
		{
			bCanDiffSelected = (FirstSelection.AssetClass == UBlueprint::StaticClass()->GetFName());
		}
	}

	return bCanDiffSelected;
}

bool FAssetContextMenu::CanExecuteCaptureThumbnail() const
{
	return GCurrentLevelEditingViewportClient != NULL;
}

bool FAssetContextMenu::CanClearCustomThumbnails() const
{
	for ( auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		if ( ContentBrowserUtils::AssetHasCustomThumbnail(*AssetIt) )
		{
			return true;
		}
	}

	return false;
}

void FAssetContextMenu::CacheCanExecuteVars()
{
	bAtLeastOneNonRedirectorSelected = false;
	bCanExecuteSCCCheckOut = false;
	bCanExecuteSCCOpenForAdd = false;
	bCanExecuteSCCCheckIn = false;
	bCanExecuteSCCHistory = false;
	bCanExecuteSCCRevert = false;
	bCanExecuteSCCSync = false;

	for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& AssetData = *AssetIt;
		if ( !AssetData.IsValid() )
		{
			continue;
		}

		if ( !bAtLeastOneNonRedirectorSelected && AssetData.AssetClass != UObjectRedirector::StaticClass()->GetFName() )
		{
			bAtLeastOneNonRedirectorSelected = true;
		}

		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		if ( ISourceControlModule::Get().IsEnabled() )
		{
			// Check the SCC state for each package in the selected paths
			FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(AssetData.PackageName.ToString()), EStateCacheUsage::Use);
			if(SourceControlState.IsValid())
			{
				if ( SourceControlState->CanCheckout() )
				{
					bCanExecuteSCCCheckOut = true;
				}

				if ( !SourceControlState->IsSourceControlled() )
				{
					bCanExecuteSCCOpenForAdd = true;
				}
				else if( !SourceControlState->IsAdded() )
				{
					bCanExecuteSCCHistory = true;
				}

				if(!SourceControlState->IsCurrent())
				{
					bCanExecuteSCCSync = true;
				}

				if ( SourceControlState->IsCheckedOut() || SourceControlState->IsAdded() )
				{
					bCanExecuteSCCCheckIn = true;
					bCanExecuteSCCRevert = true;
				}
			}
		}

		if ( bAtLeastOneNonRedirectorSelected
			&& bCanExecuteSCCCheckOut
			&& bCanExecuteSCCOpenForAdd
			&& bCanExecuteSCCCheckIn
			&& bCanExecuteSCCHistory
			&& bCanExecuteSCCRevert
			&& bCanExecuteSCCSync
			)
		{
			// All options are available, no need to keep iterating
			break;
		}
	}
}

void FAssetContextMenu::GetSelectedPackageNames(TArray<FString>& OutPackageNames) const
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		OutPackageNames.Add(SelectedAssets[AssetIdx].PackageName.ToString());
	}
}

void FAssetContextMenu::GetSelectedPackages(TArray<UPackage*>& OutPackages) const
{
	for (int32 AssetIdx = 0; AssetIdx < SelectedAssets.Num(); ++AssetIdx)
	{
		UPackage* Package = FindPackage(NULL, *SelectedAssets[AssetIdx].PackageName.ToString());

		if ( Package )
		{
			OutPackages.Add(Package);
		}
	}
}

#undef LOCTEXT_NAMESPACE
