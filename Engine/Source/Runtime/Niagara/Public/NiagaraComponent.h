// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraCommon.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Components/PrimitiveComponent.h"

#include "NiagaraComponent.generated.h"

class FMeshElementCollector;
class FNiagaraEffectInstance;
class NiagaraEffectRenderer;
class UNiagaraEffect;

/**
* UNiagaraComponent is the primitive component for a Niagara effect.
* @see ANiagaraActor
* @see UNiagaraEffect
*/
UCLASS(editinlinenew)
class NIAGARA_API UNiagaraComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()
private:
	UPROPERTY(VisibleAnywhere, Category="Niagara", meta = (DisplayName = "Niagara Effect Asset", DisableCopyPaste))
	UNiagaraEffect* Asset;

	TSharedPtr<FNiagaraEffectInstance> EffectInstance;
	
	//~ Begin UActorComponent Interface.
protected:
	virtual void OnRegister() override;
	virtual void SendRenderDynamicData_Concurrent() override;
public:
	UPROPERTY(EditAnywhere, Category="Niagara")
	TArray<FNiagaraVariable> EffectParameterLocalOverrides;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual const UObject* AdditionalStatObject() const override;
	//~ End UActorComponent Interface.

	//~ Begin UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface

	void SetAsset(UNiagaraEffect* InAsset);
	UNiagaraEffect* GetAsset() const { return Asset; }

	TSharedPtr<FNiagaraEffectInstance> GetEffectInstance() const;
	TSharedRef<FNiagaraEffectInstance> GetEffectInstance();

	//~ Begin UObject Interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Compare local overrides with the source Effect. Remove any that have mismatched types or no longer exist on the Effect. Returns whether or not any changes occurred.*/
	virtual bool SynchronizeWithSourceEffect();
#endif // WITH_EDITOR
	//~ End UObject Interface.

	static const TArray<FNiagaraVariable>& GetSystemConstants();
	static const FNiagaraVariable *FindSystemConstant(const FNiagaraVariable& InVar);
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
	TArray<class NiagaraEffectRenderer*>& GetEffectRenderers() { return EffectRenderers; }
	void AddEffectRenderer(NiagaraEffectRenderer* Renderer)	{ EffectRenderers.Add(Renderer); }
	void UpdateEffectRenderers(FNiagaraEffectInstance* InEffect);

private:
	void ReleaseRenderThreadResources();

	//~ Begin FPrimitiveSceneProxy Interface.
	virtual void CreateRenderThreadResources() override;

	//virtual void OnActorPositionChanged() override;
	virtual void OnTransformChanged() override;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

	/*
	virtual bool CanBeOccluded() const override
	{
	return !MaterialRelevance.bDisableDepthTest;
	}
	*/
	virtual uint32 GetMemoryFootprint() const override;

	uint32 GetAllocatedSize() const;


private:
	//class NiagaraEffectRenderer* EffectRenderer;
	TArray<class NiagaraEffectRenderer*>EffectRenderers;
};
