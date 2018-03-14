// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ClothingSimulationInteractor.h"
#include "ClothingSimulationNv.h"
#include "ClothingSimulationInteractorNv.generated.h"

class FClothingSimulationNv;
class FClothingSimulationContextNv;

// Command signature for handling synced command buffer
DECLARE_DELEGATE_TwoParams(NvInteractorCommand, FClothingSimulationNv*, FClothingSimulationContextNv*)

UCLASS(BlueprintType)
class UClothingSimulationInteractorNv : public UClothingSimulationInteractor
{
	GENERATED_BODY()

public:

	// UClothingSimulationInteractor Interface
	virtual void PhysicsAssetUpdated() override;
	virtual void ClothConfigUpdated() override;
	virtual void Sync(IClothingSimulation* InSimulation, IClothingSimulationContext* InContext) override;
	//////////////////////////////////////////////////////////////////////////

	// Set the stiffness of the spring force for anim drive
	UFUNCTION(BlueprintCallable, Category=ClothingSimulation)
	void SetAnimDriveSpringStiffness(float InStiffness);

	// Set the stiffness of the resistive damping force for anim drive
	UFUNCTION(BlueprintCallable, Category=ClothingSimulation)
	void SetAnimDriveDamperStiffness(float InStiffness);

	// Set a new gravity override and enable the override
	UFUNCTION(BlueprintCallable, Category=ClothingSimulation)
	void EnableGravityOverride(const FVector& InVector);

	// Disable any currently set gravity override
	UFUNCTION(BlueprintCallable, Category = ClothingSimulation)
	void DisableGravityOverride();

private:

	// Command queue processed when we hit a sync
	TArray<NvInteractorCommand> Commands;
};
