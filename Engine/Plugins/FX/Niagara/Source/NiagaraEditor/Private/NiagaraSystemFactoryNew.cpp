// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraSystemFactoryNew.h"
#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"
#include "NiagaraSystem.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraEditorSettings.h"
#include "AssetData.h"
#include "NiagaraStackGraphUtilities.h"

#define LOCTEXT_NAMESPACE "NiagaraSystemFactory"

UNiagaraSystemFactoryNew::UNiagaraSystemFactoryNew(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

	SupportedClass = UNiagaraSystem::StaticClass();
	bCreateNew = false;
	bEditAfterNew = true;
	bCreateNew = true;
}

UObject* UNiagaraSystemFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UNiagaraSystem::StaticClass()));

	const UNiagaraEditorSettings* Settings = GetDefault<UNiagaraEditorSettings>();
	check(Settings);

	UNiagaraSystem* NewSystem;
	
	if (UNiagaraSystem* Default = Cast<UNiagaraSystem>(Settings->DefaultSystem.TryLoad()))
	{
		NewSystem = Cast<UNiagaraSystem>(StaticDuplicateObject(Default, InParent, Name, Flags, Class));
		InitializeSystem(NewSystem, false);
	}
	else
	{
		NewSystem = NewObject<UNiagaraSystem>(InParent, Class, Name, Flags | RF_Transactional);
		InitializeSystem(NewSystem, true);
	}


	return NewSystem;
}

void UNiagaraSystemFactoryNew::InitializeSystem(UNiagaraSystem* System, bool bCreateDefaultNodes)
{
	UNiagaraScript* SystemSpawnScript = System->GetSystemSpawnScript();
	UNiagaraScript* SystemUpdateScript = System->GetSystemUpdateScript();
	UNiagaraScript* SystemSpawnScriptSolo = System->GetSystemSpawnScript(true);
	UNiagaraScript* SystemUpdateScriptSolo = System->GetSystemUpdateScript(true);

	UNiagaraScriptSource* SystemScriptSource = NewObject<UNiagaraScriptSource>(SystemSpawnScript, "SystemScriptSource", RF_Transactional);

	if (SystemScriptSource)
	{
		SystemScriptSource->NodeGraph = NewObject<UNiagaraGraph>(SystemScriptSource, "SystemScriptGraph", RF_Transactional);
	}

	SystemSpawnScript->SetSource(SystemScriptSource);
	SystemUpdateScript->SetSource(SystemScriptSource);
	SystemSpawnScriptSolo->SetSource(SystemScriptSource);
	SystemUpdateScriptSolo->SetSource(SystemScriptSource);

	if (bCreateDefaultNodes)
	{
		FStringAssetReference SystemUpdateScriptRef(TEXT("/Niagara/Modules/System/SystemLifeCycle.SystemLifeCycle"));
		UNiagaraScript* Script = Cast<UNiagaraScript>(SystemUpdateScriptRef.TryLoad());

		FAssetData ModuleScriptAsset(Script);
		if (SystemScriptSource && ModuleScriptAsset.IsValid())
		{
			UNiagaraNodeOutput* SpawnOutputNode = FNiagaraStackGraphUtilities::ResetGraphForOutput(*SystemScriptSource->NodeGraph, ENiagaraScriptUsage::SystemSpawnScript, SystemSpawnScript->GetUsageId());
			UNiagaraNodeOutput* UpdateOutputNode = FNiagaraStackGraphUtilities::ResetGraphForOutput(*SystemScriptSource->NodeGraph, ENiagaraScriptUsage::SystemUpdateScript, SystemUpdateScript->GetUsageId());

			if (UpdateOutputNode)
			{
				FNiagaraStackGraphUtilities::AddScriptModuleToStack(ModuleScriptAsset, *UpdateOutputNode);
			}
			FNiagaraStackGraphUtilities::RelayoutGraph(*SystemScriptSource->NodeGraph);
		}
	}
}

#undef LOCTEXT_NAMESPACE