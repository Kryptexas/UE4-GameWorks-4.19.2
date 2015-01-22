// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	World.cpp: UWorld implementation
=============================================================================*/

#include "UnrealEd.h"
#include "HierarchicalLOD.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Engine/LODActor.h"
#include "Editor/UnrealEd/Classes/Factories/Factory.h"
#include "MeshUtilities.h"
#include "ObjectTools.h"

#endif // WITH_EDITOR

DEFINE_LOG_CATEGORY_STATIC(LogLODGenerator, Log, All);

#define LOCTEXT_NAMESPACE "HierarchicalLOD"

/*-----------------------------------------------------------------------------
	HierarchicalLOD implementation.
-----------------------------------------------------------------------------*/

////////////////////////////////////////////////////////////////////////////////////////////
// Hierarchical LOD System implementation 
////////////////////////////////////////////////////////////////////////////////////////////

// utility functions
float CalculateFillingFactor(const FBox& ABox, const float AFillingFactor, const FBox& BBox, const float BFillingFactor)
{
	FBox OverlapBox = ABox.Overlap(BBox);
	FBox UnionBox = ABox + BBox;
	// it shouldn't be zero or it should be checked outside
	ensure(UnionBox.GetSize() != FVector::ZeroVector);

	// http://deim.urv.cat/~rivi/pub/3d/icra04b.pdf
	// cost is calculated based on r^3 / filling factor
	// since it subtract by AFillingFactor * 1/2 overlap volume + BfillingFactor * 1/2 overlap volume
	return (AFillingFactor * ABox.GetVolume() + BFillingFactor * BBox.GetVolume() - 0.5f * (AFillingFactor+BFillingFactor) * OverlapBox.GetVolume()) / UnionBox.GetVolume();
}

float GetDistance(float AMin, float AMax, float BMin, float BMax)
{
	if(AMin > BMax)
	{
		return (AMin-BMax);
	}

	if(BMin > AMax)
	{
		return (BMin-AMax);
	}

	return 0.f;
}
// utility functions
float CalculateDistance(const FLODCluster& A, const FLODCluster& B)
{
	FVector AMax = A.Bound.Max;
	FVector AMin = A.Bound.Min;
	FVector BMax = B.Bound.Max;
	FVector BMin = B.Bound.Min;
	FVector DistanceVector = FVector::ZeroVector;

	DistanceVector.X = GetDistance(AMin.X, AMax.X, BMin.X, BMax.X);
	DistanceVector.Y = GetDistance(AMin.Y, AMax.Y, BMin.Y, BMax.Y);
	DistanceVector.Z = GetDistance(AMin.Z, AMax.Z, BMin.Z, BMax.Z);

	return DistanceVector.SizeSquared();
}

////////////////////////////////////////////////////////////////////////////////////////////
// Hierarchical LOD Builder System implementation 
////////////////////////////////////////////////////////////////////////////////////////////

FHierarchicalLODBuilder::FHierarchicalLODBuilder(class UWorld* InWorld)
:	World(InWorld)
{}

// build hierarchical cluster
void FHierarchicalLODBuilder::Build()
{
	check (World);

	const TArray<class ULevel*>& Levels = World->GetLevels();

	for (const auto& LevelIter : Levels)
	{
		BuildClusters(LevelIter);
	}
}


// for now it only builds per level, it turns to one actor at the end
void FHierarchicalLODBuilder::BuildClusters(class ULevel* InLevel)
{
	// you still have to delete all objects just in case they had it and didn't want it anymore
	TArray<UObject*> AssetsToDelete;
	for (int32 ActorId=InLevel->Actors.Num()-1; ActorId >= 0; --ActorId)
	{
		ALODActor* LodActor = Cast<ALODActor>(InLevel->Actors[ActorId]);
		if (LodActor)
		{
			for (auto& Asset: LodActor->SubObjects)
			{
				AssetsToDelete.Add(Asset);
			}
			World->DestroyActor(LodActor);
		}
	}

	for (auto& Asset : AssetsToDelete)
	{
		ObjectTools::DeleteSingleObject(Asset, false);
	}

	// only build if it's enabled
	if(InLevel->GetWorld()->GetWorldSettings()->bEnableHierarchicalLODSystem)
	{
		const int32 TotalNumLOD = InLevel->GetWorld()->GetWorldSettings()->HierarchicalLODSetup.Num();

		for(int32 LODId=0; LODId<TotalNumLOD; ++LODId)
		{
			// initialize Clusters
			InitializeClusters(InLevel, LODId);

			// now we have all pair of nodes
			TArray<FLODCluster> MSTClusters;
			FindMST(MSTClusters);
			// replace Clusters to new culled cluster
			Clusters = MSTClusters;

			// now we have to calculate merge
			MergeClusters(InLevel, LODId);
		}
	}
}

void FHierarchicalLODBuilder::FindMST(TArray<FLODCluster>& OutClusters) 
{
	if (Clusters.Num() > 0)
	{
		// now sort edge in the order of weight
		struct FCompareCluster
		{
			FORCEINLINE bool operator()(const FLODCluster& A, const FLODCluster& B) const
			{
				return (A.GetCost() < B.GetCost());
			}
		};

		Clusters.Sort(FCompareCluster());

		// now start from small to large, find the MST
		OutClusters.Empty();

		TArray<class AActor*> Processed;
		for(auto& Cluster: Clusters)
		{
			if (Cluster.IsValid())
			{
				bool bNeedToAdd=false;

				TArray<class AActor*>& Actors=Cluster.Actors;
				for(auto& Actor : Actors)
				{
					if(!Processed.Contains(Actor))
					{
						bNeedToAdd = true;
						Processed.Add(Actor);
					}
				}
				if(bNeedToAdd)
				{
					OutClusters.Add(Cluster);
				}
			}
		}
	}
}

void FHierarchicalLODBuilder::MergeClusters(class ULevel* InLevel, const int32 LODIdx)
{
	if (Clusters.Num() > 0)
	{
		int32 LastIndex = Clusters.Num()-1; //FMath::TruncToInt(Clusters.Num() * 0.70); /*(10% ratio)*/
		AWorldSettings* WorldSetting = InLevel->GetWorld()->GetWorldSettings();
		check (WorldSetting->HierarchicalLODSetup.IsValidIndex(LODIdx));
		float DesiredBoundBoxHalfSize = WorldSetting->HierarchicalLODSetup[LODIdx].DesiredBoundBoxSize * 0.5f;
		float DesiredFillingRatio = WorldSetting->HierarchicalLODSetup[LODIdx].DesiredFillingPercentage * 0.01f;
		ensure (DesiredFillingRatio!=0.f);
		float HighestCost = FMath::Pow(DesiredBoundBoxHalfSize, 3)/(DesiredFillingRatio);

		const int32 TotalCluster = Clusters.Num();
		// now we have minimum Clusters
		for(int32 ClusterId=0; ClusterId < TotalCluster; ++ClusterId)
		{
			auto& Cluster = Clusters[ClusterId];
			UE_LOG(LogLODGenerator, Log, TEXT("%d. %0.2f {%s}"), ClusterId+1, Cluster.GetCost(), *Cluster.ToString());

			for (int32 MergedClusterId=0; MergedClusterId < ClusterId; ++MergedClusterId)
			{ 
				// compare with previous clusters
				auto& MergedCluster = Clusters[MergedClusterId];
				// see if it's valid, if it contains, check the cost
				if (MergedCluster.IsValid() && Cluster.IsValid())
				{
					// if valid, see if it contains any of this actors
					// merge whole clusters
					FLODCluster NewCluster = Cluster + MergedCluster;
					float MergeCost = NewCluster.GetCost();

					// merge two clusters
					if (MergeCost <= HighestCost)
					{
						UE_LOG(LogLODGenerator, Log, TEXT("Merging of Cluster (%d) and (%d) with merge cost (%0.2f) "), ClusterId+1, MergedClusterId+1, MergeCost);
						
						MergedCluster = NewCluster;
						// now this cluster is invalid
						Cluster.Invalidate();
						break;
					}
					else if (Cluster.Contains(MergedCluster))
					{
						UE_LOG(LogLODGenerator, Log, TEXT("Removing actor (%d) from Cluster (%d)"), MergedClusterId+1, ClusterId+1);
						// if higher than high cost, then we break them apart
						Cluster -= MergedCluster;
						break;
					}
				}
			}

			UE_LOG(LogLODGenerator, Log, TEXT("Processed(%s): %0.2f {%s}"), Cluster.IsValid()? TEXT("Valid"):TEXT("Invalid"), Cluster.GetCost(), *Cluster.ToString());
		}

		static bool bBuildActor=true;

		GWarn->BeginSlowTask(LOCTEXT("HierarchicalLOD_CreatingSimpleMesh", "Creating Simple Mesh"), true);
		GEditor->BeginTransaction(LOCTEXT("HierarchicalLOD_CreatingSimpleMesh", "Creating Simple Mesh"));
		// print data
		int32 Index=0, TotalValidCluster=0;
		for (auto& Cluster: Clusters)
		{
			if (Cluster.IsValid())
			{
				++TotalValidCluster;
			}
		}

		for(auto& Cluster: Clusters)
		{
			if(Cluster.IsValid())
			{
				GWarn->StatusUpdate(Index, TotalValidCluster, LOCTEXT("HierarchicalLOD_BuildActor", "Building Actor"));
				UE_LOG(LogLODGenerator, Log, TEXT("%d. %0.2f (%s)"), ++Index, Cluster.GetCost(), *Cluster.ToString());
				if (bBuildActor)
				{
					Cluster.BuildActor(InLevel->GetWorld(), InLevel, LODIdx);
				}
			}
		}

		GEditor->EndTransaction();
		GWarn->EndSlowTask();
	}
}

bool ShouldGenerateCluster(class AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	if (!Actor->bEnableAutoLODGeneration)
	{
		return false;
	}

	// make sure we don't generate for LODActor for Level 0
	if (Actor->IsA(ALODActor::StaticClass()))
	{
		return false;
	}

	FVector Origin, Extent;
	Actor->GetActorBounds(false, Origin, Extent);
	if(Extent.SizeSquared() <= 0.1)
	{
		return false;
	}

	// for now only consider staticmesh - I don't think skel mesh would work with simplygon merge right now @fixme
	TArray<UStaticMeshComponent*> Components;
	Actor->GetComponents<UStaticMeshComponent>(Components);
	// TODO: support instanced static meshes
	Components.RemoveAll([](UStaticMeshComponent* Val){ return Val->IsA(UInstancedStaticMeshComponent::StaticClass()); });

	return (Components.Num() > 0);
}

void FHierarchicalLODBuilder::InitializeClusters(class ULevel* InLevel, const int32 LODIdx)
{
	if (InLevel->Actors.Num() > 0)
	{
		if (LODIdx == 0)
		{
			Clusters.Empty(InLevel->Actors.Num());

			// first we generate graph with 2 pair nodes
			// this is very expensive when we have so many actors
			// so we'll need to optimize later @todo
			for(int32 ActorId=0; ActorId<InLevel->Actors.Num(); ++ActorId)
			{
				AActor* Actor1 = InLevel->Actors[ActorId];
				// make sure Actor has bound
				if(ShouldGenerateCluster(Actor1))
				{
					for(int32 SubActorId=ActorId+1; SubActorId<InLevel->Actors.Num(); ++SubActorId)
					{
						AActor* Actor2 = InLevel->Actors[SubActorId];
						if(ShouldGenerateCluster(Actor2))
						{
							FLODCluster NewClusterCandidate = FLODCluster(Actor1, Actor2);
							// @todo optimization : filter first - this might create disconnected tree - 
							// minimal it should be filled up 20% even to consider that
							// if (NewClusterCandidate.GetFillingFactor() > 0.2f)
							Clusters.Add(NewClusterCandidate);
						}
					}
				}
			}
		}
		else // at this point we only care for LODActors
		{
			Clusters.Empty();
			TArray<ALODActor*> LODActors;
			for(int32 ActorId=0; ActorId<InLevel->Actors.Num(); ++ActorId)
			{
				ALODActor* Actor = Cast<ALODActor>(InLevel->Actors[ActorId]);
				if (Actor)
				{
					LODActors.Add(Actor);
				}
			}
			// first we generate graph with 2 pair nodes
			// this is very expensive when we have so many actors
			// so we'll need to optimize later @todo
			for(int32 ActorId=0; ActorId<LODActors.Num(); ++ActorId)
			{
				ALODActor* Actor1 = (LODActors[ActorId]);
				if (Actor1->LODLevel == LODIdx)
				{
					for(int32 SubActorId=ActorId+1; SubActorId<LODActors.Num(); ++SubActorId)
					{
						ALODActor* Actor2 = LODActors[SubActorId];

						if (Actor2->LODLevel == LODIdx)
						{
							// create new cluster
							FLODCluster NewClusterCandidate = FLODCluster(Actor1, Actor2);
							// @todo optimization : filter first - this might create disconnected tree - 
							// minimal it should be filled up 20% even to consider that
							// if (NewClusterCandidate.GetFillingFactor() > 0.2f)
							Clusters.Add(NewClusterCandidate);
						}
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// LODCluster
////////////////////////////////////////////////////////////////////////////////////////////


FLODCluster::FLODCluster(const FLODCluster& Other)
: Actors(Other.Actors)
, Bound(Other.Bound)
, FillingFactor(Other.FillingFactor)
, bValid(Other.bValid)
{
}

FLODCluster::FLODCluster(class AActor* Actor1, class AActor* Actor2)
: Bound(ForceInit)
, bValid(true)
{
	FBox Actor1Bound = AddActor(Actor1);
	FBox Actor2Bound = AddActor(Actor2);

	// calculate new filling factor
	FillingFactor = CalculateFillingFactor(Actor1Bound, 1.f, Actor2Bound, 1.f);
}

FBox FLODCluster::AddActor(class AActor* NewActor)
{
	ensure (Actors.Contains(NewActor) == false);
	Actors.Add(NewActor);
	FVector Origin, Extent;
	NewActor->GetActorBounds(false, Origin, Extent);
	FBox NewBound = FBox(Origin-Extent, Origin+Extent);
	Bound += NewBound;

	return NewBound;
}

FLODCluster FLODCluster::operator+(const FLODCluster& Other) const
{
	FLODCluster UnionCluster(*this);
	UnionCluster.MergeClusters(Other);
	return UnionCluster;
}

FLODCluster FLODCluster::operator+=(const FLODCluster& Other)
{
	MergeClusters(Other);
	return *this;
}

FLODCluster FLODCluster::operator-(const FLODCluster& Other) const
{
	FLODCluster Cluster(*this);
	Cluster.SubtractCluster(Other);
	return *this;
}

FLODCluster FLODCluster::operator-=(const FLODCluster& Other)
{
	SubtractCluster(Other);
	return *this;
}

FLODCluster& FLODCluster::operator=(const FLODCluster& Other)
{
	this->bValid		= Other.bValid;
	this->Actors		= Other.Actors;
	this->Bound			= Other.Bound;
	this->FillingFactor = Other.FillingFactor;

	return *this;
}
// Merge two clusters
void FLODCluster::MergeClusters(const FLODCluster& Other)
{
	// please note that when merge, we merge two boxes from each cluster, not exactly all actors' bound
	// have to recalculate filling factor and bound based on cluster data
	FillingFactor = CalculateFillingFactor(Bound, FillingFactor, Other.Bound, Other.FillingFactor);
	Bound += Other.Bound;

	for (auto& Actor: Other.Actors)
	{
		Actors.AddUnique(Actor);
	}
}

void FLODCluster::SubtractCluster(const FLODCluster& Other)
{
	for(int32 ActorId=0; ActorId<Actors.Num(); ++ActorId)
	{
		if (Other.Actors.Contains(Actors[ActorId]))
		{
			Actors.RemoveAt(ActorId);
			--ActorId;
		}
	}

	TArray<AActor*> NewActors = Actors;
	Actors.Empty();
	// need to recalculate parameter
	if (NewActors.Num() == 0)
	{
		Invalidate();
	}
	else if (NewActors.Num() == 1)
	{
		Bound = FBox(ForceInitToZero);
		AddActor(NewActors[0]);
		FillingFactor = 1.f;
	}
	else if (NewActors.Num() == 2)
	{
		Bound = FBox(ForceInitToZero);

		FBox Actor1Bound = AddActor(NewActors[0]);
		FBox Actor2Bound = AddActor(NewActors[1]);

		// calculate new filling factor
		FillingFactor = CalculateFillingFactor(Actor1Bound, 1.f, Actor2Bound, 1.f);
	}
	else
	{
		// this shouldn't happen
		// if this has to happen, we'll either have to come up with way to subtrac bound or recalculate from scratch
		ensure (false);
	}
}

// invalidate this cluster - happens when this merged to another cluster
void FLODCluster::Invalidate()
{
	bValid = false;
}

bool FLODCluster::IsValid() const
{
	return bValid;
}

const float FLODCluster::GetCost() const
{
	FVector BoundExtent = Bound.GetExtent();
	const float Extent = BoundExtent.GetMax();
	return (FMath::Pow(Extent, 3) / FillingFactor);
}
// if criteria matches, build new LODActor and replace current Actors with that. We don't need 
// this clears previous actors and sets to this new actor
// this is required when new LOD is created from these actors, this will be replaced
// to save memory and to reduce memory increase during this process, we discard previous actors and replace with this actor
void FLODCluster::BuildActor(class UWorld* InWorld, class ULevel* InLevel, const int32 LODIdx)
{
	// do big size
	if (InWorld && InLevel)
	{
		////////////////////////////////////////////////////////////////////////////////////
		// create asset using Actors
		const FHierarchicalSimplification& LODSetup = InWorld->GetWorldSettings()->HierarchicalLODSetup[LODIdx];
		const FMeshProxySettings& ProxySetting = LODSetup.Setting;
		// Where generated assets will be stored
		UPackage* AssetsOuter = InLevel->GetOutermost(); // this asset is going to save with map, this means, I'll have to delete with it
		if (AssetsOuter)
		{
			TArray<UStaticMeshComponent*> AllComponents;

			for (auto& Actor: Actors)
			{
				TArray<UStaticMeshComponent*> Components;
				Actor->GetComponents<UStaticMeshComponent>(Components);
				// TODO: support instanced static meshes
				Components.RemoveAll([](UStaticMeshComponent* Val){ return Val->IsA(UInstancedStaticMeshComponent::StaticClass()); });
				AllComponents.Append(Components);
			}

			// it shouldn't even have come here if it didn't have any staticmesh
			if (ensure(AllComponents.Num() > 0))
			{
				// In case we don't have outer generated assets should have same path as LOD level
				const FString AssetsPath = FPackageName::GetLongPackagePath(AssetsOuter->GetName()) + TEXT("/");
				class AActor* FirstActor = Actors[0];

				TArray<UObject*> OutAssets;
				FVector OutProxyLocation = FVector::ZeroVector;
				// Generate proxy mesh and proxy material assets
				IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
				// should give unique name, so use level + actor name
				const FString PackageName = FString::Printf(TEXT("LOD_%s_%s"), *InLevel->GetName(), *FirstActor->GetName());
				if (LODSetup.bSimplifyMesh)
				{
					MeshUtilities.CreateProxyMesh(Actors, ProxySetting, AssetsOuter, PackageName, OutAssets, OutProxyLocation);
				}
				else
				{
					FMeshMergingSettings MergeSetting;
					MeshUtilities.MergeActors(Actors, MergeSetting, AssetsPath, OutAssets, OutProxyLocation );
				}

				UStaticMesh* MainMesh=NULL;
				// set staticmesh
				for(auto& Asset: OutAssets)
				{
					class UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset);

					if(StaticMesh)
					{
						MainMesh = StaticMesh;
					}
				}

				if (MainMesh)
				{
					////////////////////////////////////////////////////////////////////////////////////
					// create LODActors using the current Actors
					class ALODActor* NewActor = Cast<ALODActor>(InWorld->SpawnActor(ALODActor::StaticClass(), &OutProxyLocation, &FRotator::ZeroRotator));

					NewActor->SubObjects = OutAssets;
					NewActor->SubActors = Actors;
					NewActor->LODLevel = LODIdx+1; 
					float DrawDistance = LODSetup.DrawDistance;

					NewActor->GetStaticMeshComponent()->StaticMesh = MainMesh;

					// now set as parent
					for(auto& Actor: Actors)
					{
						Actor->SetLODParent(NewActor->GetStaticMeshComponent(), DrawDistance);
					}
				}
			}
		}
	}
}

bool FLODCluster::Contains(FLODCluster& Other) const
{
	if (IsValid() && Other.IsValid())
	{
		for(auto& Actor: Other.Actors)
		{
			if(Actors.Contains(Actor))
			{
				return true;
			}
		}
	}

	return false;
}

FString FLODCluster::ToString() const
{
	FString ActorList;
	for (auto& Actor: Actors)
	{
		ActorList += Actor->GetActorLabel();
		ActorList += ", ";
	}

	return FString::Printf(TEXT("ActorNum(%d), Actor List (%s)"), Actors.Num(), *ActorList);
}

#undef LOCTEXT_NAMESPACE 
