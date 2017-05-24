// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScript.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitterProperties.h"
#include "Package.h"

#include "NiagaraCustomVersion.h"

UNiagaraScript::UNiagaraScript(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Usage(ENiagaraScriptUsage::Function)
	, NumericOutputTypeSelectionMode(ENiagaraNumericOutputTypeSelectionMode::Largest)
#if WITH_EDITORONLY_DATA
	, LastCompileStatus(ENiagaraScriptCompileStatus::NCS_Unknown)
#endif
{

}

UNiagaraScript::~UNiagaraScript()
{
}

UNiagaraScriptSourceBase::UNiagaraScriptSourceBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

/** For spawn scripts only, returns it's companion update script. */
UNiagaraScript* UNiagaraScript::GetCompanionUpdateScript()
{
	if (IsSpawnScript())
	{
		if (UNiagaraEmitterProperties* Emitter = GetTypedOuter<UNiagaraEmitterProperties>())
		{
			return Emitter->UpdateScriptProps.Script;
		}
	}
	return nullptr;
}

/** For update scripts only, returns it's companion spawn script. */
UNiagaraScript* UNiagaraScript::GetCompanionSpawnScript()
{
	if (IsUpdateScript())
	{
		if (UNiagaraEmitterProperties* Emitter = GetTypedOuter<UNiagaraEmitterProperties>())
		{
			return Emitter->SpawnScriptProps.Script;
		}
	}
	return nullptr;
}

void UNiagaraScript::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FNiagaraCustomVersion::GUID);
}

void UNiagaraScript::PostLoad()
{
	Super::PostLoad();
	
	bool bNeedsRecompile = false;
	const int32 NiagaraVer = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
	if (NiagaraVer < FNiagaraCustomVersion::VMExternalFunctionBindingReworkPartDeux)
	{
		bNeedsRecompile = true;
		ByteCode.Empty();
#if WITH_EDITORONLY_DATA
		LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Unknown;
#endif
		UE_LOG(LogNiagara, Warning, TEXT("Niagara script is out of date and requires recompile to be used! %s"), *GetFullName());
	}

	if (GIsEditor)
	{
		// We may have inadvertently saved invalid ID's in the past. In this case, that means that the 
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
#if WITH_EDITORONLY_DATA
			LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Unknown;
#endif
			FString PathName = GetOutermost()->GetPathName();

			// Remove the invalid entries
			for (int32 j = IndicesToRemove.Num() - 1; j >= 0; j--)
			{
				UE_LOG(LogNiagara, Warning, TEXT("Non-system parameter '%s' has an invalid GUID saved. Package '%s' will need to be recompiled and saved again."), 
					*Parameters.Parameters[IndicesToRemove[j]].GetName().ToString(), *PathName);
				Parameters.Parameters.RemoveAt(IndicesToRemove[j]);
				bNeedsRecompile = true;
			}
		}

#if WITH_EDITORONLY_DATA
		// Since we're about to check the synchronized state, we need to make sure that it has been post-loaded (which 
		// can affect the results of that call).
		if (Source != nullptr)
		{	
			Source->ConditionalPostLoad();
		}

		// If we've never computed a change id or the source graph is different than the compiled script change id,
		// we're out of sync and should recompile.
		if (ChangeId.IsValid() == false || (Source != nullptr && !Source->IsSynchronized(ChangeId)))
		{
			bNeedsRecompile = true;
			ByteCode.Empty();
			LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Unknown;
			UE_LOG(LogNiagara, Warning, TEXT("Niagara script is out of date with source graph requires recompile to be used! %s"), *GetFullName());
		}
#endif
	}


#if WITH_EDITORONLY_DATA
	if (bNeedsRecompile)
	{
		// If this is a standalone script, go ahead and compile it here. Otherwise, rely on the logic of post-load for the owner of that script.
		UObject* Outer = GetOuter();
		if (Outer != nullptr && Outer->IsA<UPackage>())
		{
			FString CompileErrors;
			ENiagaraScriptCompileStatus CompileStatus = Source->Compile(CompileErrors);
			if (ENiagaraScriptCompileStatus::NCS_UpToDate != CompileStatus || CompileErrors.Len() != 0)
			{
				UE_LOG(LogNiagara, Warning, TEXT("Script '%s', compile status: %d  errors: %s"), *GetPathName(), (int32)CompileStatus, *CompileErrors);
			}
			else
			{
				UE_LOG(LogNiagara, Log, TEXT("Script '%s', compile status: Success!"), *GetPathName());
			}
		}
	}
#endif
}


#if WITH_EDITORONLY_DATA
bool UNiagaraScript::AreScriptAndSourceSynchronized() const
{
	if (Source)
	{
		return Source->IsSynchronized(ChangeId);
	}
	else
	{
		return false;
	}
}

void UNiagaraScript::MarkScriptAndSourceDesynchronized()
{
	if (Source)
	{
		Source->MarkNotSynchronized();
	}

}


UNiagaraScript* UNiagaraScript::MakeRecursiveDeepCopy(UObject* DestOuter) const
{
	check(GetOuter() != DestOuter);
	EObjectFlags Flags = RF_AllFlags & ~RF_Standalone & ~RF_Public; // Remove Standalone and Public flags.
	UNiagaraScript*	Script = CastChecked<UNiagaraScript>(StaticDuplicateObject(this, DestOuter, NAME_None, Flags));
	check(Script->HasAnyFlags(RF_Standalone) == false);
	check(Script->HasAnyFlags(RF_Public) == false);

	TMap<UObject*, UObject*> ExistingConversions;
	Script->SubsumeExternalDependencies(ExistingConversions);
	return Script;
}

void UNiagaraScript::SubsumeExternalDependencies(TMap<UObject*, UObject*>& ExistingConversions)
{
	Source->SubsumeExternalDependencies(ExistingConversions);
}
#endif