// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UNiagaraNodeInput;
struct FNiagaraVariable;
struct FNiagaraTypeDefinition;
class UNiagaraGraph;
class UNiagaraEffect;

namespace FNiagaraEditorUtilities
{
	/** Determines if the contents of two sets matches */
	// TODO: Move this to TSet.
	template<typename ElementType>
	bool SetsMatch(const TSet<ElementType>& SetA, const TSet<ElementType>& SetB)
	{
		if (SetA.Num() != SetB.Num())
		{
			return false;
		}

		for (ElementType SetItemA : SetA)
		{
			if (SetB.Contains(SetItemA) == false)
			{
				return false;
			}
		}

		return true;
	}

	/** Determines if the contents of an array matches a set */
	// TODO: Move this to TSet.
	template<typename ElementType>
	bool ArrayMatchesSet(const TArray<ElementType>& Array, const TSet<ElementType>& Set)
	{
		if (Array.Num() != Set.Num())
		{
			return false;
		}

		for (ElementType ArrayItem : Array)
		{
			if (Set.Contains(ArrayItem) == false)
			{
				return false;
			}
		}

		return true;
	}

	/** Builds a unique name from a candidate name and a set of existing names.  The candidate name will be made unique 
		if unnecessary by adding a 3 digit index to the end. */
	FName GetUniqueName(FName CandidateName, const TSet<FName>& ExistingNames);

	/** Gets a set of the system constant names. */
	TSet<FName> GetSystemConstantNames();

	/** Resets the variables value to default, either based on the struct, or if available through registered type utilities. */
	void ResetVariableToDefaultValue(FNiagaraVariable& Variable);

	/** Sets up a niagara input node for parameter usage. */
	void InitializeParameterInputNode(UNiagaraNodeInput& InputNode, const FNiagaraTypeDefinition& Type, const UNiagaraGraph* Graph, FName InputName = FName(TEXT("NewInput")));

	/** Options for the GetParameterVariablesFromEffect function. */
	struct FGetParameterVariablesFromEffectOptions
	{
		FGetParameterVariablesFromEffectOptions()
			: bIncludeStructParameters(true)
			, bIncludeDataInterfaceParameters(true)
		{
		}

		bool bIncludeStructParameters;
		bool bIncludeDataInterfaceParameters;
	};

	/** Gets the niagara variables for the input parameters on a niagara effect. */
	void GetParameterVariablesFromEffect(UNiagaraEffect& Effect, TArray<FNiagaraVariable>& ParameterVariables, FGetParameterVariablesFromEffectOptions Options = FGetParameterVariablesFromEffectOptions());
}