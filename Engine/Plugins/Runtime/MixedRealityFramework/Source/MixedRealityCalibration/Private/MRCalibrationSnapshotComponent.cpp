// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MRCalibrationSnapshotComponent.h"
#include "Camera/CameraTypes.h"
#include "Engine/TextureRenderTarget2D.h"

//------------------------------------------------------------------------------
UMRCalibrationSnapshotComponent::UMRCalibrationSnapshotComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ViewOwner(nullptr)
{}

//------------------------------------------------------------------------------
#if WITH_EDITOR
bool UMRCalibrationSnapshotComponent::GetEditorPreviewInfo(float /*DeltaTime*/, FMinimalViewInfo& ViewOut)
{
	ViewOut.Location = GetComponentLocation();
	ViewOut.Rotation = GetComponentRotation();

	ViewOut.FOV = FOVAngle;

	if (TextureTarget && TextureTarget->GetSurfaceWidth() > 0 && TextureTarget->GetSurfaceHeight() > 0)
	{
		ViewOut.AspectRatio = TextureTarget->GetSurfaceWidth() / TextureTarget->GetSurfaceHeight();
	}
	else
	{
		ViewOut.AspectRatio = 16.f / 9.f;
	}
	ViewOut.bConstrainAspectRatio = true;

	// see default in FSceneViewInitOptions
	ViewOut.bUseFieldOfViewForLOD = true;

	ViewOut.ProjectionMode = ProjectionType;
	ViewOut.OrthoWidth = OrthoWidth;

	// see BuildProjectionMatrix() in SceneCaptureRendering.cpp
	ViewOut.OrthoNearClipPlane = 0.0f;
	ViewOut.OrthoFarClipPlane = WORLD_MAX / 8.0f;;

	ViewOut.PostProcessBlendWeight = PostProcessBlendWeight;
	if (PostProcessBlendWeight > 0.0f)
	{
		ViewOut.PostProcessSettings = PostProcessSettings;
	}

	return true;
}
#endif	// WITH_EDITOR

//------------------------------------------------------------------------------
const AActor* UMRCalibrationSnapshotComponent::GetViewOwner() const
{
	return ViewOwner ? ViewOwner : Super::GetViewOwner();
}
