// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ClothingSimulation.h"
#include "ClothingSimulationFactoryInterface.h"

#include "ClothingSimulationFactory.generated.h"

UCLASS()
class CLOTHINGSYSTEMRUNTIME_API UClothingSimulationFactoryNv final : public UClothingSimulationFactory
{
	GENERATED_BODY()
public:

	virtual IClothingSimulation* CreateSimulation() override;
	virtual void DestroySimulation(IClothingSimulation* InSimulation) override;
	virtual bool SupportsAsset(UClothingAssetBase* InAsset) override;

	virtual bool SupportsRuntimeInteraction() override;
	virtual UClothingSimulationInteractor* CreateInteractor() override;
};