// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTargetActor_Radius.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/Net/UnrealNetwork.h"
#include "GameplayAbilityTargetTypes.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityTargetActor_Radius
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityTargetActor_Radius::AGameplayAbilityTargetActor_Radius(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	StaticTargetFunction = false;
	ShouldProduceTargetDataOnServer = true;
}

void AGameplayAbilityTargetActor_Radius::StartTargeting(UGameplayAbility* InAbility)
{
	Super::StartTargeting(InAbility);
	SourceActor = InAbility->GetCurrentActorInfo()->Actor.Get();
}

void AGameplayAbilityTargetActor_Radius::ConfirmTargetingAndContinue()
{
	check(ShouldProduceTargetData());
	if (SourceActor)
	{
		FVector Origin = StartLocation.GetTargetingTransform().GetLocation();
		FGameplayAbilityTargetDataHandle Handle = MakeTargetData(PerformOverlap(Origin), Origin);
		TargetDataReadyDelegate.Broadcast(Handle);
	}
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_Radius::MakeTargetData(const TArray<AActor*> Actors, const FVector& Origin) const
{
	if (OwningAbility)
	{
		/** Note: This will be cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
		FGameplayAbilityTargetData_Radius* ReturnData = new FGameplayAbilityTargetData_Radius(Actors, Origin);
		return FGameplayAbilityTargetDataHandle(ReturnData);
	}

	return FGameplayAbilityTargetDataHandle();
}

TArray<AActor*>	AGameplayAbilityTargetActor_Radius::PerformOverlap(const FVector& Origin)
{
	static FName RadiusTargetingOverlap = FName(TEXT("RadiusTargetingOverlap"));
	bool bTraceComplex = false;
	
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);

	FCollisionQueryParams Params(RadiusTargetingOverlap, bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bTraceAsyncScene = true;
	Params.AddIgnoredActors(ActorsToIgnore);

	TArray<FOverlapResult> Overlaps;

	SourceActor->GetWorld()->OverlapMulti(Overlaps, Origin, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);


	TArray<AActor*>	HitActors;

	for (int32 i=0; i < Overlaps.Num(); ++i)
	{
		// Temp, this needs to be way more robust
		APawn* PawnActor = Cast<APawn>(Overlaps[i].GetActor());
		if (PawnActor)
		{
			HitActors.Add(PawnActor);
		}
	}

	return HitActors;
}