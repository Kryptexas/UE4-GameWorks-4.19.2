// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperBatchSceneProxy.h"
#include "PaperBatchComponent.h"

//////////////////////////////////////////////////////////////////////////
// UPaperBatchComponent

UPaperBatchComponent::UPaperBatchComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FPrimitiveSceneProxy* UPaperBatchComponent::CreateSceneProxy()
{
	FPaperBatchSceneProxy* NewProxy = new FPaperBatchSceneProxy(this);
	return NewProxy;
}

FBoxSphereBounds UPaperBatchComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	// Always visible
	FBoxSphereBounds Bounds(FVector::ZeroVector, FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX), HALF_WORLD_MAX);
	return Bounds;
}
