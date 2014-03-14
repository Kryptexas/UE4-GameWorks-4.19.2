// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "AssetToolsPrivatePCH.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ISourceControlModule.h"
#include "FileHelpers.h"
#include "ObjectTools.h"
#include "MainFrame.h"

#define LOCTEXT_NAMESPACE "AssetRenameManager"

struct FAssetRenameDataWithReferencers : public FAssetRenameData
{
	TArray<FName> ReferencingPackageNames;
	FString FailureReason;
	bool bCreateRedirector;
	bool bRenameFailed;

	FAssetRenameDataWithReferencers(const FAssetRenameData& InRenameData)
		: FAssetRenameData(InRenameData)
		, bCreateRedirector(false)
		, bRenameFailed(false)
	{}
};

class SRenameFailures : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRenameFailures){}

		SLATE_ARGUMENT(TArray<FString>, FailedRenames)

	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& InArgs )
	{
		for ( auto RenameIt = InArgs._FailedRenames.CreateConstIterator(); RenameIt; ++RenameIt )
		{
			FailedRenames.Add( MakeShareable( new FString(*RenameIt) ) );
		}

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush") )
			.Padding(FMargin(4, 8, 4, 4))
			[
				SNew(SVerticalBox)

				// Title text
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock) .Text( LOCTEXT("RenameFailureTitle", "The following assets could not be renamed.") )
				]

				// Failure list
				+SVerticalBox::Slot()
				.Padding(0, 8)
				.FillHeight(1.f)
				[
					SNew(SBorder)
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					[
						SNew(SListView<TSharedPtr<FString>>)
						.ListItemsSource(&FailedRenames)
						.SelectionMode(ESelectionMode::None)
						.OnGenerateRow(this, &SRenameFailures::MakeListViewWidget)
					]
				]

				// Close button
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 4)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.OnClicked(this, &SRenameFailures::CloseClicked)
					.Text(LOCTEXT("RenameFailuresCloseButton", "Close"))
				]
			]
		];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	static void OpenRenameFailuresDialog(const TArray<FString>& InFailedRenames)
	{
		TSharedRef<SWindow> RenameWindow = SNew(SWindow)
			.Title(LOCTEXT("FailedRenamesDialog", "Failed Renames"))
			.ClientSize(FVector2D(800,400))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			[
				SNew(SRenameFailures).FailedRenames(InFailedRenames)
			];

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));

		if ( MainFrameModule.GetParentWindow().IsValid() )
		{
			FSlateApplication::Get().AddWindowAsNativeChild(RenameWindow, MainFrameModule.GetParentWindow().ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(RenameWindow);
		}
	}

private:
	TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return
			SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
			[
				SNew(STextBlock) .Text( FText::FromString( *Item.Get() ) )
			];
	}

	FReply CloseClicked()
	{
		FWidgetPath WidgetPath;
		TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(AsShared(), WidgetPath);

		if ( Window.IsValid() )
		{
			Window->RequestDestroyWindow();
		}

		return FReply::Handled();
	}

private:
	TArray< TSharedPtr<FString> > FailedRenames;
};


///////////////////////////
// FAssetRenameManager
///////////////////////////

void FAssetRenameManager::RenameAssets(const TArray<FAssetRenameData>& AssetsAndNames) const
{
	// Prep a list of assets to rename with an extra boolean to determine if they should leave a redirector or not
	TArray<FAssetRenameDataWithReferencers> AssetsToRename;
	for ( int32 AssetIdx = 0; AssetIdx < AssetsAndNames.Num(); ++AssetIdx )
	{
		new(AssetsToRename) FAssetRenameDataWithReferencers(AssetsAndNames[AssetIdx]);
	}

	// If the asset registry is still loading assets, we cant check for referencers, so we must open the rename dialog
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	if ( AssetRegistryModule.Get().IsLoadingAssets() )
	{
		// Open a dialog asking the user to wait while assets are being discovered
		SDiscoveringAssetsDialog::OpenDiscoveringAssetsDialog(
			SDiscoveringAssetsDialog::FOnAssetsDiscovered::CreateSP(this, &FAssetRenameManager::FixReferencesAndRename, AssetsToRename)
			);
	}
	else
	{
		// No need to wait, attempt to fix references and rename now.
		FixReferencesAndRename(AssetsToRename);
	}
}

void FAssetRenameManager::FixReferencesAndRename(TArray<FAssetRenameDataWithReferencers> AssetsToRename) const
{
	// Warn the user if they are about to rename an asset that is referenced by a CDO
	auto CDOAssets = FindCDOReferencedAssets(AssetsToRename);

	// Warn the user if there were any references
	if (CDOAssets.Num())
	{
		FString AssetNames;
		for (auto AssetIt = CDOAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			UObject* Asset = (*AssetIt).Get();
			if (Asset)
			{
				AssetNames += FString("\n") + Asset->GetName();
			}
		}

		const FText MessageText = FText::Format(LOCTEXT("RenameCDOReferences", "The following assets are referenced by one or more Class Default Objects: \n{0}\n\nContinuing with the rename may require code changes to fix these references. Do you wish to continue?"), FText::FromString(AssetNames) );
		if (FMessageDialog::Open(EAppMsgType::YesNo, MessageText) == EAppReturnType::No)
		{
			return;
		}
	}

	// Fill out the referencers for the assets we are renaming
	PopulateAssetReferencers(AssetsToRename);

	// Update the source control state for the packages containing the assets we are renaming if source control is enabled. If source control is enabled and this fails we can not continue.
	if ( UpdatePackageStatus(AssetsToRename) )
	{
		// Load all referencing packages and mark any assets that must have redirectors.
		TArray<UPackage*> ReferencingPackagesToSave;
		LoadReferencingPackages(AssetsToRename, ReferencingPackagesToSave);

		// Prompt to check out all referencing packages, leave redirectors for assets referenced by packages that are not checked out and remove those packages from the save list.
		const bool bUserAcceptedCheckout = CheckOutReferencingPackages(AssetsToRename, ReferencingPackagesToSave);

		if ( bUserAcceptedCheckout )
		{
			// If any referencing packages are left read-only, the checkout failed or SCC was not enabled. Trim them from the save list and leave redirectors.
			DetectReadOnlyPackages(AssetsToRename, ReferencingPackagesToSave);

			// Perform the rename, leaving redirectors only for assets which need them
			PerformAssetRename(AssetsToRename);

			// Save all packages that were referencing any of the assets that were moved without redirectors
			SaveReferencingPackages(ReferencingPackagesToSave);

			// Finally, report any failures that happened during the rename
			ReportFailures(AssetsToRename);
		}
	}
}

TArray<TWeakObjectPtr<UObject>> FAssetRenameManager::FindCDOReferencedAssets(const TArray<FAssetRenameDataWithReferencers>& AssetsToRename) const
{
	TArray<TWeakObjectPtr<UObject>> CDOAssets, LocalAssetsToRename;
	for (auto AssetIt = AssetsToRename.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		LocalAssetsToRename.Push((*AssetIt).Asset);
	}

	// Run over all CDOs and check for any references to the assets
	for ( TObjectIterator<UClass> ClassDefaultObjectIt; ClassDefaultObjectIt; ++ClassDefaultObjectIt)
	{
		UClass* Cls = (*ClassDefaultObjectIt);
		UObject* CDO = Cls->ClassDefaultObject;

		if (!CDO || !CDO->HasAllFlags(RF_ClassDefaultObject))
		{
			continue;
		}

		for (TFieldIterator<UObjectProperty> PropertyIt(Cls); PropertyIt; ++PropertyIt)
		{
			const UObject* Object = PropertyIt->GetPropertyValue(PropertyIt->ContainerPtrToValuePtr<UObject>(CDO));
			for (auto AssetIt = LocalAssetsToRename.CreateConstIterator(); AssetIt; ++AssetIt)
			{
				auto Asset = *AssetIt;
				if (Object == Asset.Get())
				{
					CDOAssets.Push(Asset);
					LocalAssetsToRename.Remove(Asset);

					if (LocalAssetsToRename.Num() == 0)
					{
						// No more assets to check
						return MoveTemp(CDOAssets);
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	return MoveTemp(CDOAssets);
}

void FAssetRenameManager::PopulateAssetReferencers(TArray<FAssetRenameDataWithReferencers>& AssetsToPopulate) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TSet<FName> RenamingAssetPackageNames;

	// Get the names of all the packages containing the assets we are renaming so they arent added to the referencing packages list
	for ( auto AssetIt = AssetsToPopulate.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		UObject* Asset = (*AssetIt).Asset.Get();
		if ( Asset )
		{
			RenamingAssetPackageNames.Add( FName(*Asset->GetOutermost()->GetName()) );
		}
	}

	// Gather all referencing packages for all assets that are being renamed
	for ( auto AssetIt = AssetsToPopulate.CreateIterator(); AssetIt; ++AssetIt )
	{
		(*AssetIt).ReferencingPackageNames.Empty();

		UObject* Asset = (*AssetIt).Asset.Get();
		if ( Asset )
		{
			TArray<FName> Referencers;
			AssetRegistryModule.Get().GetReferencers(Asset->GetOutermost()->GetFName(), Referencers);

			for ( auto ReferenceIt = Referencers.CreateConstIterator(); ReferenceIt; ++ReferenceIt )
			{
				const FName& ReferencingPackageName = *ReferenceIt;

				if ( !RenamingAssetPackageNames.Contains(ReferencingPackageName) )
				{
					(*AssetIt).ReferencingPackageNames.AddUnique( ReferencingPackageName );
				}
			}
		}
	}
}

bool FAssetRenameManager::UpdatePackageStatus(const TArray<FAssetRenameDataWithReferencers>& AssetsToRename) const
{
	if ( ISourceControlModule::Get().IsEnabled() )
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

		// Update the source control server availability to make sure we can do the rename operation
		SourceControlProvider.Login();
		if ( !SourceControlProvider.IsAvailable() )
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "SourceControl_ServerUnresponsive", "Source Control is unresponsive. Please check your connection and try again.") );
			return false;
		}

		// Gather asset package names to update SCC states in a single SCC request
		TArray<UPackage*> PackagesToUpdate;
		for ( auto AssetIt = AssetsToRename.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			UObject* Asset = (*AssetIt).Asset.Get();
			if ( Asset )
			{
				PackagesToUpdate.AddUnique(Asset->GetOutermost());
			}
		}

		SourceControlProvider.Execute(ISourceControlOperation::Create<FUpdateStatus>(), PackagesToUpdate);
	}

	return true;
}

void FAssetRenameManager::LoadReferencingPackages(TArray<FAssetRenameDataWithReferencers>& AssetsToRename, TArray<UPackage*>& OutReferencingPackagesToSave) const
{
	const FText ReferenceUpdateSlowTask = LOCTEXT("ReferenceUpdateSlowTask", "Updating Asset References");
	GWarn->BeginSlowTask( ReferenceUpdateSlowTask, true );

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

	for ( int32 AssetIdx = 0; AssetIdx < AssetsToRename.Num(); ++AssetIdx )
	{
		GWarn->StatusUpdate( AssetIdx, AssetsToRename.Num(), ReferenceUpdateSlowTask );

		FAssetRenameDataWithReferencers& RenameData = AssetsToRename[AssetIdx];

		UObject* Asset = RenameData.Asset.Get();
		if ( Asset )
		{
			// Make sure this asset is local. Only local assets should be renamed without a redirector
			FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(Asset->GetOutermost(), EStateCacheUsage::ForceUpdate);
			const bool bLocalFile = !SourceControlState.IsValid() || SourceControlState->IsAdded() || !SourceControlState->IsSourceControlled() || SourceControlState->IsIgnored();
			if ( !bLocalFile )
			{
				// This asset is not local. It is not safe to rename it without leaving a redirector
				RenameData.bCreateRedirector = true;

				// If this asset is locked or not current, mark it failed to prevent it from being renamed
				if ( SourceControlState->IsCheckedOutOther() )
				{
					RenameData.bRenameFailed = true;
					RenameData.FailureReason = LOCTEXT("RenameFailedCheckedOutByOther", "Checked out by another user.").ToString();
				}
				else if ( !SourceControlState->IsCurrent() )
				{
					RenameData.bRenameFailed = true;
					RenameData.FailureReason = LOCTEXT("RenameFailedNotCurrent", "Out of date.").ToString();
				}

				continue;
			}
		}
		else
		{
			// The asset for this rename must have been GCed or is otherwise invalid. Skip it
			continue;
		}

		TArray<UPackage*> PackagesToSaveForThisAsset;
		bool bAllPackagesLoadedForThisAsset = true;
		for ( auto PackageNameIt = RenameData.ReferencingPackageNames.CreateConstIterator(); PackageNameIt; ++PackageNameIt )
		{
			const FString PackageName = (*PackageNameIt).ToString();

			// Check if the package is a map before loading it!
			if ( FEditorFileUtils::IsMapPackageAsset(PackageName) )
			{
				// This reference was a map package, don't load it and leave a redirector for this asset
				RenameData.bCreateRedirector = true;
				bAllPackagesLoadedForThisAsset = false;
				break;
			}

			UPackage* Package = LoadPackage(NULL, *PackageName, LOAD_None);
			if ( Package )
			{
				PackagesToSaveForThisAsset.Add(Package);
			}
			else
			{
				RenameData.bCreateRedirector = true;
				bAllPackagesLoadedForThisAsset = false;
				break;
			}
		}

		if ( bAllPackagesLoadedForThisAsset )
		{
			OutReferencingPackagesToSave.Append(PackagesToSaveForThisAsset);
		}
	}

	GWarn->EndSlowTask();
}

bool FAssetRenameManager::CheckOutReferencingPackages(TArray<FAssetRenameDataWithReferencers>& AssetsToRename, TArray<UPackage*>& InOutReferencingPackagesToSave) const
{
	bool bUserAcceptedCheckout = true;
	
	if ( InOutReferencingPackagesToSave.Num() > 0 )
	{
		if ( ISourceControlModule::Get().IsEnabled() )
		{
			TArray<UPackage*> PackagesCheckedOutOrMadeWritable;
			TArray<UPackage*> PackagesNotNeedingCheckout;
			bUserAcceptedCheckout = FEditorFileUtils::PromptToCheckoutPackages( false, InOutReferencingPackagesToSave, &PackagesCheckedOutOrMadeWritable, &PackagesNotNeedingCheckout );
			if ( bUserAcceptedCheckout )
			{
				TArray<UPackage*> PackagesThatCouldNotBeCheckedOut = InOutReferencingPackagesToSave;

				for ( auto PackageIt = PackagesCheckedOutOrMadeWritable.CreateConstIterator(); PackageIt; ++PackageIt )
				{
					PackagesThatCouldNotBeCheckedOut.Remove(*PackageIt);
				}

				for ( auto PackageIt = PackagesNotNeedingCheckout.CreateConstIterator(); PackageIt; ++PackageIt )
				{
					PackagesThatCouldNotBeCheckedOut.Remove(*PackageIt);
				}

				for ( auto PackageIt = PackagesThatCouldNotBeCheckedOut.CreateConstIterator(); PackageIt; ++PackageIt )
				{
					const FName NonCheckedOutPackageName = (*PackageIt)->GetFName();

					for ( auto RenameDataIt = AssetsToRename.CreateIterator(); RenameDataIt; ++RenameDataIt )
					{
						FAssetRenameDataWithReferencers& RenameData = *RenameDataIt;
						if ( RenameData.ReferencingPackageNames.Contains(NonCheckedOutPackageName) )
						{
							RenameData.bCreateRedirector = true;
						}
					}

					InOutReferencingPackagesToSave.Remove(*PackageIt);
				}
			}
		}
	}

	return bUserAcceptedCheckout;
}

void FAssetRenameManager::DetectReadOnlyPackages(TArray<FAssetRenameDataWithReferencers>& AssetsToRename, TArray<UPackage*>& InOutReferencingPackagesToSave) const
{
	// For each valid package...
	for ( int32 PackageIdx = InOutReferencingPackagesToSave.Num() - 1; PackageIdx >= 0; --PackageIdx )
	{
		UPackage* Package = InOutReferencingPackagesToSave[PackageIdx];

		if ( Package )
		{
			// Find the package filename
			FString Filename;
			if ( FPackageName::DoesPackageExist(Package->GetName(), NULL, &Filename) )
			{
				// If the file is read only
				if ( IFileManager::Get().IsReadOnly(*Filename) )
				{
					FName PackageName = Package->GetFName();

					// Find all assets that were referenced by this package to create a redirector when named
					for ( auto RenameDataIt = AssetsToRename.CreateIterator(); RenameDataIt; ++RenameDataIt )
					{
						FAssetRenameDataWithReferencers& RenameData = *RenameDataIt;
						if ( RenameData.ReferencingPackageNames.Contains(PackageName) )
						{
							RenameData.bCreateRedirector = true;
						}
					}

					// Remove the package from the save list
					InOutReferencingPackagesToSave.RemoveAt(PackageIdx);
				}
			}
		}
	}
}

void FAssetRenameManager::PerformAssetRename(TArray<FAssetRenameDataWithReferencers>& AssetsToRename) const
{
	const FText AssetRenameSlowTask = LOCTEXT("AssetRenameSlowTask", "Renaming Assets");
	GWarn->BeginSlowTask( AssetRenameSlowTask, true );

	TArray<UPackage*> PackagesToSave;
	TArray<UPackage*> PotentialPackagesToDelete;
	for ( int32 AssetIdx = 0; AssetIdx < AssetsToRename.Num(); ++AssetIdx )
	{
		GWarn->StatusUpdate( AssetIdx, AssetsToRename.Num(), AssetRenameSlowTask );

		FAssetRenameDataWithReferencers& RenameData = AssetsToRename[AssetIdx];

		if ( RenameData.bRenameFailed )
		{
			// The rename failed at some earlier step, skip this asset
			continue;
		}

		UObject* Asset = RenameData.Asset.Get();

		if ( !Asset )
		{
			// This asset was invalid or GCed before the rename could occur
			RenameData.bRenameFailed = true;
			continue;
		}

		ObjectTools::FPackageGroupName PGN;
		PGN.ObjectName = RenameData.NewName;
		PGN.GroupName = TEXT("");
		PGN.PackageName = RenameData.PackagePath + TEXT("/") + PGN.ObjectName;
		const bool bLeaveRedirector = RenameData.bCreateRedirector;

		UPackage* OldPackage = Asset->GetOutermost();
		bool bOldPackageAddedToRootSet = false;
		if ( !bLeaveRedirector && !OldPackage->IsRooted() )
		{
			bOldPackageAddedToRootSet = true;
			OldPackage->AddToRoot();
		}

		TSet<UPackage*> ObjectsUserRefusedToFullyLoad;
		FText ErrorMessage;
		if ( ObjectTools::RenameSingleObject(Asset, PGN, ObjectsUserRefusedToFullyLoad, ErrorMessage, NULL, bLeaveRedirector) )
		{
			PackagesToSave.AddUnique( Asset->GetOutermost() );

			// Automatically save renamed assets
			if ( bLeaveRedirector )
			{
				PackagesToSave.AddUnique( OldPackage );
			}
			else if ( bOldPackageAddedToRootSet )
			{
				// Since we did not leave a redirector and the old package wasnt already rooted, attempt to delete it when we are done. 
				PotentialPackagesToDelete.AddUnique(OldPackage);
			}
		}
		else
		{
			// No need to keep the old package rooted, the asset was never renamed out of it
			if ( bOldPackageAddedToRootSet )
			{
				OldPackage->RemoveFromRoot();
			}

			// Mark the rename as a failure to report it later
			RenameData.bRenameFailed = true;
			RenameData.FailureReason = ErrorMessage.ToString();
		}
	}

	GWarn->EndSlowTask();

	// Save all renamed assets and any redirectors that were left behind
	if ( PackagesToSave.Num() > 0 )
	{
		const bool bCheckDirty = false;
		const bool bPromptToSave = false;
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);

		ISourceControlModule::Get().QueueStatusUpdate(PackagesToSave);
	}

	// Clean up all packages that were left empty
	if ( PotentialPackagesToDelete.Num() > 0 )
	{
		for ( auto PackageIt = PotentialPackagesToDelete.CreateConstIterator(); PackageIt; ++PackageIt )
		{
			(*PackageIt)->RemoveFromRoot();
		}

		ObjectTools::CleanupAfterSuccessfulDelete(PotentialPackagesToDelete);
	}
}

void FAssetRenameManager::SaveReferencingPackages(const TArray<UPackage*>& ReferencingPackagesToSave) const
{
	if ( ReferencingPackagesToSave.Num() > 0 )
	{
		const bool bCheckDirty = false;
		const bool bPromptToSave = false;
		FEditorFileUtils::PromptForCheckoutAndSave(ReferencingPackagesToSave, bCheckDirty, bPromptToSave);

		ISourceControlModule::Get().QueueStatusUpdate(ReferencingPackagesToSave);
	}
}

void FAssetRenameManager::ReportFailures(const TArray<FAssetRenameDataWithReferencers>& AssetsToRename) const
{
	TArray<FString> FailedRenames;
	for ( auto RenameIt = AssetsToRename.CreateConstIterator(); RenameIt; ++RenameIt )
	{
		const FAssetRenameDataWithReferencers& RenameData = *RenameIt;
		if ( RenameData.bRenameFailed )
		{
			UObject* Asset = RenameData.Asset.Get();
			if ( Asset )
			{
				FailedRenames.Add(Asset->GetOutermost()->GetName() + TEXT(" - ") + RenameData.FailureReason);
			}
			else
			{
				FailedRenames.Add(LOCTEXT("InvalidAssetText", "Invalid Asset").ToString());
			}
		}
	}

	if ( FailedRenames.Num() > 0 )
	{
		SRenameFailures::OpenRenameFailuresDialog(FailedRenames);
	}
}

#undef LOCTEXT_NAMESPACE