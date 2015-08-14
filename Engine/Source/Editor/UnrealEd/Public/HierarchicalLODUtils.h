// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Engine/LODActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshUtilities.h"

#include "AsyncWork.h"

#if WITH_EDITOR
#include "AssetData.h"
#include "Factories/Factory.h"
#include "ObjectTools.h"
#endif // WITH_EDITOR

#define LOCTEXT_NAMESPACE "HierarchicalLODUtils"

namespace HierarchicalLODUtils
{
	/**
	* Recursively retrieves StaticMeshComponents from a LODActor and its child LODActors
	*
	* @param Actor - LODActor instance
	* @param InOutComponents - Will hold the StaticMeshComponents
	*/
	static void ExtractStaticMeshComponentsFromLODActor(AActor* Actor, TArray<UStaticMeshComponent*>& InOutComponents)
	{
		ALODActor* LODActor = CastChecked<ALODActor>(Actor);
		for (auto& ChildActor : LODActor->SubActors)
		{
			TArray<UStaticMeshComponent*> ChildComponents;
			if (ChildActor->IsA<ALODActor>())
			{
				ExtractStaticMeshComponentsFromLODActor(ChildActor, ChildComponents);
			}
			else
			{
				ChildActor->GetComponents<UStaticMeshComponent>(ChildComponents);
			}

			InOutComponents.Append(ChildComponents);
		}
	}


	/**
	* Builds a static mesh object for the given LODACtor
	*
	* @param LODActor -
	* @param Outer -
	* @return UStaticMesh*
	*/
	static bool BuildStaticMeshForLODActor(ALODActor* LODActor, UPackage* AssetsOuter, const FHierarchicalSimplification& LODSetup, const uint32 LODIndex)
	{
		if (AssetsOuter && LODActor)
		{
			if (!LODActor->IsDirty() || LODActor->SubActors.Num() < 2)
			{
				return true;
			}

			// Clean up sub objects (if applicable)
			for( auto& SubOject : LODActor->SubObjects)
			{
				if (SubOject)
				{
					SubOject->MarkPendingKill();
				}				
			}
			LODActor->SubObjects.Empty();

			TArray<UStaticMeshComponent*> AllComponents;
						
			{
				FScopedSlowTask SlowTask(LODActor->SubActors.Num(), (LOCTEXT("HierarchicalLODUtils_CollectStaticMeshes", "Collecting Static Meshes for Cluster")));
				SlowTask.MakeDialog();
				for (auto& Actor : LODActor->SubActors)
				{
					TArray<UStaticMeshComponent*> Components;

					if (Actor->IsA<ALODActor>())
					{
						ExtractStaticMeshComponentsFromLODActor(Actor, Components);
					}
					else
					{
						Actor->GetComponents<UStaticMeshComponent>(Components);
					}

					// TODO: support instanced static meshes
					Components.RemoveAll([](UStaticMeshComponent* Val){ return Val->IsA(UInstancedStaticMeshComponent::StaticClass()); });

					AllComponents.Append(Components);
					SlowTask.EnterProgressFrame(1);
				}
			}

			// it shouldn't even have come here if it didn't have any staticmesh
			if (ensure(AllComponents.Num() > 0))
			{
				FScopedSlowTask SlowTask(LODActor->SubActors.Num(), (LOCTEXT("HierarchicalLODUtils_MergingMeshes", "Merging Static Meshes and creating LODActor")));
				SlowTask.MakeDialog();

				// In case we don't have outer generated assets should have same path as LOD level
				const FString AssetsPath = AssetsOuter->GetName() + TEXT("/");
				AActor* FirstActor = LODActor->SubActors[0];

				TArray<UObject*> OutAssets;
				FVector OutProxyLocation = FVector::ZeroVector;
				UStaticMesh* MainMesh = nullptr;
			
				// Generate proxy mesh and proxy material assets
				IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
				// should give unique name, so use level + actor name
				const FString PackageName = FString::Printf(TEXT("LOD_%s"), *FirstActor->GetName());
				if (MeshUtilities.GetMeshMergingInterface() && LODSetup.bSimplifyMesh)
				{
					MeshUtilities.CreateProxyMesh(LODActor->SubActors, LODSetup.ProxySetting, AssetsOuter, PackageName, OutAssets, OutProxyLocation);
				}
				else
				{
					MeshUtilities.MergeStaticMeshComponents(AllComponents, FirstActor->GetWorld(), LODSetup.MergeSetting, AssetsOuter, PackageName, LODIndex, OutAssets, OutProxyLocation, LODSetup.DrawDistance, true);
				}

				// we make it private, so it can't be used by outside of map since it's useless, and then remove standalone
				for (auto& AssetIter : OutAssets)
				{
					AssetIter->ClearFlags(RF_Public | RF_Standalone);
				}

				// set staticmesh
				for (auto& Asset : OutAssets)
				{
					UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);

					if (StaticMesh)
					{
						MainMesh = StaticMesh;
					}
				}

				LODActor->SetStaticMesh(MainMesh);
				LODActor->SetActorLocation(OutProxyLocation);
				LODActor->SubObjects = OutAssets;				
		
				for (auto& Actor : LODActor->SubActors)
				{
					Actor->SetLODParent(LODActor->GetStaticMeshComponent(), LODActor->LODDrawDistance);
				}

				// Freshly build so mark not dirty
				LODActor->SetIsDirty(false);

				return true;
			}
		}
		return false;
	}


	static bool ShouldGenerateCluster(AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}

		if (Actor->bHidden)
		{
			return false;
		}

		if (!Actor->bEnableAutoLODGeneration)
		{
			return false;
		}

		ALODActor* LODActor = Cast<ALODActor>(Actor);
		if (LODActor)
		{			
			return false;			
		}

		FVector Origin, Extent;
		Actor->GetActorBounds(false, Origin, Extent);
		if (Extent.SizeSquared() <= 0.1)
		{
			return false;
		}

		// for now only consider staticmesh - I don't think skel mesh would work with simplygon merge right now @fixme
		TArray<UStaticMeshComponent*> Components;
		Actor->GetComponents<UStaticMeshComponent>(Components);
		// TODO: support instanced static meshes
		Components.RemoveAll([](UStaticMeshComponent* Val){ return Val->IsA(UInstancedStaticMeshComponent::StaticClass()); });

		int32 ValidComponentCount = 0;
		// now make sure you check parent primitive, so that we don't build for the actor that already has built. 
		if (Components.Num() > 0)
		{
			for (auto& ComponentIter : Components)
			{
				if (ComponentIter->GetLODParentPrimitive())
				{
					return false;					
				}

				if (ComponentIter->bHiddenInGame)
				{
					return false;
				}

				// see if we should generate it
				if (ComponentIter->ShouldGenerateAutoLOD())
				{
					++ValidComponentCount;
					break;
				}
			}
		}

		return (ValidComponentCount > 0);
	}

	static ALODActor* GetParentLODActor(const AActor* InActor)
	{
		ALODActor* ParentActor = nullptr;
		if (InActor)
		{
			TArray<UStaticMeshComponent*> ComponentArray;
			InActor->GetComponents<UStaticMeshComponent>(ComponentArray);
			for (auto Component : ComponentArray)
			{
				UPrimitiveComponent* ParentComponent = Component->GetLODParentPrimitive();
				if (ParentComponent)
				{
					ParentActor = CastChecked<ALODActor>(ParentComponent->GetOwner());
					break;
				}
			}
		}

		return ParentActor;		
	}

	static void DeleteLODActor(ALODActor* InActor)
	{
		// Find if it has a parent ALODActor
		AActor* Actor = CastChecked<AActor>(InActor);

		ALODActor* ParentLOD = GetParentLODActor(Actor);
		if (ParentLOD)
		{
			ParentLOD->RemoveSubActor(Actor);
		}

		// Clean out sub actors and update their LODParent
		while (InActor->SubActors.Num())
		{
			auto SubActor = InActor->SubActors[0];
			InActor->RemoveSubActor(SubActor);
		}

		// you still have to delete all objects just in case they had it and didn't want it anymore
		TArray<UObject*> AssetsToDelete;
		
		AssetsToDelete.Append(InActor->SubObjects);
		
		for (auto& Asset : AssetsToDelete)
		{
			if (Asset)
			{
				Asset->MarkPendingKill();
				ObjectTools::DeleteSingleObject(Asset, false);
			}			
		}

		// Rebuild streaming data to prevent invalid references
		ULevel::BuildStreamingData(InActor->GetWorld(), InActor->GetLevel());

		// garbage collect
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
	}

	static void DeleteLODActorAssets(ALODActor* InActor)
	{
		// you still have to delete all objects just in case they had it and didn't want it anymore
		TArray<UObject*> AssetsToDelete;

		AssetsToDelete.Append(InActor->SubObjects);
		InActor->SubObjects.Empty();
		for (auto& Asset : AssetsToDelete)
		{
			if (Asset)
			{
				Asset->MarkPendingKill();
				ObjectTools::DeleteSingleObject(Asset, false);
			}
		}

		InActor->GetStaticMeshComponent()->StaticMesh = nullptr;

		// Mark render state dirty
		InActor->GetStaticMeshComponent()->MarkRenderStateDirty();	

		// Rebuild streaming data to prevent invalid references
		ULevel::BuildStreamingData(InActor->GetWorld(), InActor->GetLevel());
		
		// Garbage collecting
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
	}

	static ALODActor* CreateNewClusterActor(UWorld* InWorld, const int32 InLODLevel)
	{
		// Check incoming data
		check(InWorld != nullptr && InLODLevel >= 0);
		auto WorldSettings = InWorld->GetWorldSettings();
		check(WorldSettings->bEnableHierarchicalLODSystem && WorldSettings->HierarchicalLODSetup.Num() > InLODLevel);
					
		// Spawn and setup actor
		ALODActor* NewActor = nullptr;
		NewActor = InWorld->SpawnActor<ALODActor>(ALODActor::StaticClass(), FTransform());
		NewActor->LODLevel = InLODLevel + 1;
		NewActor->LODDrawDistance = WorldSettings->HierarchicalLODSetup[InLODLevel].DrawDistance;
		NewActor->SetStaticMesh(nullptr);

		return NewActor;		
	}

	static void DeleteLODActorsInHLODLevel(UWorld* InWorld, const int32 HLODLevelIndex)
	{
		// you still have to delete all objects just in case they had it and didn't want it anymore
		TArray<UObject*> AssetsToDelete;
		for (int32 ActorId = InWorld->PersistentLevel->Actors.Num() - 1; ActorId >= 0; --ActorId)
		{
			ALODActor* LodActor = Cast<ALODActor>(InWorld->PersistentLevel->Actors[ActorId]);
			if (LodActor && LodActor->LODLevel == ( HLODLevelIndex + 1))
			{
				DeleteLODActor(LodActor);
				InWorld->DestroyActor(LodActor);
			}
		}
	}

	
	static void TestBuild(ALODActor* LODActor, UPackage* AssetsOuter, const FHierarchicalSimplification& LODSetup, const uint32 LODIndex)
	{

	}

};

#undef LOCTEXT_NAMESPACE