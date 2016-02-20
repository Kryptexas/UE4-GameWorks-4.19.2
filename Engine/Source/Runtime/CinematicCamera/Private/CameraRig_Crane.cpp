// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CinematicCameraPrivate.h"
#include "CameraRig_Crane.h"

#define LOCTEXT_NAMESPACE "CameraRig_Crane"

ACameraRig_Crane::ACameraRig_Crane(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	CraneYaw = 0.f;
	CranePitch = 0.f;
	CraneArmLength = 500.f;

	TransformComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TransformComponent"));

	// Make the scene component the root component
	RootComponent = TransformComponent;
	
	// Setup camera defaults
	CraneYawControl = CreateDefaultSubobject<USceneComponent>(TEXT("CraneYawControl"));
	CraneYawControl->AttachParent = TransformComponent;
	CraneYawControl->RelativeLocation = FVector(0.f, 0.f, 100.f);			// pivot height off the ground
	CraneYawControl->RelativeRotation = FRotator(0.f, CraneYaw, 0.f);

	CranePitchControl = CreateDefaultSubobject<USceneComponent>(TEXT("CranePitchControl"));
	CranePitchControl->AttachParent = CraneYawControl;
	CranePitchControl->RelativeRotation = FRotator(CranePitch, 0.f, 0.f);

	CraneCameraMount = CreateDefaultSubobject<USceneComponent>(TEXT("CraneCameraMount"));
	CraneCameraMount->AttachParent = CranePitchControl;
	CraneCameraMount->RelativeLocation = FVector(CraneArmLength, 0.f, -35.f);			// negative z == underslung mount

	// preview meshes
	if (!IsRunningCommandlet() && !IsRunningDedicatedServer())
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CraneBaseMesh(TEXT("/Engine/BasicShapes/Cone"));
		PreviewMesh_CraneBase = CreateOptionalDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh_CraneBase"));
		if (PreviewMesh_CraneBase)
		{
			PreviewMesh_CraneBase->SetStaticMesh(CraneBaseMesh.Object);
			PreviewMesh_CraneBase->AlwaysLoadOnClient = false;
			PreviewMesh_CraneBase->AlwaysLoadOnServer = false;
			PreviewMesh_CraneBase->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			PreviewMesh_CraneBase->bHiddenInGame = true;
			PreviewMesh_CraneBase->CastShadow = false;
			PreviewMesh_CraneBase->PostPhysicsComponentTick.bCanEverTick = false;

			PreviewMesh_CraneBase->AttachParent = RootComponent;		// sibling of yawcontrol
			PreviewMesh_CraneBase->RelativeLocation = FVector(0.f, 0.f, 50.f);
		}

		static ConstructorHelpers::FObjectFinder<UStaticMesh> CraneArmMesh(TEXT("/Engine/BasicShapes/Cube"));
		PreviewMesh_CraneArm = CreateOptionalDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh_CraneArm"));
		if (PreviewMesh_CraneArm)
		{
			PreviewMesh_CraneArm->SetStaticMesh(CraneArmMesh.Object);
			PreviewMesh_CraneArm->AlwaysLoadOnClient = false;
			PreviewMesh_CraneArm->AlwaysLoadOnServer = false;
			PreviewMesh_CraneArm->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			PreviewMesh_CraneArm->bHiddenInGame = true;
			PreviewMesh_CraneArm->CastShadow = false;
			PreviewMesh_CraneArm->PostPhysicsComponentTick.bCanEverTick = false;
			
			PreviewMesh_CraneArm->AttachParent = CranePitchControl;		// sibling of the mount
			PreviewMesh_CraneArm->RelativeScale3D = FVector(0.2f, 0.2f, 0.2f);
		}

		UpdatePreviewMeshes();
	}
}

static const float CraneArmMesh_CounterweightOverhang = 100.f;

void ACameraRig_Crane::UpdatePreviewMeshes()
{
	if (PreviewMesh_CraneArm)
	{
		// note to explain the math here:
		// crane arm mesh is current a 100x100 cube. we want it to overhang the base pivot by 100cm
		// (chosen arbitrarily)

		float const TotalCraneArmMeshLength = CraneArmMesh_CounterweightOverhang + CraneArmLength;
		float const CraneArmXScale = TotalCraneArmMeshLength / 100.f;
		float const CraneArmXOffset = TotalCraneArmMeshLength * 0.5f - CraneArmMesh_CounterweightOverhang;

		FVector NewRelScale3D = PreviewMesh_CraneArm->RelativeScale3D;
		NewRelScale3D.X = CraneArmXScale;

		FVector NewRelLoc = PreviewMesh_CraneArm->RelativeLocation;
		NewRelLoc.X = CraneArmXOffset;

		PreviewMesh_CraneArm->SetRelativeScale3D(NewRelScale3D);
		PreviewMesh_CraneArm->SetRelativeLocation(NewRelLoc);
	}
}

void ACameraRig_Crane::UpdateCraneComponents()
{
	FRotator NewYawControlRot = CraneYawControl->RelativeRotation;
	NewYawControlRot.Yaw = CraneYaw;
	CraneYawControl->SetRelativeRotation(FRotator(0.f, CraneYaw, 0.f));

	FRotator NewPitchControlRot = CranePitchControl->RelativeRotation;
	NewPitchControlRot.Pitch = CranePitch;
	CranePitchControl->SetRelativeRotation(FRotator(CranePitch, 0.f, 0.f));

	FVector NewCameraMountLoc = CraneCameraMount->RelativeLocation;
	NewCameraMountLoc.X = CraneArmLength;
	CraneCameraMount->SetRelativeLocation(NewCameraMountLoc);

	// zero the pitch from the camera mount component
	// this effectively gives us bAbsoluteRotation for only pitch component of an attached camera
	FRotator NewCameraMountWorldRot = CraneCameraMount->GetComponentRotation();
	NewCameraMountWorldRot.Pitch = 0.f;
	CraneCameraMount->SetWorldRotation(NewCameraMountWorldRot);

	UpdatePreviewMeshes();
}


void ACameraRig_Crane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// feed exposed API into underlying components
	UpdateCraneComponents();
}

#if WITH_EDITOR
void ACameraRig_Crane::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateCraneComponents();
}

USceneComponent* ACameraRig_Crane::GetDefaultAttachComponent() const
{
	return CraneCameraMount;
}
#endif // WITH_EDITOR

bool ACameraRig_Crane::ShouldTickIfViewportsOnly() const
{
	return true;
}


#undef LOCTEXT_NAMESPACE