// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraTypes.h"
#include "NiagaraCommon.generated.h"


enum ENiagaraSimTarget
{
	CPUSim,
	GPUComputeSim
};


enum ENiagaraTickState
{
	NTS_Running,
	NTS_Suspended,
	NTS_Dieing,
	NTS_Dead
};


UENUM()
enum class ENiagaraDataSetType : uint8
{
	ParticleData,
	Shared,
	Event,
};


UENUM()
enum class ENiagaraInputNodeUsage : uint8
{
	Undefined = 0,
	Parameter,
	Attribute,
	SystemConstant
};

/**
* Enumerates states a Niagara script can be in.
*/
UENUM()
enum class ENiagaraScriptCompileStatus : uint8
{
	/** Niagara script is in an unknown state. */
	NCS_Unknown,
	/** Niagara script has been modified but not recompiled. */
	NCS_Dirty,
	/** Niagara script tried but failed to be compiled. */
	NCS_Error,
	/** Niagara script has been compiled since it was last modified. */
	NCS_UpToDate,
	/** Niagara script is in the process of being created for the first time. */
	NCS_BeingCreated,
	/** Niagara script has been compiled since it was last modified. There are warnings. */
	NCS_UpToDateWithWarnings,
	NCS_MAX,
};

USTRUCT()
struct FNiagaraDataSetID
{
	GENERATED_USTRUCT_BODY()

	FNiagaraDataSetID()
	: Name(NAME_None)
	, Type(ENiagaraDataSetType::Event)
	{}

	FNiagaraDataSetID(FName InName, ENiagaraDataSetType InType)
		: Name(InName)
		, Type(InType)
	{}

	UPROPERTY(EditAnywhere, Category = "Data Set")
	FName Name;

	UPROPERTY()
	ENiagaraDataSetType Type;

	FORCEINLINE bool operator==(const FNiagaraDataSetID& Other)const
	{
		return Name == Other.Name && Type == Other.Type;
	}

	FORCEINLINE bool operator!=(const FNiagaraDataSetID& Other)const
	{
		return !(*this == Other);
	}
};


FORCEINLINE FArchive& operator<<(FArchive& Ar, FNiagaraDataSetID& VarInfo)
{
	Ar << VarInfo.Name << VarInfo.Type;
	return Ar;
}

FORCEINLINE uint32 GetTypeHash(const FNiagaraDataSetID& Var)
{
	return HashCombine(GetTypeHash(Var.Name), (uint32)Var.Type);
}

USTRUCT()
struct FNiagaraDataSetProperties
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = "Data Set")
	FNiagaraDataSetID ID;

	UPROPERTY()
	TArray<FNiagaraVariable> Variables;
};

/** Information about an input or output of a Niagara operation node. */
class FNiagaraOpInOutInfo
{
public:
	FName Name;
	FNiagaraTypeDefinition DataType;
	FText FriendlyName;
	FText Description;
	FString Default;
	FString HlslSnippet;

	FNiagaraOpInOutInfo(FName InName, FNiagaraTypeDefinition InType, FText InFriendlyName, FText InDescription, FString InDefault, FString InHlslSnippet = TEXT(""))
		: Name(InName)
		, DataType(InType)
		, FriendlyName(InFriendlyName)
		, Description(InDescription)
		, Default(InDefault)
		, HlslSnippet(InHlslSnippet)
	{

	}
};


/** Struct containing usage information about a script. Things such as whether it reads attribute data, reads or writes events data etc.*/
USTRUCT()
struct FNiagaraScriptDataUsageInfo
{
	GENERATED_BODY()

		FNiagaraScriptDataUsageInfo()
		: bReadsAttriubteData(false)
	{}

	/** If true, this script reads attribute data. */
	UPROPERTY()
		bool bReadsAttriubteData;
};


USTRUCT()
struct NIAGARA_API FNiagaraFunctionSignature
{
	GENERATED_BODY()

	/** Name of the function. */
	UPROPERTY()
	FName Name;
	/** Input parameters to this function. */
	UPROPERTY()
	TArray<FNiagaraVariable> Inputs;
	/** Input parameters of this function. */
	UPROPERTY()
	TArray<FNiagaraVariable> Outputs;
	/** Id of the owner is this is a member function. */
	UPROPERTY()
	FGuid OwnerId;
	UPROPERTY()
	bool bRequiresContext;
	/** True if this is the signature for a "member" function of a data interface. If this is true, the first input is the owner. */
	UPROPERTY()
	bool bMemberFunction;

	/** Localized description of this node. Note that this is *not* used during the operator == below since it may vary from culture to culture.*/
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FText Description;
#endif

	FNiagaraFunctionSignature() {}
	FNiagaraFunctionSignature(FName InName, TArray<FNiagaraVariable>& InInputs, TArray<FNiagaraVariable>& InOutputs, FName InSource, bool bInRequiresContext, bool bInMemberFunction)
		: Name(InName)
		, Inputs(InInputs)
		, Outputs(InOutputs)
		, bRequiresContext(bInRequiresContext)
		, bMemberFunction(bInMemberFunction)
	{

	}

	bool operator==(const FNiagaraFunctionSignature& Other) const
	{
		return Name == Other.Name && Inputs == Other.Inputs && Outputs == Other.Outputs && bRequiresContext == Other.bRequiresContext && bMemberFunction == Other.bMemberFunction && OwnerId == Other.OwnerId;
	}

	FString GetName()const { return Name.ToString(); }

	void SetDescription(const FText& Desc)
	{
	#if WITH_EDITORONLY_DATA
		Description = Desc;
	#endif
	}
	FText GetDescription() const
	{
	#if WITH_EDITORONLY_DATA
		return Description;
	#else
		return FText::FromName(Name);
	#endif
	}
	bool IsValid()const { return Name != NAME_None && (Inputs.Num() > 0 || Outputs.Num() > 0); }
};



USTRUCT()
struct NIAGARA_API FNiagaraScriptDataInterfaceInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FNiagaraScriptDataInterfaceInfo()
		: DataInterface(nullptr)
		, Name(NAME_None)
	{

	}

	UPROPERTY(Instanced)
	class UNiagaraDataInterface* DataInterface;

	/** Generated by the compiler as source UI FNiagaraVariable gets baked down. Used at runtime to hook up any overrides.*/
	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FName Name;

	//TODO: Allow data interfaces to expose their own parameters. E.g. Curve Min/Max, SkeletalMesh NumBones, Local Transform etc.
	/** All parameters for this data interface used by this script. */
	//UPROPERTY()
	//TArray<FNiagaraVariable> Parameters;

	//TODO: Allow data interfaces to own datasets
	void CopyTo(FNiagaraScriptDataInterfaceInfo* Destination, UObject* Outer) const;
};

USTRUCT()
struct FNiagaraStatScope
{
	GENERATED_USTRUCT_BODY();

	FNiagaraStatScope() {}
	FNiagaraStatScope(FName InFullName, FText InFriendlyName):FullName(InFullName), FriendlyName(InFriendlyName){}
	FName FullName;
	FText FriendlyName;

	bool operator==(const FNiagaraStatScope& Other) const { return FullName == Other.FullName; }
};

USTRUCT()
struct FVMExternalFunctionBindingInfo
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FGuid OwnerId;

	UPROPERTY()
	TArray<bool> InputParamLocations;

	UPROPERTY()
	int32 NumOutputs;

	FORCEINLINE int32 GetNumInputs()const { return InputParamLocations.Num(); }
	FORCEINLINE int32 GetNumOutputs()const { return NumOutputs; }
};