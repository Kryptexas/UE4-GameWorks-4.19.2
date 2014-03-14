// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "NiagaraEditorModule.h"

//////////////////////////////////////////////////////////////////////////
// NiagaraGraph

UNiagaraGraph::UNiagaraGraph(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	Schema = UEdGraphSchema_Niagara::StaticClass();
}

class UNiagaraScriptSource* UNiagaraGraph::GetSource() const
{
	return CastChecked<UNiagaraScriptSource>(GetOuter());
}


//////////////////////////////////////////////////////////////////////////
// UNiagraScriptSource

UNiagaraScriptSource::UNiagaraScriptSource(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UNiagaraScriptSource::PostLoad()
{
	Super::PostLoad();
	UNiagaraScript* ScriptOwner = Cast<UNiagaraScript>(GetOuter());
	if (ScriptOwner)
	{
		ScriptOwner->ConditionalPostLoad();
		FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
		NiagaraEditorModule.CompileScript(ScriptOwner);
	}
}

void UNiagaraScriptSource::GetUpdateOutputs(TArray<FName>& ScalarOutputs)
{
	ScalarOutputs.Empty();

	ScalarOutputs.Add(FName(TEXT("Position.X")));
	ScalarOutputs.Add(FName(TEXT("Position.Y")));
	ScalarOutputs.Add(FName(TEXT("Position.Z")));

	ScalarOutputs.Add(FName(TEXT("Velocity.X")));
	ScalarOutputs.Add(FName(TEXT("Velocity.Y")));
	ScalarOutputs.Add(FName(TEXT("Velocity.Z")));

	ScalarOutputs.Add(FName(TEXT("Color.R")));
	ScalarOutputs.Add(FName(TEXT("Color.G")));
	ScalarOutputs.Add(FName(TEXT("Color.B")));
	ScalarOutputs.Add(FName(TEXT("Color.A")));

	ScalarOutputs.Add(FName(TEXT("Rotation")));

	ScalarOutputs.Add(FName(TEXT("RelativeTime")));
}

void UNiagaraScriptSource::GetUpdateInputs(TArray<FName>& ScalarInputs)
{
	GetUpdateOutputs(ScalarInputs);
	ScalarInputs.Add(FName(TEXT("DeltaTime")));
}
