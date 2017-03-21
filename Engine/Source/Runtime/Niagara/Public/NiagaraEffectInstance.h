// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraSimulation.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraEffect.h"
#include "NiagaraParameterBindingInstance.h"
#include "NiagaraDataInterfaceBindingInstance.h"
#include "UniquePtr.h"
#include "NiagaraCommon.h"

class NIAGARA_API FNiagaraEffectInstance 
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnInitialized);
	
#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE(FOnReset);
	DECLARE_MULTICAST_DELEGATE(FOnDestroyed);
#endif

public:
	/** Defines modes for updating the effect instances age. */
	enum class EAgeUpdateMode
	{
		/** Update the age using the delta time supplied to the tick function. */
		TickDeltaTime,
		/** Update the age by seeking to the DesiredAge. */
		DesiredAge
	};

public:
	/** Creates a new niagara effect instance with the supplied component. */
	explicit FNiagaraEffectInstance(UNiagaraComponent* InComponent);

	/** Cleanup*/
	virtual ~FNiagaraEffectInstance();

	/** Initializes this effect instance to simulate the supplied effect. */
	void Init(UNiagaraEffect* InEffect, bool bForceReset = false);

	/** Defines modes for resetting the effect instance. */
	enum class EResetMode
	{
		/** Defers resetting the effect instance and simulations until the next tick. */
		DeferredReset,
		/** Resets the effect instance and simulations immediately. */
		ImmediateReset,
		/** same as above, but reinitializes instead of fast resetting */
		DeferredReInit,
		ImmediateReInit
	};
	
	/** Requests the the simulation be reset on the next tick. */
	void Reset(EResetMode Mode);

	/** Updates the simulation with the specified delta. */
	void Tick(float DeltaSeconds);

	/** Gets the simulation for the supplied emitter handle. */
	FNiagaraSimulation* GetSimulationForHandle(const FNiagaraEmitterHandle& EmitterHandle);

	UNiagaraComponent *GetComponent() { return Component.Get(); }
	TArray<TSharedRef<FNiagaraSimulation> > &GetEmitters()	{ return Emitters; }
	FBox &GetEffectBounds()	{ return EffectBounds;  }
	FNiagaraParameters& GetEffectParameters() { return InstanceParameters; }
	void SetParameter(FNiagaraVariable& InParam){ InstanceParameters.SetOrAdd(InParam);	}

	EAgeUpdateMode GetAgeUpdateMode() const;

	/** Sets the age update mode for the effect instance. */
	void SetAgeUpdateMode(EAgeUpdateMode InAgeUpdateMode);

	/** Gets the desired age of the effect instance.  This is only relevant when using the DesiredAge age update mode. */
	float GetDesiredAge() const;

	/** Sets the desired age of the effect instance.  This is only relevant when using the DesiredAge age update mode. */
	void SetDesiredAge(float InDesiredAge);

	/** Gets the delta value which is used when seeking from the current age, to the desired age.  This is only relevant
	when using the DesiredAge age update mode. */
	float GetSeekDelta() const;

	/** Sets the delta value which is used when seeking from the current age, to the desired age.  This is only relevant
		when using the DesiredAge age update mode. */
	void SetSeekDelta(float InSeekDelta);

	/** Gets whether or not spawning new particles has been suppressed for this effect instance. */
	bool GetSuppressSpawning() const;

	/** Sets whether or not spawning new particles has been suppressed for this effect instance. */
	void SetSuppressSpawning(bool bInSuppressSpawning);

	/** Gets a data set either from another emitter or one owned by the effect itself. */
	FNiagaraDataSet* GetDataSet(FNiagaraDataSetID SetID, FName EmitterName = NAME_None);

	/** Gets a multicast delegate which is called whenever this instance is initialized with an effect asset. */
	FOnInitialized& OnInitialized();

#if WITH_EDITOR
	/** Gets a multicast delegate which is called whenever this instance is reset due to external changes in the source effect asset. */
	FOnReset& OnReset();

	FOnDestroyed& OnDestroyed();
#endif

	FName GetIDName() { return IDName; }

	const FNiagaraParameters& GetExternalInstanceParameters() const { return ExternalInstanceParameters; }
	const TArray<FNiagaraScriptDataInterfaceInfo>& GetExternalInstanceDataInterfaces() const { return ExternalInstanceDataInterfaces; }
	void InvalidateComponentBindings();

	/** Queue up the data sources to have PrepareForSimulation called on them next tick.*/
	void ReinitializeDataInterfaces();

private:
	/** Builds the emitter simulations. */
	void InitEmitters();

	/** Resets the effect, emitter simulations, and renderers to initial conditions. */
	void ReInitInternal();
	/** Resets for restart, assumes no change in emitter setup */
	void ResetInternal();

	/** Rebuilds the parameter binding instances. */
	void UpdateParameterBindingInstances();

	/** Rebuilds the data interface binding instances. */
	void UpdateDataInterfaceBindingInstances();

	/** Updates the renders for the simulations. Gathers both the EffectRenderers that were previously set as well as the ones that we  create within.*/
	void UpdateRenderModules(ERHIFeatureLevel::Type InFeatureLevel, TArray<NiagaraEffectRenderer*>& OutNewRenderers, TArray<NiagaraEffectRenderer*>& OutOldRenderers);

	/** Updates the scene proxy for the effect with the specified EffectRenderer array. Note that this is pushed onto the rendering thread behind the scenes.*/
	void UpdateProxy(TArray<NiagaraEffectRenderer*>& InRenderers);

	/** Ticks the effect and emitter simulations. */
	void TickInternal(float DeltaSeconds);

	/** Ticks the effect/emitter internal variable bindings.*/
	void TickEmitterInternalBindings();	
	
	/** Clears out and deletes any local per-instance data interfaces.*/
	void ClearPerInstanceDataInterfaceCache();

	/** Call PrepareForSImulation on each data source from the simulations and determine which need per-tick updates.*/
	void InitDataInterfaces();
	
	/** Perform per-tick updates on data interfaces that need it.*/
	void TickDataInterfaces(float DeltaSeconds);

	TWeakObjectPtr<UNiagaraComponent> Component;
	TWeakObjectPtr<UNiagaraEffect> Effect;
	FBox EffectBounds;

	/** Some data interfaces are required to be per-instance, this is where those instances are stored. Note that these hold on to DataInterfaces, which are UObjects.
	* We use the UNiagaraComponent's InstanceDataInterfaces to maintain ownership of these temporaries. However, we are still
	* responsible for deleting the owning struct info object (it doesn't touch the data interface in its destructor). This is done in ClearPerInstanceDataInterfaceCache().*/
	TArray<TSharedPtr<FNiagaraScriptDataInterfaceInfo>> RequiredPerInstanceDataInterfaces;

	/** Defines the mode use when updating the effect age. */
	EAgeUpdateMode AgeUpdateMode;

	/** The age of the effect instance. */
	float Age;

	/** The desired age of the effect instance.  This is only relevant when using the DesiredAge age update mode. */
	float DesiredAge;

	/** The delta time used when seeking to the desired age.  This is only relevant when using the DesiredAge age update mode. */
	float SeekDelta;

	bool bSuppressSpawning;

	/** Flag to ensure the effect instance is only reset once per frame. */
	bool bResetPending;
	bool bReInitPending;

	/** Instance parameters defined by the effect asset. */
	FNiagaraParameters ExternalInstanceParameters;

	/** Local constant table. */
	FNiagaraParameters InstanceParameters;

	/** Instance data interfaces defined by the effect asset. */
	TArray<FNiagaraScriptDataInterfaceInfo> ExternalInstanceDataInterfaces;
	
	TMap<FNiagaraDataSetID, FNiagaraDataSet> ExternalEvents;

	TArray< TSharedRef<FNiagaraSimulation> > Emitters;

	TArray<FNiagaraParameterBindingInstance> ParameterBindingInstances;
	TArray<FNiagaraDataInterfaceBindingInstance> DataInterfaceBindingInstances;

	FOnInitialized OnInitializedDelegate;

#if WITH_EDITOR
	FOnReset OnResetDelegate;
	FOnDestroyed OnDestroyedDelegate;
#endif

	//Temp method of pushing params into emitter.
	FNiagaraVariable EffectPositionParam;
	FNiagaraVariable EffectVelocityParam;
	FNiagaraVariable EffectAgeParam;
	FNiagaraVariable EffectXAxisParam;
	FNiagaraVariable EffectYAxisParam;
	FNiagaraVariable EffectZAxisParam;
	FNiagaraVariable EffectLocalToWorldParam;
	FNiagaraVariable EffectWorldToLocalParam;
	FNiagaraVariable EffectLocalToWorldTranspParam;
	FNiagaraVariable EffectWorldToLocalTranspParam;

	FGuid ID;
	FName IDName;
	
	/** Data interfaces that need a tick.*/
	TArray<TWeakObjectPtr<UNiagaraDataInterface>> PreSimulationTickInterfaces;
	
	/** Notifier that data interfaces need reinitialization next tick.*/
	bool bDataInterfacesNeedInit;
};
