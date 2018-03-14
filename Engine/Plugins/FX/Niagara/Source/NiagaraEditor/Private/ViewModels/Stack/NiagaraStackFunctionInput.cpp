// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackFunctionInput.h"
#include "NiagaraStackFunctionInputCollection.h"
#include "NiagaraStackObject.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeAssignment.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraSystemViewModel.h"
#include "NiagaraSystemScriptViewModel.h"
#include "NiagaraEmitterHandleViewModel.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraStackEditorData.h"
#include "NiagaraComponent.h"
#include "NiagaraScriptSource.h"
#include "NiagaraConstants.h"
#include "NiagaraStackGraphUtilities.h"
#include "NiagaraScriptMergeManager.h"

#include "ScopedTransaction.h"
#include "Editor.h"
#include "UObject/StructOnScope.h"
#include "AssetRegistryModule.h"
#include "ARFilter.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "NiagaraStackViewModel"

UNiagaraStackFunctionInput::UNiagaraStackFunctionInput()
	: OwningModuleNode(nullptr)
	, OwningFunctionCallNode(nullptr)
	, bCanBePinned(true)
	, bUpdatingGraphDirectly(false)
{
}

void UNiagaraStackFunctionInput::BeginDestroy()
{
	Super::BeginDestroy();
	if (OwningFunctionCallNode.IsValid())
	{
		OwningFunctionCallNode->GetGraph()->RemoveOnGraphChangedHandler(GraphChangedHandle);

		if (SourceScript.IsValid())
		{
			SourceScript->RapidIterationParameters.RemoveOnChangedHandler(RapidIterationParametersChangedHandle);
			SourceScript->GetSource()->OnChanged().RemoveAll(this);
		}
	}
}

void UNiagaraStackFunctionInput::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UNiagaraStackFunctionInput* This = Cast<UNiagaraStackFunctionInput>(InThis);
	if (This != nullptr)
	{
		This->AddReferencedObjects(Collector);
	}
	Super::AddReferencedObjects(InThis, Collector);
}

void UNiagaraStackFunctionInput::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (InputValues.DataObjects.IsValid() && InputValues.DataObjects.GetDefaultValueOwner() == FDataValues::EDefaultValueOwner::LocallyOwned)
	{
		Collector.AddReferencedObject(InputValues.DataObjects.GetDefaultValueObjectRef(), this);
	}
}

int32 UNiagaraStackFunctionInput::GetItemIndentLevel() const
{
	return ItemIndentLevel;
}

void UNiagaraStackFunctionInput::SetItemIndentLevel(int32 InItemIndentLevel)
{
	ItemIndentLevel = InItemIndentLevel;
}

// Traverses the path between the owning module node and the function call node this input belongs too collecting up the input handles between them.
void GenerateInputParameterHandlePath(UNiagaraNodeFunctionCall& ModuleNode, UNiagaraNodeFunctionCall& FunctionCallNode, TArray<FNiagaraParameterHandle>& OutHandlePath)
{
	UNiagaraNodeFunctionCall* CurrentFunctionCallNode = &FunctionCallNode;
	while (CurrentFunctionCallNode != &ModuleNode)
	{
		TArray<UEdGraphPin*> FunctionOutputPins;
		CurrentFunctionCallNode->GetOutputPins(FunctionOutputPins);
		if (ensureMsgf(FunctionOutputPins.Num() == 1 && FunctionOutputPins[0]->LinkedTo.Num() == 1 && FunctionOutputPins[0]->LinkedTo[0]->GetOwningNode()->IsA<UNiagaraNodeParameterMapSet>(),
			TEXT("Invalid Stack Graph - Dynamic Input Function call didn't have a valid connected output.")))
		{
			FNiagaraParameterHandle AliasedHandle(FunctionOutputPins[0]->LinkedTo[0]->PinName);
			OutHandlePath.Add(FNiagaraParameterHandle::CreateModuleParameterHandle(AliasedHandle.GetName()));
			UNiagaraNodeParameterMapSet* NextOverrideNode = CastChecked<UNiagaraNodeParameterMapSet>(FunctionOutputPins[0]->LinkedTo[0]->GetOwningNode());
			UEdGraphPin* NextOverrideNodeOutputPin = FNiagaraStackGraphUtilities::GetParameterMapOutputPin(*NextOverrideNode);
			
			TArray<UNiagaraNodeFunctionCall*> NextFunctionCallNodes;
			for (UEdGraphPin* NextOverrideNodeOutputPinLinkedPin : NextOverrideNodeOutputPin->LinkedTo)
			{
				UNiagaraNodeFunctionCall* NextFunctionCallNode = Cast<UNiagaraNodeFunctionCall>(NextOverrideNodeOutputPinLinkedPin->GetOwningNode());
				if (NextFunctionCallNode != nullptr)
				{
					NextFunctionCallNodes.Add(NextFunctionCallNode);
				}
			}

			if (ensureMsgf(NextFunctionCallNodes.Num() == 1, TEXT("Invalid Stack Graph - Override node not corrected to a single function call node.")))
			{
				CurrentFunctionCallNode = NextFunctionCallNodes[0];
			}
			else
			{
				OutHandlePath.Empty();
				return;
			}
		}
		else
		{
			OutHandlePath.Empty();
			return;
		}
	}
}

void UNiagaraStackFunctionInput::Initialize(
	TSharedRef<FNiagaraSystemViewModel> InSystemViewModel,
	TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel,
	UNiagaraStackEditorData& InStackEditorData,
	UNiagaraNodeFunctionCall& InModuleNode,
	UNiagaraNodeFunctionCall& InInputFunctionCallNode,
	FName InInputParameterHandle,
	FNiagaraTypeDefinition InInputType)
{
	checkf(OwningModuleNode.IsValid() == false && OwningFunctionCallNode.IsValid() == false, TEXT("Can only initialize once."));
	Super::Initialize(InSystemViewModel, InEmitterViewModel);
	StackEditorData = &InStackEditorData;
	OwningModuleNode = &InModuleNode;
	OwningFunctionCallNode = &InInputFunctionCallNode;
	OwningAssignmentNode = Cast<UNiagaraNodeAssignment>(OwningFunctionCallNode.Get());

	UNiagaraNodeOutput* OutputNode = FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(*OwningModuleNode.Get());
	UNiagaraEmitter* ParentEmitter = InEmitterViewModel->GetEmitter();
	UNiagaraSystem* ParentSystem = &InSystemViewModel->GetSystem();
	if (OutputNode)
	{
		TArray<UNiagaraScript*> Scripts;
		if (ParentEmitter != nullptr)
		{
			ParentEmitter->GetScripts(Scripts, false);
		}
		if (ParentSystem != nullptr)
		{
			Scripts.Add(ParentSystem->GetSystemSpawnScript(true));
			Scripts.Add(ParentSystem->GetSystemUpdateScript(true));
			Scripts.Add(ParentSystem->GetSystemSpawnScript(false));
			Scripts.Add(ParentSystem->GetSystemUpdateScript(false));
		}

		for (UNiagaraScript* Script : Scripts)
		{
			if (OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleEventScript)
			{
				if (Script->GetUsage() == ENiagaraScriptUsage::ParticleEventScript && Script->GetUsageId() == OutputNode->GetUsageId())
				{
					AffectedScripts.Add(Script);
					break;
				}
			}
			else if (Script->ContainsUsage(OutputNode->GetUsage()))
			{
				AffectedScripts.Add(Script);
			}
		}

		for (TWeakObjectPtr<UNiagaraScript> AffectedScript : AffectedScripts)
		{
			if (AffectedScript.IsValid() && AffectedScript->IsEquivalentUsage(OutputNode->GetUsage()) && AffectedScript->GetUsageId() == OutputNode->GetUsageId())
			{
				SourceScript = AffectedScript;
				RapidIterationParametersChangedHandle = SourceScript->RapidIterationParameters.AddOnChangedHandler(
					FNiagaraParameterStore::FOnChanged::FDelegate::CreateUObject(this, &UNiagaraStackFunctionInput::OnRapidIterationParametersChanged));
				SourceScript->GetSource()->OnChanged().AddUObject(this, &UNiagaraStackFunctionInput::OnScriptSourceChanged);
				break;
			}
		}
	}

	checkf(SourceScript.IsValid(), TEXT("Coudn't find source script in affected scripts."));

	GraphChangedHandle = OwningFunctionCallNode->GetGraph()->AddOnGraphChangedHandler(
		FOnGraphChanged::FDelegate::CreateUObject(this, &UNiagaraStackFunctionInput::OnGraphChanged));

	InputParameterHandle = FNiagaraParameterHandle(InInputParameterHandle);
	GenerateInputParameterHandlePath(*OwningModuleNode, *OwningFunctionCallNode, InputParameterHandlePath);
	InputParameterHandlePath.Add(InputParameterHandle);

	DisplayName = FText::FromName(InputParameterHandle.GetName());
	AliasedInputParameterHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputParameterHandle, OwningFunctionCallNode.Get());

	InputType = InInputType;
	StackEditorDataKey = FNiagaraStackGraphUtilities::GenerateStackFunctionInputEditorDataKey(*OwningFunctionCallNode.Get(), InputParameterHandle);

	/*
	UE_LOG(LogNiagaraEditor, Log, TEXT("Scripts for Parameter '%s'"), *RapidIterationParameterName);
	for (UNiagaraScript* Script : AffectedScripts)
	{
		UE_LOG(LogNiagaraEditor, Log, TEXT("Added... '%s'"), *Script->GetFullName());
	}
	*/

}

const UNiagaraNodeFunctionCall& UNiagaraStackFunctionInput::GetInputFunctionCallNode() const
{
	return *OwningFunctionCallNode.Get();
}

UNiagaraStackFunctionInput::EValueMode UNiagaraStackFunctionInput::GetValueMode()
{
	return InputValues.Mode;
}

bool UNiagaraStackFunctionInput::GetCanBePinned() const
{
	return OwningFunctionCallNode.IsValid() && OwningFunctionCallNode.Get()->FunctionScript->Usage == ENiagaraScriptUsage::Module;
}

const FNiagaraTypeDefinition& UNiagaraStackFunctionInput::GetInputType() const
{
	return InputType;
}

FText UNiagaraStackFunctionInput::GetTooltipText() const
{
	return GetTooltipText(InputValues.Mode);
}

FText UNiagaraStackFunctionInput::GetTooltipText(EValueMode InValueMode) const
{
	FNiagaraVariable ValueVariable;
	UNiagaraGraph* NodeGraph = nullptr;

	if (InValueMode == EValueMode::Linked)
	{
		UEdGraphPin* OverridePin = GetOverridePin();
		UEdGraphPin* ValuePin = OverridePin != nullptr ? OverridePin : GetDefaultPin();
		ValueVariable = FNiagaraVariable(InputType, InputValues.LinkedHandle.GetParameterHandleString());
		if (ValuePin != nullptr)
		{
			NodeGraph = Cast<UNiagaraGraph>(ValuePin->GetOwningNode()->GetGraph());
		}
	}
	else
	{
		ValueVariable = FNiagaraVariable(InputType, InputParameterHandle.GetParameterHandleString());
		if (OwningFunctionCallNode.IsValid() && OwningFunctionCallNode->FunctionScript != nullptr)
		{
			UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(OwningFunctionCallNode->FunctionScript->GetSource());
			const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
			NodeGraph = Source->NodeGraph;
		}
	}

	const FNiagaraVariableMetaData* MetaData = nullptr;
	if (FNiagaraConstants::IsNiagaraConstant(ValueVariable))
	{
		MetaData = FNiagaraConstants::GetConstantMetaData(ValueVariable);
	}
	else if (NodeGraph != nullptr)
	{
		MetaData = NodeGraph->GetMetaData(ValueVariable);
	}

	if (MetaData != nullptr)
	{
		return MetaData->Description;
	}

	return FText::FromName(ValueVariable.GetName());
}

void UNiagaraStackFunctionInput::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	RapidIterationParameterName = CreateRapidIterationVariableName(AliasedInputParameterHandle.GetParameterHandleString().ToString());
	RefreshValues();

	if (InputValues.Mode == EValueMode::Dynamic && InputValues.DynamicNode.IsValid())
	{
		UNiagaraStackFunctionInputCollection* DynamicInputEntry = FindCurrentChildOfTypeByPredicate<UNiagaraStackFunctionInputCollection>(CurrentChildren,
			[=](UNiagaraStackFunctionInputCollection* CurrentFunctionInputEntry) 
		{ 
			return CurrentFunctionInputEntry->GetInputFunctionCallNode() == InputValues.DynamicNode.Get() &&
				CurrentFunctionInputEntry->GetModuleNode() == OwningModuleNode.Get(); 
		});

		if (DynamicInputEntry == nullptr)
		{ 
			UNiagaraStackFunctionInputCollection::FDisplayOptions DisplayOptions;
			DisplayOptions.DisplayName = LOCTEXT("DynamicInputCollecitonDisplayName", "Dynamic Input Function Input");
			DisplayOptions.bShouldShowInStack = false;
			DisplayOptions.ChildItemIndentLevel = ItemIndentLevel + 1;

			DynamicInputEntry = NewObject<UNiagaraStackFunctionInputCollection>(this);
			DynamicInputEntry->Initialize(GetSystemViewModel(), GetEmitterViewModel(), *StackEditorData, *OwningModuleNode, *InputValues.DynamicNode.Get(), DisplayOptions);
		}
		NewChildren.Add(DynamicInputEntry);
	}

	if (InputValues.Mode == EValueMode::Data && InputValues.DataObjects.GetValueObject() != nullptr)
	{
		UNiagaraStackObject* ValueObjectEntry = FindCurrentChildOfTypeByPredicate<UNiagaraStackObject>(CurrentChildren,
			[=](UNiagaraStackObject* CurrentObjectEntry) { return CurrentObjectEntry->GetObject() == InputValues.DataObjects.GetValueObject(); });

		if(ValueObjectEntry == nullptr)
		{
			ValueObjectEntry = NewObject<UNiagaraStackObject>(this);
			ValueObjectEntry->Initialize(GetSystemViewModel(), GetEmitterViewModel(), InputValues.DataObjects.GetValueObject());
			ValueObjectEntry->SetItemIndentLevel(ItemIndentLevel + 1);
		}
		NewChildren.Add(ValueObjectEntry);
	}
}

TSharedPtr<FStructOnScope> UNiagaraStackFunctionInput::FInputValues::GetLocalStructToReuse()
{
	return Mode == EValueMode::Local ? LocalStruct : TSharedPtr<FStructOnScope>();
}

UNiagaraDataInterface* UNiagaraStackFunctionInput::FInputValues::GetDataDefaultValueObjectToReuse()
{
	return Mode == EValueMode::Data && DataObjects.IsValid() && DataObjects.GetDefaultValueOwner() == FDataValues::EDefaultValueOwner::LocallyOwned
		? DataObjects.GetDefaultValueObject()
		: nullptr;
}

void UNiagaraStackFunctionInput::RefreshValues()
{
	if (ensureMsgf(InputParameterHandle.IsModuleHandle(), TEXT("Function inputs can only be generated for module paramters.")) == false)
	{
		return;
	}

	FInputValues OldValues = InputValues;
	InputValues = FInputValues();

	UEdGraphPin* DefaultPin = GetDefaultPin();
	if (DefaultPin != nullptr)
	{
		UEdGraphPin* OverridePin = GetOverridePin();
		UEdGraphPin* ValuePin = OverridePin != nullptr ? OverridePin : DefaultPin;

		if (TryGetCurrentLocalValue(InputValues.LocalStruct, *ValuePin, OldValues.GetLocalStructToReuse()))
		{
			InputValues.Mode = EValueMode::Local;
		}
		else if (TryGetCurrentLinkedValue(InputValues.LinkedHandle, *ValuePin))
		{
			InputValues.Mode = EValueMode::Linked;
		}
		else if (TryGetCurrentDataValue(InputValues.DataObjects, OverridePin, *DefaultPin, OldValues.GetDataDefaultValueObjectToReuse()))
		{
			InputValues.Mode = EValueMode::Data;
		}
		else if (TryGetCurrentDynamicValue(InputValues.DynamicNode, OverridePin))
		{
			InputValues.Mode = EValueMode::Dynamic;
		}
	}

	bCanResetToBase.Reset();
	ValueChangedDelegate.Broadcast();
}

FText UNiagaraStackFunctionInput::GetDisplayName() const
{
	return DisplayName;
}

FName UNiagaraStackFunctionInput::GetTextStyleName() const
{
	return "NiagaraEditor.Stack.ParameterText";
}

bool UNiagaraStackFunctionInput::GetCanExpand() const
{
	return true;
}

const TArray<FNiagaraParameterHandle>& UNiagaraStackFunctionInput::GetInputParameterHandlePath() const
{
	return InputParameterHandlePath;
}

const FNiagaraParameterHandle& UNiagaraStackFunctionInput::GetInputParameterHandle() const
{
	return InputParameterHandle;
}

const FNiagaraParameterHandle& UNiagaraStackFunctionInput::GetLinkedValueHandle() const
{
	return InputValues.LinkedHandle;
}

void UNiagaraStackFunctionInput::SetLinkedValueHandle(const FNiagaraParameterHandle& InParameterHandle)
{
	if (InParameterHandle == InputValues.LinkedHandle)
	{
		return;
	}

	FScopedTransaction ScopedTransaction(LOCTEXT("UpdateLinkedInputValue", "Update linked input value"));
	UEdGraphPin& OverridePin = GetOrCreateOverridePin();
	RemoveNodesForOverridePin(OverridePin);
	if (IsRapidIterationCandidate())
	{
		RemoveRapidIterationParametersForAffectedScripts();
	}

	FNiagaraStackGraphUtilities::SetLinkedValueHandleForFunctionInput(OverridePin, InParameterHandle);
	FNiagaraStackGraphUtilities::RelayoutGraph(*OwningFunctionCallNode->GetGraph());

	RefreshValues();
}

bool UsageRunsBefore(ENiagaraScriptUsage UsageA, ENiagaraScriptUsage UsageB)
{
	static TArray<ENiagaraScriptUsage> UsagesOrderedByExecution
	{
		ENiagaraScriptUsage::SystemSpawnScript,
		ENiagaraScriptUsage::SystemUpdateScript,
		ENiagaraScriptUsage::EmitterSpawnScript,
		ENiagaraScriptUsage::EmitterUpdateScript,
		ENiagaraScriptUsage::ParticleSpawnScript,
		ENiagaraScriptUsage::ParticleUpdateScript
	};

	int32 IndexA;
	int32 IndexB;
	UsagesOrderedByExecution.Find(UsageA, IndexA);
	UsagesOrderedByExecution.Find(UsageB, IndexB);
	return IndexA < IndexB;
}

bool IsSpawnUsage(ENiagaraScriptUsage Usage)
{
	return
		Usage == ENiagaraScriptUsage::SystemSpawnScript ||
		Usage == ENiagaraScriptUsage::EmitterSpawnScript ||
		Usage == ENiagaraScriptUsage::ParticleSpawnScript;
}

FName GetNamespaceForUsage(ENiagaraScriptUsage Usage)
{
	switch (Usage)
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
		return NAME_None;
	}
}

void UNiagaraStackFunctionInput::GetAvailableParameterHandles(TArray<FNiagaraParameterHandle>& AvailableParameterHandles)
{
	// Engine Handles.
	for (const FNiagaraVariable& SystemVariable : FNiagaraConstants::GetEngineConstants())
	{
		if (SystemVariable.GetType() == InputType)
		{
			AvailableParameterHandles.Add(FNiagaraParameterHandle::CreateEngineParameterHandle(SystemVariable));
		}
	}

	UNiagaraNodeOutput* CurrentOutputNode = FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(*OwningModuleNode);

	TArray<UNiagaraNodeOutput*> AllOutputNodes;
	GetEmitterViewModel()->GetSharedScriptViewModel()->GetGraphViewModel()->GetGraph()->GetNodesOfClass<UNiagaraNodeOutput>(AllOutputNodes);
	if (GetSystemViewModel()->GetEditMode() == ENiagaraSystemViewModelEditMode::SystemAsset)
	{
		GetSystemViewModel()->GetSystemScriptViewModel()->GetGraphViewModel()->GetGraph()->GetNodesOfClass<UNiagaraNodeOutput>(AllOutputNodes);
	}

	TArray<FNiagaraVariable> ExposedVars;
	GetSystemViewModel()->GetSystem().GetExposedParameters().GetParameters(ExposedVars);
	for (const FNiagaraVariable& ExposedVar : ExposedVars)
	{
		if (ExposedVar.GetType() == InputType)
		{
			AvailableParameterHandles.Add(FNiagaraParameterHandle::CreateEngineParameterHandle(ExposedVar));
		}
	}

	for (UNiagaraNodeOutput* OutputNode : AllOutputNodes)
	{
		if (OutputNode == CurrentOutputNode || UsageRunsBefore(OutputNode->GetUsage(), CurrentOutputNode->GetUsage()))
		{
			TArray<FNiagaraParameterHandle> AvailableParameterHandlesForThisOutput;
			TArray<FNiagaraStackGraphUtilities::FStackNodeGroup> StackGroups;
			FNiagaraStackGraphUtilities::GetStackNodeGroups(*OutputNode, StackGroups);

			int32 CurrentModuleIndex = OutputNode == CurrentOutputNode
				? StackGroups.IndexOfByPredicate([=](const FNiagaraStackGraphUtilities::FStackNodeGroup Group) { return Group.EndNode == OwningModuleNode; })
				: INDEX_NONE;

			int32 MaxGroupIndex = CurrentModuleIndex != INDEX_NONE ? CurrentModuleIndex : StackGroups.Num() - 1;
			for (int32 i = 1; i < MaxGroupIndex; i++)
			{
				UNiagaraNodeFunctionCall* ModuleToCheck = Cast<UNiagaraNodeFunctionCall>(StackGroups[i].EndNode);
				FNiagaraParameterMapHistoryBuilder Builder;
				ModuleToCheck->BuildParameterMapHistory(Builder, false);

				if (Builder.Histories.Num() == 1)
				{
					for (int32 j = 0; j < Builder.Histories[0].Variables.Num(); j++)
					{
						FNiagaraVariable& Variable = Builder.Histories[0].Variables[j];
						if (Variable.GetType() == InputType)
						{
							TArray<const UEdGraphPin*>& WriteHistory = Builder.Histories[0].PerVariableWriteHistory[j];
							for (const UEdGraphPin* WritePin : WriteHistory)
							{
								if (Cast<UNiagaraNodeParameterMapSet>(WritePin->GetOwningNode()) != nullptr)
								{
									FNiagaraParameterHandle AvailableHandle = FNiagaraParameterHandle(Variable.GetName());
									AvailableParameterHandles.AddUnique(AvailableHandle);
									AvailableParameterHandlesForThisOutput.AddUnique(AvailableHandle);
									break;
								}
							}
						}
					}
				}
			}

			if (OutputNode != CurrentOutputNode && IsSpawnUsage(OutputNode->GetUsage()))
			{
				FName OutputNodeNamespace = GetNamespaceForUsage(OutputNode->GetUsage());
				if (OutputNodeNamespace.IsNone() == false)
				{
					for (FNiagaraParameterHandle& AvailableParameterHandleForThisOutput : AvailableParameterHandlesForThisOutput)
					{
						if (AvailableParameterHandleForThisOutput.GetNamespace() == OutputNodeNamespace)
						{
							AvailableParameterHandles.AddUnique(FNiagaraParameterHandle::CreateInitialParameterHandle(AvailableParameterHandleForThisOutput));
						}
					}
				}
			}
		}
	}
}

UNiagaraNodeFunctionCall* UNiagaraStackFunctionInput::GetDynamicInputNode() const
{
	return InputValues.DynamicNode.Get();
}

void UNiagaraStackFunctionInput::GetAvailableDynamicInputs(TArray<UNiagaraScript*>& AvailableDynamicInputs)
{
	UEnum* NiagaraScriptUsageEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ENiagaraScriptUsage"), true);
	FString QualifiedDynamicInputUsageString = NiagaraScriptUsageEnum->GetNameStringByValue(static_cast<uint8>(ENiagaraScriptUsage::DynamicInput));
	int32 LastColonIndex;
	QualifiedDynamicInputUsageString.FindLastChar(TEXT(':'), LastColonIndex);
	FString UnqualifiedDynamicInputUsageString = QualifiedDynamicInputUsageString.RightChop(LastColonIndex + 1);

	FARFilter DynamicInputFilter;
	DynamicInputFilter.ClassNames.Add(UNiagaraScript::StaticClass()->GetFName());
	DynamicInputFilter.TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(UNiagaraScript, Usage), UnqualifiedDynamicInputUsageString);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> DynamicInputAssets;
	AssetRegistryModule.Get().GetAssets(DynamicInputFilter, DynamicInputAssets);

	for (const FAssetData& DynamicInputAsset : DynamicInputAssets)
	{
		UNiagaraScript* DynamicInputScript = Cast<UNiagaraScript>(DynamicInputAsset.GetAsset());
		if (DynamicInputScript != nullptr)
		{
			UNiagaraScriptSource* DynamicInputScriptSource = Cast<UNiagaraScriptSource>(DynamicInputScript->GetSource());
			TArray<UNiagaraNodeOutput*> OutputNodes;
			DynamicInputScriptSource->NodeGraph->GetNodesOfClass<UNiagaraNodeOutput>(OutputNodes);
			if (OutputNodes.Num() == 1)
			{
				TArray<UEdGraphPin*> InputPins;
				OutputNodes[0]->GetInputPins(InputPins);
				if (InputPins.Num() == 1)
				{
					const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
					FNiagaraTypeDefinition PinType = NiagaraSchema->PinToTypeDefinition(InputPins[0]);
					if (PinType == InputType)
					{
						AvailableDynamicInputs.Add(DynamicInputScript);
					}
				}
			}
		}
	}
}

void UNiagaraStackFunctionInput::SetDynamicInput(UNiagaraScript* DynamicInput)
{
	FScopedTransaction ScopedTransaction(LOCTEXT("SetDynamicInput", "Make dynamic input"));

	UEdGraphPin& OverridePin = GetOrCreateOverridePin();
	RemoveNodesForOverridePin(OverridePin);
	if (IsRapidIterationCandidate())
	{
		RemoveRapidIterationParametersForAffectedScripts();
	}

	UNiagaraNodeFunctionCall* FunctionCallNode;
	FNiagaraStackGraphUtilities::SetDynamicInputForFunctionInput(OverridePin, DynamicInput, FunctionCallNode);
	FNiagaraStackGraphUtilities::InitializeDataInterfaceInputs(GetSystemViewModel(), GetEmitterViewModel(), *StackEditorData, *OwningModuleNode, *FunctionCallNode);
	FNiagaraStackGraphUtilities::RelayoutGraph(*OwningFunctionCallNode->GetGraph());

	RefreshChildren();
}

TSharedPtr<FStructOnScope> UNiagaraStackFunctionInput::GetLocalValueStruct()
{
	return InputValues.LocalStruct;
}

UNiagaraDataInterface* UNiagaraStackFunctionInput::GetDataValueObject()
{
	return InputValues.DataObjects.GetValueObject();
}

bool UNiagaraStackFunctionInput::GetIsPinned() const
{
	return StackEditorData->GetModuleInputIsPinned(StackEditorDataKey);
}

void UNiagaraStackFunctionInput::SetIsPinned(bool bIsPinned)
{
	StackEditorData->SetModuleInputIsPinned(StackEditorDataKey, bIsPinned);
	PinnedChangedDelegate.Broadcast();
}

void UNiagaraStackFunctionInput::NotifyBeginLocalValueChange()
{
	GEditor->BeginTransaction(LOCTEXT("BeginEditModuleInputLocalValue", "Edit input local value."));
}

void UNiagaraStackFunctionInput::NotifyEndLocalValueChange()
{
	if (GEditor->IsTransactionActive())
	{
		GEditor->EndTransaction();
	}
}

bool UNiagaraStackFunctionInput::IsRapidIterationCandidate() const
{
	return InputType != FNiagaraTypeDefinition::GetBoolDef() && !InputType.IsEnum() &&
		InputType != FNiagaraTypeDefinition::GetParameterMapDef() && !InputType.IsDataInterface();
}

void UNiagaraStackFunctionInput::SetLocalValue(TSharedRef<FStructOnScope> InLocalValue)
{
	UEdGraphPin* DefaultPin = GetDefaultPin();
	UEdGraphPin* OverridePin = GetOverridePin();
	UEdGraphPin* ValuePin = DefaultPin;
	
	// If the default pin in the function graph is connected internally, rapid iteration parameters can't be used since
	// the compilation currently won't use them.
	bool bCanUseRapidIterationParameter = IsRapidIterationCandidate() && DefaultPin->LinkedTo.Num() == 0;
	if (bCanUseRapidIterationParameter == false)
	{
		ValuePin = OverridePin != nullptr ? OverridePin : DefaultPin;
	}

	TSharedPtr<FStructOnScope> CurrentValue;
	bool bCanHaveLocalValue = ValuePin != nullptr;
	bool bHasLocalValue = bCanHaveLocalValue && InputValues.Mode == EValueMode::Local && TryGetCurrentLocalValue(CurrentValue, *ValuePin, TSharedPtr<FStructOnScope>());
	bool bLocalValueMatchesSetValue = bHasLocalValue && FNiagaraEditorUtilities::DataMatches(*CurrentValue.Get(), InLocalValue.Get());

	if (bCanHaveLocalValue == false || bLocalValueMatchesSetValue)
	{
		return;
	}

	FScopedTransaction ScopedTransaction(LOCTEXT("UpdateInputLocalValue", "Update input local value"));
	UNiagaraGraph* EmitterGraph = Cast<UNiagaraGraph>(OwningFunctionCallNode->GetGraph());

	bool bGraphWillNeedRelayout = false;
	if (OverridePin != nullptr && OverridePin->LinkedTo.Num() > 0)
	{
		RemoveNodesForOverridePin(*OverridePin);
		bGraphWillNeedRelayout = true;
	}

	const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	if (bCanUseRapidIterationParameter)
	{
		for (TWeakObjectPtr<UNiagaraScript> Script : AffectedScripts)
		{
			Script->Modify();
		}

		// If there is currently an override, we need to get rid of it.
		if (OverridePin != nullptr)
		{
			UNiagaraNode* OverrideNode = CastChecked<UNiagaraNode>(OverridePin->GetOwningNode());
			OverrideNode->Modify();
			OverrideNode->RemovePin(OverridePin);
		}
	
		FNiagaraVariable ValueVariable = NiagaraSchema->PinToNiagaraVariable(ValuePin, true);
		ValueVariable.SetName(*RapidIterationParameterName);

		for (TWeakObjectPtr<UNiagaraScript> Script : AffectedScripts)
		{
			int32 FoundOffset = Script->RapidIterationParameters.IndexOf(ValueVariable);
			checkSlow(!ValueVariable.IsDataInterface());
			if (FoundOffset == INDEX_NONE)
			{
				Script->RapidIterationParameters.AddParameter(ValueVariable);
				UE_LOG(LogNiagaraEditor, Log, TEXT("Adding Parameter %s to Script %s"), *ValueVariable.GetName().ToString(), *Script->GetFullName());
			}
			Script->RapidIterationParameters.SetParameterData(InLocalValue->GetStructMemory(), ValueVariable);
		}
		GetSystemViewModel()->ReInitializeSystemInstances();
	}
	else 
	{
		FNiagaraVariable LocalValueVariable(InputType, NAME_None);
		LocalValueVariable.SetData(InLocalValue->GetStructMemory());
		FString PinDefaultValue;
		if(ensureMsgf(NiagaraSchema->TryGetPinDefaultValueFromNiagaraVariable(LocalValueVariable, PinDefaultValue),
			TEXT("Could not generate default value string for non-rapid iteration parameter.")))
		{
			if (OverridePin == nullptr)
			{
				OverridePin = &GetOrCreateOverridePin();
				bGraphWillNeedRelayout = true;
			}

			OverridePin->Modify();
			OverridePin->DefaultValue = PinDefaultValue;
			EmitterGraph->NotifyGraphNeedsRecompile();
		}
	}
	
	if (bGraphWillNeedRelayout)
	{
		FNiagaraStackGraphUtilities::RelayoutGraph(*EmitterGraph);
	}

	RefreshValues();
}

bool UNiagaraStackFunctionInput::CanReset() const
{
	if(InputValues.Mode == EValueMode::Data)
	{
		// For data values a copy of the default object should have been created automatically and attached to the override pin for this input.  If a 
		// copy of the default object wasn't created, the input can be reset to create one.  If a copy of the data object is available it can be
		// reset if it's different from it's default value.
		bool bHasDataValueObject = InputValues.DataObjects.GetValueObject() != nullptr;
		bool bHasDefaultDataValueObject = InputValues.DataObjects.GetDefaultValueObject() != nullptr;
		bool bIsDataValueDifferentFromDefaultDataValue = bHasDataValueObject && bHasDefaultDataValueObject 
			&& InputValues.DataObjects.GetValueObject()->Equals(InputValues.DataObjects.GetDefaultValueObject()) == false;
		return bHasDataValueObject == false || bHasDefaultDataValueObject == false || bIsDataValueDifferentFromDefaultDataValue;
	}
	else
	{
		if (IsRapidIterationCandidate())
		{
			if (GetOverridePin() != nullptr)
			{
				return true;
			}
			else if (InputValues.LocalStruct.IsValid() && !FNiagaraEditorUtilities::DataMatches(GetDefaultValue(), *(InputValues.LocalStruct.Get())))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// All other input modes can be reset if there is an override pin available.
			return GetOverridePin() != nullptr;
		}
	}
}

FNiagaraVariable UNiagaraStackFunctionInput::GetDefaultValue() const
{
	FNiagaraVariable Var;
	UEdGraphPin* DefaultPin = GetDefaultPin();
	if (DefaultPin != nullptr)
	{
		const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
		Var = NiagaraSchema->PinToNiagaraVariable(DefaultPin, true);
		Var.SetName(*RapidIterationParameterName);
	}
	return Var;
}

bool UNiagaraStackFunctionInput::UpdateRapidIterationParametersForAffectedScripts(const uint8* Data)
{
	for (TWeakObjectPtr<UNiagaraScript> Script : AffectedScripts)
	{
		Script->Modify();
	}

	FNiagaraVariable Var(InputType, *RapidIterationParameterName);

	for (TWeakObjectPtr<UNiagaraScript> Script : AffectedScripts)
	{
		int32 FoundOffset = Script->RapidIterationParameters.IndexOf(Var);
		checkSlow(!Var.IsDataInterface());
		if (FoundOffset == INDEX_NONE)
		{
			Script->RapidIterationParameters.AddParameter(Var);
			UE_LOG(LogNiagaraEditor, Log, TEXT("Adding Parameter %s to Script %s"), *Var.GetName().ToString(), *Script->GetFullName());

		}
		Script->RapidIterationParameters.SetParameterData(Data, Var);
	}
	GetSystemViewModel()->ResetSystem();
	return true;
}

bool UNiagaraStackFunctionInput::RemoveRapidIterationParametersForAffectedScripts()
{
	for (TWeakObjectPtr<UNiagaraScript> Script : AffectedScripts)
	{
		Script->Modify();
	}

	FNiagaraVariable Var(InputType, *RapidIterationParameterName);
	for (TWeakObjectPtr<UNiagaraScript> Script : AffectedScripts)
	{
		int32 FoundOffset = Script->RapidIterationParameters.IndexOf(Var);
		if (FoundOffset == INDEX_NONE)
		{
			return false;
		}

		UE_LOG(LogNiagaraEditor, Log, TEXT("Removed Var '%s' from Script %s"), *Var.GetName().ToString(), *Script->GetFullName());

		Script->RapidIterationParameters.RemoveParameter(Var);
	}
	return true;
}

void UNiagaraStackFunctionInput::Reset()
{
	if(InputValues.Mode == EValueMode::Data)
	{
		// For data values they are reset by making sure the data object owned by this input matches the default
		// data object.  If there is no data object owned by the input, one is created and and updated to match the default.
		FScopedTransaction ScopedTransaction(LOCTEXT("ResetInputObjectTransaction", "Reset the inputs data interface object to default."));
		if (InputValues.DataObjects.GetValueObject() != nullptr && InputValues.DataObjects.GetDefaultValueObject() != nullptr)
		{
			InputValues.DataObjects.GetDefaultValueObject()->CopyTo(InputValues.DataObjects.GetValueObject());
		}
		else
		{
			UEdGraphPin& OverridePin = GetOrCreateOverridePin();
			RemoveNodesForOverridePin(OverridePin);

			FString InputNodeName = InputParameterHandlePath[0].GetName().ToString();
			for (int32 i = 1; i < InputParameterHandlePath.Num(); i++)
			{
				InputNodeName += "." + InputParameterHandlePath[i].GetName().ToString();
			}

			UNiagaraDataInterface* InputValueObject;
			FNiagaraStackGraphUtilities::SetDataValueObjectForFunctionInput(OverridePin, const_cast<UClass*>(InputType.GetClass()), InputNodeName, InputValueObject);
			if (InputValues.DataObjects.GetDefaultValueObject() != nullptr)
			{
				InputValues.DataObjects.GetDefaultValueObject()->CopyTo(InputValueObject);
			}

			FNiagaraStackGraphUtilities::RelayoutGraph(*OwningFunctionCallNode->GetGraph());
		}
	}
	else
	{
		// For all other value modes removing the nodes connected to the override pin resets them.
		UNiagaraNodeParameterMapSet* OverrideNode = GetOverrideNode();
		UEdGraphPin* OverridePin = GetOverridePin();
		bool bGraphNeedsRecompile = false;
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("ResetInputStructTransaction", "Reset the inputs value to default."));
			
			if (IsRapidIterationCandidate())
			{
				if (OverrideNode != nullptr && OverridePin != nullptr)
				{
					RemoveNodesForOverridePin(*OverridePin);
					OverrideNode->Modify();
					OverrideNode->RemovePin(OverridePin);
					bGraphNeedsRecompile = true;
				}

				// Get the default value of the graph pin and use that to reset the rapid iteration variables...
				UEdGraphPin* DefaultPin = GetDefaultPin();
				if (DefaultPin != nullptr)
				{
					FNiagaraVariable DefaultVar = GetDefaultValue();
					UpdateRapidIterationParametersForAffectedScripts(DefaultVar.GetData());
				}
			}
			else
			{
				if (ensureMsgf(OverrideNode != nullptr && OverridePin != nullptr, TEXT("Can not reset the value of an input that doesn't have a valid override node and override pin")))
				{
					RemoveNodesForOverridePin(*OverridePin);
					OverrideNode->Modify();
					OverrideNode->RemovePin(OverridePin);
					bGraphNeedsRecompile =  true;
				}
			}

			if (bGraphNeedsRecompile)
			{
				OwningFunctionCallNode->GetNiagaraGraph()->NotifyGraphNeedsRecompile();
				FNiagaraStackGraphUtilities::RelayoutGraph(*OwningFunctionCallNode->GetGraph());
			}
		}
	}
	RefreshChildren();
}

bool UNiagaraStackFunctionInput::EmitterHasBase() const
{
	return GetSystemViewModel()->GetEditMode() == ENiagaraSystemViewModelEditMode::SystemAsset;
}

bool UNiagaraStackFunctionInput::CanResetToBase() const
{
	if (EmitterHasBase())
	{
		if (bCanResetToBase.IsSet() == false)
		{
			bool bIsModuleInput = OwningFunctionCallNode == OwningModuleNode;
			if (bIsModuleInput)
			{
				TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();

				UNiagaraNodeOutput* OutputNode = FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(*OwningFunctionCallNode.Get());
				if(MergeManager->IsMergeableScriptUsage(OutputNode->GetUsage()))
				{
					const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*GetEmitterViewModel()->GetEmitter(), GetSystemViewModel()->GetSystem());

					bCanResetToBase = BaseEmitter != nullptr && MergeManager->IsModuleInputDifferentFromBase(
						*GetEmitterViewModel()->GetEmitter(),
						*BaseEmitter,
						OutputNode->GetUsage(),
						OutputNode->GetUsageId(),
						OwningModuleNode->NodeGuid,
						InputParameterHandle.GetName().ToString());
				}
				else
				{
					bCanResetToBase = false;
				}
			}
			else
			{
				bCanResetToBase = false;
			}
		}
		return bCanResetToBase.GetValue();
	}
	return false;
}

void UNiagaraStackFunctionInput::ResetToBase()
{
	if (CanResetToBase())
	{
		TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();

		TSharedPtr<FNiagaraEmitterHandleViewModel> ThisEmitterHandleViewModel;
		for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : GetSystemViewModel()->GetEmitterHandleViewModels())
		{
			if (EmitterHandleViewModel->GetEmitterViewModel() == GetEmitterViewModel())
			{
				ThisEmitterHandleViewModel = EmitterHandleViewModel;
				break;
			}
		}

		const UNiagaraEmitter* BaseEmitter = ThisEmitterHandleViewModel->GetEmitterHandle()->GetSource();
		UNiagaraNodeOutput* OutputNode = FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(*OwningFunctionCallNode.Get());

		FScopedTransaction ScopedTransaction(LOCTEXT("ResetInputToBaseTransaction", "Reset this input to match the parent emitter."));
		FNiagaraScriptMergeManager::FApplyDiffResults Results = MergeManager->ResetModuleInputToBase(
			*GetEmitterViewModel()->GetEmitter(),
			*BaseEmitter,
			OutputNode->GetUsage(),
			OutputNode->GetUsageId(),
			OwningModuleNode->NodeGuid,
			InputParameterHandle.GetName().ToString());

		if (Results.bSucceeded)
		{
			// If resetting to the base succeeded, an unknown number of rapid iteration parameters may have been added.  To fix
			// this copy all of the owning scripts rapid iteration parameters to all other affected scripts.
			// TODO: Either the merge should take care of this directly, or at least provide more information about what changed.
			UNiagaraScript* OwningScript = GetEmitterViewModel()->GetEmitter()->GetScript(OutputNode->GetUsage(), OutputNode->GetUsageId());
			TArray<FNiagaraVariable> OwningScriptRapidIterationParameters;
			OwningScript->RapidIterationParameters.GetParameters(OwningScriptRapidIterationParameters);
			if (OwningScriptRapidIterationParameters.Num() > 0)
			{
				for (TWeakObjectPtr<UNiagaraScript> AffectedScript : AffectedScripts)
				{
					if (AffectedScript.Get() != OwningScript)
					{
						AffectedScript->Modify();
						for (FNiagaraVariable& OwningScriptRapidIterationParameter : OwningScriptRapidIterationParameters)
						{
							AffectedScript->RapidIterationParameters.AddParameter(OwningScriptRapidIterationParameter);
							AffectedScript->RapidIterationParameters.SetParameterData(
								OwningScript->RapidIterationParameters.GetParameterData(OwningScriptRapidIterationParameter), OwningScriptRapidIterationParameter);
						}
					}
				}
			}
		}
		RefreshChildren();
	}
}

FString UNiagaraStackFunctionInput::CreateRapidIterationVariableName(const FString& InName)
{
	UNiagaraNodeOutput* OutputNode = FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(*OwningModuleNode.Get());
	UNiagaraEmitter* ParentEmitter = GetEmitterViewModel()->GetEmitter();
	UNiagaraSystem* ParentSystem = &GetSystemViewModel()->GetSystem();
	FNiagaraVariable Temp(InputType, *InName);
	if (OutputNode->GetUsage() == ENiagaraScriptUsage::SystemSpawnScript || OutputNode->GetUsage() == ENiagaraScriptUsage::SystemUpdateScript)
	{
		Temp = FNiagaraParameterMapHistory::ConvertVariableToRapidIterationConstantName(Temp, nullptr); // These names *should* have the emitter baked in...
	}
	else
	{
		Temp = FNiagaraParameterMapHistory::ConvertVariableToRapidIterationConstantName(Temp, *ParentEmitter->GetUniqueEmitterName());
	}

	return Temp.GetName().ToString();
}


bool UNiagaraStackFunctionInput::CanRenameInput() const
{
	// Only module level assignment node inputs can be renamed.
	return OwningAssignmentNode.IsValid() && InputParameterHandlePath.Num() == 1;
}

bool UNiagaraStackFunctionInput::GetIsRenamePending() const
{
	return CanRenameInput() && StackEditorData->GetModuleInputIsRenamePending(StackEditorDataKey);
}

void UNiagaraStackFunctionInput::SetIsRenamePending(bool bIsRenamePending)
{
	if (CanRenameInput())
	{
		StackEditorData->SetModuleInputIsRenamePending(StackEditorDataKey, bIsRenamePending);
	}
}

void UNiagaraStackFunctionInput::RenameInput(FName NewName)
{
	if (OwningAssignmentNode.IsValid() && InputParameterHandlePath.Num() == 1 && InputParameterHandle.GetName() != NewName)
	{
		bool bIsCurrentlyPinned = GetIsPinned();
		bool bIsCurrentlyExpanded = StackEditorData->GetStackEntryIsExpanded(FNiagaraStackGraphUtilities::GenerateStackModuleEditorDataKey(*OwningAssignmentNode), false);

		FNiagaraParameterHandle TargetHandle(OwningAssignmentNode->AssignmentTarget.GetName());
		FNiagaraParameterHandle RenamedTargetHandle(TargetHandle.GetNamespace(), NewName);

		OwningAssignmentNode->AssignmentTarget.SetName(RenamedTargetHandle.GetParameterHandleString());
		OwningAssignmentNode->RefreshFromExternalChanges();

		InputParameterHandle = FNiagaraParameterHandle(InputParameterHandle.GetNamespace(), NewName);
		InputParameterHandlePath.Empty(1);
		InputParameterHandlePath.Add(InputParameterHandle);
		AliasedInputParameterHandle = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(InputParameterHandle, OwningAssignmentNode.Get());
		DisplayName = FText::FromName(InputParameterHandle.GetName());

		if (IsRapidIterationCandidate())
		{
			FString OriginalName = RapidIterationParameterName;
			FString NewNameForRIParam = CreateRapidIterationVariableName(AliasedInputParameterHandle.GetParameterHandleString().ToString());

			FNiagaraVariable Temp(InputType, *OriginalName);
			for (TWeakObjectPtr<UNiagaraScript> Script : AffectedScripts)
			{
				Script->RapidIterationParameters.RenameParameter(Temp, *NewNameForRIParam);
			}
			UE_LOG(LogNiagaraEditor, Log, TEXT("Renaming %s to %s"), *RapidIterationParameterName, *NewNameForRIParam);
			RapidIterationParameterName = NewNameForRIParam;
		}

		UEdGraphPin* OverridePin = GetOverridePin();
		if (OverridePin != nullptr)
		{
			OverridePin->PinName = AliasedInputParameterHandle.GetParameterHandleString();
		}

		StackEditorDataKey = FNiagaraStackGraphUtilities::GenerateStackFunctionInputEditorDataKey(*OwningFunctionCallNode.Get(), InputParameterHandle);
		StackEditorData->SetModuleInputIsPinned(StackEditorDataKey, bIsCurrentlyPinned);
		StackEditorData->SetStackEntryIsExpanded(FNiagaraStackGraphUtilities::GenerateStackModuleEditorDataKey(*OwningAssignmentNode), bIsCurrentlyExpanded);

		CastChecked<UNiagaraGraph>(OwningAssignmentNode->GetGraph())->NotifyGraphNeedsRecompile();
	}
}

void UNiagaraStackFunctionInput::GetNamespacesForNewParameters(TArray<FName>& OutNamespacesForNewParameters) const
{
	UNiagaraNodeOutput* OutputNode = FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(*OwningFunctionCallNode);
	bool bIsEditingSystem = GetSystemViewModel()->GetEditMode() == ENiagaraSystemViewModelEditMode::SystemAsset;

	if (OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleSpawnScript || OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleUpdateScript)
	{
		OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::ParticleAttributeNamespace);
		OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::EmitterNamespace);
		if (bIsEditingSystem)
		{
			OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::SystemNamespace);
			OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::UserNamespace);
		}
	}
	else if (OutputNode->GetUsage() == ENiagaraScriptUsage::EmitterSpawnScript || OutputNode->GetUsage() == ENiagaraScriptUsage::EmitterUpdateScript)
	{
		OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::EmitterNamespace);
		if (bIsEditingSystem)
		{
			OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::SystemNamespace);
			OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::UserNamespace);
		}
	}
	else if (OutputNode->GetUsage() == ENiagaraScriptUsage::SystemSpawnScript || OutputNode->GetUsage() == ENiagaraScriptUsage::ParticleUpdateScript && bIsEditingSystem)
	{
		OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::SystemNamespace);
		OutNamespacesForNewParameters.Add(FNiagaraParameterHandle::UserNamespace);
	}
}

UNiagaraStackFunctionInput::FOnValueChanged& UNiagaraStackFunctionInput::OnValueChanged()
{
	return ValueChangedDelegate;
}

UNiagaraStackFunctionInput::FOnPinnedChanged& UNiagaraStackFunctionInput::OnPinnedChanged()
{
	return PinnedChangedDelegate;
}

void UNiagaraStackFunctionInput::OnGraphChanged(const struct FEdGraphEditAction& InAction)
{
	if (bUpdatingGraphDirectly == false)
	{
		OverrideNodeCache.Reset();
		OverridePinCache.Reset();
	}
}

void UNiagaraStackFunctionInput::OnRapidIterationParametersChanged()
{
	bCanResetToBase.Reset();
}

void UNiagaraStackFunctionInput::OnScriptSourceChanged()
{
	bCanResetToBase.Reset();
}

UNiagaraNodeParameterMapSet* UNiagaraStackFunctionInput::GetOverrideNode() const
{
	if (OverrideNodeCache.IsSet() == false)
	{
		UNiagaraNodeParameterMapSet* OverrideNode = nullptr;
		if (OwningFunctionCallNode.IsValid())
		{
			OverrideNode = FNiagaraStackGraphUtilities::GetStackFunctionOverrideNode(*OwningFunctionCallNode);
		}
		OverrideNodeCache = OverrideNode;
	}
	return OverrideNodeCache.GetValue();
}

UNiagaraNodeParameterMapSet& UNiagaraStackFunctionInput::GetOrCreateOverrideNode()
{
	UNiagaraNodeParameterMapSet* OverrideNode = GetOverrideNode();
	if (OverrideNode == nullptr)
	{
		TGuardValue<bool>(bUpdatingGraphDirectly, true);
		OverrideNode = &FNiagaraStackGraphUtilities::GetOrCreateStackFunctionOverrideNode(*OwningFunctionCallNode);
		OverrideNodeCache = OverrideNode;
	}
	return *OverrideNode;
}

UEdGraphPin* UNiagaraStackFunctionInput::GetDefaultPin() const
{
	return OwningFunctionCallNode->FindParameterMapDefaultValuePin(InputParameterHandle.GetParameterHandleString());
}

UEdGraphPin* UNiagaraStackFunctionInput::GetOverridePin() const
{
	if (OverridePinCache.IsSet() == false)
	{
		OverridePinCache = FNiagaraStackGraphUtilities::GetStackFunctionInputOverridePin(*OwningFunctionCallNode, AliasedInputParameterHandle);
	}
	return OverridePinCache.GetValue();
}

UEdGraphPin& UNiagaraStackFunctionInput::GetOrCreateOverridePin()
{
	UEdGraphPin* OverridePin = GetOverridePin();
	if (OverridePin == nullptr)
	{
		TGuardValue<bool>(bUpdatingGraphDirectly, true);
		OverridePin = &FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(*OwningFunctionCallNode, AliasedInputParameterHandle, InputType);
		OverridePinCache = OverridePin;
	}
	return *OverridePin;
}

bool UNiagaraStackFunctionInput::TryGetCurrentLocalValue(TSharedPtr<FStructOnScope>& LocalValue, UEdGraphPin& ValuePin, TSharedPtr<FStructOnScope> OldValueToReuse)
{
	if (InputType.IsDataInterface() == false && ValuePin.LinkedTo.Num() == 0)
	{
		const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
		FNiagaraVariable ValueVariable = NiagaraSchema->PinToNiagaraVariable(&ValuePin, true);
		if (OldValueToReuse.IsValid() && OldValueToReuse->GetStruct() == ValueVariable.GetType().GetStruct())
		{
			LocalValue = OldValueToReuse;
		}
		else
		{
			LocalValue = MakeShared<FStructOnScope>(ValueVariable.GetType().GetStruct());
		}

		if (IsRapidIterationCandidate())
		{
			ValueVariable.SetName(*RapidIterationParameterName);

			int32 FoundOffset = SourceScript->RapidIterationParameters.IndexOf(ValueVariable);
			if (FoundOffset == INDEX_NONE)
			{
				// If the source script didn't have a value for this variable, update all affected scripts
				// with the default value from the function.
				UpdateRapidIterationParametersForAffectedScripts(ValueVariable.GetData());
			}

			FoundOffset = SourceScript->RapidIterationParameters.IndexOf(ValueVariable);
			if (FoundOffset != INDEX_NONE)
			{
				FMemory::Memcpy(LocalValue->GetStructMemory(), SourceScript->RapidIterationParameters.GetParameterData(FoundOffset), ValueVariable.GetSizeInBytes());
			}
			else
			{
				// How would we ever get here in practice, we just added above?
				ensure(false);
				return false;
			}
		}
		else
		{
			ValueVariable.CopyTo(LocalValue->GetStructMemory());
		}
		return true;
	}
	return false;
}

bool UNiagaraStackFunctionInput::TryGetCurrentDataValue(FDataValues& DataValues, UEdGraphPin* OverrideValuePin, UEdGraphPin& DefaultValuePin, UNiagaraDataInterface* LocallyOwnedDefaultDataValueObjectToReuse)
{
	if (InputType.GetClass() != nullptr)
	{
		UNiagaraDataInterface* DataValueObject = nullptr;
		if (OverrideValuePin != nullptr)
		{
			if (OverrideValuePin->LinkedTo.Num() == 1)
			{
				UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(OverrideValuePin->LinkedTo[0]->GetOwningNode());
				if (InputNode != nullptr)
				{
					if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter)
					{
						DataValueObject = InputNode->GetDataInterface();
					}
				}
			}
		}

		UNiagaraDataInterface* DefaultDataValueObject = nullptr;
		FDataValues::EDefaultValueOwner DefaultDataValueOwner = FDataValues::EDefaultValueOwner::Invalid;
		if (DefaultValuePin.LinkedTo.Num() == 1)
		{
			UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(DefaultValuePin.LinkedTo[0]->GetOwningNode());
			if (InputNode != nullptr && InputNode->Usage == ENiagaraInputNodeUsage::Parameter && InputNode->GetDataInterface() != nullptr)
			{
				DefaultDataValueObject = InputNode->GetDataInterface();
				DefaultDataValueOwner = FDataValues::EDefaultValueOwner::FunctionOwned;
			}
		}

		if (DefaultDataValueObject == nullptr)
		{
			if (LocallyOwnedDefaultDataValueObjectToReuse == nullptr)
			{
				DefaultDataValueObject = NewObject<UNiagaraDataInterface>(this, const_cast<UClass*>(InputType.GetClass()));
			}
			else
			{
				DefaultDataValueObject = LocallyOwnedDefaultDataValueObjectToReuse;
			}
			DefaultDataValueOwner = FDataValues::EDefaultValueOwner::LocallyOwned;
		}
		
		DataValues = FDataValues(DataValueObject, DefaultDataValueObject, DefaultDataValueOwner);
		return true;
	}
	return false;
}

bool UNiagaraStackFunctionInput::TryGetCurrentLinkedValue(FNiagaraParameterHandle& LinkedValueHandle, UEdGraphPin& ValuePin)
{
	if (ValuePin.LinkedTo.Num() == 1)
	{
		UEdGraphPin* CurrentValuePin = &ValuePin;
		TSharedPtr<TArray<FNiagaraParameterHandle>> AvailableHandles;
		while (CurrentValuePin != nullptr)
		{
			UEdGraphPin* LinkedValuePin = CurrentValuePin->LinkedTo[0];
			CurrentValuePin = nullptr;

			UNiagaraNodeParameterMapGet* GetNode = Cast<UNiagaraNodeParameterMapGet>(LinkedValuePin->GetOwningNode());
			if (GetNode == nullptr)
			{
				// Only parameter map get nodes are supported for linked values.
				return false;
			}

			// If a parameter map get node was found, the linked handle will be stored in the pin name.  
			FNiagaraParameterHandle LinkedValueHandleFromNode(LinkedValuePin->PinName);

			UEdGraphPin* LinkedValueHandleDefaultPin = GetNode->GetDefaultPin(LinkedValuePin);
			if (LinkedValueHandleDefaultPin->LinkedTo.Num() == 0)
			{
				// If the default value pin for this get node isn't connected this is the last read in the chain
				// so return the handle.
				LinkedValueHandle = LinkedValueHandleFromNode;
				return true;
			}
			else
			{
				// If the default value pin for the get node is connected then there are a chain of possible values.
				// if the value of the current get node is available it can be returned, otherwise we need to check the
				// next node.
				if (AvailableHandles.IsValid() == false)
				{
					AvailableHandles = MakeShared<TArray<FNiagaraParameterHandle>>();
					GetAvailableParameterHandles(*AvailableHandles);
				}

				if (AvailableHandles->Contains(LinkedValueHandleFromNode))
				{
					LinkedValueHandle = LinkedValueHandleFromNode;
					return true;
				}
				else
				{
					CurrentValuePin = LinkedValueHandleDefaultPin;
				}
			}
		}
	}
	return false;
}

bool UNiagaraStackFunctionInput::TryGetCurrentDynamicValue(TWeakObjectPtr<UNiagaraNodeFunctionCall>& DynamicValue, UEdGraphPin* OverridePin)
{
	if (OverridePin != nullptr && OverridePin->LinkedTo.Num() == 1)
	{
		UNiagaraNodeFunctionCall* DynamicNode = Cast<UNiagaraNodeFunctionCall>(OverridePin->LinkedTo[0]->GetOwningNode());
		if (DynamicNode != nullptr)
		{
			DynamicValue = DynamicNode;
			return true;
		}
	}
	return false;
}

void UNiagaraStackFunctionInput::RemoveNodesForOverridePin(UEdGraphPin& OverridePin)
{
	TArray<TWeakObjectPtr<UNiagaraDataInterface>> RemovedDataObjects;
	FNiagaraStackGraphUtilities::RemoveNodesForStackFunctionInputOverridePin(OverridePin, RemovedDataObjects);
	for (TWeakObjectPtr<UNiagaraDataInterface> RemovedDataObject : RemovedDataObjects)
	{
		if (RemovedDataObject.IsValid())
		{
			OnDataObjectModified().Broadcast(RemovedDataObject.Get());
		}
	}
}

#undef LOCTEXT_NAMESPACE
