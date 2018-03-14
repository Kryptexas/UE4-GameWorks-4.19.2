// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityBillboard.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "MixedRealityUtilLibrary.h" // for GetHMDCameraComponent()
#include "MixedRealityCaptureComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstance.h"
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h" // for GetDeviceWorldPose()
#include "IXRCamera.h"

/* MixedRealityProjection_Impl
 *****************************************************************************/

namespace MixedRealityProjection_Impl
{
	static APawn* FindTargetPlayer(const AMixedRealityProjectionActor* ProjectionActor)
	{
		APawn* TargetPawn = nullptr;

		USceneComponent* AttacheRoot = ProjectionActor->GetRootComponent();
		USceneComponent* AttachParent = ensure(AttacheRoot) ? AttacheRoot->GetAttachParent() : nullptr;
		AActor* OwningActor = AttachParent ? AttachParent->GetOwner() : nullptr;

		if (UWorld* TargetWorld = ProjectionActor->GetWorld())
		{
			APawn* FallbackPawn = nullptr;

			const TArray<ULocalPlayer*>& LocalPlayers = GEngine->GetGamePlayers(TargetWorld);
			for (ULocalPlayer* Player : LocalPlayers)
			{
				APlayerController* Controller = Player->GetPlayerController(TargetWorld);
				if (Controller == nullptr)
				{
					continue;
				}

				APawn* PlayerPawn = Controller->GetPawn();
				if (PlayerPawn == nullptr)
				{
					continue;
				}
				else if (FallbackPawn == nullptr)
				{
					FallbackPawn = PlayerPawn;
				}

				const bool bIsOwningPlayer =
					(OwningActor && (OwningActor->IsOwnedBy(Controller) || OwningActor->IsOwnedBy(PlayerPawn))) ||
					(ProjectionActor->IsOwnedBy(Controller) || ProjectionActor->IsOwnedBy(PlayerPawn));

				if (bIsOwningPlayer)
				{
					TargetPawn = PlayerPawn;
					break;
				}
			}

			if (!TargetPawn)
			{
				TargetPawn = FallbackPawn;
			}
		}

		return TargetPawn;
	}
}

//------------------------------------------------------------------------------
UMixedRealityBillboard::UMixedRealityBillboard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	if (AActor* Owner = GetOwner())
	{
		AddTickPrerequisiteActor(Owner);
	}
}

//------------------------------------------------------------------------------
void UMixedRealityBillboard::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	FVector ViewOffset = FVector::ForwardVector * (GNearClippingPlane + 0.01f);
	if (GEngine->XRSystem.IsValid())
	{
		IIdentifiableXRDevice* HMDDevice = GEngine->XRSystem->GetXRCamera().Get();
		if (HMDDevice != nullptr)
		{
			FVector  HMDPos;
			FRotator HMDRot;

			bool bHasTracking = false, bHasPositionalTracking = false;
			UHeadMountedDisplayFunctionLibrary::GetDeviceWorldPose(/*WorldContext =*/this, HMDDevice, /*[out]*/bHasTracking, /*[out]*/HMDRot, /*[out]*/bHasPositionalTracking, /*[out]*/HMDPos);

			if (bHasPositionalTracking)
			{
				// ASSUMPTION: this is positioned (attached) directly relative to some view
				USceneComponent* MRViewComponent = GetAttachParent();
				if (ensure(MRViewComponent))
				{
					const FVector ViewWorldPos = MRViewComponent->GetComponentLocation();
					const FVector ViewToHMD = (HMDPos - ViewWorldPos);
					const FVector ViewFacingDir = MRViewComponent->GetForwardVector();

					const float HMDDepth = (/*DotProduct: */ViewFacingDir | ViewToHMD);
					ViewOffset = FVector::ForwardVector * HMDDepth;
				}
			}
		}
	}

	SetRelativeLocationAndRotation(ViewOffset, FRotator::ZeroRotator);
}

/* AMixedRealityBillboardActor
 *****************************************************************************/

//------------------------------------------------------------------------------
AMixedRealityProjectionActor::AMixedRealityProjectionActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial;
		float DefaultAspectRatio;

		FConstructorStatics()
			: DefaultMaterial(TEXT("/MixedRealityFramework/M_MRCamSrcProcessing"))
			, DefaultAspectRatio(16.f / 9.f)
		{}
	};
	static FConstructorStatics ConstructorStatics;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	UWorld* MyWorld = GetWorld();
	const bool bIsEditorInst = MyWorld && (MyWorld->WorldType == EWorldType::Editor || MyWorld->WorldType == EWorldType::EditorPreview);
	
	ProjectionComponent = CreateDefaultSubobject<UMixedRealityBillboard>(TEXT("MR_ProjectionMesh"));
	ProjectionComponent->SetupAttachment(RootComponent);
	ProjectionComponent->AddElement(ConstructorStatics.DefaultMaterial.Object, 
		/*DistanceToOpacityCurve =*/nullptr, 
		/*bSizeIsInScreenSpace =*/true, 
		/*BaseSizeX =*/1.0f, 
		/*BaseSizeY=*/ConstructorStatics.DefaultAspectRatio, 
		/*DistanceToSizeCurve =*/nullptr);
	ProjectionComponent->CastShadow = false;
	ProjectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// expects that this actor (or one of its owners) is used as the scene's view 
	// actor (hidden in the editor via UMixedRealityBillboard::GetHiddenEditorViews)
	ProjectionComponent->bOnlyOwnerSee = !bIsEditorInst;
	ProjectionComponent->SetComponentTickEnabled(false);
	ProjectionComponent->SetRelativeLocation(FVector::ForwardVector * (GNearClippingPlane + 0.01f));
}

//------------------------------------------------------------------------------
void AMixedRealityProjectionActor::BeginPlay()
{
	Super::BeginPlay();

	UWorld* MyWorld = GetWorld();
	for (FConstPlayerControllerIterator PlayerIt = MyWorld->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
	{
		if (APlayerController* PlayerController = PlayerIt->Get())
		{
			PlayerController->HiddenPrimitiveComponents.AddUnique(ProjectionComponent);
		}
	}

	ProjectionComponent->SetRelativeLocation(FVector::ForwardVector * (GNearClippingPlane + 0.01f));
}

//------------------------------------------------------------------------------
void AMixedRealityProjectionActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

//------------------------------------------------------------------------------
void AMixedRealityProjectionActor::SetProjectionMaterial(UMaterialInterface* VidProcessingMat)
{
	ProjectionComponent->SetMaterial(/*ElementIndex =*/0, VidProcessingMat);
}

//------------------------------------------------------------------------------
void AMixedRealityProjectionActor::SetProjectionAspectRatio(const float NewAspectRatio)
{
	FMaterialSpriteElement& Sprite = ProjectionComponent->Elements[0];
	if (Sprite.BaseSizeY != NewAspectRatio)
	{
		Sprite.BaseSizeY = NewAspectRatio;
		ProjectionComponent->MarkRenderStateDirty();
	}	
}

//------------------------------------------------------------------------------
FVector AMixedRealityProjectionActor::GetTargetPosition() const
{
	if (AttachTarget.IsValid())
	{
		return AttachTarget->GetComponentLocation();
	}
	return GetActorLocation();
}

//------------------------------------------------------------------------------
void AMixedRealityProjectionActor::SetDepthTarget(const APawn* PlayerPawn)
{
	if (AttachTarget.IsValid())
	{
		RemoveTickPrerequisiteComponent(AttachTarget.Get());
	}

	UCameraComponent* HMDCam = UMixedRealityUtilLibrary::GetHMDCameraComponent(PlayerPawn);
	if (HMDCam)
	{
		AttachTarget = HMDCam;
	}
	else if (PlayerPawn)
	{
		AttachTarget = PlayerPawn->GetRootComponent();
	}
	else
	{
		AttachTarget.Reset();
	}
	RefreshTickState();
}

//------------------------------------------------------------------------------
void AMixedRealityProjectionActor::RefreshTickState()
{
	if (USceneComponent* AttachParent = RootComponent->GetAttachParent())
	{
		AddTickPrerequisiteComponent(AttachParent);
	}

	const bool bValidAttachTarget = AttachTarget.IsValid();
	if (bValidAttachTarget)
	{
		AddTickPrerequisiteComponent(AttachTarget.Get());
	}
	ProjectionComponent->SetComponentTickEnabled(bValidAttachTarget);
}

