// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTargetActor_Trace.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityTargetActor_Trace
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityTargetActor_Trace::AGameplayAbilityTargetActor_Trace(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	StaticTargetFunction = false;

	MaxRange = 999999.0f;
	TraceChannel = ECC_WorldStatic;
}

void AGameplayAbilityTargetActor_Trace::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ReticleActor.IsValid())
	{
		ReticleActor.Get()->Destroy();
	}

	Super::EndPlay(EndPlayReason);
}

void AGameplayAbilityTargetActor_Trace::LineTraceWithFilter(FHitResult& ReturnHitResult, const UWorld* InWorld, const FGameplayTargetDataFilterHandle InFilterHandle, const FVector& InTraceStart, const FVector& InTraceEnd, ECollisionChannel Channel, const FCollisionQueryParams Params) const
{
	check(InWorld);
	FCollisionQueryParams LocalParams = Params;
	while (true)
	{
		FHitResult TempHitResult;
		InWorld->LineTraceSingle(TempHitResult, InTraceStart, InTraceEnd, Channel, LocalParams);
		if (TempHitResult.bBlockingHit && TempHitResult.Actor.IsValid() && !InFilterHandle.FilterPassesForActor(TempHitResult.Actor))
		{
			LocalParams.AddIgnoredActor(TempHitResult.Actor.Get());
			continue;
		}
		//Either hit something we're not ignoring, or didn't hit anything.
		ReturnHitResult = TempHitResult;
		break;
	};
}

void AGameplayAbilityTargetActor_Trace::SweepWithFilter(FHitResult& ReturnHitResult, const UWorld* InWorld, const FGameplayTargetDataFilterHandle InFilterHandle, const FVector& InTraceStart, const FVector& InTraceEnd, const FQuat& InRotation, ECollisionChannel Channel, const FCollisionShape CollisionShape, const FCollisionQueryParams Params) const
{
	check(InWorld);
	FCollisionQueryParams LocalParams = Params;
	while (true)
	{
		FHitResult TempHitResult;
		InWorld->SweepSingle(TempHitResult, InTraceStart, InTraceEnd, InRotation, Channel, CollisionShape, LocalParams);
		if (TempHitResult.bBlockingHit && TempHitResult.Actor.IsValid() && !InFilterHandle.FilterPassesForActor(TempHitResult.Actor))
		{
			LocalParams.AddIgnoredActor(ReturnHitResult.Actor.Get());
			continue;
		}
		//Either hit something we're not ignoring, or didn't hit anything.
		ReturnHitResult = TempHitResult;
		break;
	};
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_Trace::StaticGetTargetData(UWorld * World, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	check(false);		//This should never actually be called, and if it is, it will require a const version of PerformTrace()
	AActor* StaticSourceActor = ActorInfo->AvatarActor.Get();
	//return MakeTargetData(PerformTrace(StaticSourceActor));		//Old way, requires PerformTrace to be a const call
	return MakeTargetData(FHitResult());
}

void AGameplayAbilityTargetActor_Trace::AimWithPlayerController(AActor* InSourceActor, FCollisionQueryParams Params, FVector TraceStart, FVector& TraceEnd) const
{
	if (OwningAbility)		//Server and launching client only
	{
		APlayerController* AimingPC = OwningAbility->GetCurrentActorInfo()->PlayerController.Get();
		check(AimingPC);
		FVector CamLoc;
		FRotator CamRot;
		AimingPC->GetPlayerViewPoint(CamLoc, CamRot);
		FVector CamDir = CamRot.Vector();
		FVector CamTarget = CamLoc + (CamDir * MaxRange);		//Straight, dumb aiming to a point that's reasonable though not exactly correct

		ClipCameraRayToAbilityRange(CamLoc, CamDir, TraceStart, MaxRange, CamTarget);

		FHitResult TempHitResult;
		LineTraceWithFilter(TempHitResult, InSourceActor->GetWorld(), Filter, CamLoc, CamTarget, TraceChannel, Params);
		if (TempHitResult.bBlockingHit && (FVector::DistSquared(TraceStart, TempHitResult.Location) <= (MaxRange * MaxRange)))
		{
			//We actually made a hit? Pull back.
			TraceEnd = TempHitResult.Location;
		}
		else
		{
			//If we didn't make a hit, use the clipped location.
			TraceEnd = CamTarget;
		}

		//Readjust so we have a full-length line going through the predicted target point.
		FVector AimDirection = (TraceEnd - TraceStart).SafeNormal();
		if (AimDirection.SizeSquared() > 0.0f)
		{
			TraceEnd = TraceStart + (AimDirection * MaxRange);
		}
		else
		{
			FVector TraceEnd = TraceStart + (InSourceActor->GetActorForwardVector() * MaxRange);		//Default
		}
	}
}

bool AGameplayAbilityTargetActor_Trace::ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition)
{
	FVector CameraToCenter = AbilityCenter - CameraLocation;
	float DotToCenter = FVector::DotProduct(CameraToCenter, CameraDirection);
	if (DotToCenter >= 0)		//If this fails, we're pointed away from the center, but we might be inside the sphere and able to find a good exit point.
	{
		float DistanceSquared = CameraToCenter.SizeSquared() - (DotToCenter * DotToCenter);
		float RadiusSquared = (AbilityRange * AbilityRange);
		if (DistanceSquared <= RadiusSquared)
		{
			float DistanceFromCamera = FMath::Sqrt(RadiusSquared - DistanceSquared);
			float DistanceAlongRay = DotToCenter + DistanceFromCamera;						//Subtracting instead of adding will get the other intersection point
			ClippedPosition = CameraLocation + (DistanceAlongRay * CameraDirection);		//Cam aim point clipped to range sphere
			return true;
		}
	}
	return false;
}

void AGameplayAbilityTargetActor_Trace::StartTargeting(UGameplayAbility* InAbility)
{
	Super::StartTargeting(InAbility);
	SourceActor = InAbility->GetCurrentActorInfo()->AvatarActor.Get();

	if (ReticleClass)
	{
		AGameplayAbilityWorldReticle* SpawnedReticleActor = GetWorld()->SpawnActor<AGameplayAbilityWorldReticle>(ReticleClass, GetActorLocation(), GetActorRotation());
		if (SpawnedReticleActor)
		{
			SpawnedReticleActor->InitializeReticle(this, ReticleParams);
			ReticleActor = SpawnedReticleActor;

			// This is to catch cases of playing on a listen server where we are using a replicated reticle actor.
			// (In a client controlled player, this would only run on the client and therefor never replicate. If it runs
			// on a listen server, the reticle actor may replicate. We want consistancy between client/listen server players.
			// Just saying 'make the reticle actor non replicated' isnt a good answer, since we want to mix and match reticle
			// actors and there may be other targeting types that want to replicate the same reticle actor class).
			if (!ShouldProduceTargetDataOnServer)
			{
				SpawnedReticleActor->SetReplicates(false);
			}
		}
	}
}

void AGameplayAbilityTargetActor_Trace::Tick(float DeltaSeconds)
{
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

void AGameplayAbilityTargetActor_Trace::ConfirmTargetingAndContinue()
{
	check(ShouldProduceTargetData());
	if (SourceActor)
	{
		bDebug = false;
		FGameplayAbilityTargetDataHandle Handle = MakeTargetData(PerformTrace(SourceActor));
		TargetDataReadyDelegate.Broadcast(Handle);
	}
}

FGameplayAbilityTargetDataHandle AGameplayAbilityTargetActor_Trace::MakeTargetData(FHitResult HitResult) const
{
	/** Note: This will be cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
	return StartLocation.MakeTargetDataHandleFromHitResult(OwningAbility, HitResult);
}
