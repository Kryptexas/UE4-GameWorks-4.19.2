// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorManager.h"
#include "Editor/UnrealEd/Public/PackageTools.h"
#include "AssetRegistryModule.h"
#include "Editor/Persona/Public/PersonaModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "SSkeletonWidget.h"

#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Skeleton::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Skeletons = GetTypedWeakObjectPtrs<USkeleton>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Skeleton_Edit", "Edit"),
		LOCTEXT("Skeleton_EditTooltip", "Opens the selected skeleton in the persona."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteEdit, Skeletons ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Skeleton_NewAnimBlueprint", "Create Anim Blueprint"),
		LOCTEXT("Skeleton_NewAnimBlueprintTooltip", "Creates an Anim Blueprint using the selected skeleton."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteNewAnimBlueprint, Skeletons ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddSubMenu( 
		LOCTEXT( "SkeletalMesh_NewAimOffset", "Create AimOffset" ), 
		LOCTEXT("SkeletalMesh_NewAimOffsetTooltip", "Creates an aimoffset blendspace using the selected skeleton."),
		FNewMenuDelegate::CreateSP( this, &FAssetTypeActions_Skeleton::FillAimOffsetBlendSpaceMenu, Skeletons ) );

	MenuBuilder.AddSubMenu( 
		LOCTEXT( "SkeletalMesh_NewBlendspace", "Create BlendSpace" ), 
		LOCTEXT( "SkeletalMesh_NewBlendspaceTooltip", "Creates a blendspace using the skeleton of the selected mesh." ),
		FNewMenuDelegate::CreateSP( this, &FAssetTypeActions_Skeleton::FillBlendSpaceMenu, Skeletons ) );

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Skeleton_NewAnimComposite", "Create AnimComposite"),
		LOCTEXT("Skeleton_NewAnimCompositeTooltip", "Creates an AnimComposite using the selected skeleton."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteNewAnimAsset<UAnimCompositeFactory, UAnimComposite>, Skeletons, FString("_Composite") ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Skeleton_NewAnimMontage", "Create AnimMontage"),
		LOCTEXT("Skeleton_NewAnimMontageTooltip", "Creates an AnimMontage using the selected skeleton."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteNewAnimAsset<UAnimMontageFactory, UAnimMontage>, Skeletons, FString("_Montage") ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Skeleton_Retarget", "Retarget to another Skeleton"),
		LOCTEXT("Skeleton_RetargetTooltip", "Allow all animation assets for this skeleton retarget to another skeleton."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteRetargetSkeleton, Skeletons ),
			FCanExecuteAction()
			)
		);

	// @todo ImportAnimation
	/*
	MenuBuilder.AddMenuEntry(
		LOCTEXT("Skeleton_ImportAnimation", "Import Animation"),
		LOCTEXT("Skeleton_ImportAnimationTooltip", "Imports an animation for the selected skeleton."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteImportAnimation, Skeletons ),
			FCanExecuteAction()
			)
		);
	*/
}

void FAssetTypeActions_Skeleton::FillBlendSpaceMenu( FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons )
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SkeletalMesh_New1DBlendspace", "Create 1D BlendSpace"),
		LOCTEXT("SkeletalMesh_New1DBlendspaceTooltip", "Creates a 1D blendspace using the skeleton of the selected mesh."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteNewAnimAsset<UBlendSpaceFactory1D, UBlendSpace1D>, Skeletons, FString("_BlendSpace1D") ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SkeletalMesh_New2DBlendspace", "Create 2D BlendSpace"),
		LOCTEXT("SkeletalMesh_New2DBlendspaceTooltip", "Creates a 2D blendspace using the skeleton of the selected mesh."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteNewAnimAsset<UBlendSpaceFactoryNew, UBlendSpace>, Skeletons, FString("_BlendSpace2D") ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_Skeleton::FillAimOffsetBlendSpaceMenu( FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons )
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SkeletalMesh_New1DAimOffset", "Create 1D AimOffset"),
		LOCTEXT("SkeletalMesh_New1DAimOffsetTooltip", "Creates a 1D aimoffset blendspace using the selected skeleton."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteNewAnimAsset<UAimOffsetBlendSpaceFactory1D, UAimOffsetBlendSpace1D>, Skeletons, FString("_AimOffset1D") ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SkeletalMesh_New2DAimOffset", "Create 2D AimOffset"),
		LOCTEXT("SkeletalMesh_New2DAimOffsetTooltip", "Creates a 2D aimoffset blendspace using the selected skeleton."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Skeleton::ExecuteNewAnimAsset<UAimOffsetBlendSpaceFactoryNew, UAimOffsetBlendSpace>, Skeletons, FString("_AimOffset2D") ),
			FCanExecuteAction()
			)
		);
}

void FAssetTypeActions_Skeleton::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Skeleton = Cast<USkeleton>(*ObjIt);
		if (Skeleton != NULL)
		{
			FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );
			PersonaModule.CreatePersona( Mode, EditWithinLevelEditor, Skeleton, NULL, NULL, NULL );
		}
	}
}

void FAssetTypeActions_Skeleton::ExecuteEdit(TArray<TWeakObjectPtr<USkeleton>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

template <typename TFactory, typename T>
void FAssetTypeActions_Skeleton::ExecuteNewAnimAsset(TArray<TWeakObjectPtr<USkeleton>> Objects, const FString InSuffix)
{
	if ( Objects.Num() == 1 )
	{
		auto Object = Objects[0].Get();

		if ( Object )
		{
			// Determine an appropriate name for inline-rename
			FString Name;
			FString PackageName;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), InSuffix, PackageName, Name);

			TFactory* Factory = ConstructObject<TFactory>(TFactory::StaticClass());
			Factory->TargetSkeleton = Object;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackageName), T::StaticClass(), Factory);
		}
	}
	else
	{
		CreateAnimationAssets(Objects, T::StaticClass(), InSuffix);
	}
}

void FAssetTypeActions_Skeleton::ExecuteNewAnimBlueprint(TArray<TWeakObjectPtr<USkeleton>> Objects)
{
	const FString DefaultSuffix = TEXT("_AnimBlueprint");

	if ( Objects.Num() == 1 )
	{
		auto Object = Objects[0].Get();

		if ( Object )
		{
			// Determine an appropriate name for inline-rename
			FString Name;
			FString PackageName;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

			UAnimBlueprintFactory* Factory = ConstructObject<UAnimBlueprintFactory>(UAnimBlueprintFactory::StaticClass());
			Factory->TargetSkeleton = Object;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackageName), UAnimBlueprint::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;
		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();
			if ( Object )
			{
				// Determine an appropriate name
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the anim blueprint factory used to generate the asset
				UAnimBlueprintFactory* Factory = ConstructObject<UAnimBlueprintFactory>(UAnimBlueprintFactory::StaticClass());
				Factory->TargetSkeleton = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UAnimBlueprint::StaticClass(), Factory);

				if ( NewAsset )
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if ( ObjectsToSync.Num() > 0 )
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

void FAssetTypeActions_Skeleton::CreateAnimationAssets(const TArray<TWeakObjectPtr<USkeleton>>& Skeletons, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix)
{
	TArray<UObject*> ObjectsToSync;
	for (auto SkelIt = Skeletons.CreateConstIterator(); SkelIt; ++SkelIt)
	{
		USkeleton* Skeleton = (*SkelIt).Get();
		if ( Skeleton )
		{
			// Determine an appropriate name
			FString Name;
			FString PackageName;
			CreateUniqueAssetName(Skeleton->GetOutermost()->GetName(), InPrefix, PackageName, Name);

			// Create the asset, and assign its skeleton
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
			UAnimationAsset* NewAsset = Cast<UAnimationAsset>(AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), AssetClass, NULL));

			if ( NewAsset )
			{
				NewAsset->SetSkeleton(Skeleton);
				NewAsset->MarkPackageDirty();

				ObjectsToSync.Add(NewAsset);
			}
		}
	}

	if ( ObjectsToSync.Num() > 0 )
	{
		FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
	}
}

void FAssetTypeActions_Skeleton::ExecuteRetargetSkeleton(TArray<TWeakObjectPtr<USkeleton>> Skeletons)
{
	// warn the user to shut down any persona that is opened
	if ( FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("CloseReferencingEditors", "You need to close Persona or anything that references animation, mesh or animation blueprint before this step. Continue?")) == EAppReturnType::Yes )
	{
		TArray<UObject*> ObjectsToSync;
		for (auto SkelIt = Skeletons.CreateConstIterator(); SkelIt; ++SkelIt)
		{	
			USkeleton * OldSkeleton = (*SkelIt).Get();
			USkeleton * NewSkeleton = NULL;

			const FText Message = LOCTEXT("RemapSkeleton_Warning", "This only converts animation data -i.e. animation assets and Anim Blueprints. \nIf you'd like to convert SkeletalMesh, use the context menu (Assign Skeleton) for each mesh. \n\nIf you'd like to convert mesh as well, please do so before converting animation data. \nOtherwise you will lose any extra track that is in the new mesh.");

			// ask user what they'd like to change to 
			if (SAnimationRemapSkeleton::ShowModal(OldSkeleton, NewSkeleton, Message ))
			{
				// find all assets who references old skeleton
				TArray<FName> Packages;

				// If the asset registry is still loading assets, we cant check for referencers, so we must open the rename dialog
				FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				AssetRegistryModule.Get().GetReferencers(OldSkeleton->GetOutermost()->GetFName(), Packages);

				if ( AssetRegistryModule.Get().IsLoadingAssets() )
				{
					// Open a dialog asking the user to wait while assets are being discovered
					SDiscoveringAssetsDialog::OpenDiscoveringAssetsDialog(
						SDiscoveringAssetsDialog::FOnAssetsDiscovered::CreateSP(this, &FAssetTypeActions_Skeleton::PerformRetarget, OldSkeleton, NewSkeleton, Packages)
						);
				}
				else
				{
					PerformRetarget(OldSkeleton, NewSkeleton, Packages);
				}
				
			}
		}
	}
}

void FAssetTypeActions_Skeleton::PerformRetarget(USkeleton *OldSkeleton, USkeleton *NewSkeleton, TArray<FName> Packages) const
{
	TArray<FAssetToRemapSkeleton> AssetsToRemap;

	// populate AssetsToRemap
	AssetsToRemap.Empty(Packages.Num());

	for ( int32 AssetIndex=0; AssetIndex<Packages.Num(); ++AssetIndex)
	{
		AssetsToRemap.Add( FAssetToRemapSkeleton(Packages[AssetIndex]) );
	}

	// Load all packages 
	TArray<UPackage*> PackagesToSave;
	LoadPackages(AssetsToRemap, PackagesToSave);

	// Update the source control state for the packages containing the assets we are remapping
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( ISourceControlModule::Get().IsEnabled() )
	{
		SourceControlProvider.Execute(ISourceControlOperation::Create<FUpdateStatus>(), PackagesToSave);
	}

	// Prompt to check out all referencing packages, leave redirectors for assets referenced by packages that are not checked out and remove those packages from the save list.
	const bool bUserAcceptedCheckout = CheckOutPackages(AssetsToRemap, PackagesToSave);

	if ( bUserAcceptedCheckout )
	{
		// If any referencing packages are left read-only, the checkout failed or SCC was not enabled. Trim them from the save list and leave redirectors.
		DetectReadOnlyPackages(AssetsToRemap, PackagesToSave);

		// retarget skeleton
		RetargetSkeleton(AssetsToRemap, OldSkeleton, NewSkeleton);

		// Save all packages that were referencing any of the assets that were moved without redirectors
		SavePackages(PackagesToSave);

		// Finally, report any failures that happened during the rename
		ReportFailures(AssetsToRemap);
	}
}

void FAssetTypeActions_Skeleton::LoadPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& OutPackagesToSave) const
{
	const FText StatusUpdate = LOCTEXT("RemapSkeleton_LoadPackage", "Loading Packages");
	GWarn->BeginSlowTask( StatusUpdate, true );

	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	// go through all assets try load
	for ( int32 AssetIdx = 0; AssetIdx < AssetsToRemap.Num(); ++AssetIdx )
	{
		GWarn->StatusUpdate( AssetIdx, AssetsToRemap.Num(), StatusUpdate );

		FAssetToRemapSkeleton& RemapData = AssetsToRemap[AssetIdx];

		const FString PackageName = RemapData.PackageName.ToString();

		// load package
		UPackage* Package = LoadPackage(NULL, *PackageName, LOAD_None);
		if ( !Package )
		{
			RemapData.ReportFailed( LOCTEXT("RemapSkeletonFailed_LoadPackage", "Could not load the package.").ToString());
			continue;
		}

		// get all the objects
		TArray<UObject*> Objects;
		GetObjectsWithOuter(Package, Objects);

		// see if we have skeletalmesh
		bool bSkeletalMeshPackage = false;
		for (auto Iter=Objects.CreateIterator(); Iter; ++Iter)
		{
			// we only care animation asset or animation blueprint
			if ((*Iter)->GetClass()->IsChildOf(UAnimationAsset::StaticClass()) ||
				(*Iter)->GetClass()->IsChildOf(UAnimBlueprint::StaticClass()))
			{
				// add to asset 
				RemapData.Asset = (*Iter);
				break;
			}
			else if ((*Iter)->GetClass()->IsChildOf(USkeletalMesh::StaticClass()) )
			{
				bSkeletalMeshPackage = true;
				break;
			}
		}

		// if we have skeletalmesh, we ignore this package, do not report as error
		if ( bSkeletalMeshPackage )
		{
			continue;
		}

		// if none was relevant - skeletal mesh is going to get here
		if ( !RemapData.Asset.IsValid() )
		{
			RemapData.ReportFailed( LOCTEXT("RemapSkeletonFailed_LoadObject", "Could not load any related object.").ToString());
			continue;
		}

		OutPackagesToSave.Add(Package);
	}

	GWarn->EndSlowTask();
}

bool FAssetTypeActions_Skeleton::CheckOutPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& InOutPackagesToSave) const
{
	bool bUserAcceptedCheckout = true;
	
	if ( InOutPackagesToSave.Num() > 0 )
	{
		if ( ISourceControlModule::Get().IsEnabled() )
		{
			TArray<UPackage*> PackagesCheckedOutOrMadeWritable;
			TArray<UPackage*> PackagesNotNeedingCheckout;
			bUserAcceptedCheckout = FEditorFileUtils::PromptToCheckoutPackages( false, InOutPackagesToSave, &PackagesCheckedOutOrMadeWritable, &PackagesNotNeedingCheckout );
			if ( bUserAcceptedCheckout )
			{
				TArray<UPackage*> PackagesThatCouldNotBeCheckedOut = InOutPackagesToSave;

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

					for ( auto RenameDataIt = AssetsToRemap.CreateIterator(); RenameDataIt; ++RenameDataIt )
					{
						FAssetToRemapSkeleton& RemapData = *RenameDataIt;

						if (RemapData.Asset.IsValid() && RemapData.Asset.Get()->GetOutermost() == *PackageIt)
						{
							RemapData.ReportFailed( LOCTEXT("RemapSkeletonFailed_CheckOutFailed", "Check out failed").ToString());
						}
					}

					InOutPackagesToSave.Remove(*PackageIt);
				}
			}
		}
	}

	return bUserAcceptedCheckout;
}

void FAssetTypeActions_Skeleton::DetectReadOnlyPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& InOutPackagesToSave) const
{
	// For each valid package...
	for ( int32 PackageIdx = InOutPackagesToSave.Num() - 1; PackageIdx >= 0; --PackageIdx )
	{
		UPackage* Package = InOutPackagesToSave[PackageIdx];

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

					for ( auto RenameDataIt = AssetsToRemap.CreateIterator(); RenameDataIt; ++RenameDataIt )
					{
						FAssetToRemapSkeleton& RenameData = *RenameDataIt;

						if (RenameData.Asset.IsValid() && RenameData.Asset.Get()->GetOutermost() == Package)
						{
							RenameData.ReportFailed( LOCTEXT("RemapSkeletonFailed_FileReadOnly", "File still read only").ToString());
						}
					}				

					// Remove the package from the save list
					InOutPackagesToSave.RemoveAt(PackageIdx);
				}
			}
		}
	}
}

void FAssetTypeActions_Skeleton::SavePackages(const TArray<UPackage*> PackagesToSave) const
{
	if ( PackagesToSave.Num() > 0 )
	{
		const bool bCheckDirty = false;
		const bool bPromptToSave = false;
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);

		ISourceControlModule::Get().QueueStatusUpdate(PackagesToSave);
	}
}

void FAssetTypeActions_Skeleton::ReportFailures(const TArray<FAssetToRemapSkeleton>& AssetsToRemap) const
{
	TArray<FString> FailedToRemap;
	for ( auto RenameIt = AssetsToRemap.CreateConstIterator(); RenameIt; ++RenameIt )
	{
		const FAssetToRemapSkeleton& RemapData = *RenameIt;
		if ( RemapData.bRemapFailed )
		{
			UObject* Asset = RemapData.Asset.Get();
			if ( Asset )
			{
				FailedToRemap.Add(Asset->GetOutermost()->GetName() + TEXT(" - ") + RemapData.FailureReason);
			}
			else
			{
				FailedToRemap.Add(LOCTEXT("RemapSkeletonFailed_InvalidAssetText", "Invalid Asset").ToString());
			}
		}
	}

	if ( FailedToRemap.Num() > 0 )
	{
		SRemapFailures::OpenRemapFailuresDialog(FailedToRemap);
	}
}

void FAssetTypeActions_Skeleton::RetargetSkeleton(TArray<FAssetToRemapSkeleton>& AssetsToRemap, USkeleton* OldSkeleton, USkeleton* NewSkeleton) const
{
	TArray<UAnimBlueprint*>	AnimBlueprints;

	// first we convert all individual assets
	for ( auto RenameIt = AssetsToRemap.CreateIterator(); RenameIt; ++RenameIt )
	{
		FAssetToRemapSkeleton& RenameData = *RenameIt;
		if ( RenameData.bRemapFailed == false  )
		{
			UObject* Asset = RenameData.Asset.Get();
			if ( Asset )
			{
				if ( UAnimationAsset * AnimAsset = Cast<UAnimationAsset>(Asset) )
				{
					AnimAsset->ReplaceSkeleton(NewSkeleton);
				}
				else if ( UAnimBlueprint * AnimBlueprint = Cast<UAnimBlueprint>(Asset) )
				{
					AnimBlueprints.Add(AnimBlueprint);
				}
			}
		}
	}

	// convert all Animation Blueprints and compile 
	for ( auto AnimBPIter = AnimBlueprints.CreateIterator(); AnimBPIter; ++AnimBPIter )
	{
		UAnimBlueprint * AnimBlueprint = (*AnimBPIter);
		AnimBlueprint->TargetSkeleton = NewSkeleton;
		
		bool bIsRegeneratingOnLoad = false;
		bool bSkipGarbageCollection = true;
		FBlueprintEditorUtils::RefreshAllNodes(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint, bIsRegeneratingOnLoad, bSkipGarbageCollection);
	}

	// now update any running instance
	for (FObjectIterator Iter(USkeletalMeshComponent::StaticClass()); Iter; ++Iter)
	{
		USkeletalMeshComponent * MeshComponent = Cast<USkeletalMeshComponent>(*Iter);
		if (MeshComponent->SkeletalMesh && MeshComponent->SkeletalMesh->Skeleton == OldSkeleton)
		{
			MeshComponent->InitAnim(true);
		}
	}
}
#undef LOCTEXT_NAMESPACE
