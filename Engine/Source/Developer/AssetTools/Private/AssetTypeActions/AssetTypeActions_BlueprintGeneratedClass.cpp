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

void FAssetTypeActions_BlueprintGeneratedClass::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto BlueprintGeneratedClasses = GetTypedWeakObjectPtrs<UBlueprintGeneratedClass>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Blueprint_Edit", "Open in Full Editor"),
		LOCTEXT("Blueprint_EditTooltip", "Opens the selected blueprints in the either the blueprint editor or in Persona (depending on type)."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_BlueprintGeneratedClass::ExecuteEdit, BlueprintGeneratedClasses ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Blueprint_EditDefaults", "Edit Defaults"),
		LOCTEXT("Blueprint_EditDefaultsTooltip", "Edits the default properties for the selected blueprints."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_BlueprintGeneratedClass::ExecuteEditDefaults, BlueprintGeneratedClasses ),
			FCanExecuteAction()
			)
		);

	if ( BlueprintGeneratedClasses.Num() == 1 )
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Blueprint_NewDerivedBlueprint", "Create Blueprint based on this"),
			LOCTEXT("Blueprint_NewDerivedBlueprintTooltip", "Creates a blueprint based on the selected blueprint."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &FAssetTypeActions_BlueprintGeneratedClass::ExecuteNewDerivedBlueprint, BlueprintGeneratedClasses[0] ),
				FCanExecuteAction()
				)
			);
	}
}

void FAssetTypeActions_BlueprintGeneratedClass::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto GeneratedClass = Cast<UBlueprintGeneratedClass>(*ObjIt);
		auto Blueprint = Cast<UBlueprint>(GeneratedClass->ClassGeneratedBy);
		if (Blueprint && Blueprint->SkeletonGeneratedClass && Blueprint->GeneratedClass )
		{
			FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>( "Kismet" );
			TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor( Mode, EditWithinLevelEditor, Blueprint, ShouldUseDataOnlyEditor(Blueprint) );
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("Blueprint_FailedToLoadDueToInvalidClass", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

void FAssetTypeActions_BlueprintGeneratedClass::ExecuteEdit(TArray<TWeakObjectPtr<UBlueprintGeneratedClass>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto GeneratedClass = (*ObjIt).Get();
		UBlueprint* Object = GeneratedClass ? Cast<UBlueprint>(GeneratedClass->ClassGeneratedBy) : NULL;
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

void FAssetTypeActions_BlueprintGeneratedClass::ExecuteEditDefaults(TArray<TWeakObjectPtr<UBlueprintGeneratedClass>> Objects)
{
	TArray< UBlueprint* > Blueprints;

	FMessageLog EditorErrors("EditorErrors");
	EditorErrors.NewPage(LOCTEXT("ExecuteEditDefaultsNewLogPage", "Loading Blueprints"));

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto GeneratedClass = (*ObjIt).Get();
		UBlueprint* Object = GeneratedClass ? Cast<UBlueprint>(GeneratedClass->ClassGeneratedBy) : NULL;
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

void FAssetTypeActions_BlueprintGeneratedClass::ExecuteNewDerivedBlueprint(TWeakObjectPtr<UBlueprintGeneratedClass> InObject)
{
	auto Object = InObject.Get();
	if ( Object )
	{
		// The menu option should ONLY be available if there is only one blueprint selected, validated by the menu creation code
		UClass* TargetClass = Object;

		if(!FKismetEditorUtilities::CanCreateBlueprintOfClass(TargetClass))
		{
			FMessageDialog::Open( EAppMsgType::Ok, FText::FromString(TEXT("Invalid class with which to make a Blueprint.")));
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

bool FAssetTypeActions_BlueprintGeneratedClass::ShouldUseDataOnlyEditor( const UBlueprint* Blueprint ) const
{
	return FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint) 
		&& !FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint) 
		&& !FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint)
		&& !Blueprint->bForceFullEditor
		&& !Blueprint->bIsNewlyCreated;
}

void FAssetTypeActions_BlueprintGeneratedClass::PerformAssetDiff(UObject* OldAsset, UObject* NewAsset, const FRevisionInfo& OldRevision, const FRevisionInfo& NewRevision) const
{
	UBlueprintGeneratedClass* OldClass = CastChecked<UBlueprintGeneratedClass>(OldAsset);
	UBlueprint* OldBlueprint = CastChecked<UBlueprint>(OldClass->ClassGeneratedBy);
	check(OldBlueprint->SkeletonGeneratedClass != NULL);

	UBlueprintGeneratedClass* NewClass = CastChecked<UBlueprintGeneratedClass>(NewAsset);
	UBlueprint* NewBlueprint = CastChecked<UBlueprint>(NewClass->ClassGeneratedBy);
	check(NewBlueprint->SkeletonGeneratedClass != NULL);

	const TSharedPtr<SWindow> Window = SNew(SWindow)
		.Title( FText::Format( LOCTEXT("Blueprint Diff", "{0} - Blueprint Diff"), FText::FromString( NewBlueprint->GetName() ) ) )
		.ClientSize(FVector2D(1000,800));

	Window->SetContent(SNew(SBlueprintDiff)
					  .BlueprintOld(OldBlueprint)
					  .BlueprintNew(NewBlueprint)
					  .OldRevision(OldRevision)
					  .NewRevision(NewRevision)
					  .OpenInDefaults(const_cast<FAssetTypeActions_BlueprintGeneratedClass*>(this), &FAssetTypeActions_BlueprintGeneratedClass::OpenInDefaults) );

	FSlateApplication::Get().AddWindow( Window.ToSharedRef(), true );
}

UThumbnailInfo* FAssetTypeActions_BlueprintGeneratedClass::GetThumbnailInfo(UObject* Asset) const
{
	// Blueprint thumbnail scenes are disabled for now
	UBlueprintGeneratedClass* GeneratedClass = CastChecked<UBlueprintGeneratedClass>(Asset);
	UBlueprint* Blueprint = CastChecked<UBlueprint>(GeneratedClass->ClassGeneratedBy);
	UThumbnailInfo* ThumbnailInfo = Blueprint->ThumbnailInfo;
	if ( ThumbnailInfo == NULL )
	{
		ThumbnailInfo = ConstructObject<USceneThumbnailInfo>(USceneThumbnailInfo::StaticClass(), Blueprint);
		Blueprint->ThumbnailInfo = ThumbnailInfo;
	}

	return ThumbnailInfo;
}

void FAssetTypeActions_BlueprintGeneratedClass::OpenInDefaults( class UBlueprint* OldBlueprint, class UBlueprint* NewBlueprint ) const
{
	FString OldTextFilename = DumpAssetToTempFile(OldBlueprint->GeneratedClass->GetDefaultObject());
	FString NewTextFilename = DumpAssetToTempFile(NewBlueprint->GeneratedClass->GetDefaultObject());
	FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

	// args are just 2 temp filenames
	FString DiffArgs = FString::Printf(TEXT("%s %s"), *OldTextFilename, *NewTextFilename);

	CreateDiffProcess(DiffCommand, DiffArgs);
}

#undef LOCTEXT_NAMESPACE
