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

USTRUCT()
struct NIAGARA_API FNiagaraEmitterInternalVariableBinding
{
	GENERATED_USTRUCT_BODY()

	FNiagaraEmitterInternalVariableBinding()
	{
	}

	bool operator == (const FNiagaraEmitterInternalVariableBinding &Other) const
	{
		return SourceParameterId == Other.SourceParameterId &&
			DestinationEmitterId == Other.DestinationEmitterId &&
			DestinationEmitterVariableName == Other.DestinationEmitterVariableName;
	}

	UPROPERTY()
	FGuid SourceParameterId;

	UPROPERTY()
	FGuid DestinationEmitterId;

	UPROPERTY()
	FString DestinationEmitterVariableName;
};

FORCEINLINE uint32 GetTypeHash(const FNiagaraEmitterInternalVariableBinding& Binding)
{
	return GetTypeHash(Binding.SourceParameterId) ^ GetTypeHash(Binding.DestinationEmitterId) ^ GetTypeHash(Binding.DestinationEmitterVariableName);
}

UCLASS()
class NIAGARA_API UNiagaraEffect : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	//~ UObject interface
	void PostInitProperties();
	void Serialize(FArchive& Ar)override;
	virtual void PostLoad() override;

	/** Gets an array of the emitter handles. */
	const TArray<FNiagaraEmitterHandle>& GetEmitterHandles();

#if WITH_EDITORONLY_DATA
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
#endif

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

	/** Gets the DataInterface bindings for this effect. */
	const TArray<FNiagaraParameterBinding>& GetDataInterfaceBindings() const;

	/** Adds a DataInterface binding to the effect. */
	void AddDataInterfaceBinding(FNiagaraParameterBinding InDataInterfaceBinding);

	/** Removes all DataInterface bindings from this effect. */
	void ClearDataInterfaceBindings();

	/** Allows Niagara variables in the effect to drive simulator internal variables that would otherwise be unavailable.*/
	void AddEmitterInternalVariableBinding(const FGuid& EffectParamId, const FGuid& EmitterId, const FString& EmitterInternalParamName);

	/** Removes all internal emitter variable bindings from this effect. */
	void ClearEmitterInternalVariableBindings();

	/** Accessors for the bindings for emitter variables.*/
	const TArray<FNiagaraEmitterInternalVariableBinding>& GetEmitterInternalVariableBindings() const;

#if WITH_EDITORONLY_DATA
	/** Get whether or not we auto-import applied changes. See bAutoImportChangedEmitters member documentation for details.*/
	bool GetAutoImportChangedEmitters() const;
	/** Set whether or not we auto-import applied changes. See bAutoImportChangedEmitters member documentation for details.*/
	void SetAutoImportChangedEmitters(bool bShouldImport);
		
	/** Called to query whether or not this emitter is referenced as the source to any emitter handles for this effect.*/
	bool ReferencesSourceEmitter(UNiagaraEmitterProperties* Emitter);

	/** Goes through all handles on the effect and compares them to the latest on their source. Refreshes from source if needed. Returns true if any were refreshed.*/
	bool ResynchronizeAllHandles();

	/** Recompile all emitters and the effect script associated with this effect.*/
	void Compile();

	/** Compile just the effect script associated with this effect.*/
	ENiagaraScriptCompileStatus CompileEffectScript(FString& OutCompileErrors);
#endif
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

	/** Bindings from effect data interface to emitter data interface which are generated from the effect script graph. */
	UPROPERTY()
	TArray<FNiagaraParameterBinding> DataInterfaceBindings;

	/** Bindings from effect variables to emitter variables which are generated from the effect script graph. */
	UPROPERTY()
	TArray<FNiagaraEmitterInternalVariableBinding> InternalEmitterVariableBindings;

#if WITH_EDITORONLY_DATA
	/** Effects are the final step in the process of creating a Niagara system. Artists may wish to lock an effect so that it only uses
	the handle's cached version of the scripts, rather than the external assets that may be subject to changes. If this flag is set, we will only update
	the emitters if told to do so explicitly by the user.*/
	UPROPERTY()
	uint32 bAutoImportChangedEmitters : 1;	
#endif
};
