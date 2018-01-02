// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterHandle.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterface.h"
#include "NiagaraModule.h"

#include "Modules/ModuleManager.h"

const FNiagaraEmitterHandle FNiagaraEmitterHandle::InvalidHandle;

FNiagaraEmitterHandle::FNiagaraEmitterHandle() :
#if WITH_EDITORONLY_DATA
	Source(nullptr),
	LastMergedSource(nullptr),
	bIsolated(false),
#endif
	Instance(nullptr)
{
}

FNiagaraEmitterHandle::FNiagaraEmitterHandle(UNiagaraEmitter& Emitter)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(true)
	, Name(TEXT("Emitter"))
#if WITH_EDITORONLY_DATA
	, Source(&Emitter)
	, LastMergedSource(&Emitter)
	, bIsolated(false)
#endif
	, Instance(&Emitter)
{
	
}

#if WITH_EDITORONLY_DATA
FNiagaraEmitterHandle::FNiagaraEmitterHandle(UNiagaraEmitter& InSourceEmitter, FName InName, UNiagaraSystem& InOuterSystem)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(true)
	, Name(InName)
	, Source(&InSourceEmitter)
	, LastMergedSource(Cast<UNiagaraEmitter>(StaticDuplicateObject(Source, &InOuterSystem)))
	, bIsolated(false)
	, Instance(Cast<UNiagaraEmitter>(StaticDuplicateObject(Source, &InOuterSystem)))
{
	Instance->SetUniqueEmitterName(Name.ToString());
}

FNiagaraEmitterHandle::FNiagaraEmitterHandle(const FNiagaraEmitterHandle& InHandleToDuplicate, FName InDuplicateName, UNiagaraSystem& InDuplicateOwnerSystem)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(InHandleToDuplicate.bIsEnabled)
	, Name(InDuplicateName)
	, Source(InHandleToDuplicate.Source)
	, LastMergedSource(InHandleToDuplicate.LastMergedSource != nullptr ? Cast<UNiagaraEmitter>(StaticDuplicateObject(InHandleToDuplicate.LastMergedSource, &InDuplicateOwnerSystem)) : nullptr)
	, bIsolated(false)
	, Instance(Cast<UNiagaraEmitter>(StaticDuplicateObject(InHandleToDuplicate.Instance, &InDuplicateOwnerSystem)))
{
	// Clear stand alone and public flags from the referenced emitters since they are not assets.
	Instance->ClearFlags(RF_Standalone | RF_Public);
	Instance->SetUniqueEmitterName(Name.ToString());
	LastMergedSource->ClearFlags(RF_Standalone | RF_Public);
}
#endif

bool FNiagaraEmitterHandle::IsValid() const
{
	return Id.IsValid();
}

FGuid FNiagaraEmitterHandle::GetId() const
{
	return Id;
}

FName FNiagaraEmitterHandle::GetIdName() const
{
	return IdName;
}

FName FNiagaraEmitterHandle::GetName() const
{
	return Name;
}

void FNiagaraEmitterHandle::SetName(FName InName, UNiagaraSystem& InOwnerSystem)
{
	if (InName == Name)
	{
		return;
	}

	FString TempNameString = InName.ToString();
	TempNameString.ReplaceInline(TEXT(" "), TEXT("_"));
	TempNameString.ReplaceInline(TEXT("\t"), TEXT("_"));
	InName = *TempNameString;

	TSet<FName> OtherEmitterNames;
	for (const FNiagaraEmitterHandle& OtherEmitterHandle : InOwnerSystem.GetEmitterHandles())
	{
		if (OtherEmitterHandle.GetId() != GetId())
		{
			OtherEmitterNames.Add(OtherEmitterHandle.GetName());
		}
	}
	FName UniqueName = FNiagaraUtilities::GetUniqueName(InName, OtherEmitterNames);

	Name = UniqueName;
	Instance->SetUniqueEmitterName(Name.ToString());
}

bool FNiagaraEmitterHandle::GetIsEnabled() const
{
	return bIsEnabled;
}

void FNiagaraEmitterHandle::SetIsEnabled(bool bInIsEnabled)
{
	bIsEnabled = bInIsEnabled;
}

#if WITH_EDITORONLY_DATA
const UNiagaraEmitter* FNiagaraEmitterHandle::GetSource() const
{
	return Source;
}

#endif

UNiagaraEmitter* FNiagaraEmitterHandle::GetInstance() const
{
	return Instance;
}

FString FNiagaraEmitterHandle::GetUniqueInstanceName()const
{
	check(Instance);
	return Instance->GetUniqueEmitterName();
}

#if WITH_EDITORONLY_DATA

bool FNiagaraEmitterHandle::IsSynchronizedWithSource() const
{
	if (Source == nullptr || LastMergedSource == nullptr)
	{
		// If either the source or last merged sources is missing, then we're not synchronized.  The
		// merge logic will detect this and print an appropriate message to the log.
		return false;
	}

	if (Source->GetChangeId().IsValid() == false ||
		LastMergedSource->GetChangeId().IsValid() == false)
	{
		// If any of the change Ids aren't valid then we assume we're out of sync.
		return false;
	}

	return Source->GetChangeId() == LastMergedSource->GetChangeId();
}

bool FNiagaraEmitterHandle::NeedsRecompile() const
{
	TArray<UNiagaraScript*> Scripts;
	Instance->GetScripts(Scripts);

	for (UNiagaraScript* Script : Scripts)
	{
		if (Script->IsCompilable() && !Script->AreScriptAndSourceSynchronized())
		{
			return true;
		}
	}
	return false;
}

void FNiagaraEmitterHandle::ConditionalPostLoad()
{
	if (Source != nullptr)
	{
		Source->ConditionalPostLoad();
	}
	if (LastMergedSource != nullptr)
	{
		LastMergedSource->ConditionalPostLoad();
	}
	Instance->ConditionalPostLoad();
}

INiagaraModule::FMergeEmitterResults FNiagaraEmitterHandle::MergeSourceChanges()
{
	if (Source == nullptr)
	{
		// If we don't have a copy of the source emitter, this emitter can't safely be merged.
		INiagaraModule::FMergeEmitterResults MergeResults;
		MergeResults.bSucceeded = false;
		MergeResults.bModifiedGraph = false;
		MergeResults.ErrorMessages.Add(NSLOCTEXT("NiagaraEmitterHandle", "NoSourceErrorMessage", "This emitter has no 'Source' so changes can't be merged in."));
		return MergeResults;
	}

	if (LastMergedSource == nullptr)
	{
		// If we don't have a copy of the last merged source emitter, this emitter can't safely be
		// merged.
		INiagaraModule::FMergeEmitterResults MergeResults;
		MergeResults.bSucceeded = false;
		MergeResults.bModifiedGraph = false;
		MergeResults.ErrorMessages.Add(NSLOCTEXT("NiagaraEmitterHandle", "NoLastMergedSourceErrorMessage", "This emitter has no 'LastMergedSource' so changes can't be merged in."));
		return MergeResults;
	}

	INiagaraModule& NiagaraModule = FModuleManager::Get().GetModuleChecked<INiagaraModule>("Niagara");
	INiagaraModule::FMergeEmitterResults MergeResults = NiagaraModule.MergeEmitter(*Source, *LastMergedSource, *Instance);
	if (MergeResults.bSucceeded)
	{
		UObject* Outer = Instance->GetOuter();
		Instance = MergeResults.MergedInstance;

		// Rename the merged instance into this package with the correct handle name and then clear it's stand alone and public flags since it's not a root asset.
		FName NewInstanceName = MakeUniqueObjectName(Outer, UNiagaraEmitter::StaticClass(), Name);
		Instance->Rename(*NewInstanceName.ToString(), Outer, REN_ForceNoResetLoaders);
		Instance->ClearFlags(RF_Standalone | RF_Public);
		Instance->SetUniqueEmitterName(Name.ToString());

		// Update the last merged source and clear it's stand alone and public flags since it's not an asset.
		LastMergedSource = CastChecked<UNiagaraEmitter>(StaticDuplicateObject(Source, Outer));
		LastMergedSource->ClearFlags(RF_Standalone | RF_Public);
	}
	return MergeResults;
}

#endif