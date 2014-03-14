// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "ScopedTransaction.h"
#include "MainFrame.h"
#include "Dialogs/SMeshProxyDialog.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "MeshUtilities.h"
#include "AssetRegistryModule.h"
#include "ContentBrowserModule.h"


void UUnrealEdEngine::edactRegroupFromSelected()
{
	if(GEditor->bGroupingActive)
	{
		TArray<AActor*> ActorsToAdd;

		ULevel* ActorLevel = NULL;

		bool bActorsInSameLevel = true;
		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ); It; ++It )
		{
			AActor* Actor = CastChecked<AActor>(*It);
			if( !ActorLevel )
			{
				ActorLevel = Actor->GetLevel();
			}
			else if( ActorLevel != Actor->GetLevel() )
			{
				bActorsInSameLevel = false;
				break;
			}

			if ( Actor->IsA(AActor::StaticClass()) && !Actor->IsA(AGroupActor::StaticClass()) )
			{
				// Add each selected actor to our new group
				// Adding an actor will remove it from any existing groups.
				ActorsToAdd.Add( Actor );

			}
		}

		if( bActorsInSameLevel )
		{
			if( ActorsToAdd.Num() > 1 )
			{
				check(ActorLevel);
				// Store off the current level and make the level that contain the actors to group as the current level
				UWorld* World = ActorLevel->OwningWorld;
				check( World );
				{
					const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "Group_Regroup", "Regroup Ctrl+G") );

					FActorSpawnParameters SpawnInfo;
					SpawnInfo.OverrideLevel = ActorLevel;
					AGroupActor* SpawnedGroupActor = World->SpawnActor<AGroupActor>( SpawnInfo );

					for( int32 ActorIndex = 0; ActorIndex < ActorsToAdd.Num(); ++ActorIndex )
					{
						SpawnedGroupActor->Add( *ActorsToAdd[ActorIndex] );
					}

					SpawnedGroupActor->CenterGroupLocation();
					SpawnedGroupActor->Lock();
				}
			}
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Group_CantCreateGroupMultipleLevels", "Can't group the selected actors because they are in different levels.") );
		}
	}
}


void UUnrealEdEngine::edactUngroupFromSelected()
{
	if(GEditor->bGroupingActive)
	{
		TArray<AGroupActor*> OutermostGroupActors;
		
		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = CastChecked<AActor>( *It );

			// Get the outermost locked group
			AGroupActor* OutermostGroup = AGroupActor::GetRootForActor( Actor, true );
			if( OutermostGroup == NULL )
			{
				// Failed to find locked root group, try to find the immediate parent
				OutermostGroup = AGroupActor::GetParentForActor( Actor );
			}
			
			if( OutermostGroup )
			{
				OutermostGroupActors.AddUnique( OutermostGroup );
			}
		}

		if( OutermostGroupActors.Num() )
		{
			const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "Group_Disband", "Disband Group") );
			for( int32 GroupIndex = 0; GroupIndex < OutermostGroupActors.Num(); ++GroupIndex )
			{
				AGroupActor* GroupActor = OutermostGroupActors[GroupIndex];
				GroupActor->ClearAndRemove();
			}
		}
	}
}


void UUnrealEdEngine::edactLockSelectedGroups()
{
	if(GEditor->bGroupingActive)
	{
		AGroupActor::LockSelectedGroups();
	}
}


void UUnrealEdEngine::edactUnlockSelectedGroups()
{
	if(GEditor->bGroupingActive)
	{
		AGroupActor::UnlockSelectedGroups();
	}
}


void UUnrealEdEngine::edactAddToGroup()
{
	if(GEditor->bGroupingActive)
	{
		AGroupActor::AddSelectedActorsToSelectedGroup();
	}
}


void UUnrealEdEngine::edactRemoveFromGroup()
{
	if(GEditor->bGroupingActive)
	{
		TArray<AActor*> ActorsToRemove;
		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			// See if an entire group is being removed
			AGroupActor* GroupActor = Cast<AGroupActor>(Actor);
			if(GroupActor == NULL)
			{
				// See if the actor selected belongs to a locked group, if so remove the group in lieu of the actor
				GroupActor = AGroupActor::GetParentForActor(Actor);
				if(GroupActor && !GroupActor->IsLocked())
				{
					GroupActor = NULL;
				}
			}

			if(GroupActor)
			{
				// If the GroupActor has no parent, do nothing, otherwise just add the group for removal
				if(AGroupActor::GetParentForActor(GroupActor))
				{
					ActorsToRemove.AddUnique(GroupActor);
				}
			}
			else
			{
				ActorsToRemove.AddUnique(Actor);
			}
		}

		const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "Group_Remove", "Remove from Group") );
		for ( int32 ActorIndex = 0; ActorIndex < ActorsToRemove.Num(); ++ActorIndex )
		{
			AActor* Actor = ActorsToRemove[ActorIndex];
			AGroupActor* ActorGroup = AGroupActor::GetParentForActor(Actor);

			if(ActorGroup)
			{
				AGroupActor* ActorGroupParent = AGroupActor::GetParentForActor(ActorGroup);
				if(ActorGroupParent)
				{
					ActorGroupParent->Add(*Actor);
				}
				else
				{
					ActorGroup->Remove(*Actor);
				}
			}
		}
		// Do a re-selection of each actor, to maintain group selection rules
		GEditor->SelectNone(true, true);
		for ( int32 ActorIndex = 0; ActorIndex < ActorsToRemove.Num(); ++ActorIndex )
		{
			GEditor->SelectActor( ActorsToRemove[ActorIndex], true, false);
		}
	}
}

void UUnrealEdEngine::edactMergeActors()
{
#if WITH_SIMPLYGON
	TSharedPtr<IMeshProxyDialog> MeshProxyControls;
	if (MeshProxyControls.IsValid() && MeshProxyControls->GetParentWindow().IsValid())
	{
		MeshProxyControls->GetParentWindow()->HideWindow();
	}

	if (!MeshProxyControls.IsValid())
	{
		// Create the controls if they haven't already been created
		MeshProxyControls = IMeshProxyDialog::MakeControls();
	}

	if (!MeshProxyControls->GetParentWindow().IsValid())
	{
		// Create window
		TSharedRef<SWindow> MeshProxyDialog = SNew(SWindow);
		MeshProxyDialog->SetTitle(NSLOCTEXT("SMeshProxyDialog", "MeshProxyDialogTitle", "Mesh Proxy"));
		MeshProxyDialog->Resize(FVector2D(400.0f, 280.0f));

		MeshProxyControls->AssignWindowContent(MeshProxyDialog);
		MeshProxyControls->SetParentWindow(MeshProxyDialog);
		
		MeshProxyDialog->SetOnWindowClosed(FOnWindowClosed::CreateSP(MeshProxyControls.ToSharedRef(), &IMeshProxyDialog::OnWindowClosed ) );

		// Spawn the dialog window
		TSharedPtr<SWindow> ParentWindow;
		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::GetModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		if (ParentWindow.IsValid())
		{
			// Parent the window to the main frame 
			FSlateApplication::Get().AddWindowAsNativeChild(MeshProxyDialog, ParentWindow.ToSharedRef());
		}
		else
		{
			// Spawn new window
			FSlateApplication::Get().AddWindow(MeshProxyDialog);
		}
	}
	MeshProxyControls->GetParentWindow()->ShowWindow();
	MeshProxyControls->MarkDirty();

#endif
}

void UUnrealEdEngine::edactMergeActorsByMaterials()
{
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	USelection* SelectedActors = GetSelectedActors();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;
	for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			Actors.Add(Actor);
			UniqueLevels.AddUnique(Actor->GetLevel());
		}
	}

	// This restriction is only for replacement of selected actors with merged mesh actor
	// Should be optional
	if (UniqueLevels.Num() > 1)
	{
		FText Message = NSLOCTEXT("UnrealEd", "FailedToMergeActorsSublevels_Msg", "The selected actors should be in the same level");
		OpenMsgDlgInt( EAppMsgType::Ok, Message, NSLOCTEXT("UnrealEd", "FailedToMergeActors_Title", "Unable to merge actors") );
		return;
	}

	FString PackageName;

	// TODO: optional
	// Bring a dialog to choose destination package name
	{
		// Use this static mesh path as destination package name for a merged mesh
		for (AActor* Actor : Actors)
		{
			TArray<UStaticMeshComponent*> SMComponets; 
			Actor->GetComponents<UStaticMeshComponent>(SMComponets);
			for (UStaticMeshComponent* Component : SMComponets)
			{
				if (Component->StaticMesh)
				{
					PackageName = FPackageName::GetLongPackagePath(Component->StaticMesh->GetOutermost()->GetName());
					PackageName+= FString(TEXT("/MergedMesh_")) + Component->StaticMesh->GetName();
					break;
				}
			}

			if (!PackageName.IsEmpty())
			{
				break;
			}
		}

		if (PackageName.IsEmpty())
		{
			PackageName = FPackageName::FilenameToLongPackageName(FPaths::GameContentDir() + TEXT("MergedMesh"));
		}
	
		TSharedRef<SDlgPickAssetPath> AssetPathDlg = 
			SNew(SDlgPickAssetPath)
			.Title(NSLOCTEXT("UnrealEd", "SelectDestinationPath", "Select destination path"))
			.DefaultAssetPath(FText::FromString(PackageName));

		if (AssetPathDlg->ShowModal() == EAppReturnType::Cancel)
		{
			return;
		}

		PackageName = AssetPathDlg->GetFullAssetPath().ToString();
	}
			
	FVector MergedActorLocation;
	TArray<UObject*> AssetsToSync;
	MeshUtilities.MergeActors(Actors, PackageName, AssetsToSync, MergedActorLocation);
		
	if (AssetsToSync.Num())
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		int32 AssetCount = AssetsToSync.Num();
		for(int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++ )
		{
			AssetRegistry.AssetCreated(AssetsToSync[AssetIndex]);
			GEditor->BroadcastObjectReimported(AssetsToSync[AssetIndex]);
		}

		//Also notify the content browser that the new assets exists
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets( AssetsToSync, true );

		// Place new mesh in the world
		{
			UWorld* World = UniqueLevels[0]->OwningWorld;
			FActorSpawnParameters Params;
			Params.OverrideLevel = UniqueLevels[0];
			FRotator MergedActorRotation(ForceInit);

			AStaticMeshActor* MergedActor = World->SpawnActor<AStaticMeshActor>(MergedActorLocation, MergedActorRotation, Params);
			MergedActor->StaticMeshComponent->StaticMesh = Cast<UStaticMesh>(AssetsToSync[0]);
			MergedActor->SetActorLabel(AssetsToSync[0]->GetName());

			// Add source actors as children to merged actor and hide them
			for (AActor* Actor : Actors)
			{
				Actor->AttachRootComponentToActor(MergedActor, NAME_None, EAttachLocation::KeepWorldPosition);
				Actor->SetActorHiddenInGame(true);
				Actor->SetIsTemporarilyHiddenInEditor(true);
				Actor->GetRootComponent()->SetVisibility(false, true);
			}
		}
	}
}