// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ComponentVisualizersPrivatePCH.h"
#include "ComponentVisualizers.h"

#include "SoundDefinitions.h"

#include "PointLightComponentVisualizer.h"
#include "SpotLightComponentVisualizer.h"
#include "AudioComponentVisualizer.h"
#include "RadialForceComponentVisualizer.h"
#include "ConstraintComponentVisualizer.h"
#include "SpringArmComponentVisualizer.h"
#include "SplineComponentVisualizer.h"

IMPLEMENT_MODULE( FComponentVisualizersModule, ComponentVisualizers );

void FComponentVisualizersModule::StartupModule()
{
	RegisterComponentVisualizer(UPointLightComponent::StaticClass()->GetFName(), MakeShareable(new FPointLightComponentVisualizer));
	RegisterComponentVisualizer(USpotLightComponent::StaticClass()->GetFName(), MakeShareable(new FSpotLightComponentVisualizer));
	RegisterComponentVisualizer(UAudioComponent::StaticClass()->GetFName(), MakeShareable(new FAudioComponentVisualizer));
	RegisterComponentVisualizer(URadialForceComponent::StaticClass()->GetFName(), MakeShareable(new FRadialForceComponentVisualizer));
	RegisterComponentVisualizer(UPhysicsConstraintComponent::StaticClass()->GetFName(), MakeShareable(new FConstraintComponentVisualizer));
	RegisterComponentVisualizer(USpringArmComponent::StaticClass()->GetFName(), MakeShareable(new FSpringArmComponentVisualizer));
	RegisterComponentVisualizer(USplineComponent::StaticClass()->GetFName(), MakeShareable(new FSplineComponentVisualizer));
}

void FComponentVisualizersModule::ShutdownModule()
{
	if(GUnrealEd != NULL)
	{
		// Iterate over all class names we registered for
		for(FName ClassName : RegisteredComponentClassNames)
		{
			GUnrealEd->UnregisterComponentVisualizer(ClassName);
		}
	}
}

void FComponentVisualizersModule::RegisterComponentVisualizer(FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer)
{
	if (GUnrealEd != NULL)
	{
		GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
	}

	RegisteredComponentClassNames.Add(ComponentClassName);
}
