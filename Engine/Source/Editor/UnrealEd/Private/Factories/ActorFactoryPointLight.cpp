// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimCompositeFactory.cpp: Factory for AnimComposite
=============================================================================*/

#include "ActorFactories/ActorFactoryPointLight.h"
#include "GameFramework/Actor.h"
#include "Components/PointLightComponent.h"

void UActorFactoryPointLight::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	// Make all spawned actors use the candela units.
	TArray<UPointLightComponent*> PointLightComponents;
	NewActor->GetComponents<UPointLightComponent>(PointLightComponents);
	for (UPointLightComponent* Component : PointLightComponents)
	{
		if (Component && Component->CreationMethod == EComponentCreationMethod::Native)
		{
			static const auto CVarDefaultPointLightUnits = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DefaultFeature.PointLightUnits"));
			ELightUnits DefaultUnits = (ELightUnits)CVarDefaultPointLightUnits->GetValueOnAnyThread();

			Component->Intensity *= UPointLightComponent::GetUnitsConversionFactor(Component->IntensityUnits, DefaultUnits);
			Component->IntensityUnits = DefaultUnits;
		}
	}
}

