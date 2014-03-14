// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ParticleDefinitions.h"

#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "UObjectToken.h"

#define LOCTEXT_NAMESPACE "CameraActor"

//////////////////////////////////////////////////////////////////////////
// ACameraActor

ACameraActor::ACameraActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Setup camera defaults
	CameraComponent = PCIP.CreateDefaultSubobject<UCameraComponent>(this, TEXT("CameraComponent"));
	CameraComponent->FieldOfView = 90.0f;
	CameraComponent->bConstrainAspectRatio = true;
	CameraComponent->AspectRatio = 1.777778f;
	CameraComponent->PostProcessBlendWeight = 1.0f;

	// Make the camera component the root component
	RootComponent = CameraComponent;

	// Initialize deprecated properties (needed for backwards compatibility due to delta serialization)
	FOVAngle_DEPRECATED = 90.0f;
	bConstrainAspectRatio_DEPRECATED = true;
	AspectRatio_DEPRECATED = 1.777778f;
	PostProcessBlendWeight_DEPRECATED = 1.0f;
	// End of deprecated property initialization
}

void ACameraActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if ((Ar.UE4Ver() < VER_UE4_CAMERA_ACTOR_USING_CAMERA_COMPONENT) && Ar.IsLoading())
	{
		CameraComponent->bConstrainAspectRatio = bConstrainAspectRatio_DEPRECATED;
		CameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
		CameraComponent->AspectRatio = AspectRatio_DEPRECATED;
		CameraComponent->FieldOfView = FOVAngle_DEPRECATED;
		CameraComponent->PostProcessBlendWeight = PostProcessBlendWeight_DEPRECATED;
		CameraComponent->PostProcessSettings = PostProcessSettings_DEPRECATED;
	}
}

void ACameraActor::PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph)
{
	if (GetLinkerUE4Version() < VER_UE4_CAMERA_ACTOR_USING_CAMERA_COMPONENT)
	{
		USceneComponent* OldRoot = RootComponent;
		USceneComponent* OldAttachParent = OldRoot->AttachParent;

		Super::PostLoadSubobjects(OuterInstanceGraph);

		CameraComponent->AttachParent = OldAttachParent;
		OldRoot->AttachParent = NULL;
	}
	else
	{
		Super::PostLoadSubobjects(OuterInstanceGraph);
	}
}

#undef LOCTEXT_NAMESPACE
