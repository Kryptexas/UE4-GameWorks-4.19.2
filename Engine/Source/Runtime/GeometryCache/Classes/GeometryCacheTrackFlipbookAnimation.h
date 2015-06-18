// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GeometryCacheMeshData.h"

#include "GeometryCacheTrackFlipbookAnimation.generated.h"

/** Derived GeometryCacheTrack class, used for Transform animation. */
UCLASS(collapsecategories, hidecategories = Object, BlueprintType, config = Engine)
class GEOMETRYCACHE_API UGeometryCacheTrack_FlipbookAnimation : public UGeometryCacheTrack
{
	GENERATED_UCLASS_BODY()

	virtual ~UGeometryCacheTrack_FlipbookAnimation();

	// Begin UObject interface.
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void BeginDestroy() override;
	// End UObject interface.

	// Begin UGeometryCacheTrack interface.
	virtual const bool UpdateMeshData(const float Time, const bool bLooping, int32& InOutMeshSampleIndex, FGeometryCacheMeshData*& OutMeshData) override;
	virtual const float GetMaxSampleTime() const override;
	// End UGeometryCacheTrack interface.

	/**
	* Add a GeometryCacheMeshData sample to the Track
	*
	* @param MeshData - Holds the mesh data for the specific sample
	* @param SampleTime - SampleTime for the specific sample being added
	* @return void
	*/
	UFUNCTION()
	void AddMeshSample(const FGeometryCacheMeshData& MeshData, const float SampleTime);

private:
	/** Number of Mesh Sample within this track */
	UPROPERTY(VisibleAnywhere, Category = GeometryCache)
	uint32 NumMeshSamples;

	/** Stored data for each Mesh sample */
	TArray<FGeometryCacheMeshData> MeshSamples;
	TArray<float> MeshSampleTimes;
};

