// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraComponent.generated.h"

UENUM()
enum ERenderModuleType
{
	Sprites = 0,
	Ribbon
};


UCLASS()
class ENGINE_API UNiagaraComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** The simulation being executed for this component. */
	class FNiagaraSimulation* Simulation;

	/** Script to run for these particles */
	UPROPERTY(EditAnywhere, Category=NiagaraComponent)
	class UNiagaraScript* UpdateScript;
	UPROPERTY(EditAnywhere, Category = NiagaraComponent)
	class UNiagaraScript* SpawnScript;

	/** The method for rendering the simulated data */
	UPROPERTY(EditAnywhere, Category=Rendering)
	TEnumAsByte<ERenderModuleType> RenderModuleType;

	/** Material with which to render particles. */
	UPROPERTY(EditAnywhere, Category=Rendering)
	UMaterialInterface* Material;

	/** TEMP Spawn rate */
	UPROPERTY(EditAnywhere, Category=NiagaraComponent)
	float SpawnRate;

	/** age of the emitter in seconds */
	float EmitterAge;

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






/**
* Scene proxy for drawing niagara particle simulations.
*/
class FNiagaraSceneProxy : public FPrimitiveSceneProxy
{
public:

	FNiagaraSceneProxy(const UNiagaraComponent* InComponent);
	~FNiagaraSceneProxy();

	/** Called on render thread to assign new dynamic data */
	void SetDynamicData_RenderThread(struct FNiagaraDynamicDataBase* NewDynamicData);
	class NiagaraEffectRenderer *GetEffectRenderer() { return EffectRenderer; }

private:
	void ReleaseRenderThreadResources();

	// FPrimitiveSceneProxy interface.
	virtual void CreateRenderThreadResources() override;

	virtual void OnActorPositionChanged() override;
	virtual void OnTransformChanged() override;

	virtual void PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) override;
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)  override;
	/*
	virtual bool CanBeOccluded() const override
	{
	return !MaterialRelevance.bDisableDepthTest;
	}
	*/
	virtual uint32 GetMemoryFootprint() const override;

	uint32 GetAllocatedSize() const;


private:
	class NiagaraEffectRenderer *EffectRenderer;
};
