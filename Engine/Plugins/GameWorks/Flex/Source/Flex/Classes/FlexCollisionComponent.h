// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "FlexCollisionComponent.generated.h"

/**
*	Used to add properties to actors flex particles are colliding with.
*/
UCLASS(Blueprintable, hidecategories = (Object), meta = (BlueprintSpawnableComponent))
class FLEX_API UFlexCollisionComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** Enable particle deletion on contact*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	bool bDrain;

	/** Numer of particles overlaping the primitive component this frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	int32 Count;

	/** If this component acts as localized simulation parent for Flex objects then this should be enabled to ensure correct collision handling*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	uint32 bIsLocalSimParent : 1;
};

FORCEINLINE UFlexCollisionComponent::UFlexCollisionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	bDrain = false;
	Count = 0;
	bIsLocalSimParent = 0;
}
