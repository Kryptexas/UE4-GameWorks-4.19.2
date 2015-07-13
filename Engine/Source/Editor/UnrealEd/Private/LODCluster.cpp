// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "LODCluster.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Engine/LODActor.h"
#include "Editor/UnrealEd/Classes/Factories/Factory.h"
#include "MeshUtilities.h"
#include "ObjectTools.h"
#endif // WITH_EDITOR

#include "GameFramework/WorldSettings.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "HierarchicalLODVolume.h"

#define LOCTEXT_NAMESPACE "LODCluster"
#define CM_TO_METER		0.01f
#define METER_TO_CM		100.0f

/** Utility function to calculate overlap of two spheres */
const float CalculateOverlap(const FSphere& ASphere, const float AFillingFactor, const FSphere& BSphere, const float BFillingFactor)
{
	// if it doesn't intersect, return zero 
	if (!ASphere.Intersects(BSphere))
	{
		return 0.f;
	}

	if (ASphere.IsInside(BSphere, 1.f))
	{
		return ASphere.GetVolume();
	}

	if(BSphere.IsInside(ASphere, 1.f))
	{
		return BSphere.GetVolume();
	}

	if (ASphere.Equals(BSphere, 1.f))
	{
		return ASphere.GetVolume();
	}

	float Distance = (ASphere.Center-BSphere.Center).Size();
	check (Distance > 0.f);

	float ARadius = ASphere.W;
	float BRadius = BSphere.W;

	float ACapHeight = (BRadius*BRadius - (ARadius - Distance)*(ARadius - Distance)) / (2*Distance);
	float BCapHeight = (ARadius*ARadius - (BRadius - Distance)*(BRadius - Distance)) / (2*Distance);

	if (ACapHeight<=0.f || BCapHeight<=0.f)
	{
		// it's possible to get cap height to be less than 0 
		// since when we do check intersect, we do have regular tolerance
		return 0.f;		
	}

	float OverlapRadius1 = ((ARadius + BRadius)*(ARadius + BRadius) - Distance*Distance) * (Distance*Distance - (ARadius - BRadius)*(ARadius - BRadius));
	float OverlapRadius2 = 2 * Distance;
	float OverlapRedius = FMath::Sqrt(OverlapRadius1) / OverlapRadius2;
	float OverlapRediusSq = OverlapRedius*OverlapRedius;

	check (OverlapRadius1 >= 0.f);

	float ConstPI = PI/6.0f;
	float AVolume = ConstPI*(3*OverlapRediusSq + ACapHeight*ACapHeight) * ACapHeight;
	float BVolume = ConstPI*(3*OverlapRediusSq + BCapHeight*BCapHeight) * BCapHeight;

	check (AVolume > 0.f &&  BVolume > 0.f);

	float TotalVolume = AFillingFactor*AVolume + BFillingFactor*BVolume;
	return TotalVolume;
}

/** Utility function that calculates filling factor */
const float CalculateFillingFactor(const FSphere& ASphere, const float AFillingFactor, const FSphere& BSphere, const float BFillingFactor)
{
	const float OverlapVolume = CalculateOverlap( ASphere, AFillingFactor, BSphere, BFillingFactor);
	FSphere UnionSphere = ASphere + BSphere;
	// it shouldn't be zero or it should be checked outside
	ensure(UnionSphere.W != 0.f);

	// http://deim.urv.cat/~rivi/pub/3d/icra04b.pdf
	// cost is calculated based on r^3 / filling factor
	// since it subtract by AFillingFactor * 1/2 overlap volume + BfillingFactor * 1/2 overlap volume
	return FMath::Max(0.f, (AFillingFactor * ASphere.GetVolume() + BFillingFactor * BSphere.GetVolume() - OverlapVolume) / UnionSphere.GetVolume());
}

FLODCluster::FLODCluster(const FLODCluster& Other)
: Actors(Other.Actors)
, Bound(Other.Bound)
, FillingFactor(Other.FillingFactor)
, ClusterCost( FMath::Pow(Bound.W, 3) / FillingFactor )
, bValid(Other.bValid)
{
}

FLODCluster::FLODCluster(AActor* Actor1)
: Bound(ForceInit)
, bValid(true)
{
	AddActor(Actor1);
	// calculate new filling factor
	FillingFactor = 1.f;	
	ClusterCost = FMath::Pow(Bound.W, 3) / FillingFactor;
}

FLODCluster::FLODCluster(AActor* Actor1, AActor* Actor2)
: Bound(ForceInit)
, bValid(true)
{
	FSphere Actor1Bound = AddActor(Actor1);
	FSphere Actor2Bound = AddActor(Actor2);
	
	// calculate new filling factor
	FillingFactor = CalculateFillingFactor(Actor1Bound, 1.f, Actor2Bound, 1.f);	
	ClusterCost = FMath::Pow(Bound.W, 3) / FillingFactor;
}

FLODCluster::FLODCluster()
: Bound(ForceInit)
, bValid(false)
{
	// calculate new filling factor
	FillingFactor = 1.f;
	ClusterCost = FMath::Pow(Bound.W, 3) / FillingFactor;
}

FSphere FLODCluster::AddActor(AActor* NewActor)
{
	bValid = true;
	ensure (Actors.Contains(NewActor) == false);
	Actors.Add(NewActor);
	FVector Origin, Extent;
#ifdef WITH_EDITORONLY_DATA
	if (NewActor->IsA<ALODActor>())
	{
		ALODActor* LODActor = CastChecked<ALODActor>(NewActor);

		if (LODActor->IsPreviewActorInstance())
		{
			Origin = LODActor->GetDrawSphereComponent()->GetComponentLocation();
			Extent = FVector(LODActor->GetDrawSphereComponent()->GetScaledSphereRadius());
		}
		else
		{
			NewActor->GetActorBounds(false, Origin, Extent);
		}
	}
	else
#endif // WITH_EDITORONLY_DATA
	{
		NewActor->GetActorBounds(false, Origin, Extent);
	}

	// scale 0.01 (change to meter from centimeter) QQ Extens.GetSize	
	FSphere NewBound = FSphere(Origin*CM_TO_METER, Extent.Size()*CM_TO_METER);
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
	this->ClusterCost = Other.ClusterCost;

	return *this;
}

void FLODCluster::MergeClusters(const FLODCluster& Other)
{
	// please note that when merge, we merge two boxes from each cluster, not exactly all actors' bound
	// have to recalculate filling factor and bound based on cluster data
	FillingFactor = CalculateFillingFactor(Bound, FillingFactor, Other.Bound, Other.FillingFactor);
	Bound += Other.Bound;	
	ClusterCost = FMath::Pow(Bound.W, 3) / FillingFactor;

	for (auto& Actor: Other.Actors)
	{
		Actors.AddUnique(Actor);
	}

	if (Actors.Num() > 0)
	{
		bValid = true;
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
		Bound = FSphere(ForceInitToZero);
		AddActor(NewActors[0]);
		FillingFactor = 1.f;
		ClusterCost = FMath::Pow(Bound.W, 3) / FillingFactor;
	}
	else if (NewActors.Num() >= 2)
	{
		Bound = FSphere(ForceInit);

		FSphere Actor1Bound = AddActor(NewActors[0]);
		FSphere Actor2Bound = AddActor(NewActors[1]);

		// calculate new filling factor
		FillingFactor = CalculateFillingFactor(Actor1Bound, 1.f, Actor2Bound, 1.f);

		// if more actors, we add them manually
		for (int32 ActorId=2; ActorId<NewActors.Num(); ++ActorId)
		{
			// if not contained, it shouldn't be
			check (!Actors.Contains(NewActors[ActorId]));

			FSphere NewBound = AddActor(NewActors[ActorId]);
			FillingFactor = CalculateFillingFactor(NewBound, 1.f, Bound, FillingFactor);
			Bound += NewBound;
		}

		ClusterCost = FMath::Pow(Bound.W, 3) / FillingFactor;
	}
}

void FLODCluster::BuildActor(ULevel* InLevel, const int32 LODIdx, const bool bCreateMeshes)
{
	FColor Colours[8] = { FColor::Cyan, FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Magenta, FColor::White, FColor::Black };
	// do big size
	if (InLevel && InLevel->GetWorld())
	{
		// create asset using Actors
		const FHierarchicalSimplification& LODSetup = InLevel->GetWorld()->GetWorldSettings()->HierarchicalLODSetup[LODIdx];

		// Retrieve draw distance for current and next LOD level
		const float DrawDistance = LODSetup.DrawDistance;		
		const int32 LODCount = InLevel->GetWorld()->GetWorldSettings()->HierarchicalLODSetup.Num();
		const float NextDrawDistance = (LODIdx < (LODCount - 1)) ? InLevel->GetWorld()->GetWorldSettings()->HierarchicalLODSetup[LODIdx + 1].DrawDistance : 0.0f;

		// Where generated assets will be stored
		UPackage* AssetsOuter = InLevel->GetOutermost(); // this asset is going to save with map, this means, I'll have to delete with it
		if (AssetsOuter)
		{
			TArray<UStaticMeshComponent*> AllComponents;

			for (auto& Actor: Actors)
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
			}

			// it shouldn't even have come here if it didn't have any staticmesh
			if (ensure(AllComponents.Num() > 0))
			{
				// In case we don't have outer generated assets should have same path as LOD level

				const FString AssetsPath = AssetsOuter->GetName() + TEXT("/");
				AActor* FirstActor = Actors[0];

				TArray<UObject*> OutAssets;
				FVector OutProxyLocation = FVector::ZeroVector;
				UStaticMesh* MainMesh = nullptr;
				if (bCreateMeshes)
				{
					// Generate proxy mesh and proxy material assets
					IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
					// should give unique name, so use level + actor name
					const FString PackageName = FString::Printf(TEXT("LOD_%s"), *FirstActor->GetName());
					if (MeshUtilities.GetMeshMergingInterface() && LODSetup.bSimplifyMesh)
					{
						MeshUtilities.CreateProxyMesh(Actors, LODSetup.ProxySetting, AssetsOuter, PackageName, OutAssets, OutProxyLocation);
					}
					else
					{						
						MeshUtilities.MergeStaticMeshComponents(AllComponents, FirstActor->GetWorld(), LODSetup.MergeSetting, AssetsOuter, PackageName, LODIdx, OutAssets, OutProxyLocation, LODSetup.DrawDistance, true);
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
				}
							

				if (MainMesh || !bCreateMeshes)
				{
					UWorld* LevelWorld = Cast<UWorld>(InLevel->GetOuter());

					check (LevelWorld);
										
					// create LODActors using the current Actors
					ALODActor* NewActor = Cast<ALODActor>(LevelWorld->SpawnActor(ALODActor::StaticClass(), &OutProxyLocation, &FRotator::ZeroRotator));
					NewActor->SubObjects = OutAssets;
					NewActor->SubActors = Actors;
					NewActor->LODLevel = LODIdx+1; 					
					NewActor->LODDrawDistance = DrawDistance;
					NewActor->SetStaticMesh( MainMesh );
					NewActor->SetFolderPath("/HLODActors");											

#if WITH_EDITORONLY_DATA						
					// Setup sphere drawing visualization
					UDrawSphereComponent* SphereComponent = NewActor->GetDrawSphereComponent();
					SphereComponent->SetSphereRadius(Bound.W * METER_TO_CM);
					SphereComponent->SetWorldLocation(Bound.Center * METER_TO_CM);					
					SphereComponent->ShapeColor = Colours[LODIdx % 7];
					SphereComponent->MinDrawDistance = DrawDistance;
					SphereComponent->LDMaxDrawDistance = NextDrawDistance;
					SphereComponent->CachedMaxDrawDistance = NextDrawDistance;	
#endif // WITH_EDITORONLY_DATA						
					// now set as parent
					for (auto& Actor : Actors)
					{
						Actor->SetLODParent(NewActor->GetStaticMeshComponent(), DrawDistance);
					}					
				}
			}
		}
	}
}

void FLODCluster::ExtractStaticMeshComponentsFromLODActor(AActor* Actor, TArray<UStaticMeshComponent*> &InOutComponents)
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
