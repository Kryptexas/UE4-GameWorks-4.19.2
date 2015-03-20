// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FoliagePrivate.h"
#include "ProceduralFoliageSpawner.h"
#include "ProceduralFoliageTile.h"
#include "Components/BoxComponent.h"
#include "PhysicsPublic.h"
#include "Async/Async.h"

#define LOCTEXT_NAMESPACE "ProceduralFoliage"

UProceduralFoliageSpawner::UProceduralFoliageSpawner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TileSize = 10000;	//100 m
	NumUniqueTiles = 10;
	RandomSeed = 42;
}

#if WITH_EDITOR
void UProceduralFoliageSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	bNeedsSimulation = true;
}
#endif

UProceduralFoliageTile* UProceduralFoliageSpawner::CreateTempTile()
{
	UProceduralFoliageTile* TmpTile = NewObject<UProceduralFoliageTile>(this);
	TmpTile->InitSimulation(this, 0);

	return TmpTile;
}

void UProceduralFoliageSpawner::CreateProceduralFoliageInstances()
{
	for(FProceduralFoliageTypeData& TypeData : Types)
	{
		if( TypeData.Type )
		{
			TypeData.TypeInstance = NewObject<UFoliageType_InstancedStaticMesh>(this, TypeData.Type);
			TypeData.ChangeCount = TypeData.TypeInstance->ChangeCount;
		}
		else
		{
			TypeData.TypeInstance = nullptr;
		}
	}
}

void UProceduralFoliageSpawner::SetClean()
{
	for (FProceduralFoliageTypeData& TypeData : Types)
	{
		if( TypeData.Type && TypeData.TypeInstance )
		{
			TypeData.TypeInstance->ChangeCount = TypeData.Type->GetDefaultObject<UFoliageType_InstancedStaticMesh>()->ChangeCount;
		}
	}

	bNeedsSimulation = false;
}

// Custom serialization version for all packages containing ProceduralFoliage
struct FProceduralFoliageCustomVersion
{
	enum Type
	{
		// Before any version changes were made in the plugin
		BeforeCustomVersionWasAdded = 0,
		// We serialize the type clean change map to ensure validity
		SerializeTypeMap = 1,
		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FProceduralFoliageCustomVersion() {}
};

const FGuid FProceduralFoliageCustomVersion::GUID(0xaafe32bd, 0x53954c14, 0xb66a5e25, 0x1032d1dd);
// Register the custom version with core
FCustomVersionRegistration GRegisterProceduralFoliageCustomVersion(FProceduralFoliageCustomVersion::GUID, FProceduralFoliageCustomVersion::LatestVersion, TEXT("ProceduralFoliageVer"));

void UProceduralFoliageSpawner::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FProceduralFoliageCustomVersion::GUID);
}

bool UProceduralFoliageSpawner::AnyDirty() const
{
	bool bDirty = bNeedsSimulation;

	for (const FProceduralFoliageTypeData& TypeData : Types)
	{
		if (TypeData.Type)
		{
			if (TypeData.TypeInstance && TypeData.Type->GetDefaultObject<UFoliageType_InstancedStaticMesh>()->ChangeCount != TypeData.ChangeCount)
			{
				bDirty = true;
				break;
			}
			else if (!TypeData.TypeInstance)
			{
				bDirty = true;
				break;
			}
		}
	}

	return bDirty;
}

void UProceduralFoliageSpawner::SimulateIfNeeded()
{
	//if (AnyDirty())	@todo: for now we must force simulation every time since precomputed tiles are weak pointers
	{
		Simulate();
	}

}

const UProceduralFoliageTile* UProceduralFoliageSpawner::GetRandomTile(int32 X, int32 Y)
{
	if (PrecomputedTiles.Num())
	{
		FRandomStream HashStream;	//using this as a hash function
		HashStream.Initialize(X);
		const float XRand = HashStream.FRand();
		HashStream.Initialize(Y);
		const float YRand = HashStream.FRand();
		const int32 RandomNumber = (RAND_MAX * XRand / (YRand + 0.01f));
		const int32 Idx = FMath::Clamp(RandomNumber % PrecomputedTiles.Num(), 0, PrecomputedTiles.Num() - 1);
		return PrecomputedTiles[Idx].Get();
	}

	return nullptr;
}

void UProceduralFoliageSpawner::Simulate(int32 NumSteps)
{
	RandomStream.Initialize(RandomSeed);
	CreateProceduralFoliageInstances();

	LastCancel.Increment();

	PrecomputedTiles.Empty();
	TArray<TFuture< UProceduralFoliageTile* >> Futures;

	for (int i = 0; i < NumUniqueTiles; ++i)
	{
		UProceduralFoliageTile* NewTile = NewObject<UProceduralFoliageTile>(this);
		const int32 RandomNumber = GetRandomNumber();
		const int32 LastCancelInit = LastCancel.GetValue();

		Futures.Add(Async<UProceduralFoliageTile*>(EAsyncExecution::ThreadPool, [=]()
		{
			NewTile->Simulate(this, RandomNumber, NumSteps, LastCancelInit);
			return NewTile;
		}));
	}

	const FText StatusMessage = LOCTEXT("SimulateProceduralFoliage", "Simulate ProceduralFoliage...");
	GWarn->BeginSlowTask(StatusMessage, true, true);
	
	const int32 TotalTasks = Futures.Num();

	bool bCancelled = false;

	for (int32 FutureIdx = 0; FutureIdx < Futures.Num(); ++FutureIdx)
	{
		while (Futures[FutureIdx].WaitFor(FTimespan(0, 0, 0, 0, 100)) == false)		//sleep for 100ms if not ready. Needed so cancel is responsive
		{
			GWarn->StatusUpdate(FutureIdx, TotalTasks, LOCTEXT("SimulateProceduralFoliage", "Simulate ProceduralFoliage..."));
			if (GWarn->ReceivedUserCancel() && bCancelled == false)	//If the user wants to cancel just increment this. Tiles compare against the original count and cancel if needed
			{
				LastCancel.Increment();
				bCancelled = true;
			}
		}

		PrecomputedTiles.Add(Futures[FutureIdx].Get());		//Even if we cancel we block until threads have exited safely to ensure memory isn't GCed
	}

	GWarn->EndSlowTask();

	if (bCancelled)
	{
		PrecomputedTiles.Empty();
	}
	else
	{
		SetClean();
	}
}

int32 UProceduralFoliageSpawner::GetRandomNumber()
{
	return RandomStream.FRand() * RAND_MAX;
}

#undef LOCTEXT_NAMESPACE