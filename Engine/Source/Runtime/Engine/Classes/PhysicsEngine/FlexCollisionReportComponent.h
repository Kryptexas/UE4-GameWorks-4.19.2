// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "FlexCollisionReportComponent.generated.h"

UCLASS(Blueprintable, hidecategories = (Object), meta = (BlueprintSpawnableComponent))
class ENGINE_API UFlexCollisionReportComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** Enable particle deletion on contact*/
	UPROPERTY(EditAnywhere, Category = Flex)
	bool bDrain;

	/** Numer of particles overlaping the primitive component this frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Flex)
	int32 Count;
};

FORCEINLINE UFlexCollisionReportComponent::UFlexCollisionReportComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}