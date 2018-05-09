// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FlexSoftJointComponentVisualizer.h"
#include "SceneManagement.h"
#include "FlexSoftJointComponent.h"



void FFlexSoftJointComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UFlexSoftJointComponent* SoftJointComp = Cast<const UFlexSoftJointComponent>(Component);
	if (SoftJointComp != NULL)
	{
		FTransform TM = SoftJointComp->GetComponentTransform();
		TM.RemoveScaling();

		// Draw light radius
		DrawWireSphereAutoSides(PDI, TM, FColor(200, 255, 255), SoftJointComp->Radius, SDPG_World);
	}
}
