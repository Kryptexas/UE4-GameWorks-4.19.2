// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/MainFrame/Public/MainFrame.h"
#include "LevelEditor.h"
#include "DesktopPlatformModule.h"

#include "StreamingLevelCustomization.h"
#include "StreamingLevelCollectionModel.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

FStreamingLevelCollectionModel::FStreamingLevelCollectionModel( const TWeakObjectPtr< UEditorEngine >& InEditor )
	: FLevelCollectionModel(InEditor)
	, AddedLevelStreamingClass(ULevelStreamingKismet::StaticClass())
{
}

FStreamingLevelCollectionModel::~FStreamingLevelCollectionModel()
{
	Editor->UnregisterForUndo( this );
}

void FStreamingLevelCollectionModel::Initialize()
{
	BindCommands();	
	Editor->RegisterForUndo( this );
	
	FLevelCollectionModel::Initialize();
}

void FStreamingLevelCollectionModel::OnLevelsCollectionChanged()
{
	InvalidSelectedLevels.Empty();
	
	// We have to have valid world
	if (!CurrentWorld.IsValid())
	{
		return;
	}

	// Add model for a persistent level
	TSharedPtr<FStreamingLevelModel> PersistentLevelModel = MakeShareable(new FStreamingLevelModel(Editor, *this, NULL));
	PersistentLevelModel->SetLevelExpansionFlag(true);
	RootLevelsList.Add(PersistentLevelModel);
	AllLevelsList.Add(PersistentLevelModel);

	// Add models for each streaming level in the world
	for (auto It = CurrentWorld->StreamingLevels.CreateConstIterator(); It; ++It)
	{
		ULevelStreaming* StreamingLevel = (*It);
		if (StreamingLevel != NULL)
		{
			TSharedPtr<FStreamingLevelModel> LevelModel = MakeShareable(new FStreamingLevelModel(Editor, *this, StreamingLevel));
			AllLevelsList.Add(LevelModel);

			PersistentLevelModel->AddChild(LevelModel);
			LevelModel->SetParent(PersistentLevelModel);
		}
	}

	RefreshSortIndexes();
	FLevelCollectionModel::OnLevelsCollectionChanged();
}

void FStreamingLevelCollectionModel::UnloadLevels(const FLevelModelList& InLevelList)
{
	if (IsReadOnly())
	{
		return;
	}

	// Persistent level cannot be unloaded
	if (InLevelList.Num() == 1 && InLevelList[0]->IsPersistent())
	{
		return;
	}
		
	bool bHaveDirtyLevels = false;
	for (auto It = InLevelList.CreateConstIterator(); It; ++It)
	{
		if ((*It)->IsDirty() && !(*It)->IsLocked() && !(*It)->IsPersistent())
		{
			// this level is dirty and can be removed from the world
			bHaveDirtyLevels = true;
			break;
		}
	}

	// Depending on the state of the level, create a warning message
	FText LevelWarning = LOCTEXT("RemoveLevel_Undo", "Removing levels cannot be undone.  Proceed?");
	if (bHaveDirtyLevels)
	{
		LevelWarning = LOCTEXT("RemoveLevel_Dirty", "Removing levels cannot be undone.  Any changes to these levels will be lost.  Proceed?");
	}

	// Ask the user if they really wish to remove the level(s)
	FSuppressableWarningDialog::FSetupInfo Info( LevelWarning, LOCTEXT("RemoveLevel_Message", "Remove Level"), "RemoveLevelWarning" );
	Info.ConfirmText = LOCTEXT( "RemoveLevel_Yes", "Yes");
	Info.CancelText = LOCTEXT( "RemoveLevel_No", "No");	
	FSuppressableWarningDialog RemoveLevelWarning( Info );
	if (RemoveLevelWarning.ShowModal() == FSuppressableWarningDialog::Cancel)
	{
		return;
	}
			
	// This will remove streaming levels from a persistent world, so we need to re-populate levels list
	FLevelCollectionModel::UnloadLevels(InLevelList);
	PopulateLevelsList();
}

void FStreamingLevelCollectionModel::BindCommands()
{
	FLevelCollectionModel::BindCommands();
	
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
	FUICommandList& ActionList = *CommandList;

	//invalid selected levels
	ActionList.MapAction( Commands.FixUpInvalidReference,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::FixupInvalidReference_Executed ) );
	
	ActionList.MapAction( Commands.RemoveInvalidReference,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::RemoveInvalidSelectedLevels_Executed ));

	//levels
	ActionList.MapAction( Commands.CreateEmptyLevel,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::CreateEmptyLevel_Executed  ) );
	
	ActionList.MapAction( Commands.AddExistingLevel,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::AddExistingLevel_Executed ) );

	ActionList.MapAction( Commands.AddSelectedActorsToNewLevel,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::AddSelectedActorsToNewLevel_Executed  ),
		FCanExecuteAction::CreateSP( this, &FLevelCollectionModel::AreActorsSelected ) );
	
	ActionList.MapAction( Commands.RemoveSelectedLevels,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::UnloadSelectedLevels_Executed  ),
		FCanExecuteAction::CreateSP( this, &FLevelCollectionModel::AreAllSelectedLevelsEditable ) );
	
	ActionList.MapAction( Commands.MergeSelectedLevels,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::MergeSelectedLevels_Executed  ),
		FCanExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::AreAllSelectedLevelsEditable ) );
	
	// new level streaming method
	ActionList.MapAction( Commands.SetAddStreamingMethod_Blueprint,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::SetAddedLevelStreamingClass_Executed, ULevelStreamingKismet::StaticClass()  ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &FStreamingLevelCollectionModel::IsNewStreamingMethodChecked, ULevelStreamingKismet::StaticClass()));

	ActionList.MapAction( Commands.SetAddStreamingMethod_AlwaysLoaded,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::SetAddedLevelStreamingClass_Executed, ULevelStreamingAlwaysLoaded::StaticClass()  ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &FStreamingLevelCollectionModel::IsNewStreamingMethodChecked, ULevelStreamingAlwaysLoaded::StaticClass()));

	// change streaming method
	ActionList.MapAction( Commands.SetStreamingMethod_Blueprint,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::SetStreamingLevelsClass_Executed, ULevelStreamingKismet::StaticClass()  ),
		FCanExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::AreAllSelectedLevelsEditable ),
		FIsActionChecked::CreateSP( this, &FStreamingLevelCollectionModel::IsStreamingMethodChecked, ULevelStreamingKismet::StaticClass()));

	ActionList.MapAction( Commands.SetStreamingMethod_AlwaysLoaded,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::SetStreamingLevelsClass_Executed, ULevelStreamingAlwaysLoaded::StaticClass()  ),
		FCanExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::AreAllSelectedLevelsEditable ),
		FIsActionChecked::CreateSP( this, &FStreamingLevelCollectionModel::IsStreamingMethodChecked, ULevelStreamingAlwaysLoaded::StaticClass()));

	//streaming volume
	ActionList.MapAction( Commands.SelectStreamingVolumes,
		FExecuteAction::CreateSP( this, &FStreamingLevelCollectionModel::SelectStreamingVolumes_Executed  ),
		FCanExecuteAction::CreateSP( this, &FLevelCollectionModel::AreAllSelectedLevelsEditable));

}

TSharedPtr<FLevelDragDropOp> FStreamingLevelCollectionModel::CreateDragDropOp() const
{
	TArray<TWeakObjectPtr<ULevelStreaming>> LevelsToDrag;

	for (auto It = SelectedLevelsList.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<FStreamingLevelModel> TargetModel = StaticCastSharedPtr<FStreamingLevelModel>(*It);
		
		if (TargetModel->GetLevelStreaming().IsValid())
		{
			LevelsToDrag.Add(TargetModel->GetLevelStreaming());
		}
	}

	if (LevelsToDrag.Num())
	{
		return FLevelDragDropOp::New(LevelsToDrag);
	}
	else
	{
		return FLevelCollectionModel::CreateDragDropOp();
	}
}

void FStreamingLevelCollectionModel::BuildHierarchyMenu(FMenuBuilder& InMenuBuilder) const
{
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();

	const FLevelModelList& SelectedLevels = GetSelectedLevels();
			
	bool bOnlyInvalidSelectedLevel	= InvalidSelectedLevels.Num() == 1 && SelectedLevels.Num() == 0;
	bool bOnlySelectedLevel			= InvalidSelectedLevels.Num() == 0 && SelectedLevels.Num() == 1;
			
	if (bOnlyInvalidSelectedLevel)
	{
		// We only show the level missing level commands if only 1 invalid level and no valid levels
		InMenuBuilder.BeginSection("MissingLevel", LOCTEXT("ViewHeaderRemove", "Missing Level") );
		{
			InMenuBuilder.AddMenuEntry( Commands.FixUpInvalidReference );
			InMenuBuilder.AddMenuEntry( Commands.RemoveInvalidReference );
		}
		InMenuBuilder.EndSection();
	}
	else if ( bOnlySelectedLevel )
	{
		const auto& LevelModel = SelectedLevels[0];

		InMenuBuilder.BeginSection("Level", FText::FromName(LevelModel->GetLongPackageName()));
		{
			InMenuBuilder.AddMenuEntry( Commands.MakeLevelCurrent );
			InMenuBuilder.AddMenuEntry( Commands.MoveActorsToSelected );
		}
		InMenuBuilder.EndSection();
	}

	// Adds common commands
	FLevelCollectionModel::BuildHierarchyMenu(InMenuBuilder);

	InMenuBuilder.BeginSection("LevelsAddChangeStreaming");
	{
		if (AreAnyLevelsSelected())
		{
			InMenuBuilder.AddMenuEntry(Commands.RemoveSelectedLevels);
			
			//
			InMenuBuilder.AddSubMenu( 
				LOCTEXT("LevelsChangeStreamingMethod", "Change Streaming Method"),
				LOCTEXT("LevelsChangeStreamingMethod_Tooltip", "Changes the streaming method for the selected levels"),
				FNewMenuDelegate::CreateRaw(this, &FStreamingLevelCollectionModel::FillSetStreamingMethodMenu ) );
		}
	}
	InMenuBuilder.EndSection();
}

void FStreamingLevelCollectionModel::FillSetStreamingMethodMenu(FMenuBuilder& InMenuBuilder)
{
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
	InMenuBuilder.AddMenuEntry( Commands.SetStreamingMethod_Blueprint, NAME_None, LOCTEXT("SetStreamingMethodBlueprintOverride", "Blueprint") );
	InMenuBuilder.AddMenuEntry( Commands.SetStreamingMethod_AlwaysLoaded, NAME_None, LOCTEXT("SetStreamingMethodAlwaysLoadedOverride", "Always Loaded") );
}

void FStreamingLevelCollectionModel::CustomizeFileMainMenu(FMenuBuilder& InMenuBuilder) const
{
	FLevelCollectionModel::CustomizeFileMainMenu(InMenuBuilder);

	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
		
	InMenuBuilder.BeginSection("LevelsAddLevel");
	{
		InMenuBuilder.AddSubMenu( 
			LOCTEXT("LevelsStreamingMethod", "Default Streaming Method"),
			LOCTEXT("LevelsStreamingMethod_Tooltip", "Changes the default streaming method for a new levels"),
			FNewMenuDelegate::CreateRaw(this, &FStreamingLevelCollectionModel::FillDefaultStreamingMethodMenu ) );
		
		InMenuBuilder.AddMenuEntry( Commands.CreateEmptyLevel );
		InMenuBuilder.AddMenuEntry( Commands.AddExistingLevel );
		InMenuBuilder.AddMenuEntry( Commands.AddSelectedActorsToNewLevel );
		InMenuBuilder.AddMenuEntry( Commands.MergeSelectedLevels );
	}
	InMenuBuilder.EndSection();
}

void FStreamingLevelCollectionModel::FillDefaultStreamingMethodMenu(FMenuBuilder& InMenuBuilder)
{
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
	InMenuBuilder.AddMenuEntry( Commands.SetAddStreamingMethod_Blueprint, NAME_None, LOCTEXT("SetAddStreamingMethodBlueprintOverride", "Blueprint") );
	InMenuBuilder.AddMenuEntry( Commands.SetAddStreamingMethod_AlwaysLoaded, NAME_None, LOCTEXT("SetAddStreamingMethodAlwaysLoadedOverride", "Always Loaded") );
}

void FStreamingLevelCollectionModel::RegisterDetailsCustomization(FPropertyEditorModule& InPropertyModule, 
																	TSharedPtr<IDetailsView> InDetailsView)
{
	TSharedRef<FStreamingLevelCollectionModel> WorldModel = StaticCastSharedRef<FStreamingLevelCollectionModel>(this->AsShared());

	InDetailsView->RegisterInstancedCustomPropertyLayout(ULevelStreaming::StaticClass(), 
		FOnGetDetailCustomizationInstance::CreateStatic(&FStreamingLevelCustomization::MakeInstance,  WorldModel)
		);
}

void FStreamingLevelCollectionModel::UnregisterDetailsCustomization(FPropertyEditorModule& InPropertyModule,
																	TSharedPtr<IDetailsView> InDetailsView)
{
	InDetailsView->UnregisterInstancedCustomPropertyLayout(ULevelStreaming::StaticClass());
}

bool FStreamingLevelCollectionModel::CanShiftSelection()
{
	if (!IsOneLevelSelected())
	{
		return false;
	}

	for (int32 i = 0; i < SelectedLevelsList.Num(); ++i)
	{
		if (SelectedLevelsList[i]->IsLocked() || SelectedLevelsList[i]->IsPersistent())
		{
			return false;
		}
	}

	return true;
}

void FStreamingLevelCollectionModel::ShiftSelection( bool bUp )
{
	if (!CanShiftSelection())
	{
		return;
	}

	if (!IsSelectedLevelEditable())
	{
		FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("ShiftLevelLocked", "Shift Level: The requested operation could not be completed because the level is locked or not loaded.") );
		return;
	}
	
	TSharedPtr<FStreamingLevelModel> TargetModel = StaticCastSharedPtr<FStreamingLevelModel>(SelectedLevelsList[0]);
	ULevelStreaming* InLevelStreaming = TargetModel->GetLevelStreaming().Get();
	
	int32 FoundLevelIndex =  CurrentWorld->StreamingLevels.Find(InLevelStreaming);
	int32 PrevFoundLevelIndex = FoundLevelIndex - 1;
	int32 PostFoundLevelIndex = FoundLevelIndex + 1;

	// If we found the level . . .
	if ( FoundLevelIndex != INDEX_NONE )
	{
		// Check if we found a destination index to swap it to.
		const int32 DestIndex = bUp ? PrevFoundLevelIndex : PostFoundLevelIndex;
		if (CurrentWorld->StreamingLevels.IsValidIndex(DestIndex))
		{
			// Swap the level into position.
			const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "ShiftLevelInLevelBrowser", "Shift level in Level Browser") );
			CurrentWorld->Modify();
			CurrentWorld->StreamingLevels.Swap( FoundLevelIndex, DestIndex );
			CurrentWorld->MarkPackageDirty();
		}
	}

	RefreshSortIndexes();
}


const FLevelModelList& FStreamingLevelCollectionModel::GetInvalidSelectedLevels() const 
{ 
	return InvalidSelectedLevels;
}

void FStreamingLevelCollectionModel::RefreshSortIndexes()
{
	//for( auto LevelIt = AllLevelsList.CreateIterator(); LevelIt; ++LevelIt )
	//{
	//	(*LevelIt)->RefreshStreamingLevelIndex();
	//}

	//OnFilterChanged();
}

void FStreamingLevelCollectionModel::SortFilteredLevels()
{
	struct FCompareLevels
	{
		FORCEINLINE bool operator()(const TSharedPtr<FLevelModel>& InLhs, 
									const TSharedPtr<FLevelModel>& InRhs) const 
		{ 
			TSharedPtr<FStreamingLevelModel> Lhs = StaticCastSharedPtr<FStreamingLevelModel>(InLhs);
			TSharedPtr<FStreamingLevelModel> Rhs = StaticCastSharedPtr<FStreamingLevelModel>(InRhs);
				
			// Sort by a user-defined order.
			return (Lhs->GetStreamingLevelIndex() < Rhs->GetStreamingLevelIndex());
		}
	};

	FilteredLevelsList.Sort(FCompareLevels());
}


//levels
void FStreamingLevelCollectionModel::CreateEmptyLevel_Executed()
{
	EditorLevelUtils::CreateNewLevel( CurrentWorld.Get(), false, AddedLevelStreamingClass );

	// Force a cached level list rebuild
	PopulateLevelsList();
}

void FStreamingLevelCollectionModel::AddExistingLevel_Executed()
{
	AddExistingLevel();
}

bool FStreamingLevelCollectionModel::AddExistingLevel()
{
	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
		NSLOCTEXT("UnrealEd", "Open", "Open").ToString(),
		*FEditorDirectories::Get().GetLastDirectory(ELastDirectory::UNR),
		TEXT(""),
		*FEditorFileUtils::GetFilterString(FI_Load),
			EFileDialogFlags::Multiple,
			OpenFilenames
			);
	}

	if( bOpened )
	{
		// Save the path as default for next time
		FEditorDirectories::Get().SetLastDirectory( ELastDirectory::UNR, FPaths::GetPath( OpenFilenames[ 0 ] ) );

		TArray<FString> Filenames;
		for( int32 FileIndex = 0 ; FileIndex < OpenFilenames.Num() ; ++FileIndex )
		{
			// Strip paths from to get the level package names.
			const FString FilePath( OpenFilenames[FileIndex] );

			// make sure the level is in our package cache, because the async loading code will use this to find it
			if (!FPaths::FileExists(FilePath))
			{
				FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_LevelImportFromExternal", "Importing external sublevels is not allowed. Move the level files into the standard content directory and try again.\nAfter moving the level(s), restart the editor.") );
				return false;				
			}

			FText ErrorMessage;
			bool bFilenameIsValid = FEditorFileUtils::IsValidMapFilename(OpenFilenames[FileIndex], ErrorMessage);
			if ( !bFilenameIsValid )
			{
				// Start the loop over, prompting for save again
				const FText DisplayFilename = FText::FromString( IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*OpenFilenames[FileIndex]) );
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("Filename"), DisplayFilename);
				Arguments.Add(TEXT("LineTerminators"), FText::FromString(LINE_TERMINATOR LINE_TERMINATOR));
				Arguments.Add(TEXT("ErrorMessage"), ErrorMessage);
				const FText DisplayMessage = FText::Format( NSLOCTEXT("UnrealEd", "Error_InvalidLevelToAdd", "Unable to add streaming level {Filename}{LineTerminators}{ErrorMessage}"), Arguments );
				FMessageDialog::Open( EAppMsgType::Ok, DisplayMessage );
				return false;
			}

			Filenames.Add( FilePath );
		}

		// Sort the level packages alphabetically by name.
		Filenames.Sort();

		// Fire ULevel::LevelDirtiedEvent when falling out of scope.
		FScopedLevelDirtied LevelDirtyCallback;

		// Try to add the levels that were specified in the dialog.
		ULevel* NewLevel = nullptr;
		for( int32 FileIndex = 0 ; FileIndex < Filenames.Num() ; ++FileIndex )
		{
			const FString& PackageName = FPackageName::FilenameToLongPackageName(Filenames[FileIndex]);

			NewLevel = EditorLevelUtils::AddLevelToWorld(CurrentWorld.Get(), *PackageName, AddedLevelStreamingClass);
			if (NewLevel)
			{
				LevelDirtyCallback.Request();
			}

		} // for each file

		// Force a cached level list rebuild
		PopulateLevelsList();

		// Set the last loaded level to be the current level
		if (NewLevel)
		{
			CurrentWorld->SetCurrentLevel(NewLevel);
		}

		// For safety
		if( GEditorModeTools().IsModeActive( FBuiltinEditorModes::EM_Landscape ) )
		{
			GEditorModeTools().ActivateMode( FBuiltinEditorModes::EM_Default );
		}

		// refresh editor windows
		FEditorDelegates::RefreshAllBrowsers.Broadcast();
	}

	// Update volume actor visibility for each viewport since we loaded a level which could potentially contain volumes
	GUnrealEd->UpdateVolumeActorVisibility(NULL);

	// return whether a level was selected
	return bOpened;
}

void FStreamingLevelCollectionModel::AddSelectedActorsToNewLevel_Executed()
{
	EditorLevelUtils::CreateNewLevel( CurrentWorld.Get(), true, AddedLevelStreamingClass );

	// Force a cached level list rebuild
	PopulateLevelsList();
}

void FStreamingLevelCollectionModel::FixupInvalidReference_Executed()
{
	// Browsing is essentially the same as adding an existing level
	if (AddExistingLevel())
	{
		RemoveInvalidSelectedLevels_Executed();
	}
}

void FStreamingLevelCollectionModel::RemoveInvalidSelectedLevels_Executed()
{
	for (auto It = InvalidSelectedLevels.CreateIterator(); It; ++It )
	{
		TSharedPtr<FStreamingLevelModel> TargetModel = StaticCastSharedPtr<FStreamingLevelModel>(*It);
		ULevelStreaming* LevelStreaming = TargetModel->GetLevelStreaming().Get();

		if (LevelStreaming)
		{
			EditorLevelUtils::RemoveInvalidLevelFromWorld(LevelStreaming);
		}
	}

	// Force a cached level list rebuild
	PopulateLevelsList();
}

void FStreamingLevelCollectionModel::MergeSelectedLevels_Executed()
{
	// Stash off a copy of the original array, so the selection can be restored
	FLevelModelList SelectedLevelsCopy = SelectedLevelsList;

	//make sure the selected levels are made visible (and thus fully loaded) before merging
	ShowSelectedLevels_Executed();

	//restore the original selection and select all actors in the selected levels
	SetSelectedLevels(SelectedLevelsCopy);
	SelectActors_Executed();

	//Create a new level with the selected actors
	ULevel* NewLevel = EditorLevelUtils::CreateNewLevel(CurrentWorld.Get(),  true, AddedLevelStreamingClass);

	//If the new level was successfully created (ie the user did not cancel)
	if ((NewLevel != NULL) && (CurrentWorld.IsValid()))
	{
		if (CurrentWorld->SetCurrentLevel(NewLevel))
		{
			FEditorDelegates::NewCurrentLevel.Broadcast();
		}

		Editor->NoteSelectionChange();

		//restore the original selection and remove the levels that were merged
		SetSelectedLevels(SelectedLevelsCopy);
		UnloadSelectedLevels_Executed();
	}

	// Force a cached level list rebuild
	PopulateLevelsList();
}

void FStreamingLevelCollectionModel::SetAddedLevelStreamingClass_Executed(UClass* InClass)
{
	AddedLevelStreamingClass = InClass;
}

bool FStreamingLevelCollectionModel::IsNewStreamingMethodChecked(UClass* InClass) const
{
	return AddedLevelStreamingClass == InClass;
}

bool FStreamingLevelCollectionModel::IsStreamingMethodChecked(UClass* InClass) const
{
	for (auto It = SelectedLevelsList.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<FStreamingLevelModel> TargetModel = StaticCastSharedPtr<FStreamingLevelModel>(*It);
		ULevelStreaming* LevelStreaming = TargetModel->GetLevelStreaming().Get();
		
		if (LevelStreaming && LevelStreaming->GetClass() == InClass)
		{
			return true;
		}
	}
	return false;
}

void FStreamingLevelCollectionModel::SetStreamingLevelsClass_Executed(UClass* InClass)
{
	// First prompt to save the selected levels, as changing the streaming method will unload/reload them
	SaveSelectedLevels_Executed();

	// Stash off a copy of the original array, as changing the streaming method can destroy the selection
	FLevelModelList SelectedLevelsCopy = GetSelectedLevels();

	// Apply the new streaming method to the selected levels
	for (auto It = SelectedLevelsCopy.CreateIterator(); It; ++It)
	{
		TSharedPtr<FStreamingLevelModel> TargetModel = StaticCastSharedPtr<FStreamingLevelModel>(*It);
		TargetModel->SetStreamingClass(InClass);
	}

	SetSelectedLevels(SelectedLevelsCopy);

	// Force a cached level list rebuild
	PopulateLevelsList();
}

//streaming volumes
void FStreamingLevelCollectionModel::SelectStreamingVolumes_Executed()
{
	// Iterate over selected levels and make a list of volumes to select.
	TArray<ALevelStreamingVolume*> LevelStreamingVolumesToSelect;
	for (auto It = SelectedLevelsList.CreateIterator(); It; ++It)
	{
		TSharedPtr<FStreamingLevelModel> TargetModel = StaticCastSharedPtr<FStreamingLevelModel>(*It);
		ULevelStreaming* StreamingLevel = TargetModel->GetLevelStreaming().Get();

		if (StreamingLevel)
		{
			for (int32 i = 0; i < StreamingLevel->EditorStreamingVolumes.Num(); ++i)
			{
				ALevelStreamingVolume* LevelStreamingVolume = StreamingLevel->EditorStreamingVolumes[i];
				if (LevelStreamingVolume)
				{
					LevelStreamingVolumesToSelect.Add(LevelStreamingVolume);
				}
			}
		}
	}

	// Select the volumes.
	const FScopedTransaction Transaction( LOCTEXT("SelectAssociatedStreamingVolumes", "Select Associated Streaming Volumes") );
	Editor->GetSelectedActors()->Modify();
	Editor->SelectNone(false, true);

	for (int32 i = 0 ; i < LevelStreamingVolumesToSelect.Num() ; ++i)
	{
		ALevelStreamingVolume* LevelStreamingVolume = LevelStreamingVolumesToSelect[i];
		Editor->SelectActor(LevelStreamingVolume, /*bInSelected=*/ true, false, true);
	}
	
	Editor->NoteSelectionChange();
}

#undef LOCTEXT_NAMESPACE
