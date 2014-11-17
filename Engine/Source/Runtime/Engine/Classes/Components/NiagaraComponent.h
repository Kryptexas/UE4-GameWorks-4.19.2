// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "NiagaraComponent.generated.h"

UCLASS()
class ENGINE_API UNiagaraComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** The simulation being executed for this component. */
	class FNiagaraSimulation* Simulation;

	/** Script to run for these particles */
	UPROPERTY(EditAnywhere, Category=NiagaraComponent)
	class UNiagaraScript* UpdateScript;

	/** Material with which to render particles. */
	UPROPERTY(EditAnywhere, Category=Rendering)
	UMaterialInterface* Material;

	/** TEMP Spawn rate */
	UPROPERTY(EditAnywhere, Category=NiagaraComponent)
	float SpawnRate;

	// Begin UActorComponent interface.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
protected:
	virtual void OnRegister() override;
	virtual void OnUnregister()  override;
	virtual void SendRenderDynamicData_Concurrent() override;
public:
	// End UActorComponent interface.

	// Begin UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const override;
	virtual class UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual void SetMaterial(int32 ElementIndex, class UMaterialInterface* Material) override;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent Interface

	// Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	// End UObject interface.
};



