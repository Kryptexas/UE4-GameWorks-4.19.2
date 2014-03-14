// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CameraTypes.generated.h"

//@TODO: Document
UENUM()
namespace ECameraProjectionMode
{
	enum Type
	{
		Perspective,
		Orthographic
	};
}

USTRUCT()
struct FMinimalViewInfo
{
	GENERATED_USTRUCT_BODY()

	/** Location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	FVector Location;

	/** Rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	FRotator Rotation;

	/** The field of view (in degrees) in perspective mode (ignored in Orthographic mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float FOV;

	/** The desired width (in world units) of the orthographic view (ignored in Perspective mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float OrthoWidth;

	// Aspect Ratio (Width/Height); ignored unless bConstrianAspectRatio is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float AspectRatio;

	// If bConstrainAspectRatio is true, black bars will be added if the destination view has a different aspect ratio than this camera requested.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	uint32 bConstrainAspectRatio:1; 

	// The type of camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	TEnumAsByte<ECameraProjectionMode::Type> ProjectionMode;

	/** Indicates if PostProcessSettings should be applied. */
	UPROPERTY()
	float PostProcessBlendWeight;

	/** Post-process settings to use if PostProcessBlendWeight is non-zero. */
	UPROPERTY()
	struct FPostProcessSettings PostProcessSettings;

	FMinimalViewInfo()
		: Location(ForceInit)
		, Rotation(ForceInit)
		, FOV(90.0f)
		, OrthoWidth(512.0f)
		, AspectRatio(1.33333333f)
		, bConstrainAspectRatio(false)
		, ProjectionMode(ECameraProjectionMode::Perspective)
		, PostProcessBlendWeight(0.0f)
	{
	}

	// Is this equivalent to the other one?
	ENGINE_API bool Equals(const FMinimalViewInfo& OtherInfo) const;

	// Blends view information
	// Note: booleans are orred together, instead of blending
	ENGINE_API void BlendViewInfo(FMinimalViewInfo& OtherInfo, float OtherWeight);
};
