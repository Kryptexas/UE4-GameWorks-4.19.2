// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FlexSoftJointComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Actor.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "FlexSoftJointActor.h"

#include "PhysicsPublic.h"

#include "FlexActor.h"
#include "FlexComponent.h"
#include "Particles/ParticleSystemComponent.h"

#include "EngineUtils.h"
#include "FlexManager.h"

//////////////////////////////////////////////////////////////////////////
// SOFTJOINTCOMPONENT
UFlexSoftJointComponent::UFlexSoftJointComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; //  TG_PostUpdateWork
	Radius = 200.0f;
	Stiffness = 0.1f;
	ContainerTemplate = nullptr;
	ContainerInstance = nullptr;
	bAutoActivate = true;
	NumParticles = 0;
	bAutoActivate = true;
	bJointIsInitialized = false;
	SoftJoint = nullptr;
}

UFlexContainer* UFlexSoftJointComponent::GetContainerTemplate()
{
	return ContainerInstance ? ContainerInstance->Template : nullptr;
}

void UFlexSoftJointComponent::OnUnregister()
{
	Super::OnUnregister();

	if (ContainerInstance && SoftJoint)
	{
		ContainerInstance->DestroySoftJointInstance(SoftJoint);
		SoftJoint = nullptr;

		ParticleIndices.Empty();
		ParticleLocalPositions.Empty();
	}

	if (ContainerInstance)
	{
		ContainerInstance->Unregister(this);
		ContainerInstance = nullptr;
	}
}

void UFlexSoftJointComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Do the initialization the first time this joint is ticked
	if (!bJointIsInitialized)
	{
		// Create rigid attachments to overlapping Flex actors
		const FVector Origin = GetComponentLocation();

		// Find all UFlexComponents except fluids overlapping the joint radius
		for (TActorIterator<AFlexActor> It(GetWorld()); It; ++It)
		{
			AFlexActor* FlexActor = (*It);
			UFlexComponent* FlexComponent = Cast<UFlexComponent>(FlexActor->GetRootComponent());

			if (FlexComponent && FlexComponent->AssetInstance && !FlexComponent->Phase.Fluid)
			{
				const FBoxSphereBounds FlexBounds = FlexComponent->GetBounds();

				// Dist of joint to flex bounds
				const float DistSq = FlexBounds.ComputeSquaredDistanceFromBoxToPoint(Origin);

				// Find all particles belonging to these UFlexComponents which are inside the joint radius
				if (DistSq < Radius*Radius)
				{
					for (int i = 0; i < FlexComponent->AssetInstance->numParticles; ++i)
					{
						FVector ParticlePos = FlexComponent->SimPositions[i];

						// Test distance from component origin
						FVector LocalPosition = FVector(ParticlePos) - Origin;

						if (LocalPosition.SizeSquared() < Radius*Radius)
						{
							int ParticleIndex = FlexComponent->AssetInstance->particleIndices[i];
							ParticleIndices.Add(ParticleIndex);

							// Currently it still stores the global particle position, we will subtract the center of mass of the joint from it later
							ParticleLocalPositions.Add(ParticlePos);	

							++NumParticles;
						}
					}
				}
			}
		}

		// Compute center of mass of the joint
		FVector JointCom(0.0f, 0.0f, 0.0f);
		for (const FVector& ParticlePosition : ParticleLocalPositions)
		{
			JointCom += ParticlePosition;
		}
		JointCom /= float(NumParticles);

		// Compute local positions relative to the joint com
		for (FVector& ParticleLocalPosition : ParticleLocalPositions)
		{
			// Now it stores the relative particle position
			ParticleLocalPosition -= JointCom;
		}

		// Create shape-matching constraint between joint particles
		FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();
		const uint32 FlexBit = ECC_TO_BITFIELD(ECC_Flex);
		if (PhysScene)
		{
			FFlexContainerInstance* Container = FFlexManager::get().FindFlexContainerInstance(PhysScene, ContainerTemplate);
			if (Container)
			{
				ContainerInstance = Container;
				ContainerInstance->Register(this);

				SoftJoint = ContainerInstance->CreateSoftJointInstance(ParticleIndices, ParticleLocalPositions, NumParticles, Stiffness);
			}
		}

		bJointIsInitialized = true;
	}
}

//////////////////////////////////////////////////////////////////////////
// ARB_SOFTJOINTACTOR
AFlexSoftJointActor::AFlexSoftJointActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SoftJointComponent = CreateDefaultSubobject<UFlexSoftJointComponent>(TEXT("SoftJointComponent0"));

#if WITH_EDITOR
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		// Structure to hold one-time initialization
		if (!IsRunningCommandlet())
		{
			struct FConstructorStatics
			{
				ConstructorHelpers::FObjectFinderOptional<UTexture2D> SoftJointTexture;
				FName ID_Physics;
				FText NAME_Physics;
				FConstructorStatics()
					: SoftJointTexture(TEXT("/Engine/EditorResources/S_SoftJoint.S_SoftJoint"))
					, ID_Physics(TEXT("Physics"))
					, NAME_Physics(NSLOCTEXT("SpriteCategory", "Physics", "Physics"))
				{
				}
			};
			static FConstructorStatics ConstructorStatics;

			SpriteComponent->Sprite = ConstructorStatics.SoftJointTexture.Get();

#if WITH_EDITORONLY_DATA
			SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Physics;
			SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Physics;
#endif // WITH_EDITORONLY_DATA
		}

		SpriteComponent->RelativeScale3D.X = 0.5f;
		SpriteComponent->RelativeScale3D.Y = 0.5f;
		SpriteComponent->RelativeScale3D.Z = 0.5f;
		SpriteComponent->SetupAttachment(SoftJointComponent);
		SpriteComponent->bIsScreenSizeScaled = true;
	}
#endif

	RootComponent = SoftJointComponent;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 0.1f;
}

#if WITH_EDITOR
void AFlexSoftJointActor::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	FVector ModifiedScale = DeltaScale * (AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f);

	const float Multiplier = (ModifiedScale.X > 0.0f || ModifiedScale.Y > 0.0f || ModifiedScale.Z > 0.0f) ? 1.0f : -1.0f;
	if (SoftJointComponent)
	{
		SoftJointComponent->Radius += Multiplier * ModifiedScale.Size();
		SoftJointComponent->Radius = FMath::Max(0.f, SoftJointComponent->Radius);
	}
}
#endif


/** Returns SoftJointComponent subobject **/
UFlexSoftJointComponent* AFlexSoftJointActor::GetSoftJointComponent() const { return SoftJointComponent; }
#if WITH_EDITORONLY_DATA
/** Returns SpriteComponent subobject **/
UBillboardComponent* AFlexSoftJointActor::GetSpriteComponent() const { return SpriteComponent; }
#endif
