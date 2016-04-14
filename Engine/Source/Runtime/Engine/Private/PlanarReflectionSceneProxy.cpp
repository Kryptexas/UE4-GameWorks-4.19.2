// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PlanarReflectionProxy.cpp
=============================================================================*/

#include "EnginePrivate.h"
#include "Components/PlanarReflectionComponent.h"
#include "PlanarReflectionSceneProxy.h"

FPlanarReflectionSceneProxy::FPlanarReflectionSceneProxy(UPlanarReflectionComponent* Component, FPlanarReflectionRenderTarget* InRenderTarget)
{
	RenderTarget = InRenderTarget;

	DistanceFromPlaneFadeEnd = Component->DistanceFromPlaneFadeEnd;

	float ClampedFadeStart = FMath::Max(Component->DistanceFromPlaneFadeStart, 0.0f);
	float ClampedFadeEnd = FMath::Max(Component->DistanceFromPlaneFadeEnd, 0.0f);
	float DistanceFadeScale = 1.0f / FMath::Max(ClampedFadeEnd - ClampedFadeStart, DELTA);

	PlanarReflectionParameters = FVector(
		DistanceFadeScale,
		-ClampedFadeStart * DistanceFadeScale,
		Component->NormalDistortionStrength);

	const float CosFadeStart = FMath::Cos(FMath::Clamp(Component->AngleFromPlaneFadeStart, 0.1f, 89.9f) * (float)PI / 180.0f);
	const float CosFadeEnd = FMath::Cos(FMath::Clamp(Component->AngleFromPlaneFadeEnd, 0.1f, 89.9f) * (float)PI / 180.0f);
	const float Range = 1.0f / FMath::Max(CosFadeStart - CosFadeEnd, DELTA);

	PlanarReflectionParameters2 = FVector2D(
		Range,
		-CosFadeEnd * Range);

	Component->GetProjectionWithExtraFOV(ProjectionWithExtraFOV);

	OwnerName = Component->GetOwner() ? Component->GetOwner()->GetFName() : NAME_None;

	UpdateTransform(Component->ComponentToWorld.ToMatrixWithScale());

	PlanarReflectionId = Component->GetPlanarReflectionId();
	PrefilterRoughness = Component->PrefilterRoughness;
	PrefilterRoughnessDistance = Component->PrefilterRoughnessDistance;
}
