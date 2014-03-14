// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PrecomputedLightVolume.cpp: Implementation of a precomputed light volume.
=============================================================================*/

#include "EnginePrivate.h"
#include "PrecomputedLightVolume.h"
#include "TargetPlatform.h"

FArchive& operator<<(FArchive& Ar, FVolumeLightingSample& Sample)
{
	Ar << Sample.Position;
	Ar << Sample.Radius;

	if (Ar.UE4Ver() < VER_UE4_CHANGED_VOLUME_SAMPLE_FORMAT)
	{
		uint8 Temp = 0;
		FColor Temp2(0, 0, 0, 0);

		Ar << Temp << Temp;
		Ar << Temp << Temp;
		Ar << Temp2 << Temp2 << Temp2;
		Ar << Temp;
	}
	else
	{
		Ar << Sample.Lighting;
	}

	if (Ar.UE4Ver() >= VER_UE4_VOLUME_SAMPLE_LOW_QUALITY_SUPPORT)
	{
		Ar << Sample.DirectionalLightShadowing;
	}
	
	return Ar;
}

FPrecomputedLightVolume::FPrecomputedLightVolume() :
	bInitialized(false),
	bAddedToScene(false),
	HighQualityLightmapOctree(FVector::ZeroVector, HALF_WORLD_MAX),
	LowQualityLightmapOctree(FVector::ZeroVector, HALF_WORLD_MAX),
	OctreeForRendering(NULL)
{}

FPrecomputedLightVolume::~FPrecomputedLightVolume()
{
	if (bInitialized)
	{
		const SIZE_T VolumeBytes = GetAllocatedBytes();
		DEC_DWORD_STAT_BY(STAT_PrecomputedLightVolumeMemory, VolumeBytes);
	}
}

FArchive& operator<<(FArchive& Ar,FPrecomputedLightVolume& Volume)
{
	if (Ar.IsCountingMemory())
	{
		const int32 AllocatedBytes = Volume.GetAllocatedBytes();
		Ar.CountBytes(AllocatedBytes, AllocatedBytes);
	}
	else if (Ar.IsLoading())
	{
		Ar << Volume.bInitialized;
		if (Volume.bInitialized)
		{
			FBox Bounds;
			Ar << Bounds;
			float SampleSpacing = 0.0f;
			Ar << SampleSpacing;
			Volume.Initialize(Bounds);
			TArray<FVolumeLightingSample> HighQualitySamples;
			// Deserialize samples as an array, and add them to the octree
			Ar << HighQualitySamples;

			if (FPlatformProperties::SupportsHighQualityLightmaps() 
				&& (GIsEditor || AllowHighQualityLightmaps()))
			{
				for(int32 SampleIndex = 0; SampleIndex < HighQualitySamples.Num(); SampleIndex++)
				{
					Volume.AddHighQualityLightingSample(HighQualitySamples[SampleIndex]);
				}
			}

			TArray<FVolumeLightingSample> LowQualitySamples;

			if (Ar.UE4Ver() >= VER_UE4_VOLUME_SAMPLE_LOW_QUALITY_SUPPORT)
			{
				Ar << LowQualitySamples;
			}

			if (FPlatformProperties::SupportsLowQualityLightmaps() 
				&& (GIsEditor || !AllowHighQualityLightmaps()))
			{
				for(int32 SampleIndex = 0; SampleIndex < LowQualitySamples.Num(); SampleIndex++)
				{
					Volume.AddLowQualityLightingSample(LowQualitySamples[SampleIndex]);
				}
			}

			Volume.FinalizeSamples();
		}
	}
	else if (Ar.IsSaving())
	{
		Ar << Volume.bInitialized;
		if (Volume.bInitialized)
		{
			Ar << Volume.Bounds;
			float SampleSpacing = 0.0f;
			Ar << SampleSpacing;

			TArray<FVolumeLightingSample> HighQualitySamples;

			if (!Ar.IsCooking() || Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::HighQualityLightmaps))
			{
				// Gather an array of samples from the octree
				for(FLightVolumeOctree::TConstIterator<> NodeIt(Volume.HighQualityLightmapOctree); NodeIt.HasPendingNodes(); NodeIt.Advance())
				{
					const FLightVolumeOctree::FNode& CurrentNode = NodeIt.GetCurrentNode();

					FOREACH_OCTREE_CHILD_NODE(ChildRef)
					{
						if(CurrentNode.HasChild(ChildRef))
						{
							NodeIt.PushChild(ChildRef);
						}
					}

					for (FLightVolumeOctree::ElementConstIt ElementIt(CurrentNode.GetElementIt()); ElementIt; ++ElementIt)
					{
						const FVolumeLightingSample& Sample = *ElementIt;
						HighQualitySamples.Add(Sample);
					}
				}
			}
			
			Ar << HighQualitySamples;

			TArray<FVolumeLightingSample> LowQualitySamples;

			if (!Ar.IsCooking() || Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::LowQualityLightmaps))
			{
				// Gather an array of samples from the octree
				for(FLightVolumeOctree::TConstIterator<> NodeIt(Volume.LowQualityLightmapOctree); NodeIt.HasPendingNodes(); NodeIt.Advance())
				{
					const FLightVolumeOctree::FNode& CurrentNode = NodeIt.GetCurrentNode();

					FOREACH_OCTREE_CHILD_NODE(ChildRef)
					{
						if(CurrentNode.HasChild(ChildRef))
						{
							NodeIt.PushChild(ChildRef);
						}
					}

					for (FLightVolumeOctree::ElementConstIt ElementIt(CurrentNode.GetElementIt()); ElementIt; ++ElementIt)
					{
						const FVolumeLightingSample& Sample = *ElementIt;
						LowQualitySamples.Add(Sample);
					}
				}
			}

			Ar << LowQualitySamples;
		}
	}
	return Ar;
}

/** Frees any previous samples, prepares the volume to have new samples added. */
void FPrecomputedLightVolume::Initialize(const FBox& NewBounds)
{
	InvalidateLightingCache();
	bInitialized = true;
	Bounds = NewBounds;
	// Initialize the octree based on the passed in bounds
	HighQualityLightmapOctree = FLightVolumeOctree(NewBounds.GetCenter(), NewBounds.GetExtent().GetMax());
	LowQualityLightmapOctree = FLightVolumeOctree(NewBounds.GetCenter(), NewBounds.GetExtent().GetMax());

	OctreeForRendering = AllowHighQualityLightmaps() ? &HighQualityLightmapOctree : &LowQualityLightmapOctree;
}

/** Adds a lighting sample. */
void FPrecomputedLightVolume::AddHighQualityLightingSample(const FVolumeLightingSample& NewHighQualitySample)
{
	check(bInitialized);
	HighQualityLightmapOctree.AddElement(NewHighQualitySample);
}

void FPrecomputedLightVolume::AddLowQualityLightingSample(const FVolumeLightingSample& NewLowQualitySample)
{
	check(bInitialized);
	LowQualityLightmapOctree.AddElement(NewLowQualitySample);
}

/** Shrinks the octree and updates memory stats. */
void FPrecomputedLightVolume::FinalizeSamples()
{
	check(bInitialized);
	// No more samples will be added, shrink octree node element arrays
	HighQualityLightmapOctree.ShrinkElements();
	LowQualityLightmapOctree.ShrinkElements();
	const SIZE_T VolumeBytes = GetAllocatedBytes();
	INC_DWORD_STAT_BY(STAT_PrecomputedLightVolumeMemory, VolumeBytes);
}

/** Invalidates anything produced by the last lighting build. */
void FPrecomputedLightVolume::InvalidateLightingCache()
{
	if (bInitialized)
	{
		check(!bAddedToScene);

		// Release existing samples
		const SIZE_T VolumeBytes = GetAllocatedBytes();
		DEC_DWORD_STAT_BY(STAT_PrecomputedLightVolumeMemory, VolumeBytes);
		HighQualityLightmapOctree.Destroy();
		LowQualityLightmapOctree.Destroy();
		OctreeForRendering = NULL;
		bInitialized = false;
	}
}

void FPrecomputedLightVolume::AddToScene(FSceneInterface* Scene)
{
	check(!bAddedToScene);
	bAddedToScene = true;

	if (bInitialized && Scene)
	{
		Scene->AddPrecomputedLightVolume(this);
	}
}

void FPrecomputedLightVolume::RemoveFromScene(FSceneInterface* Scene)
{
	if (bAddedToScene)
	{
		bAddedToScene = false;

		if (bInitialized && Scene)
		{
			Scene->RemovePrecomputedLightVolume(this);
		}
	}
}

void FPrecomputedLightVolume::InterpolateIncidentRadiancePoint(
		const FVector& WorldPosition, 
		float& AccumulatedWeight,
		float& AccumulatedDirectionalLightShadowing,
		FSHVectorRGB2& AccumulatedIncidentRadiance) const
{
	// Handle being called on a NULL volume, which can happen for a newly created level,
	// Or a volume that hasn't been initialized yet, which can happen if lighting hasn't been built yet.
	if (this && bInitialized)
	{
		FBoxCenterAndExtent BoundingBox( WorldPosition, FVector::ZeroVector );
		// Iterate over the octree nodes containing the query point.
		for (FLightVolumeOctree::TConstElementBoxIterator<> OctreeIt(*OctreeForRendering, BoundingBox);
			OctreeIt.HasPendingElements();
			OctreeIt.Advance())
		{
			const FVolumeLightingSample& VolumeSample = OctreeIt.GetCurrentElement();
			const float DistanceSquared = (VolumeSample.Position - WorldPosition).SizeSquared();
			const float RadiusSquared = FMath::Square(VolumeSample.Radius);

			if (DistanceSquared < RadiusSquared)
			{
				const float InvRadiusSquared = 1.0f / RadiusSquared;
				// Weight each sample with the fraction that Position is to the center of the sample, and inversely to the sample radius.
				// The weight goes to 0 when Position is on the bounding radius of the sample, so the interpolated result is continuous.
				// The sample's radius size is a factor so that smaller samples contribute more than larger, low detail ones.
				const float SampleWeight = (1.0f - DistanceSquared * InvRadiusSquared) * InvRadiusSquared;
				// Accumulate weighted results and the total weight for normalization later
				AccumulatedIncidentRadiance += VolumeSample.Lighting * SampleWeight;
				AccumulatedDirectionalLightShadowing += VolumeSample.DirectionalLightShadowing * SampleWeight;
				AccumulatedWeight += SampleWeight;
			}
		}
	}
}

/** Interpolates incident radiance to Position. */
void FPrecomputedLightVolume::InterpolateIncidentRadianceBlock(
	const FBoxCenterAndExtent& BoundingBox, 
	const FIntVector& QueryCellDimensions,
	const FIntVector& DestCellDimensions,
	const FIntVector& DestCellPosition,
	TArray<float>& AccumulatedWeights,
	TArray<FSHVectorRGB2>& AccumulatedIncidentRadiance) const
{
	// Handle being called on a NULL volume, which can happen for a newly created level,
	// Or a volume that hasn't been initialized yet, which can happen if lighting hasn't been built yet.
	if (this && bInitialized)
	{
		static TArray<const FVolumeLightingSample*> PotentiallyIntersectingSamples;
		PotentiallyIntersectingSamples.Reset(100);

		// Iterate over the octree nodes containing the query point.
		for (FLightVolumeOctree::TConstElementBoxIterator<> OctreeIt(*OctreeForRendering, BoundingBox);
			OctreeIt.HasPendingElements();
			OctreeIt.Advance())
		{
			PotentiallyIntersectingSamples.Add(&OctreeIt.GetCurrentElement());
		}
		
		for (int32 SampleIndex = 0; SampleIndex < PotentiallyIntersectingSamples.Num(); SampleIndex++)
		{
			const FVolumeLightingSample& VolumeSample = *PotentiallyIntersectingSamples[SampleIndex];
			
			for (int32 Z = 0; Z < QueryCellDimensions.Z; Z++)
			{
				for (int32 Y = 0; Y < QueryCellDimensions.Y; Y++)
				{
					for (int32 X = 0; X < QueryCellDimensions.X; X++)
					{
						const int32 LinearIndex = 
							(Z + DestCellPosition.Z) * DestCellDimensions.Y * DestCellDimensions.X 
							+ (Y + DestCellPosition.Y) * DestCellDimensions.X
							+ (X + DestCellPosition.X);

						const FVector WorldPosition = BoundingBox.Center - BoundingBox.Extent + FVector(X, Y, Z) / FVector(QueryCellDimensions) * BoundingBox.Extent * 2;	
						const float DistanceSquared = (VolumeSample.Position - WorldPosition).SizeSquared();
						const float RadiusSquared = FMath::Square(VolumeSample.Radius);

						if (DistanceSquared < RadiusSquared)
						{
							// Weight each sample with the fraction that Position is to the center of the sample, and inversely to the sample radius.
							// The weight goes to 0 when Position is on the bounding radius of the sample, so the interpolated result is continuous.
							// The sample's radius size is a factor so that smaller samples contribute more than larger, low detail ones.
							const float SampleWeight = (1.0f - DistanceSquared / RadiusSquared) / RadiusSquared;
							// Accumulate weighted results and the total weight for normalization later
							AccumulatedIncidentRadiance[LinearIndex] += VolumeSample.Lighting * SampleWeight;
							AccumulatedWeights[LinearIndex] += SampleWeight;
						}
					}
				}
			}
		}
	}
}

void FPrecomputedLightVolume::DebugDrawSamples(FPrimitiveDrawInterface* PDI, bool bDrawDirectionalShadowing) const
{
	for (FLightVolumeOctree::TConstElementBoxIterator<> OctreeIt(*OctreeForRendering, OctreeForRendering->GetRootBounds());
		OctreeIt.HasPendingElements();
		OctreeIt.Advance())
	{
		const FVolumeLightingSample& VolumeSample = OctreeIt.GetCurrentElement();
		const FLinearColor AverageColor = bDrawDirectionalShadowing
			? FLinearColor(VolumeSample.DirectionalLightShadowing, VolumeSample.DirectionalLightShadowing, VolumeSample.DirectionalLightShadowing)
			: VolumeSample.Lighting.CalcIntegral() / (FSHVector2::ConstantBasisIntegral * PI);
		PDI->DrawPoint(VolumeSample.Position, AverageColor, 10, SDPG_World);
	}
}

SIZE_T FPrecomputedLightVolume::GetAllocatedBytes() const
{
	SIZE_T NodeBytes = 0;

	for (FLightVolumeOctree::TConstIterator<> NodeIt(HighQualityLightmapOctree); NodeIt.HasPendingNodes(); NodeIt.Advance())
	{
		const FLightVolumeOctree::FNode& CurrentNode = NodeIt.GetCurrentNode();
		NodeBytes += sizeof(FLightVolumeOctree::FNode);
		NodeBytes += CurrentNode.GetElements().GetAllocatedSize();

		FOREACH_OCTREE_CHILD_NODE(ChildRef)
		{
			if(CurrentNode.HasChild(ChildRef))
			{
				NodeIt.PushChild(ChildRef);
			}
		}
	}

	for (FLightVolumeOctree::TConstIterator<> NodeIt(LowQualityLightmapOctree); NodeIt.HasPendingNodes(); NodeIt.Advance())
	{
		const FLightVolumeOctree::FNode& CurrentNode = NodeIt.GetCurrentNode();
		NodeBytes += sizeof(FLightVolumeOctree::FNode);
		NodeBytes += CurrentNode.GetElements().GetAllocatedSize();

		FOREACH_OCTREE_CHILD_NODE(ChildRef)
		{
			if(CurrentNode.HasChild(ChildRef))
			{
				NodeIt.PushChild(ChildRef);
			}
		}
	}

	return NodeBytes;
}

/**
 * Specialization for LightVolumeOctree elements
 */
template<>
void ElementsApplyOffset(const FVector& InOffset, FLightVolumeOctree::ElementArrayType& Elements)
{
	for (int32 i = 0; i < Elements.Num(); ++i)
	{
		Elements[i].Position+= InOffset;
	}
}

void FPrecomputedLightVolume::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Bounds.Min+= InOffset;
	Bounds.Max+= InOffset;
	HighQualityLightmapOctree.ApplyOffset(InOffset);
	LowQualityLightmapOctree.ApplyOffset(InOffset);
}
