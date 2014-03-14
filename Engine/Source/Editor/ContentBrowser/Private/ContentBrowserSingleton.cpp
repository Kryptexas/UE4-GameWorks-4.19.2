// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "ContentBrowserPCH.h"
#include "ContentBrowserModule.h"
#include "ObjectTools.h"
#include "WorkspaceMenuStructureModule.h"
#include "STutorialWrapper.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

FContentBrowserSingleton::FContentBrowserSingleton()
	: SettingsStringID(0)
{
	// Register the tab spawners for all content browsers
	const FSlateIcon ContentBrowserIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.TabIcon");
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
	TSharedRef<FWorkspaceItem> ContentBrowserGroup = MenuStructure.GetToolsCategory()->AddGroup(LOCTEXT( "WorkspaceMenu_ContentBrowserCategory", "Content Browser" ), ContentBrowserIcon, true);

	for ( int32 BrowserIdx = 0; BrowserIdx < ARRAY_COUNT(ContentBrowserTabIDs); BrowserIdx++ )
	{
		const FName TabID = FName(*FString::Printf(TEXT("ContentBrowserTab%d"), BrowserIdx + 1));
		ContentBrowserTabIDs[BrowserIdx] = TabID;

		const FText DefaultDisplayName = GetContentBrowserLabelWithIndex( BrowserIdx );

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner( TabID, FOnSpawnTab::CreateRaw(this, &FContentBrowserSingleton::SpawnContentBrowserTab, BrowserIdx) )
			.SetDisplayName(DefaultDisplayName)
			.SetGroup( ContentBrowserGroup )
			.SetIcon(ContentBrowserIcon);
	}

	// Register a couple legacy tab ids
	FGlobalTabmanager::Get()->AddLegacyTabType( "LevelEditorContentBrowser", "ContentBrowserTab1" );
	FGlobalTabmanager::Get()->AddLegacyTabType( "MajorContentBrowserTab", "ContentBrowserTab2" );

	// Register to be notified when properties are edited
	FCoreDelegates::OnObjectPropertyChanged.Add( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &FContentBrowserSingleton::OnObjectPropertyChanged) );
	FEditorDelegates::LoadSelectedAssetsIfNeeded.AddRaw(this, &FContentBrowserSingleton::OnEditorLoadSelectedAssetsIfNeeded);
}

FContentBrowserSingleton::~FContentBrowserSingleton()
{
	FCoreDelegates::OnObjectPropertyChanged.Remove( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &FContentBrowserSingleton::OnObjectPropertyChanged) );
	FEditorDelegates::LoadSelectedAssetsIfNeeded.RemoveAll(this);

	if ( FSlateApplication::IsInitialized() )
	{
		for ( int32 BrowserIdx = 0; BrowserIdx < ARRAY_COUNT(ContentBrowserTabIDs); BrowserIdx++ )
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( ContentBrowserTabIDs[BrowserIdx] );
		}
	}
}

TSharedRef<SWidget> FContentBrowserSingleton::CreateAssetPicker(const FAssetPickerConfig& AssetPickerConfig)
{
	return SNew( SAssetPicker )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.AssetPickerConfig(AssetPickerConfig);
}

TSharedRef<SWidget> FContentBrowserSingleton::CreatePathPicker(const FPathPickerConfig& PathPickerConfig)
{
	return SNew( SPathPicker )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.PathPickerConfig(PathPickerConfig);
}

TSharedRef<class SWidget> FContentBrowserSingleton::CreateCollectionPicker(const FCollectionPickerConfig& CollectionPickerConfig)
{
	return SNew( SCollectionPicker )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.CollectionPickerConfig(CollectionPickerConfig);
}

bool FContentBrowserSingleton::HasPrimaryContentBrowser() const
{
	if ( PrimaryContentBrowser.IsValid() )
	{
		// There is a primary content browser
		return true;
	}
	else
	{
		for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
		{
			if ( AllContentBrowsers[BrowserIdx].IsValid() )
			{
				// There is at least one valid content browser
				return true;
			}
		}

		// There were no valid content browsers
		return false;
	}
}

void FContentBrowserSingleton::FocusPrimaryContentBrowser()
{
	// See if the primary content browser is still valid
	if ( !PrimaryContentBrowser.IsValid() )
	{
		ChooseNewPrimaryBrowser();
	}

	if ( PrimaryContentBrowser.IsValid() )
	{
		FocusContentBrowser( PrimaryContentBrowser.Pin() );
	}
	else
	{
		// If we couldn't find a primary content browser, open one
		SummonNewBrowser();
	}
}

void FContentBrowserSingleton::CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory)
{
	FocusPrimaryContentBrowser();

	if ( PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->CreateNewAsset(DefaultAssetName, PackagePath, AssetClass, Factory);
	}
}

void FContentBrowserSingleton::SyncBrowserToAssets(const TArray<FAssetData>& AssetDataList, bool bAllowLockedBrowsers)
{
	TSharedPtr<SContentBrowser> ContentBrowserToSync;

	if ( !PrimaryContentBrowser.IsValid() )
	{
		ChooseNewPrimaryBrowser();
	}

	if ( PrimaryContentBrowser.IsValid() && (bAllowLockedBrowsers || !PrimaryContentBrowser.Pin()->IsLocked()) )
	{
		// If the primary content browser is not locked, sync it
		ContentBrowserToSync = PrimaryContentBrowser.Pin();
	}
	else
	{
		// If there is no primary or it is locked, find the first non-locked valid browser
		for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
		{
			if ( AllContentBrowsers[BrowserIdx].IsValid() && (bAllowLockedBrowsers || !AllContentBrowsers[BrowserIdx].Pin()->IsLocked()) )
			{
				ContentBrowserToSync = AllContentBrowsers[BrowserIdx].Pin();
				break;
			}
		}
	}

	if ( !ContentBrowserToSync.IsValid() )
	{
		// There are no valid, unlocked browsers, attempt to summon a new one.
		SummonNewBrowser(bAllowLockedBrowsers);

		// Now try to find a non-locked valid browser again, now that a new one may exist
		for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
		{
			if ( AllContentBrowsers[BrowserIdx].IsValid() && (bAllowLockedBrowsers || !AllContentBrowsers[BrowserIdx].Pin()->IsLocked()) )
			{
				ContentBrowserToSync = AllContentBrowsers[BrowserIdx].Pin();
				break;
			}
		}
	}

	if ( ContentBrowserToSync.IsValid() )
	{
		// Finally, focus and sync the browser that was found
		FocusContentBrowser(ContentBrowserToSync);
		ContentBrowserToSync->SyncToAssets(AssetDataList);
	}
	else
	{
		UE_LOG( LogContentBrowser, Log, TEXT( "Unable to sync content browser, all browsers appear to be locked" ) );
	}
}

void FContentBrowserSingleton::SyncBrowserToAssets(const TArray<UObject*>& AssetList, bool bAllowLockedBrowsers)
{
	// Convert UObject* array to FAssetData array
	TArray<FAssetData> AssetDataList;
	for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); ++AssetIdx)
	{
		if ( AssetList[AssetIdx] )
		{
			AssetDataList.Add( FAssetData(AssetList[AssetIdx]) );
		}
	}

	SyncBrowserToAssets(AssetDataList, bAllowLockedBrowsers);
}

void FContentBrowserSingleton::GetSelectedAssets(TArray<FAssetData>& SelectedAssets)
{
	if ( PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->GetSelectedAssets(SelectedAssets);
	}
}

void FContentBrowserSingleton::OnEditorLoadSelectedAssetsIfNeeded()
{
	if ( PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->LoadSelectedObjectsIfNeeded();
	}
}

FContentBrowserSingleton& FContentBrowserSingleton::Get()
{
	FContentBrowserModule& Module = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
	return static_cast<FContentBrowserSingleton&>(Module.Get());
}

void FContentBrowserSingleton::SetPrimaryContentBrowser(const TSharedRef<SContentBrowser>& NewPrimaryBrowser)
{
	if ( PrimaryContentBrowser.IsValid() && PrimaryContentBrowser.Pin().ToSharedRef() == NewPrimaryBrowser )
	{
		// This is already the primary content browser
		return;
	}

	if ( PrimaryContentBrowser.IsValid() )
	{
		PrimaryContentBrowser.Pin()->SetIsPrimaryContentBrowser(false);
	}

	PrimaryContentBrowser = NewPrimaryBrowser;
	NewPrimaryBrowser->SetIsPrimaryContentBrowser(true);
}

void FContentBrowserSingleton::ContentBrowserClosed(const TSharedRef<SContentBrowser>& ClosedBrowser)
{
	// Find the browser in the all browsers list
	for (int32 BrowserIdx = AllContentBrowsers.Num() - 1; BrowserIdx >= 0; --BrowserIdx)
	{
		if ( !AllContentBrowsers[BrowserIdx].IsValid() || AllContentBrowsers[BrowserIdx].Pin() == ClosedBrowser )
		{
			AllContentBrowsers.RemoveAt(BrowserIdx);
		}
	}

	if ( !PrimaryContentBrowser.IsValid() || ClosedBrowser == PrimaryContentBrowser.Pin() )
	{
		ChooseNewPrimaryBrowser();
	}

	BrowserToLastKnownTabManagerMap.Add(ClosedBrowser->GetInstanceName(), ClosedBrowser->GetTabManager());
}

void FContentBrowserSingleton::ChooseNewPrimaryBrowser()
{
	// Find the first valid browser and trim any invalid browsers along the way
	for (int32 BrowserIdx = 0; BrowserIdx < AllContentBrowsers.Num(); ++BrowserIdx)
	{
		if ( AllContentBrowsers[BrowserIdx].IsValid() )
		{
			SetPrimaryContentBrowser(AllContentBrowsers[BrowserIdx].Pin().ToSharedRef());
			break;
		}
		else
		{
			// Trim any invalid content browsers
			AllContentBrowsers.RemoveAt(BrowserIdx);
			BrowserIdx--;
		}
	}
}

void FContentBrowserSingleton::FocusContentBrowser(const TSharedPtr<SContentBrowser>& BrowserToFocus)
{
	if ( BrowserToFocus.IsValid() )
	{
		TSharedRef<SContentBrowser> Browser = BrowserToFocus.ToSharedRef();
		TSharedPtr<FTabManager> TabManager = Browser->GetTabManager();
		if ( TabManager.IsValid() )
		{
			TabManager->InvokeTab(Browser->GetInstanceName());
		}
	}
}

void FContentBrowserSingleton::SummonNewBrowser(bool bAllowLockedBrowsers)
{
	TSet<FName> OpenBrowserIDs;

	// Find all currently open browsers to help find the first open slot
	for (int32 BrowserIdx = AllContentBrowsers.Num() - 1; BrowserIdx >= 0; --BrowserIdx)
	{
		const TWeakPtr<SContentBrowser>& Browser = AllContentBrowsers[BrowserIdx];
		if ( Browser.IsValid() )
		{
			OpenBrowserIDs.Add(Browser.Pin()->GetInstanceName());
		}
	}
	
	FName NewTabName;
	for ( int32 BrowserIdx = 0; BrowserIdx < ARRAY_COUNT(ContentBrowserTabIDs); BrowserIdx++ )
	{
		FName TestTabID = ContentBrowserTabIDs[BrowserIdx];
		if ( !OpenBrowserIDs.Contains(TestTabID) && (bAllowLockedBrowsers || !IsLocked(TestTabID)) )
		{
			// Found the first index that is not currently open
			NewTabName = TestTabID;
			break;
		}
	}

	if ( NewTabName != NAME_None )
	{
		const TWeakPtr<FTabManager>& TabManagerToInvoke = BrowserToLastKnownTabManagerMap.FindRef(NewTabName);
		if ( TabManagerToInvoke.IsValid() )
		{
			TabManagerToInvoke.Pin()->InvokeTab(NewTabName);
		}
		else
		{
			FGlobalTabmanager::Get()->InvokeTab(NewTabName);
		}
	}
	else
	{
		// No available slots... don't summon anything
	}
}

void FContentBrowserSingleton::OnObjectPropertyChanged(UObject* ObjectBeingModified)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
	}

	if ( ObjectBeingModified->HasAnyFlags(RF_ClassDefaultObject) && ObjectBeingModified->GetClass()->ClassGeneratedBy != NULL )
	{
		// This is a CDO for a blueprint. Pretend the blueprint changed instead.
		ObjectBeingModified = ObjectBeingModified->GetClass()->ClassGeneratedBy;
	}

	if ( ObjectBeingModified->IsAsset() )
	{
		// An object in memory was modified.  We'll mark it's thumbnail as dirty so that it'll be
		// regenerated on demand later. (Before being displayed in the browser, or package saves, etc.)
		FObjectThumbnail* Thumbnail = ThumbnailTools::GetThumbnailForObject( ObjectBeingModified );

		if ( Thumbnail == NULL )
		{
			// If we don't yet have a thumbnail map, load one from disk if possible
			FName ObjectFullName = FName(*ObjectBeingModified->GetFullName());
			TArray<FName> ObjectFullNames;
			FThumbnailMap LoadedThumbnails;
			ObjectFullNames.Add( ObjectFullName );
			if ( ThumbnailTools::ConditionallyLoadThumbnailsForObjects( ObjectFullNames, LoadedThumbnails ) )
			{
				Thumbnail = LoadedThumbnails.Find(ObjectFullName);
				
				if ( Thumbnail != NULL )
				{
					Thumbnail = ThumbnailTools::CacheThumbnail(ObjectBeingModified->GetFullName(), Thumbnail, ObjectBeingModified->GetOutermost());
				}
			}
		}

		if( Thumbnail != NULL )
		{
			// Mark the thumbnail as dirty
			Thumbnail->MarkAsDirty();
		}
	}
}

TSharedRef<SDockTab> FContentBrowserSingleton::SpawnContentBrowserTab( const FSpawnTabArgs& SpawnTabArgs, int32 BrowserIdx )
{	
	TAttribute<FText> Label = TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateRaw( this, &FContentBrowserSingleton::GetContentBrowserTabLabel, BrowserIdx ) );

	TSharedRef<SDockTab> NewTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label( Label )
		.ToolTip( IDocumentation::Get()->CreateToolTip( Label, nullptr, "Shared/ContentBrowser", "Tab" ) );

	TSharedRef<SContentBrowser> NewBrowser =
		SNew( SContentBrowser, SpawnTabArgs.GetTabId().TabType )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.ContainingTab( NewTab );

	AllContentBrowsers.Add(NewBrowser);

	if ( !PrimaryContentBrowser.IsValid() )
	{
		ChooseNewPrimaryBrowser();
	}

	// Add wrapper for tutorial highlighting
	TSharedRef<STutorialWrapper> Wrapper = 
		SNew(STutorialWrapper)
		.Name(FName(TEXT("ContentBrowser")))
		.Content()
		[
			NewBrowser
		];

	NewTab->SetContent( Wrapper );

	return NewTab;
}

bool FContentBrowserSingleton::IsLocked(const FName& InstanceName) const
{
	// First try all the open browsers, as their locked state might be newer than the configs
	for (int32 AllBrowsersIdx = AllContentBrowsers.Num() - 1; AllBrowsersIdx >= 0; --AllBrowsersIdx)
	{
		const TWeakPtr<SContentBrowser>& Browser = AllContentBrowsers[AllBrowsersIdx];
		if ( Browser.IsValid() && Browser.Pin()->GetInstanceName() == InstanceName )
		{
			return Browser.Pin()->IsLocked();
		}
	}

	// Fallback to getting the locked state from the config instead
	bool bIsLocked = false;
	GConfig->GetBool(*SContentBrowser::SettingsIniSection, *(InstanceName.ToString() + TEXT(".Locked")), bIsLocked, GEditorUserSettingsIni);
	return bIsLocked;
}

FText FContentBrowserSingleton::GetContentBrowserLabelWithIndex( int32 BrowserIdx )
{
	return FText::Format(LOCTEXT("ContentBrowserTabNameWithIndex", "Content Browser {0}"), FText::AsNumber(BrowserIdx + 1));
}

FText FContentBrowserSingleton::GetContentBrowserTabLabel(int32 BrowserIdx)
{
	int32 NumOpenContentBrowsers = 0;
	for (int32 AllBrowsersIdx = AllContentBrowsers.Num() - 1; AllBrowsersIdx >= 0; --AllBrowsersIdx)
	{
		const TWeakPtr<SContentBrowser>& Browser = AllContentBrowsers[AllBrowsersIdx];
		if ( Browser.IsValid() )
		{
			NumOpenContentBrowsers++;
		}
		else
		{
			AllContentBrowsers.RemoveAt(AllBrowsersIdx);
		}
	}

	if ( NumOpenContentBrowsers > 1 )
	{
		return GetContentBrowserLabelWithIndex( BrowserIdx );
	}
	else
	{
		return LOCTEXT("ContentBrowserTabName", "Content Browser");
	}
}

#undef LOCTEXT_NAMESPACE