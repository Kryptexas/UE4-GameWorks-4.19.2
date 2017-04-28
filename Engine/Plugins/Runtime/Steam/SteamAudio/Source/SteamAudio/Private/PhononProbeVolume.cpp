//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "PhononProbeVolume.h"

#include "PhononProbeComponent.h"
#include "PhononScene.h"
#include "PhononCommon.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditorViewport.h"
#endif

#include <algorithm>

APhononProbeVolume::APhononProbeVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, HorizontalSpacing(400.0f)
	, HeightAboveFloor(150.0f)
	, NumProbes(0)
{
	FRotator DefaultRotation(0, 0, 0);
	PhononProbeComponent = CreateDefaultSubobject<UPhononProbeComponent>(TEXT("PhononProbeComponent0"));
	PhononProbeComponent->SetWorldLocation(this->GetActorLocation());
	PhononProbeComponent->SetWorldRotation(DefaultRotation);
	PhononProbeComponent->SetupAttachment(this->GetRootComponent());
}

#if WITH_EDITOR
void APhononProbeVolume::PlaceProbes(IPLhandle PhononScene, IPLProbePlacementProgressCallback ProbePlacementCallback, 
	TArray<IPLSphere>& ProbeSpheres)
{
	// Clear out old data
	ProbeBoxData.Empty();
	ProbeBatchData.Empty();

	// Compute bounding box in Phonon coords
	IPLhandle ProbeBox = nullptr;
	FVector BoxCenter, BoxExtents;
	this->Brush->Bounds.GetBox().GetCenterAndExtents(BoxCenter, BoxExtents);
	BoxExtents *= this->GetTransform().GetScale3D();
	FVector Center = this->GetTransform().GetLocation();
	IPLVector3 MinExtent = SteamAudio::UnrealToPhononIPLVector3(Center - BoxExtents);
	IPLVector3 MaxExtent = SteamAudio::UnrealToPhononIPLVector3(Center + BoxExtents);
	
	if (MinExtent.x > MaxExtent.x)
		std::swap(MinExtent.x, MaxExtent.x);

	if (MinExtent.y > MaxExtent.y)
		std::swap(MinExtent.y, MaxExtent.y);

	if (MinExtent.z > MaxExtent.z)
		std::swap(MinExtent.z, MaxExtent.z);

	IPLBox ProbeBoxExtents;
	ProbeBoxExtents.minCoordinates = MinExtent;
	ProbeBoxExtents.maxCoordinates = MaxExtent;

	// Configure placement parameters
	IPLProbePlacementParams ProbePlacementParameters;
	ProbePlacementParameters.placement = PlacementStrategy == EPhononProbePlacementStrategy::CENTROID ? IPL_PLACEMENT_CENTROID : IPL_PLACEMENT_UNIFORMFLOOR;
	ProbePlacementParameters.heightAboveFloor = HeightAboveFloor * SteamAudio::SCALEFACTOR;
	ProbePlacementParameters.spacing = HorizontalSpacing * SteamAudio::SCALEFACTOR;
	ProbePlacementParameters.maxOctreeDepth = 0;
	ProbePlacementParameters.maxOctreeTriangles = 0;

	// Create probe box, generate probes
	iplCreateProbeBox(PhononScene, ProbeBoxExtents, ProbePlacementParameters, ProbePlacementCallback, &ProbeBox);

	// Get probe locations/radii
	NumProbes = iplGetProbeSpheres(ProbeBox, nullptr);
	ProbeSpheres.SetNumUninitialized(NumProbes);
	iplGetProbeSpheres(ProbeBox, ProbeSpheres.GetData());

	IPLhandle ProbeBatch = nullptr;
	iplCreateProbeBatch(&ProbeBatch);

	for (IPLint32 i = 0; i < NumProbes; ++i)
	{ 
		iplAddProbeToBatch(ProbeBatch, ProbeBox, i);
	}

	// Save probe box data
	ProbeBoxDataSize = iplSaveProbeBox(ProbeBox, nullptr);
	ProbeBoxData.SetNumUninitialized(ProbeBoxDataSize);
	iplSaveProbeBox(ProbeBox, ProbeBoxData.GetData());

	// Save probe batch data
	iplFinalizeProbeBatch(ProbeBatch);
	IPLint32 ProbeBatchDataSize = iplSaveProbeBatch(ProbeBatch, nullptr);
	ProbeBatchData.SetNumUninitialized(ProbeBatchDataSize);
	iplSaveProbeBatch(ProbeBatch, ProbeBatchData.GetData());

	// Clean up
	iplDestroyProbeBox(&ProbeBox);
	iplDestroyProbeBatch(&ProbeBatch);
}

bool APhononProbeVolume::CanEditChange(const UProperty* InProperty) const
{
	const bool ParentVal = Super::CanEditChange(InProperty);

	if ((InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(APhononProbeVolume, HorizontalSpacing)) ||
		(InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(APhononProbeVolume, HeightAboveFloor)))
	{
		return ParentVal && PlacementStrategy == EPhononProbePlacementStrategy::UNIFORM_FLOOR;
	}

	return ParentVal;
}
#endif

void APhononProbeVolume::UpdateProbeBoxData(IPLhandle ProbeBox)
{
	// Update probe box serialized data
	ProbeBoxDataSize = iplSaveProbeBox(ProbeBox, nullptr);
	ProbeBoxData.SetNumUninitialized(ProbeBoxDataSize);
	iplSaveProbeBox(ProbeBox, ProbeBoxData.GetData());

	// Update probe batch serialized data
	IPLhandle ProbeBatch = nullptr;
	iplCreateProbeBatch(&ProbeBatch);

	NumProbes = iplGetProbeSpheres(ProbeBox, nullptr);
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

uint8* APhononProbeVolume::GetProbeBoxData()
{
	return ProbeBoxData.GetData();
}

const int32 APhononProbeVolume::GetProbeBoxDataSize() const
{
	return ProbeBoxData.Num();
}

uint8* APhononProbeVolume::GetProbeBatchData()
{
	return ProbeBatchData.GetData();
}

const int32 APhononProbeVolume::GetProbeBatchDataSize() const
{
	return ProbeBatchData.Num();
}