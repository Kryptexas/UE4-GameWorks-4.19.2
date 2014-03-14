// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "LODUtilities.h"

#include "AnimSetViewer/LODSettingsWindow.h"

#if WITH_FBX
	#include "FbxImporter.h"
	#include "FbxExporter.h"
#endif // WITH_FBX

#include "MeshUtilities.h"

#if WITH_APEX_CLOTHING
	#include "ApexClothingUtils.h"
#endif // #if WITH_APEX_CLOTHING

void FLODUtilities::RemoveLOD(FSkeletalMeshUpdateContext& UpdateContext, int32 DesiredLOD )
{
	USkeletalMesh* SkeletalMesh = UpdateContext.SkeletalMesh;
	FSkeletalMeshResource* SkelMeshResource = SkeletalMesh->GetImportedResource();

	if( SkelMeshResource->LODModels.Num() == 1 )
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "NoLODToRemove", "No LODs to remove!") );
		return;
	}

	// Now display combo to choose which LOD to remove.
	TArray<FString> LODStrings;
	LODStrings.AddZeroed( SkelMeshResource->LODModels.Num()-1 );
	for(int32 i=0; i<SkelMeshResource->LODModels.Num()-1; i++)
	{
		LODStrings[i] = FString::Printf( TEXT("%d"), i+1 );
	}

	check( SkeletalMesh->LODInfo.Num() == SkelMeshResource->LODModels.Num() );

	// If its a valid LOD, kill it.
	if( DesiredLOD > 0 && DesiredLOD < SkelMeshResource->LODModels.Num() )
	{
		//We'll be modifying the skel mesh data so reregister

		//TODO - do we need to reregister something else instead?
		FMultiComponentReregisterContext ReregisterContext(UpdateContext.AssociatedComponents);

		// Release rendering resources before deleting LOD
		SkelMeshResource->ReleaseResources();

		// Block until this is done
		FlushRenderingCommands();

		SkelMeshResource->LODModels.RemoveAt(DesiredLOD);
		SkeletalMesh->LODInfo.RemoveAt(DesiredLOD);
		SkeletalMesh->InitResources();

		RefreshLODChange(SkeletalMesh);

		// Set the forced LOD to Auto.
		for(auto Iter = UpdateContext.AssociatedComponents.CreateIterator(); Iter; ++Iter)
		{
			USkinnedMeshComponent* SkinnedComponent = Cast<USkinnedMeshComponent>(*Iter);
			if(SkinnedComponent)
			{
				SkinnedComponent->ForcedLodModel = 0;
			}
		}
		
		//Notify calling system of change
		UpdateContext.OnLODChanged.ExecuteIfBound();

		// Mark things for saving.
		SkeletalMesh->MarkPackageDirty();
	}
}

void FLODUtilities::ImportMeshLOD( FSkeletalMeshUpdateContext& UpdateContext, int32 DesiredLOD)
{
	USkeletalMesh* SkeletalMesh = UpdateContext.SkeletalMesh;

#if WITH_FBX
	if(!SkeletalMesh)
	{
		return;
	}

	FString ExtensionStr;

	ExtensionStr += TEXT("FBX files|*.fbx|");

	ExtensionStr += TEXT("All files|*.*");

	// First, display the file open dialog for selecting the file.
	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpen = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		bOpen = DesktopPlatform->SaveFileDialog(
			ParentWindowWindowHandle,
		NSLOCTEXT("ImportSkeletalMeshLOD", "ImportMeshLOD", "Import Mesh LOD...").ToString(), 
		*(FEditorDirectories::Get().GetLastDirectory(ELastDirectory::FBX)),
		TEXT(""),
		*ExtensionStr,
			EFileDialogFlags::None,
			OpenFilenames
			);
	}

	// Only continue if we pressed OK and have only one file selected.
	if( bOpen )
	{
		if(OpenFilenames.Num() == 0)
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("ImportSkeletalMeshLOD", "NoFileSelectedForLOD", "No file was selected for the LOD.") );
		}
		else if(OpenFilenames.Num() > 1)
		{
			FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("ImportSkeletalMeshLOD", "MultipleFilesSelectedForLOD", "You may only select one file for the LOD.") );
		}
		else
		{

			FString Filename = OpenFilenames[0];
			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::FBX, FPaths::GetPath(Filename)); // Save path as default for next time.

			// Check the file extension for FBX. Anything that isn't .FBX is rejected
			const FString FileExtension = FPaths::GetExtension(Filename);
			const bool bIsFBX = FCString::Stricmp(*FileExtension, TEXT("FBX")) == 0;

			if (bIsFBX)
			{
				UnFbx::FFbxImporter* FFbxImporter = UnFbx::FFbxImporter::GetInstance();
				// don't import material and animation
				UnFbx::FBXImportOptions* ImportOptions = FFbxImporter->GetImportOptions();
				ImportOptions->bImportMaterials = false;
				ImportOptions->bImportTextures = false;
				ImportOptions->bImportAnimations = false;

				if ( !FFbxImporter->ImportFromFile( *Filename ) )
				{
					// Log the error message and fail the import.
					//UE_LOG(LogAnimSetViewer, Log, TEXT("FBX file parse failed"));
					//Warn->Log(ELogVerbosity::Error, FFbxImporter->GetErrorMessage() );
				}
				else
				{
					bool bUseLODs = true;
					int32 MaxLODLevel = 0;
					TArray< TArray<FbxNode*>* > MeshArray;
					TArray<FString> LODStrings;
					TArray<FbxNode*>* MeshObject = NULL;;

					// Populate the mesh array
					FFbxImporter->FillFbxSkelMeshArrayInScene(FFbxImporter->Scene->GetRootNode(), MeshArray, false);

					// Nothing found, error out
					if (MeshArray.Num() == 0)
					{
						FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("ImportMeshLOD", "Prompt_NoMeshFound", "No meshes were found in file.") );
						FFbxImporter->ReleaseScene();
						return;
					}

					MeshObject = MeshArray[0];

					// check if there is LODGroup for this skeletal mesh
					for (int32 j = 0; j < MeshObject->Num(); j++)
					{
						FbxNode* Node = (*MeshObject)[j];
						if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup)
						{
							// get max LODgroup level
							if (MaxLODLevel < (Node->GetChildCount() - 1))
							{
								MaxLODLevel = Node->GetChildCount() - 1;
							}
						}
					}

					// No LODs found, switch to supporting a mesh array containing meshes instead of LODs
					if (MaxLODLevel == 0)
					{
						bUseLODs = false;
						MaxLODLevel = SkeletalMesh->LODInfo.Num();
					}

					// Create LOD dropdown strings
					LODStrings.AddZeroed(MaxLODLevel + 1);
					LODStrings[0] = FString::Printf( TEXT("Base") );
					for(int32 i = 1; i < MaxLODLevel + 1; i++)
					{
						LODStrings[i] = FString::Printf(TEXT("%d"), i);
					}


					int32 SelectedLOD = DesiredLOD;
					if (SelectedLOD > SkeletalMesh->LODInfo.Num())
					{
						// Make sure they don't manage to select a bad LOD index
						FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("ImportMeshLOD", "Prompt_InvalidLODIndex", "Invalid mesh LOD index {0}, as no prior LOD index exists!"), FText::AsNumber(SelectedLOD)) );
					}
					else
					{
						// Find the LOD node to import (if using LODs)
						TArray<FbxNode*> SkelMeshNodeArray;
						if (bUseLODs)
						{
							for (int32 j = 0; j < MeshObject->Num(); j++)
							{
								FbxNode* Node = (*MeshObject)[j];
								if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup)
								{
									if (Node->GetChildCount() > SelectedLOD)
									{
										SkelMeshNodeArray.Add(Node->GetChild(SelectedLOD));
									}
									else // in less some LODGroups have less level, use the last level
									{
										SkelMeshNodeArray.Add(Node->GetChild(Node->GetChildCount() - 1));
									}
								}
								else
								{
									SkelMeshNodeArray.Add(Node);
								}
							}
						}

						// Import mesh
						USkeletalMesh* TempSkelMesh = NULL;
						UFbxSkeletalMeshImportData* TempImportData = NULL;
						TempSkelMesh = (USkeletalMesh*)FFbxImporter->ImportSkeletalMesh(GetTransientPackage(), bUseLODs? SkelMeshNodeArray: *MeshObject, NAME_None, (EObjectFlags)0, TempImportData, FPaths::GetBaseFilename(Filename));

						// Add imported mesh to existing model
						bool bImportSucceeded = false;
						if( TempSkelMesh )
						{
							bImportSucceeded = ImportMeshLOD(UpdateContext, TempSkelMesh, SelectedLOD);

							//Notify calling system of change
							UpdateContext.OnLODChanged.ExecuteIfBound();

							// Mark package containing skeletal mesh as dirty.
							SkeletalMesh->MarkPackageDirty();
						}

						if (bImportSucceeded)
						{
							// Pop up a box saying it worked.
							FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("ImportMeshLOD", "LODImportSuccessful", "Mesh for LOD {0} imported successfully!"), FText::AsNumber(SelectedLOD)) );
						}
						else
						{
							// Import failed
							FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("ImportMeshLOD", "LODImportFail", "Failed to import mesh for LOD {0}!"), FText::AsNumber(SelectedLOD)) );
						}
					}

					// Cleanup
					for (int32 i=0; i<MeshArray.Num(); i++)
					{
						delete MeshArray[i];
					}					
				}
				FFbxImporter->ReleaseScene();
			}
		}
	}
#endif
}

bool FLODUtilities::ImportMeshLOD(FSkeletalMeshUpdateContext& UpdateContext, USkeletalMesh* InputSkeletalMesh, int32 DesiredLOD)
{
	check(InputSkeletalMesh);

	USkeletalMesh* DestSkeletalMesh = UpdateContext.SkeletalMesh;
	FSkeletalMeshResource* InputSkelMeshResource = InputSkeletalMesh->GetImportedResource();
	FSkeletalMeshResource* DestSkelMeshResource = UpdateContext.SkeletalMesh->GetImportedResource();

	// Now we copy the base FStaticLODModel from the imported skeletal mesh as the new LOD in the selected mesh.
	check(InputSkelMeshResource->LODModels.Num() == 1);

	// Names of root bones must match.
	if(InputSkeletalMesh->RefSkeleton.GetBoneName(0) != DestSkeletalMesh->RefSkeleton.GetBoneName(0))
	{
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("ImportSkeletalMeshLOD", "LODRootNameIncorrect", "Root bone in LOD is '{0}' instead of '{1}'.\nImport failed."),
			FText::FromName(InputSkeletalMesh->RefSkeleton.GetBoneName(0)), FText::FromName(DestSkeletalMesh->RefSkeleton.GetBoneName(0))) );
		return false;
	}

	// We do some checking here that for every bone in the mesh we just imported, it's in our base ref skeleton, and the parent is the same.
	for(int32 i=0; i<InputSkeletalMesh->RefSkeleton.GetNum(); i++)
	{
		int32 LODBoneIndex = i;
		FName LODBoneName = InputSkeletalMesh->RefSkeleton.GetBoneName(LODBoneIndex);
		int32 BaseBoneIndex = DestSkeletalMesh->RefSkeleton.FindBoneIndex(LODBoneName);
		if( BaseBoneIndex == INDEX_NONE )
		{
			// If we could not find the bone from this LOD in base mesh - we fail.
			FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("ImportSkeletalMeshLOD", "LODBoneDoesNotMatch", "Bone '{0}' not found in base SkeletalMesh '{1}'.\nImport failed."),
				FText::FromName(LODBoneName), FText::FromString(DestSkeletalMesh->GetName())) );
			return false;
		}

		if(i>0)
		{
			int32 LODParentIndex = InputSkeletalMesh->RefSkeleton.GetParentIndex(LODBoneIndex);
			FName LODParentName = InputSkeletalMesh->RefSkeleton.GetBoneName(LODParentIndex);

			int32 BaseParentIndex = DestSkeletalMesh->RefSkeleton.GetParentIndex(BaseBoneIndex);
			FName BaseParentName = DestSkeletalMesh->RefSkeleton.GetBoneName(BaseParentIndex);

			if(LODParentName != BaseParentName)
			{
				// If bone has different parents, display an error and don't allow import.
				FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("ImportSkeletalMeshLOD", "LODBoneHasIncorrectParent", "Bone '{0}' in LOD has parent '{1}' instead of '{2}'"),
					FText::FromName(LODBoneName), FText::FromName(LODParentName), FText::FromName(BaseParentName)) );
				return false;
			}
		}
	}

	FStaticLODModel& NewLODModel = InputSkelMeshResource->LODModels[0];

	// Enforce LODs having only single-influence vertices.
	bool bCheckSingleInfluence;
	GConfig->GetBool( TEXT("AnimSetViewer"), TEXT("CheckSingleInfluenceLOD"), bCheckSingleInfluence, GEditorIni );
	if( bCheckSingleInfluence && 
		DesiredLOD > 0 )
	{
		for(int32 ChunkIndex = 0;ChunkIndex < NewLODModel.Chunks.Num();ChunkIndex++)
		{
			if(NewLODModel.Chunks[ChunkIndex].SoftVertices.Num() > 0)
			{
				FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("ImportSkeletalMeshLOD", "LODHasSoftVertices", "Warning: The mesh LOD you are importing has some vertices with more than one influence.") );
			}
		}
	}

	// If this LOD is going to be the lowest one, we check all bones we have sockets on are present in it.
	if( DesiredLOD == DestSkelMeshResource->LODModels.Num() || 
		DesiredLOD == DestSkelMeshResource->LODModels.Num()-1 )
	{
		const TArray<USkeletalMeshSocket*>& SocketList = DestSkeletalMesh->GetActiveSocketList();

		for(int32 i=0; i<SocketList.Num(); i++)
		{
			// Find bone index the socket is attached to.
			USkeletalMeshSocket* Socket = SocketList[i];
			int32 SocketBoneIndex = InputSkeletalMesh->RefSkeleton.FindBoneIndex( Socket->BoneName );

			// If this LOD does not contain the socket bone, abort import.
			if( SocketBoneIndex == INDEX_NONE )
			{
				FMessageDialog::Open( EAppMsgType::Ok, FText::Format(NSLOCTEXT("ImportSkeletalMeshLOD", "LODMissingSocketBone", "This LOD is missing bone '{0}' used by socket '{1}'.\nAborting import."),
					FText::FromName(Socket->BoneName), FText::FromName(Socket->SocketName)) );
				return false;
			}
		}
	}

	// Fix up the ActiveBoneIndices array.
	for(int32 i=0; i<NewLODModel.ActiveBoneIndices.Num(); i++)
	{
		int32 LODBoneIndex = NewLODModel.ActiveBoneIndices[i];
		FName LODBoneName = InputSkeletalMesh->RefSkeleton.GetBoneName(LODBoneIndex);
		int32 BaseBoneIndex = DestSkeletalMesh->RefSkeleton.FindBoneIndex(LODBoneName);
		NewLODModel.ActiveBoneIndices[i] = BaseBoneIndex;
	}

	// Fix up the chunk BoneMaps.
	for(int32 ChunkIndex = 0;ChunkIndex < NewLODModel.Chunks.Num();ChunkIndex++)
	{
		FSkelMeshChunk& Chunk = NewLODModel.Chunks[ChunkIndex];
		for(int32 i=0; i<Chunk.BoneMap.Num(); i++)
		{
			int32 LODBoneIndex = Chunk.BoneMap[i];
			FName LODBoneName = InputSkeletalMesh->RefSkeleton.GetBoneName(LODBoneIndex);
			int32 BaseBoneIndex = DestSkeletalMesh->RefSkeleton.FindBoneIndex(LODBoneName);
			Chunk.BoneMap[i] = BaseBoneIndex;
		}
	}

	// Create the RequiredBones array in the LODModel from the ref skeleton.
	for(int32 i=0; i<NewLODModel.RequiredBones.Num(); i++)
	{
		FName LODBoneName = InputSkeletalMesh->RefSkeleton.GetBoneName(NewLODModel.RequiredBones[i]);
		int32 BaseBoneIndex = DestSkeletalMesh->RefSkeleton.FindBoneIndex(LODBoneName);
		if(BaseBoneIndex != INDEX_NONE)
		{
			NewLODModel.RequiredBones[i] = BaseBoneIndex;
		}
		else
		{
			NewLODModel.RequiredBones.RemoveAt(i--);
		}
	}

	// Also sort the RequiredBones array to be strictly increasing.
	NewLODModel.RequiredBones.Sort();

	// To be extra-nice, we apply the difference between the root transform of the meshes to the verts.
	FMatrix LODToBaseTransform = InputSkeletalMesh->GetRefPoseMatrix(0).Inverse() * DestSkeletalMesh->GetRefPoseMatrix(0);

	for(int32 ChunkIndex = 0;ChunkIndex < NewLODModel.Chunks.Num();ChunkIndex++)
	{
		FSkelMeshChunk& Chunk = NewLODModel.Chunks[ChunkIndex];
		// Fix up rigid verts.
		for(int32 i=0; i<Chunk.RigidVertices.Num(); i++)
		{
			Chunk.RigidVertices[i].Position = LODToBaseTransform.TransformPosition( Chunk.RigidVertices[i].Position );
			Chunk.RigidVertices[i].TangentX = LODToBaseTransform.TransformVector( Chunk.RigidVertices[i].TangentX );
			Chunk.RigidVertices[i].TangentY = LODToBaseTransform.TransformVector( Chunk.RigidVertices[i].TangentY );
			Chunk.RigidVertices[i].TangentZ = LODToBaseTransform.TransformVector( Chunk.RigidVertices[i].TangentZ );
		}

		// Fix up soft verts.
		for(int32 i=0; i<Chunk.SoftVertices.Num(); i++)
		{
			Chunk.SoftVertices[i].Position = LODToBaseTransform.TransformPosition( Chunk.SoftVertices[i].Position );
			Chunk.SoftVertices[i].TangentX = LODToBaseTransform.TransformVector( Chunk.SoftVertices[i].TangentX );
			Chunk.SoftVertices[i].TangentY = LODToBaseTransform.TransformVector( Chunk.SoftVertices[i].TangentY );
			Chunk.SoftVertices[i].TangentZ = LODToBaseTransform.TransformVector( Chunk.SoftVertices[i].TangentZ );
		}
	}

	// Shut down the skeletal mesh component that is previewing this mesh.
	{
		FMultiComponentReregisterContext ReregisterContext(UpdateContext.AssociatedComponents);
		FlushRenderingCommands();

		// If we want to add this as a new LOD to this mesh - add to LODModels/LODInfo array.
		if(DesiredLOD == DestSkelMeshResource->LODModels.Num())
		{
			new(DestSkelMeshResource->LODModels)FStaticLODModel();

			// Add element to LODInfo array.
			DestSkeletalMesh->LODInfo.AddZeroed();
			check( DestSkeletalMesh->LODInfo.Num() == DestSkelMeshResource->LODModels.Num() );
			DestSkeletalMesh->LODInfo[DesiredLOD] = InputSkeletalMesh->LODInfo[0];
		}
		else
		{
			// if it's overwriting existing LOD, need to update section information
			// update to the right # of sections 
			// Set up LODMaterialMap to number of materials in new mesh.
			// InputSkeletalMesh->LOD 0 is the newly imported mesh
			FSkeletalMeshLODInfo& LODInfo = DestSkeletalMesh->LODInfo[DesiredLOD];
			FStaticLODModel& LODModel = InputSkelMeshResource->LODModels[0];
			// if section # has been changed
 			if (LODInfo.TriangleSortSettings.Num() != LODModel.Sections.Num())
 			{
 				// Save old information so that I can copy it over
 				TArray<FTriangleSortSettings> OldTriangleSortSettings;
 
 				OldTriangleSortSettings = LODInfo.TriangleSortSettings;
 
 				// resize to the correct number
 				LODInfo.TriangleSortSettings.Empty(LODModel.Sections.Num());
 				// fill up data
 				for ( int32 SectionIndex = 0 ; SectionIndex < LODModel.Sections.Num() ; ++SectionIndex )
 				{
 					// if found from previous data, copy over
 					if ( SectionIndex < OldTriangleSortSettings.Num() )
 					{
 						LODInfo.TriangleSortSettings.Add( OldTriangleSortSettings[SectionIndex] );
 					}
 					else
 					{
 						// if not add default data
 						LODInfo.TriangleSortSettings.AddZeroed();
 					}
 				}
 			}
		}

		// Set up LODMaterialMap to number of materials in new mesh.
		FSkeletalMeshLODInfo& LODInfo = DestSkeletalMesh->LODInfo[DesiredLOD];
		LODInfo.LODMaterialMap.Empty();

		// Now set up the material mapping array.
		for(int32 MatIdx = 0; MatIdx < InputSkeletalMesh->Materials.Num(); MatIdx++)
		{
			// Try and find the auto-assigned material in the array.
			int32 LODMatIndex = INDEX_NONE;
			if (InputSkeletalMesh->Materials[MatIdx].MaterialInterface != NULL)
			{
				LODMatIndex = DestSkeletalMesh->Materials.Find( InputSkeletalMesh->Materials[MatIdx] );
			}

			// If we didn't just use the index - but make sure its within range of the Materials array.
			if(LODMatIndex == INDEX_NONE)
			{
				LODMatIndex = FMath::Clamp(MatIdx, 0, DestSkeletalMesh->Materials.Num() - 1);
			}

			LODInfo.LODMaterialMap.Add(LODMatIndex);
		}

		// if new LOD has more material slot, add the extra to main skeletal
		if ( DestSkeletalMesh->Materials.Num() < InputSkeletalMesh->Materials.Num() )
		{
			DestSkeletalMesh->Materials.AddZeroed(InputSkeletalMesh->Materials.Num()-DestSkeletalMesh->Materials.Num());
		}

		// Release all resources before replacing the model
		DestSkeletalMesh->PreEditChange(NULL);

		// Index buffer will be destroyed when we copy the LOD model so we must copy the index buffer and reinitialize it after the copy
		FMultiSizeIndexContainerData Data;
		NewLODModel.MultiSizeIndexContainer.GetIndexBufferData( Data );

		// Assign new FStaticLODModel to desired slot in selected skeletal mesh.
		DestSkelMeshResource->LODModels[DesiredLOD] = NewLODModel;

		DestSkelMeshResource->LODModels[DesiredLOD].MultiSizeIndexContainer.RebuildIndexBuffer( Data );

		// rebuild vertex buffers and reinit RHI resources
		DestSkeletalMesh->PostEditChange();		

		// ReregisterContexts go out of scope here, reregistering skel components with the scene.
	}

	return true;
}

void FLODUtilities::SimplifySkeletalMesh( FSkeletalMeshUpdateContext& UpdateContext, TArray<FSkeletalMeshOptimizationSettings> &InSettings )
{
	USkeletalMesh* SkeletalMesh = UpdateContext.SkeletalMesh;
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	IMeshReduction* MeshReduction = MeshUtilities.GetMeshReductionInterface();

	if ( MeshReduction && SkeletalMesh )
	{
		// Simplify each LOD
		for( int32 LODIndex = 0; LODIndex < InSettings.Num(); ++LODIndex)
		{
			uint32 DesiredLOD = LODIndex+1;

			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("DesiredLOD"), DesiredLOD );
				Args.Add( TEXT("SkeletalMeshName"), FText::FromString(SkeletalMesh->GetName()) );
				const FText StatusUpdate = FText::Format( NSLOCTEXT("UnrealEd", "MeshSimp_GeneratingLOD_F", "Generating LOD{DesiredLOD} for {SkeletalMeshName}..." ), Args );
				GWarn->BeginSlowTask( StatusUpdate, true );
			}

			if ( MeshReduction->ReduceSkeletalMesh( SkeletalMesh, DesiredLOD, InSettings[LODIndex] , true ) )
			{
				check( SkeletalMesh->LODInfo.Num() >= 2 );
				SkeletalMesh->MarkPackageDirty();
#if WITH_APEX_CLOTHING
				ApexClothingUtils::ReImportClothingSectionsFromClothingAsset(SkeletalMesh);
#endif// #if WITH_APEX_CLOTHING
			}
			else
			{
				// Simplification failed! Warn the user.
				FFormatNamedArguments Args;
				Args.Add( TEXT("SkeletalMeshName"), FText::FromString(SkeletalMesh->GetName()) );
				const FText Message = FText::Format( NSLOCTEXT("UnrealEd", "MeshSimp_GenerateLODFailed_F", "An error occurred while simplifying the geometry for mesh '{SkeletalMeshName}'.  Consider adjusting simplification parameters and re-simplifying the mesh." ), Args );
				FMessageDialog::Open( EAppMsgType::Ok, Message );
			}
			GWarn->EndSlowTask();
		}

		//Notify calling system of change
		UpdateContext.OnLODChanged.ExecuteIfBound();
	}
}

void FLODUtilities::RefreshLODChange(const USkeletalMesh * SkeletalMesh)
{
	for (FObjectIterator Iter(USkeletalMeshComponent::StaticClass()); Iter; ++Iter)
	{
		USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(*Iter);
		if  (SkeletalMeshComponent->SkeletalMesh == SkeletalMesh)
		{
			// it needs to recreate IF it already has been created
			if (SkeletalMeshComponent->IsRegistered())
			{
				SkeletalMeshComponent->UpdateLODStatus();
				SkeletalMeshComponent->MarkRenderStateDirty();
			}
		}
	}
}

void FLODUtilities::CreateLODSettingsDialog(FSkeletalMeshUpdateContext& UpdateContext)
{
	if ( UpdateContext.SkeletalMesh )
	{
		// Display the LOD Settings dialog window.
		FDlgSkeletalMeshLODSettings CreateLODsDialog( UpdateContext );
		CreateLODsDialog.ShowModal();
	}
}
