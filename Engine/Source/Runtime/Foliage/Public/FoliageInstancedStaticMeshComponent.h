// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FoliageInstancedStaticMeshComponent.generated.h"

USTRUCT(BlueprintType)
struct FFoliageInstanceDamage
{
	GENERATED_USTRUCT_BODY()

	FFoliageInstanceDamage()
	: InstanceIndex(0)
	, DamageAmount(0.f)
	{}

	FFoliageInstanceDamage(int32 InInstanceIndex, float InDamageAmount)
	:	InstanceIndex(InInstanceIndex)
	,	DamageAmount(InDamageAmount)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FoliageInstanceDamage)
	int32 InstanceIndex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FoliageInstanceDamage)
	float DamageAmount;
};

/**
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(FInstancePointDamageSignature, int32, InstanceIndex, float, Damage, class AController*, InstigatedBy, FVector, HitLocation, FVector, ShotFromDirection, const class UDamageType*, DamageType, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FInstanceRadialDamageSignature, const TArray<FFoliageInstanceDamage>&, DamageInstances, class AController*, InstigatedBy, FVector, Origin, float, MaxRadius, const class UDamageType*, DamageType, AActor*, DamageCauser);

UCLASS(ClassGroup = Foliage, Blueprintable)
class FOLIAGE_API UFoliageInstancedStaticMeshComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "Game|Damage")
	FInstancePointDamageSignature OnInstanceTakePointDamage;

	UPROPERTY(BlueprintAssignable, Category = "Game|Damage")
	FInstanceRadialDamageSignature OnInstanceTakeRadialDamage;

	virtual void ReceiveComponentDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};

