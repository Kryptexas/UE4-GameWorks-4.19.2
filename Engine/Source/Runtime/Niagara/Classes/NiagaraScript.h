// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "NiagaraCommon.h"
#include "NiagaraParameters.h"

#include "NiagaraScript.generated.h"

class UNiagaraDataInterface;

#define NIAGARA_INVALID_MEMORY (0xBA)

/** Defines what will happen to unused attributes when a script is run. */
UENUM()
enum class EUnusedAttributeBehaviour : uint8
{
	/** The previous value of the attribute is copied across. */
	Copy,
	/** The attribute is set to zero. */
	Zero,
	/** The attribute is untouched. */
	None,
	/** The memory for the attribute is set to NIAGARA_INVALID_MEMORY. */
	MarkInvalid, 
	/** The attribute is passed through without double buffering */
	PassThrough,
};

/** Defines different usages for a niagara script. */
UENUM()
enum class ENiagaraScriptUsage : uint8
{
	/** The script defines a function for use in modules and other functions. */
	Function,
	/** The script defines a module for use in emitter scripts. */
	Module,
	/** The script is an emitter spawn script. */
	SpawnScript UMETA(Hidden),
	/** Spawn script using interpolation. */
	SpawnScriptInterpolated UMETA(Hidden),
	/** The script is an emitter update script. */
	UpdateScript UMETA(Hidden),
	/** The script is an effect script. */
	EffectScript UMETA(Hidden)
};


/** Runtime script for a Niagara system */
UCLASS(MinimalAPI)
class UNiagaraScript : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Byte code to execute for this system */
	UPROPERTY()
	TArray<uint8> ByteCode;

	/** All the data for using constants in the script. */
	UPROPERTY()
	FNiagaraParameters Parameters;

	UPROPERTY()
	FNiagaraParameters InternalParameters;

	/** Attributes used by this script. */
	UPROPERTY()
 	TArray<FNiagaraVariable> Attributes;

	/** Information about the events this script receives and which variables are accessed. */
	UPROPERTY()
	TArray<FNiagaraDataSetProperties> EventReceivers;

	/** Information about the events this script generates and which variables are written. */
	UPROPERTY()
	TArray<FNiagaraDataSetProperties> EventGenerators;

	/** Contains various usage information for this script. */
	UPROPERTY()
	FNiagaraScriptDataUsageInfo DataUsage;

	/** Gets how this script is to be used. */
	UPROPERTY(AssetRegistrySearchable, EditAnywhere, Category=Script)
	ENiagaraScriptUsage Usage;

	/** Information about all data interfaces used by this script. */
	UPROPERTY()
	TArray<FNiagaraScriptDataInterfaceInfo> DataInterfaceInfo;

	/** Array of ordered vm external functions to place in the function table. */
	UPROPERTY()
	TArray<FVMExternalFunctionBindingInfo> CalledVMExternalFunctions;

	/** The mode to use when deducing the type of numeric output pins from the types of the input pins. */
	UPROPERTY(EditAnywhere, Category=Script)
	ENiagaraNumericOutputTypeSelectionMode NumericOutputTypeSelectionMode;

	TArray<FNiagaraDataSetID> ReadDataSets;
	TArray<FNiagaraDataSetProperties> WriteDataSets;

	/** Scopes we'll track with stats.*/
	UPROPERTY()
	TArray<FNiagaraStatScope> StatScopes;
	
#if WITH_EDITORONLY_DATA
	/** 'Source' data/graphs for this script */
	UPROPERTY()
	class UNiagaraScriptSourceBase*	Source;

	UPROPERTY(EditAnywhere, Category = Script)
	FText Description;

	FText GetDescription() { return Description.IsEmpty() ? FText::FromString(GetName()) : Description; }

	/** Adjusted every time that we compile this script. Lets us know that we might differ from any cached versions.*/
	UPROPERTY()
	FGuid ChangeId;

	/** Last known compile status. Lets us determine the latest state of the script byte buffer.*/
	UPROPERTY()
	ENiagaraScriptCompileStatus LastCompileStatus;

	/** Makes a deep copy of any script depedencies, including itself.*/
	virtual UNiagaraScript* MakeRecursiveDeepCopy(UObject* DestOuter) const;

	/** Determine if there are any external dependencies with respect to scripts and ensure that those dependencies are sucked into the existing package.*/
	virtual void SubsumeExternalDependencies(TMap<UObject*, UObject*>& ExistingConversions);

	/** Determine if the Script and its source graph are in sync.*/
	NIAGARA_API bool AreScriptAndSourceSynchronized() const;
	
	/** Ensure that the Script and its source graph are marked out of sync.*/
	NIAGARA_API void MarkScriptAndSourceDesynchronized();
#endif
	
	bool IsSpawnScript() { return Usage == ENiagaraScriptUsage::SpawnScript || Usage == ENiagaraScriptUsage::SpawnScriptInterpolated; }
	bool IsInterpolatedSpawnScript() { return Usage == ENiagaraScriptUsage::SpawnScriptInterpolated; }
	bool IsUpdateScript() { return Usage == ENiagaraScriptUsage::UpdateScript; }
	bool IsModuleScript() { return Usage == ENiagaraScriptUsage::Module; }
	bool IsFunctionScript() { return Usage == ENiagaraScriptUsage::Function; }
	bool IsEffectScript() { return Usage == ENiagaraScriptUsage::EffectScript; }

	/** For spawn scripts only, returns it's companion update script. */
	NIAGARA_API UNiagaraScript* GetCompanionUpdateScript();
	/** For update scripts only, returns it's companion spawn script. */
	NIAGARA_API UNiagaraScript* GetCompanionSpawnScript();

	//~ Begin UObject interface
	void Serialize(FArchive& Ar)override;
	virtual void PostLoad() override;
	//~ End UObject interface

	virtual ~UNiagaraScript();
};
