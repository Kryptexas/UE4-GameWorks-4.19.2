/* Copyright 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Classes/GoogleVRMotionControllerComponent.h"
#include "GoogleVRController.h"
#include "Classes/GoogleVRPointerInputComponent.h"
#include "Classes/GoogleVRControllerFunctionLibrary.h"
#include "MotionControllerComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/WorldSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoogleVRMotionController, Log, All);

const FVector UGoogleVRMotionControllerComponent::TOUCHPAD_POINT_DIMENSIONS = FVector(0.01f, 0.01f, 0.0004f);

UGoogleVRMotionControllerComponent::UGoogleVRMotionControllerComponent()
: ControllerMesh(nullptr)
, ControllerTouchPointMesh(nullptr)
, IdleMaterial(nullptr)
, TouchpadMaterial(nullptr)
, AppMaterial(nullptr)
, SystemMaterial(nullptr)
, ControllerTouchPointMaterial(nullptr)
, ControllerReticleMaterial(nullptr)
, ParameterCollection(nullptr)
, LaserParticleSystem(nullptr)
, LaserDistanceMax(0.75f)
, ReticleDistanceMin(0.45f)
, ReticleDistanceMax(2.5f)
, ReticleSize(0.05f)
, EnterRadiusCoeff(0.1f)
, ExitRadiusCoeff(0.2f)
, RequireInputComponent(true)
, TranslucentSortPriority(1)
, PlayerController(nullptr)
, MotionControllerComponent(nullptr)
, ControllerMeshComponent(nullptr)
, ControllerTouchPointMeshComponent(nullptr)
, PointerContainerComponent(nullptr)
, LaserParticleSystemComponent(nullptr)
, ReticleBillboardComponent(nullptr)
, TouchMeshScale(FVector::ZeroVector)
, bAreSubComponentsEnabled(false)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;

	ControllerMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/GoogleVRController/ControllerMesh")));
	ControllerTouchPointMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Cylinder")));
	IdleMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/GoogleVRController/ControllerIdleMaterial")));
	TouchpadMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/GoogleVRController/ControllerPadMaterial")));
	AppMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/GoogleVRController/ControllerAppMaterial")));
	SystemMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/GoogleVRController/ControllerSysMaterial")));
	ControllerTouchPointMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/GoogleVRController/TouchMaterial")));
	ControllerReticleMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/GoogleVRController/ControllerRetMaterial")));
	ParameterCollection = Cast<UMaterialParameterCollection>(StaticLoadObject(UMaterialParameterCollection::StaticClass(), NULL, TEXT("/GoogleVRController/ControllerParameters")));
	LaserParticleSystem = Cast<UParticleSystem>(StaticLoadObject(UParticleSystem::StaticClass(), NULL, TEXT("/GoogleVRController/LaserPointerParticleSystem")));
}

UMotionControllerComponent* UGoogleVRMotionControllerComponent::GetMotionController() const
{
	return MotionControllerComponent;
}

UStaticMeshComponent* UGoogleVRMotionControllerComponent::GetControllerMesh() const
{
	return ControllerMeshComponent;
}

UParticleSystemComponent* UGoogleVRMotionControllerComponent::GetLaser() const
{
	return LaserParticleSystemComponent;
}

UMaterialBillboardComponent* UGoogleVRMotionControllerComponent::GetReticle() const
{
	return ReticleBillboardComponent;
}

void UGoogleVRMotionControllerComponent::SetPointerDistance(float Distance)
{
	UpdateLaserDistance(Distance);
	UpdateReticleDistance(Distance);
}

void UGoogleVRMotionControllerComponent::OnRegister()
{
	Super::OnRegister();

	// Check that required UPROPERTIES are set.
	check(ControllerMesh != nullptr);
	check(IdleMaterial != nullptr);

	// Get the world to meters scale.
	const float WorldToMetersScale = GetWorldToMetersScale();

	// Create the MotionController and attach it to ourselves.
	MotionControllerComponent = NewObject<UMotionControllerComponent>(this, TEXT("MotionController"));
	MotionControllerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MotionControllerComponent->SetupAttachment(this);
	MotionControllerComponent->RegisterComponent();

	// Create the Controller mesh and attach it to the MotionController.
	ControllerMeshComponent = NewObject<UStaticMeshComponent>(this, TEXT("ControllerMesh"));
	ControllerMeshComponent->SetStaticMesh(ControllerMesh);
	ControllerMeshComponent->SetTranslucentSortPriority(TranslucentSortPriority);
	ControllerMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ControllerMeshComponent->SetupAttachment(MotionControllerComponent);
	ControllerMeshComponent->RegisterComponent();

	// Position the ControllerMesh so that the back of the model is at the pivot point.
	// This code assumes that the 3d model was exported with the mesh centered.
	FVector BoundsMin, BoundsMax;
	ControllerMeshComponent->GetLocalBounds(BoundsMin, BoundsMax);
	float ControllerLength = BoundsMax.X - BoundsMin.X;
	float ControllerHalfLength = ControllerLength * 0.5f * CONTROLLER_OFFSET_RATIO;
	ControllerMeshComponent->SetRelativeLocation(FVector(ControllerHalfLength, 0.0f, 0.0f));

	// Create the Controller Touch Point Mesh and attach it to the ControllerMesh.
	if (ControllerTouchPointMesh != nullptr)
	{
		ControllerTouchPointMeshComponent = NewObject<UStaticMeshComponent>(this, TEXT("ControllerTouchPointMesh"));
		ControllerTouchPointMeshComponent->SetStaticMesh(ControllerTouchPointMesh);
		ControllerTouchPointMeshComponent->SetTranslucentSortPriority(TranslucentSortPriority + 1);
		ControllerTouchPointMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (ControllerTouchPointMaterial != nullptr)
		{
			ControllerTouchPointMeshComponent->SetMaterial(0, ControllerTouchPointMaterial);
		}

		// Determine what the scale of the mesh should be based on the
		// Size of the mesh and the desired size of the touch point.
		// This assumes that the controller is a certain size.
		FVector TouchMeshSize;
		ControllerTouchPointMeshComponent->GetLocalBounds(BoundsMin, BoundsMax);
		TouchMeshSize = BoundsMax - BoundsMin;
		TouchMeshScale = TOUCHPAD_POINT_DIMENSIONS * WorldToMetersScale;
		TouchMeshScale.X /= TouchMeshSize.X;
		TouchMeshScale.Y /= TouchMeshSize.Y;
		TouchMeshScale.Z /= TouchMeshSize.Z;

		ControllerTouchPointMeshComponent->SetRelativeScale3D(TouchMeshScale);
		ControllerTouchPointMeshComponent->SetupAttachment(ControllerMeshComponent);
		ControllerTouchPointMeshComponent->RegisterComponent();
	}

	// Create the pointer container.
	PointerContainerComponent = NewObject<USceneComponent>(this, TEXT("Pointer"));
	PointerContainerComponent->SetupAttachment(MotionControllerComponent);
	PointerContainerComponent->RegisterComponent();

	// Create the laser.
	if (LaserParticleSystem != nullptr)
	{
		LaserParticleSystemComponent = NewObject<UParticleSystemComponent>(this, TEXT("LaserParticleSystem"));
		LaserParticleSystemComponent->SetTemplate(LaserParticleSystem);
		LaserParticleSystemComponent->SetTranslucentSortPriority(TranslucentSortPriority);
		LaserParticleSystemComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FParticleSysParam DistanceParam;
		DistanceParam.Name = "LaserDistance";
		DistanceParam.ParamType = EParticleSysParamType::PSPT_Scalar;
		LaserParticleSystemComponent->InstanceParameters.Add(DistanceParam);
		LaserParticleSystemComponent->SetupAttachment(PointerContainerComponent);
		LaserParticleSystemComponent->RegisterComponent();
	}

	// Create the reticle.
	if (ControllerReticleMaterial != nullptr)
	{
		ReticleBillboardComponent = NewObject<UMaterialBillboardComponent>(this, TEXT("Reticle"));
		ReticleBillboardComponent->AddElement(ControllerReticleMaterial, nullptr, false, 1.0f, 1.0f, nullptr);
		ReticleBillboardComponent->SetTranslucentSortPriority(TranslucentSortPriority);
		ReticleBillboardComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ReticleBillboardComponent->SetupAttachment(PointerContainerComponent);
		ReticleBillboardComponent->RegisterComponent();
	}

	// Now that everything is created, set the visibility based on the active status.
	// Set bAreSubComponentsEnabled to prevent SetSubComponentsEnabled from returning early since
	// The components have only just been created and may not be set properly yet.
	SetSubComponentsEnabled(true);
}

void UGoogleVRMotionControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<UGoogleVRPointerInputComponent*> Components;
	UGoogleVRPointerInputComponent* InputComponent = nullptr;

	GetOwner()->GetComponents(Components);
	if(Components.Num() == 0)
	{
		if (RequireInputComponent)
		{
			UE_LOG(LogGoogleVRMotionController, Warning, TEXT("GoogleVRMotionControllerComponent has RequireInputComponent set to true, but the actor does not have a GoogleVRPointerInputComponent. Creating GoogleVRPointerInputComponent."));

			InputComponent = NewObject<UGoogleVRPointerInputComponent>(GetOwner(), "GoogleVRPointerInputComponent");
			InputComponent->RegisterComponent();
		}
	}
	else
	{
		InputComponent = Components[0];
	}

	// If we found an InputComponent and it doesn't already have a Pointer, automatically set
	// it to this. If you want to switch between multiple pointers, or have multiple InputComponents,
	// then set the Pointers manually.
	if (InputComponent != nullptr && InputComponent->GetPointer() == nullptr)
	{
		InputComponent->SetPointer(this);
	}

	// Get the Playercontroller to use for input events.
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	check(PlayerController != nullptr);

	// Get the world to meters scale.
	const float WorldToMetersScale = GetWorldToMetersScale();

	// Set the laser and reticle distances to their default positions.
	UpdateLaserDistance(LaserDistanceMax * WorldToMetersScale);
	UpdateReticleDistance(ReticleDistanceMax * WorldToMetersScale);
}

void UGoogleVRMotionControllerComponent::Activate(bool bReset)
{
	Super::Activate(bReset);

	SetSubComponentsEnabled(IsPointerActive());
}

void UGoogleVRMotionControllerComponent::Deactivate()
{
	Super::Deactivate();

	SetSubComponentsEnabled(IsPointerActive());
}

void UGoogleVRMotionControllerComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	SetSubComponentsEnabled(IsPointerActive());

	if (!bAreSubComponentsEnabled)
	{
		return;
	}

	const float WorldToMetersScale = GetWorldToMetersScale();

	if (PlayerController->IsInputKeyDown(FGamepadKeyNames::MotionController_Left_Thumbstick)
		|| PlayerController->IsInputKeyDown(FGamepadKeyNames::MotionController_Right_Thumbstick))
	{
		TrySetControllerMaterial(TouchpadMaterial);
		if (ControllerTouchPointMeshComponent != nullptr)
		{
			ControllerTouchPointMeshComponent->SetVisibility(false);
		}
	}
	else
	{
		if (PlayerController->IsInputKeyDown(FGamepadKeyNames::MotionController_Left_Shoulder)
			|| PlayerController->IsInputKeyDown(FGamepadKeyNames::MotionController_Right_Shoulder))
		{
			TrySetControllerMaterial(AppMaterial);
		}
		else if (PlayerController->IsInputKeyDown(FGamepadKeyNames::SpecialLeft)
				 || PlayerController->IsInputKeyDown(FGamepadKeyNames::SpecialRight))
		{
			TrySetControllerMaterial(SystemMaterial);
		}
		else
		{
			TrySetControllerMaterial(IdleMaterial);
		}

		// Update the touch point's scale and position
		if (ControllerTouchPointMeshComponent != nullptr)
		{
			ControllerTouchPointMeshComponent->SetVisibility(true);

			if (PlayerController->IsInputKeyDown(EKeys::Steam_Touch_0))
			{
				ControllerTouchPointMeshComponent->SetRelativeScale3D(ControllerTouchPointMeshComponent->RelativeScale3D * TOUCHPAD_POINT_FILTER_STRENGTH +
																	  TouchMeshScale * (1.0f - TOUCHPAD_POINT_FILTER_STRENGTH));

				float TouchPadX = PlayerController->GetInputAnalogKeyState(FGamepadKeyNames::MotionController_Left_Thumbstick_X);
				float TouchPadY = PlayerController->GetInputAnalogKeyState(FGamepadKeyNames::MotionController_Left_Thumbstick_Y);
				float X = TouchPadX * TOUCHPAD_RADIUS * WorldToMetersScale;
				float Y = TouchPadY * TOUCHPAD_RADIUS * WorldToMetersScale;

				FVector TouchPointRelativeLocation = FVector(TOUCHPAD_POINT_X_OFFSET * WorldToMetersScale  - Y, X, TOUCHPAD_POINT_ELEVATION * WorldToMetersScale);
				ControllerTouchPointMeshComponent->SetRelativeLocation(TouchPointRelativeLocation);
			}
			else
			{
				ControllerTouchPointMeshComponent->SetRelativeScale3D(ControllerTouchPointMeshComponent->RelativeScale3D * TOUCHPAD_POINT_FILTER_STRENGTH);
			}
		}
	}

	// Update the position of the pointer.
	FVector PointerPositionOffset = UGoogleVRControllerFunctionLibrary::GetArmModelPointerPositionOffset();
	float PointerTiltAngle = UGoogleVRControllerFunctionLibrary::GetArmModelPointerTiltAngle();
	PointerContainerComponent->SetRelativeLocation(PointerPositionOffset);
	PointerContainerComponent->SetRelativeRotation(FRotator(-PointerTiltAngle, 0.0f, 0.0f));

	// Adjust Transparency
	if (ParameterCollection != nullptr)
	{
		float AlphaValue = UGoogleVRControllerFunctionLibrary::GetControllerAlphaValue();
		UMaterialParameterCollectionInstance* ParameterCollectionInstance = GetWorld()->GetParameterCollectionInstance(ParameterCollection);
		const bool bFoundParameter = ParameterCollectionInstance->SetScalarParameterValue("GoogleVRMotionControllerAlpha", AlphaValue);
		if (!bFoundParameter)
		{
			UE_LOG(LogGoogleVRMotionController, Warning, TEXT("Unable to find GoogleVRMotionControllerAlpha parameter in Material Collection."));
		}
	}

	// If we don't explicitly mark the particly system dirty then it won't
	// Follow the position of the controller correctly.
	if (LaserParticleSystemComponent != nullptr)
	{
		LaserParticleSystemComponent->MarkRenderStateDirty();
	}
}

void UGoogleVRMotionControllerComponent::TrySetControllerMaterial(UMaterialInterface* NewMaterial)
{
	if (NewMaterial != nullptr)
	{
		ControllerMeshComponent->SetMaterial(0, NewMaterial);
	}
	else
	{
		ControllerMeshComponent->SetMaterial(0, IdleMaterial);
	}
}

void UGoogleVRMotionControllerComponent::UpdateLaserDistance(float Distance)
{
	if (LaserParticleSystemComponent != nullptr)
	{
		const float WorldToMetersScale = GetWorldToMetersScale();
		float ClampedDistance = FMath::Clamp(Distance, 0.0f, LaserDistanceMax * WorldToMetersScale);
		FParticleSysParam& DistanceParam = LaserParticleSystemComponent->InstanceParameters[0];
		DistanceParam.Scalar = ClampedDistance;

		// Make sure that the laser is facing the Reticle.
		if (ReticleBillboardComponent != nullptr)
		{
			FVector LookAtPos = ReticleBillboardComponent->RelativeLocation - LaserParticleSystemComponent->RelativeLocation;
			LaserParticleSystemComponent->SetRelativeRotation(FRotationMatrix::MakeFromX(LookAtPos).Rotator());
		}
	}
}

void UGoogleVRMotionControllerComponent::UpdateReticleDistance(float Distance)
{
	if (ReticleBillboardComponent != nullptr)
	{
		const float WorldToMetersScale = GetWorldToMetersScale();
		float ClampedDistance = FMath::Clamp(Distance,
											 ReticleDistanceMin * WorldToMetersScale,
											 ReticleDistanceMax * WorldToMetersScale);

		ReticleBillboardComponent->SetRelativeLocation(FVector(ClampedDistance, 0.0f, 0.0f));
	}

	UpdateReticleSize();
}

void UGoogleVRMotionControllerComponent::UpdateReticleLocation(FVector Location, FVector OriginLocation)
{
	if (ReticleBillboardComponent != nullptr)
	{
		const float WorldToMetersScale = GetWorldToMetersScale();
		FVector Difference = Location - OriginLocation;
		FVector ClampedDifference = Difference.GetClampedToSize(ReticleDistanceMin * WorldToMetersScale,
																ReticleDistanceMax * WorldToMetersScale);
		Location = OriginLocation + ClampedDifference;

		ReticleBillboardComponent->SetWorldLocation(Location);
	}

	UpdateReticleSize();
}

void UGoogleVRMotionControllerComponent::UpdateReticleSize()
{
	if (ReticleBillboardComponent != nullptr)
	{
		FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
		float ReticleDistanceFromCamera = (ReticleBillboardComponent->GetComponentLocation() - CameraLocation).Size();
		float SpriteSize = ReticleSize * ReticleDistanceFromCamera;

		FMaterialSpriteElement& Sprite = ReticleBillboardComponent->Elements[0];
		if (Sprite.BaseSizeX != SpriteSize)
		{
			Sprite.BaseSizeX = SpriteSize;
			Sprite.BaseSizeY = SpriteSize;
			ReticleBillboardComponent->MarkRenderStateDirty();
		}
	}
}

void UGoogleVRMotionControllerComponent::SetSubComponentsEnabled(bool bNewEnabled)
{
	if (bNewEnabled == bAreSubComponentsEnabled)
	{
		return;
	}

	bAreSubComponentsEnabled = bNewEnabled;

	// Explicitly set the visibility of each elements instead of propagating recursively.
	// If we do it recursively, then we might change the visisiblity of something unintentionally.
	// I.E. An Object that is being "grabbed" with the controller.

	if (MotionControllerComponent != nullptr)
	{
		MotionControllerComponent->SetActive(bNewEnabled);
		MotionControllerComponent->SetVisibility(bNewEnabled);
	}

	if (ControllerMeshComponent != nullptr)
	{
		ControllerMeshComponent->SetActive(bNewEnabled);
		ControllerMeshComponent->SetVisibility(bNewEnabled);
	}

	if (ControllerTouchPointMeshComponent != nullptr)
	{
		ControllerTouchPointMeshComponent->SetActive(bNewEnabled);
		ControllerTouchPointMeshComponent->SetVisibility(bNewEnabled);
	}

	if (LaserParticleSystemComponent != nullptr)
	{
		LaserParticleSystemComponent->SetActive(bNewEnabled);
		LaserParticleSystemComponent->SetVisibility(bNewEnabled);
	}

	if (ReticleBillboardComponent != nullptr)
	{
		ReticleBillboardComponent->SetActive(bNewEnabled);
		ReticleBillboardComponent->SetVisibility(bNewEnabled);
	}
}

bool UGoogleVRMotionControllerComponent::IsControllerConnected() const
{
	return UGoogleVRControllerFunctionLibrary::GetGoogleVRControllerState() == EGoogleVRControllerState::Connected;
}

float UGoogleVRMotionControllerComponent::GetWorldToMetersScale() const
{
	if (GWorld != nullptr)
	{
		return GWorld->GetWorldSettings()->WorldToMeters;
	}

	// Default value, assume Unreal units are in centimeters
	return 100.0f;
}

void UGoogleVRMotionControllerComponent::OnPointerEnter(const FHitResult& HitResult, bool IsHitInteractive)
{
	OnPointerHover(HitResult, IsHitInteractive);
}

void UGoogleVRMotionControllerComponent::OnPointerHover(const FHitResult& HitResult, bool IsHitInteractive)
{
	FVector Location = HitResult.Location;
	FVector OriginLocation = HitResult.TraceStart;
	UpdateReticleLocation(Location, OriginLocation);

	FVector Difference = Location - PointerContainerComponent->GetComponentLocation();
	float Distance = Difference.Size();
	UpdateLaserDistance(Distance);
}

void UGoogleVRMotionControllerComponent::OnPointerExit(const FHitResult& HitResult)
{
	const float WorldToMetersScale = GetWorldToMetersScale();
	UpdateReticleDistance(ReticleDistanceMax * WorldToMetersScale);
	UpdateLaserDistance(LaserDistanceMax * WorldToMetersScale);
}

FVector UGoogleVRMotionControllerComponent::GetOrigin() const
{
	if (PointerContainerComponent != nullptr)
	{
		return PointerContainerComponent->GetComponentLocation();
	}

	return FVector::ZeroVector;
}

FVector UGoogleVRMotionControllerComponent::GetDirection() const
{
	if (PointerContainerComponent != nullptr)
	{
		return PointerContainerComponent->GetForwardVector();
	}

	return FVector::ZeroVector;
}

void UGoogleVRMotionControllerComponent::GetRadius(float& OutEnterRadius, float& OutExitRadius) const
{
	if (ReticleBillboardComponent != nullptr)
	{
		FMaterialSpriteElement& Sprite = ReticleBillboardComponent->Elements[0];
		float SpriteSize = Sprite.BaseSizeX;

		// Fixed size for enter radius to avoid flickering.
		// TODO: This will cause some slight variability based on the distance of the object from the camera,
		// and is optimized for the average case. For this to be fixed, the hit test must be done via a cone instead
		// of the spherecast that is currently used.
		const float WorldToMetersScale = GetWorldToMetersScale();
		OutEnterRadius = ReticleSize * WorldToMetersScale * EnterRadiusCoeff;

		// Dynamic size for exit radius.
		// Will always be correct because we know the intersection point of the object and are therefore using
		// the correct radius based on the object's distance from the camera.
		OutExitRadius = SpriteSize * ExitRadiusCoeff;
	}
	else
	{
		OutEnterRadius = 0.0f;
		OutExitRadius = 0.0f;
	}
}

float UGoogleVRMotionControllerComponent::GetMaxPointerDistance() const
{
	float WorldToMetersScale = GetWorldToMetersScale();
	return ReticleDistanceMax * WorldToMetersScale;
}

bool UGoogleVRMotionControllerComponent::IsPointerActive() const
{
	return IsActive() && IsControllerConnected();
}
