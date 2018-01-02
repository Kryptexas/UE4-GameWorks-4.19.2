// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraWorldManager.h"
#include "NiagaraModule.h"
#include "Modules/ModuleManager.h"
#include "NiagaraTypes.h"
#include "NiagaraEvents.h"
#include "NiagaraSettings.h"
#include "NiagaraParameterCollection.h"
#include "NiagaraSystemInstance.h"
#include "Scalability.h"
#include "ConfigCacheIni.h"
#include "NiagaraDataInterfaceSkeletalMesh.h"


FNiagaraWorldManager* FNiagaraWorldManager::Get(UWorld* World)
{
	INiagaraModule& NiagaraModule = FModuleManager::LoadModuleChecked<INiagaraModule>("Niagara");
	return NiagaraModule.GetWorldManager(World);
}

void FNiagaraWorldManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(ParameterCollections);
}

UNiagaraParameterCollectionInstance* FNiagaraWorldManager::GetParameterCollection(UNiagaraParameterCollection* Collection)
{
	if (!Collection)
	{
		return nullptr;
	}

	UNiagaraParameterCollectionInstance** OverrideInst = ParameterCollections.Find(Collection);
	if (!OverrideInst)
	{
		UNiagaraParameterCollectionInstance* DefaultInstance = Collection->GetDefaultInstance();
		OverrideInst = &ParameterCollections.Add(Collection);
		*OverrideInst = CastChecked<UNiagaraParameterCollectionInstance>(StaticDuplicateObject(DefaultInstance, World));
#if WITH_EDITORONLY_DATA
		//Bind to the default instance so that changes to the collection propagate through.
		DefaultInstance->GetParameterStore().Bind(&(*OverrideInst)->GetParameterStore());
#endif
	}

	check(OverrideInst && *OverrideInst);
	return *OverrideInst;
}

void FNiagaraWorldManager::SetParameterCollection(UNiagaraParameterCollectionInstance* NewInstance)
{
	check(NewInstance);
	if (NewInstance)
	{
		UNiagaraParameterCollection* Collection = NewInstance->GetParent();
		UNiagaraParameterCollectionInstance** OverrideInst = ParameterCollections.Find(Collection);
		if (!OverrideInst)
		{
			OverrideInst = &ParameterCollections.Add(Collection);
		}
		else
		{
			if (*OverrideInst && NewInstance)
			{
				UNiagaraParameterCollectionInstance* DefaultInstance = Collection->GetDefaultInstance();
				//Need to transfer existing bindings from old instance to new one.
				FNiagaraParameterStore& ExistingStore = (*OverrideInst)->GetParameterStore();
				FNiagaraParameterStore& NewStore = NewInstance->GetParameterStore();

				ExistingStore.TransferBindings(NewStore);

#if WITH_EDITOR
				//If the existing store was this world's duplicate of the default then we must be sure it's unbound.
				DefaultInstance->GetParameterStore().Unbind(&ExistingStore);
#endif
			}
		}

		*OverrideInst = NewInstance;
	}
}

void FNiagaraWorldManager::CleanupParameterCollections()
{
#if WITH_EDITOR
	for (TPair<UNiagaraParameterCollection*, UNiagaraParameterCollectionInstance*> CollectionInstPair : ParameterCollections)
	{
		UNiagaraParameterCollection* Collection = CollectionInstPair.Key;
		UNiagaraParameterCollectionInstance* CollectionInst = CollectionInstPair.Value;
		//Ensure that the default instance is not bound to the override.
		UNiagaraParameterCollectionInstance* DefaultInst = Collection->GetDefaultInstance();
		DefaultInst->GetParameterStore().Unbind(&CollectionInst->GetParameterStore());
	}
#endif
	ParameterCollections.Empty();
}

TSharedRef<FNiagaraSystemSimulation, ESPMode::ThreadSafe> FNiagaraWorldManager::GetSystemSimulation(UNiagaraSystem* System)
{
	TSharedRef<FNiagaraSystemSimulation, ESPMode::ThreadSafe>* SimPtr = SystemSimulations.Find(System);
	if (SimPtr != nullptr)
	{
		return *SimPtr;
	}
	
	TSharedRef<FNiagaraSystemSimulation, ESPMode::ThreadSafe> Sim = MakeShared<FNiagaraSystemSimulation, ESPMode::ThreadSafe>();
	SystemSimulations.Add(System, Sim);
	Sim->Init(System, World);
	return Sim;
}

void FNiagaraWorldManager::DestroySystemSimulation(UNiagaraSystem* System)
{
	TSharedRef<FNiagaraSystemSimulation, ESPMode::ThreadSafe>* Sim = SystemSimulations.Find(System);
	if (Sim)
	{
		(*Sim)->Destroy();
		SystemSimulations.Remove(System);
	}	
}

void FNiagaraWorldManager::OnWorldCleanup(bool bSessionEnded, bool bCleanupResources)
{
	for (TPair<UNiagaraSystem*, TSharedRef<FNiagaraSystemSimulation, ESPMode::ThreadSafe>>& SimPair : SystemSimulations)
	{
		SimPair.Value->Destroy();
	}
	SystemSimulations.Empty();
	CleanupParameterCollections();
}

void FNiagaraWorldManager::Tick(float DeltaSeconds)
{
/*
Some experimental tinkering with links to effects quality.
Going off this idea tbh

	int32 CurrentEffectsQuality = Scalability::GetEffectsQualityDirect(true);
	if (CachedEffectsQuality != CurrentEffectsQuality)
	{
		CachedEffectsQuality = CurrentEffectsQuality;
		FString SectionName = FString::Printf(TEXT("%s@%d"), TEXT("EffectsQuality"), CurrentEffectsQuality);
		if (FConfigFile* File = GConfig->FindConfigFileWithBaseName(TEXT("Niagara")))
		{
			FString ScalabilityCollectionString;			
			if (File->GetString(*SectionName, TEXT("ScalabilityCollection"), ScalabilityCollectionString))
			{
				//NewLandscape_Material = LoadObject<UMaterialInterface>(NULL, *NewLandsfgcapeMaterialName, NULL, LOAD_NoWarn);
				UNiagaraParameterCollectionInstance* ScalabilityCollection = LoadObject<UNiagaraParameterCollectionInstance>(NULL, *ScalabilityCollectionString, NULL, LOAD_NoWarn);
				if (ScalabilityCollection)
				{
					SetParameterCollection(ScalabilityCollection);
				}
			}
		}
	}
*/
	SkeletalMeshGeneratedData.TickGeneratedData(DeltaSeconds);

	//Tick our collections to push any changes to bound stores.
	for (TPair<UNiagaraParameterCollection*, UNiagaraParameterCollectionInstance*> CollectionInstPair : ParameterCollections)
	{
		check(CollectionInstPair.Value);
		CollectionInstPair.Value->Tick();
	}

	//Now tick all system instances. 
	TArray<UNiagaraSystem*> DeadSystems;
	for (TPair<UNiagaraSystem*, TSharedRef<FNiagaraSystemSimulation, ESPMode::ThreadSafe>>& SystemSim : SystemSimulations)
	{
		if (SystemSim.Value->Tick(DeltaSeconds) == false)
		{
			DeadSystems.Add(SystemSim.Key);
		}
	}

	for (UNiagaraSystem* DeadSystem : DeadSystems)
	{
		SystemSimulations.Remove(DeadSystem);
	}
}