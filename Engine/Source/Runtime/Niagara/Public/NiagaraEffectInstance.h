// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraSimulation.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraEffect.h"
#include "NiagaraParameterBindingInstance.h"


enum NiagaraTickState
{
	ERunning,		// normally running
	ESuspended,		// stop simulating and spawning, still render
	EDieing,		// stop spawning, still simulate and render
	EDead			// no live particles, no new spawning
};

class NIAGARA_API FNiagaraEffectInstance
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnInitialized);
	
#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE(FOnReset);
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

	/** Initializes this effect instance to simulate the supplied effect. */
	void Init(UNiagaraEffect* InEffect);

	/** Requests the the simulation be reset on the next tick. */
	void RequestResetSimulation();

	/** Updates the simulation with the specified delta. */
	void Tick(float DeltaSeconds);

	/** Gets the simulation for the supplied emitter handle. */
	FNiagaraSimulation* GetSimulationForHandle(const FNiagaraEmitterHandle& EmitterHandle);

	UNiagaraComponent *GetComponent() { return Component; }
	TArray<TSharedRef<FNiagaraSimulation> > &GetEmitters()	{ return Emitters; }
	FBox GetEffectBounds()	{ return EffectBounds;  }
	FNiagaraParameters& GetEffectParameters() { return InstanceParameters; }
	void SetParameter(FNiagaraVariable& InParam){ InstanceParameters.SetOrAdd(InParam);	}

	/** Sets the age update mode for the effect instance. */
	void SetAgeUpdateMode(EAgeUpdateMode InAgeUpdateMode);

	/** Sets the desired age of the effect instance.  This is only relevant when using the DesiredAge age update mode. */
	void SetDesiredAge(float InDesiredAge);

	/** Sets the delta value which is used when seeking from the current age, to the desired age.  This is only relevant
		when using the DesiredAge age update mode. */
	void SetSeekDelta(float InSeekDelta);

	/** Gets a data set either from another emitter or one owned by the effect itself. */
	FNiagaraDataSet* GetDataSet(FNiagaraDataSetID SetID, FName EmitterName = NAME_None);

	/** Gets a multicast delegate which is called whenever this instance is initialized with an effect asset. */
	FOnInitialized& OnInitialized();

#if WITH_EDITOR
	/** Gets a multicast delegate which is called whenever this instance is reset due to external changes in the source effect asset. */
	FOnReset& OnReset();
#endif

	FName GetIDName() { return IDName; }

	const FNiagaraParameters& GetExternalInstanceParameters() const { return ExternalInstanceParameters; }

	void InvalidateComponentBindings();

private:
	/** Builds the emitter simulations. */
	void InitEmitters();

	/** Resets the effect, emitter simulations, and renderers to initial conditions. */
	void ResetInternal();

	/** Rebuilds the parameter binding instances. */
	void UpdateParameterBindingInstances();

	/** Updates the renders for the simulations. */
	void UpdateRenderModules(ERHIFeatureLevel::Type InFeatureLevel);

	/** Updates the scene proxy for the effect. */
	void UpdateProxy();

	/** Ticks the effect and emitter simulations. */
	void TickInternal(float DeltaSeconds);

	UNiagaraComponent* Component;
	UNiagaraEffect *Effect;
	FBox EffectBounds;

	/** Defines the mode use when updating the effect age. */
	EAgeUpdateMode AgeUpdateMode;

	/** The age of the effect instance. */
	float Age;

	/** The desired age of the effect instance.  This is only relevant when using the DesiredAge age update mode. */
	float DesiredAge;

	/** The delta time used when seeking to the desired age.  This is only relevant when using the DesiredAge age update mode. */
	float SeekDelta;

	/** Flag to ensure the effect instance is only reset once per frame. */
	bool bResetPending;

	/** Instance parameters defined by the effect asset. */
	FNiagaraParameters ExternalInstanceParameters;

	/** Local constant table. */
	FNiagaraParameters InstanceParameters;
	
	TMap<FNiagaraDataSetID, FNiagaraDataSet> ExternalEvents;

	TArray< TSharedRef<FNiagaraSimulation> > Emitters;

	TArray<FNiagaraParameterBindingInstance> ParameterBindingInstances;
	
	FOnInitialized OnInitializedDelegate;

#if WITH_EDITOR
	FOnReset OnResetDelegate;
#endif

	//Temp method of pushing params into emitter.
	FNiagaraVariable EffectPositionParam;
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
};