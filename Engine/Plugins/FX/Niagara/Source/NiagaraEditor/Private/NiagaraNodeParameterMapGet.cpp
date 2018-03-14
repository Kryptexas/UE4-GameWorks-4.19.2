// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeParameterMapGet.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraEditorUtilities.h"
#include "SNiagaraGraphNodeConvert.h"
#include "NiagaraGraph.h"
#include "NiagaraHlslTranslator.h"
#include "ScopedTransaction.h"
#include "SNiagaraGraphParameterMapGetNode.h"
#include "NiagaraEditorModule.h"
#include "INiagaraEditorTypeUtilities.h"
#include "ModuleManager.h"
#include "SharedPointer.h"
#include "NiagaraConstants.h"
#include "MultiBoxBuilder.h"
#include "SBox.h"
#include "SEditableTextBox.h"
#include "EdGraph/EdGraphNode.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeParameterMapGet"

UNiagaraNodeParameterMapGet::UNiagaraNodeParameterMapGet() : UNiagaraNodeParameterMapBase(), PinPendingRename(nullptr)
{

}


void UNiagaraNodeParameterMapGet::AllocateDefaultPins()
{
	PinPendingRename = nullptr;
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(FNiagaraTypeDefinition::GetParameterMapDef()), TEXT("Source"));
	CreateAddPin(EGPD_Output);
}


TSharedPtr<SGraphNode> UNiagaraNodeParameterMapGet::CreateVisualWidget()
{
	return SNew(SNiagaraGraphParameterMapGetNode, this);
}

bool UNiagaraNodeParameterMapGet::IsPinNameEditable(const UEdGraphPin* GraphPinObj) const
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	FNiagaraTypeDefinition TypeDef = Schema->PinToTypeDefinition(GraphPinObj);
	if (TypeDef.IsValid() && GraphPinObj && GraphPinObj->Direction == EGPD_Output && CanRenamePin(GraphPinObj))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool UNiagaraNodeParameterMapGet::IsPinNameEditableUponCreation(const UEdGraphPin* GraphPinObj) const
{
	if (GraphPinObj == PinPendingRename && GraphPinObj->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool UNiagaraNodeParameterMapGet::VerifyEditablePinName(const FText& InName, FText& OutErrorMessage, const UEdGraphPin* InGraphPinObj) const
{
	if (InName.IsEmptyOrWhitespace() && InGraphPinObj->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		OutErrorMessage = LOCTEXT("InvalidName", "Invalid pin name");
		return false;
	}
	return true;
}

UEdGraphPin* UNiagaraNodeParameterMapGet::CreateDefaultPin(UEdGraphPin* OutputPin)
{
	if (OutputPin == nullptr)
	{
		return nullptr;
	}

	UEdGraphPin* DefaultPin = CreatePin(EEdGraphPinDirection::EGPD_Input, OutputPin->PinType, TEXT(""));

	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	FNiagaraVariable Var = Schema->PinToNiagaraVariable(OutputPin, true);
	FString PinDefaultValue;
	if (Schema->TryGetPinDefaultValueFromNiagaraVariable(Var, PinDefaultValue))
	{
		DefaultPin->DefaultValue = PinDefaultValue;
	}
	
	if (!OutputPin->PersistentGuid.IsValid())
	{
		OutputPin->PersistentGuid = FGuid::NewGuid();
	}
	if (!DefaultPin->PersistentGuid.IsValid())
	{
		DefaultPin->PersistentGuid = FGuid::NewGuid();
	}
	PinOutputToPinDefaultPersistentId.Add(OutputPin->PersistentGuid, DefaultPin->PersistentGuid);

	SynchronizeDefaultInputPin(DefaultPin, OutputPin);
	return DefaultPin;
}

void UNiagaraNodeParameterMapGet::OnPinRenamed(UEdGraphPin* RenamedPin, const FString& OldName)
{
	RenamedPin->PinFriendlyName = FText::FromName(RenamedPin->PinName);

	FNiagaraTypeDefinition VarType = CastChecked<UEdGraphSchema_Niagara>(GetSchema())->PinToTypeDefinition(RenamedPin);
	FNiagaraVariable Var(VarType, *OldName);

	UNiagaraGraph* Graph = GetNiagaraGraph();
	FNiagaraVariableMetaData* OldMetaData = Graph->GetMetaData(Var);

	if (OldMetaData)
	{
		Graph->Modify();
		OldMetaData->ReferencerNodes.Remove(TWeakObjectPtr<UObject>(this));
	}

	FNiagaraVariable NewVar(VarType, *RenamedPin->GetName());
	FNiagaraVariableMetaData* NewMetaData = Graph->GetMetaData(NewVar);

	// If no variable has already defined the meta-data for this entry and it isn't being redefined to one of our constants, copy over the old values that make sense (not display name).
	if (NewMetaData == nullptr && OldMetaData != nullptr && !FNiagaraConstants::IsNiagaraConstant(NewVar))
	{
		FNiagaraVariableMetaData& NewVarMetaData = Graph->FindOrAddMetaData(NewVar);
		NewVarMetaData.PropertyMetaData = OldMetaData->PropertyMetaData;
		NewVarMetaData.PropertyMetaData.Remove(TEXT("DisplayName")); // DisplayName is probably incorrect, so drop it.
		NewVarMetaData.ReferencerNodes.Add(TWeakObjectPtr<UObject>(this));
	}

	if (OldMetaData)
	{
		Graph->PurgeUnreferencedMetaData();
	}

	if (RenamedPin == PinPendingRename)
	{
		PinPendingRename = nullptr;
	}


	UEdGraphPin* DefaultPin = GetDefaultPin(RenamedPin);
	
	if (DefaultPin)
	{
		DefaultPin->Modify();
		SynchronizeDefaultInputPin(DefaultPin, RenamedPin);
	}

	Graph->NotifyGraphNeedsRecompile();
}


void UNiagaraNodeParameterMapGet::OnNewTypedPinAdded(UEdGraphPin* NewPin)
{
	if (NewPin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
		FNiagaraTypeDefinition TypeDef = Schema->PinToTypeDefinition(NewPin);

		UEdGraphPin* MatchingDefault = GetDefaultPin(NewPin);
		if (MatchingDefault == nullptr)
		{
			UEdGraphPin* DefaultPin = CreateDefaultPin(NewPin);
		}
	}

	if (HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_NeedInitialization))
	{
		return;
	}

	if (NewPin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		PinPendingRename = NewPin;
	}

}

void UNiagaraNodeParameterMapGet::RemoveDynamicPin(UEdGraphPin* Pin)
{
	FScopedTransaction RemovePinTransaction(LOCTEXT("RemovePinTransaction", "Remove pin"));

	UEdGraphPin* DefaultPin = nullptr;
	if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		DefaultPin = GetDefaultPin(Pin);

		FNiagaraVariable Var = CastChecked<UEdGraphSchema_Niagara>(GetSchema())->PinToNiagaraVariable(Pin);

		UNiagaraGraph* Graph = GetNiagaraGraph();
		FNiagaraVariableMetaData* OldMetaData = Graph->GetMetaData(Var);

		if (OldMetaData)
		{
			Graph->Modify();
			OldMetaData->ReferencerNodes.Remove(TWeakObjectPtr<UObject>(this));
		}
	}

	RemovePin(Pin);

	if (DefaultPin != nullptr)
	{
		RemovePin(DefaultPin);
	}

	GetGraph()->NotifyGraphChanged();
}

UEdGraphPin* UNiagaraNodeParameterMapGet::GetDefaultPin(UEdGraphPin* OutputPin)
{
	TArray<UEdGraphPin*> InputPins;
	GetInputPins(InputPins);

	const FGuid* InputGuid = PinOutputToPinDefaultPersistentId.Find(OutputPin->PersistentGuid);

	if (InputGuid != nullptr)
	{
		for (UEdGraphPin* InputPin : InputPins)
		{
			if ((*InputGuid) == InputPin->PersistentGuid)
			{
				return InputPin;
			}
		}
	}

	return nullptr;
}


UEdGraphPin* UNiagaraNodeParameterMapGet::GetOutputPinForDefault(UEdGraphPin* DefaultPin)
{
	TArray<UEdGraphPin*> OutputPins;
	GetOutputPins(OutputPins);

	const FGuid* OutputGuid = PinOutputToPinDefaultPersistentId.Find(DefaultPin->PersistentGuid);

	if (OutputGuid != nullptr)
	{
		for (UEdGraphPin* OutputPin : OutputPins)
		{
			if ((*OutputGuid) == OutputPin->PersistentGuid)
			{
				return OutputPin;
			}
		}
	}

	return nullptr;
}

void UNiagaraNodeParameterMapGet::PostLoad()
{
	Super::PostLoad();
	TArray<UEdGraphPin*> OutputPins;
	GetOutputPins(OutputPins);
	for (int32 i = 0; i < OutputPins.Num(); i++)
	{
		if (IsAddPin(OutputPins[i]))
		{
			continue;
		}
		UEdGraphPin* InputPin = GetDefaultPin(OutputPins[i]);
		if (InputPin == nullptr)
		{
			CreateDefaultPin(OutputPins[i]);
		}
		else
		{
			SynchronizeDefaultInputPin(InputPin, OutputPins[i]);
		}
	}
}


void UNiagaraNodeParameterMapGet::SynchronizeDefaultInputPin(UEdGraphPin* DefaultPin, UEdGraphPin* OutputPin)
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	if (!DefaultPin)
	{
		return;
	}

	if (FNiagaraParameterMapHistory::IsEngineParameter(Schema->PinToNiagaraVariable(OutputPin)))
	{
		DefaultPin->bDefaultValueIsIgnored = true;
		DefaultPin->bNotConnectable = true;
		DefaultPin->bHidden = true;
		DefaultPin->PinToolTip = FText::Format(LOCTEXT("DefaultValueTooltip", "Default value for {0}. Disabled for Engine Parameters."), FText::FromName(OutputPin->PinName)).ToString();
	}
	else
	{
		DefaultPin->bDefaultValueIsIgnored = false;
		DefaultPin->bNotConnectable = false;
		DefaultPin->bHidden = false;
		DefaultPin->PinToolTip = FText::Format(LOCTEXT("DefaultValueTooltip", "Default value for {0} if no other module has set it previously in the stack."), FText::FromName(OutputPin->PinName)).ToString();
	}
}


FText UNiagaraNodeParameterMapGet::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("UNiagaraNodeParameterMapGetName", "Map Get");
}

void UNiagaraNodeParameterMapGet::BuildParameterMapHistory(FNiagaraParameterMapHistoryBuilder& OutHistory, bool bRecursive)
{
	if (bRecursive)
	{
		OutHistory.VisitInputPin(GetInputPin(0), this);
	}

	if (!IsNodeEnabled() && OutHistory.GetIgnoreDisabled())
	{
		RouteParameterMapAroundMe(OutHistory, bRecursive);
		return;
	}

	int32 ParamMapIdx = INDEX_NONE;
	if (GetInputPin(0)->LinkedTo.Num() != 0)
	{
		ParamMapIdx = OutHistory.TraceParameterMapOutputPin(GetInputPin(0)->LinkedTo[0]);
	}

	if (ParamMapIdx != INDEX_NONE)
	{
		TArray<UEdGraphPin*> OutputPins;
		GetOutputPins(OutputPins);
		for (int32 i = 0; i < OutputPins.Num(); i++)
		{
			if (IsAddPin(OutputPins[i]))
			{
				continue;
			}

			bool bUsedDefaults = false;
			if (bRecursive)
			{
				OutHistory.HandleVariableRead(ParamMapIdx, OutputPins[i], true, GetDefaultPin(OutputPins[i]), bUsedDefaults);
			}
			else
			{
				OutHistory.HandleVariableRead(ParamMapIdx, OutputPins[i], true, nullptr, bUsedDefaults);
			}
		}
	}
}

void UNiagaraNodeParameterMapGet::Compile(class FHlslNiagaraTranslator* Translator, TArray<int32>& Outputs)
{
	const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(GetSchema());

	TArray<UEdGraphPin*> InputPins;
	GetInputPins(InputPins);
	TArray<UEdGraphPin*> OutputPins;
	GetOutputPins(OutputPins);

	// Initialize the outputs to invalid values.
	check(Outputs.Num() == 0);
	for (int32 i = 0; i < OutputPins.Num(); i++)
	{
		if (IsAddPin(OutputPins[i]))
		{
			continue;
		}
		Outputs.Add(INDEX_NONE);
	}

	// First compile fully down the hierarchy for our predecessors..
	TArray<int32> CompileInputs;

	for (int32 i = 0; i < InputPins.Num(); i++)
	{
		UEdGraphPin* InputPin = InputPins[i];

		if (InputPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType || 
			InputPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryEnum)
		{
			int32 CompiledInput = INDEX_NONE;
			if (i == 0) // Only the zeroth item is not an default value pin.
			{
				CompiledInput = Translator->CompilePin(InputPin);
				if (CompiledInput == INDEX_NONE)
				{
					Translator->Error(LOCTEXT("InputError", "Error compiling input for param map get node."), this, InputPin);
				}
			}
			CompileInputs.Add(CompiledInput);
		}
	}

	UNiagaraGraph* Graph = Cast<UNiagaraGraph>(GetGraph());
	// By this point, we've visited all of our child nodes in the call graph. We can mine them to find out everyone contributing to the parameter map (and when).
	if (GetInputPin(0) != nullptr && GetInputPin(0)->LinkedTo.Num() > 0)
	{
		Translator->ParameterMapGet(this, CompileInputs, Outputs);
	}
}


bool UNiagaraNodeParameterMapGet::CommitEditablePinName(const FText& InName, UEdGraphPin* InGraphPinObj) 
{
	if (Pins.Contains(InGraphPinObj) && InGraphPinObj->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		FScopedTransaction AddNewPinTransaction(LOCTEXT("Rename Pin", "Renamed pin"));
		Modify();
		InGraphPinObj->Modify();

		FString OldPinName = InGraphPinObj->PinName.ToString();
		InGraphPinObj->PinName = *InName.ToString();

		OnPinRenamed(InGraphPinObj, OldPinName);

		return true;
	}
	return false;
}

void UNiagaraNodeParameterMapGet::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	UNiagaraNodeParameterMapBase::GetContextMenuActions(Context);

	UEdGraphPin* Pin = const_cast<UEdGraphPin*>(Context.Pin);
	if (Pin && Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		
		FNiagaraVariable Var = CastChecked<UEdGraphSchema_Niagara>(GetSchema())->PinToNiagaraVariable(Pin);
		const UNiagaraGraph* Graph = GetNiagaraGraph();

		if (!FNiagaraConstants::IsNiagaraConstant(Var))
		{
			Context.MenuBuilder->BeginSection("EdGraphSchema_NiagaraMetaDataActions", LOCTEXT("EditPinMenuHeader", "Meta-Data"));
			TSharedRef<SWidget> RenameWidget =
				SNew(SBox)
				.WidthOverride(100)
				.Padding(FMargin(5, 0, 0, 0))
				[
					SNew(SEditableTextBox)
					.Text_UObject(this, &UNiagaraNodeParameterMapBase::GetPinDescriptionText, Pin)
					.OnTextCommitted_UObject(this, &UNiagaraNodeParameterMapBase::PinDescriptionTextCommitted, Pin)
				];
			Context.MenuBuilder->AddWidget(RenameWidget, LOCTEXT("DescMenuItem", "Description"));
			Context.MenuBuilder->EndSection();
		}
	}
}

#undef LOCTEXT_NAMESPACE
