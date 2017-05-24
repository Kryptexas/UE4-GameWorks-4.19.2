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
UCLASS(ClassGroup = (Rendering, Common), hidecategories = (Object, "Components|Activation"), editinlinenew, meta = (BlueprintSpawnableComponent))
class NIAGARA_API UNiagaraComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()
private:
	UPROPERTY(EditAnywhere, Category="Niagara", meta = (DisplayName = "Niagara Effect Asset"))
	UNiagaraEffect* Asset;

	TSharedPtr<FNiagaraEffectInstance> EffectInstance;
	
	//~ Begin UActorComponent Interface.
protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void SendRenderDynamicData_Concurrent() override;

public:
	UPROPERTY(EditAnywhere, Category="Niagara")
	TArray<FNiagaraVariable> EffectParameterLocalOverrides;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	TArray<FNiagaraScriptDataInterfaceInfo> EffectDataInterfaceLocalOverrides;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual const UObject* AdditionalStatObject() const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ End UActorComponent Interface.

	//~ Begin UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent Interface

	void SetAsset(UNiagaraEffect* InAsset);
	UNiagaraEffect* GetAsset() const { return Asset; }

	TSharedPtr<FNiagaraEffectInstance> GetEffectInstance() const;
	TSharedRef<FNiagaraEffectInstance> GetEffectInstance();

	/** Sets a Niagara Vector3 parameter by name, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Variable (Vector4)"))
	void SetNiagaraVariableVec4(const FString& InVariableName, const FVector4& InValue);

	/** Sets a Niagara Vector3 parameter by name, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Variable (Vector3)"))
	void SetNiagaraVariableVec3(const FString& InVariableName, FVector InValue);

	/** Sets a Niagara Vector3 parameter by name, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Variable (Vector2)"))
	void SetNiagaraVariableVec2(const FString& InVariableName, FVector2D InValue);

	/** Sets a Niagara float parameter by name, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Variable (Float)"))
	void SetNiagaraVariableFloat(const FString& InVariableName, float InValue);

	/** Sets a Niagara float parameter by name, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Variable (Bool)"))
	void SetNiagaraVariableBool(const FString& InVariableName, bool InValue);

	/** Sets a local Niagara emitter's spawn rate, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Emitter Spawn Rate"))
	void SetNiagaraEmitterSpawnRate(const FString& InEmitterName, float InValue);

	/** Sets a Niagara static mesh data interface's Source parameter by name, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Static Mesh Data Interface Actor"))
	void SetNiagaraStaticMeshDataInterfaceActor(const FString& InVariableName, AActor* InSource);

	/** Gets a Niagara parameter by id if it exists in the local overrides.*/
	FNiagaraVariable* GetLocalOverrideParameter(FGuid Guid);

	/** Gets a Niagara parameter by name from the owned effect.*/
	const FNiagaraVariable* GetEffectParameter(FName VarName);

	/** Gets a Niagara parameter by id from the owned effect.*/
	const FNiagaraVariable* GetEffectParameter(FGuid ParameterId);

	/** Gets a Niagara data interface by id from the owned effect.*/
	const FNiagaraScriptDataInterfaceInfo* GetEffectDataInterface(FGuid ParameterId);

	/** Gets a Niagara data interface by id from the owned effect.*/
	const FNiagaraScriptDataInterfaceInfo* GetEffectDataInterface(FName VarName);

	/** Gets a Niagara data interface by id if it exists in the local overrides.*/
	FNiagaraScriptDataInterfaceInfo* GetLocalOverrideDataInterface(FGuid Id);

	/** Checks to see if there is an existing overrides, if not it creates it. If the effect doesn't have a variable of the same id, we return nullptr.*/
	FNiagaraScriptDataInterfaceInfo* CopyOnWriteDataInterface(FGuid Id);

	/** Checks to see if there is an existing overrides, if not it creates it. If ReqiuredType doesn't match or
	the effect doesn't have a variable of the same name, we return nullptr.*/
	FNiagaraScriptDataInterfaceInfo* CopyOnWriteDataInterface(FName VarName, UClass* RequiredClass);

	/** Checks to see if there is an existing overrides, if not it creates it. If ReqiuredType doesn't match or
	the effect doesn't have a variable of the same name, we return nullptr.*/
	FNiagaraVariable* CopyOnWriteParameter(FName VarName, FNiagaraTypeDefinition RequiredType);

	/** Checks to see if there is an existing overrides, if not it creates it. If the effect doesn't have a variable of the same id, we return nullptr.*/
	FNiagaraVariable* CopyOnWriteParameter(FGuid ParameterId);

	void ClearLocalOverrideParameter(FGuid ParameterId);

	/** Resets the effect to it's initial pre-simulated state. */
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Reset Effect"))
	void ResetEffect();

	/** Called on when an external object wishes to force this effect to reinitialize itself from the effect data.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Reinitialize Effect"))
	void ReinitializeEffect();

	/** Gets whether or not rendering is enabled for this component. */
	bool GetRenderingEnabled() const;

	/** Sets whether or not rendering is enabled for this component. */
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Rendering Enabled"))
	void SetRenderingEnabled(bool bInRenderingEnabled);

	//~ Begin UObject Interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Compare local overrides with the source Effect. Remove any that have mismatched types or no longer exist on the Effect. Returns whether or not any changes occurred.*/
	virtual bool SynchronizeWithSourceEffect();
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient , DuplicateTransient)
	TArray<UNiagaraDataInterface*> EditorTemporaries;
#endif // WITH_EDITOR
	//~ End UObject Interface.

	UPROPERTY(Transient, DuplicateTransient)
	TArray<UNiagaraDataInterface*> InstanceDataInterfaces;

	static const TArray<FNiagaraVariable>& GetSystemConstants();
	static const FNiagaraVariable *FindSystemConstant(const FNiagaraVariable& InVar);

	UPROPERTY()
	bool bRenderingEnabled;
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
	void AddEffectRenderer(NiagaraEffectRenderer* Renderer);
	void UpdateEffectRenderers(TArray<NiagaraEffectRenderer*>& InRenderers);

	/** Gets whether or not this scene proxy should be rendered. */
	bool GetRenderingEnabled() const;

	/** Sets whether or not this scene proxy should be rendered. */
	void SetRenderingEnabled(bool bInRenderingEnabled);

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

	/** Callback from the renderer to gather simple lights that this proxy wants renderered. */
	virtual void GatherSimpleLights(const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const override;


	virtual uint32 GetMemoryFootprint() const override;

	uint32 GetAllocatedSize() const;

private:
	//class NiagaraEffectRenderer* EffectRenderer;
	TArray<class NiagaraEffectRenderer*>EffectRenderers;

	bool bRenderingEnabled;
};
