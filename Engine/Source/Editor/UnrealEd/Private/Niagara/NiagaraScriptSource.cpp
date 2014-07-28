// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Engine/NiagaraScript.h"
#include "Engine/NiagaraConstants.h"
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


void UNiagaraScriptSource::GetParticleAttributes(TArray<FName>& VectorOutputs)
{
	VectorOutputs.Empty();

	VectorOutputs.Add(FName(TEXT("Particle Position")));
	VectorOutputs.Add(FName(TEXT("Particle Velocity")));
	VectorOutputs.Add(FName(TEXT("Particle Color")));
	VectorOutputs.Add(FName(TEXT("Particle Rotation")));
	VectorOutputs.Add(FName(TEXT("Particle Age")));
}

void UNiagaraScriptSource::GetEmitterAttributes(TArray<FName>& VectorInputs)
{
	for (uint32 i=0; i < NiagaraConstants::NumBuiltinConstants; i++)
	{
		VectorInputs.Add(NiagaraConstants::ConstantNames[i]);
	}
}

