// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SequencerMeshTrail.h"
#include "SequencerKeyActor.h"
#include "ViewportWorldInteraction.h"
#include "EditorWorldManager.h"
#include "Materials/MaterialInstance.h" 

ASequencerMeshTrail::ASequencerMeshTrail()
	: Super()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void ASequencerMeshTrail::Cleanup()
{
	// Destroy all the key actors this trail created
	for (auto& KeyMesh : KeyMeshActors)
	{
		KeyMesh.Get<1>()->Destroy();
	}
	this->Destroy();
}

void ASequencerMeshTrail::AddKeyMeshActor(float KeyTime, const FTransform KeyTransform, class UMovieScene3DTransformSection* TrackSection)
{
	ASequencerKeyActor* KeyMeshActor = nullptr;
	UStaticMesh* KeyEditorMesh = nullptr;
	UMaterialInstance* KeyEditorMaterial = nullptr;

	TTuple<float, ASequencerKeyActor*>* KeyPtr = KeyMeshActors.FindByPredicate([KeyTime](const TTuple<float, ASequencerKeyActor*> InKey)
	{
		return FMath::IsNearlyEqual(KeyTime, InKey.Get<0>());
	});

	// If we don't currently have an actor for this time, create one
	if (KeyPtr == nullptr)
	{
		UViewportWorldInteraction* ViewportWorldInteraction = GEditor->GetEditorWorldManager()->GetEditorWorldWrapper(GEditor->GetActiveViewport()->GetClient()->GetWorld())->GetViewportWorldInteraction();
		KeyMeshActor = ViewportWorldInteraction->SpawnTransientSceneActor<ASequencerKeyActor>(TEXT("KeyMesh"), false);
		KeyMeshActor->SetMobility(EComponentMobility::Movable);
		KeyMeshActor->SetActorTransform(KeyTransform);
		KeyMeshActor->SetKeyData(TrackSection, KeyTime);
		KeyMeshActor->SetOwner(this);
		TTuple<float, ASequencerKeyActor*> NewKey = MakeTuple(KeyTime, KeyMeshActor);
		KeyMeshActors.Add(NewKey);
	}
	else
	{
		// Just update the transform
		KeyPtr->Get<1>()->SetActorTransform(KeyTransform);
	}
}

void ASequencerMeshTrail::AddFrameMeshComponent(const float FrameTime, const FTransform FrameTransform)
{
	UStaticMeshComponent* FrameMeshComponent = nullptr;
	UStaticMesh* KeyEditorMesh = nullptr;
	UMaterialInterface* KeyEditorMaterial = nullptr;

	TTuple<float, UStaticMeshComponent*>* FramePtr = FrameMeshComponents.FindByPredicate([FrameTime](const TTuple<float, UStaticMeshComponent*> InFrame)
	{
		return FMath::IsNearlyEqual(FrameTime, InFrame.Get<0>());
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
		TTuple<float, UStaticMeshComponent*> NewFrame = MakeTuple(FrameTime, FrameMeshComponent);
		FrameMeshComponents.Add(NewFrame);
	}
	else
	{
		// Just update the transform
		FramePtr->Get<1>()->SetWorldTransform(FrameTransform);
	}
}
