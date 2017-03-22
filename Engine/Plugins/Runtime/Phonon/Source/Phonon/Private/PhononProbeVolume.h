//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "PhononCommon.h"
#include "GameFramework/Volume.h"
#include "PhononProbeVolume.generated.h"

UENUM()
enum class EPhononProbePlacementStrategy : uint8
{
	CENTROID = 0 UMETA(DisplayName = "Centroid"),
	//OCTREE = 1 UMETA(DisplayName = "Octree"),
	UNIFORM_FLOOR = 1 UMETA(DisplayName = "Uniform Floor"),
};

UENUM()
enum class EPhononProbeMobility : uint8
{
	// Static probes remain fixed at runtime.
	STATIC = 0 UMETA(DisplayName = "Static"),
	// Dynamic probes inherit this volume's offset at runtime.
	DYNAMIC = 1 UMETA(DisplayName = "Dynamic")
};

/**
 * Phonon Probe volumes ...
 */
UCLASS(HideCategories = (Actor, Advanced, Attachment, Collision))
class PHONON_API APhononProbeVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

public:

	void GenerateProbes();

	uint8* GetProbeBoxData()
	{
		return ProbeBoxData.GetData();
	}

	const int32 GetProbeBoxDataSize() const
	{
		return ProbeBoxData.Num();
	}

	uint8* GetProbeBatchData()
	{
		return ProbeBatchData.GetData();
	}

	const int32 GetProbeBatchDataSize() const
	{
		return ProbeBatchData.Num();
	}

	void UpdateProbeBoxData(IPLhandle ProbeBox)
	{
		// Update probe box serialized data
		auto NewProbeBoxDataSize = iplSaveProbeBox(ProbeBox, nullptr);
		ProbeBoxData.SetNumUninitialized(NewProbeBoxDataSize);
		iplSaveProbeBox(ProbeBox, ProbeBoxData.GetData());

		// Update probe batch serialized data
		IPLhandle ProbeBatch = nullptr;
		iplCreateProbeBatch(&ProbeBatch);

		auto NumProbes = iplGetProbeSpheres(ProbeBox, nullptr);
		for (auto i = 0; i < NumProbes; ++i)
		{
			iplAddProbeToBatch(ProbeBatch, ProbeBox, i);
		}

		iplFinalizeProbeBatch(ProbeBatch);
		auto ProbeBatchDataSize = iplSaveProbeBatch(ProbeBatch, nullptr);
		ProbeBatchData.SetNumUninitialized(ProbeBatchDataSize);
		iplSaveProbeBatch(ProbeBatch, ProbeBatchData.GetData());
		iplDestroyProbeBatch(&ProbeBatch);
	}

	UPROPERTY(EditAnywhere, Category = ProbeGeneration)
	EPhononProbePlacementStrategy PlacementStrategy;

	UPROPERTY(EditAnywhere, Category = ProbeGeneration, meta = (ClampMin = "0.0", ClampMax = "1024.0", UIMin = "0.0", UIMax = "1024.0"))
	float HorizontalSpacing;

	UPROPERTY(EditAnywhere, Category = ProbeGeneration, meta = (ClampMin = "0.0", ClampMax = "1024.0", UIMin = "0.0", UIMax = "1024.0"))
	float HeightAboveFloor;

private:

	UPROPERTY()
	TArray<class UPhononProbeComponent*> PhononProbeComponents;

	UPROPERTY()
	TArray<uint8> ProbeBoxData;

	UPROPERTY()
	TArray<uint8> ProbeBatchData;
};