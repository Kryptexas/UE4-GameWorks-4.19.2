// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ClothingSimulationFactory.h"
#include "Assets/ClothingAsset.h"

#if WITH_NVCLOTH
#include "ClothingSimulationNv.h"
#endif

#include "ClothingSimulationInteractorNv.h"
#include "Package.h"

IClothingSimulation* UClothingSimulationFactoryNv::CreateSimulation()
{
#if WITH_NVCLOTH
	FClothingSimulationBase* Simulation = new FClothingSimulationNv();
	return Simulation;
#endif
	return nullptr;
}

void UClothingSimulationFactoryNv::DestroySimulation(IClothingSimulation* InSimulation)
{
#if WITH_NVCLOTH
	delete InSimulation;
#endif
}

bool UClothingSimulationFactoryNv::SupportsAsset(UClothingAssetBase* InAsset)
{
#if WITH_NVCLOTH
	return true;
#endif
	return false;
}

bool UClothingSimulationFactoryNv::SupportsRuntimeInteraction()
{
	return true;
}

UClothingSimulationInteractor* UClothingSimulationFactoryNv::CreateInteractor()
{
	return CastChecked<UClothingSimulationInteractor>(NewObject<UClothingSimulationInteractorNv>(GetTransientPackage()));
}
