// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeEmitter.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEditorUtilities.h"
#include "EdGraphSchema_Niagara.h"
#include "EdGraph/EdGraph.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterface.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeInput.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeEmitter"

void UNiagaraNodeEmitter::InitDefaultEmitterProperties()
{
	SpawnRate = FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), FName(*FNiagaraSimulation::GetEmitterSpawnRateInternalVarName()));
	SpawnRate.SetId(FGuid::NewGuid());

	EmitterEnabled = FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), FName(*FNiagaraSimulation::GetEmitterEnabledInternalVarName()));
	EmitterEnabled.SetId(FGuid::NewGuid());
}

void UNiagaraNodeEmitter::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) == false)
	{
		InitDefaultEmitterProperties();
	}
}

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

void UNiagaraNodeEmitter::BuildNameList(TSet<FName>& ParameterNames, TSet<FName>& DuplicateParameterNames, UNiagaraScript* Script)
{
	if (Script == nullptr)
	{
		return;
	}

	if (Script->Source == nullptr || !Script->Source->IsA(UNiagaraScriptSource::StaticClass()))
	{
		return;
	}

	UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->Source);
	if (ScriptSource->NodeGraph == nullptr)
	{
		return;
	}

	TArray<UNiagaraNodeInput*> InputNodes;
	UNiagaraGraph::FFindInputNodeOptions Options;
	Options.bSort = true;
	Options.bFilterDuplicates = true;
	Options.bIncludeAttributes = false;
	Options.bIncludeSystemConstants = false;
	ScriptSource->NodeGraph->FindInputNodes(InputNodes, Options);

	for (const UNiagaraNodeInput* InputNode : InputNodes)
	{
		if (ParameterNames.Contains(InputNode->Input.GetName()))
		{
			DuplicateParameterNames.Add(InputNode->Input.GetName());
		}
		else
		{
			ParameterNames.Add(InputNode->Input.GetName());
		}
	}
}

void UNiagaraNodeEmitter::PostLoad()
{
	Super::PostLoad();

	if (SpawnRate.GetId().IsValid() == false || EmitterEnabled.GetId().IsValid() == false)
	{
		InitDefaultEmitterProperties();
		GenerateDefaultPins();
	}
}

void UNiagaraNodeEmitter::GenerateDefaultPins()
{
	const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(GetSchema());

	UEdGraphPin* NewPin = CreatePin(EGPD_Input, NiagaraSchema->TypeDefinitionToPinType(EmitterEnabled.GetType()), EmitterEnabled.GetName().ToString(), 0);
	NewPin->PinFriendlyName = FText::FromString("Emitter Enabled");
	NewPin->bDefaultValueIsIgnored = true;
	NewPin->PersistentGuid = EmitterEnabled.GetId();

	NewPin = CreatePin(EGPD_Input, NiagaraSchema->TypeDefinitionToPinType(SpawnRate.GetType()), SpawnRate.GetName().ToString(), 1);
	NewPin->PinFriendlyName = FText::FromString("Emitter Spawn Rate");
	NewPin->bDefaultValueIsIgnored = true;
	NewPin->PersistentGuid = SpawnRate.GetId();
}

void UNiagaraNodeEmitter::GeneratePinsForScript(const UEdGraphSchema_Niagara* NiagaraSchema, const TSet<FName>& SystemConstantNames, TSet<FName>& ParameterNames, TSet<FName>& DuplicateParameterNames, UNiagaraScript* Script, const FString& DuplicatePrefix)
{
	if (Script == nullptr)
	{
		return;
	}

	if (Script->Source == nullptr || !Script->Source->IsA(UNiagaraScriptSource::StaticClass()))
	{
		return;
	}

	UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->Source);
	if (ScriptSource->NodeGraph == nullptr)
	{
		return;
	}

	TArray<UNiagaraNodeInput*> InputNodes;
	UNiagaraGraph::FFindInputNodeOptions Options;
	Options.bSort = true;
	Options.bFilterDuplicates = true;
	Options.bIncludeAttributes = false;
	Options.bIncludeSystemConstants = false;
	ScriptSource->NodeGraph->FindInputNodes(InputNodes, Options);
	
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		FString PinName = DuplicateParameterNames.Contains(InputNode->Input.GetName())
			? DuplicatePrefix + InputNode->Input.GetName().ToString()
			: InputNode->Input.GetName().ToString();
		UEdGraphPin* NewPin = CreatePin(EGPD_Input, NiagaraSchema->TypeDefinitionToPinType(InputNode->Input.GetType()), PinName);
		NewPin->bDefaultValueIsIgnored = true;
		NewPin->PersistentGuid = InputNode->Input.GetId();
	}
}

void UNiagaraNodeEmitter::AllocateDefaultPins()
{
	UNiagaraEmitterProperties* Emitter = nullptr;
	if (OwnerEffect)
	{
		for (int32 i = 0; i < OwnerEffect->GetNumEmitters(); ++i)
		{
			if (OwnerEffect->GetEmitterHandle(i).GetId() == EmitterHandleId)
			{
				Emitter = OwnerEffect->GetEmitterHandle(i).GetInstance();
			}
		}
	}

	if (Emitter != nullptr)
	{
		GenerateDefaultPins();

		TSet<FName> SystemConstantNames = FNiagaraEditorUtilities::GetSystemConstantNames();
		const UEdGraphSchema_Niagara* NiagaraSchema = Cast<UEdGraphSchema_Niagara>(GetSchema());

		// Gather duplicate names for use when generating pins.
		TSet<FName> ParameterNames;
		TSet<FName> DuplicateParameterNames;

		BuildNameList(ParameterNames, DuplicateParameterNames, Emitter->SpawnScriptProps.Script);
		BuildNameList(ParameterNames, DuplicateParameterNames, Emitter->UpdateScriptProps.Script);
		BuildNameList(ParameterNames, DuplicateParameterNames, Emitter->EventHandlerScriptProps.Script);
		
		// Generate the pins.
		GeneratePinsForScript(NiagaraSchema, SystemConstantNames, ParameterNames, DuplicateParameterNames, Emitter->SpawnScriptProps.Script, TEXT("Spawn."));
		GeneratePinsForScript(NiagaraSchema, SystemConstantNames, ParameterNames, DuplicateParameterNames, Emitter->UpdateScriptProps.Script, TEXT("Update."));
		GeneratePinsForScript(NiagaraSchema, SystemConstantNames, ParameterNames, DuplicateParameterNames, Emitter->EventHandlerScriptProps.Script, TEXT("EventHandler."));

		CachedEmitterChangeId = Emitter->ChangeId;
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

bool UNiagaraNodeEmitter::RefreshFromExternalChanges()
{
	// First get the emitter that we're referencing..
	UNiagaraEmitterProperties* Emitter = nullptr;
	if (OwnerEffect)
	{
		for (int32 i = 0; i < OwnerEffect->GetNumEmitters(); ++i)
		{
			if (OwnerEffect->GetEmitterHandle(i).GetId() == EmitterHandleId)
			{
				Emitter = OwnerEffect->GetEmitterHandle(i).GetInstance();
			}
		}
	}

	if (Emitter && Emitter->ChangeId != CachedEmitterChangeId)
	{
		// TODO - Leverage code in reallocate pins to determine if any pins have changed...
		ReallocatePins();
		DisplayName = GetNameFromEmitter();
		return true;
	}
	return false;
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
bool UNiagaraNodeEmitter::IsEmitterInternalParameter(const FString& InVar)
{
	if (InVar == FNiagaraSimulation::GetEmitterEnabledInternalVarName() || InVar == FNiagaraSimulation::GetEmitterSpawnRateInternalVarName())
	{
		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE // NiagaraNodeEmitter
