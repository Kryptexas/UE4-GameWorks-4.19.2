// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhysicsEngine/SoftJointComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Actor.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/MovementComponent.h"
#include "PhysicsEngine/SoftJointActor.h"
#include "Components/DestructibleComponent.h"

#if WITH_FLEX
#include "PhysicsEngine/FlexActor.h"
#include "PhysicsEngine/FlexComponent.h"
#include "Particles/ParticleSystemComponent.h"
#endif

//////////////////////////////////////////////////////////////////////////
// SOFTJOINTCOMPONENT
USoftJointComponent::USoftJointComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Radius = 200.0f;
	bAutoActivate = true;
	NumParticles = 0;
}

void USoftJointComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_FLEX
	// create rigid attachments to overlapping Flex actors
	const FVector Origin = GetComponentLocation();

	// Find all UFlexComponents except fluids overlapping the joint radius
	// Find all particles belonging to these UFlexComponents which are inside the joint radius
	for (TActorIterator<AFlexActor> It(GetWorld()); It; ++It)
	{
		AFlexActor* FlexActor = (*It);
		UFlexComponent* FlexComponent = Cast<UFlexComponent>(FlexActor->GetRootComponent());	

		if (FlexComponent && !FlexComponent->Phase.Fluid)
		{
			const FBoxSphereBounds FlexBounds = FlexComponent->GetBounds();

			// dist of joint to flex bounds
			const float DistSq = FlexBounds.ComputeSquaredDistanceFromBoxToPoint(Origin);

			if (DistSq < Radius*Radius)
			{
				for (int ParticleIndex = 0; ParticleIndex < FlexComponent->SimPositions.Num(); ++ParticleIndex)
				{
					FVector ParticlePos = FlexComponent->SimPositions[ParticleIndex];

					// test distance from component origin
					FVector LocalPosition = FVector(ParticlePos) - Origin;

					if (LocalPosition.SizeSquared() < Radius*Radius)
					{
						ParticleIndices.Add(ParticleIndex);
						ParticleLocalPositions.Add(LocalPosition);

						++NumParticles;
					}
				}
			}
		}
	}

	// TODO:
	// Create shape-matching constraint between joint particles
	FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();
	if (PhysScene)
	{
		PhysScene->AddSoftJointToFlex(ParticleIndices, ParticleLocalPositions, NumParticles, Stiffness);
	}

#endif
}

void USoftJointComponent::PostLoad()
{
	Super::PostLoad();
}

#if WITH_EDITOR

void USoftJointComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif

//////////////////////////////////////////////////////////////////////////
// ARB_SOFTJOINTACTOR
ASoftJointActor::ASoftJointActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SoftJointComponent = CreateDefaultSubobject<USoftJointComponent>(TEXT("SoftJointComponent0"));

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
void ASoftJointActor::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
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
USoftJointComponent* ASoftJointActor::GetSoftJointComponent() const { return SoftJointComponent; }
#if WITH_EDITORONLY_DATA
/** Returns SpriteComponent subobject **/
UBillboardComponent* ASoftJointActor::GetSpriteComponent() const { return SpriteComponent; }
#endif
