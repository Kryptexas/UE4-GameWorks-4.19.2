// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTargetActor_SingleLineTrace.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/Net/UnrealNetwork.h"

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
	bBindToConfirmCancelInputs = true;
}

void AGameplayAbilityTargetActor_SingleLineTrace::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGameplayAbilityTargetActor_SingleLineTrace, bDebug);
	DOREPLIFETIME(AGameplayAbilityTargetActor_SingleLineTrace, SourceActor);
}


FHitResult AGameplayAbilityTargetActor_SingleLineTrace::PerformTrace(AActor *InSourceActor) const
{
	static const FName LineTraceSingleName(TEXT("AGameplayAbilityTargetActor_SingleLineTrace"));
	bool bTraceComplex = false;
	TArray<AActor*> ActorsToIgnore;

	ActorsToIgnore.Add(InSourceActor);

	FCollisionQueryParams Params(LineTraceSingleName, bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bTraceAsyncScene = true;
	Params.AddIgnoredActors(ActorsToIgnore);

	FVector TraceStart = InSourceActor->GetActorLocation();
	FVector TraceEnd = TraceStart + (InSourceActor->GetActorForwardVector() * MaxRange);

	// ------------------------------------------------------

	FHitResult ReturnHitResult;
	InSourceActor->GetWorld()->LineTraceSingle(ReturnHitResult, TraceStart, TraceEnd, ECC_WorldStatic, Params);
	return ReturnHitResult;
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_SingleLineTrace::StaticGetTargetData(UWorld * World, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	AActor* StaticSourceActor = ActorInfo->Actor.Get();
	check(StaticSourceActor);
	
	return MakeTargetData(PerformTrace(StaticSourceActor));
}

void AGameplayAbilityTargetActor_SingleLineTrace::StartTargeting(UGameplayAbility* InAbility)
{
	Ability = InAbility;
	SourceActor = InAbility->GetCurrentActorInfo()->Actor.Get();

	// We can bind directly to our ASC's confirm/cancel events, or wait to be told from an outside source to confirm or cancel
	if (bBindToConfirmCancelInputs && (MasterPC && MasterPC->IsLocalController()))
	{
		UAbilitySystemComponent* ASC = Ability->GetCurrentActorInfo()->AbilitySystemComponent.Get();
		if (ASC)
		{
			ASC->ConfirmCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor_SingleLineTrace::ConfirmTargeting);
			ASC->CancelCallbacks.AddDynamic(this, &AGameplayAbilityTargetActor_SingleLineTrace::CancelTargeting);
		}
	}
	
	bDebug = true;
}

void AGameplayAbilityTargetActor_SingleLineTrace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// very temp - do a mostly hardcoded trace from the source actor
	if (SourceActor)
	{
		FHitResult HitResult = PerformTrace(SourceActor);

		FVector EndPoint = HitResult.Component.IsValid() ? HitResult.ImpactPoint : HitResult.TraceEnd;
		
		if (bDebug)
		{
			DrawDebugLine(GetWorld(), SourceActor->GetActorLocation(), EndPoint, FLinearColor::Green, false);
			DrawDebugSphere(GetWorld(), EndPoint, 16, 10, FLinearColor::Green, false);
		}
		SetActorLocationAndRotation(EndPoint, SourceActor->GetActorRotation());
	}
}

void AGameplayAbilityTargetActor_SingleLineTrace::ConfirmTargeting()
{
	if (SourceActor)
	{
		bDebug = false;
		FGameplayAbilityTargetDataHandle Handle = MakeTargetData(PerformTrace(SourceActor));
		TargetDataReadyDelegate.Broadcast(Handle);
	}

	Destroy();
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_SingleLineTrace::MakeTargetData(FHitResult HitResult) const
{
	/** Note this is cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
	FGameplayAbilityTargetData_SingleTargetHit* ReturnData = new FGameplayAbilityTargetData_SingleTargetHit();
	ReturnData->HitResult = HitResult;
	return FGameplayAbilityTargetDataHandle(ReturnData);
}