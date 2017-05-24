// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "GearVRControllerComponent.generated.h"

UCLASS(ClassGroup=(GearVR), meta=(BlueprintSpawnableComponent))
class UGearVRControllerComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGearVRControllerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* ControllerMesh;
	
	UFUNCTION(BlueprintCallable, Category = "GearVRController", meta = (Keywords = "Gear VR"))
	UMotionControllerComponent* GetMotionController() const;

	UFUNCTION(BlueprintCallable, Category = "GearVRController", meta = (Keywords = "Gear VR"))
	UStaticMeshComponent* GetControllerMesh() const;

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:	

	UMotionControllerComponent* MotionControllerComponent;
	UStaticMeshComponent* ControllerMeshComponent;
};
