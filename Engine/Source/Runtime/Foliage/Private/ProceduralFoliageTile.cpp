// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FoliagePrivate.h"
#include "ProceduralFoliageTile.h"
#include "ProceduralFoliage.h"
#include "ProceduralFoliageBroadphase.h"
#include "InstancedFoliageActor.h"

#define LOCTEXT_NAMESPACE "ProceduralFoliage"

UProceduralFoliageTile::UProceduralFoliageTile(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}


bool UProceduralFoliageTile::HandleOverlaps(FProceduralFoliageInstance* Instance)
{
	//We now check what the instance overlaps. If any of its overlaps dominate we remove the instance and leave everything else alone.
	//If the instance survives we mark all dominated overlaps as pending removal. They will be removed from the broadphase and will not spread seeds or age.
	//Note that this introduces potential indeterminism! If the iteration order changes we could get different results. This is needed because it gives us huge performance savings.
	//Note that if the underlying data structures stay the same (i.e. no core engine changes) this should not matter. This gives us short term determinism, but not long term.

	bool bSurvived = true;
	TArray<FProceduralFoliageOverlap> Overlaps;
	Broadphase.GetOverlaps(Instance, Overlaps);

	//Check if the instance survives
	for (const FProceduralFoliageOverlap& Overlap : Overlaps)
	{
		FProceduralFoliageInstance* Dominated = FProceduralFoliageInstance::Domination(Overlap.A, Overlap.B, Overlap.OverlapType);
		if (Dominated == Instance)
		{
			bSurvived = false;
			break;
		}
	}

	if (bSurvived)
	{
		for (const FProceduralFoliageOverlap& Overlap : Overlaps)
		{
			if (FProceduralFoliageInstance* Dominated = FProceduralFoliageInstance::Domination(Overlap.A, Overlap.B, Overlap.OverlapType))
			{
				check(Dominated != Instance);	//we shouldn't be here if we survived
				MarkPendingRemoval(Dominated);	//We can't immediately remove because we're potentially iterating over existing instances.
			}
		}
	}
	else
	{
		//didn't survive so just die
		MarkPendingRemoval(Instance);
	}

	return bSurvived;
}

FProceduralFoliageInstance* UProceduralFoliageTile::NewSeed(const FVector& Location, float Scale, const UFoliageType_InstancedStaticMesh* Type, float InAge, bool bBlocker)
{
	const float InitRadius = Type->GetMaxRadius() * Scale;
	{
		FProceduralFoliageInstance* NewInst = new FProceduralFoliageInstance();
		NewInst->Location = Location;
		NewInst->Rotation = FQuat(FVector(0, 0, 1), RandomStream.FRandRange(0, 2.f*PI));
		NewInst->Age = InAge;
		NewInst->Type = Type;
		NewInst->Normal = FVector(0, 0, 1);
		NewInst->Scale = Scale;
		NewInst->bBlocker = bBlocker;
		
	

		Broadphase.Insert(NewInst);
		const bool bSurvived = HandleOverlaps(NewInst);
		return bSurvived ? NewInst : nullptr;
	}

	return nullptr;
}

float GetSeedMinDistance(const FProceduralFoliageInstance* Instance, const float NewInstanceAge, const int32 SimulationStep)
{
	const UFoliageType_InstancedStaticMesh* Type = Instance->Type;
	const int32 StepsLeft = Type->MaxAge - SimulationStep;
	const float InstanceMaxAge = Type->GetNextAge(Instance->Age, StepsLeft);
	const float NewInstanceMaxAge = Type->GetNextAge(NewInstanceAge, StepsLeft);

	const float InstanceMaxScale = Type->GetScaleForAge(InstanceMaxAge);
	const float NewInstanceMaxScale = Type->GetScaleForAge(NewInstanceMaxAge);

	const float InstanceMaxRadius = InstanceMaxScale * Type->GetMaxRadius();
	const float NewInstanceMaxRadius = NewInstanceMaxScale * Type->GetMaxRadius();

	return InstanceMaxRadius + NewInstanceMaxRadius;
}

/** Generates a random number with a normal distribution with mean=0 and variance = 1. Uses Box-Muller transformation http://mathworld.wolfram.com/Box-MullerTransformation.html */
float UProceduralFoliageTile::GetRandomGaussian()
{
	const float Rand1 = FMath::Max<float>(RandomStream.FRand(), SMALL_NUMBER);
	const float Rand2 = FMath::Max<float>(RandomStream.FRand(), SMALL_NUMBER);
	const float SqrtLn = FMath::Sqrt(-2.f * FMath::Loge(Rand1));
	const float Rand2TwoPi = Rand2 * 2.f * PI;
	const float Z1 = SqrtLn * FMath::Cos(Rand2TwoPi);
	return Z1;
}

FVector UProceduralFoliageTile::GetSeedOffset(const UFoliageType_InstancedStaticMesh* Type, float MinDistance)
{
	//We want 10% of seeds to be the max distance so we use a z score of +- 1.64
	const float MaxZScore = 1.64f;
	const float Z1 = GetRandomGaussian();
	const float Z1Clamped = FMath::Clamp(Z1, -MaxZScore, MaxZScore);
	const float VariationDistance = Z1Clamped * Type->SpreadVariance / MaxZScore;
	const float AverageDistance = MinDistance + Type->AverageSpreadDistance;
	
	const float RandRad = FMath::Max<float>(RandomStream.FRand(), SMALL_NUMBER) * PI * 2.f;
	const FVector Dir = FVector(FMath::Cos(RandRad), FMath::Sin(RandRad), 0);
	return Dir * (AverageDistance + VariationDistance);
}

void UProceduralFoliageTile::AgeSeeds()
{
	TArray<FProceduralFoliageInstance*> NewSeeds;
	for (FProceduralFoliageInstance* Instance : Instances)
	{
		if (Instance->IsAlive())
		{
			const UFoliageType_InstancedStaticMesh* Type = Instance->Type;
			if (SimulationStep <= Type->NumSteps && Type->bGrowsInShade == bSimulateShadeGrowth)
			{
				const float CurrentAge = Instance->Age;
				const float NewAge = Type->GetNextAge(Instance->Age, 1);
				const float NewScale = Type->GetScaleForAge(NewAge);

				const FVector Location = Instance->Location;

				MarkPendingRemoval(Instance);
				if (FProceduralFoliageInstance* Inst = NewSeed(Location, NewScale, Type, NewAge))
				{
					NewSeeds.Add(Inst);
				}
			}
		}
	}

	for (FProceduralFoliageInstance* Seed : NewSeeds)
	{
		Instances.Add(Seed);
	}

	FlushPendingRemovals();
	
}

void UProceduralFoliageTile::SpreadSeeds(TArray<FProceduralFoliageInstance*>& NewSeeds)
{
	for (FProceduralFoliageInstance* Inst : Instances)
	{
		if (Inst->IsAlive() == false)	//The instance has been killed so don't bother spreading seeds. Note this introduces potential indeterminism if the order of instance traversal changes (implementation details of TSet for example)
		{
			continue;
		}

		const UFoliageType_InstancedStaticMesh* Type = Inst->Type;

		if (SimulationStep <= Type->NumSteps  && Type->bGrowsInShade == bSimulateShadeGrowth)
		{
			for (int32 i = 0; i < Type->SeedsPerStep; ++i)
			{
				//spread new seeds
				const float NewAge = Type->GetInitAge(RandomStream);
				const float NewScale = Type->GetScaleForAge(NewAge);
				const float MinDistanceToClear = GetSeedMinDistance(Inst, NewAge, SimulationStep);
				const FVector GlobalOffset = GetSeedOffset(Type, MinDistanceToClear);
				
				if (GlobalOffset.SizeSquared2D() + SMALL_NUMBER > MinDistanceToClear*MinDistanceToClear)
				{
					const FVector NewLocation = GlobalOffset + Inst->Location;
					if (FProceduralFoliageInstance* Inst = NewSeed(NewLocation, NewScale, Type, NewAge))
					{
						NewSeeds.Add(Inst);
					}
				}
			}
		}
	}
}

void UProceduralFoliageTile::AddRandomSeeds(TArray<FProceduralFoliageInstance*>& OutInstances)
{
	const float SizeTenM2 = (ProceduralFoliage->TileSize * ProceduralFoliage->TileSize) / (1000.f * 1000.f);

	TMap<int32,float> MaxShadeRadii;
	TMap<int32, float> MaxCollisionRadii;
	TMap<const UFoliageType*, int32> SeedsLeftMap;
	TMap<const UFoliageType*, FRandomStream> RandomStreamPerType;

	TArray<const UFoliageType_InstancedStaticMesh*> TypesToSeed;

	for (const FProceduralFoliageTypeData& Data : ProceduralFoliage->GetTypes())
	{
		const UFoliageType_InstancedStaticMesh* Type = Data.TypeInstance;
		if (Type && Type->bGrowsInShade == bSimulateShadeGrowth)
		{
			{	//compute the number of initial seeds
				const int32 NumSeeds = FMath::RoundToInt(Type->GetSeedDensitySquared() * SizeTenM2);
				SeedsLeftMap.Add(Type, NumSeeds);
				if (NumSeeds > 0)
				{
					TypesToSeed.Add(Type);
				}
			}

			{	//save the random stream per type
				RandomStreamPerType.Add(Type, FRandomStream(Type->DistributionSeed));
			}

			{	//compute the needed offsets for initial seed variance
				const int32 DistributionSeed = Type->DistributionSeed;
				const float MaxScale = Type->GetScaleForAge(Type->MaxAge);
				const float TypeMaxCollisionRadius = MaxScale * Type->CollisionRadius;
				if (float* MaxRadius = MaxCollisionRadii.Find(DistributionSeed))
				{
					*MaxRadius = FMath::Max(*MaxRadius, TypeMaxCollisionRadius);
				}
				else
				{
					MaxCollisionRadii.Add(DistributionSeed, TypeMaxCollisionRadius);
				}

				const float TypeMaxShadeRadius = MaxScale * Type->ShadeRadius;
				if (float* MaxRadius = MaxShadeRadii.Find(DistributionSeed))
				{
					*MaxRadius = FMath::Max(*MaxRadius, TypeMaxShadeRadius);
				}
				else
				{
					MaxShadeRadii.Add(DistributionSeed, TypeMaxShadeRadius);
				}
			}
			
		}
	}

	int32 TypeIdx = -1;
	const int32 NumTypes = TypesToSeed.Num();
	int32 TypesLeftToSeed = NumTypes;
	const int32 LastShadeCastingIndex = InstancesArray.Num() - 1; //when placing shade growth types we want to spawn in shade if possible
	while (TypesLeftToSeed > 0)
	{
		TypeIdx = (TypeIdx + 1) % NumTypes;	//keep cycling through the types that we spawn initial seeds for to make sure everyone gets fair chance

		if (const UFoliageType_InstancedStaticMesh* Type = TypesToSeed[TypeIdx])
		{
			int32& SeedsLeft = SeedsLeftMap.FindChecked(Type);
			if (SeedsLeft == 0)
			{
				continue;
			}

			const float NewAge = Type->GetInitAge(RandomStream);
			const float Scale = Type->GetScaleForAge(NewAge);

			FRandomStream& TypeRandomStream = RandomStreamPerType.FindChecked(Type);
			float InitX = 0.f;
			float InitY = 0.f;
			float NeededRadius = 0.f;

			if (bSimulateShadeGrowth && LastShadeCastingIndex >= 0)
			{
				const int32 InstanceSpawnerIdx = TypeRandomStream.FRandRange(0, LastShadeCastingIndex);
				const FProceduralFoliageInstance& Spawner = InstancesArray[InstanceSpawnerIdx];
				InitX = Spawner.Location.X;
				InitY = Spawner.Location.Y;
				NeededRadius = Spawner.GetCollisionRadius() * (Scale + Spawner.Type->GetScaleForAge(Spawner.Age));
			}
			else
			{
				InitX = TypeRandomStream.FRandRange(0, ProceduralFoliage->TileSize);
				InitY = TypeRandomStream.FRandRange(0, ProceduralFoliage->TileSize);
				NeededRadius = MaxShadeRadii.FindRef(Type->DistributionSeed);
			}

			const float Rad = RandomStream.FRandRange(0, PI*2.f);
			
			
			const FVector GlobalOffset = (RandomStream.FRandRange(0, Type->MaxInitialSeedOffset) + NeededRadius) * FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f);

			const float X = InitX + GlobalOffset.X;
			const float Y = InitY + GlobalOffset.Y;

			if (FProceduralFoliageInstance* NewInst = NewSeed(FVector(X, Y, 0.f), Scale, Type, NewAge))
			{
				OutInstances.Add(NewInst);
			}

			--SeedsLeft;
			if (SeedsLeft == 0)
			{
				--TypesLeftToSeed;
			}
		}
	}
}

void UProceduralFoliageTile::MarkPendingRemoval(FProceduralFoliageInstance* ToRemove)
{
	if (ToRemove->IsAlive())
	{
		Broadphase.Remove(ToRemove);	//we can remove from broadphase right away
		ToRemove->TerminateInstance();
		PendingRemovals.Add(ToRemove);
	}
}

void UProceduralFoliageTile::RemoveInstances()
{
	for (FProceduralFoliageInstance* Inst : Instances)
	{
		MarkPendingRemoval(Inst);
	}

	InstancesArray.Empty();
	FlushPendingRemovals();
}

void UProceduralFoliageTile::InstancesToArray()
{
	InstancesArray.Empty(Instances.Num());
	for (FProceduralFoliageInstance* FromInst : Instances)
	{
		if (FromInst->bBlocker == false)	//blockers do not get instantiated so don't bother putting it into array
		{
			new(InstancesArray)FProceduralFoliageInstance(*FromInst);
		}
	}
}

void UProceduralFoliageTile::RemoveInstance(FProceduralFoliageInstance* ToRemove)
{
	if (ToRemove->IsAlive())
	{
		Broadphase.Remove(ToRemove);
		ToRemove->TerminateInstance();
	}
	
	Instances.Remove(ToRemove);
	delete ToRemove;
}

void UProceduralFoliageTile::FlushPendingRemovals()
{
	for (FProceduralFoliageInstance* ToRemove : PendingRemovals)
	{
		RemoveInstance(ToRemove);
	}

	PendingRemovals.Empty();
}

void UProceduralFoliageTile::InitSimulation(const UProceduralFoliage* InProceduralFoliage, const int32 RandomSeed)
{
	RandomStream.Initialize(RandomSeed);
	ProceduralFoliage = InProceduralFoliage;
	SimulationStep = 0;
	Broadphase = FProceduralFoliageBroadphase(ProceduralFoliage->TileSize);
}

void UProceduralFoliageTile::StepSimulation()
{
	TArray<FProceduralFoliageInstance*> NewInstances;
	if (SimulationStep == 0)
	{
		AddRandomSeeds(NewInstances);
	}
	else
	{
		AgeSeeds();
		SpreadSeeds(NewInstances);
	}

	for (FProceduralFoliageInstance* Inst : NewInstances)
	{
		Instances.Add(Inst);
	}

	FlushPendingRemovals();
}

void UProceduralFoliageTile::StartSimulation(const int32 MaxNumSteps, bool bShadeGrowth)
{
	int32 MaxSteps = 0;

	for (const FProceduralFoliageTypeData& Data : ProceduralFoliage->GetTypes())
	{
		const UFoliageType_InstancedStaticMesh* TypeInstance = Data.TypeInstance;
		if (TypeInstance && TypeInstance->bGrowsInShade == bShadeGrowth)
		{
			MaxSteps = FMath::Max(MaxSteps, TypeInstance->NumSteps + 1);
		}
	}

	if (MaxNumSteps >= 0)
	{
		MaxSteps = FMath::Min(MaxSteps, MaxNumSteps);	//only take as many steps as given
	}

	SimulationStep = 0;
	bSimulateShadeGrowth = bShadeGrowth;
	for (int32 Step = 0; Step < MaxSteps; ++Step)
	{
		StepSimulation();
		++SimulationStep;
	}

	InstancesToArray();
}

void UProceduralFoliageTile::Simulate(const UProceduralFoliage* InProceduralFoliage, const int32 RandomSeed, const int32 MaxNumSteps)
{
	InitSimulation(InProceduralFoliage, RandomSeed);

	StartSimulation(MaxNumSteps, false);
	StartSimulation(MaxNumSteps, true);
}


void UProceduralFoliageTile::BeginDestroy()
{
	Super::BeginDestroy();
	RemoveInstances();
}

void UProceduralFoliageTile::CreateInstancesToSpawn(TArray<FDesiredFoliageInstance>& OutInstances, const FTransform& WorldTM, const FGuid& ProceduralGuid, const float HalfHeight, const FBodyInstance* VolumeBodyInstance) const
{
	const FCollisionQueryParams Params(true);
	FHitResult Hit;

	OutInstances.Reserve(Instances.Num());
	for (const FProceduralFoliageInstance& Instance : InstancesArray)
	{
		FVector StartRay = Instance.Location + WorldTM.GetLocation();
		StartRay.Z += HalfHeight;
		FVector EndRay = StartRay;
		EndRay.Z -= HalfHeight*2.f;

		FDesiredFoliageInstance* DesiredInst = new (OutInstances)FDesiredFoliageInstance(StartRay, EndRay, Instance.GetMaxRadius());
		DesiredInst->Rotation = Instance.Rotation;
		DesiredInst->ProceduralGuid = ProceduralGuid;
		DesiredInst->FoliageType = Instance.Type;
		DesiredInst->Age = Instance.Age;
		DesiredInst->ProceduralVolumeBodyInstance = VolumeBodyInstance;
		DesiredInst->PlacementMode = EFoliagePlacementMode::Procedural;
	}
}

void UProceduralFoliageTile::Empty()
{
	Broadphase.Empty();
	InstancesArray.Empty();
	
	for (FProceduralFoliageInstance* Inst : Instances)
	{
		delete Inst;
	}

	Instances.Empty();
	PendingRemovals.Empty();
}

SIZE_T UProceduralFoliageTile::GetResourceSize(EResourceSizeMode::Type Mode)
{
	SIZE_T TotalSize = 0;
	for (FProceduralFoliageInstance* Inst : Instances)
	{
		TotalSize += sizeof(FProceduralFoliageInstance);
	}
	
	//@TODO: account for broadphase
	return TotalSize;
}


void UProceduralFoliageTile::GetInstancesInAABB(const FBox2D& LocalAABB, TArray<FProceduralFoliageInstance*>& OutInstances, bool bOnTheBorder) const
{
	TArray<FProceduralFoliageInstance*> InstancesInAABB;
	Broadphase.GetInstancesInBox(LocalAABB, InstancesInAABB);

	OutInstances.Reserve(OutInstances.Num() + InstancesInAABB.Num());
	for (FProceduralFoliageInstance* Inst : InstancesInAABB)
{
		const float Rad = Inst->GetMaxRadius();
		const FVector& Location = Inst->Location;

		if (bOnTheBorder || (Location.X - Rad >= LocalAABB.Min.X && Location.X + Rad <= LocalAABB.Max.X && Location.Y - Rad >= LocalAABB.Min.Y && Location.Y + Rad <= LocalAABB.Max.Y))
		{
			OutInstances.Add(Inst);
		}
	}
}

void UProceduralFoliageTile::AddInstances(const TArray<FProceduralFoliageInstance*>& NewInstances, const FTransform& RelativeTM, const FBox2D& InnerLocalAABB)
{
	for (const FProceduralFoliageInstance* Inst : NewInstances)
	{
		const FVector& Location = Inst->Location;	//we need the local space because we're comparing it to the AABB
		//Instances in InnerLocalAABB or on the border of the max sides of the AABB will be visible and instantiated by this tile
		//Instances outside of the InnerLocalAABB are only used for rejection purposes. This is needed for overlapping tiles
		const float Radius = Inst->GetMaxRadius();
		bool bBlocker = false;
		if (Location.X + Radius <= InnerLocalAABB.Min.X || Location.X - Radius > InnerLocalAABB.Max.X || Location.Y + Radius <= InnerLocalAABB.Min.Y || Location.Y - Radius > InnerLocalAABB.Max.Y)
		{
			//Only used for blocking instances in case of overlap, a different tile will instantiate it
			bBlocker = true;
		}

		const FVector NewLocation = RelativeTM.TransformPosition(Inst->Location);
		if (FProceduralFoliageInstance* NewInst = NewSeed(NewLocation, Inst->Scale, Inst->Type, Inst->Age, bBlocker))
		{
			
			Instances.Add(NewInst);
		}
	}

	FlushPendingRemovals();
}

#undef LOCTEXT_NAMESPACE