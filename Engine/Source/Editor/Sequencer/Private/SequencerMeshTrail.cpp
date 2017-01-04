// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SequencerMeshTrail.h"
#include "SequencerKeyActor.h"
#include "ViewportWorldInteraction.h"
#include "EditorWorldExtension.h"
#include "Materials/MaterialInstance.h" 
#include "Components/BillboardComponent.h"

ASequencerMeshTrail::ASequencerMeshTrail()
	: Super()
{
	UBillboardComponent* PrimitiveRootComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("RootComponent"));
	PrimitiveRootComponent->bSelectable = false;
	PrimitiveRootComponent->SetVisibility(false, false);
	SetRootComponent(PrimitiveRootComponent);
}

void ASequencerMeshTrail::Cleanup()
{
	// Destroy all the key actors this trail created
	for (FKeyActorData KeyMesh : KeyMeshActors)
	{
		KeyMesh.KeyActor->Destroy();
	}
	this->Destroy();
}

void ASequencerMeshTrail::AddKeyMeshActor(float KeyTime, const FTransform KeyTransform, class UMovieScene3DTransformSection* TrackSection)
{
	ASequencerKeyActor* KeyMeshActor = nullptr;
	UStaticMesh* KeyEditorMesh = nullptr;
	UMaterialInstance* KeyEditorMaterial = nullptr;

	FKeyActorData* KeyPtr = KeyMeshActors.FindByPredicate([KeyTime](const FKeyActorData InKey)
	{
		return FMath::IsNearlyEqual(KeyTime, InKey.Time);
	});

	// If we don't currently have an actor for this time, create one
	if (KeyPtr == nullptr)
	{
		KeyMeshActor = UViewportWorldInteraction::SpawnTransientSceneActor<ASequencerKeyActor>(GetWorld(), TEXT("KeyMesh"), false);
		KeyMeshActor->SetMobility(EComponentMobility::Movable);
		KeyMeshActor->SetActorTransform(KeyTransform);
		KeyMeshActor->SetKeyData(TrackSection, KeyTime);
		KeyMeshActor->SetOwner(this);
		FKeyActorData NewKey = FKeyActorData(KeyTime, KeyMeshActor);
		KeyMeshActors.Add(NewKey);
	}
	else
	{
		// Just update the transform
		KeyPtr->KeyActor->SetActorTransform(KeyTransform);
	}
}

void ASequencerMeshTrail::AddFrameMeshComponent(const float FrameTime, const FTransform FrameTransform)
{
	UStaticMeshComponent* FrameMeshComponent = nullptr;
	UStaticMesh* KeyEditorMesh = nullptr;
	UMaterialInterface* KeyEditorMaterial = nullptr;

	FFrameComponentData* FramePtr = FrameMeshComponents.FindByPredicate([FrameTime](const FFrameComponentData InFrame)
	{
		return FMath::IsNearlyEqual(FrameTime, InFrame.Time);
	});

	// If we don't currently have a component for this time, create one
	if (FramePtr == nullptr)
	{
		FrameMeshComponent = NewObject<UStaticMeshComponent>(this);
		this->AddOwnedComponent(FrameMeshComponent);
		FrameMeshComponent->RegisterComponent();

		KeyEditorMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
		check(KeyEditorMesh != nullptr);
		KeyEditorMaterial = LoadObject<UMaterialInstance>(nullptr, TEXT("/Engine/VREditor/LaserPointer/LaserPointerMaterial_Inst"));
		check(KeyEditorMaterial != nullptr);
		FrameMeshComponent->SetStaticMesh(KeyEditorMesh);
		FrameMeshComponent->SetMaterial(0, KeyEditorMaterial);
		FrameMeshComponent->SetWorldTransform(FrameTransform);
		FrameMeshComponent->SetMobility(EComponentMobility::Movable);
		FrameMeshComponent->bSelectable = false;
		FFrameComponentData NewFrame = FFrameComponentData(FrameTime, FrameMeshComponent);
		FrameMeshComponents.Add(NewFrame);
	}
	else
	{
		// Just update the transform
		FramePtr->FrameComponent->SetWorldTransform(FrameTransform);
	}
}
