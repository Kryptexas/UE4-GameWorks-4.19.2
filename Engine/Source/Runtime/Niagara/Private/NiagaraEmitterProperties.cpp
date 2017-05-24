// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "NiagaraEmitterProperties.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraCustomVersion.h"
#include "Package.h"

void FNiagaraEmitterScriptProperties::InitDataSetAccess()
{
	EventReceivers.Empty();
	EventGenerators.Empty();

	if (Script)
	{
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

//////////////////////////////////////////////////////////////////////////

UNiagaraEmitterProperties::UNiagaraEmitterProperties(const FObjectInitializer& Initializer)
: Super(Initializer)
, SpawnRate(50)
, Material(nullptr)
, StartTime(0.0f)
, EndTime(0.0f)
, NumLoops(0)
, CollisionMode(ENiagaraCollisionMode::None)
, RendererProperties(nullptr)
, bInterpolatedSpawning(false)
{
}

void UNiagaraEmitterProperties::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) == false)
	{
		RendererProperties = NewObject<UNiagaraSpriteRendererProperties>(this, "Renderer");

		SpawnScriptProps.Script = NewObject<UNiagaraScript>(this, "SpawnScript", EObjectFlags::RF_Transactional);
		SpawnScriptProps.Script->Usage = ENiagaraScriptUsage::SpawnScript;

		UpdateScriptProps.Script = NewObject<UNiagaraScript>(this, "UpdateScript", EObjectFlags::RF_Transactional);
		UpdateScriptProps.Script->Usage = ENiagaraScriptUsage::UpdateScript;
	}
}

void UNiagaraEmitterProperties::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FNiagaraCustomVersion::GUID);
}

void UNiagaraEmitterProperties::PostLoad()
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
// 		ENiagaraScriptUsage ActualUsage = bInterpolatedSpawning ? ENiagaraScriptUsage::SpawnScriptInterpolated : ENiagaraScriptUsage::SpawnScript;
// 		if (SpawnScriptProps.Script->Usage != ActualUsage)
// 		{
// 			SpawnScriptProps.Script->Usage = ActualUsage;
// 			SpawnScriptProps.Script->ByteCode.Empty();//Force them to recompile.
// 			UE_LOG(LogNiagara, Warning, TEXT("Niagara Emitter had mis matched bInterpolatedSpawning values. You must recompile this emitter. %s"), *GetFullName());
// 		}
// 	}
		
	//Temporarily disabling interpolated spawn.
	if (SpawnScriptProps.Script)
	{
		SpawnScriptProps.Script->ConditionalPostLoad();
		bool bActualInterpolatedSpawning = SpawnScriptProps.Script->Usage == ENiagaraScriptUsage::SpawnScriptInterpolated ? true : false;
		if (bInterpolatedSpawning || (!bInterpolatedSpawning && bActualInterpolatedSpawning))
		{
			bInterpolatedSpawning = false;
			if (bActualInterpolatedSpawning)
			{
				SpawnScriptProps.Script->ByteCode.Empty();//clear out the script as it was compiled with interpolated spawn.
#if WITH_EDITORONLY_DATA
				SpawnScriptProps.Script->ChangeId.Invalidate(); // Identify that we have no idea if we're synchronized or not.
#endif
				SpawnScriptProps.Script->Usage = ENiagaraScriptUsage::SpawnScript;
			}
			UE_LOG(LogNiagara, Warning, TEXT("Temporarily disabling interpolated spawn. Emitter may need recompile.. %s"), *GetFullName());
		}
	}

	// Referenced scripts may need to be invalidated due to version mismatch or 
	// other issues. This is determined in PostLoad for UNiagaraScript, so we need
	// to make sure that PostLoad has happened for each of them first.
	bool bNeedsRecompile = false;
	if (SpawnScriptProps.Script)
	{
		SpawnScriptProps.Script->ConditionalPostLoad();
		if (SpawnScriptProps.Script->ByteCode.Num() == 0)
		{
			bNeedsRecompile = true;
		}
	}
	if (UpdateScriptProps.Script)
	{
		UpdateScriptProps.Script->ConditionalPostLoad();
		if (UpdateScriptProps.Script->ByteCode.Num() == 0)
		{
			bNeedsRecompile = true;
		}
	}
	if (EventHandlerScriptProps.Script)
	{
		EventHandlerScriptProps.Script->ConditionalPostLoad();
		if (EventHandlerScriptProps.Script->ByteCode.Num() == 0)
		{
			bNeedsRecompile = true;
		}
	}

	// Check ourselves against the current script rebuild version. Force our children
	// to need to update.
	const int32 NiagaraVer = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
	if (NiagaraVer < FNiagaraCustomVersion::PostLoadCompilationEnabled)
	{
		bNeedsRecompile = true;
		if (SpawnScriptProps.Script)
		{
			SpawnScriptProps.Script->ByteCode.Empty();//clear out the script.
#if WITH_EDITORONLY_DATA
			SpawnScriptProps.Script->ChangeId.Invalidate(); // Identify that we have no idea if we're synchronized or not.
#endif
		}
		if (UpdateScriptProps.Script)
		{
			UpdateScriptProps.Script->ByteCode.Empty();//clear out the script.
#if WITH_EDITORONLY_DATA
			UpdateScriptProps.Script->ChangeId.Invalidate(); // Identify that we have no idea if we're synchronized or not.
#endif
		}
		if (EventHandlerScriptProps.Script)
		{
			EventHandlerScriptProps.Script->ByteCode.Empty();//clear out the script.
#if WITH_EDITORONLY_DATA
			EventHandlerScriptProps.Script->ChangeId.Invalidate(); // Identify that we have no idea if we're synchronized or not.
#endif
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
			CompileScripts(ScriptStatuses, ScriptErrors, PathNames);

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
#endif
}

#if WITH_EDITOR
void UNiagaraEmitterProperties::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


#if WITH_EDITORONLY_DATA
void UNiagaraEmitterProperties::CompileScripts(TArray<ENiagaraScriptCompileStatus>& OutScriptStatuses, TArray<FString>& OutGraphLevelErrorMessages, TArray<FString>& PathNames)
{
	PathNames.AddDefaulted(2);
	OutScriptStatuses.Empty();
	OutGraphLevelErrorMessages.Empty();

	FString ErrorMsg;
	check((int32)EScriptCompileIndices::SpawnScript == 0);
	OutScriptStatuses.Add(SpawnScriptProps.Script->Source->Compile(ErrorMsg));
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::SpawnScript
	SpawnScriptProps.InitDataSetAccess();
	PathNames[(int32)EScriptCompileIndices::SpawnScript] = SpawnScriptProps.Script->GetPathName();

	ErrorMsg.Empty();
	check((int32)EScriptCompileIndices::UpdateScript == 1);
	OutScriptStatuses.Add(UpdateScriptProps.Script->Source->Compile(ErrorMsg));
	OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::UpdateScript
	UpdateScriptProps.InitDataSetAccess();
	PathNames[(int32)EScriptCompileIndices::UpdateScript] = UpdateScriptProps.Script->GetPathName();

	if (EventHandlerScriptProps.Script)
	{
		check((int32)EScriptCompileIndices::EventScript == 2);
		ErrorMsg.Empty();
		OutScriptStatuses.Add(EventHandlerScriptProps.Script->Source->Compile(ErrorMsg));
		OutGraphLevelErrorMessages.Add(ErrorMsg); // EScriptCompileIndices::EventScript
		EventHandlerScriptProps.InitDataSetAccess();
		PathNames.Add(EventHandlerScriptProps.Script->GetPathName());
	}
	ChangeId = FGuid::NewGuid();
}

ENiagaraScriptCompileStatus UNiagaraEmitterProperties::CompileScript(EScriptCompileIndices InScriptToCompile, FString& OutGraphLevelErrorMessages)
{
	ENiagaraScriptCompileStatus CompileStatus = ENiagaraScriptCompileStatus::NCS_Unknown;
	switch (InScriptToCompile)
	{
	case EScriptCompileIndices::SpawnScript:
		CompileStatus = SpawnScriptProps.Script->Source->Compile(OutGraphLevelErrorMessages);
		SpawnScriptProps.InitDataSetAccess();
		break;
	case EScriptCompileIndices::UpdateScript:
		CompileStatus = UpdateScriptProps.Script->Source->Compile(OutGraphLevelErrorMessages);
		UpdateScriptProps.InitDataSetAccess();
		break;
	case EScriptCompileIndices::EventScript:
		if (EventHandlerScriptProps.Script)
		{
			CompileStatus = EventHandlerScriptProps.Script->Source->Compile(OutGraphLevelErrorMessages);
			EventHandlerScriptProps.InitDataSetAccess();
		}
		else
		{
			return CompileStatus;
		}
		break;
	default:
		return CompileStatus;
	}
	ChangeId = FGuid::NewGuid();
	return CompileStatus;
}

UNiagaraEmitterProperties* UNiagaraEmitterProperties::MakeRecursiveDeepCopy(UObject* DestOuter) const
{
	EObjectFlags Flags = RF_AllFlags & ~RF_Standalone & ~RF_Public; // Remove Standalone and Public flags..
	UNiagaraEmitterProperties* Props = CastChecked<UNiagaraEmitterProperties>(StaticDuplicateObject(this, DestOuter, NAME_None, Flags));

	check(Props->HasAnyFlags(RF_Standalone) == false);
	check(Props->HasAnyFlags(RF_Public) == false);

	// Suck in the referenced scripts into this package.
	TMap<UObject*, UObject*> ExistingConversions;
	if (Props->SpawnScriptProps.Script)
	{
		Props->SpawnScriptProps.Script->SubsumeExternalDependencies(ExistingConversions);
	}

	if (Props->UpdateScriptProps.Script)
	{
		Props->UpdateScriptProps.Script->SubsumeExternalDependencies(ExistingConversions);
	}
	
	if (Props->EventHandlerScriptProps.Script)
	{
		Props->EventHandlerScriptProps.Script->SubsumeExternalDependencies(ExistingConversions);
	}
	return Props;
}
#endif