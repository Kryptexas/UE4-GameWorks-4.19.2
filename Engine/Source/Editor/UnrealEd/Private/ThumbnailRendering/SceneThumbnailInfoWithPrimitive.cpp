// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

USceneThumbnailInfoWithPrimitive::USceneThumbnailInfoWithPrimitive(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimitiveType = TPT_Sphere;
	OrbitPitch = -35;
	OrbitYaw = -180.f;
	OrbitZoom = -400.f;
}

void USceneThumbnailInfoWithPrimitive::PostLoad()
{
	Super::PostLoad();

	if ( GIsEditor )
	{
		// Load the PreviewMesh with the thumbnail info so it will be ready for visualization.
		if ( PreviewMesh.IsValid() )
		{
			LoadObject<UObject>(nullptr, *PreviewMesh.ToString());
		}
	}
}
