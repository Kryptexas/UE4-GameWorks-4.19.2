// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#include "NiagaraSystem.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraRenderer.h"
#include "NiagaraConstants.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraCustomVersion.h"
#include "NiagaraModule.h"
#include "ModuleManager.h"
#include "NiagaraEmitter.h"
#include "Package.h"
#include "NiagaraEmitterHandle.h"
#include "AssetData.h"
#include "NiagaraStats.h"

DECLARE_CYCLE_STAT(TEXT("Niagara - System - CompileScript"), STAT_Niagara_System_CompileScript, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Niagara - System - CompileScript_ResetAfter"), STAT_Niagara_System_CompileScriptResetAfter, STATGROUP_Niagara);

// UNiagaraSystemCategory::UNiagaraSystemCategory(const FObjectInitializer& ObjectInitializer)
// 	: Super(ObjectInitializer)
// {
// }

//////////////////////////////////////////////////////////////////////////

UNiagaraSystem::UNiagaraSystem(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, ExposedParameters(this)
#if WITH_EDITORONLY_DATA
, bIsolateEnabled(false)
#endif
, bAutoDeactivate(true)
{
}

void UNiagaraSystem::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITORONLY_DATA
	ThumbnailImageOutOfDate = true;
#endif
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) == false)
	{
		SystemSpawnScript = NewObject<UNiagaraScript>(this, "SystemSpawnScript", RF_Transactional);
		SystemSpawnScript->SetUsage(ENiagaraScriptUsage::SystemSpawnScript);

		SystemUpdateScript = NewObject<UNiagaraScript>(this, "SystemUpdateScript", RF_Transactional);
		SystemUpdateScript->SetUsage(ENiagaraScriptUsage::SystemUpdateScript);

		SystemSpawnScriptSolo = NewObject<UNiagaraScript>(this, "SystemSpawnScriptSolo", RF_Transactional);
		SystemSpawnScriptSolo->SetUsage(ENiagaraScriptUsage::SystemSpawnScript);

		SystemUpdateScriptSolo = NewObject<UNiagaraScript>(this, "SystemUpdateScriptSolo", RF_Transactional);
		SystemUpdateScriptSolo->SetUsage(ENiagaraScriptUsage::SystemUpdateScript);
	}
}

bool UNiagaraSystem::IsLooping() const
{ 
	return false; 
} //sckime todo fix this!

bool UNiagaraSystem::UsesCollection(const UNiagaraParameterCollection* Collection)const
{
	if (SystemSpawnScript->UsesCollection(Collection) ||
		SystemSpawnScriptSolo->UsesCollection(Collection) ||
		SystemUpdateScript->UsesCollection(Collection) ||
		SystemUpdateScriptSolo->UsesCollection(Collection))
	{
		return true;
	}

	for (const FNiagaraEmitterHandle& EmitterHandle : GetEmitterHandles())
	{
		if (EmitterHandle.GetInstance()->UsesCollection(Collection))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITORONLY_DATA
bool UNiagaraSystem::UsesScript(const UNiagaraScript* Script)const
{
	if (SystemSpawnScript == Script ||
		SystemSpawnScriptSolo == Script ||
		SystemUpdateScript == Script ||
		SystemUpdateScriptSolo == Script)
	{
		return true;
	}

	for (FNiagaraEmitterHandle EmitterHandle : GetEmitterHandles())
	{
		if ((EmitterHandle.GetSource() && EmitterHandle.GetSource()->UsesScript(Script)) || (EmitterHandle.GetInstance() && EmitterHandle.GetInstance()->UsesScript(Script)))
		{
			return true;
		}
	}
	
	return false;
}
#endif

void UNiagaraSystem::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FNiagaraCustomVersion::GUID);
}

#if WITH_EDITOR
void UNiagaraSystem::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FNiagaraSystemUpdateContext(this, true);

	ThumbnailImageOutOfDate = true;
}
#endif 

void UNiagaraSystem::PostLoad()
{
	Super::PostLoad();

	if (GIsEditor)
	{
		SetFlags(RF_Transactional);
	}

	// Check to see if our version is out of date. If so, we'll finally need to recompile.
	bool bNeedsRecompile = false;
	const int32 NiagaraVer = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
	if (NiagaraVer < FNiagaraCustomVersion::LatestVersion)
	{
		bNeedsRecompile = true;
	}

	// Previously added emitters didn't have their stand alone and public flags cleared so
	// they 'leak' into the system package.  Clear the flags here so they can be collected
	// during the next save.
	UPackage* PackageOuter = Cast<UPackage>(GetOuter());
	if (PackageOuter != nullptr && HasAnyFlags(RF_Public | RF_Standalone))
	{
		TArray<UObject*> ObjectsInPackage;
		GetObjectsWithOuter((UObject*)PackageOuter, ObjectsInPackage);
		for (UObject* ObjectInPackage : ObjectsInPackage)
		{
			UNiagaraEmitter* Emitter = Cast<UNiagaraEmitter>(ObjectInPackage);
			if (Emitter != nullptr)
			{
				Emitter->ConditionalPostLoad();
				Emitter->ClearFlags(RF_Standalone | RF_Public);
			}
		}
	}
	
	// We will be using this later potentially, so make sure that it's postload is already up-to-date.
	if (SystemSpawnScript)
	{
		SystemSpawnScript->ConditionalPostLoad();
	}
	if (SystemUpdateScript)
	{
		SystemUpdateScript->ConditionalPostLoad();
	}

	if (NiagaraVer < FNiagaraCustomVersion::SystemEmitterScriptSimulations)
	{
		bNeedsRecompile = true;
		SystemSpawnScriptSolo = NewObject<UNiagaraScript>(this, "SystemSpawnScriptSolo", RF_Transactional);
		SystemSpawnScriptSolo->SetUsage(ENiagaraScriptUsage::SystemSpawnScript);

		SystemUpdateScriptSolo = NewObject<UNiagaraScript>(this, "SystemUpdateScriptSolo", RF_Transactional);
		SystemUpdateScriptSolo->SetUsage(ENiagaraScriptUsage::SystemUpdateScript);
#if WITH_EDITORONLY_DATA
		if (SystemSpawnScript)
		{
			SystemSpawnScriptSolo->SetSource(SystemSpawnScript->GetSource());
		}

		if (SystemUpdateScript)
		{
			SystemUpdateScriptSolo->SetSource(SystemUpdateScript->GetSource());
		}
#endif
	}

	if (bNeedsRecompile)
	{
		for (FNiagaraEmitterHandle& Handle : EmitterHandles)
		{
			UNiagaraEmitter* Props = Handle.GetInstance();
			if (Props)
			{
				// We will be refreshing later potentially, so make sure that it's postload is already up-to-date.
				Props->ConditionalPostLoad();
			}
		}
	}

#if WITH_EDITORONLY_DATA
	if (SystemSpawnScript == nullptr)
	{
		SystemSpawnScript = NewObject<UNiagaraScript>(this, "SystemSpawnScript", RF_Transactional);
		SystemSpawnScript->SetUsage(ENiagaraScriptUsage::SystemSpawnScript);
	}

	if (SystemUpdateScript == nullptr)
	{
		SystemUpdateScript = NewObject<UNiagaraScript>(this, "SystemUpdateScript", RF_Transactional);
		SystemUpdateScript->SetUsage(ENiagaraScriptUsage::SystemUpdateScript);
	}

	//TODO: This causes a crash becuase the script source ptr is null? Fix
	//For existing emitters before the lifecylce rework, ensure they have the system lifecycle module.
 	if (NiagaraVer < FNiagaraCustomVersion::LifeCycleRework)
 	{
 		/*UNiagaraScriptSourceBase* SystemScriptSource = SystemUpdateScript->GetSource();
 		if (SystemScriptSource)
 		{
			bool bFoundModule;
			if (SystemScriptSource->AddModuleIfMissing(TEXT("/Niagara/Modules/System/SystemLifeCycle.SystemLifeCycle"), ENiagaraScriptUsage::SystemUpdateScript, bFoundModule))
			{
				bNeedsRecompile = true;
			}
 		}*/
 	}

	for (FNiagaraEmitterHandle& EmitterHandle : EmitterHandles)
	{
		EmitterHandle.ConditionalPostLoad();
		if (EmitterHandle.IsSynchronizedWithSource() == false)
		{
			INiagaraModule::FMergeEmitterResults Results = MergeChangesForEmitterHandle(EmitterHandle);
			bNeedsRecompile |= Results.bSucceeded && Results.bModifiedGraph;
		}
	}

	if (bNeedsRecompile)
	{
		Compile(false);
	}
#endif
}

#if WITH_EDITORONLY_DATA

UObject* UNiagaraSystem::GetEditorData()
{
	return EditorData;
}

const UObject* UNiagaraSystem::GetEditorData() const
{
	return EditorData;
}

void UNiagaraSystem::SetEditorData(UObject* InEditorData)
{
	EditorData = InEditorData;
}

INiagaraModule::FMergeEmitterResults UNiagaraSystem::MergeChangesForEmitterHandle(FNiagaraEmitterHandle& EmitterHandle)
{
	INiagaraModule::FMergeEmitterResults Results = EmitterHandle.MergeSourceChanges();
	if (Results.bSucceeded)
	{
		UNiagaraEmitter* Instance = EmitterHandle.GetInstance();
		RefreshSystemParametersFromEmitter(EmitterHandle);
		if (Instance->bInterpolatedSpawning)
		{
			Instance->UpdateScriptProps.Script->RapidIterationParameters.CopyParametersTo(
				Instance->SpawnScriptProps.Script->RapidIterationParameters, false);
		}
	}
	else
	{
		UE_LOG(LogNiagara, Warning, TEXT("Failed to merge changes for base emitter.  System: %s  Emitter: %s  Error Message: %s"), 
			*GetPathName(), *EmitterHandle.GetName().ToString(), *Results.GetErrorMessagesString());
	}
	return Results;
}

bool UNiagaraSystem::ReferencesSourceEmitter(UNiagaraEmitter& Emitter)
{
	for (FNiagaraEmitterHandle& Handle : EmitterHandles)
	{
		if (&Emitter == Handle.GetSource())
		{
			return true;
		}
	}
	return false;
}

void UNiagaraSystem::UpdateFromEmitterChanges(UNiagaraEmitter& ChangedSourceEmitter)
{
	bool bNeedsCompile = false;
	for(FNiagaraEmitterHandle& EmitterHandle : EmitterHandles)
	{
		if (EmitterHandle.GetSource() == &ChangedSourceEmitter)
		{
			INiagaraModule::FMergeEmitterResults Results = MergeChangesForEmitterHandle(EmitterHandle);
			bNeedsCompile |= Results.bSucceeded && Results.bModifiedGraph;
		}
	}

	if (bNeedsCompile)
	{
		Compile(false);
	}
}

void UNiagaraSystem::RefreshSystemParametersFromEmitter(const FNiagaraEmitterHandle& EmitterHandle)
{
	if (ensureMsgf(EmitterHandles.ContainsByPredicate([=](const FNiagaraEmitterHandle& OwnedEmitterHandle) { return OwnedEmitterHandle.GetId() == EmitterHandle.GetId(); }),
		TEXT("Can't refresh parameters from an emitter handle this system doesn't own.")))
	{
		EmitterHandle.GetInstance()->EmitterSpawnScriptProps.Script->RapidIterationParameters.CopyParametersTo(SystemSpawnScript->RapidIterationParameters, false);
		EmitterHandle.GetInstance()->EmitterSpawnScriptProps.Script->RapidIterationParameters.CopyParametersTo(SystemSpawnScriptSolo->RapidIterationParameters, false);
		EmitterHandle.GetInstance()->EmitterUpdateScriptProps.Script->RapidIterationParameters.CopyParametersTo(SystemUpdateScript->RapidIterationParameters, false);
		EmitterHandle.GetInstance()->EmitterUpdateScriptProps.Script->RapidIterationParameters.CopyParametersTo(SystemUpdateScriptSolo->RapidIterationParameters, false);
	}
}

void UNiagaraSystem::RemoveSystemParametersForEmitter(const FNiagaraEmitterHandle& EmitterHandle)
{
	if (ensureMsgf(EmitterHandles.ContainsByPredicate([=](const FNiagaraEmitterHandle& OwnedEmitterHandle) { return OwnedEmitterHandle.GetId() == EmitterHandle.GetId(); }),
		TEXT("Can't remove parameters for an emitter handle this system doesn't own.")))
	{
		EmitterHandle.GetInstance()->EmitterSpawnScriptProps.Script->RapidIterationParameters.RemoveParameters(SystemSpawnScript->RapidIterationParameters);
		EmitterHandle.GetInstance()->EmitterSpawnScriptProps.Script->RapidIterationParameters.RemoveParameters(SystemSpawnScriptSolo->RapidIterationParameters);
		EmitterHandle.GetInstance()->EmitterUpdateScriptProps.Script->RapidIterationParameters.RemoveParameters(SystemUpdateScript->RapidIterationParameters);
		EmitterHandle.GetInstance()->EmitterUpdateScriptProps.Script->RapidIterationParameters.RemoveParameters(SystemUpdateScriptSolo->RapidIterationParameters);
	}
}
#endif


const TArray<FNiagaraEmitterHandle>& UNiagaraSystem::GetEmitterHandles()
{
	return EmitterHandles;
}

const TArray<FNiagaraEmitterHandle>& UNiagaraSystem::GetEmitterHandles()const
{
	return EmitterHandles;
}

bool UNiagaraSystem::IsReadyToRun() const
{
	if (!SystemSpawnScript || !SystemSpawnScriptSolo || !SystemUpdateScript || !SystemUpdateScriptSolo)
	{
		return false;
	}

	if (SystemSpawnScript->IsScriptCompilationPending(false) || SystemSpawnScriptSolo->IsScriptCompilationPending(false) || 
		SystemUpdateScript->IsScriptCompilationPending(false) || SystemUpdateScriptSolo->IsScriptCompilationPending(false))
	{
		return false;
	}

	for (const FNiagaraEmitterHandle& Handle : EmitterHandles)
	{
		if (!Handle.GetInstance()->IsReadyToRun())
		{
			return false;
		}
	}
	return true;
}

bool UNiagaraSystem::IsValid()const
{
	if (!SystemSpawnScript || !SystemSpawnScriptSolo || !SystemUpdateScript || !SystemUpdateScriptSolo)
	{
		return false;
	}

	if ((!SystemSpawnScript->IsScriptCompilationPending(false) && !SystemSpawnScript->DidScriptCompilationSucceed(false)) ||
		(!SystemSpawnScriptSolo->IsScriptCompilationPending(false) && !SystemSpawnScriptSolo->DidScriptCompilationSucceed(false)) ||
		(!SystemUpdateScript->IsScriptCompilationPending(false) && !SystemUpdateScript->DidScriptCompilationSucceed(false)) ||
		(!SystemUpdateScriptSolo->IsScriptCompilationPending(false) && !SystemUpdateScriptSolo->DidScriptCompilationSucceed(false)))
	{
		return false;
	}


	for (const FNiagaraEmitterHandle& Handle : EmitterHandles)
	{
		if (!Handle.GetInstance()->IsValid())
		{
			return false;
		}
	}

	return true;
}
#if WITH_EDITORONLY_DATA

FNiagaraEmitterHandle UNiagaraSystem::AddEmitterHandle(UNiagaraEmitter& SourceEmitter, FName EmitterName)
{
	FNiagaraEmitterHandle EmitterHandle(SourceEmitter, EmitterName, *this);
	EmitterHandles.Add(EmitterHandle);
	RefreshSystemParametersFromEmitter(EmitterHandle);
	return EmitterHandle;
}

FNiagaraEmitterHandle UNiagaraSystem::AddEmitterHandleWithoutCopying(UNiagaraEmitter& Emitter)
{
	FNiagaraEmitterHandle EmitterHandle(Emitter);
	EmitterHandles.Add(EmitterHandle);
	RefreshSystemParametersFromEmitter(EmitterHandle);
	return EmitterHandle;
}

FNiagaraEmitterHandle UNiagaraSystem::DuplicateEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDuplicate, FName EmitterName)
{
	FNiagaraEmitterHandle EmitterHandle(EmitterHandleToDuplicate, EmitterName, *this);
	EmitterHandles.Add(EmitterHandle);
	RefreshSystemParametersFromEmitter(EmitterHandle);
	return EmitterHandle;
}

void UNiagaraSystem::RemoveEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDelete)
{
	UNiagaraEmitter* EditableEmitter = EmitterHandleToDelete.GetInstance();
	RemoveSystemParametersForEmitter(EmitterHandleToDelete);
	auto RemovePredicate = [&](const FNiagaraEmitterHandle& EmitterHandle) { return EmitterHandle.GetId() == EmitterHandleToDelete.GetId(); };
	EmitterHandles.RemoveAll(RemovePredicate);
}

void UNiagaraSystem::RemoveEmitterHandlesById(const TSet<FGuid>& HandlesToRemove)
{
	auto RemovePredicate = [&](const FNiagaraEmitterHandle& EmitterHandle)
	{
		return HandlesToRemove.Contains(EmitterHandle.GetId());
	};
	EmitterHandles.RemoveAll(RemovePredicate);
}
#endif


UNiagaraScript* UNiagaraSystem::GetSystemSpawnScript(bool bSolo)
{
	return bSolo ? SystemSpawnScriptSolo : SystemSpawnScript;
}

UNiagaraScript* UNiagaraSystem::GetSystemUpdateScript(bool bSolo)
{
	return bSolo ? SystemUpdateScriptSolo : SystemUpdateScript;
}

#if WITH_EDITORONLY_DATA

bool UNiagaraSystem::GetIsolateEnabled() const
{
	return bIsolateEnabled;
}

void UNiagaraSystem::SetIsolateEnabled(bool bIsolate)
{
	bIsolateEnabled = bIsolate;
}


void UNiagaraSystem::CompileScripts(TArray<ENiagaraScriptCompileStatus>& OutScriptStatuses, TArray<FString>& OutGraphLevelErrorMessages, TArray<FString>& PathNames, TArray<UNiagaraScript*>& Scripts, bool bForce)
{
	float StartTime = FPlatformTime::Seconds();

	SCOPE_CYCLE_COUNTER(STAT_Niagara_System_CompileScript);
	PathNames.Reset();
	OutScriptStatuses.Reset();
	OutGraphLevelErrorMessages.Reset();

	TArray<FNiagaraVariable> ExposedVars;

	//Compile all emitters
	bool bAnyUnsynchronized = false;
	for (FNiagaraEmitterHandle Handle : EmitterHandles)
	{
		TArray<ENiagaraScriptCompileStatus> ScriptStatuses;
		TArray<FString> ScriptErrors;

		UNiagaraScriptSourceBase* GraphSource = Handle.GetInstance()->GraphSource;
		check(!GraphSource->IsPreCompiled());
		GraphSource->PreCompile(Handle.GetInstance());

		if (!Handle.GetInstance()->AreAllScriptAndSourcesSynchronized())
		{
			bAnyUnsynchronized = true;
		}
		Handle.GetInstance()->CompileScripts(ScriptStatuses, ScriptErrors, PathNames, Scripts, bForce);

		GraphSource->GatherPreCompiledVariables(TEXT("User"), ExposedVars);
		GraphSource->PostCompile();

		OutScriptStatuses.Append(ScriptStatuses);
		OutGraphLevelErrorMessages.Append(ScriptErrors);
	}

	bool bForceSystems = bForce || bAnyUnsynchronized;

	check(SystemSpawnScript->GetSource() == SystemUpdateScript->GetSource());
	check(SystemSpawnScript->GetSource()->IsPreCompiled() == false);

	SystemSpawnScript->GetSource()->PreCompile(nullptr);
	SystemSpawnScript->GetSource()->GatherPreCompiledVariables(TEXT("User"), ExposedVars);

	FString ErrorMsg;
	check((int32)EScriptCompileIndices::SpawnScript == 0);
	OutScriptStatuses.Add(SystemSpawnScript->Compile(ErrorMsg, bForceSystems));
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::SpawnScript
	Scripts.Add(SystemSpawnScript);
	PathNames.Add(SystemSpawnScript->GetPathName());

	ErrorMsg.Empty();
	check((int32)EScriptCompileIndices::UpdateScript == 1);
	OutScriptStatuses.Add(SystemUpdateScript->Compile(ErrorMsg, bForceSystems));
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::SpawnScript
	Scripts.Add(SystemUpdateScript);
	PathNames.Add(SystemUpdateScript->GetPathName());

	ErrorMsg.Empty();
	check((int32)EScriptCompileIndices::SpawnScript == 0);
	OutScriptStatuses.Add(SystemSpawnScriptSolo->Compile(ErrorMsg, bForceSystems));
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::SpawnScript
	Scripts.Add(SystemSpawnScriptSolo);
	PathNames.Add(SystemSpawnScriptSolo->GetPathName());

	ErrorMsg.Empty();
	check((int32)EScriptCompileIndices::UpdateScript == 1);
	OutScriptStatuses.Add(SystemUpdateScriptSolo->Compile(ErrorMsg, bForceSystems));
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::SpawnScript
	Scripts.Add(SystemUpdateScriptSolo);
	PathNames.Add(SystemUpdateScriptSolo->GetPathName());

	SystemSpawnScript->GetSource()->PostCompile();

	InitEmitterSpawnAttributes();
	
	// Now let's synchronize the variables that we expose to the end user.
	TArray<FNiagaraVariable> OriginalVars;
	ExposedParameters.GetParameters(OriginalVars);
	for (int32 i = 0; i < ExposedVars.Num(); i++)
	{
		if (OriginalVars.Contains(ExposedVars[i]) == false)
		{
			// Just in case it wasn't added previously..
			ExposedParameters.AddParameter(ExposedVars[i]);
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_Niagara_System_CompileScriptResetAfter);

		FNiagaraSystemUpdateContext UpdateCtx(this, true);
	}
	//I assume this was added to recompile emitters too but I'm doing that above. We need to rework this compilation flow.
	//Compile();


	UE_LOG(LogNiagara, Log, TEXT("Compiling System %s took %f sec."), *GetFullName(), FPlatformTime::Seconds() - StartTime);
}

void UNiagaraSystem::Compile(bool bForce)
{
	float StartTime = FPlatformTime::Seconds();

	bool bAnyUnsynchronized = false;
	TArray<FNiagaraVariable> ExposedVars;
	for (FNiagaraEmitterHandle Handle: EmitterHandles)
	{
		TArray<ENiagaraScriptCompileStatus> ScriptStatuses;
		TArray<FString> ScriptErrors;
		TArray<FString> PathNames;
		TArray<UNiagaraScript*> Scripts;
		
		UNiagaraScriptSourceBase* GraphSource = Handle.GetInstance()->GraphSource;
		check(!GraphSource->IsPreCompiled());
		GraphSource->PreCompile(Handle.GetInstance());

		if (!Handle.GetInstance()->AreAllScriptAndSourcesSynchronized())
		{
			bAnyUnsynchronized = true;
		}
		Handle.GetInstance()->CompileScripts(ScriptStatuses, ScriptErrors, PathNames, Scripts, bForce);

		GraphSource->GatherPreCompiledVariables(TEXT("User"), ExposedVars);

		GraphSource->PostCompile();

		for (int32 i = 0; i < ScriptStatuses.Num(); i++)
		{
			if (ScriptErrors[i].Len() != 0 || ScriptStatuses[i] != ENiagaraScriptCompileStatus::NCS_UpToDate)
			{
				UE_LOG(LogNiagara, Warning, TEXT("Script '%s', compile status: %d  errors: %s"), *PathNames[i], (int32)ScriptStatuses[i], *ScriptErrors[i]);
			}
			else
			{
				UE_LOG(LogNiagara, Log, TEXT("Script '%s', compile status: Success!"), *PathNames[i]);
			}
		}
	}
	bool bForceSystems = bForce || bAnyUnsynchronized;

	check(SystemSpawnScript->GetSource() == SystemUpdateScript->GetSource());
	check(SystemSpawnScript->GetSource()->IsPreCompiled() == false);

	SystemSpawnScript->GetSource()->PreCompile(nullptr);
	SystemSpawnScript->GetSource()->GatherPreCompiledVariables(TEXT("User"), ExposedVars);

	FString CompileErrors;
	ENiagaraScriptCompileStatus CompileStatus = SystemSpawnScript->Compile(CompileErrors, bForceSystems);
	if (CompileErrors.Len() != 0 || CompileStatus != ENiagaraScriptCompileStatus::NCS_UpToDate)
	{
		UE_LOG(LogNiagara, Warning, TEXT("System Spawn Script '%s', compile status: %d  errors: %s"), *SystemSpawnScript->GetPathName(), (int32)CompileStatus, *CompileErrors);
	}
	else
	{
		UE_LOG(LogNiagara, Log, TEXT("Script Spawn '%s', compile status: Success!"), *SystemSpawnScript->GetPathName());
	}

	CompileStatus = SystemUpdateScript->Compile(CompileErrors, bForceSystems);
	if (CompileErrors.Len() != 0 || CompileStatus != ENiagaraScriptCompileStatus::NCS_UpToDate)
	{
		UE_LOG(LogNiagara, Warning, TEXT("System Update Script '%s', compile status: %d  errors: %s"), *SystemUpdateScript->GetPathName(), (int32)CompileStatus, *CompileErrors);
	}
	else
	{
		UE_LOG(LogNiagara, Log, TEXT("Script Update '%s', compile status: Success!"), *SystemUpdateScript->GetPathName());
	}

	if(SystemSpawnScriptSolo)
	{
		CompileStatus = SystemSpawnScriptSolo->Compile(CompileErrors, bForceSystems);
		if (CompileErrors.Len() != 0 || CompileStatus != ENiagaraScriptCompileStatus::NCS_UpToDate)
		{
			UE_LOG(LogNiagara, Warning, TEXT("System Spawn Solo Script '%s', compile status: %d  errors: %s"), *SystemSpawnScriptSolo->GetPathName(), (int32)CompileStatus, *CompileErrors);
		}
		else
		{
			UE_LOG(LogNiagara, Log, TEXT("Script Spawn Solo '%s', compile status: Success!"), *SystemSpawnScriptSolo->GetPathName());
		}
	}

	if (SystemUpdateScriptSolo)
	{
		CompileStatus = SystemUpdateScriptSolo->Compile(CompileErrors, bForceSystems);
		if (CompileErrors.Len() != 0 || CompileStatus != ENiagaraScriptCompileStatus::NCS_UpToDate)
		{
			UE_LOG(LogNiagara, Warning, TEXT("System Update Solo Script '%s', compile status: %d  errors: %s"), *SystemUpdateScriptSolo->GetPathName(), (int32)CompileStatus, *CompileErrors);
		}
		else
		{
			UE_LOG(LogNiagara, Log, TEXT("Script Update Solo '%s', compile status: Success!"), *SystemUpdateScriptSolo->GetPathName());
		}
	}
	SystemSpawnScript->GetSource()->PostCompile();

	InitEmitterSpawnAttributes();
	TArray<FNiagaraVariable> OriginalVars;
	ExposedParameters.GetParameters(OriginalVars);
	for (int32 i = 0; i < ExposedVars.Num(); i++)
	{
		ExposedParameters.AddParameter(ExposedVars[i]);
	}
	for (int32 i = 0; i < OriginalVars.Num(); i++)
	{
		if (ExposedVars.Contains(OriginalVars[i]) == false)
		{
			ExposedParameters.RemoveParameter(OriginalVars[i]);
		}
	}

	{

		SCOPE_CYCLE_COUNTER(STAT_Niagara_System_CompileScriptResetAfter);
		FNiagaraSystemUpdateContext UpdateCtx(this, true);
	}

	UE_LOG(LogNiagara, Log, TEXT("Compiling System %s took %f sec."), *GetFullName(), FPlatformTime::Seconds() - StartTime);

}
#endif

void UNiagaraSystem::InitEmitterSpawnAttributes()
{
	EmitterSpawnAttributes.Empty();
	EmitterSpawnAttributes.SetNum(EmitterHandles.Num());
	FNiagaraTypeDefinition SpawnInfoDef = FNiagaraTypeDefinition(FNiagaraSpawnInfo::StaticStruct());
	for (FNiagaraVariable& Var : SystemSpawnScript->Attributes)
	{
		for (int32 EmitterIdx = 0; EmitterIdx < EmitterHandles.Num(); ++EmitterIdx)
		{
			UNiagaraEmitter* Emitter = EmitterHandles[EmitterIdx].GetInstance();
			FString EmitterName = Emitter->GetUniqueEmitterName() + TEXT(".");
			if (Var.GetType() == SpawnInfoDef && Var.GetName().ToString().StartsWith(EmitterName))
			{
				EmitterSpawnAttributes[EmitterIdx].SpawnAttributes.AddUnique(Var.GetName());
			}
		}
	}
	for (FNiagaraVariable& Var : SystemUpdateScript->Attributes)
	{
		for (int32 EmitterIdx = 0; EmitterIdx < EmitterHandles.Num(); ++EmitterIdx)
		{
			UNiagaraEmitter* Emitter = EmitterHandles[EmitterIdx].GetInstance();
			FString EmitterName = Emitter->GetUniqueEmitterName() + TEXT(".");
			if (Var.GetType() == SpawnInfoDef && Var.GetName().ToString().StartsWith(EmitterName))
			{
				EmitterSpawnAttributes[EmitterIdx].SpawnAttributes.AddUnique(Var.GetName());
			}
		}
	}
}
