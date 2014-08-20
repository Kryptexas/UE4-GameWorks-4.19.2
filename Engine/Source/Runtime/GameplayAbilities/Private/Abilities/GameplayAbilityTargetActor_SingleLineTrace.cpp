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
	DOREPLIFETIME(AGameplayAbilityTargetActor_SingleLineTrace, SourceComponent);
	DOREPLIFETIME(AGameplayAbilityTargetActor_SingleLineTrace, SourceSocketName);
}

void AGameplayAbilityTargetActor_SingleLineTrace::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ReticleActor.IsValid())
	{
		ReticleActor.Get()->Destroy();
	}

	Super::EndPlay(EndPlayReason);
}


FHitResult AGameplayAbilityTargetActor_SingleLineTrace::PerformTrace(AActor* InSourceActor, USkeletalMeshComponent* InSourceComponent, FName InSourceSocketName) const
{
	static const FName LineTraceSingleName(TEXT("AGameplayAbilityTargetActor_SingleLineTrace"));
	bool bTraceComplex = false;
	TArray<AActor*> ActorsToIgnore;

	ActorsToIgnore.Add(InSourceActor);

	FCollisionQueryParams Params(LineTraceSingleName, bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bTraceAsyncScene = true;
	Params.AddIgnoredActors(ActorsToIgnore);

	FVector AimDirection = InSourceActor->GetActorForwardVector();		//Default
	UGameplayAbility* MyAbility = Ability.Get();
	if (MyAbility)		//Server and launching client only
	{
		APlayerController* AimingPC = MyAbility->GetCurrentActorInfo()->PlayerController.Get();
		check(AimingPC);
		FVector CamLoc;
		FRotator CamRot;
		AimingPC->GetPlayerViewPoint(CamLoc, CamRot);
		AimDirection = CamRot.Vector();
	}

	FVector TraceStart = InSourceActor->GetActorLocation();
	FVector TraceEnd = TraceStart + (AimDirection * 3000.f);

	//If we're using a socket, adjust the starting location and aim direction after the end position has been found. This way we can still aim with the camera, then fire accurately from the socket.
	if (InSourceComponent)
	{
		TraceStart = InSourceComponent->GetSocketLocation(InSourceSocketName);		//If socket name is invalid, this just gets the location of the component.
		AimDirection = (TraceEnd - TraceStart).SafeNormal();
	}

	// ------------------------------------------------------

	FHitResult ReturnHitResult;
	InSourceActor->GetWorld()->LineTraceSingle(ReturnHitResult, TraceStart, TraceEnd, ECC_WorldStatic, Params);
	if (AActor* LocalReticleActor = ReticleActor.Get())
	{
		LocalReticleActor->SetActorLocation(ReturnHitResult.Location);
	}
	return ReturnHitResult;
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_SingleLineTrace::StaticGetTargetData(UWorld * World, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	AActor* StaticSourceActor = ActorInfo->Actor.Get();
	check(StaticSourceActor);
	UAnimInstance* AnimInstance = ActorInfo->AnimInstance.Get();
	USkeletalMeshComponent* StaticSourceComponent = AnimInstance ? AnimInstance->GetOwningComponent() : NULL;

	return MakeTargetData(PerformTrace(StaticSourceActor, StaticSourceComponent, SourceSocketName));
}

void AGameplayAbilityTargetActor_SingleLineTrace::StartTargeting(UGameplayAbility* InAbility)
{
	Ability = InAbility;
	SourceActor = InAbility->GetCurrentActorInfo()->Actor.Get();
	UAnimInstance* AnimInstance = InAbility->GetCurrentActorInfo()->AnimInstance.Get();
	SourceComponent = AnimInstance ? AnimInstance->GetOwningComponent() : NULL;

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

	ReticleActor = GetWorld()->SpawnActor<AGameplayAbilityWorldReticle>(ReticleClass, GetActorLocation(), GetActorRotation());
	if (AGameplayAbilityWorldReticle* CachedReticleActor = ReticleActor.Get())
	{
		CachedReticleActor->InitializeReticle(this);
	}
}

void AGameplayAbilityTargetActor_SingleLineTrace::Tick(float DeltaSeconds)
{
	// very temp - do a mostly hardcoded trace from the source actor
	if (SourceActor)
	{
		FHitResult HitResult = PerformTrace(SourceActor, SourceComponent, SourceSocketName);
		FVector EndPoint = HitResult.Component.IsValid() ? HitResult.ImpactPoint : HitResult.TraceEnd;

		if (bDebug)
		{
			DrawDebugLine(GetWorld(), SourceActor->GetActorLocation(), EndPoint, FLinearColor::Green, false);
			DrawDebugSphere(GetWorld(), EndPoint, 16, 10, FLinearColor::Green, false);
		}

		SetActorLocationAndRotation(EndPoint, SourceActor->GetActorRotation());

		if (ShouldProduceTargetData() && bAutoFire)
		{
			if (TimeUntilAutoFire <= 0.0f)
			{
				ConfirmTargeting();
				bAutoFire = false;
			}
			TimeUntilAutoFire -= DeltaSeconds;
		}
	}
}

void AGameplayAbilityTargetActor_SingleLineTrace::ConfirmTargeting()
{
	check(ShouldProduceTargetData());

	if (SourceActor)
	{
		bDebug = false;
		bAutoFire = false;
		FGameplayAbilityTargetDataHandle Handle = MakeTargetData(PerformTrace(SourceActor, SourceComponent, SourceSocketName));
		TargetDataReadyDelegate.Broadcast(Handle);
	}

	Destroy();
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_SingleLineTrace::MakeTargetData(FHitResult HitResult) const
{
	/** Note these are cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
	if (SourceComponent)
	{
		FGameplayAbilityTargetData_Mesh* ReturnData = new FGameplayAbilityTargetData_Mesh();
		ReturnData->SourceActor = SourceActor;
		ReturnData->SourceComponent = SourceComponent;
		ReturnData->SourceSocketName = SourceSocketName;
		ReturnData->TargetPoint = HitResult.Location;
		return FGameplayAbilityTargetDataHandle(ReturnData);
	}
	else
	{
		FGameplayAbilityTargetData_SingleTargetHit* ReturnData = new FGameplayAbilityTargetData_SingleTargetHit();
		ReturnData->HitResult = HitResult;
		return FGameplayAbilityTargetDataHandle(ReturnData);
	}
}