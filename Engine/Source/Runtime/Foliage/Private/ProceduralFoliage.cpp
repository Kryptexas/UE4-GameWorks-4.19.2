// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FoliagePrivate.h"
#include "ProceduralFoliage.h"
#include "ProceduralFoliageTile.h"
#include "Components/BoxComponent.h"
#include "PhysicsPublic.h"
#include "Async/Async.h"

#define LOCTEXT_NAMESPACE "ProceduralFoliage"

UProceduralFoliage::UProceduralFoliage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TileSize = 10000;	//100 m
	NumUniqueTiles = 10;
	RandomSeed = 42;
}

#if WITH_EDITOR
void UProceduralFoliage::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	bNeedsSimulation = true;
}
#endif

UProceduralFoliageTile* UProceduralFoliage::CreateTempTile()
{
	UProceduralFoliageTile* TmpTile = NewObject<UProceduralFoliageTile>(this);
	TmpTile->InitSimulation(this, 0);

	return TmpTile;
}

void UProceduralFoliage::GenerateTile(const int32 NumSteps)
{
	UProceduralFoliageTile* NewTile = NewObject<UProceduralFoliageTile>(this);
	PrecomputedTiles.Add(NewTile);

	NewTile->Simulate(this, GetRandomNumber(), NumSteps);
}

void UProceduralFoliage::CreateProceduralFoliageInstances()
{
	for(FProceduralFoliageTypeData& TypeData : Types)
	{
		if( TypeData.Type )
		{
			TypeData.TypeInstance = ConstructObject<UFoliageType_InstancedStaticMesh>(TypeData.Type, this);
			TypeData.ChangeCount = TypeData.TypeInstance->ChangeCount;
		}
		else
		{
			TypeData.TypeInstance = nullptr;
		}
	}
}

void UProceduralFoliage::SetClean()
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

void UProceduralFoliage::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FProceduralFoliageCustomVersion::GUID);
}

bool UProceduralFoliage::AnyDirty() const
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

const UProceduralFoliageTile* UProceduralFoliage::GetRandomTile(int32 X, int32 Y)
{
	if (AnyDirty())
	{
		Simulate();
	}

	if (PrecomputedTiles.Num())
	{
		FRandomStream HashStream;	//using this as a hash function
		HashStream.Initialize(X);
		const float XRand = HashStream.FRand();
		HashStream.Initialize(Y);
		const float YRand = HashStream.FRand();
		const int32 RandomNumber = (RAND_MAX * XRand / (YRand + 0.01f));
		const int32 Idx = FMath::Clamp(RandomNumber % PrecomputedTiles.Num(), 0, PrecomputedTiles.Num() - 1);
		return PrecomputedTiles[Idx];
	}

	return nullptr;
}

void UProceduralFoliage::Simulate(int32 NumSteps)
{
	RandomStream.Initialize(RandomSeed);
	CreateProceduralFoliageInstances();

	PrecomputedTiles.Empty();
	TArray<TFuture< UProceduralFoliageTile* >> Futures;

	for (int i = 0; i < NumUniqueTiles; ++i)
	{
		UProceduralFoliageTile* NewTile = NewObject<UProceduralFoliageTile>(this);
		const int32 RandomNumber = GetRandomNumber();

		Futures.Add(Async<UProceduralFoliageTile*>(EAsyncExecution::ThreadPool, [=]()
		{
			NewTile->Simulate(this, RandomNumber, NumSteps);
			return NewTile;
		}));
	}

	FScopedSlowTask SlowTask(NumUniqueTiles, LOCTEXT("SimulateProceduralFoliage", "Simulate ProceduralFoliage..."));
	SlowTask.MakeDialog();

	for (int32 FutureIdx = 0; FutureIdx < Futures.Num(); ++FutureIdx)
	{
		PrecomputedTiles.Add(Futures[FutureIdx].Get());
		SlowTask.EnterProgressFrame(1);
	}

	SetClean();
	
}

int32 UProceduralFoliage::GetRandomNumber()
{
	return RandomStream.FRand() * RAND_MAX;
}

#undef LOCTEXT_NAMESPACE