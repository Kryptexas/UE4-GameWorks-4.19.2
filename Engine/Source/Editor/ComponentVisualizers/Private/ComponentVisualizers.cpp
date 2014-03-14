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

IMPLEMENT_MODULE( FComponentVisualizersModule, ComponentVisualizers );

void FComponentVisualizersModule::StartupModule()
{
	if(GUnrealEd != NULL)
	{
		GUnrealEd->RegisterComponentVisualizer(UPointLightComponent::StaticClass(), FOnDrawComponentVisualizer::CreateStatic( &FPointLightComponentVisualizer::DrawVisualization ));
		GUnrealEd->RegisterComponentVisualizer(USpotLightComponent::StaticClass(), FOnDrawComponentVisualizer::CreateStatic( &FSpotLightComponentVisualizer::DrawVisualization ));
		GUnrealEd->RegisterComponentVisualizer(UAudioComponent::StaticClass(), FOnDrawComponentVisualizer::CreateStatic( &FAudioComponentVisualizer::DrawVisualization ));
		GUnrealEd->RegisterComponentVisualizer(URadialForceComponent::StaticClass(), FOnDrawComponentVisualizer::CreateStatic( &FRadialForceComponentVisualizer::DrawVisualization ));
		GUnrealEd->RegisterComponentVisualizer(UPhysicsConstraintComponent::StaticClass(), FOnDrawComponentVisualizer::CreateStatic( &FConstraintComponentVisualizer::DrawVisualization ));
		GUnrealEd->RegisterComponentVisualizer(USpringArmComponent::StaticClass(), FOnDrawComponentVisualizer::CreateStatic( &FSpringArmComponentVisualizer::DrawVisualization ));
	}
}

void FComponentVisualizersModule::ShutdownModule()
{
	if(GUnrealEd != NULL)
	{
		GUnrealEd->UnregisterComponentVisualizer(UPointLightComponent::StaticClass());
		GUnrealEd->UnregisterComponentVisualizer(USpotLightComponent::StaticClass());
		GUnrealEd->UnregisterComponentVisualizer(UAudioComponent::StaticClass());
		GUnrealEd->UnregisterComponentVisualizer(URadialForceComponent::StaticClass());
		GUnrealEd->UnregisterComponentVisualizer(UPhysicsConstraintComponent::StaticClass());
		GUnrealEd->UnregisterComponentVisualizer(USpringArmComponent::StaticClass());
	}
}
