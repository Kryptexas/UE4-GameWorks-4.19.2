// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetActor.h"
#include "GameplayAbilityWorldReticle_ActorVisualization.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityWorldReticle_ActorVisualization
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityWorldReticle_ActorVisualization::AGameplayAbilityWorldReticle_ActorVisualization(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	//PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickGroup = TG_PrePhysics;

	CollisionComponent = PCIP.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("CollisionCapsule0"));
	CollisionComponent->InitCapsuleSize(0.f, 0.f);
	CollisionComponent->AlwaysLoadOnClient = true;
	CollisionComponent->bAbsoluteScale = true;
	//CollisionComponent->AlwaysLoadOnServer = true;
	CollisionComponent->bCanEverAffectNavigation = false;
	CollisionComponent->BodyInstance.bEnableCollision_DEPRECATED = false;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent0"));
	RootComponent = CollisionComponent;
}

void AGameplayAbilityWorldReticle_ActorVisualization::InitializeReticleTurretInformation(AActor* TurretActor, UMaterialInterface *TurretMaterial)
{
	if (TurretActor)
	{
		//Get components
		TArray<UMeshComponent*> MeshComps;
		USceneComponent* MyRoot = GetRootComponent();
		TurretActor->GetComponents(MeshComps);
		check(MyRoot);

		for (UMeshComponent* MeshComp : MeshComps)
		{
			//Special case: If we don't clear the root component explicitly, the component will be destroyed along with the original turret actor.
			if (MeshComp == TurretActor->GetRootComponent())
			{
				TurretActor->SetRootComponent(NULL);
			}

			//Disable collision on turret mesh parts so it doesn't interfere with aiming or any other client-side collision/prediction/physics stuff
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);		//All mesh components are primitive components, so no cast is needed

			//Move components from one actor to the other, attaching as needed. Hierarchy should not be important, but we can do fixups if it becomes important later.
			MeshComp->DetachFromParent();
			MeshComp->AttachTo(MyRoot);
			MeshComp->Rename(nullptr, this);
			if (TurretMaterial)
			{
				MeshComp->SetMaterial(0, TurretMaterial);
			}
		}
	}
}

void AGameplayAbilityWorldReticle_ActorVisualization::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TArray<UActorComponent*> CompArray;
	GetComponents(CompArray);
	CompArray.Num();

	Super::EndPlay(EndPlayReason);
}
