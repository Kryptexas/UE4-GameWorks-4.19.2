// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Collision.cpp: AActor collision implementation
=============================================================================*/

#include "EnginePrivate.h"

//////////////////////////////////////////////////////////////////////////
// FHitResult

FHitResult::FHitResult(class AActor* InActor, class UPrimitiveComponent* InComponent, FVector const& HitLoc, FVector const& HitNorm)
{
	FMemory::Memzero(this, sizeof(FHitResult));
	Location = HitLoc;
	ImpactPoint = HitLoc;
	Normal = HitNorm;
	ImpactNormal = HitNorm;
	Actor = InActor;
	Component = InComponent;
}

AActor* FHitResult::GetActor() const
{
	return Actor.Get();
}

UPrimitiveComponent* FHitResult::GetComponent() const
{
	return Component.Get();
}

//////////////////////////////////////////////////////////////////////////
// FOverlapResult

AActor* FOverlapResult::GetActor() const
{
	return Actor.Get();
}

UPrimitiveComponent* FOverlapResult::GetComponent() const
{
	return Component.Get();
}


//////////////////////////////////////////////////////////////////////////
// FCollisionQueryParams

FCollisionQueryParams::FCollisionQueryParams(FName InTraceTag, bool bInTraceComplex, const AActor* InIgnoreActor)
{
	bTraceComplex = bInTraceComplex;
	TraceTag = InTraceTag;
	bTraceAsyncScene = false;
	bFindInitialOverlaps = true;
	bReturnFaceIndex = false;
	bReturnPhysicalMaterial = false;

	AddIgnoredActor(InIgnoreActor);
	if (InIgnoreActor != NULL)
	{
		OwnerTag = InIgnoreActor->GetFName();
	}
}

void FCollisionQueryParams::AddIgnoredActor(const AActor* InIgnoreActor)
{
	if (InIgnoreActor != NULL)
	{
		IgnoreActors.AddUnique(InIgnoreActor->GetUniqueID());
	}
}

void FCollisionQueryParams::AddIgnoredActors(const TArray<AActor*>& InIgnoreActors)
{
	for (int32 Idx = 0; Idx < InIgnoreActors.Num(); ++Idx)
	{
		AActor const* const A = InIgnoreActors[Idx];
		if (A)
		{
			IgnoreActors.Add(A->GetUniqueID());
		}
	}
}

void FCollisionQueryParams::AddIgnoredActors(const TArray<TWeakObjectPtr<AActor> >& InIgnoreActors)
{
	for (int32 Idx = 0; Idx < InIgnoreActors.Num(); ++Idx)
	{
		AActor const* const A = InIgnoreActors[Idx].Get();
		if (A)
		{
			IgnoreActors.Add(A->GetUniqueID());
		}
	}
}