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

	/** The mode to use when deducing the type of numeric output pins from the types of the input pins. */
	UPROPERTY(EditAnywhere, Category=Script)
	ENiagaraNumericOutputTypeSelectionMode NumericOutputTypeSelectionMode;

	TArray<FNiagaraDataSetID> ReadDataSets;
	TArray<FNiagaraDataSetProperties> WriteDataSets;
#if WITH_EDITORONLY_DATA
	/** 'Source' data/graphs for this script */
	UPROPERTY()
	class UNiagaraScriptSourceBase*	Source;
#endif

	//~ Begin UObject interface
	virtual void PostLoad() override;
	//~ End UObject interface

};
