// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "PackageTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "BlueprintEditorModule.h"
#include "AssetRegistryModule.h"
#include "SBlueprintDiff.h"
#include "ISourceControlModule.h"
#include "MessageLog.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Blueprint::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Blueprints = GetTypedWeakObjectPtrs<UBlueprint>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Blueprint_EditAction", "Open in Full Editor"),
		LOCTEXT("Blueprint_EditActionTooltip", "Opens the selected blueprints in the either the blueprint editor or in Persona (depending on type)."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Blueprint::ExecuteEdit, Blueprints ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Blueprint_EditDefaults", "Edit Defaults"),
		LOCTEXT("Blueprint_EditDefaultsTooltip", "Edits the default properties for the selected blueprints."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Blueprint::ExecuteEditDefaults, Blueprints ),
			FCanExecuteAction()
			)
		);

	if ( Blueprints.Num() == 1 && CanCreateNewDerivedBlueprint() )
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Blueprint_NewDerivedBlueprint", "Create Blueprint based on this"),
			LOCTEXT("Blueprint_NewDerivedBlueprintTooltip", "Creates a blueprint based on the selected blueprint."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetTypeActions_Blueprint::ExecuteNewDerivedBlueprint, Blueprints[0] ),
				FCanExecuteAction()
				)
			);
	}
}

void FAssetTypeActions_Blueprint::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Blueprint = Cast<UBlueprint>(*ObjIt);
		if (Blueprint && Blueprint->SkeletonGeneratedClass && Blueprint->GeneratedClass )
		{
			FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>( "Kismet" );
			TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor( Mode, EditWithinLevelEditor, Blueprint, ShouldUseDataOnlyEditor(Blueprint) );
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("FailedToLoadBlueprint", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

void FAssetTypeActions_Blueprint::ExecuteEdit(TArray<TWeakObjectPtr<UBlueprint>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			// Force full (non data-only) editor if possible.
			const bool bSavedForceFullEditor = Object->bForceFullEditor;
			Object->bForceFullEditor = true;

			FAssetEditorManager::Get().OpenEditorForAsset(Object);

			Object->bForceFullEditor = bSavedForceFullEditor;
		}
	}
}

bool FAssetTypeActions_Blueprint::CanCreateNewDerivedBlueprint() const
{
	return true;
}

void FAssetTypeActions_Blueprint::ExecuteEditDefaults(TArray<TWeakObjectPtr<UBlueprint>> Objects)
{
	TArray< UBlueprint* > Blueprints;

	FMessageLog EditorErrors("EditorErrors");
	EditorErrors.NewPage(LOCTEXT("ExecuteEditDefaultsNewLogPage", "Loading Blueprints"));

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			// If the blueprint is valid, allow it to be added to the list, otherwise log the error.
			if (Object && Object->SkeletonGeneratedClass && Object->GeneratedClass )
			{
				Blueprints.Add(Object);
			}
			else
			{
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("ObjectName"), FText::FromString(Object->GetName()));
				EditorErrors.Error(FText::Format(LOCTEXT("LoadBlueprint_FailedLog", "{ObjectName} could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"), Arguments ) );
			}
		}
	}

	if ( Blueprints.Num() > 0 )
	{
		FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>( "Kismet" );
		TSharedRef< IBlueprintEditor > NewBlueprintEditor = BlueprintEditorModule.CreateBlueprintEditor(  EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), Blueprints );
	}

	// Report errors
	EditorErrors.Notify(LOCTEXT("OpenDefaults_Failed", "Opening Blueprint Defaults Failed!"));
}

void FAssetTypeActions_Blueprint::ExecuteNewDerivedBlueprint(TWeakObjectPtr<UBlueprint> InObject)
{
	auto Object = InObject.Get();
	if ( Object )
	{
		// The menu option should ONLY be available if there is only one blueprint selected, validated by the menu creation code
		UBlueprint* TargetBP = Object;
		UClass* TargetClass = TargetBP->GeneratedClass;

		if(!FKismetEditorUtilities::CanCreateBlueprintOfClass(TargetClass))
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("InvalidClassToMakeBlueprintFrom", "Invalid class with which to make a Blueprint."));
			return;
		}

		FString Name;
		FString PackageName;
		CreateUniqueAssetName(Object->GetOutermost()->GetName(), TEXT("_Child"), PackageName, Name);

		UPackage* Package = CreatePackage(NULL, *PackageName);
		if ( ensure(Package) )
		{
			// Create and init a new Blueprint
			UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(TargetClass, Package, FName(*Name), BPTYPE_Normal, UBlueprint::StaticClass());
			if(NewBP)
			{
				FAssetEditorManager::Get().OpenEditorForAsset(NewBP);

				// Notify the asset registry
				FAssetRegistryModule::AssetCreated(NewBP);

				// Mark the package dirty...
				Package->MarkPackageDirty();
			}
		}
	}
}

bool FAssetTypeActions_Blueprint::ShouldUseDataOnlyEditor( const UBlueprint* Blueprint ) const
{
	return FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint) 
		&& !FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint) 
		&& !FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint)
		&& !Blueprint->bForceFullEditor
		&& !Blueprint->bIsNewlyCreated;
}

void FAssetTypeActions_Blueprint::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const
{
	UBlueprint* OldBlueprint = Cast<UBlueprint>(OldAsset);
	check(OldBlueprint != NULL);
	check(OldBlueprint->SkeletonGeneratedClass != NULL);

	UBlueprint* NewBlueprint = Cast<UBlueprint>(NewAsset);
	check(NewBlueprint != NULL);
	check(NewBlueprint->SkeletonGeneratedClass != NULL);

	// sometimes we're comparing different revisions of one single asset (other 
	// times we're comparing two completely separate assets altogether)
	bool bIsSingleAsset = (NewBlueprint->GetName() == OldBlueprint->GetName());

	FText WindowTitle = LOCTEXT("NamelessBlueprintDiff", "Blueprint Diff");
	// if we're diff'ing one asset against itself 
	if (bIsSingleAsset)
	{
		// identify the assumed single asset in the window's title
		WindowTitle = FText::Format(LOCTEXT("Blueprint Diff", "{0} - Blueprint Diff"), FText::FromString(NewBlueprint->GetName()));
	}

	const TSharedPtr<SWindow> Window = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(FVector2D(1000,800));

	Window->SetContent(SNew(SBlueprintDiff)
					  .BlueprintOld(OldBlueprint)
					  .BlueprintNew(NewBlueprint)
					  .OldRevision(OldRevision)
					  .NewRevision(NewRevision)
					  .ShowAssetNames(!bIsSingleAsset)
					  .OpenInDefaults(const_cast<FAssetTypeActions_Blueprint*>(this), &FAssetTypeActions_Blueprint::OpenInDefaults) );

	// Make this window a child of the modal window if we've been spawned while one is active.
	TSharedPtr<SWindow> ActiveModal = FSlateApplication::Get().GetActiveModalWindow();
	if ( ActiveModal.IsValid() )
	{
		FSlateApplication::Get().AddWindowAsNativeChild( Window.ToSharedRef(), ActiveModal.ToSharedRef() );
	}
	else
	{
		FSlateApplication::Get().AddWindow( Window.ToSharedRef() );
	}
}

UThumbnailInfo* FAssetTypeActions_Blueprint::GetThumbnailInfo(UObject* Asset) const
{
	// Blueprint thumbnail scenes are disabled for now
	UBlueprint* Blueprint = CastChecked<UBlueprint>(Asset);
	UThumbnailInfo* ThumbnailInfo = Blueprint->ThumbnailInfo;
	if ( ThumbnailInfo == NULL )
	{
		ThumbnailInfo = ConstructObject<USceneThumbnailInfo>(USceneThumbnailInfo::StaticClass(), Blueprint);
		Blueprint->ThumbnailInfo = ThumbnailInfo;
	}

	return ThumbnailInfo;
}

void FAssetTypeActions_Blueprint::OpenInDefaults( class UBlueprint* OldBlueprint, class UBlueprint* NewBlueprint ) const
{
	FString OldTextFilename = DumpAssetToTempFile(OldBlueprint->GeneratedClass->GetDefaultObject());
	FString NewTextFilename = DumpAssetToTempFile(NewBlueprint->GeneratedClass->GetDefaultObject());
	FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

	// args are just 2 temp filenames
	FString DiffArgs = FString::Printf(TEXT("%s %s"), *OldTextFilename, *NewTextFilename);

	CreateDiffProcess(DiffCommand, DiffArgs);
}

FText FAssetTypeActions_Blueprint::GetAssetDescription(const FAssetData& AssetData) const
{
	if (const FString* pDescription = AssetData.TagsAndValues.Find(GET_MEMBER_NAME_CHECKED(UBlueprint, BlueprintDescription)))
	{
		if (!pDescription->IsEmpty())
		{
			const FString DescriptionStr(*pDescription);
			return FText::FromString(DescriptionStr.Replace(TEXT("\\n"), TEXT("\n")));
		}
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
