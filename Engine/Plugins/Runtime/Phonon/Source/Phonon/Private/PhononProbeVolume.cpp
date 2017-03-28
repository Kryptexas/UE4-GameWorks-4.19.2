//
// Copyright (C) Impulsonic, Inc. All rights reserved.
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
	, HorizontalSpacing(1.0)
	, HeightAboveFloor(1.5)
{
}

void APhononProbeVolume::GenerateProbes()
{
#if WITH_EDITOR

	// Clear out old data
	ProbeBoxData.Empty();
	ProbeBatchData.Empty();

	// Load the scene
	UWorld* World = GEditor->LevelViewportClients[0]->GetWorld();
	IPLhandle PhononScene = nullptr;
	TArray<IPLhandle> PhononStaticMeshes;
	Phonon::LoadScene(World, &PhononScene, &PhononStaticMeshes);

	// Compute bounding box in Phonon coords
	IPLhandle ProbeBox = nullptr;
	FVector BoxCenter, BoxExtents;
	this->Brush->Bounds.GetBox().GetCenterAndExtents(BoxCenter, BoxExtents);
	BoxExtents *= this->GetTransform().GetScale3D();
	FVector Center = this->GetTransform().GetLocation();
	IPLVector3 MinExtent = Phonon::UnrealToPhononIPLVector3(Center - BoxExtents);
	IPLVector3 MaxExtent = Phonon::UnrealToPhononIPLVector3(Center + BoxExtents);
	
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
	ProbePlacementParameters.heightAboveFloor = HeightAboveFloor;
	ProbePlacementParameters.spacing = HorizontalSpacing;
	ProbePlacementParameters.maxOctreeDepth = 0;
	ProbePlacementParameters.maxOctreeTriangles = 0;

	// Create probe box, generate probes
	iplCreateProbeBox(PhononScene, ProbeBoxExtents, ProbePlacementParameters, nullptr, &ProbeBox);

	// Get probe locations/radii
	IPLint32 NumProbes = iplGetProbeSpheres(ProbeBox, nullptr);
	TArray<IPLSphere> ProbeSpheres;
	ProbeSpheres.SetNumUninitialized(NumProbes);
	iplGetProbeSpheres(ProbeBox, ProbeSpheres.GetData());

	for (UPhononProbeComponent * PhononProbeComponent : PhononProbeComponents)
	{
		PhononProbeComponent->UnregisterComponent();
	}
	PhononProbeComponents.Empty();

	IPLhandle ProbeBatch = nullptr;
	iplCreateProbeBatch(&ProbeBatch);

	// Spawn probe components
	FRotator DefaultRotation(0, 0, 0);
	for (IPLint32 i = 0; i < NumProbes; ++i)
	{ 
		FVector ProbeLocation = Phonon::PhononToUnrealFVector(Phonon::FVectorFromIPLVector3(ProbeSpheres[i].center));
		UE_LOG(LogPhonon, Log, TEXT("Spawning at location %f %f %f"), ProbeLocation.X, ProbeLocation.Y, ProbeLocation.Z);

		FName ProbeName = FName(*FString::Printf(TEXT("PhononProbeComponent%d"), i));
		UPhononProbeComponent* PhononProbeComponent = NewObject<UPhononProbeComponent>(this, ProbeName);

		PhononProbeComponent->RegisterComponent();
		PhononProbeComponent->SetWorldLocation(ProbeLocation);
		PhononProbeComponent->SetWorldRotation(DefaultRotation);
		PhononProbeComponent->InfluenceRadius = ProbeSpheres[i].radius;
		PhononProbeComponent->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
		PhononProbeComponents.Add(PhononProbeComponent);

		iplAddProbeToBatch(ProbeBatch, ProbeBox, i);
	}

	// Save probe box data
	IPLint32 ProbeBoxDataSize = iplSaveProbeBox(ProbeBox, nullptr);
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
	
	for (IPLhandle PhononStaticMesh : PhononStaticMeshes)
	{
		iplDestroyStaticMesh(&PhononStaticMesh);
	}
	iplDestroyScene(&PhononScene);

#endif
}