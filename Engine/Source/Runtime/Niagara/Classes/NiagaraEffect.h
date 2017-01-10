// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "NiagaraCommon.h"
#include "NiagaraDataSet.h"
#include "NiagaraSimulation.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEffect.generated.h"

/** Represents a binding between a parameter on an effect and a parameter on an emitter. */
USTRUCT()
struct NIAGARA_API FNiagaraParameterBinding
{
	GENERATED_USTRUCT_BODY()

	FNiagaraParameterBinding()
	{
	}

	/**
	 * Creates a new parameter binding
	 * @param InSourceParameterId The id of the source parameter from the effect. 
	 * @param InDestinationId The id of the emitter handle which owns the bound destination parameter.
	 * @param InDestinationParamterId The id of the destination parameter in the emitter.
	 */
	FNiagaraParameterBinding(FGuid InSourceParameterId, FGuid InDestinationEmitterId, FGuid InDestinationParameterId)
		: SourceParameterId(InSourceParameterId)
		, DestinationEmitterId(InDestinationEmitterId)
		, DestinationParameterId(InDestinationParameterId)
	{
	}

	/** Gets the id of the source parameter from the effect. */
	FGuid GetSourceParameterId() const { return SourceParameterId; }

	/** Gets the id of the emitter handle which owns the bound destination parameter. */
	FGuid GetDestinationEmitterId() const { return DestinationEmitterId; }

	/** Gets the id of the destination parameter in the emitter. */
	FGuid GetDestinationParameterId() const { return DestinationParameterId; }

	bool operator == (const FNiagaraParameterBinding &Other) const
	{
		return SourceParameterId == Other.SourceParameterId &&
			DestinationEmitterId == Other.DestinationEmitterId &&
			DestinationParameterId == Other.DestinationParameterId;
	}


private:
	UPROPERTY()
	FGuid SourceParameterId;

	UPROPERTY()
	FGuid DestinationEmitterId;
	UPROPERTY()
	FGuid DestinationParameterId;
};

FORCEINLINE uint32 GetTypeHash(const FNiagaraParameterBinding& Binding)
{
	return GetTypeHash(Binding.GetSourceParameterId()) ^ GetTypeHash(Binding.GetDestinationEmitterId()) ^ GetTypeHash(Binding.GetDestinationParameterId());
}

UCLASS()
class NIAGARA_API UNiagaraEffect : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	//~ UObject interface
	void PostInitProperties();
	virtual void PostLoad() override;

	/** Gets an array of the emitter handles. */
	const TArray<FNiagaraEmitterHandle>& GetEmitterHandles();

	/** Adds a new emitter handle to this effect.  The new handle exposes an Instance value which is a copy of the
		original asset. */
	FNiagaraEmitterHandle AddEmitterHandle(const UNiagaraEmitterProperties& SourceEmitter, FName EmitterName);

	/** Adds a new emitter handle to this effect.  The new handle will not copy the emitter and any changes made to it's
		Instance value will modify the original asset.  This should only be used in the emitter toolkit for simulation
		purposes. */
	FNiagaraEmitterHandle AddEmitterHandleWithoutCopying(UNiagaraEmitterProperties& Emitter);

	/** Duplicates an existing emitter handle and adds it to the effect.  The new handle will reference the same source asset,
		but will have a copy of the duplicated Instance value. */
	FNiagaraEmitterHandle DuplicateEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDuplicate, FName EmitterName);

	/** Removes the provided emitter handle. */
	void RemoveEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDelete);

	/** Removes the emitter handles which have an Id in the supplied set. */
	void RemoveEmitterHandlesById(const TSet<FGuid>& HandlesToRemove);

	FNiagaraEmitterHandle& GetEmitterHandle(int Idx)
	{
		check(Idx < EmitterHandles.Num());
		return EmitterHandles[Idx];
	};

	const FNiagaraEmitterHandle& GetEmitterHandle(int Idx) const
	{
		check(Idx < EmitterHandles.Num());
		return EmitterHandles[Idx];
	};

	int GetNumEmitters()
	{
		return EmitterHandles.Num();
	}

	/** Gets the effect script which is used to populate the effect parameters and parameter bindings. */
	UNiagaraScript* GetEffectScript();

	/** Gets the parameter bindins for this effect. */
	const TArray<FNiagaraParameterBinding>& GetParameterBindings() const;

	/** Adds a parameter binding to the effect. */
	void AddParameterBinding(FNiagaraParameterBinding InParameterBinding);

	/** Removes all parameter bindings from this effect. */
	void ClearParameterBindings();

protected:
	/** Handles to the emitter this effect will simulate. */
	UPROPERTY(VisibleAnywhere, Category = "Emitters")
	TArray<FNiagaraEmitterHandle> EmitterHandles;

	/** The script which defines the effect parameters, and which generates the bindings from effect
		parameter to emitter parameter. */
	UPROPERTY()
	UNiagaraScript* EffectScript;

	/** Bindings from effect parameter to emitter parameter which are generated from the effect script graph. */
	UPROPERTY()
	TArray<FNiagaraParameterBinding> ParameterBindings;
};
