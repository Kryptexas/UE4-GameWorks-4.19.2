// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraCustomVersion.h"
#include "Package.h"
#include "Linker.h"
#include "NiagaraModule.h"

static int32 GbForceNiagaraCompileOnLoad = 0;
static FAutoConsoleVariableRef CVarForceNiagaraCompileOnLoad(
	TEXT("fx.ForceCompileOnLoad"),
	GbForceNiagaraCompileOnLoad,
	TEXT("If > 0 emitters will be forced to compile on load. \n"),
	ECVF_Default
	);

void FNiagaraEmitterScriptProperties::InitDataSetAccess()
{
	EventReceivers.Empty();
	EventGenerators.Empty();

	if (Script)
	{
		//UE_LOG(LogNiagara, Log, TEXT("InitDataSetAccess: %s %d %d"), *Script->GetPathName(), Script->ReadDataSets.Num(), Script->WriteDataSets.Num());
		// TODO: add event receiver and generator lists to the script properties here
		//
		for (FNiagaraDataSetID &ReadID : Script->ReadDataSets)
		{
			EventReceivers.Add( FNiagaraEventReceiverProperties(ReadID.Name, "", "") );
		}

		for (FNiagaraDataSetProperties &WriteID : Script->WriteDataSets)
		{
			FNiagaraEventGeneratorProperties Props(WriteID, "", "");
			EventGenerators.Add(Props);
		}
	}
}

bool FNiagaraEmitterScriptProperties::DataSetAccessSynchronized() const
{
	if (Script)
	{
		if (Script->ReadDataSets.Num() != EventReceivers.Num())
		{
			return false;
		}
		if (Script->WriteDataSets.Num() != EventGenerators.Num())
		{
			return false;
		}
		return true;
	}
	else
	{
		return EventReceivers.Num() == 0 && EventGenerators.Num() == 0;
	}
}

//////////////////////////////////////////////////////////////////////////

UNiagaraEmitter::UNiagaraEmitter(const FObjectInitializer& Initializer)
: Super(Initializer)
, CollisionMode(ENiagaraCollisionMode::None)
, FixedBounds(FBox(FVector(-100), FVector(100)))
, MinDetailLevel(0)
, MaxDetailLevel(4)
, bInterpolatedSpawning(false)
, bFixedBounds(false)
, bUseMinDetailLevel(false)
, bUseMaxDetailLevel(false)
#if WITH_EDITORONLY_DATA
, ThumbnailImageOutOfDate(true)
#endif
{
}

void UNiagaraEmitter::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) == false)
	{
		RendererProperties.Add(NewObject<UNiagaraSpriteRendererProperties>(this, "Renderer"));

		SpawnScriptProps.Script = NewObject<UNiagaraScript>(this, "SpawnScript", EObjectFlags::RF_Transactional);
		SpawnScriptProps.Script->SetUsage(ENiagaraScriptUsage::ParticleSpawnScript);

		UpdateScriptProps.Script = NewObject<UNiagaraScript>(this, "UpdateScript", EObjectFlags::RF_Transactional);
		UpdateScriptProps.Script->SetUsage(ENiagaraScriptUsage::ParticleUpdateScript);

		EmitterSpawnScriptProps.Script = NewObject<UNiagaraScript>(this, "EmitterSpawnScript", EObjectFlags::RF_Transactional);
		EmitterSpawnScriptProps.Script->SetUsage(ENiagaraScriptUsage::EmitterSpawnScript);
		
		EmitterUpdateScriptProps.Script = NewObject<UNiagaraScript>(this, "EmitterUpdateScript", EObjectFlags::RF_Transactional);
		EmitterUpdateScriptProps.Script->SetUsage(ENiagaraScriptUsage::EmitterUpdateScript);

	}
	UniqueEmitterName = TEXT("Emitter");
}

void UNiagaraEmitter::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FNiagaraCustomVersion::GUID);
}

void UNiagaraEmitter::PostLoad()
{
	Super::PostLoad();

	if (GIsEditor)
	{
		SetFlags(RF_Transactional);
	}

// 	//Have to force the usage to be correct.
// 	if (SpawnScriptProps.Script)
// 	{
// 		SpawnScriptProps.Script->ConditionalPostLoad();
// 		ENiagaraScriptUsage ActualUsage = bInterpolatedSpawning ? ENiagaraScriptUsage::ParticleSpawnScriptInterpolated : ENiagaraScriptUsage::ParticleSpawnScript;
// 		if (SpawnScriptProps.Script->Usage != ActualUsage)
// 		{
// 			SpawnScriptProps.Script->Usage = ActualUsage;
// 			SpawnScriptProps.Script->ByteCode.Empty();//Force them to recompile.
// 			UE_LOG(LogNiagara, Warning, TEXT("Niagara Emitter had mis matched bInterpolatedSpawning values. You must recompile this emitter. %s"), *GetFullName());
// 		}
// 	}

	if (EmitterSpawnScriptProps.Script == nullptr || EmitterUpdateScriptProps.Script == nullptr)
	{
		EmitterSpawnScriptProps.Script = NewObject<UNiagaraScript>(this, "EmitterSpawnScript", EObjectFlags::RF_Transactional);
		EmitterSpawnScriptProps.Script->SetUsage(ENiagaraScriptUsage::EmitterSpawnScript);

		EmitterUpdateScriptProps.Script = NewObject<UNiagaraScript>(this, "EmitterUpdateScript", EObjectFlags::RF_Transactional);
		EmitterUpdateScriptProps.Script->SetUsage(ENiagaraScriptUsage::EmitterUpdateScript);

#if WITH_EDITORONLY_DATA
		if (SpawnScriptProps.Script)
		{
			EmitterSpawnScriptProps.Script->SetSource(SpawnScriptProps.Script->GetSource());
			EmitterUpdateScriptProps.Script->SetSource(SpawnScriptProps.Script->GetSource());
		}
#endif
	}
		
	//Temporarily disabling interpolated spawn if the script type and flag don't match.
	if (SpawnScriptProps.Script)
	{
		SpawnScriptProps.Script->ConditionalPostLoad();
		bool bActualInterpolatedSpawning = SpawnScriptProps.Script->IsInterpolatedParticleSpawnScript();
		if (bInterpolatedSpawning != bActualInterpolatedSpawning)
		{
			bInterpolatedSpawning = false;
			if (bActualInterpolatedSpawning)
			{
				SpawnScriptProps.Script->InvalidateScript();//clear out the script as it was compiled with interpolated spawn.

				SpawnScriptProps.Script->SetUsage(ENiagaraScriptUsage::ParticleSpawnScript);
			}
			UE_LOG(LogNiagara, Warning, TEXT("Disabling interpolated spawn because emitter flag and script type don't match. Did you adjust this value in the UI? Emitter may need recompile.. %s"), *GetFullName());
		}
	}

	// Referenced scripts may need to be invalidated due to version mismatch or 
	// other issues. This is determined in PostLoad for UNiagaraScript, so we need
	// to make sure that PostLoad has happened for each of them first.
	bool bNeedsRecompile = false;
	if (SpawnScriptProps.Script)
	{
		SpawnScriptProps.Script->ConditionalPostLoad();
		if (SpawnScriptProps.Script->GetByteCode().Num() == 0 || GbForceNiagaraCompileOnLoad > 0)
		{
			bNeedsRecompile = true;
		}
	}
	if (UpdateScriptProps.Script)
	{
		UpdateScriptProps.Script->ConditionalPostLoad();
		if (UpdateScriptProps.Script->GetByteCode().Num() == 0 || GbForceNiagaraCompileOnLoad > 0)
		{
			bNeedsRecompile = true;
		}
	}

	for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
	{
		if (EventHandlerScriptProps[i].Script)
		{
			EventHandlerScriptProps[i].Script->ConditionalPostLoad();
			if (EventHandlerScriptProps[i].Script->GetByteCode().Num() == 0 || GbForceNiagaraCompileOnLoad > 0)
			{
				bNeedsRecompile = true;
			}
		}
	}

#if WITH_EDITORONLY_DATA
	GraphSource->ConditionalPostLoad();
	bNeedsRecompile |= GraphSource->PostLoadFromEmitter(*this);
#endif

	// Check ourselves against the current script rebuild version. Force our children
	// to need to update.
	const int32 NiagaraVer = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
	if (NiagaraVer < FNiagaraCustomVersion::LatestVersion)
	{
		bNeedsRecompile = true;
	}
	
	if (bNeedsRecompile)
	{
		if (SpawnScriptProps.Script)
		{
			SpawnScriptProps.Script->InvalidateScript();//clear out the script.
		}
		if (UpdateScriptProps.Script)
		{
			UpdateScriptProps.Script->InvalidateScript();//clear out the script.
		}

		for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
		{
			if (EventHandlerScriptProps[i].Script)
			{
				EventHandlerScriptProps[i].Script->InvalidateScript();//clear out the script.
			}
		}
	}

#if WITH_EDITORONLY_DATA
	// Go ahead and do a recompile for all referenced scripts. Note that the compile will 
	// cause the emitter's change id to be regenerated.
	if (bNeedsRecompile)
	{
		UObject* Outer = GetOuter();
		if (Outer != nullptr && Outer->IsA<UPackage>())
		{
			TArray<ENiagaraScriptCompileStatus> ScriptStatuses;
			TArray<FString> ScriptErrors;
			TArray<FString> PathNames;
			TArray<UNiagaraScript*> Scripts;
			CompileScripts(ScriptStatuses, ScriptErrors, PathNames, Scripts, false);

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
	}

	GraphSource->OnChanged().AddUObject(this, &UNiagaraEmitter::GraphSourceChanged);

	EmitterSpawnScriptProps.Script->RapidIterationParameters.AddOnChangedHandler(
		FNiagaraParameterStore::FOnChanged::FDelegate::CreateUObject(this, &UNiagaraEmitter::ScriptRapidIterationParameterChanged));
	EmitterUpdateScriptProps.Script->RapidIterationParameters.AddOnChangedHandler(
		FNiagaraParameterStore::FOnChanged::FDelegate::CreateUObject(this, &UNiagaraEmitter::ScriptRapidIterationParameterChanged));

	if (SpawnScriptProps.Script)
	{
		SpawnScriptProps.Script->RapidIterationParameters.AddOnChangedHandler(
			FNiagaraParameterStore::FOnChanged::FDelegate::CreateUObject(this, &UNiagaraEmitter::ScriptRapidIterationParameterChanged));
	}
	
	if (UpdateScriptProps.Script)
	{
		UpdateScriptProps.Script->RapidIterationParameters.AddOnChangedHandler(
			FNiagaraParameterStore::FOnChanged::FDelegate::CreateUObject(this, &UNiagaraEmitter::ScriptRapidIterationParameterChanged));
	}

	for (FNiagaraEventScriptProperties& EventScriptProperties : EventHandlerScriptProps)
	{
		EventScriptProperties.Script->RapidIterationParameters.AddOnChangedHandler(
			FNiagaraParameterStore::FOnChanged::FDelegate::CreateUObject(this, &UNiagaraEmitter::ScriptRapidIterationParameterChanged));
	}

	for (UNiagaraRendererProperties* Renderer : RendererProperties)
	{
		Renderer->OnChanged().AddUObject(this, &UNiagaraEmitter::RendererChanged);
	}
#endif
}

#if WITH_EDITOR
void UNiagaraEmitter::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName;
	if (PropertyChangedEvent.Property)
	{
		PropertyName = PropertyChangedEvent.Property->GetFName();
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraEmitter, bInterpolatedSpawning))
	{
		bool bActualInterpolatedSpawning = SpawnScriptProps.Script->IsInterpolatedParticleSpawnScript();
		if (bInterpolatedSpawning != bActualInterpolatedSpawning)
		{
			//Recompile spawn script if we've altered the interpolated spawn property.
			SpawnScriptProps.Script->SetUsage(bInterpolatedSpawning ? ENiagaraScriptUsage::ParticleSpawnScriptInterpolated : ENiagaraScriptUsage::ParticleSpawnScript);
			UE_LOG(LogNiagara, Log, TEXT("Updating script usage: Script->IsInterpolatdSpawn %d Emitter->bInterpolatedSpawning %d"), (int32)SpawnScriptProps.Script->IsInterpolatedParticleSpawnScript(), bInterpolatedSpawning);
			if (GraphSource != nullptr)
			{
				GraphSource->MarkNotSynchronized();
			}
		}
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UNiagaraEmitter, SimTarget))
	{
		if (GraphSource != nullptr)
		{
			GraphSource->MarkNotSynchronized();
		}
	}
	ThumbnailImageOutOfDate = true;
	ChangeId = FGuid::NewGuid();
	OnPropertiesChangedDelegate.Broadcast();
}


UNiagaraEmitter::FOnPropertiesChanged& UNiagaraEmitter::OnPropertiesChanged()
{
	return OnPropertiesChangedDelegate;
}
#endif


bool UNiagaraEmitter::IsValid()const
{
	if (!SpawnScriptProps.Script || !UpdateScriptProps.Script)
	{
		return false;
	}

	if (SimTarget == ENiagaraSimTarget::CPUSim || SimTarget == ENiagaraSimTarget::DynamicLoadBalancedSim)
	{
		if (!SpawnScriptProps.Script->IsScriptCompilationPending(false) && !SpawnScriptProps.Script->DidScriptCompilationSucceed(false))
		{
			return false;
		}
		if (!UpdateScriptProps.Script->IsScriptCompilationPending(false) && !UpdateScriptProps.Script->DidScriptCompilationSucceed(false))
		{
			return false;
		}
		if (EventHandlerScriptProps.Num() != 0)
		{
			for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
			{
				if (!EventHandlerScriptProps[i].Script->IsScriptCompilationPending(false) &&
					!EventHandlerScriptProps[i].Script->DidScriptCompilationSucceed(false))
				{
					return false;
				}
			}
		}
	}

	if (SimTarget == ENiagaraSimTarget::GPUComputeSim || SimTarget == ENiagaraSimTarget::DynamicLoadBalancedSim)
	{
		if (!SpawnScriptProps.Script->IsScriptCompilationPending(true) && 
			!SpawnScriptProps.Script->DidScriptCompilationSucceed(true))
		{
			return false;
		}
	}
	return true;
}

bool UNiagaraEmitter::IsReadyToRun() const
{
	//Check for various failure conditions and bail.
	if (!UpdateScriptProps.Script || !SpawnScriptProps.Script)
	{
		return false;
	}

	if (SimTarget == ENiagaraSimTarget::CPUSim || SimTarget == ENiagaraSimTarget::DynamicLoadBalancedSim)
	{
		if (SpawnScriptProps.Script->IsScriptCompilationPending(false))
		{
			return false;
		}
		if (UpdateScriptProps.Script->IsScriptCompilationPending(false))
		{
			return false;
		}
		if (EventHandlerScriptProps.Num() != 0)
		{
			for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
			{
				if (EventHandlerScriptProps[i].Script->IsScriptCompilationPending(false))
				{
					return false;
				}
			}
		}
	}

	if (SimTarget == ENiagaraSimTarget::GPUComputeSim || SimTarget == ENiagaraSimTarget::DynamicLoadBalancedSim)
	{
		if (SpawnScriptProps.Script->IsScriptCompilationPending(true))
		{
			return false;
		}
	}

	return true;
}

void UNiagaraEmitter::GetScripts(TArray<UNiagaraScript*>& OutScripts, bool bCompilableOnly)
{
	OutScripts.Add(SpawnScriptProps.Script);
	OutScripts.Add(UpdateScriptProps.Script);
	if (!bCompilableOnly)
	{
		OutScripts.Add(EmitterSpawnScriptProps.Script);
		OutScripts.Add(EmitterUpdateScriptProps.Script);
	}

	for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
	{
		if (EventHandlerScriptProps[i].Script)
		{
			OutScripts.Add(EventHandlerScriptProps[i].Script);
		}
	}
}

UNiagaraScript* UNiagaraEmitter::GetScript(ENiagaraScriptUsage Usage, FGuid UsageId)
{
	TArray<UNiagaraScript*> Scripts;
	GetScripts(Scripts, false);
	for (UNiagaraScript* Script : Scripts)
	{
		if (Script->IsEquivalentUsage(Usage) && Script->GetUsageId() == UsageId)
		{
			return Script;
		}
	}
	return nullptr;
}

bool UNiagaraEmitter::IsAllowedByDetailLevel()const
{
	int32 DetailLevel = INiagaraModule::GetDetailLevel();
	if ((bUseMinDetailLevel && DetailLevel < MinDetailLevel) || (bUseMaxDetailLevel && DetailLevel > MaxDetailLevel))
	{
		return false;
	}

	return true;
}

#if WITH_EDITORONLY_DATA

FGuid UNiagaraEmitter::GetChangeId() const
{
	return ChangeId;
}

bool UNiagaraEmitter::AreAllScriptAndSourcesSynchronized() const
{
	if (SpawnScriptProps.Script->IsCompilable() && !SpawnScriptProps.Script->AreScriptAndSourceSynchronized())
	{
		return false;
	}

	if (UpdateScriptProps.Script->IsCompilable() && !UpdateScriptProps.Script->AreScriptAndSourceSynchronized())
	{
		return false;
	}

	if (EmitterSpawnScriptProps.Script->IsCompilable() && !EmitterSpawnScriptProps.Script->AreScriptAndSourceSynchronized())
	{
		return false;
	}

	if (EmitterUpdateScriptProps.Script->IsCompilable() && !EmitterUpdateScriptProps.Script->AreScriptAndSourceSynchronized())
	{
		return false;
	}

	for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
	{
		if (EventHandlerScriptProps[i].Script && EventHandlerScriptProps[i].Script->IsCompilable() && !EventHandlerScriptProps[i].Script->AreScriptAndSourceSynchronized())
		{
			return false;
		}
	}

	return true;
}

void UNiagaraEmitter::CompileScripts(TArray<ENiagaraScriptCompileStatus>& OutScriptStatuses, TArray<FString>& OutGraphLevelErrorMessages, TArray<FString>& PathNames,
	TArray<UNiagaraScript*>& Scripts, bool bForce)
{
	PathNames.AddDefaulted(2);
	OutScriptStatuses.Empty();
	OutGraphLevelErrorMessages.Empty();
	bool bPostCompile = false;

	// Force the end-user to reopen the file to resolve the problems.
	if (GraphSource == nullptr)
	{
		OutScriptStatuses.AddDefaulted(2);
		OutGraphLevelErrorMessages.AddDefaulted(2);

		OutGraphLevelErrorMessages[0] = OutGraphLevelErrorMessages[1] = TEXT("Please reopen asset in editor.");
		OutScriptStatuses[0] = OutScriptStatuses[1] = ENiagaraScriptCompileStatus::NCS_Error;
		return;
	}
	else if (!GraphSource->IsPreCompiled()) // If we haven't already precompiled this source, do so..
	{
		GraphSource->PreCompile(this);
		bPostCompile = true;
	}

	TArray<int32> OutputScriptIndices;
	TArray<int32> OutputStatusIndices;

	FString ErrorMsg;
	check((int32)EScriptCompileIndices::SpawnScript == 0);
	int32 StatusIdx = OutScriptStatuses.Add(SpawnScriptProps.Script->Compile(ErrorMsg, bForce));
	OutputStatusIndices.Add(StatusIdx);
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::SpawnScript
	int32 ScriptAddIdx = Scripts.Add(SpawnScriptProps.Script);
	OutputScriptIndices.Add(ScriptAddIdx);
	SpawnScriptProps.InitDataSetAccess();
	PathNames[(int32)EScriptCompileIndices::SpawnScript] = SpawnScriptProps.Script->GetPathName();

	ErrorMsg.Empty();
	check((int32)EScriptCompileIndices::UpdateScript == 1);
	StatusIdx = OutScriptStatuses.Add(UpdateScriptProps.Script->Compile(ErrorMsg, bForce));
	OutputStatusIndices.Add(StatusIdx);
	ScriptAddIdx = Scripts.Add(UpdateScriptProps.Script);
	OutputScriptIndices.Add(ScriptAddIdx);
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::UpdateScript
	UpdateScriptProps.InitDataSetAccess();
	PathNames[(int32)EScriptCompileIndices::UpdateScript] = UpdateScriptProps.Script->GetPathName();

	for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
	{
		if (EventHandlerScriptProps[i].Script)
		{
			check((int32)EScriptCompileIndices::EventScript == 2);
			ErrorMsg.Empty();
			StatusIdx = OutScriptStatuses.Add(EventHandlerScriptProps[i].Script->Compile(ErrorMsg, bForce));
			OutputStatusIndices.Add(StatusIdx);
			ScriptAddIdx = Scripts.Add(EventHandlerScriptProps[i].Script);
			OutputScriptIndices.Add(ScriptAddIdx);
			OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::EventScript
			EventHandlerScriptProps[i].InitDataSetAccess();
			PathNames.Add(EventHandlerScriptProps[i].Script->GetPathName());
		}
	}

	check(OutputScriptIndices.Num() == OutputStatusIndices.Num());
	check(OutGraphLevelErrorMessages.Num() == OutputScriptIndices.Num());

	// Go through all attached renderers and make sure that the scripts work properly for those renderers.
	for (int32 OutputScriptIdx = 0; OutputScriptIdx < OutputScriptIndices.Num(); OutputScriptIdx++)
	{
		int32 LoopScriptIdx = OutputScriptIndices[OutputScriptIdx];
		int32 LoopStatusIdx = OutputStatusIndices[OutputScriptIdx];
		UNiagaraScript* CurrentScript = Scripts[LoopScriptIdx];

		// Only care about scripts that successfully compiled so far.
		if (OutScriptStatuses[LoopStatusIdx] != ENiagaraScriptCompileStatus::NCS_UpToDate)
		{
			continue;
		}

		// Ignore missing scripts.
		if (CurrentScript == nullptr)
		{
			continue;
		}

		// Only care about particle scripts
		if (!(CurrentScript->IsParticleSpawnScript() ||
			CurrentScript->IsParticleUpdateScript()))
		{
			continue;
		}

		for (int32 i = 0; i < RendererProperties.Num(); i++)
		{
			if (RendererProperties[i] != nullptr)
			{
				const TArray<FNiagaraVariable>& RequiredAttrs = RendererProperties[i]->GetRequiredAttributes();

				for (FNiagaraVariable Attr : RequiredAttrs)
				{
					// TODO .. should we always be namespaced?
					FString AttrName = Attr.GetName().ToString();
					if (AttrName.RemoveFromStart(TEXT("Particles.")))
					{
						Attr.SetName(*AttrName);
					}

					if (false == Scripts[LoopScriptIdx]->Attributes.ContainsByPredicate([&Attr](const FNiagaraVariable& Var) { return Var.GetName() == Attr.GetName(); }))
					{
						OutGraphLevelErrorMessages[LoopStatusIdx] += FString::Printf(TEXT("\nCannot bind to renderer %s because it does not define attribute %s %s."), *RendererProperties[i]->GetName(), *Attr.GetType().GetNameText().ToString(), *Attr.GetName().ToString());
						OutScriptStatuses[LoopStatusIdx] = ENiagaraScriptCompileStatus::NCS_Error;
					}
				}
			}
		}
	}

	if (GraphSource && bPostCompile) // Cleanup if we precompiled inside this function
	{
		GraphSource->PostCompile();
	}
}

ENiagaraScriptCompileStatus UNiagaraEmitter::CompileScript(EScriptCompileIndices InScriptToCompile, FString& OutGraphLevelErrorMessages, bool bForce, int32 InSubScriptIdx)
{
	ENiagaraScriptCompileStatus CompileStatus = ENiagaraScriptCompileStatus::NCS_Unknown;
	bool bPostCompile = false;
	if (GraphSource && !GraphSource->IsPreCompiled()) // If we haven't already precompiled this source, do so..
	{
		bPostCompile = true;
		GraphSource->PreCompile(this);
	}

	switch (InScriptToCompile)
	{
	case EScriptCompileIndices::SpawnScript:
		CompileStatus = SpawnScriptProps.Script->Compile(OutGraphLevelErrorMessages, bForce);
		SpawnScriptProps.InitDataSetAccess();
		break;
	case EScriptCompileIndices::UpdateScript:
		CompileStatus = UpdateScriptProps.Script->Compile(OutGraphLevelErrorMessages, bForce);
		UpdateScriptProps.InitDataSetAccess();
		break;
	case EScriptCompileIndices::EventScript:
		if (EventHandlerScriptProps[InSubScriptIdx].Script)
		{
			CompileStatus = EventHandlerScriptProps[InSubScriptIdx].Script->Compile(OutGraphLevelErrorMessages, bForce);
			EventHandlerScriptProps[InSubScriptIdx].InitDataSetAccess();
		}
		else
		{
			return CompileStatus;
		}
		break;
	default:
		return CompileStatus;
	}

	if (GraphSource && bPostCompile) // Cleanup if we precompiled inside this function
	{
		GraphSource->PostCompile();
	}
	return CompileStatus;
}

UNiagaraEmitter* UNiagaraEmitter::MakeRecursiveDeepCopy(UObject* DestOuter) const
{
	TMap<const UObject*, UObject*> ExistingConversions;
	return MakeRecursiveDeepCopy(DestOuter, ExistingConversions);
}

UNiagaraEmitter* UNiagaraEmitter::MakeRecursiveDeepCopy(UObject* DestOuter, TMap<const UObject*, UObject*>& ExistingConversions) const
{
	ResetLoaders(GetTransientPackage());
	GetTransientPackage()->LinkerCustomVersion.Empty();

	EObjectFlags Flags = RF_AllFlags & ~RF_Standalone & ~RF_Public; // Remove Standalone and Public flags..
	UNiagaraEmitter* Props = CastChecked<UNiagaraEmitter>(StaticDuplicateObject(this, GetTransientPackage(), *GetName(), Flags));
	check(Props->HasAnyFlags(RF_Standalone) == false);
	check(Props->HasAnyFlags(RF_Public) == false);
	Props->Rename(nullptr, DestOuter, REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
	UE_LOG(LogNiagara, Warning, TEXT("MakeRecursiveDeepCopy %s"), *Props->GetFullName());
	ExistingConversions.Add(this, Props);

	check(GraphSource != Props->GraphSource);

	Props->GraphSource->SubsumeExternalDependencies(ExistingConversions);
	ExistingConversions.Add(GraphSource, Props->GraphSource);

	// Suck in the referenced scripts into this package.
	if (Props->SpawnScriptProps.Script)
	{
		Props->SpawnScriptProps.Script->SubsumeExternalDependencies(ExistingConversions);
		check(Props->GraphSource == Props->SpawnScriptProps.Script->GetSource());
	}

	if (Props->UpdateScriptProps.Script)
	{
		Props->UpdateScriptProps.Script->SubsumeExternalDependencies(ExistingConversions);
		check(Props->GraphSource == Props->UpdateScriptProps.Script->GetSource());
	}

	if (Props->EmitterSpawnScriptProps.Script)
	{
		Props->EmitterSpawnScriptProps.Script->SubsumeExternalDependencies(ExistingConversions);
		check(Props->GraphSource == Props->EmitterSpawnScriptProps.Script->GetSource());
	}
	if (Props->EmitterUpdateScriptProps.Script)
	{
		Props->EmitterUpdateScriptProps.Script->SubsumeExternalDependencies(ExistingConversions);
		check(Props->GraphSource == Props->EmitterUpdateScriptProps.Script->GetSource());
	}


	for (int32 i = 0; i < Props->GetEventHandlers().Num(); i++)
	{
		if (Props->GetEventHandlers()[i].Script)
		{
			Props->GetEventHandlers()[i].Script->SubsumeExternalDependencies(ExistingConversions);
			check(Props->GraphSource == Props->GetEventHandlers()[i].Script->GetSource());
		}
	}
	return Props;
}
#endif

bool UNiagaraEmitter::UsesScript(const UNiagaraScript* Script)const
{
	if (SpawnScriptProps.Script == Script || UpdateScriptProps.Script == Script || EmitterSpawnScriptProps.Script == Script || EmitterUpdateScriptProps.Script == Script)
	{
		return true;
	}
	for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
	{
		if (EventHandlerScriptProps[i].Script == Script)
		{
			return true;
		}
	}
	return false;
}

//TODO
// bool UNiagaraEmitter::UsesDataInterface(UNiagaraDataInterface* Interface)
//{
//}

bool UNiagaraEmitter::UsesCollection(const class UNiagaraParameterCollection* Collection)const
{
	if (SpawnScriptProps.Script && SpawnScriptProps.Script->UsesCollection(Collection))
	{
		return true;
	}
	if (UpdateScriptProps.Script && UpdateScriptProps.Script->UsesCollection(Collection))
	{
		return true;
	}
	for (int32 i = 0; i < EventHandlerScriptProps.Num(); i++)
	{
		if (EventHandlerScriptProps[i].Script && EventHandlerScriptProps[i].Script->UsesCollection(Collection))
		{
			return true;
		}
	}
	return false;
}

FString UNiagaraEmitter::GetUniqueEmitterName()const
{
	return UniqueEmitterName;
}

void UNiagaraEmitter::SetUniqueEmitterName(const FString& InName)
{
	if (InName != UniqueEmitterName)
	{
		Modify();
		FString OldName = UniqueEmitterName;
		UniqueEmitterName = InName;

		TMap<FString, FString> RenameMap;
		RenameMap.Add(OldName, InName);

		TArray<UNiagaraScript*> Scripts;
		GetScripts(Scripts, false);

		for (UNiagaraScript* Script : Scripts)
		{
			Script->Modify();

			// First handle any rapid iteration parameters...
			TArray<FNiagaraVariable> Params;
			Script->RapidIterationParameters.GetParameters(Params);
			for (FNiagaraVariable Var : Params)
			{
				FNiagaraVariable NewVar = FNiagaraVariable::ResolveAliases(Var, RenameMap);
				if (NewVar.GetName() != Var.GetName())
				{
					Script->RapidIterationParameters.RenameParameter(Var, NewVar.GetName());
				}
			}

			// Now handle any Parameters overall..
			for (int32 i = 0; i < Script->Parameters.Parameters.Num(); i++)
			{
				FNiagaraVariable Var = Script->Parameters.Parameters[i];
				FNiagaraVariable NewVar = FNiagaraVariable::ResolveAliases(Var, RenameMap);
				if (NewVar.GetName() != Var.GetName())
				{
					Script->Parameters.Parameters[i] = NewVar;
				}
			}

			// Also handle any data set mappings...
			auto Iterator = Script->DataSetToParameters.CreateIterator();
			while (Iterator)
			{
				for (int32 i = 0; i < Iterator.Value().Parameters.Num(); i++)
				{
					FNiagaraVariable Var = Iterator.Value().Parameters[i];
					FNiagaraVariable NewVar = FNiagaraVariable::ResolveAliases(Var, RenameMap);
					if (NewVar.GetName() != Var.GetName())
					{
						Iterator.Value().Parameters[i] = NewVar;
					}
				}
				++Iterator;
			}
		}

		// Since we use the emitter name as part of the script, we now need to recompile to work properly.
#if WITH_EDITORONLY_DATA
		if (GraphSource != nullptr)
		{
			GraphSource->MarkNotSynchronized();
		}
#endif
	}
}

FNiagaraVariable UNiagaraEmitter::ToEmitterParameter(const FNiagaraVariable& EmitterVar)const
{
	FNiagaraVariable Var = EmitterVar;
	Var.SetName(*Var.GetName().ToString().Replace(TEXT("Emitter."), *(GetUniqueEmitterName() + TEXT("."))));
	return Var;
}

void UNiagaraEmitter::AddRenderer(UNiagaraRendererProperties* Renderer)
{
	Modify();
	RendererProperties.Add(Renderer);
#if WITH_EDITOR
	Renderer->OnChanged().AddUObject(this, &UNiagaraEmitter::RendererChanged);
	UpdateChangeId();
#endif
}

void UNiagaraEmitter::RemoveRenderer(UNiagaraRendererProperties* Renderer)
{
	Modify();
	RendererProperties.Remove(Renderer);
#if WITH_EDITOR
	Renderer->OnChanged().RemoveAll(this);
	UpdateChangeId();
#endif
}

FNiagaraEventScriptProperties* UNiagaraEmitter::GetEventHandlerByIdUnsafe(FGuid ScriptUsageId)
{
	for (FNiagaraEventScriptProperties& EventScriptProperties : EventHandlerScriptProps)
	{
		if (EventScriptProperties.Script->GetUsageId() == ScriptUsageId)
		{
			return &EventScriptProperties;
		}
	}
	return nullptr;
}

void UNiagaraEmitter::AddEventHandler(FNiagaraEventScriptProperties EventHandler)
{
	EventHandlerScriptProps.Add(EventHandler);
#if WITH_EDITOR
	EventHandler.Script->RapidIterationParameters.AddOnChangedHandler(
		FNiagaraParameterStore::FOnChanged::FDelegate::CreateUObject(this, &UNiagaraEmitter::ScriptRapidIterationParameterChanged));
	UpdateChangeId();
#endif
}

void UNiagaraEmitter::RemoveEventHandlerByUsageId(FGuid EventHandlerUsageId)
{
	auto FindEventHandlerById = [=](const FNiagaraEventScriptProperties& EventHandler) { return EventHandler.Script->GetUsageId() == EventHandlerUsageId; };
#if WITH_EDITOR
	FNiagaraEventScriptProperties* EventHandler = EventHandlerScriptProps.FindByPredicate(FindEventHandlerById);
	if (EventHandler != nullptr)
	{
		EventHandler->Script->RapidIterationParameters.RemoveAllOnChangedHandlers(this);
	}
#endif
	EventHandlerScriptProps.RemoveAll(FindEventHandlerById);
#if WITH_EDITOR
	UpdateChangeId();
#endif
}

void UNiagaraEmitter::BeginDestroy()
{
#if WITH_EDITOR
	if (GraphSource != nullptr)
	{
		GraphSource->OnChanged().RemoveAll(this);
	}
#endif
	Super::BeginDestroy();
}

#if WITH_EDITORONLY_DATA

void UNiagaraEmitter::UpdateChangeId()
{
	Modify();
	ChangeId = FGuid::NewGuid();
}

void UNiagaraEmitter::ScriptRapidIterationParameterChanged()
{
	UpdateChangeId();
}

void UNiagaraEmitter::RendererChanged()
{
	UpdateChangeId();
}

void UNiagaraEmitter::GraphSourceChanged()
{
	UpdateChangeId();
}
#endif
