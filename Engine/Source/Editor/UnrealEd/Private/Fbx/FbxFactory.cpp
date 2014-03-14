// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*
* Copyright 2008-2009 Autodesk, Inc.  All Rights Reserved.
*
* Permission to use, copy, modify, and distribute this software in object
* code form for any purpose and without fee is hereby granted, provided
* that the above copyright notice appears in all copies and that both
* that copyright notice and the limited warranty and restricted rights
* notice below appear in all supporting documentation.
*
* AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS.
* AUTODESK SPECIFICALLY DISCLAIMS ANY AND ALL WARRANTIES, WHETHER EXPRESS
* OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTY
* OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE OR NON-INFRINGEMENT
* OF THIRD PARTY RIGHTS.  AUTODESK DOES NOT WARRANT THAT THE OPERATION
* OF THE PROGRAM WILL BE UNINTERRUPTED OR ERROR FREE.
*
* In no event shall Autodesk, Inc. be liable for any direct, indirect,
* incidental, special, exemplary, or consequential damages (including,
* but not limited to, procurement of substitute goods or services;
* loss of use, data, or profits; or business interruption) however caused
* and on any theory of liability, whether in contract, strict liability,
* or tort (including negligence or otherwise) arising in any way out
* of such code.
*
* This software is provided to the U.S. Government with the same rights
* and restrictions as described herein.
*/

#include "UnrealEd.h"
#include "Factories.h"
#include "BusyCursor.h"
#include "SSkeletonWidget.h"

#include "FbxImporter.h"


#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "FBXFactory"

UFbxFactory::UFbxFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SupportedClass = NULL;
	Formats.Add(TEXT("fbx;FBX meshes and animations"));

	bCreateNew = false;
	bText = false;
	bEditorImport = true;
	bOperationCanceled = false;
	bDetectImportTypeOnImport = false;
}


void UFbxFactory::PostInitProperties()
{
	Super::PostInitProperties();
	bEditorImport = true;
	bText = false;

	ImportUI = ConstructObject<UFbxImportUI>( UFbxImportUI::StaticClass(), this, NAME_None, RF_NoFlags );
}


bool UFbxFactory::DoesSupportClass(UClass * Class)
{
	return (Class == UStaticMesh::StaticClass() || Class == USkeletalMesh::StaticClass() || Class == UAnimSequence::StaticClass());
}

UClass* UFbxFactory::ResolveSupportedClass()
{
	UClass* ImportClass = NULL;

	if (ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh)
	{
		ImportClass = USkeletalMesh::StaticClass();
	}
	else if (ImportUI->MeshTypeToImport == FBXIT_Animation)
	{
		ImportClass = UAnimSequence::StaticClass();
	}
	else
	{
		ImportClass = UStaticMesh::StaticClass();
	}

	return ImportClass;
}


bool UFbxFactory::DetectImportType(const FString& InFilename)
{
	UnFbx::FFbxImporter* FFbxImporter = UnFbx::FFbxImporter::GetInstance();
	UnFbx::FFbxLoggerSetter Logger(FFbxImporter);
	int32 ImportType = FFbxImporter->GetImportType(InFilename);
	if ( ImportType == -1)
	{
		return false;
	}
	else
	{
		ImportUI->MeshTypeToImport = EFBXImportType(ImportType);
	}
	
	return true;
}


UObject* UFbxFactory::ImportANode(void* VoidFbxImporter, void* VoidNode, UObject* InParent, FName InName, EObjectFlags Flags, int32& NodeIndex, int32 Total, UObject* InMesh, int LODIndex)
{
	UnFbx::FFbxImporter* FFbxImporter = (UnFbx::FFbxImporter*)VoidFbxImporter;
	FbxNode* Node = (FbxNode*)VoidNode;

	UObject* NewObject = NULL;
	FName OutputName = FFbxImporter->MakeNameForMesh(InName.ToString(), Node);
	
	{
		// skip collision models
		FbxString NodeName(Node->GetName());
		if ( NodeName.Find("UCX") != -1 || NodeName.Find("MCDCX") != -1 ||
			 NodeName.Find("UBX") != -1 || NodeName.Find("USP") != -1 )
		{
			return NULL;
		}

		NewObject = FFbxImporter->ImportStaticMesh( InParent, Node, OutputName, Flags, ImportUI->StaticMeshImportData, Cast<UStaticMesh>(InMesh), LODIndex );
	}

	if (NewObject)
	{
		NodeIndex++;
		FFormatNamedArguments Args;
		Args.Add( TEXT("NodeIndex"), NodeIndex );
		Args.Add( TEXT("ArrayLength"), Total );
		GWarn->StatusUpdate( NodeIndex, Total, FText::Format( NSLOCTEXT("UnrealEd", "Importingf", "Importing ({NodeIndex} of {ArrayLength})"), Args ) );
	}

	return NewObject;
}

bool UFbxFactory::ConfigureProperties()
{
	bDetectImportTypeOnImport = true;
	EnableShowOption();

	return true;
}

UObject* UFbxFactory::FactoryCreateBinary
(
 UClass*			Class,
 UObject*			InParent,
 FName				Name,
 EObjectFlags		Flags,
 UObject*			Context,
 const TCHAR*		Type,
 const uint8*&		Buffer,
 const uint8*		BufferEnd,
 FFeedbackContext*	Warn,
 bool&				bOutOperationCanceled
 )
{
	if( bOperationCanceled )
	{
		bOutOperationCanceled = true;
		FEditorDelegates::OnAssetPostImport.Broadcast(this, NULL);
		return NULL;
	}

	FEditorDelegates::OnAssetPreImport.Broadcast(this, Class, InParent, Name, Type);

	UObject* NewObject = NULL;

	if ( bDetectImportTypeOnImport )
	{
		if ( !DetectImportType(UFactory::CurrentFilename) )
		{
			// Failed to read the file info, fail the import
			FEditorDelegates::OnAssetPostImport.Broadcast(this, NULL);
			return NULL;
		}
	}
	// logger for all error/warnings
	// this one prints all messages that are stored in FFbxImporter
	UnFbx::FFbxImporter* FFbxImporter = UnFbx::FFbxImporter::GetInstance();
	UnFbx::FFbxLoggerSetter Logger(FFbxImporter);

	bool bShowImportDialog = bShowOption && !GIsAutomationTesting;
	UnFbx::FBXImportOptions* ImportOptions = GetImportOptions(FFbxImporter, ImportUI, bShowImportDialog, InParent->GetPathName(), bOperationCanceled);
	bOutOperationCanceled = bOperationCanceled;

	// For multiple files, use the same settings
	bDetectImportTypeOnImport = false;

	if (ImportOptions)
	{
		Warn->BeginSlowTask( NSLOCTEXT("FbxFactory", "BeginImportingFbxMeshTask", "Importing FBX mesh"), true );
		if ( !FFbxImporter->ImportFromFile( *UFactory::CurrentFilename ) )
		{
			// Log the error message and fail the import.
			Warn->Log(ELogVerbosity::Error, FFbxImporter->GetErrorMessage() );
		}
		else
		{
			// Log the import message and import the mesh.
			const TCHAR* errorMessage = FFbxImporter->GetErrorMessage();
			if (errorMessage[0] != NULL)
			{
				Warn->Log( errorMessage );
			}

			FbxNode* RootNodeToImport = NULL;
			RootNodeToImport = FFbxImporter->Scene->GetRootNode();

			// For animation and static mesh we assume there is at lease one interesting node by default
			int32 InterestingNodeCount = 1;
			TArray< TArray<FbxNode*>* > SkelMeshArray;

			bool bImportStaticMeshLODs = ImportUI->StaticMeshImportData->bImportMeshLODs;
			bool bCombineMeshes = ImportUI->bCombineMeshes;
			if ( ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh )
			{
				FFbxImporter->FillFbxSkelMeshArrayInScene(RootNodeToImport, SkelMeshArray, false);
				InterestingNodeCount = SkelMeshArray.Num();
			}
			else if( ImportUI->MeshTypeToImport == FBXIT_StaticMesh )
			{
				if( bCombineMeshes && !bImportStaticMeshLODs )
				{
					// If Combine meshes and dont import mesh LODs, the interesting node count should be 1 so all the meshes are grouped together into one static mesh
					InterestingNodeCount = 1;
				}
				else
				{
					// count meshes in lod groups if we dont care about importing LODs
					bool bCountLODGroupMeshes = !bImportStaticMeshLODs;
					int32 NumLODGroups = 0;
					InterestingNodeCount = FFbxImporter->GetFbxMeshCount(RootNodeToImport,bCountLODGroupMeshes,NumLODGroups);

					// if there were LODs in the file, do not combine meshes even if requested
					if( bImportStaticMeshLODs && bCombineMeshes )
					{
						bCombineMeshes = NumLODGroups == 0;
					}
				}
			}

		
			if (InterestingNodeCount > 1)
			{
				// the option only works when there are only one asset
				ImportOptions->bUsedAsFullName = false;
			}

			const FString Filename( UFactory::CurrentFilename );
			if (RootNodeToImport && InterestingNodeCount > 0)
			{  
				int32 NodeIndex = 0;

				int32 ImportedMeshCount = 0;
				UStaticMesh* NewStaticMesh = NULL;
				if ( ImportUI->MeshTypeToImport == FBXIT_StaticMesh )  // static mesh
				{
					if (bCombineMeshes)
					{
						TArray<FbxNode*> FbxMeshArray;
						FFbxImporter->FillFbxMeshArray(RootNodeToImport, FbxMeshArray, FFbxImporter);
						if (FbxMeshArray.Num() > 0)
						{
							NewStaticMesh = FFbxImporter->ImportStaticMeshAsSingle(InParent, FbxMeshArray, Name, Flags, ImportUI->StaticMeshImportData, NULL, 0);
						}

						ImportedMeshCount = NewStaticMesh ? 1 : 0;
					}
					else
					{
						TArray<UObject*> AllNewAssets;
						UObject* Object = RecursiveImportNode(FFbxImporter,RootNodeToImport,InParent,Name,Flags,NodeIndex,InterestingNodeCount, AllNewAssets);

						NewStaticMesh = Cast<UStaticMesh>( Object );

						// Make sure to notify the asset registry of all assets created other than the one returned, which will notify the asset registry automatically.
						for ( auto AssetIt = AllNewAssets.CreateConstIterator(); AssetIt; ++AssetIt )
						{
							UObject* Asset = *AssetIt;
							if ( Asset != NewStaticMesh )
							{
								FAssetRegistryModule::AssetCreated(Asset);
								Asset->MarkPackageDirty();
							}
						}

						ImportedMeshCount = AllNewAssets.Num();
					}

					// Importing static mesh sockets only works if one mesh is being imported
					if( ImportedMeshCount == 1 && NewStaticMesh )
					{
						FFbxImporter->ImportStaticMeshSockets( NewStaticMesh );
					}

					NewObject = NewStaticMesh;

				}
				else if ( ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh )// skeletal mesh
				{
					for (int32 i = 0; i < SkelMeshArray.Num(); i++)
					{
						TArray<FbxNode*> NodeArray = *SkelMeshArray[i];
					
						// check if there is LODGroup for this skeletal mesh
						int32 MaxLODLevel = 1;
						for (int32 j = 0; j < NodeArray.Num(); j++)
						{
							FbxNode* Node = NodeArray[j];
							if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup)
							{
								// get max LODgroup level
								if (MaxLODLevel < Node->GetChildCount())
								{
									MaxLODLevel = Node->GetChildCount();
								}
							}
						}
					
						int32 LODIndex;
						bool bImportSkeletalMeshLODs = ImportUI->SkeletalMeshImportData->bImportMeshLODs;
						for (LODIndex = 0; LODIndex < MaxLODLevel; LODIndex++)
						{
							if ( !bImportSkeletalMeshLODs && LODIndex > 0) // not import LOD if UI option is OFF
							{
								break;
							}
						
							TArray<FbxNode*> SkelMeshNodeArray;
							for (int32 j = 0; j < NodeArray.Num(); j++)
							{
								FbxNode* Node = NodeArray[j];
								if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup)
								{
									if (Node->GetChildCount() > LODIndex)
									{
										SkelMeshNodeArray.Add(Node->GetChild(LODIndex));
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
						
							if (LODIndex == 0 && SkelMeshNodeArray.Num() != 0)
							{
								FName OutputName = FFbxImporter->MakeNameForMesh(Name.ToString(), SkelMeshNodeArray[0]);

								USkeletalMesh* NewMesh = FFbxImporter->ImportSkeletalMesh( InParent, SkelMeshNodeArray, OutputName, Flags, ImportUI->SkeletalMeshImportData, FPaths::GetBaseFilename(Filename) );
								NewObject = NewMesh;

								if ( NewMesh && ImportUI->bImportAnimations )
								{
									FFbxImporter->SetupAnimationDataFromMesh(NewMesh, InParent, SkelMeshNodeArray, ImportUI->AnimSequenceImportData, OutputName.ToString());
								}
							}
							else if (NewObject) // the base skeletal mesh is imported successfully
							{
								USkeletalMesh* BaseSkeletalMesh = Cast<USkeletalMesh>(NewObject);
								FName LODObjectName = NAME_None;
								USkeletalMesh *LODObject = FFbxImporter->ImportSkeletalMesh( GetTransientPackage(), SkelMeshNodeArray, LODObjectName, RF_NoFlags, ImportUI->SkeletalMeshImportData, FPaths::GetBaseFilename(Filename) );
								FFbxImporter->ImportSkeletalMeshLOD( LODObject, BaseSkeletalMesh, LODIndex);
								// Set LOD Model's DisplayFactor
								/*
								fbxDistance Threshold;

								if ( LodGroup->GetThreshold(i-1, Threshold) )
								{
								BaseSkeletalMesh->LODInfo(i).DisplayFactor = (float)((Threshold.value() - LodGroup->MinDistance.Get()) /
								(LodGroup->MaxDistance.Get() - LodGroup->MinDistance.Get()));
								}
								*/
								BaseSkeletalMesh->LODInfo[LODIndex].DisplayFactor = 1.0f / (MaxLODLevel * LODIndex);
							}
						
							// import morph target
							if ( NewObject && ImportUI->SkeletalMeshImportData->bImportMorphTargets)
							{
								// Disable material importing when importing morph targets
								uint32 bImportMaterials = ImportOptions->bImportMaterials;
								ImportOptions->bImportMaterials = 0;

								FFbxImporter->ImportFbxMorphTarget(SkelMeshNodeArray, Cast<USkeletalMesh>(NewObject), InParent, Filename, LODIndex);
							
								ImportOptions->bImportMaterials = !!bImportMaterials;
							}
						}
					
						if (NewObject)
						{
							NodeIndex++;
							FFormatNamedArguments Args;
							Args.Add( TEXT("NodeIndex"), NodeIndex );
							Args.Add( TEXT("ArrayLength"), SkelMeshArray.Num() );
							GWarn->StatusUpdate( NodeIndex, SkelMeshArray.Num(), FText::Format( NSLOCTEXT("UnrealEd", "Importingf", "Importing ({NodeIndex} of {ArrayLength})"), Args ) );
						}
					}
				
					for (int32 i = 0; i < SkelMeshArray.Num(); i++)
					{
						delete SkelMeshArray[i];
					}
				}
				else if ( ImportUI->MeshTypeToImport == FBXIT_Animation )// animation
				{
					if (ImportOptions->SkeletonForAnimation)
					{
						// will return the last animation sequence that were added
						NewObject = UEditorEngine::ImportFbxAnimation( ImportOptions->SkeletonForAnimation, InParent, ImportUI->AnimSequenceImportData, *Filename, *Name.ToString(), true );
					}
				}
			}
			else
			{
				if (RootNodeToImport == NULL)
				{
					FFbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, LOCTEXT("FailedToImport_InvalidRoot", "Could not find root node.")));
				}
				else if (ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh)
				{
					FFbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, LOCTEXT("FailedToImport_InvalidBone", "Failed to find any bone hierarchy. Try to import as Rigid Mesh option enabled. ")));
				}
				else
				{
					FFbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, LOCTEXT("FailedToImport_InvalidNode", "Could not find any node.")));
				}
			}
		}

		if (NewObject == NULL)
		{
			FFbxImporter->AddTokenizedErrorMessage(FTokenizedMessage::Create(EMessageSeverity::Error, LOCTEXT("FailedToImport_NoObject", "Import failed.")));
		}

		FFbxImporter->ReleaseScene();
		Warn->EndSlowTask();
	}

	FEditorDelegates::OnAssetPostImport.Broadcast(this, NewObject);

	return NewObject;
}


UObject* UFbxFactory::RecursiveImportNode(void* VoidFbxImporter, void* VoidNode, UObject* InParent, FName InName, EObjectFlags Flags, int32& NodeIndex, int32 Total, TArray<UObject*>& OutNewAssets)
{
	UObject* NewObject = NULL;

	FbxNode* Node = (FbxNode*)VoidNode;
	if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eLODGroup && Node->GetChildCount() > 0 )
	{
		// import base mesh
		NewObject = ImportANode(VoidFbxImporter, Node->GetChild(0), InParent, InName, Flags, NodeIndex, Total);

		if ( NewObject )
		{
			OutNewAssets.Add(NewObject);
		}

		bool bImportMeshLODs;
		if ( ImportUI->MeshTypeToImport == FBXIT_StaticMesh )
		{
			bImportMeshLODs = ImportUI->StaticMeshImportData->bImportMeshLODs;
		}
		else if ( ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh )
		{
			bImportMeshLODs = ImportUI->SkeletalMeshImportData->bImportMeshLODs;
		}
		else
		{
			bImportMeshLODs = false;
		}

		if (NewObject && bImportMeshLODs)
		{
			// import LOD meshes
			for (int32 LODIndex = 1; LODIndex < Node->GetChildCount(); LODIndex++)
			{
				FbxNode* ChildNode = Node->GetChild(LODIndex);
				ImportANode(VoidFbxImporter, ChildNode, InParent, InName, Flags, NodeIndex, Total, NewObject, LODIndex);
			}
		}
	}
	else
	{
		if (Node->GetMesh())
		{
			NewObject = ImportANode(VoidFbxImporter, Node, InParent, InName, Flags, NodeIndex, Total);

			if ( NewObject )
			{
				OutNewAssets.Add(NewObject);
			}
		}
		
		for (int32 ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
		{
			UObject* SubObject = RecursiveImportNode(VoidFbxImporter,Node->GetChild(ChildIndex),InParent,InName,Flags,NodeIndex,Total,OutNewAssets);

			if ( SubObject )
			{
				OutNewAssets.Add(SubObject);
			}

			if (NewObject==NULL)
			{
				NewObject = SubObject;
			}
		}
	}

	return NewObject;
}


void UFbxFactory::CleanUp() 
{
	UnFbx::FFbxImporter* FFbxImporter = UnFbx::FFbxImporter::GetInstance();
	bDetectImportTypeOnImport = true;
	// load options
	if (FFbxImporter)
	{
		struct UnFbx::FBXImportOptions* ImportOptions = FFbxImporter->GetImportOptions();
		if ( ImportOptions )
		{
			ImportOptions->SkeletonForAnimation = NULL;
			ImportOptions->PhysicsAsset = NULL;
		}
	}
}

UFbxImportUI::UFbxImportUI(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCombineMeshes = true;

	StaticMeshImportData = ConstructObject<UFbxStaticMeshImportData>(UFbxStaticMeshImportData::StaticClass(), this);
	SkeletalMeshImportData = ConstructObject<UFbxSkeletalMeshImportData>(UFbxSkeletalMeshImportData::StaticClass(), this);
	AnimSequenceImportData = ConstructObject<UFbxAnimSequenceImportData>(UFbxAnimSequenceImportData::StaticClass(), this);
	TextureImportData = ConstructObject<UFbxTextureImportData>(UFbxTextureImportData::StaticClass(), this);
}


bool UFbxImportUI::CanEditChange( const UProperty* InProperty ) const
{
	bool bIsMutable = Super::CanEditChange( InProperty );
	if( bIsMutable && InProperty != NULL )
	{
		if( InProperty->GetFName() == TEXT( "AnimationName" ) )
		{
			bIsMutable = (MeshTypeToImport == FBXIT_SkeletalMesh);
		}
	}

	return bIsMutable;
}

#undef LOCTEXT_NAMESPACE