// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "USDSceneImportFactory.h"
#include "USDImportOptions.h"
#include "ActorFactories/ActorFactoryEmptyActor.h"
#include "Engine/Selection.h"
#include "ScopedTransaction.h"
#include "ILayers.h"
#include "IAssetRegistry.h"
#include "AssetRegistryModule.h"
#include "IUSDImporterModule.h"
#include "USDConversionUtils.h"
#include "ObjectTools.h"
#include "ScopedSlowTask.h"
#include "PackageTools.h"
#include "Editor.h"
#include "PropertySetter.h"
#include "AssetSelection.h"
#include "JsonObjectConverter.h"

#define LOCTEXT_NAMESPACE "USDImportPlugin"

UUSDSceneImportFactory::UUSDSceneImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UWorld::StaticClass();

	bEditorImport = true;
	bText = false;

	ImportOptions = ObjectInitializer.CreateDefaultSubobject<UUSDSceneImportOptions>(this, TEXT("USDSceneImportOptions"));

	Formats.Add(TEXT("usd;Universal Scene Descriptor files"));
	Formats.Add(TEXT("usda;Universal Scene Descriptor files"));
	Formats.Add(TEXT("usdc;Universal Scene Descriptor files"));
}

UObject* UUSDSceneImportFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UUSDImporter* USDImporter = IUSDImporterModule::Get().GetImporter();

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TArray<UObject*> AllAssets;

	if(IsAutomatedImport() || USDImporter->ShowImportOptions(*ImportOptions))
	{
		// @todo: Disabled.  This messes with the ability to replace existing actors since actors with this name could still be in the transaction buffer
		//FScopedTransaction ImportUSDScene(LOCTEXT("ImportUSDSceneTransaction", "Import USD Scene"));

		IUsdStage* Stage = USDImporter->ReadUSDFile(ImportContext, Filename);
		if (Stage)
		{
			IUsdPrim* RootPrim = Stage->GetRootPrim();

			ImportContext.Init(InParent, InName.ToString(), Flags, Stage);
			ImportContext.ImportOptions = ImportOptions;
			ImportContext.ImportPathName = ImportOptions->PathForAssets.Path;
	
			// Actors will have the transform
			ImportContext.bApplyWorldTransformToGeometry = false;

			TArray<FUsdPrimToImport> PrimsToImport;
			USDImporter->FindPrimsToImport(ImportContext, PrimsToImport);

			//Find Meshes to import
			struct FPrimToAssetName
			{
				FUsdPrimToImport PrimData;
				FString UniqueAssetName;
			};

			TMap<FString, TArray<FPrimToAssetName>> ExistingNamesToCount;


			for (const FUsdPrimToImport& PrimData : PrimsToImport)
			{
				FString MeshName = USDToUnreal::ConvertString(PrimData.Prim->GetPrimName());

				// Make unique names
				TArray<FPrimToAssetName>& ExistingPrimsWithName = ExistingNamesToCount.FindOrAdd(MeshName);

				int32 ExistingCount = ExistingPrimsWithName.Num();
				if (ExistingCount)
				{
					MeshName += TEXT("_");
					MeshName.AppendInt(ExistingCount);
				}

				MeshName = ObjectTools::SanitizeObjectName(MeshName);

				FPrimToAssetName PrimToAssetName;
				PrimToAssetName.PrimData = PrimData;
				PrimToAssetName.UniqueAssetName = MeshName;

				ExistingPrimsWithName.Add(PrimToAssetName);
			}

			FScopedSlowTask SlowTask(2.0f, LOCTEXT("ImportingUSDMeshes", "Importing USD Meshes"));
			SlowTask.Visibility = ESlowTaskVisibility::ForceVisible;
			int32 MeshCount = 0;


			for (auto It = ExistingNamesToCount.CreateConstIterator(); It; ++It)
			{
				const FString RawMeshName = It.Key();
				const TArray<FPrimToAssetName>& PrimToAssetNames = It.Value();

				UObject* Asset = nullptr;
				int32 Count = PrimToAssetNames.Num();

				for(const FPrimToAssetName& PrimToAssetName : PrimToAssetNames)
				{
					FString FullPath;
					if (!ImportOptions->bGenerateUniqueMeshes)
					{
						FullPath = ImportContext.ImportPathName / RawMeshName;
					}
					else if (ImportOptions->bGenerateUniquePathPerUSDPrim)
					{
						FString USDPath = USDToUnreal::ConvertString(PrimToAssetName.PrimData.Prim->GetPrimPath());
						USDPath.RemoveFromEnd(RawMeshName);
						FullPath = ImportContext.ImportPathName / USDPath / RawMeshName;
					}
					else
					{
						FullPath = ImportContext.ImportPathName / PrimToAssetName.UniqueAssetName;
					}

					if (AssetRegistry.IsLoadingAssets())
					{
						Asset = LoadObject<UObject>(nullptr, *FullPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
					}
					else
					{
						// Use the asset registry if it is available for faster searching
						FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(*FullPath);
						Asset = AssetData.GetAsset();
					}

					const bool bImportAsset =
						ImportOptions->bImportMeshes &&
						(!Asset || ImportOptions->ExistingAssetPolicy == EExistingAssetPolicy::Reimport);

					SlowTask.EnterProgressFrame(1.0f / PrimsToImport.Num(), FText::Format(LOCTEXT("ImportingUSDMesh", "Importing Mesh {0} of {1}"), MeshCount + 1, PrimsToImport.Num()));

					if (bImportAsset)
					{
						FString NewPackageName = PackageTools::SanitizePackageName(FullPath);
						UPackage* Package = CreatePackage(nullptr, *NewPackageName);
						Package->FullyLoad();

						ImportContext.Parent = Package;
						ImportContext.ObjectName = FPackageName::GetLongPackageAssetName(Package->GetOutermost()->GetName());

						Asset = USDImporter->ImportSingleMesh(ImportContext, ImportOptions->MeshImportType, PrimToAssetName.PrimData);

						if(Asset)
						{
							FAssetRegistryModule::AssetCreated(Asset);
							Package->MarkPackageDirty();
						}
					}
	
					if(Asset)
					{
						ImportContext.PrimToAssetMap.Add(PrimToAssetName.PrimData.Prim, Asset);
					}

					++MeshCount;
				}
			}


			// Important: Actors should not be created with RF_Public or RF_Standalone so remove them now.   We only want transactional for actors.
			ImportContext.ObjectFlags = RF_Transactional;

			EExistingActorPolicy ExistingActorPolicy = ImportOptions->ExistingActorPolicy;

			TArray<FActorSpawnData> RootSpawnDatas;

			int32 TotalNumSpawnables = 0;
			GenerateSpawnables(RootSpawnDatas, TotalNumSpawnables);

			// We need to check here for any actors that exist that need to be deleted before we continue (they are getting replaced)
			{
				bool bDeletedActors = false;

				USelection* ActorSelection = GEditor->GetSelectedActors();
				ActorSelection->BeginBatchSelectOperation();

				if (ExistingActorPolicy == EExistingActorPolicy::Replace && ImportContext.ActorsToDestroy.Num())
				{
					for (FName ExistingActorName : ImportContext.ActorsToDestroy)
					{
						AActor* ExistingActor = ImportContext.ExistingActors.FindAndRemoveChecked(ExistingActorName);
						if (ExistingActor)
						{
							bDeletedActors = true;
							if (ExistingActor->IsSelected())
							{
								GEditor->SelectActor(ExistingActor, false, false);
							}
							ImportContext.World->DestroyActor(ExistingActor);
						}
					}
				}

				ActorSelection->EndBatchSelectOperation();

				if (!IsAutomatedImport())
				{
					GEditor->NoteSelectionChange();
				}

				if (bDeletedActors)
				{
					// We need to make sure the actors are really gone before we start replacing them
					CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
				}
			}

			// Refresh actor labels as we deleted actors which were cached
			ULevel* CurrentLevel = ImportContext.World->GetCurrentLevel();
			check(CurrentLevel);

			for (AActor* Actor : CurrentLevel->Actors)
			{
				if (Actor)
				{
					ImportContext.ActorLabels.Add(Actor->GetActorLabel());
				}
			}

			for (const FActorSpawnData& SpawnData : RootSpawnDatas)
			{
				SpawnActors(SpawnData, nullptr, TotalNumSpawnables, SlowTask);
			}

		}

		FEditorDelegates::OnAssetPostImport.Broadcast(this, ImportContext.World);

		ImportContext.DisplayErrorMessages(IsAutomatedImport());

		return ImportContext.World;
	}
	else
	{
		bOutOperationCanceled = true;
		return nullptr;
	}
}

bool UUSDSceneImportFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	if (Extension == TEXT("usd") || Extension == TEXT("usda") || Extension == TEXT("usdc"))
	{
		return true;
	}

	return false;
}

void UUSDSceneImportFactory::CleanUp()
{
	ImportContext = FUSDSceneImportContext();
	UnrealUSDWrapper::CleanUp();
}

void UUSDSceneImportFactory::ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson)
{
	FJsonObjectConverter::JsonObjectToUStruct(ImportSettingsJson, ImportOptions->GetClass(), ImportOptions, 0, CPF_InstancedReference);
}

void UUSDSceneImportFactory::GenerateSpawnables(TArray<FActorSpawnData>& OutRootSpawnDatas, int32& OutTotalNumSpawnables)
{
	IUsdPrim* RootPrim = ImportContext.RootPrim;

	if(RootPrim->HasTransform())
	{
		InitSpawnData_Recursive(RootPrim, OutRootSpawnDatas, OutTotalNumSpawnables);
	}
	else
	{
		for (int32 ChildIdx = 0; ChildIdx < RootPrim->GetNumChildren(); ++ChildIdx)
		{
			IUsdPrim* Child = RootPrim->GetChild(ChildIdx);

			InitSpawnData_Recursive(Child, OutRootSpawnDatas, OutTotalNumSpawnables);
		}
	}
}

void UUSDSceneImportFactory::SpawnActors(const FActorSpawnData& SpawnData, AActor* AttachParent, int32 TotalNumSpawnables, FScopedSlowTask& SlowTask)
{
	SlowTask.EnterProgressFrame(1.0f / TotalNumSpawnables, LOCTEXT("SpawningActors", "Spawning Actors"));

	const bool bFlattenHierarchy = ImportOptions->bFlattenHierarchy;

	AActor* ModifiedActor = nullptr;

	// Look for an existing actor and decide what to do based on the users choice
	AActor* ExistingActor = ImportContext.ExistingActors.FindRef(SpawnData.ActorName);

	bool bShouldSpawnNewActor = true;
	EExistingActorPolicy ExistingActorPolicy = ImportOptions->ExistingActorPolicy;

	const FMatrix ActorMtx = USDToUnreal::ConvertMatrix(bFlattenHierarchy ? SpawnData.Prim->GetLocalToWorldTransform() : SpawnData.Prim->GetLocalToParentTransform());

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

	bShouldSpawnNewActor &= (SpawnData.Asset || SpawnData.CustomActorClass || !bFlattenHierarchy);

	if (bShouldSpawnNewActor)
	{
		UActorFactory* Factory = SpawnData.ActorFactory;

		AActor* SpawnedActor = nullptr;
		if (SpawnData.CustomActorClass)
		{
			SpawnedActor = ImportContext.World->SpawnActor<AActor>(SpawnData.CustomActorClass);
		}
		else
		{
			SpawnedActor = Factory->CreateActor(SpawnData.Asset, ImportContext.World->GetCurrentLevel(), FTransform::Identity, ImportContext.ObjectFlags, SpawnData.ActorName);
		}

		if (AttachParent)
		{
			SpawnedActor->AttachToActor(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		SpawnedActor->SetActorRelativeTransform(ActorTransform);

		FActorLabelUtilities::SetActorLabelUnique(SpawnedActor, SpawnData.ActorName.ToString(), &ImportContext.ActorLabels);
		ImportContext.ActorLabels.Add(SpawnedActor->GetActorLabel());

		OnActorSpawned(SpawnedActor, SpawnData);

		ModifiedActor = SpawnedActor;
	}

	for (const FActorSpawnData& ChildData : SpawnData.Children)
	{
		SpawnActors(ChildData, !bFlattenHierarchy ? ModifiedActor : nullptr, TotalNumSpawnables, SlowTask);
	}

}

void UUSDSceneImportFactory::InitSpawnData_Recursive(IUsdPrim* Prim, TArray<FActorSpawnData>& OutSpawnDatas, int32& OutTotalNumSpawnables)
{
	TArray<FActorSpawnData>* SpawnDataArray = &OutSpawnDatas;

	FString AssetPath;
	FName ActorClassName;
	if(Prim->HasTransform())
	{
		FActorSpawnData SpawnData;

		SpawnData.ActorFactory = ImportContext.EmptyActorFactory;

		if (Prim->GetUnrealActorClass())
		{
			ActorClassName = USDToUnreal::ConvertName(Prim->GetUnrealActorClass());
		}

		if (Prim->GetUnrealAssetPath())
		{
			AssetPath = USDToUnreal::ConvertString(Prim->GetUnrealAssetPath());
		}

		SpawnData.Prim = Prim;

		// Spawn a custom actor class (may not have an associated actor factory)
		if (ActorClassName != NAME_None)
		{
			TSubclassOf<AActor> ActorClass = nullptr;

			// Attempt to use the fully qualified path first.  If not use the expensive slow path.
			{
				ActorClass = LoadClass<AActor>(nullptr, *ActorClassName.ToString(), nullptr);
			}

			if(!ActorClass)
			{
				IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

				UObject* TestObject = nullptr;
				TArray<FAssetData> AssetDatas;

				AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetDatas);
				
				UClass* TestClass = nullptr;
				for (const FAssetData& AssetData : AssetDatas)
				{
					if (AssetData.AssetName == ActorClassName)
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
							FText::FromName(ActorClassName),
							FText::FromString(USDToUnreal::ConvertString(Prim->GetPrimPath()))));

				}
			}

			SpawnData.CustomActorClass = ActorClass;

		}
		else if (!AssetPath.IsEmpty())
		{
			SpawnData.Asset = LoadObject<UObject>(nullptr, *AssetPath);
			if (!SpawnData.Asset)
			{
				ImportContext.AddErrorMessage(
					EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindUnrealAssetPath", "Could not find Unreal Asset '{0}' for USD prim '{1}'"), 
					FText::FromString(AssetPath), 
					FText::FromString(USDToUnreal::ConvertString(Prim->GetPrimPath()))));

				UE_LOG(LogUSDImport, Error, TEXT("Could not find Unreal Asset '%s' for USD prim '%s'"), *AssetPath, *SpawnData.ActorName.ToString());
			}
			
		}

		if (!SpawnData.Asset)
		{
			SpawnData.Asset = ImportContext.PrimToAssetMap.FindRef(Prim);
		}

		if (SpawnData.Asset)
		{
			UClass* AssetClass = SpawnData.Asset->GetClass();

			// Find an actor factory to use to spawn this asset.  Cache it off so we are not constantly searching through the entire factory list
			// likely event that the usd file has more than one of the same type of actor
			UActorFactory* Factory = ImportContext.UsedFactories.FindRef(AssetClass);
			if (!Factory)
			{
				Factory = FActorFactoryAssetProxy::GetFactoryForAssetObject(SpawnData.Asset);
				if (Factory)
				{
					ImportContext.UsedFactories.Add(AssetClass, Factory);
				}
				else
				{
					ImportContext.AddErrorMessage(
						EMessageSeverity::Error, FText::Format(LOCTEXT("CouldNotFindActorFactory", "Could not find an actor type to spawn for '{0}'"),
							FText::FromString(SpawnData.Asset->GetName()))
					);

					Factory = ImportContext.EmptyActorFactory;
				}
			}

			SpawnData.ActorFactory = Factory;
		}

		FName PrimName = USDToUnreal::ConvertName(Prim->GetPrimName());
		SpawnData.ActorName = PrimName;

		if (ImportOptions->ExistingActorPolicy == EExistingActorPolicy::Replace && ImportContext.ExistingActors.Contains(SpawnData.ActorName))
		{
			ImportContext.ActorsToDestroy.Add(SpawnData.ActorName);
		}

		int32 Index = OutSpawnDatas.Add(SpawnData);

		SpawnDataArray = &OutSpawnDatas[Index].Children;

		++OutTotalNumSpawnables;
	}

	if(!ImportContext.bFindUnrealAssetReferences || AssetPath.IsEmpty())
	{
		for (int32 ChildIdx = 0; ChildIdx < Prim->GetNumChildren(); ++ChildIdx)
		{
			IUsdPrim* Child = Prim->GetChild(ChildIdx);

			InitSpawnData_Recursive(Child, *SpawnDataArray, OutTotalNumSpawnables);
		}
	}
}

void UUSDSceneImportFactory::OnActorSpawned(AActor* SpawnedActor, const FActorSpawnData& SpawnData)
{
	if(Cast<UUSDSceneImportOptions>(ImportContext.ImportOptions)->bImportProperties)
	{
		FUSDPropertySetter PropertySetter(ImportContext);

		PropertySetter.ApplyPropertiesToActor(SpawnedActor, SpawnData.Prim, TEXT(""));
	}
}

void FUSDSceneImportContext::Init(UObject* InParent, const FString& InName, EObjectFlags InFlags, IUsdStage* InStage)
{
	FUsdImportContext::Init(InParent, InName, InFlags, InStage);

	World = GEditor->GetEditorWorldContext().World();

	ULevel* CurrentLevel = World->GetCurrentLevel();
	check(CurrentLevel);

	for (AActor* Actor : CurrentLevel->Actors)
	{
		if (Actor)
		{
			ExistingActors.Add(Actor->GetFName(), Actor);
		}
	}


	UActorFactoryEmptyActor* NewEmptyActorFactory = NewObject<UActorFactoryEmptyActor>();
	// Do not create sprites for empty actors.  These will likely just be parents of mesh actors;
	NewEmptyActorFactory->bVisualizeActor = false;

	EmptyActorFactory = NewEmptyActorFactory;

	bFindUnrealAssetReferences = true;
}


#undef LOCTEXT_NAMESPACE
