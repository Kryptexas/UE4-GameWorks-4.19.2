// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "PhysXSupport.h"


UPhysicsThrusterComponent::UPhysicsThrusterComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	ThrustStrength = 100.0;
}

void UPhysicsThrusterComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Applied force to the base, so if we don't have one, do nothing.
	if( bIsActive && AttachParent )
	{
		FVector WorldForce = ThrustStrength * ComponentToWorld.TransformVectorNoScale( FVector(-1.f,0.f,0.f) );

		UPrimitiveComponent* BasePrimComp = Cast<UPrimitiveComponent>(AttachParent);
		if(BasePrimComp)
		{
			BasePrimComp->AddForceAtLocation(WorldForce, GetComponentLocation(), NAME_None);
		}
	}
}

void UPhysicsThrusterComponent::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerUE4Version() < VER_UE4_CONFORM_COMPONENT_ACTIVATE_FLAG)
	{
		bAutoActivate = bThrustEnabled_DEPRECATED;
	}
}

//////////////////////////////////////////////////////////////////////////

APhysicsThruster::APhysicsThruster(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> ThrusterTexture;
		FName ID_Physics;
		FText NAME_Physics;
		FConstructorStatics()
			: ThrusterTexture(TEXT("/Engine/EditorResources/S_Thruster"))
			, ID_Physics(TEXT("Physics"))
			, NAME_Physics(NSLOCTEXT( "SpriteCategory", "Physics", "Physics" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	ThrusterComponent = PCIP.CreateDefaultSubobject<UPhysicsThrusterComponent>(this, TEXT("Thruster0"));
	RootComponent = ThrusterComponent;

#if WITH_EDITORONLY_DATA
	ArrowComponent = PCIP.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("ArrowComponent0"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowSize = 1.7f;
		ArrowComponent->ArrowColor = FColor(255, 180, 0);

		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Physics;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Physics;
		ArrowComponent->AttachParent = ThrusterComponent;
	}

	SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.ThrusterTexture.Get();
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Physics;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Physics;
		SpriteComponent->AttachParent = ThrusterComponent;
	}
#endif // WITH_EDITORONLY_DATA
}