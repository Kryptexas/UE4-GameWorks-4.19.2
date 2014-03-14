// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TriggerActors.cpp: Trigger implementation
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
namespace
{
	static const FColor TriggerBaseColor(100, 255, 100, 255);
}
ATriggerCapsule::ATriggerCapsule(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP.SetDefaultSubobjectClass<UCapsuleComponent>(TEXT("CollisionComp")))
{
	UCapsuleComponent* CapsuleCollisionComponent = CastChecked<UCapsuleComponent>(CollisionComponent);
	CapsuleCollisionComponent->ShapeColor = TriggerBaseColor;
	CapsuleCollisionComponent->InitCapsuleSize(+40.0f, +80.0f);
	CapsuleCollisionComponent->BodyInstance.bEnableCollision_DEPRECATED = true;
	static FName CollisionProfileName(TEXT("Trigger"));
	CapsuleCollisionComponent->SetCollisionProfileName(CollisionProfileName);

	bCollideWhenPlacing = true;

	if (SpriteComponent)
	{
		SpriteComponent->AttachParent = CapsuleCollisionComponent;
	}
}

#if WITH_EDITOR
void ATriggerCapsule::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * ( AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f );

	UCapsuleComponent * CapsuleComponent = Cast<UCapsuleComponent>(GetRootComponent());
	if ( bCtrlDown )
	{
		// CTRL+Scaling modifies trigger collision height.  This is for convenience, so that height
		// can be changed without having to use the non-uniform scaling widget (which is
		// inaccessable with spacebar widget cycling).
		const float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
		float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
		
		CapsuleHalfHeight += ModifiedScale.X;
		CapsuleHalfHeight = FMath::Max( 0.0f, CapsuleHalfHeight );
		CapsuleComponent->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	}
	else
	{
		float CapsuleRadius = CapsuleComponent->GetUnscaledCapsuleRadius();
		float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
		
		CapsuleRadius += ModifiedScale.X;
		CapsuleRadius = FMath::Max( 0.0f, CapsuleRadius );

		// If non-uniformly scaling, Z scale affects height and Y can affect radius too.
		if ( !ModifiedScale.AllComponentsEqual() )
		{
			// *2 to keep the capsule more capsule shaped during scaling
			CapsuleHalfHeight+= ModifiedScale.Z * 2.0f;
			CapsuleHalfHeight = FMath::Max( 0.0f, CapsuleHalfHeight );

			CapsuleRadius += ModifiedScale.Y;
			CapsuleRadius = FMath::Max( 0.0f, CapsuleRadius );
		}
		else
		{
			// uniform scaling, so apply to height as well

			// *2 to keep the capsule more capsule shaped during scaling
			CapsuleHalfHeight += ModifiedScale.Z * 2.0f;
			CapsuleHalfHeight = FMath::Max( 0.0f, CapsuleHalfHeight );
		}

		CapsuleComponent->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	}
}
#endif	//WITH_EDITOR

ATriggerBox::ATriggerBox(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP.SetDefaultSubobjectClass<UBoxComponent>(TEXT("CollisionComp")))
{
	UBoxComponent* BoxCollisionComponent = CastChecked<UBoxComponent>(CollisionComponent);

	BoxCollisionComponent->ShapeColor = TriggerBaseColor;
	BoxCollisionComponent->InitBoxExtent(FVector(40.0f, 40.0f, 40.0f));
	BoxCollisionComponent->BodyInstance.bEnableCollision_DEPRECATED = true;

	static FName CollisionProfileName(TEXT("Trigger"));
	BoxCollisionComponent->SetCollisionProfileName(CollisionProfileName);

	TSubobjectPtr<UBillboardComponent> SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->AttachParent = BoxCollisionComponent;
	}
}

#if WITH_EDITOR
void ATriggerBox::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * ( AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f );

	UBoxComponent * BoxComponent = Cast<UBoxComponent>(GetRootComponent());
	if ( bCtrlDown )
	{
		// CTRL+Scaling modifies trigger collision height.  This is for convenience, so that height
		// can be changed without having to use the non-uniform scaling widget (which is
		// inaccessable with spacebar widget cycling).
		FVector Extent = BoxComponent->GetUnscaledBoxExtent() + FVector(0, 0, ModifiedScale.X);
		Extent.Z = FMath::Max(0.0f, Extent.Z);
		BoxComponent->SetBoxExtent(Extent);
	}
	else
	{
		FVector Extent = BoxComponent->GetUnscaledBoxExtent() + FVector(ModifiedScale.X, ModifiedScale.Y, ModifiedScale.Z);
		Extent.X = FMath::Max(0.0f, Extent.X);
		Extent.Y = FMath::Max(0.0f, Extent.Y);
		Extent.Z = FMath::Max(0.0f, Extent.Z);
		BoxComponent->SetBoxExtent(Extent);
	}
}
#endif	//WITH_EDITOR

ATriggerSphere::ATriggerSphere(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP.SetDefaultSubobjectClass<USphereComponent>(TEXT("CollisionComp")))
{
	USphereComponent* SphereCollisionComponent = CastChecked<USphereComponent>(CollisionComponent);

	SphereCollisionComponent->ShapeColor = TriggerBaseColor;
	SphereCollisionComponent->InitSphereRadius(+40.0f);
	SphereCollisionComponent->BodyInstance.bEnableCollision_DEPRECATED = true;
	static FName CollisionProfileName(TEXT("Trigger"));
	SphereCollisionComponent->SetCollisionProfileName(CollisionProfileName);

	TSubobjectPtr<UBillboardComponent> SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->AttachParent = SphereCollisionComponent;
	}
}

#if WITH_EDITOR
void ATriggerSphere::EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
	const FVector ModifiedScale = DeltaScale * ( AActor::bUsePercentageBasedScaling ? 500.0f : 5.0f );

	USphereComponent * SphereComponent = Cast<USphereComponent>(GetRootComponent());
	SphereComponent->SetSphereRadius(FMath::Max<float>(0.0f, SphereComponent->GetUnscaledSphereRadius() + ModifiedScale.X));
}
#endif	//WITH_EDITOR

