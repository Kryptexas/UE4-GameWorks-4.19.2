// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackAddScriptModuleItem.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScript.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeParameterMapBase.h"
#include "NiagaraConstants.h"
#include "Stack/NiagaraParameterHandle.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "NiagaraStackViewModel"

void UNiagaraStackAddScriptModuleItem::Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData, UNiagaraNodeOutput& InOutputNode, int32 InTargetIndex)
{
	Super::Initialize(InSystemViewModel, InEmitterViewModel, InStackEditorData);
	OutputNode = &InOutputNode;
	TargetIndex = InTargetIndex;
}

UNiagaraStackAddModuleItem::EDisplayMode UNiagaraStackAddScriptModuleItem::GetDisplayMode() const
{
	if (TargetIndex != INDEX_NONE)
	{
		return EDisplayMode::Compact;
	}
	return EDisplayMode::Standard;
}

void UNiagaraStackAddScriptModuleItem::GetAvailableParameters(TArray<FNiagaraVariable>& OutAvailableParameterVariables) const
{
	TArray<FNiagaraParameterMapHistory> Histories = UNiagaraNodeParameterMapBase::GetParameterMaps(OutputNode->GetNiagaraGraph());

	if (OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleSpawnScript ||
		OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleSpawnScriptInterpolated ||
		OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleUpdateScript ||
		OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleEventScript)
	{
		OutAvailableParameterVariables.Append(FNiagaraConstants::GetCommonParticleAttributes());
	}

	for (FNiagaraParameterMapHistory& History : Histories)
	{
		for (FNiagaraVariable& Variable : History.Variables)
		{
			if (History.IsPrimaryDataSetOutput(Variable, OutputNode->GetUsage()))
			{
				OutAvailableParameterVariables.AddUnique(Variable);
			}
		}
	}
}

void UNiagaraStackAddScriptModuleItem::GetNewParameterAvailableTypes(TArray<FNiagaraTypeDefinition>& OutAvailableTypes) const
{
	for (const FNiagaraTypeDefinition& RegisteredParameterType : FNiagaraTypeRegistry::GetRegisteredParameterTypes())
	{
		if (RegisteredParameterType != FNiagaraTypeDefinition::GetGenericNumericDef() && RegisteredParameterType != FNiagaraTypeDefinition::GetParameterMapDef())
		{
			OutAvailableTypes.Add(RegisteredParameterType);
		}
	}
}

TOptional<FName> UNiagaraStackAddScriptModuleItem::GetNewParameterNamespace() const
{
	switch (GetOutputUsage())
	{
	case ENiagaraScriptUsage::ParticleSpawnScript:
	case ENiagaraScriptUsage::ParticleUpdateScript:
		return FNiagaraParameterHandle::ParticleAttributeNamespace;
	case ENiagaraScriptUsage::EmitterSpawnScript:
	case ENiagaraScriptUsage::EmitterUpdateScript:
		return FNiagaraParameterHandle::EmitterNamespace;
	case ENiagaraScriptUsage::SystemSpawnScript:
	case ENiagaraScriptUsage::SystemUpdateScript:
		return FNiagaraParameterHandle::SystemNamespace;
	default:
		return TOptional<FName>();
	}
}

ENiagaraScriptUsage UNiagaraStackAddScriptModuleItem::GetOutputUsage() const
{
	return OutputNode->GetUsage();
}

UNiagaraNodeOutput* UNiagaraStackAddScriptModuleItem::GetOrCreateOutputNode()
{
	return OutputNode.Get();
}

int32 UNiagaraStackAddScriptModuleItem::GetTargetIndex() const
{
	return TargetIndex;
}

FName UNiagaraStackAddScriptModuleItem::GetItemBackgroundName() const
{
	if (GetDisplayMode() == UNiagaraStackAddModuleItem::EDisplayMode::Compact)
	{
		return "NiagaraEditor.Stack.Group.BackgroundColor";
	}
	else
	{
		return Super::GetItemBackgroundName();
	}
}

#undef LOCTEXT_NAMESPACE