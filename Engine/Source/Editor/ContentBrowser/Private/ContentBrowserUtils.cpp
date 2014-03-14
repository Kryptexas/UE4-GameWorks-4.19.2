// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "ContentBrowserPCH.h"

#include "Editor/UnrealEd/Public/Toolkits/AssetEditorManager.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "ImageUtils.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

// Keep a map of all the paths that have custom colors, so updating the color in one location updates them all
static TMap< FString, TSharedPtr< FLinearColor > > PathColors;

class SContentBrowserPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SContentBrowserPopup ){}

		SLATE_ATTRIBUTE( FText, Message )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& InArgs )
	{
		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
			.Padding(10)
			.OnMouseButtonDown(this, &SContentBrowserPopup::OnBorderClicked)
			.BorderBackgroundColor(this, &SContentBrowserPopup::GetBorderBackgroundColor)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(SImage) .Image( FEditorStyle::GetBrush("ContentBrowser.PopupMessageIcon") )
				]

				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(InArgs._Message)
					.WrapTextAt(450)
				]
			]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	static void DisplayMessage( const FText& Message, const FSlateRect& ScreenAnchor, TSharedRef<SWidget> ParentContent )
	{
		TSharedRef<SContentBrowserPopup> PopupContent = SNew(SContentBrowserPopup) .Message(Message);

		const FVector2D ScreenLocation = FVector2D(ScreenAnchor.Left, ScreenAnchor.Top);
		const bool bFocusImmediately = true;
		const bool bShouldAutoSize = true;
		const FVector2D WindowSize = FVector2D::ZeroVector;
		const FVector2D SummonLocationSize = ScreenAnchor.GetSize();

		TSharedRef<SWindow> PopupWindow = FSlateApplication::Get().PushMenu(
			ParentContent,
			PopupContent,
			ScreenLocation,
			FPopupTransitionEffect( FPopupTransitionEffect::TopMenu ),
			bFocusImmediately,
			bShouldAutoSize,
			WindowSize,
			SummonLocationSize
			);

		PopupContent->SetWindow(PopupWindow);
	}

private:
	void SetWindow( const TSharedRef<SWindow>& InWindow )
	{
		Window = InWindow;
	}

	FReply OnBorderClicked(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
	{
		if ( Window.IsValid() )
		{
			Window.Pin()->RequestDestroyWindow();
		}

		return FReply::Handled();
	}

	FSlateColor GetBorderBackgroundColor() const
	{
		return IsHovered() ? FLinearColor(0.5, 0.5, 0.5, 1) : FLinearColor::White;
	}

private:
	TWeakPtr<SWindow> Window;
};

/** A miniture confirmation popup for quick yes/no questions */
class SContentBrowserConfirmPopup :  public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SContentBrowserConfirmPopup ) {}
			
		/** The text to display */
		SLATE_ARGUMENT(FText, Prompt)

		/** The Yes Button to display */
		SLATE_ARGUMENT(FText, YesText)

		/** The No Button to display */
		SLATE_ARGUMENT(FText, NoText)

		/** Invoked when yes is clicked */
		SLATE_EVENT(FOnClicked, OnYesClicked)

		/** Invoked when no is clicked */
		SLATE_EVENT(FOnClicked, OnNoClicked)

	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& InArgs )
	{
		OnYesClicked = InArgs._OnYesClicked;
		OnNoClicked = InArgs._OnNoClicked;

		ChildSlot
		[
			SNew(SBorder)
			. BorderImage(FEditorStyle::GetBrush("Menu.Background"))
			. Padding(10)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 5)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
						.Text(InArgs._Prompt)
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(3)
					+ SUniformGridPanel::Slot(0, 0)
					.HAlign(HAlign_Fill)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(InArgs._YesText)
						.OnClicked( this, &SContentBrowserConfirmPopup::YesClicked )
					]

					+ SUniformGridPanel::Slot(1, 0)
					.HAlign(HAlign_Fill)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(InArgs._NoText)
						.OnClicked( this, &SContentBrowserConfirmPopup::NoClicked )
					]
				]
			]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/** Opens the popup using the specified component as its parent */
	void OpenPopup(const TSharedRef<SWidget>& ParentContent)
	{
		// Show dialog to confirm the delete
		PopupWindow = FSlateApplication::Get().PushMenu(
			ParentContent,
			SharedThis(this),
			FSlateApplication::Get().GetCursorPos(),
			FPopupTransitionEffect( FPopupTransitionEffect::TopMenu )
			);
	}

private:
	/** The yes button was clicked */
	FReply YesClicked()
	{
		if ( OnYesClicked.IsBound() )
		{
			OnYesClicked.Execute();
		}

		PopupWindow.Pin()->RequestDestroyWindow();

		return FReply::Handled();
	}

	/** The no button was clicked */
	FReply NoClicked()
	{
		if ( OnNoClicked.IsBound() )
		{
			OnNoClicked.Execute();
		}

		PopupWindow.Pin()->RequestDestroyWindow();

		return FReply::Handled();
	}

	/** The window containing this popup */
	TWeakPtr<SWindow> PopupWindow;

	/** Delegates for button clicks */
	FOnClicked OnYesClicked;
	FOnClicked OnNoClicked;
};


bool ContentBrowserUtils::OpenEditorForAsset(const FString& ObjectPath)
{
	// Load the asset if unloaded
	TArray<UObject*> LoadedObjects;
	TArray<FString> ObjectPaths;
	ObjectPaths.Add(ObjectPath);
	ContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedObjects);

	// Open the editor for the specified asset
	UObject* FoundObject = FindObject<UObject>(NULL, *ObjectPath);
			
	return OpenEditorForAsset(FoundObject);
}

bool ContentBrowserUtils::OpenEditorForAsset(UObject* Asset)
{
	if( Asset != NULL )
	{
		// @todo toolkit minor: Needs world-centric support?
		return FAssetEditorManager::Get().OpenEditorForAsset(Asset);
	}

	return false;
}

bool ContentBrowserUtils::OpenEditorForAsset(const TArray<UObject*>& Assets)
{
	if ( Assets.Num() == 1 )
	{
		return OpenEditorForAsset(Assets[0]);
	}
	else if ( Assets.Num() > 1 )
	{
		return FAssetEditorManager::Get().OpenEditorForAssets(Assets);
	}
	
	return false;
}

bool ContentBrowserUtils::LoadAssetsIfNeeded(const TArray<FString>& ObjectPaths, TArray<UObject*>& LoadedObjects, bool bAllowedToPromptToLoadAssets)
{
	bool bAnyObjectsWereLoadedOrUpdated = false;

	// Build a list of unloaded assets
	TArray<FString> UnloadedObjectPaths;
	for (int32 PathIdx = 0; PathIdx < ObjectPaths.Num(); ++PathIdx)
	{
		const FString& ObjectPath = ObjectPaths[PathIdx];

		if ( !FEditorFileUtils::IsMapPackageAsset(ObjectPath) )
		{
			UObject* FoundObject = FindObject<UObject>(NULL, *ObjectPath);

			if ( FoundObject )
			{
				LoadedObjects.Add(FoundObject);
			}
			else
			{
				// Unloaded asset, we will load it later
				UnloadedObjectPaths.Add(ObjectPath);
			}
		}
	}

	// if we are allowed to prompt the user to load and we have enough assets that requires prompting then we should 
	// prompt and load assets if the user said it was ok
	bool bShouldLoadAssets = true;
	if( bAllowedToPromptToLoadAssets && ShouldPromptToLoadAssets(ObjectPaths, UnloadedObjectPaths) )
	{
		bShouldLoadAssets = PromptToLoadAssets(ObjectPaths);
	}


	// Ask for confirmation if the user is attempting to load a large number of assets
	if (bShouldLoadAssets == false)
	{
		return false;
	}

	// Make sure all selected objects are loaded, where possible
	if ( UnloadedObjectPaths.Num() > 0 )
	{
		// Get the maximum objects to load before displaying the slow task
		const bool bShowProgressDialog = UnloadedObjectPaths.Num() > GetDefault<UContentBrowserSettings>()->NumObjectsToLoadBeforeWarning;
		GWarn->BeginSlowTask(LOCTEXT("LoadingObjects", "Loading Objects..."), bShowProgressDialog);

		GIsEditorLoadingPackage = true;

		FString FailedObjectPaths;
		for (int32 PathIdx = 0; PathIdx < UnloadedObjectPaths.Num(); ++PathIdx)
		{
			const FString& ObjectPath = UnloadedObjectPaths[PathIdx];

			// We never want to follow redirects when loading objects for the Content Browser.  It would
			// allow a user to interact with a ghost/unverified asset as if it were still alive.
			const ELoadFlags LoadFlags = LOAD_NoRedirects;

			// Load up the object
			UObject* LoadedObject = LoadObject<UObject>(NULL, *ObjectPath, NULL, LoadFlags, NULL);
			if ( LoadedObject )
			{
				LoadedObjects.Add(LoadedObject);
			}
			else
			{
				FailedObjectPaths += ObjectPath + LINE_TERMINATOR;
			}

			if ( bShowProgressDialog )
			{
				GWarn->UpdateProgress(PathIdx, UnloadedObjectPaths.Num());
			}

			if (GWarn->ReceivedUserCancel())
			{
				// If the user has canceled stop loading the remaining objects. We don't add the remaining objects to the failed string,
				// this would only result in launching another dialog when by their actions the user clearly knows not all of the 
				// assets will have been loaded.
				break;
			}
		}
		GIsEditorLoadingPackage = false;
		GWarn->EndSlowTask();

		if ( !FailedObjectPaths.IsEmpty() )
		{
			// Load failed at least one asset for some reason.  Play a warning message.
			const FText Warning = FText::Format( LOCTEXT("LoadObjectFailed", "Failed to load the following assets:\r\n{0}"), FText::FromString( FailedObjectPaths ) );
			FMessageDialog::Open( EAppMsgType::Ok, Warning);
			return false;
		}
	}

	return true;
}

bool ContentBrowserUtils::ShouldPromptToLoadAssets(const TArray<FString>& ObjectPaths, TArray<FString>& OutUnloadedObjects)
{
	OutUnloadedObjects.Empty();

	bool bShouldPrompt = false;
	// Build a list of unloaded assets
	for (int32 PathIdx = 0; PathIdx < ObjectPaths.Num(); ++PathIdx)
	{
		const FString& ObjectPath = ObjectPaths[PathIdx];

		if ( !FEditorFileUtils::IsMapPackageAsset(ObjectPath) )
		{
			UObject* FoundObject = FindObject<UObject>(NULL, *ObjectPath);
			if ( !FoundObject )
			{
				// Unloaded asset, we will load it later
				OutUnloadedObjects.Add(ObjectPath);
			}
		}
	}

	// Get the maximum objects to load before displaying a warning
	// Ask for confirmation if the user is attempting to load a large number of assets
	if (OutUnloadedObjects.Num() > GetDefault<UContentBrowserSettings>()->NumObjectsToLoadBeforeWarning)
	{
		bShouldPrompt = true;
	}

	return bShouldPrompt;
}

bool ContentBrowserUtils::PromptToLoadAssets(const TArray<FString>& UnloadedObjects)
{
	bool bShouldLoadAssets = false;

	// Prompt the user to load assets
	const FText Question = FText::Format( LOCTEXT("ConfirmLoadAssets", "You are about to load {0} assets. Would you like to proceed?"), FText::AsNumber( UnloadedObjects.Num() ) );
	if ( EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, Question) )
	{
		bShouldLoadAssets = true;
	}

	return bShouldLoadAssets;
}

void ContentBrowserUtils::RenameAsset(UObject* Asset, const FString& NewName, FText& ErrorMessage)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FAssetRenameData> AssetsAndNames;
	const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	new(AssetsAndNames) FAssetRenameData(Asset, PackagePath, NewName);
	AssetToolsModule.Get().RenameAssets(AssetsAndNames);
}

void ContentBrowserUtils::MoveAssets(const TArray<UObject*>& Assets, const FString& DestPath, const FString& SourcePath)
{
	check(DestPath.Len() > 0);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FAssetRenameData> AssetsAndNames;
	for ( auto AssetIt = Assets.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		UObject* Asset = *AssetIt;

		if ( !ensure(Asset) )
		{
			continue;
		}

		FString PackagePath;
		FString ObjectName = Asset->GetName();

		if ( SourcePath.Len() )
		{
			const FString CurrentPackageName = Asset->GetOutermost()->GetName();

			// This is a relative operation
			if ( !ensure(CurrentPackageName.StartsWith(SourcePath)) )
			{
				continue;
			}
				
			// Collect the relative path then use it to determine the new location
			// For example, if SourcePath = /Game/MyPath and CurrentPackageName = /Game/MyPath/MySubPath/MyAsset
			//     /Game/MyPath/MySubPath/MyAsset -> /MySubPath

			const int32 ShortPackageNameLen = FPackageName::GetLongPackageAssetName(CurrentPackageName).Len();
			const int32 RelativePathLen = CurrentPackageName.Len() - ShortPackageNameLen - SourcePath.Len() - 1; // -1 to exclude the trailing "/"
			const FString RelativeDestPath = CurrentPackageName.Mid(SourcePath.Len(), RelativePathLen);

			PackagePath = DestPath + RelativeDestPath;
		}
		else
		{
			// Only a DestPath was supplied, use it
			PackagePath = DestPath;
		}

		new(AssetsAndNames) FAssetRenameData(Asset, PackagePath, ObjectName);
	}

	if ( AssetsAndNames.Num() > 0 )
	{
		AssetToolsModule.Get().RenameAssets(AssetsAndNames);
	}
}

int32 ContentBrowserUtils::DeleteAssets(const TArray<UObject*>& AssetsToDelete)
{
	return ObjectTools::DeleteObjects(AssetsToDelete);
}

bool ContentBrowserUtils::DeleteFolders(const TArray<FString>& PathsToDelete)
{
	// Get a list of assets in the paths to delete
	TArray<FAssetData> AssetDataList;
	GetAssetsInPaths(PathsToDelete, AssetDataList);

	const int32 NumAssetsInPaths = AssetDataList.Num();
	bool bAllowFolderDelete = false;
	if ( NumAssetsInPaths == 0 )
	{
		// There were no assets, allow the folder delete.
		bAllowFolderDelete = true;
	}
	else
	{
		// Load all the assets in the folder and attempt to delete them.
		// If it was successful, allow the folder delete.

		// Get a list of object paths for input into LoadAssetsIfNeeded
		TArray<FString> ObjectPaths;
		for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			ObjectPaths.Add((*AssetIt).ObjectPath.ToString());
		}

		// Load all the assets in the selected paths
		TArray<UObject*> LoadedAssets;
		if ( ContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedAssets) )
		{
			// Make sure we loaded all of them
			if ( LoadedAssets.Num() == NumAssetsInPaths )
			{
				const int32 NumAssetsDeleted = ContentBrowserUtils::DeleteAssets(LoadedAssets);
				if ( NumAssetsDeleted == NumAssetsInPaths )
				{
					// Successfully deleted all assets in the specified path. Allow the folder to be removed.
					bAllowFolderDelete = true;
				}
				else
				{
					// Not all the assets in the selected paths were deleted
				}
			}
			else
			{
				// Not all the assets in the selected paths were loaded
			}
		}
		else
		{
			// The user declined to load some assets or some assets failed to load
		}
	}
	
	if ( bAllowFolderDelete )
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		for ( auto PathIt = PathsToDelete.CreateConstIterator(); PathIt; ++PathIt )
		{
			AssetRegistryModule.Get().RemovePath(*PathIt);
		}

		return true;
	}

	return false;
}

void ContentBrowserUtils::GetAssetsInPaths(const TArray<FString>& InPaths, TArray<FAssetData>& OutAssetDataList)
{
	// Load the asset registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (int32 PathIdx = 0; PathIdx < InPaths.Num(); ++PathIdx)
	{
		new (Filter.PackagePaths) FName(*InPaths[PathIdx]);
	}

	// Query for a list of assets in the selected paths
	AssetRegistryModule.Get().GetAssets(Filter, OutAssetDataList);
}

bool ContentBrowserUtils::SavePackages(const TArray<UPackage*>& Packages)
{
	TArray< UPackage* > PackagesWithExternalRefs;
	FString PackageNames;
	if( PackageTools::CheckForReferencesToExternalPackages( &Packages, &PackagesWithExternalRefs ) )
	{
		for(int32 PkgIdx = 0; PkgIdx < PackagesWithExternalRefs.Num(); ++PkgIdx)
		{
			PackageNames += FString::Printf(TEXT("%s\n"), *PackagesWithExternalRefs[ PkgIdx ]->GetName());
		}
		bool bProceed = EAppReturnType::Yes == FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::Format(
				NSLOCTEXT("UnrealEd", "Warning_ExternalPackageRef", "The following assets have references to external assets: \n{0}\nExternal assets won't be found when in a game and all references will be broken.  Proceed?"),
				FText::FromString(PackageNames) ) );
		if(!bProceed)
		{
			return false;
		}
	}

	const bool bCheckDirty = false;
	const bool bPromptToSave = false;
	const FEditorFileUtils::EPromptReturnCode Return = FEditorFileUtils::PromptForCheckoutAndSave(Packages, bCheckDirty, bPromptToSave);

	return Return == FEditorFileUtils::EPromptReturnCode::PR_Success;
}

bool ContentBrowserUtils::SaveDirtyPackages()
{
	const bool bPromptUserToSave = true;
	const bool bSaveMapPackages = false;
	const bool bSaveContentPackages = true;
	return FEditorFileUtils::SaveDirtyPackages( bPromptUserToSave, bSaveMapPackages, bSaveContentPackages );
}

TArray<UPackage*> ContentBrowserUtils::LoadPackages(const TArray<FString>& PackageNames)
{
	TArray<UPackage*> LoadedPackages;

	GWarn->BeginSlowTask( LOCTEXT("LoadingPackages", "Loading Packages..."), true );

	for (int32 PackageIdx = 0; PackageIdx < PackageNames.Num(); ++PackageIdx)
	{
		const FString& PackageName = PackageNames[PackageIdx];

		if ( !ensure(PackageName.Len() > 0) )
		{
			// Empty package name. Skip it.
			continue;
		}

		if ( FEditorFileUtils::IsMapPackageAsset(PackageName) )
		{
			// Map package. Skip it.
			continue;
		}

		UPackage* Package = FindPackage(NULL, *PackageName);

		if ( Package != NULL )
		{
			// The package is at least partially loaded. Fully load it.
			Package->FullyLoad();
		}
		else
		{
			// The package is unloaded. Try to load the package from disk.
			Package = PackageTools::LoadPackage(PackageName);
		}

		// If the package was loaded, add it to the loaded packages list.
		if ( Package != NULL )
		{
			LoadedPackages.Add(Package);
		}
	}

	GWarn->EndSlowTask();

	return LoadedPackages;
}

void ContentBrowserUtils::DisplayMessage(const FText& Message, const FSlateRect& ScreenAnchor, const TSharedRef<SWidget>& ParentContent)
{
	SContentBrowserPopup::DisplayMessage(Message, ScreenAnchor, ParentContent);
}

void ContentBrowserUtils::DisplayConfirmationPopup(const FText& Message, const FText& YesString, const FText& NoString, const TSharedRef<SWidget>& ParentContent, const FOnClicked& OnYesClicked, const FOnClicked& OnNoClicked)
{
	TSharedRef<SContentBrowserConfirmPopup> Popup = 
		SNew(SContentBrowserConfirmPopup)
		.Prompt(Message)
		.YesText(YesString)
		.NoText(NoString)
		.OnYesClicked( OnYesClicked )
		.OnNoClicked( OnNoClicked );

	Popup->OpenPopup(ParentContent);
}

void ContentBrowserUtils::CopyFolders(const TArray<FString>& InSourcePathNames, const FString& DestPath)
{
	TMap<FString, TArray<UObject*> > SourcePathToLoadedAssets;

	// Make sure the destination path is not in the source path list
	TArray<FString> SourcePathNames = InSourcePathNames;
	SourcePathNames.Remove(DestPath);

	// Load all assets in the source paths
	PrepareFoldersForDragDrop(SourcePathNames, SourcePathToLoadedAssets);

	// Load the Asset Registry to update paths during the copy
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	// For every path which contained valid assets...
	for ( auto PathIt = SourcePathToLoadedAssets.CreateConstIterator(); PathIt; ++PathIt )
	{
		// Put dragged folders in a sub-folder under the destination path
		FString SubFolderName = FPackageName::GetLongPackageAssetName(PathIt.Key());
		FString Destination = DestPath + TEXT("/") + SubFolderName;

		// Add the new path to notify sources views
		AssetRegistryModule.Get().AddPath(Destination);

		// If any assets were in this path...
		if ( PathIt.Value().Num() > 0 )
		{
			// Copy assets and supply a source path to indicate it is relative
			ObjectTools::DuplicateObjects( PathIt.Value(), PathIt.Key(), Destination, /*bOpenDialog=*/false );
		}
	}
}

void ContentBrowserUtils::MoveFolders(const TArray<FString>& InSourcePathNames, const FString& DestPath)
{
	TMap<FString, TArray<UObject*> > SourcePathToLoadedAssets;

	// Make sure the destination path is not in the source path list
	TArray<FString> SourcePathNames = InSourcePathNames;
	SourcePathNames.Remove(DestPath);

	// Load all assets in the source paths
	PrepareFoldersForDragDrop(SourcePathNames, SourcePathToLoadedAssets);
	
	// Load the Asset Registry to update paths during the move
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// For every path which contained valid assets...
	for ( auto PathIt = SourcePathToLoadedAssets.CreateConstIterator(); PathIt; ++PathIt )
	{
		// Put dragged folders in a sub-folder under the destination path
		const FString SourcePath = PathIt.Key();
		const FString SubFolderName = FPackageName::GetLongPackageAssetName(SourcePath);
		const FString Destination = DestPath + TEXT("/") + SubFolderName;

		// Add the new path to notify sources views
		AssetRegistryModule.Get().AddPath(Destination);

		// If any assets were in this path...
		if ( PathIt.Value().Num() > 0 )
		{
			// Move assets and supply a source path to indicate it is relative
			ContentBrowserUtils::MoveAssets( PathIt.Value(), Destination, PathIt.Key() );
		}

		// Attempt to remove the old paths. This operation will silently fail if any assets failed to move.
		AssetRegistryModule.Get().RemovePath(SourcePath);
	}
}

void ContentBrowserUtils::PrepareFoldersForDragDrop(const TArray<FString>& SourcePathNames, TMap< FString, TArray<UObject*> >& OutSourcePathToLoadedAssets)
{
	TSet<UObject*> AllFoundObjects;

	GWarn->BeginSlowTask( LOCTEXT("FolderDragDrop_Loading", "Loading folders"), true);

	// Load the Asset Registry to update paths during the move
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// For every source path, load every package in the path (if necessary) and keep track of the assets that were loaded
	for ( auto PathIt = SourcePathNames.CreateConstIterator(); PathIt; ++PathIt )
	{
		// Get all assets in this path
		TArray<FAssetData> AssetDataList;
		AssetRegistryModule.Get().GetAssetsByPath(FName(**PathIt), AssetDataList, true);

		// Form a list of all object paths for these assets
		TArray<FString> ObjectPaths;
		for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			ObjectPaths.Add((*AssetIt).ObjectPath.ToString());
		}

		// Load all assets in this path if needed
		TArray<UObject*> AllLoadedAssets;
		LoadAssetsIfNeeded(ObjectPaths, AllLoadedAssets);

		// Find all files in this path and subpaths
		TArray<FString> Filenames;
		FString RootFolder = FPackageName::LongPackageNameToFilename(*PathIt);
		FPackageName::FindPackagesInDirectory(Filenames, RootFolder);

		// Add a slash to the end of the path so StartsWith doesn't get a false positive on similarly named folders
		const FString SourcePathWithSlash = *PathIt + TEXT("/");

		// Now find all assets in memory that were loaded from this path that are valid for drag-droppping
		TArray<UObject*> ValidLoadedAssets;
		for ( auto AssetIt = AllLoadedAssets.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			UObject* Asset = *AssetIt;
			if ( (Asset->GetClass() != UObjectRedirector::StaticClass() &&				// Skip object redirectors
				 !Asset->GetOutermost()->ContainsMap() &&								// Skip assets in maps
				 !AllFoundObjects.Contains(Asset)										// Skip assets we have already found to avoid processing them twice
				) )
			{
				ValidLoadedAssets.Add(Asset);
				AllFoundObjects.Add(Asset);
			}
		}

		// Add an entry of the map of source paths to assets found, whether any assets were found or not
		OutSourcePathToLoadedAssets.Add(*PathIt, ValidLoadedAssets);
	}

	GWarn->EndSlowTask();

	ensure(SourcePathNames.Num() == OutSourcePathToLoadedAssets.Num());
}

void ContentBrowserUtils::CopyAssetReferencesToClipboard(const TArray<FAssetData>& AssetsToCopy)
{
	FString ClipboardText;
	for ( auto AssetIt = AssetsToCopy.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		if ( ClipboardText.Len() > 0 )
		{
			ClipboardText += LINE_TERMINATOR;
		}

		ClipboardText += (*AssetIt).GetExportTextName();
	}

	FPlatformMisc::ClipboardCopy( *ClipboardText );
}

void ContentBrowserUtils::CaptureThumbnailFromViewport(FViewport* InViewport, const TArray<FAssetData>& InAssetsToAssign)
{
	//capture the thumbnail
	uint32 SrcWidth = InViewport->GetSizeXY().X;
	uint32 SrcHeight = InViewport->GetSizeXY().Y;
	// Read the contents of the viewport into an array.
	TArray<FColor> OrigBitmap;
	if (InViewport->ReadPixels(OrigBitmap))
	{
		check(OrigBitmap.Num() == SrcWidth * SrcHeight);

		//pin to smallest value
		int32 CropSize = FMath::Min<uint32>(SrcWidth, SrcHeight);
		//pin to max size
		int32 ScaledSize  = FMath::Min<uint32>(ThumbnailTools::DefaultThumbnailSize, CropSize);

		//calculations for cropping
		TArray<FColor> CroppedBitmap;
		CroppedBitmap.AddUninitialized(CropSize*CropSize);
		//Crop the image
		int32 CroppedSrcTop  = (SrcHeight - CropSize)/2;
		int32 CroppedSrcLeft = (SrcWidth - CropSize)/2;
		for (int32 Row = 0; Row < CropSize; ++Row)
		{
			//Row*Side of a row*byte per color
			int32 SrcPixelIndex = (CroppedSrcTop+Row)*SrcWidth + CroppedSrcLeft;
			const void* SrcPtr = &(OrigBitmap[SrcPixelIndex]);
			void* DstPtr = &(CroppedBitmap[Row*CropSize]);
			FMemory::Memcpy(DstPtr, SrcPtr, CropSize*4);
		}

		//Scale image down if needed
		TArray<FColor> ScaledBitmap;
		if (ScaledSize < CropSize)
		{
			FImageUtils::ImageResize( CropSize, CropSize, CroppedBitmap, ScaledSize, ScaledSize, ScaledBitmap, true );
		}
		else
		{
			//just copy the data over. sizes are the same
			ScaledBitmap = CroppedBitmap;
		}

		//setup actual thumbnail
		FObjectThumbnail TempThumbnail;
		TempThumbnail.SetImageSize( ScaledSize, ScaledSize );
		TArray<uint8>& ThumbnailByteArray = TempThumbnail.AccessImageData();

		// Copy scaled image into destination thumb
		int32 MemorySize = ScaledSize*ScaledSize*sizeof(FColor);
		ThumbnailByteArray.AddUninitialized(MemorySize);
		FMemory::Memcpy(&(ThumbnailByteArray[0]), &(ScaledBitmap[0]), MemorySize);

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		//check if each asset should receive the new thumb nail
		for ( auto AssetIt = InAssetsToAssign.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			const FAssetData& CurrentAsset = *AssetIt;

			// check whether this is a type that uses one of the shared static thumbnails
			if ( AssetToolsModule.Get().AssetUsesGenericThumbnail( CurrentAsset ) )
			{
				//assign the thumbnail and dirty
				const FString ObjectFullName = CurrentAsset.GetFullName();
				const FString PackageName    = CurrentAsset.PackageName.ToString();

				UPackage* AssetPackage = FindObject<UPackage>( NULL, *PackageName );
				if ( ensure(AssetPackage) )
				{
					FObjectThumbnail* NewThumbnail = ThumbnailTools::CacheThumbnail(ObjectFullName, &TempThumbnail, AssetPackage);
					if ( ensure(NewThumbnail) )
					{
						//we need to indicate that the package needs to be resaved
						AssetPackage->MarkPackageDirty();

						// Let the content browser know that we've changed the thumbnail
						NewThumbnail->MarkAsDirty();
						
						// Signal that the asset was changed if it is loaded so thumbnail pools will update
						if ( CurrentAsset.IsAssetLoaded() )
						{
							CurrentAsset.GetAsset()->PostEditChange();
						}

						//Set that thumbnail as a valid custom thumbnail so it'll be saved out
						NewThumbnail->SetCreatedAfterCustomThumbsEnabled();
					}
				}
			}
		}
	}
}

void ContentBrowserUtils::ClearCustomThumbnails(const TArray<FAssetData>& InAssetsToAssign)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

	//check if each asset should receive the new thumb nail
	for ( auto AssetIt = InAssetsToAssign.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		const FAssetData& CurrentAsset = *AssetIt;

		// check whether this is a type that uses one of the shared static thumbnails
		if ( AssetToolsModule.Get().AssetUsesGenericThumbnail( CurrentAsset ) )
		{
			//assign the thumbnail and dirty
			const FString ObjectFullName = CurrentAsset.GetFullName();
			const FString PackageName    = CurrentAsset.PackageName.ToString();

			UPackage* AssetPackage = FindObject<UPackage>( NULL, *PackageName );
			if ( ensure(AssetPackage) )
			{
				ThumbnailTools::CacheEmptyThumbnail( ObjectFullName, AssetPackage);

				//we need to indicate that the package needs to be resaved
				AssetPackage->MarkPackageDirty();

				// Signal that the asset was changed if it is loaded so thumbnail pools will update
				if ( CurrentAsset.IsAssetLoaded() )
				{
					CurrentAsset.GetAsset()->PostEditChange();
				}
			}
		}
	}
}

bool ContentBrowserUtils::AssetHasCustomThumbnail( const FAssetData& AssetData )
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if ( AssetToolsModule.Get().AssetUsesGenericThumbnail(AssetData) )
	{
		const FObjectThumbnail* CachedThumbnail = ThumbnailTools::FindCachedThumbnail(AssetData.GetFullName());
		if ( CachedThumbnail != NULL && !CachedThumbnail->IsEmpty() )
		{
			return true;
		}

		// If we don't yet have a thumbnail map, check the disk
		FName ObjectFullName = FName(*AssetData.GetFullName());
		TArray<FName> ObjectFullNames;
		FThumbnailMap LoadedThumbnails;
		ObjectFullNames.Add( ObjectFullName );
		if ( ThumbnailTools::ConditionallyLoadThumbnailsForObjects( ObjectFullNames, LoadedThumbnails ) )
		{
			const FObjectThumbnail* Thumbnail = LoadedThumbnails.Find(ObjectFullName);

			if ( Thumbnail != NULL && !Thumbnail->IsEmpty() )
			{
				return true;
			}
		}
	}

	return false;
}

bool ContentBrowserUtils::IsEngineFolder( const FString& InPath )
{
	return InPath.StartsWith(TEXT("/Engine")) || InPath == TEXT("Engine");
}

bool ContentBrowserUtils::IsDevelopersFolder( const FString& InPath )
{
	const FString DeveloperPathWithSlash = FPackageName::FilenameToLongPackageName(FPaths::GameDevelopersDir());
	const FString DeveloperPathWithoutSlash = DeveloperPathWithSlash.LeftChop(1);
		
	return InPath.StartsWith(DeveloperPathWithSlash) || InPath == DeveloperPathWithoutSlash;
}

void ContentBrowserUtils::GetObjectsInAssetData(const TArray<FAssetData>& AssetList, TArray<UObject*>& OutDroppedObjects)
{	
	for (int32 AssetIdx = 0; AssetIdx < AssetList.Num(); ++AssetIdx)
	{
		const FAssetData& AssetData = AssetList[AssetIdx];

		if ( !AssetData.IsAssetLoaded() && FEditorFileUtils::IsMapPackageAsset(AssetData.ObjectPath.ToString()) )
		{
			// Don't load assets in map packages
			continue;
		}

		UObject* Obj = AssetData.GetAsset();
		if (Obj)
		{
			OutDroppedObjects.Add(Obj);
		}
	}
}

bool ContentBrowserUtils::IsValidFolderName(const FString& FolderName, FText& Reason)
{
	// Check length of the folder name
	if ( FolderName.Len() == 0 )
	{
		Reason = LOCTEXT( "InvalidFolderName_IsTooShort", "Please provide a name for this folder." );
		return false;
	}
		
	if ( FolderName.Len() > MAX_UNREAL_FILENAME_LENGTH )
	{
		Reason = FText::Format( LOCTEXT("InvalidFolderName_TooLongForCooking", "Filename '{0}' is too long; this may interfere with cooking for consoles. Unreal filenames should be no longer than {1} characters." ),
			FText::FromString(FolderName), FText::AsNumber(MAX_UNREAL_FILENAME_LENGTH) );
		return false;
	}

	const FString InvalidChars = INVALID_LONGPACKAGE_CHARACTERS TEXT("/"); // Slash is an invalid character for a folder name

	// See if the name contains invalid characters.
	FString Char;
	for( int32 CharIdx = 0; CharIdx < FolderName.Len(); ++CharIdx )
	{
		Char = FolderName.Mid(CharIdx, 1);

		if ( InvalidChars.Contains(*Char) )
		{
			FString ReadableInvalidChars = InvalidChars;
			ReadableInvalidChars.ReplaceInline(TEXT("\r"), TEXT(""));
			ReadableInvalidChars.ReplaceInline(TEXT("\n"), TEXT(""));
			ReadableInvalidChars.ReplaceInline(TEXT("\t"), TEXT(""));

			Reason = FText::Format(LOCTEXT("InvalidFolderName_InvalidCharacters", "A folder name may not contain any of the following characters: {0}"), FText::FromString(ReadableInvalidChars));
			return false;
		}
	}
	
	return FEditorFileUtils::IsFilenameValidForSaving( FolderName, Reason );
}

bool ContentBrowserUtils::DoesFolderExist(const FString& FolderPath)
{
	TArray<FString> SubPaths;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetSubPaths(FPaths::GetPath(FolderPath), SubPaths, false);

	for(auto SubPathIt(SubPaths.CreateConstIterator()); SubPathIt; SubPathIt++)	
	{
		if ( *SubPathIt == FolderPath )
		{
			return true;
		}
	}

	return false;
}

bool ContentBrowserUtils::IsAssetRootDir(const FString& FolderPath)
{
	return FolderPath == TEXT("/Game") || FolderPath == TEXT("/Engine") || FolderPath == TEXT("/Classes");
}

const TSharedPtr<FLinearColor> ContentBrowserUtils::LoadColor(const FString& FolderPath)
{
	// Ignore classes folder and newly created folders
	if ( FolderPath != TEXT("/Classes") )
	{
		// Load the color from the config at this path
		const FString RelativePath = FPackageName::LongPackageNameToFilename(FolderPath + TEXT("/"));

		// See if we have a value cached first
		TSharedPtr< FLinearColor >* CachedColor = PathColors.Find( RelativePath );
		if ( CachedColor )
		{
			return *CachedColor;
		}
		
		// Loads the color of folder at the given path from the config
		if (FPaths::FileExists(GEditorUserSettingsIni))
		{
			// Create a new entry from the config, skip if it's default
			FString ColorStr;
			if ( GConfig->GetString(TEXT("PathColor"), *RelativePath, ColorStr, GEditorUserSettingsIni) )
			{
				FLinearColor Color;
				if( Color.InitFromString( ColorStr ) && !Color.Equals( ContentBrowserUtils::GetDefaultColor() ) )
				{
					return PathColors.Add( RelativePath, MakeShareable( new FLinearColor( Color ) ) );
				}
			}
			else
			{
				return PathColors.Add( RelativePath, MakeShareable( new FLinearColor( ContentBrowserUtils::GetDefaultColor() ) ) );
			}
		}
	}

	return NULL;
}

void ContentBrowserUtils::SaveColor(const FString& FolderPath, const TSharedPtr<FLinearColor> FolderColor, bool bForceAdd)
{
	check( FolderPath != TEXT("/Classes") );

	const FString RelativePath = FPackageName::LongPackageNameToFilename(FolderPath + TEXT("/"));

	// Remove the color if it's invalid or default
	const bool bRemove = !FolderColor.IsValid() || ( !bForceAdd && FolderColor->Equals( ContentBrowserUtils::GetDefaultColor() ) );

	// Saves the color of the folder to the config
	if (FPaths::FileExists(GEditorUserSettingsIni))
	{
		// If this is no longer custom, remove it
		if ( bRemove )
		{
			GConfig->RemoveKey(TEXT("PathColor"), *RelativePath, GEditorUserSettingsIni);
		}
		else
		{
			GConfig->SetString(TEXT("PathColor"), *RelativePath, *FolderColor->ToString(), GEditorUserSettingsIni);
		}
	}

	// Update the map too
	if ( bRemove )
	{
		PathColors.Remove( RelativePath );
	}
	else
	{
		PathColors.Add( RelativePath, FolderColor );
	}
}

bool ContentBrowserUtils::HasCustomColors( TArray< FLinearColor >* OutColors )
{
	// Check to see how many paths are currently using this color
	// Note: we have to use the config, as paths which haven't been rendered yet aren't registered in the map
	bool bHasCustom = false;
	if (FPaths::FileExists(GEditorUserSettingsIni))
	{
		// Read individual entries from a config file.
		TArray< FString > Section; 
		GConfig->GetSection( TEXT("PathColor"), Section, GEditorUserSettingsIni );

		for( int32 SectionIndex = 0; SectionIndex < Section.Num(); SectionIndex++ )
		{
			FString EntryStr = Section[ SectionIndex ];
			EntryStr.Trim();

			FString PathStr;
			FString ColorStr;
			if ( EntryStr.Split( TEXT( "/=" ), &PathStr, &ColorStr ) )	// DoesFolderExist doesn't like ending in a '/' so trim it here
			{
				// Ignore any that reference old folders
				if ( FPackageName::TryConvertFilenameToLongPackageName(PathStr, PathStr) && DoesFolderExist( PathStr ) )
				{
					// Ignore any that have invalid or default colors
					FLinearColor CurrentColor;
					if( CurrentColor.InitFromString( ColorStr ) && !CurrentColor.Equals( ContentBrowserUtils::GetDefaultColor() ) )
					{
						bHasCustom = true;
						if ( OutColors )
						{
							// Only add if not already present (ignores near matches too)
							bool bAdded = false;
							for( int32 ColorIndex = 0; ColorIndex < OutColors->Num(); ColorIndex++ )
							{
								const FLinearColor& Color = (*OutColors)[ ColorIndex ];
								if( CurrentColor.Equals( Color ) )
								{
									bAdded = true;
									break;
								}
							}
							if ( !bAdded )
							{
								OutColors->Add( CurrentColor );
							}
						}
						else
						{
							break;
						}
					}
				}
			}
		}
	}
	return bHasCustom;
}

FLinearColor ContentBrowserUtils::GetDefaultColor()
{
	// The default tint the folder should appear as
	return FLinearColor::Gray;
}

FText ContentBrowserUtils::GetExploreFolderText()
{
#if PLATFORM_MAC
	return LOCTEXT("Mac_ShowInFinder", "Show In Finder");
#else
	return LOCTEXT("Win_ShowInExplorer", "Show In Explorer");
#endif
}

#undef LOCTEXT_NAMESPACE
