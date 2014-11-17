// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTargetActor_SingleLineTrace.h"
#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityTargetActor_SingleLineTrace
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityTargetActor_SingleLineTrace::AGameplayAbilityTargetActor_SingleLineTrace(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	StaticTargetFunction = false;
	bDebug = false;
	bBindToConfirmCancelInputs = false;
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_SingleLineTrace::StaticGetTargetData(UWorld * World, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo)
{
	// very temp - do a mostly hardcoded trace from the source actor

	AActor * SourceActor = ActorInfo->Actor.Get();
	check(SourceActor);

	static const FName LineTraceSingleName(TEXT("AGameplayAbilityTargetActor_SingleLineTrace"));
	bool bTraceComplex = false;
	TArray<AActor*> ActorsToIgnore;

	ActorsToIgnore.Add(SourceActor);

	FCollisionQueryParams Params(LineTraceSingleName, bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bTraceAsyncScene = true;
	Params.AddIgnoredActors(ActorsToIgnore);

	FVector Start = SourceActor->GetActorLocation();
	FVector End = Start + (SourceActor->GetActorForwardVector() * 3000.f);

	// ------------------------------------------------------

	FGameplayAbilityTargetData_SingleTargetHit* ReturnData = new FGameplayAbilityTargetData_SingleTargetHit();

	SourceActor->GetWorld()->LineTraceSingle(ReturnData->HitResult, Start, End, ECC_WorldStatic, Params);

	if (bDebug)
	{
		DrawDebugLine(World, Start, End, FLinearColor::Green, false);
		DrawDebugSphere(World, ReturnData->HitResult.Location, 16, 10, FLinearColor::Green, false);
	}

	return FGameplayAbilityTargetDataHandle(ReturnData);
}

void AGameplayAbilityTargetActor_SingleLineTrace::StartTargeting(UGameplayAbility* InAbility)
{
	Ability = InAbility;

	// We can bind directly to our ASC's confirm/cancel events, or wait to be told from an outside source to confirm or cancel
	if (bBindToConfirmCancelInputs)
	{
		UAbilitySystemComponent* ASC = Ability->GetCurrentActorInfo()->AbilitySystemComponent.Get();
		if (ASC)
		{
			ASC->ConfirmCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor_SingleLineTrace::Confirm);
			ASC->CancelCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor_SingleLineTrace::Cancel);
		}
	}
	
	bDebug = true;
}

void AGameplayAbilityTargetActor_SingleLineTrace::Confirm()
{
	if (Ability.IsValid())
	{
		bDebug = false;
		FGameplayAbilityTargetDataHandle Handle = StaticGetTargetData(Ability->GetWorld(), Ability->GetCurrentActorInfo(), Ability->GetCurrentActivationInfo());
		TargetDataReadyDelegate.Broadcast(Handle);
	}

	Destroy();
}

void AGameplayAbilityTargetActor_SingleLineTrace::Cancel()
{
	Destroy();
}

void AGameplayAbilityTargetActor_SingleLineTrace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	/** Temp: Do a trace wiuth bDebug=true to draw a cheap "preview" */
	if (Ability.IsValid())
	{
		StaticGetTargetData(Ability->GetWorld(), Ability->GetCurrentActorInfo(), Ability->GetCurrentActivationInfo());
	}
}

void AGameplayAbilityTargetActor_SingleLineTrace::ConfirmTargeting()
{
	if (Ability.IsValid())
	{
		bDebug = false;
		FGameplayAbilityTargetDataHandle Handle = StaticGetTargetData(Ability->GetWorld(), Ability->GetCurrentActorInfo(), Ability->GetCurrentActivationInfo());
		TargetDataReadyDelegate.Broadcast(Handle);
	}

	Destroy();
}
