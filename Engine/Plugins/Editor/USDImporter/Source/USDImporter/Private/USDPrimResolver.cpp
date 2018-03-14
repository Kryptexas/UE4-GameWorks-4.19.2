// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "USDPrimResolver.h"
#include "USDImporter.h"
#include "USDConversionUtils.h"
#include "AssetData.h"
#include "ModuleManager.h"
#include "AssetRegistryModule.h"
#include "USDSceneImportFactory.h"
#include "AssetSelection.h"
#include "IUSDImporterModule.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "ActorFactories/ActorFactory.h"

#define LOCTEXT_NAMESPACE "USDImportPlugin"

void UUSDPrimResolver::Init()
{
	AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
}

void UUSDPrimResolver::FindMeshAssetsToImport(FUsdImportContext& ImportContext, IUsdPrim* StartPrim, TArray<FUsdAssetPrimToImport>& OutAssetsToImport, bool bRecursive) const
{
	const FString PrimName = USDToUnreal::ConvertString(StartPrim->GetPrimName());

	const FString KindName = USDToUnreal::ConvertString(StartPrim->GetKind());

	bool bHasUnrealAssetPath = StartPrim->GetUnrealAssetPath() != nullptr;
	bool bHasUnrealActorClass = StartPrim->GetUnrealActorClass() != nullptr;

	if (!StartPrim->IsProxyOrGuide())
	{
		if (StartPrim->HasGeometryDataOrLODVariants())
		{
			FUsdAssetPrimToImport NewTopLevelPrim;

			FString FinalPrimName;
			// if the prim has a path use that as the final name
			if (bHasUnrealAssetPath)
			{
				FinalPrimName = StartPrim->GetUnrealAssetPath();
			}
			else
			{
				FinalPrimName = PrimName;
			}

			NewTopLevelPrim.Prim = StartPrim;
			NewTopLevelPrim.AssetPath = FinalPrimName;

			FindMeshChildren(ImportContext, StartPrim, true, NewTopLevelPrim.MeshPrims);

			for (IUsdPrim* MeshPrim : NewTopLevelPrim.MeshPrims)
			{
				NewTopLevelPrim.NumLODs = FMath::Max(NewTopLevelPrim.NumLODs, MeshPrim->GetNumLODs());
			}

			OutAssetsToImport.Add(NewTopLevelPrim);
		}
		else if(bRecursive)
		{
			int32 NumChildren = StartPrim->GetNumChildren();
			for (int32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
			{
				FindMeshAssetsToImport(ImportContext, StartPrim->GetChild(ChildIdx), OutAssetsToImport);
			}
		}
	}
}

void UUSDPrimResolver::FindActorsToSpawn(FUSDSceneImportContext& ImportContext, TArray<FActorSpawnData>& OutActorSpawnDatas) const
{
	IUsdPrim* RootPrim = ImportContext.RootPrim;

	if (RootPrim->HasTransform())
	{
		FindActorsToSpawn_Recursive(ImportContext, RootPrim, nullptr, OutActorSpawnDatas);
	}
	else
	{
		for (int32 ChildIdx = 0; ChildIdx < RootPrim->GetNumChildren(); ++ChildIdx)
		{
			IUsdPrim* Child = RootPrim->GetChild(ChildIdx);

			FindActorsToSpawn_Recursive(ImportContext, Child, nullptr, OutActorSpawnDatas);
		}
	}
}

AActor* UUSDPrimResolver::SpawnActor(FUSDSceneImportContext& ImportContext, const FActorSpawnData& SpawnData)
{
	UUSDImporter* USDImporter = IUSDImporterModule::Get().GetImporter();

	UUSDSceneImportOptions* ImportOptions = Cast<UUSDSceneImportOptions>(ImportContext.ImportOptions);

	const bool bFlattenHierarchy = ImportOptions->bFlattenHierarchy;

	AActor* ModifiedActor = nullptr;

	// Look for an existing actor and decide what to do based on the users choice
	AActor* ExistingActor = ImportContext.ExistingActors.FindRef(SpawnData.ActorName);

	bool bShouldSpawnNewActor = true;
	EExistingActorPolicy ExistingActorPolicy = ImportOptions->ExistingActorPolicy;

	const FMatrix& ActorMtx = SpawnData.WorldTransform;

	const FTransform ConversionTransform = ImportContext.ConversionTransform;

	const FTransform ActorTransform = ConversionTransform*FTransform(ActorMtx)*ConversionTransform;

	if (ExistingActor && ExistingActorPolicy == EExistingActorPolicy::UpdateTransform)
	{
		ExistingActor->Modify();
		ModifiedActor = ExistingActor;

		ExistingActor->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
		ExistingActor->SetActorRelativeTransform(ActorTransform);

		bShouldSpawnNewActor = false;
	}
	else if (ExistingActor && ExistingActorPolicy == EExistingActorPolicy::Ignore)
	{
		// Ignore this actor and do nothing
		bShouldSpawnNewActor = false;
	}

	if (bShouldSpawnNewActor)
	{
		UActorFactory* ActorFactory = ImportContext.EmptyActorFactory;

		AActor* SpawnedActor = nullptr;

		// The asset which should be used to spawn the actor
		UObject* ActorAsset = nullptr;

		TArray<UObject*> ImportedAssets;

		// Note: an asset path on the actor is multually exclusive with importing geometry.
		if (SpawnData.AssetPath.IsEmpty() && SpawnData.AssetsToImport.Num() > 0)
		{
			ImportedAssets = USDImporter->ImportMeshes(ImportContext, SpawnData.AssetsToImport);

			// If there is more than one asset imported just use the first one.  This may be invalid if the actor only supports one mesh
			// so we will warn about that below if we can.
			ActorAsset = ImportedAssets.Num() > 0 ? ImportedAssets[0] : nullptr;
		}
		else if(!SpawnData.AssetPath.IsEmpty() && SpawnData.AssetsToImport.Num() > 0)
		{
			ImportContext.AddErrorMessage(EMessageSeverity::Warning,
				FText::Format(
					LOCTEXT("ConflictWithAssetPathAndMeshes", "Actor has an asset path '{0} but also contains meshes {1} to import. Meshes will be ignored"),
					FText::FromString(SpawnData.AssetPath),
					FText::AsNumber(SpawnData.AssetsToImport.Num())
				)
			);
		}

		if (!SpawnData.ActorClassName.IsEmpty())
		{
			TSubclassOf<AActor> ActorClass = FindActorClass(ImportContext, SpawnData);
			if (ActorClass)
			{
				SpawnedActor = ImportContext.World->SpawnActor<AActor>(ActorClass);
			}

		}
		else if (!SpawnData.AssetPath.IsEmpty())
		{
			ActorAsset = LoadObject<UObject>(nullptr, *SpawnData.AssetPath);
			if (!ActorAsset)
			{
				ImportContext.AddErrorMessage(
					EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindUnrealAssetPath", "Could not find Unreal Asset '{0}' for USD prim '{1}'"),
						FText::FromString(SpawnData.AssetPath),
						FText::FromString(USDToUnreal::ConvertString(SpawnData.ActorPrim->GetPrimPath()))));

				UE_LOG(LogUSDImport, Error, TEXT("Could not find Unreal Asset '%s' for USD prim '%s'"), *SpawnData.AssetPath, *SpawnData.ActorName.ToString());
			}
		}

		if (SpawnData.ActorClassName.IsEmpty() && ActorAsset)
		{
			UClass* AssetClass = ActorAsset->GetClass();

			UActorFactory* Factory = ImportContext.UsedFactories.FindRef(AssetClass);
			if (!Factory)
			{
				Factory = FActorFactoryAssetProxy::GetFactoryForAssetObject(ActorAsset);
				if (Factory)
				{
					ImportContext.UsedFactories.Add(AssetClass, Factory);
				}
				else
				{
					ImportContext.AddErrorMessage(
						EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindActorFactory", "Could not find an actor type to spawn for '{0}'"),
							FText::FromString(ActorAsset->GetName()))
					);
				}
			}

			ActorFactory = Factory;

		}

		if (!SpawnedActor && ActorFactory)
		{
			SpawnedActor = ActorFactory->CreateActor(ActorAsset, ImportContext.World->GetCurrentLevel(), FTransform::Identity, RF_Transactional, SpawnData.ActorName);

			// For empty group actors set their initial mobility to static 
			if (ActorFactory == ImportContext.EmptyActorFactory)
			{
				SpawnedActor->GetRootComponent()->SetMobility(EComponentMobility::Static);
			}

			if (ImportedAssets.Num() > 1)
			{
				// Multiple assets were found but factories only support creating from one asset so warn about this
				ImportContext.AddErrorMessage(
					EMessageSeverity::Warning, FText::Format(LOCTEXT("MultipleAssetsForASingleActor", "Actor type '{0}' only supports one asset but {1} assets were imported.   The first imported asset '{2}' was assigned to the actor"),
						FText::FromString(SpawnedActor->GetClass()->GetName()),
						FText::AsNumber(ImportedAssets.Num()),
						FText::FromString(ActorAsset ? ActorAsset->GetName() : TEXT("None"))
					)
				);
			}
		}

		if(SpawnedActor)
		{
			SpawnedActor->SetActorRelativeTransform(ActorTransform);

			if (SpawnData.AttachParentPrim && !bFlattenHierarchy)
			{
				// Spawned actor should be attached to a parent
				AActor* AttachPrim = PrimToActorMap.FindRef(SpawnData.AttachParentPrim);
				if (AttachPrim)
				{
					SpawnedActor->AttachToActor(AttachPrim, FAttachmentTransformRules::KeepRelativeTransform);
				}
			}

			FActorLabelUtilities::SetActorLabelUnique(SpawnedActor, SpawnData.ActorName.ToString(), &ImportContext.ActorLabels);
			ImportContext.ActorLabels.Add(SpawnedActor->GetActorLabel());
		}


		ModifiedActor = SpawnedActor;
	}

	PrimToActorMap.Add(SpawnData.ActorPrim, ModifiedActor);

	return ModifiedActor;
}


TSubclassOf<AActor> UUSDPrimResolver::FindActorClass(FUSDSceneImportContext& ImportContext, const FActorSpawnData& SpawnData) const
{
	TSubclassOf<AActor> ActorClass = nullptr;

	FString ActorClassName = SpawnData.ActorClassName;
	FName ActorClassFName = *ActorClassName;

	// Attempt to use the fully qualified path first.  If not use the expensive slow path.
	{
		ActorClass = LoadClass<AActor>(nullptr, *ActorClassName, nullptr);
	}

	if (!ActorClass)
	{
		UObject* TestObject = nullptr;
		TArray<FAssetData> AssetDatas;

		AssetRegistry->GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetDatas);

		UClass* TestClass = nullptr;
		for (const FAssetData& AssetData : AssetDatas)
		{
			if (AssetData.AssetName == ActorClassFName)
			{
				TestClass = Cast<UBlueprint>(AssetData.GetAsset())->GeneratedClass;
				break;
			}
		}

		if (TestClass && TestClass->IsChildOf<AActor>())
		{
			ActorClass = TestClass;
		}

		if (!ActorClass)
		{
			ImportContext.AddErrorMessage(
				EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindUnrealActorClass", "Could not find Unreal Actor Class '{0}' for USD prim '{1}'"),
					FText::FromString(ActorClassName),
					FText::FromString(USDToUnreal::ConvertString(SpawnData.ActorPrim->GetPrimPath()))));

		}
	}

	return ActorClass;
}

void UUSDPrimResolver::FindMeshChildren(FUsdImportContext& ImportContext, IUsdPrim* ParentPrim, bool bOnlyLODRoots, TArray<IUsdPrim*>& OutMeshChildren) const
{
	const FString PrimName = USDToUnreal::ConvertString(ParentPrim->GetPrimName());

	const FString KindName = USDToUnreal::ConvertString(ParentPrim->GetKind());

	const bool bHasUnrealAssetPath = ParentPrim->GetUnrealAssetPath() != nullptr;
	const bool bHasUnrealActorClass = ParentPrim->GetUnrealActorClass() != nullptr;

	const bool bIncludeLODs = bOnlyLODRoots;

	if(bOnlyLODRoots && ParentPrim->GetNumLODs() > 0)
	{
		// We're only looking for lod roots and this prim has LODs so add the prim and dont recurse into children
		OutMeshChildren.Add(ParentPrim);
	}
	else
	{
		if (ParentPrim->HasGeometryData())
		{
			OutMeshChildren.Add(ParentPrim);
		}

		const int32 NumChildren = ParentPrim->GetNumChildren();
		for (int32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
		{
			IUsdPrim* Child = ParentPrim->GetChild(ChildIdx);
			if (!Child->IsProxyOrGuide() && !Child->IsKindChildOf(USDKindTypes::Component))
			{
				FindMeshChildren(ImportContext, Child, bOnlyLODRoots, OutMeshChildren);
			}
		}
	}
}

void UUSDPrimResolver::FindActorsToSpawn_Recursive(FUSDSceneImportContext& ImportContext, IUsdPrim* Prim, IUsdPrim* ParentPrim, TArray<FActorSpawnData>& OutSpawnDatas) const
{
	TArray<FActorSpawnData>* SpawnDataArray = &OutSpawnDatas;

	UUSDSceneImportOptions* ImportOptions = Cast<UUSDSceneImportOptions>(ImportContext.ImportOptions);

	FActorSpawnData SpawnData;

	FString AssetPath;
	FName ActorClassName;
	if (Prim->HasTransform())
	{
		if (Prim->GetUnrealActorClass())
		{
			SpawnData.ActorClassName = USDToUnreal::ConvertString(Prim->GetUnrealActorClass());
		}

		if (Prim->GetUnrealAssetPath())
		{
			SpawnData.AssetPath = USDToUnreal::ConvertString(Prim->GetUnrealAssetPath());
		}

		FindMeshAssetsToImport(ImportContext, Prim, SpawnData.AssetsToImport, false);

		FName PrimName = USDToUnreal::ConvertName(Prim->GetPrimName());
		SpawnData.ActorName = PrimName;
		SpawnData.WorldTransform = USDToUnreal::ConvertMatrix(Prim->GetLocalToWorldTransform());
		SpawnData.AttachParentPrim = ParentPrim;
		SpawnData.ActorPrim = Prim;

		if (ImportOptions->ExistingActorPolicy == EExistingActorPolicy::Replace && ImportContext.ExistingActors.Contains(SpawnData.ActorName))
		{
			ImportContext.ActorsToDestroy.Add(SpawnData.ActorName);
		}

		OutSpawnDatas.Add(SpawnData);
	}

	if (!ImportContext.bFindUnrealAssetReferences || AssetPath.IsEmpty())
	{
		for (int32 ChildIdx = 0; ChildIdx < Prim->GetNumChildren(); ++ChildIdx)
		{
			IUsdPrim* Child = Prim->GetChild(ChildIdx);
			FindActorsToSpawn_Recursive(ImportContext, Child, Prim, *SpawnDataArray);
		}
	}
}


bool UUSDPrimResolver::IsValidPathForImporting(const FString& TestPath) const
{
	return FPackageName::GetPackageMountPoint(TestPath) != NAME_None;
}

#undef LOCTEXT_NAMESPACE