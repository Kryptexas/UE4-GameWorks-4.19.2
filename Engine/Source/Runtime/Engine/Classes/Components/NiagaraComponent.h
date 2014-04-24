// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "NiagaraComponent.generated.h"

UCLASS(HeaderGroup=Component)
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
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
protected:
	virtual void OnRegister() OVERRIDE;
	virtual void OnUnregister()  OVERRIDE;
	virtual void SendRenderDynamicData_Concurrent() OVERRIDE;
public:
	// End UActorComponent interface.

	// Begin UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const OVERRIDE;
	virtual class UMaterialInterface* GetMaterial(int32 ElementIndex) const OVERRIDE;
	virtual void SetMaterial(int32 ElementIndex, class UMaterialInterface* Material) OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	// End UPrimitiveComponent Interface

	// Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject interface.
};



