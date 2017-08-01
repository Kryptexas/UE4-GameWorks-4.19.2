// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "Factories/SceneImportFactory.h"
#include "USDImporter.h"
#include "Editor/EditorEngine.h"
#include "Factories/ImportSettings.h"
#include "USDSceneImportFactory.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUSDSceneImport, Log, All);

class UWorld;

struct FActorSpawnData
{
	FActorSpawnData()
		: Asset(nullptr)
		, CustomActorClass(nullptr)
		, Prim(nullptr)
	{}

	UActorFactory* ActorFactory;
	UObject* Asset;
	TSubclassOf<AActor> CustomActorClass;
	FName ActorName;
	FName InstanceName;
	IUsdPrim* Prim;

	TArray<FActorSpawnData> Children;
};

USTRUCT()
struct FUSDSceneImportContext : public FUsdImportContext
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UWorld* World;

	UPROPERTY()
	TMap<FName, AActor*> ExistingActors;

	UPROPERTY()
	TArray<FName> ActorsToDestroy;

	UPROPERTY()
	class UActorFactory* EmptyActorFactory;

	UPROPERTY()
	TMap<UClass*, UActorFactory*> UsedFactories;

	FCachedActorLabels ActorLabels;

	virtual void Init(UObject* InParent, const FString& InName, EObjectFlags InFlags, class IUsdStage* InStage);
};

UCLASS(transient)
class UUSDSceneImportFactory : public USceneImportFactory, public IImportSettingsParser
{
	GENERATED_UCLASS_BODY()

public:
	/** UFactory Interface */
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual void CleanUp() override;
	virtual IImportSettingsParser* GetImportSettingsParser() override { return this; }

	/** IImportSettingsParser interface */
	virtual void ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson) override;
private:

	void GenerateSpawnables(TArray<FActorSpawnData>& OutRootSpawnData, int32& OutTotalNumSpawnables);
	void InitSpawnData_Recursive(class IUsdPrim* Prim, TArray<FActorSpawnData>& OutSpawnDatas, int32& OutTotalNumSpawnables);
	void SpawnActors(const FActorSpawnData& SpawnData, AActor* AttachParent, int32 TotalNumSpawnables, FScopedSlowTask& SlowTask);
	void OnActorSpawned(AActor* SpawnedActor, const FActorSpawnData& SpawnData);
private:
	UPROPERTY()
	FUSDSceneImportContext ImportContext;

	UPROPERTY()
	UUSDSceneImportOptions* ImportOptions;
};
