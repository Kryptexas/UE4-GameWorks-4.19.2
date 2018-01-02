// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LandscapeBlueprintSupport.cpp: Landscape blueprint functions
  =============================================================================*/

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "LandscapeProxy.h"
#include "LandscapeSplineSegment.h"
#include "LandscapeSplineRaster.h"
#include "Components/SplineComponent.h"
#include "LandscapeComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void ALandscapeProxy::EditorApplySpline(USplineComponent* InSplineComponent, float StartWidth, float EndWidth, float StartSideFalloff, float EndSideFalloff, float StartRoll, float EndRoll, int32 NumSubdivisions, bool bRaiseHeights, bool bLowerHeights, ULandscapeLayerInfoObject* PaintLayer)
{
#if WITH_EDITOR
	if (InSplineComponent && !GetWorld()->IsGameWorld())
	{
		TArray<FLandscapeSplineInterpPoint> Points;
		LandscapeSplineRaster::Pointify(InSplineComponent->SplineCurves.Position, Points, NumSubdivisions, 0.0f, 0.0f, StartWidth, EndWidth, StartSideFalloff, EndSideFalloff, StartRoll, EndRoll);

		FTransform SplineToWorld = InSplineComponent->GetComponentTransform();
		LandscapeSplineRaster::RasterizeSegmentPoints(GetLandscapeInfo(), MoveTemp(Points), SplineToWorld, bRaiseHeights, bLowerHeights, PaintLayer);
	}
#endif
}

void ALandscapeProxy::SetLandscapeMaterialTextureParameterValue(FName ParameterName, class UTexture* Value)
{	
	if (bUseDynamicMaterialInstance)
	{
		for (ULandscapeComponent* Component : LandscapeComponents)
		{
			for (UMaterialInstanceDynamic* MaterialInstance : Component->MaterialInstancesDynamic)
			{
				if (MaterialInstance != nullptr)
				{
					MaterialInstance->SetTextureParameterValue(ParameterName, Value);
				}
			}
		}
	}
}

void ALandscapeProxy::SetLandscapeMaterialVectorParameterValue(FName ParameterName, FLinearColor Value)
{
	if (bUseDynamicMaterialInstance)
	{
		for (ULandscapeComponent* Component : LandscapeComponents)
		{
			for (UMaterialInstanceDynamic* MaterialInstance : Component->MaterialInstancesDynamic)
			{
				if (MaterialInstance != nullptr)
				{
					MaterialInstance->SetVectorParameterValue(ParameterName, Value);
				}
			}
		}		
	}
}

void ALandscapeProxy::SetLandscapeMaterialScalarParameterValue(FName ParameterName, float Value)
{
	if (bUseDynamicMaterialInstance)
	{
		for (ULandscapeComponent* Component : LandscapeComponents)
		{
			for (UMaterialInstanceDynamic* MaterialInstance : Component->MaterialInstancesDynamic)
			{
				if (MaterialInstance != nullptr)
				{
					MaterialInstance->SetScalarParameterValue(ParameterName, Value);
				}
			}
		}			
	}
}
