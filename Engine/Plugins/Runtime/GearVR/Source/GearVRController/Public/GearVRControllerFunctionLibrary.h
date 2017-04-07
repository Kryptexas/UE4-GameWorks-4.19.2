// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GearVRControllerFunctionLibrary.generated.h"

UENUM(BlueprintType)
enum class EGearVRControllerHandedness : uint8
{
	RightHanded,
	LeftHanded,
	Unknown
};

UCLASS()
class UGearVRControllerFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	/** Set Gear VR CPU and GPU Levels */
	UFUNCTION(BlueprintCallable, Category = "GearVRController")
	static EGearVRControllerHandedness GetGearVRControllerHandedness();

	/** Set Gear VR CPU and GPU Levels */
	UFUNCTION(BlueprintCallable, Category = "GearVRController")
	static void EnableArmModel(bool bArmModelEnable);

	/** Is controller active */
	UFUNCTION(BlueprintCallable, Category = "GearVRController")
	static bool IsControllerActive();

};
