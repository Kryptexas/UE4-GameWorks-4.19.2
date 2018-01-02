// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ClothingSimulationInteractor.generated.h"

class IClothingSimulation;
class IClothingSimulationContext;

/** 
 * If a clothing simulation is able to be interacted with at runtime then a derived
 * interactor should be created, and at least the basic API implemented for that
 * simulation.
 * Only write to the simulation and context during the call to Sync, as that is
 * guaranteed to be a safe place to access this data.
 */
UCLASS(Abstract)
class CLOTHINGSYSTEMRUNTIMEINTERFACE_API UClothingSimulationInteractor : public UObject
{
	GENERATED_BODY()

public:

	// Basic interface that clothing simulations are expected to support

	// Called to update collision status without restarting the simulation
	UFUNCTION(BlueprintCallable, Category=ClothingSimulation)
	virtual void PhysicsAssetUpdated()
	PURE_VIRTUAL(UClothingSimulationInteractor::PhysicsAssetUpdated, );

	// Called to update the cloth config without restarting the simulation
	UFUNCTION(BlueprintCallable, Category=ClothingSimulation)
	virtual void ClothConfigUpdated()
	PURE_VIRTUAL(UClothingSimulationInteractor::ClothConfigUpdated, );

	bool IsDirty() { return bDirty; }

	// Sync the interactor to the provided context for the clothing simulation to use
	// on its next update. This will be called at a time that is guaranteed to
	// never overlap with the running simulation. The simulation will have to be
	// written in a way to pick up these changes on the next update
	virtual void Sync(IClothingSimulation* InSimulation, IClothingSimulationContext* InContext)
	PURE_VIRTUAL(UClothingSimulationInteractor::Sync, );

protected:

	// Intended to be called by functions on the interactor to message to the 
	// owning skeletal mesh component that this interactor requires a sync.
	void MarkDirty() { bDirty = true; }

	// Flag to control sync calls
	bool bDirty;

};