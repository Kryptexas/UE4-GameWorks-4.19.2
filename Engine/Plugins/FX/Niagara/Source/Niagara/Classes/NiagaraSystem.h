// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "NiagaraCommon.h"
#include "NiagaraDataSet.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraParameterCollection.h"
#include "NiagaraSystem.generated.h"

USTRUCT()
struct FNiagaraEmitterSpawnAttributes
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FName> SpawnAttributes;
};

/** Container for multiple emitters that combine together to create a particle system effect.*/
UCLASS(BlueprintType)
class NIAGARA_API UNiagaraSystem : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	//~ UObject interface
	void PostInitProperties();
	void Serialize(FArchive& Ar)override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Gets an array of the emitter handles. */
	const TArray<FNiagaraEmitterHandle>& GetEmitterHandles();
	const TArray<FNiagaraEmitterHandle>& GetEmitterHandles()const;

	/** Returns true if this system is valid and can be instanced. False otherwise. */
	bool IsValid()const;

#if WITH_EDITORONLY_DATA
	/** Adds a new emitter handle to this System.  The new handle exposes an Instance value which is a copy of the
		original asset. */
	FNiagaraEmitterHandle AddEmitterHandle(UNiagaraEmitter& SourceEmitter, FName EmitterName);

	/** Adds a new emitter handle to this System.  The new handle will not copy the emitter and any changes made to it's
		Instance value will modify the original asset.  This should only be used in the emitter toolkit for simulation
		purposes. */
	FNiagaraEmitterHandle AddEmitterHandleWithoutCopying(UNiagaraEmitter& Emitter);

	/** Duplicates an existing emitter handle and adds it to the System.  The new handle will reference the same source asset,
		but will have a copy of the duplicated Instance value. */
	FNiagaraEmitterHandle DuplicateEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDuplicate, FName EmitterName);

	/** Removes the provided emitter handle. */
	void RemoveEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDelete);

	/** Removes the emitter handles which have an Id in the supplied set. */
	void RemoveEmitterHandlesById(const TSet<FGuid>& HandlesToRemove);
#endif


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

	/** From the last compile, what are the variables that were exported out of the system for external use?*/
	const FNiagaraParameterStore& GetExposedParameters() const {	return ExposedParameters; }
	FNiagaraParameterStore& GetExposedParameters()  { return ExposedParameters; }


	/** Gets the System script which is used to populate the System parameters and parameter bindings. */
	UNiagaraScript* GetSystemSpawnScript(bool bSolo=false);
	UNiagaraScript* GetSystemUpdateScript(bool bSolo=false);

	bool IsReadyToRun() const;

#if WITH_EDITORONLY_DATA
	/** Called to query whether or not this emitter is referenced as the source to any emitter handles for this System.*/
	bool ReferencesSourceEmitter(UNiagaraEmitter& Emitter);

	/** Updates all handles which use this emitter as their source. */
	void UpdateFromEmitterChanges(UNiagaraEmitter& ChangedSourceEmitter);

	/** Updates the system's rapid iteration parameters from a specific emitter. */
	void RefreshSystemParametersFromEmitter(const FNiagaraEmitterHandle& EmitterHandle);

	/** Removes the system's rapid iteration parameters for a specific emitter. */
	void RemoveSystemParametersForEmitter(const FNiagaraEmitterHandle& EmitterHandle);

	/** Recompile all emitters and the System script associated with this System.*/
	void Compile(bool bForce);

	/** Compile just the System script associated with this System.*/
	void CompileScripts(TArray<ENiagaraScriptCompileStatus>& OutScriptStatuses, TArray<FString>& OutGraphLevelErrorMessages, TArray<FString>& PathNames, TArray<UNiagaraScript*>& Scripts, bool bForce);

	/** Gets editor specific data stored with this system. */
	UObject* GetEditorData();

	/** Gets editor specific data stored with this system. */
	const UObject* GetEditorData() const;

	/** Sets editor specific data stored with this system. */
	void SetEditorData(UObject* InEditorData);

	/** Internal: The thumbnail image.*/
	UPROPERTY()
	class UTexture2D* ThumbnailImage;

	/** Internal: Indicates the thumbnail image is out of date.*/
	UPROPERTY()
	uint32 ThumbnailImageOutOfDate : 1;

	bool GetIsolateEnabled() const;
	void SetIsolateEnabled(bool bIsolate);
#endif

	bool ShouldAutoDeactivate() const { return bAutoDeactivate; }
	bool IsLooping() const;

	const TArray<FNiagaraEmitterSpawnAttributes>& GetEmitterSpawnAttributes()const {	return EmitterSpawnAttributes;	};

	bool UsesCollection(const UNiagaraParameterCollection* Collection)const;
#if WITH_EDITORONLY_DATA
	bool UsesScript(const UNiagaraScript* Script)const;
#endif

	FORCEINLINE UNiagaraParameterCollectionInstance* GetParameterCollectionOverride(UNiagaraParameterCollection* Collection)
	{
		UNiagaraParameterCollectionInstance** Found = ParameterCollectionOverrides.FindByPredicate(
			[&](const UNiagaraParameterCollectionInstance* CheckInst)
		{
			return CheckInst && Collection == CheckInst->Collection;
		});

		return Found ? *Found : nullptr;
	}


private:
#if WITH_EDITORONLY_DATA
	INiagaraModule::FMergeEmitterResults MergeChangesForEmitterHandle(FNiagaraEmitterHandle& EmitterHandle);
#endif

protected:

	/** Handles to the emitter this System will simulate. */
	UPROPERTY(VisibleAnywhere, Category = "Emitters")
	TArray<FNiagaraEmitterHandle> EmitterHandles;

	UPROPERTY(EditAnywhere, Category="System")
	TArray<UNiagaraParameterCollectionInstance*> ParameterCollectionOverrides;

// 	/** Category of this system. */
// 	UPROPERTY(EditAnywhere, Category = System)
// 	UNiagaraSystemCategory* Category;

	/** The script which defines the System parameters, and which generates the bindings from System
		parameter to emitter parameter. */
	UPROPERTY()
	UNiagaraScript* SystemSpawnScript;

	/** The script which defines the System parameters, and which generates the bindings from System
	parameter to emitter parameter. */
	UPROPERTY()
	UNiagaraScript* SystemUpdateScript;

	/** Spawn script compiled to be run individually on a single instance of the system rather than batched as the main spawn script */
	UPROPERTY()
	UNiagaraScript* SystemSpawnScriptSolo;

	/** Update script compiled to be run individually on a single instance of the system rather than batched as the main spawn script */
	UPROPERTY()
	UNiagaraScript* SystemUpdateScriptSolo;

	/** Attribute names in the data set that are driving each emitter's spawning. */
	UPROPERTY()
	TArray<FNiagaraEmitterSpawnAttributes> EmitterSpawnAttributes;

	/** Variables exposed to the outside work for tweaking*/
	UPROPERTY()
	FNiagaraParameterStore ExposedParameters;

#if WITH_EDITORONLY_DATA	

	/** Data used by the editor to maintain UI state etc.. */
	UPROPERTY()
	UObject* EditorData;

	bool bIsolateEnabled;
#endif

	UPROPERTY(EditAnywhere, Category = Performance, meta = (ToolTip = "Auto-deactivate system if all emitters are determined to not spawn particles again, regardless of lifetime."))
	bool bAutoDeactivate;

	void InitEmitterSpawnAttributes();
};
