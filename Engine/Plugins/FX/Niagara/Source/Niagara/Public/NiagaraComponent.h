// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraCommon.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Components/PrimitiveComponent.h"
#include "NiagaraParameterStore.h"
#include "NiagaraSystemInstance.h"

#include "NiagaraComponent.generated.h"

class FMeshElementCollector;
class NiagaraRenderer;
class UNiagaraSystem;
class UNiagaraParameterCollection;
class UNiagaraParameterCollectionInstance;
class FNiagaraSystemSimulation;

// Called when the particle system is done
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNiagaraSystemFinished, class UNiagaraComponent*, PSystem);

/**
* UNiagaraComponent is the primitive component for a Niagara System.
* @see ANiagaraActor
* @see UNiagaraSystem
*/
UCLASS(ClassGroup = (Rendering, Common), hidecategories = Object, hidecategories = Physics, hidecategories = Collision, showcategories = Trigger, editinlinenew, meta = (BlueprintSpawnableComponent))
class NIAGARA_API UNiagaraComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()
		
	/** Defines modes for updating the component's age. */
	enum class EAgeUpdateMode
	{
		/** Update the age using the delta time supplied to the tick function. */
		TickDeltaTime,
		/** Update the age by seeking to the DesiredAge. */
		DesiredAge
	};

#if WITH_EDITORONLY_DATA
	DECLARE_MULTICAST_DELEGATE(FOnSystemInstanceChanged);
#endif

private:
	UPROPERTY(EditAnywhere, Category="Niagara", meta = (DisplayName = "Niagara System Asset"))
	UNiagaraSystem* Asset;

	/** Initial values for parameter overrides. 
	TODO: This should be a minimal set of explicitly override parameters similar to how parameter collection instances override their collections store. 
	Should expose anything in the "User" namespace.
	*/
	UPROPERTY(EditAnywhere, Category = Parameters)
	FNiagaraParameterStore OverrideParameters;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TMap<FName, bool> EditorOverridesValue;

	FOnSystemInstanceChanged OnSystemInstanceChangedDelegate;
#endif

	/**
	When true, this component's system will be force to update via a slower "solo" path rather than the more optimal batched path with other instances of the same system.
	*/
	UPROPERTY(EditAnywhere, Category = Parameters)
	uint32 bForceSolo : 1;

	TUniquePtr<FNiagaraSystemInstance> SystemInstance;

	/** Defines the mode use when updating the System age. */
	EAgeUpdateMode AgeUpdateMode;
	
	/** The desired age of the System instance.  This is only relevant when using the DesiredAge age update mode. */
	float DesiredAge;

	/** The delta time used when seeking to the desired age.  This is only relevant when using the DesiredAge age update mode. */
	float SeekDelta;

	UPROPERTY()
	uint32 bAutoDestroy : 1;

	UPROPERTY()
	uint32 bRenderingEnabled : 1;

	//~ Begin UActorComponent Interface.
protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual void BeginDestroy() override;
public:
	/**
	* True if we should automatically attach to AutoAttachParent when activated, and detach from our parent when completed.
	* This overrides any current attachment that may be present at the time of activation (deferring initial attachment until activation, if AutoAttachParent is null).
	* When enabled, detachment occurs regardless of whether AutoAttachParent is assigned, and the relative transform from the time of activation is restored.
	* This also disables attachment on dedicated servers, where we don't actually activate even if bAutoActivate is true.
	* @see AutoAttachParent, AutoAttachSocketName, AutoAttachLocationType
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attachment)
		uint32 bAutoManageAttachment : 1;


	virtual void Activate(bool bReset = false)override;
	virtual void Deactivate()override;

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

	TSharedPtr<FNiagaraSystemSimulation, ESPMode::ThreadSafe> GetSystemSimulation();

	void OnSystemComplete();
	void DestroyInstance();

		UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara System Asset"))
	void SetAsset(UNiagaraSystem* InAsset);
	UNiagaraSystem* GetAsset() const { return Asset; }

	void SetForceSolo(bool bInForceSolo) { bForceSolo = bInForceSolo; }
	bool GetForceSolo()const { return bForceSolo; }

	EAgeUpdateMode GetAgeUpdateMode() const;

	/** Sets the age update mode for the System instance. */
	void SetAgeUpdateMode(EAgeUpdateMode InAgeUpdateMode);

	/** Gets the desired age of the System instance.  This is only relevant when using the DesiredAge age update mode. */
	float GetDesiredAge() const;

	/** Sets the desired age of the System instance.  This is only relevant when using the DesiredAge age update mode. */
	void SetDesiredAge(float InDesiredAge);

	/** Gets the delta value which is used when seeking from the current age, to the desired age.  This is only relevant
	when using the DesiredAge age update mode. */
	float GetSeekDelta() const;

	/** Sets the delta value which is used when seeking from the current age, to the desired age.  This is only relevant
	when using the DesiredAge age update mode. */
	void SetSeekDelta(float InSeekDelta);

	void SetAutoDestroy(bool bInAutoDestroy) { bAutoDestroy = bInAutoDestroy; }
	FNiagaraSystemInstance* GetSystemInstance() const;

	/** Returns true if this component forces it's instances to run in "Solo" mode. A sub optimal path required in some situations. */
	bool ForcesSolo()const;

	/** Sets a Niagara FLinearColor parameter by name, overriding locally if necessary.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Variable (LinearColor)"))
	void SetNiagaraVariableLinearColor(const FString& InVariableName, const FLinearColor& InValue);

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

	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Niagara Variable (Actor)"))
	void SetNiagaraVariableActor(const FString& InVariableName, AActor* Actor) {} // TODO sckime ?????? fix this!!!!

	/** Debug accessors for getting positions in blueprints. */
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Get Niagara Emitter Positions"))
	TArray<FVector> GetNiagaraParticlePositions_DebugOnly(const FString& InEmitterName);
	
	/** Debug accessors for getting a float attribute array in blueprints. */
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Get Niagara Emitter Float Attrib"))
	TArray<float> GetNiagaraParticleValues_DebugOnly(const FString& InEmitterName, const FString& InValueName);
	
	/** Debug accessors for getting a FVector attribute array in blueprints. */
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Get Niagara Emitter Vec3 Attrib"))
	TArray<FVector> GetNiagaraParticleValueVec3_DebugOnly(const FString& InEmitterName, const FString& InValueName);

	/** Resets the System to it's initial pre-simulated state. */
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Reset System"))
	void ResetSystem();

	/** Called on when an external object wishes to force this System to reinitialize itself from the System data.*/
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Reinitialize System"))
	void ReinitializeSystem();

	/** Gets whether or not rendering is enabled for this component. */
	bool GetRenderingEnabled() const;

	/** Sets whether or not rendering is enabled for this component. */
	UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName = "Set Rendering Enabled"))
	void SetRenderingEnabled(bool bInRenderingEnabled);

	//~ Begin UObject Interface.
	virtual void PostLoad();
#if WITH_EDITOR
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Compare local overrides with the source System. Remove any that have mismatched types or no longer exist on the System. Returns whether or not any changes occurred.*/
	virtual bool SynchronizeWithSourceSystem();

	bool IsParameterValueOverriddenLocally(const FName& InParamName);
	void SetParameterValueOverriddenLocally(const FNiagaraVariable& InParam, bool bInOverridden);
	
	FOnSystemInstanceChanged& OnSystemInstanceChanged() { return OnSystemInstanceChangedDelegate; }
#endif

	FNiagaraParameterStore& GetOverrideParameters() { return OverrideParameters; }

	//~ End UObject Interface.

	// Called when the particle system is done
	UPROPERTY(BlueprintAssignable)
	FOnNiagaraSystemFinished OnSystemFinished;

public:
	/**
	 * Component we automatically attach to when activated, if bAutoManageAttachment is true.
	 * If null during registration, we assign the existing AttachParent and defer attachment until we activate.
	 * @see bAutoManageAttachment
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category=Attachment, meta=(EditCondition="bAutoManageAttachment"))
	TWeakObjectPtr<USceneComponent> AutoAttachParent;

	/**
	 * Socket we automatically attach to on the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attachment, meta=(EditCondition="bAutoManageAttachment"))
	FName AutoAttachSocketName;

	/**
	 * Options for how we handle our location when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule AutoAttachLocationRule;

	/**
	 * Options for how we handle our rotation when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule AutoAttachRotationRule;

	/**
	 * Options for how we handle our scale when we attach to the AutoAttachParent, if bAutoManageAttachment is true.
	 * @see bAutoManageAttachment, EAttachmentRule
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attachment, meta = (EditCondition = "bAutoManageAttachment"))
	EAttachmentRule AutoAttachScaleRule;


	/**
	 * Set AutoAttachParent, AutoAttachSocketName, AutoAttachLocationRule, AutoAttachRotationRule, AutoAttachScaleRule to the specified parameters. Does not change bAutoManageAttachment; that must be set separately.
	 * @param  Parent			Component to attach to. 
	 * @param  SocketName		Socket on Parent to attach to.
	 * @param  LocationRule		Option for how we handle our location when we attach to Parent.
	 * @param  RotationRule		Option for how we handle our rotation when we attach to Parent.
	 * @param  ScaleRule		Option for how we handle our scale when we attach to Parent.
	 * @see bAutoManageAttachment, AutoAttachParent, AutoAttachSocketName, AutoAttachLocationRule, AutoAttachRotationRule, AutoAttachScaleRule
	 */
	UFUNCTION(BlueprintCallable, Category = Niagara)
	void SetAutoAttachmentParameters(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule, EAttachmentRule ScaleRule);

private:

	/** Did we auto attach during activation? Used to determine if we should restore the relative transform during detachment. */
	uint32 bDidAutoAttach : 1;

	/** Restore relative transform from auto attachment and optionally detach from parent (regardless of whether it was an auto attachment). */
	void CancelAutoAttachment(bool bDetachFromParent);


	/** Saved relative transform before auto attachment. Used during detachment to restore the transform if we had automatically attached. */
	FVector SavedAutoAttachRelativeLocation;
	FRotator SavedAutoAttachRelativeRotation;
	FVector SavedAutoAttachRelativeScale3D;
};






/**
* Scene proxy for drawing niagara particle simulations.
*/
class FNiagaraSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override;

	FNiagaraSceneProxy(const UNiagaraComponent* InComponent);
	~FNiagaraSceneProxy();

	/** Called on render thread to assign new dynamic data */
	void SetDynamicData_RenderThread(struct FNiagaraDynamicDataBase* NewDynamicData);
	TArray<class NiagaraRenderer*>& GetEmitterRenderers() { return EmitterRenderers; }
	void AddEmitterRenderer(NiagaraRenderer* Renderer);
	void UpdateEmitterRenderers(TArray<NiagaraRenderer*>& InRenderers);

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
	//class NiagaraRenderer* EmitterRenderer;
	TArray<class NiagaraRenderer*>EmitterRenderers;

	bool bRenderingEnabled;
};
