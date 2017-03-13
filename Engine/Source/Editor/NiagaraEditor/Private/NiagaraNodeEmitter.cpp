// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeEmitter.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEditorUtilities.h"
#include "EdGraphSchema_Niagara.h"
#include "EdGraph/EdGraph.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeEmitter"


UNiagaraEffect* UNiagaraNodeEmitter::GetOwnerEffect() const
{
	return OwnerEffect;
}

void UNiagaraNodeEmitter::SetOwnerEffect(UNiagaraEffect* InOwnerEffect)
{
	OwnerEffect = InOwnerEffect;
	DisplayName = GetNameFromEmitter();
}

FGuid UNiagaraNodeEmitter::GetEmitterHandleId() const
{
	return EmitterHandleId;
}

void UNiagaraNodeEmitter::SetEmitterHandleId(FGuid InEmitterHandleId)
{
	EmitterHandleId = InEmitterHandleId;
	DisplayName = GetNameFromEmitter();
}

void UNiagaraNodeEmitter::AllocateDefaultPins()
{
	UNiagaraEmitterProperties* Emitter = nullptr;
	for (int32 i = 0; i < OwnerEffect->GetNumEmitters(); ++i)
	{
		if (OwnerEffect->GetEmitterHandle(i).GetId() == EmitterHandleId)
		{
			Emitter = OwnerEffect->GetEmitterHandle(i).GetInstance();
		}
	}

	if (Emitter != nullptr)
	{
		TSet<FName> SystemConstantNames = FNiagaraEditorUtilities::GetSystemConstantNames();
		const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(GetSchema());

		// Gather duplicate names for use when generating pins.
		TSet<FName> ParameterNames;
		TSet<FName> DuplicateParameterNames;

		for (const FNiagaraVariable& SpawnInputParameter : Emitter->SpawnScriptProps.Script->Parameters.Parameters)
		{
			if (ParameterNames.Contains(SpawnInputParameter.GetName()))
			{
				DuplicateParameterNames.Add(SpawnInputParameter.GetName());
			}
			else
			{
				ParameterNames.Add(SpawnInputParameter.GetName());
			}
		}

		for (const FNiagaraVariable& UpdateInputParameter : Emitter->UpdateScriptProps.Script->Parameters.Parameters)
		{
			if (ParameterNames.Contains(UpdateInputParameter.GetName()))
			{
				DuplicateParameterNames.Add(UpdateInputParameter.GetName());
			}
			else
			{
				ParameterNames.Add(UpdateInputParameter.GetName());
			}
		}

		// Generate the pins.
		for (const FNiagaraVariable& SpawnInputParameter : Emitter->SpawnScriptProps.Script->Parameters.Parameters)
		{
			if (SystemConstantNames.Contains(SpawnInputParameter.GetName()) == false)
			{
				FString PinName = DuplicateParameterNames.Contains(SpawnInputParameter.GetName())
					? TEXT("Spawn.") + SpawnInputParameter.GetName().ToString()
					: SpawnInputParameter.GetName().ToString();
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, NiagaraSchema->TypeDefinitionToPinType(SpawnInputParameter.GetType()), PinName);
				NewPin->bDefaultValueIsIgnored = true;
				NewPin->PersistentGuid = SpawnInputParameter.GetId();
			}
		}

		for (const FNiagaraVariable& UpdateInputParameter : Emitter->UpdateScriptProps.Script->Parameters.Parameters)
		{
			if (SystemConstantNames.Contains(UpdateInputParameter.GetName()) == false)
			{
				FString PinName = DuplicateParameterNames.Contains(UpdateInputParameter.GetName())
					? TEXT("Update.") + UpdateInputParameter.GetName().ToString()
					: UpdateInputParameter.GetName().ToString();
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, NiagaraSchema->TypeDefinitionToPinType(UpdateInputParameter.GetType()), PinName);
				NewPin->bDefaultValueIsIgnored = true;
				NewPin->PersistentGuid = UpdateInputParameter.GetId();
			}
		}
	}
}

bool UNiagaraNodeEmitter::CanUserDeleteNode() const
{
	return false;
}

bool UNiagaraNodeEmitter::CanDuplicateNode() const
{
	return false;
}

FText UNiagaraNodeEmitter::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return DisplayName;
}

FLinearColor UNiagaraNodeEmitter::GetNodeTitleColor() const
{
	return CastChecked<UEdGraphSchema_Niagara>(GetSchema())->NodeTitleColor_Attribute;
}

void UNiagaraNodeEmitter::NodeConnectionListChanged()
{
	GetGraph()->NotifyGraphChanged();
}

void UNiagaraNodeEmitter::RefreshFromExternalChanges()
{
	ReallocatePins();
	DisplayName = GetNameFromEmitter();
}

FText UNiagaraNodeEmitter::GetNameFromEmitter()
{
	if (OwnerEffect != nullptr && EmitterHandleId.IsValid())
	{
		for (const FNiagaraEmitterHandle& EmitterHandle : OwnerEffect->GetEmitterHandles())
		{
			if (EmitterHandle.GetId() == EmitterHandleId)
			{
				return FText::FromName(EmitterHandle.GetName());
			}
		}
	}
	return FText();
}

#undef LOCTEXT_NAMESPACE // NiagaraNodeEmitter
