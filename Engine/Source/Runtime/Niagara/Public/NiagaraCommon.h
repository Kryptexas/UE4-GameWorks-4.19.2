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
	/** The full name of the Owner. */
	UPROPERTY()
	FName Owner;
	UPROPERTY()
	bool bRequiresContext;
	/** True if this is the signature for a "member" function of a data interface. If this is true, the first input is the owner. */
	UPROPERTY()
	bool bMemberFunction;

	FNiagaraFunctionSignature() {}
	FNiagaraFunctionSignature(FName InName, TArray<FNiagaraVariable>& InInputs, TArray<FNiagaraVariable>& InOutputs, FName InSource, bool bInRequiresContext, bool bInMemberFunction)
		: Name(InName)
		, Inputs(InInputs)
		, Outputs(InOutputs)
		, Owner(InSource)
		, bRequiresContext(bInRequiresContext)
		, bMemberFunction(bInMemberFunction)
	{

	}

	bool operator==(const FNiagaraFunctionSignature& Other) const
	{
		bool bRet = Name == Other.Name && Inputs == Other.Inputs && Outputs == Other.Outputs && bRequiresContext == Other.bRequiresContext && bMemberFunction == Other.bMemberFunction;
		
		if (bMemberFunction)
		{
			//If we're a member function then the owners are allowed to differ.
			bRet &= Owner == Other.Owner;
		}
		else
		{
			//If no then the same function with different owners is an error.
			//I suppose we can allow this but not sure we should.
			check(!bRet || Owner == Other.Owner);
		}
		return bRet;
	}

	FString GetName()const { return Name.ToString(); }
	FString GetSymbol()const;

	bool IsValid()const { return Name != NAME_None && Owner != NAME_None && (Inputs.Num() > 0 || Outputs.Num() > 0); }
};



USTRUCT()
struct FNiagaraScriptDataInterfaceInfo
{
	GENERATED_USTRUCT_BODY()
public:
	FNiagaraScriptDataInterfaceInfo()
		: DataInterface(nullptr)
	{

	}

	UPROPERTY(Instanced)
	class UNiagaraDataInterface* DataInterface;

	/** All external functions used by this data interface in this script. */
	UPROPERTY()
	TArray<FNiagaraFunctionSignature> ExternalFunctions;

	//TODO: Allow data interfaces to expose their own parameters. E.g. Curve Min/Max, SkeletalMesh NumBones, Local Transform etc.
	/** All parameters for this data interface used by this script. */
	//UPROPERTY()
	//TArray<FNiagaraVariable> Parameters;

	//TODO: Allow data interfaces to own datasets
};