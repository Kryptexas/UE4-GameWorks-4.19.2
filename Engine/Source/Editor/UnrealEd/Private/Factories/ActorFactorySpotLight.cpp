// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ActorFactorySpotLight.cpp: Factory for SpotLight
=============================================================================*/

#include "ActorFactories/ActorFactorySpotLight.h"
#include "GameFramework/Actor.h"
#include "Components/SpotLightComponent.h"

void UActorFactorySpotLight::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	// Make all spawned actors use the candela units.
	TArray<UPointLightComponent*> PointLightComponents;
	NewActor->GetComponents<UPointLightComponent>(PointLightComponents);
	for (UPointLightComponent* Component : PointLightComponents)
	{
		if (Component && Component->CreationMethod == EComponentCreationMethod::Native)
		{
			static const auto CVarDefaultSpotLightUnits = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DefaultFeature.SpotLightUnits"));
			ELightUnits DefaultUnits = (ELightUnits)CVarDefaultSpotLightUnits->GetValueOnAnyThread();

			Component->Intensity *= UPointLightComponent::GetUnitsConversionFactor(Component->IntensityUnits, DefaultUnits);
			Component->IntensityUnits = DefaultUnits;
		}
	}
}

