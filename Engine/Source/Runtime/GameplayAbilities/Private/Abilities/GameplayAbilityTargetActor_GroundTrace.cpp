// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTargetActor_GroundTrace.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	AGameplayAbilityTargetActor_GroundTrace
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

AGameplayAbilityTargetActor_GroundTrace::AGameplayAbilityTargetActor_GroundTrace(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	CollisionHeightOffset = 0.0f;
}

void AGameplayAbilityTargetActor_GroundTrace::StartTargeting(UGameplayAbility* InAbility)
{
	//CollisionShape starts as "line," which is correct as a default for our purposes
	if (CollisionRadius > 0.0f)
	{
		//Check CollisionHeight here because the shape code doesn't. It is used as half-height
		if ((CollisionHeight * 0.5f) > CollisionRadius)
		{
			CollisionShape = FCollisionShape::MakeCapsule(CollisionRadius, CollisionHeight * 0.5f);
			CollisionHeightOffset = CollisionHeight * 0.5f;
		}
		else
		{
			CollisionShape = FCollisionShape::MakeSphere(CollisionRadius);
			CollisionHeight = 0.0f;
			CollisionHeightOffset = CollisionRadius;
		}
	}
	Super::StartTargeting(InAbility);
}

bool AGameplayAbilityTargetActor_GroundTrace::AdjustCollisionResultForShape(const FVector OriginalStartPoint, const FVector OriginalEndPoint, const FCollisionQueryParams Params, FHitResult& OutHitResult) const
{
	UWorld *ThisWorld = GetWorld();
	//Pull back toward player to find a better spot, accounting for the width of our object
	FVector Movement = (OriginalEndPoint - OriginalStartPoint);
	FVector MovementDirection = Movement.SafeNormal();
	float MovementMagnitude2D = Movement.Size2D();

	if (bDebug)
	{
		if (CollisionHeight > 0.0f)
		{
			DrawDebugCapsule(ThisWorld, OriginalEndPoint, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Black);
		}
		else
		{
			DrawDebugSphere(ThisWorld, OriginalEndPoint, CollisionRadius, 8, FColor::Black);
		}
	}

	if ((MovementMagnitude2D < (CollisionRadius * 2.0f)) || (CollisionRadius <= 1.0f))
	{
		return false;		//Bad case!
	}

	float IncrementSize = FMath::Clamp<float>(CollisionRadius * 0.5f, 25.0f, 250.0f);
	float LerpIncrement = IncrementSize / MovementMagnitude2D;
	FHitResult LocalResult;
	FVector TraceStart;
	FVector TraceEnd;
	//This needs to ramp up - the first few increments should be small, then we should start moving in larger steps.
	for (float LerpValue = CollisionRadius / MovementMagnitude2D; LerpValue < 1.0f; LerpValue += LerpIncrement)
	{
		TraceEnd = TraceStart = OriginalEndPoint - (LerpValue * Movement);
		TraceEnd.Z -= 99999.0f;
		ThisWorld->SweepSingle(LocalResult, TraceStart, TraceEnd, FQuat::Identity, ECC_WorldStatic, CollisionShape, Params);
		if (!LocalResult.bStartPenetrating)
		{
			if (!LocalResult.bBlockingHit)
			{
				//This is probably off the map and should not be considered valid. This should not happen in a non-debug map.
				if (bDebug)
				{
					if (CollisionHeight > 0.0f)
					{
						DrawDebugCapsule(ThisWorld, LocalResult.Location, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Yellow);
					}
					else
					{
						DrawDebugSphere(ThisWorld, LocalResult.Location, CollisionRadius, 8, FColor::Yellow);
					}
				}
				continue;
				//LocalResult.Location = TraceStart;
			}
			if (bDebug)
			{
				if (CollisionHeight > 0.0f)
				{
					DrawDebugCapsule(ThisWorld, LocalResult.Location, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Green);
				}
				else
				{
					DrawDebugSphere(ThisWorld, LocalResult.Location, CollisionRadius, 8, FColor::Green);
				}
			}
			OutHitResult = LocalResult;
			return true;
		}
		if (bDebug)
		{
			if (CollisionHeight > 0.0f)
			{
				DrawDebugCapsule(ThisWorld, TraceStart, CollisionHeight * 0.5f, CollisionRadius, FQuat::Identity, FColor::Red);
			}
			else
			{
				DrawDebugSphere(ThisWorld, TraceStart, CollisionRadius, 8, FColor::Red);
			}
		}
	}
	return false;
}

FHitResult AGameplayAbilityTargetActor_GroundTrace::PerformTrace(AActor* InSourceActor) const
{
	static const FName LineTraceSingleName(TEXT("AGameplayAbilityTargetActor_GroundTrace"));
	bool bTraceComplex = false;

	FCollisionQueryParams Params(LineTraceSingleName, bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bTraceAsyncScene = true;
	Params.AddIgnoredActor(InSourceActor);

	FVector AimDirection = InSourceActor->GetActorForwardVector();		//Default
	
	if (OwningAbility)		//Server and launching client only
	{
		APlayerController* AimingPC = OwningAbility->GetCurrentActorInfo()->PlayerController.Get();
		check(AimingPC);
		FVector CamLoc;
		FRotator CamRot;
		AimingPC->GetPlayerViewPoint(CamLoc, CamRot);
		AimDirection = CamRot.Vector();
	}

	FVector TraceStart = InSourceActor->GetActorLocation();
	FVector TraceEnd = TraceStart + (AimDirection * MaxRange);

	//If we're using a socket, adjust the starting location and aim direction after the end position has been found. This way we can still aim with the camera, then fire accurately from the socket.
	if (OwningAbility)		//Server and launching client only
	{
		TraceStart = StartLocation.GetTargetingTransform().GetLocation();
	}
	AimDirection = (TraceEnd - TraceStart).SafeNormal();

	// ------------------------------------------------------

	FHitResult ReturnHitResult;
	FVector LineTraceStart = TraceStart;
	FVector LineTraceEnd = TraceEnd;
	//Use a line trace initially to see where the player is actually pointing
	InSourceActor->GetWorld()->LineTraceSingle(ReturnHitResult, TraceStart, TraceEnd, ECC_WorldStatic, Params);
	//Default to end of trace line if we don't hit anything.
	if (!ReturnHitResult.bBlockingHit)
	{
		ReturnHitResult.Location = TraceEnd;
	}

	//Second trace, straight down. Consider using InSourceActor->GetWorld()->NavigationSystem->ProjectPointToNavigation() instead of tracing in the case of movement abilities (flag/bool).
	TraceStart = ReturnHitResult.Location - AimDirection;		//Pull back very slightly to avoid scraping down walls
	TraceEnd = TraceStart;
	TraceStart.Z += CollisionHeightOffset;
	TraceEnd.Z -= 99999.0f;
	InSourceActor->GetWorld()->LineTraceSingle(ReturnHitResult, TraceStart, TraceEnd, ECC_WorldStatic, Params);
	//if (!ReturnHitResult.bBlockingHit) then our endpoint may be off the map. Hopefully this is only possible in debug maps.

	//Use collision shape to find a valid ground spot, if appropriate
	if (CollisionShape.ShapeType != ECollisionShape::Line)
	{
		ReturnHitResult.Location.Z += CollisionHeightOffset;		//Rise up out of the ground
		TraceStart = InSourceActor->GetActorLocation();
		TraceEnd = ReturnHitResult.Location;
		TraceStart.Z += CollisionHeightOffset;
		if (AdjustCollisionResultForShape(TraceStart, TraceEnd, Params, ReturnHitResult))
		{
			ReturnHitResult.Location.Z -= CollisionHeightOffset;	//Undo the artificial height adjustment
		}
	}

	if (AActor* LocalReticleActor = ReticleActor.Get())
	{
		LocalReticleActor->SetActorLocation(ReturnHitResult.Location);
	}

	return ReturnHitResult;
}
