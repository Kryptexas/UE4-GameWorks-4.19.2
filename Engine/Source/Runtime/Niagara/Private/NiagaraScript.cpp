// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScript.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraComponent.h"
#include "Package.h"



UNiagaraScript::UNiagaraScript(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Usage(ENiagaraScriptUsage::Function)
	, NumericOutputTypeSelectionMode(ENiagaraNumericOutputTypeSelectionMode::Largest)
{

}

UNiagaraScriptSourceBase::UNiagaraScriptSourceBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UNiagaraScript::PostLoad()
{
	Super::PostLoad();

	if (GIsEditor)
	{
		
		// We may have inadvertantly saved invalid ID's in the past. In this case, that means that the 
		// information will be invalid in the script, so let's clear out the bytecode and remove
		// any of the invalid attributes.
		TArray<int32> IndicesToRemove;
		for (int32 i = 0; i < Parameters.Parameters.Num(); i++)
		{
			if (Parameters.Parameters[i].GetId().IsValid() == false)
			{
				const FNiagaraVariable* FoundSystemVar = UNiagaraComponent::FindSystemConstant(Parameters.Parameters[i]);
				if (FoundSystemVar == nullptr)
				{
					IndicesToRemove.Add(i);
				}
			}
		}


		// Clear out the bytecode if we found any bad values.
		if (IndicesToRemove.Num() > 0)
		{
			ByteCode.Empty();
			FString PathName = GetOutermost()->GetPathName();

			// Remove the invalid entries
			for (int32 j = IndicesToRemove.Num() - 1; j >= 0; j--)
			{
				UE_LOG(LogNiagara, Warning, TEXT("Non-system parameter '%s' has an invalid GUID saved. Package '%s' will need to be recompiled and saved again."), 
					*Parameters.Parameters[IndicesToRemove[j]].GetName().ToString(), *PathName);
				Parameters.Parameters.RemoveAt(IndicesToRemove[j]);
			}
		}
	}
}