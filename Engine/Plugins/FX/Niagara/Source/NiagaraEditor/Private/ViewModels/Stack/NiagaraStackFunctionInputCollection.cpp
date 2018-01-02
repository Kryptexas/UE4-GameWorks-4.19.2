// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackFunctionInputCollection.h"
#include "NiagaraStackFunctionInput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraEmitterEditorData.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraStackGraphUtilities.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraDataInterface.h"

#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "UNiagaraStackFunctionInputCollection"

UNiagaraStackFunctionInputCollection::UNiagaraStackFunctionInputCollection()
	: ModuleNode(nullptr)
	, InputFunctionCallNode(nullptr)
{
}

UNiagaraNodeFunctionCall* UNiagaraStackFunctionInputCollection::GetModuleNode() const
{
	return ModuleNode;
}

UNiagaraNodeFunctionCall* UNiagaraStackFunctionInputCollection::GetInputFunctionCallNode() const
{
	return InputFunctionCallNode;
}

void UNiagaraStackFunctionInputCollection::Initialize(
	TSharedRef<FNiagaraSystemViewModel> InSystemViewModel,
	TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel,
	UNiagaraStackEditorData& InStackEditorData,
	UNiagaraNodeFunctionCall& InModuleNode,
	UNiagaraNodeFunctionCall& InInputFunctionCallNode,
	FDisplayOptions InDisplayOptions)
{
	checkf(ModuleNode == nullptr && InputFunctionCallNode == nullptr, TEXT("Can not set the node more than once."));
	Super::Initialize(InSystemViewModel, InEmitterViewModel);
	StackEditorData = &InStackEditorData;
	ModuleNode = &InModuleNode;
	InputFunctionCallNode = &InInputFunctionCallNode;
	DisplayOptions = InDisplayOptions;
	if (DisplayOptions.ChildFilter.IsBound())
	{
		AddChildFilter(DisplayOptions.ChildFilter);
	}
}

FText UNiagaraStackFunctionInputCollection::GetDisplayName() const
{
	return DisplayOptions.DisplayName;
}

FName UNiagaraStackFunctionInputCollection::GetTextStyleName() const
{
	return "NiagaraEditor.Stack.ParameterCollectionText";
}

bool UNiagaraStackFunctionInputCollection::GetCanExpand() const
{
	return true;
}

bool UNiagaraStackFunctionInputCollection::GetShouldShowInStack() const
{
	return DisplayOptions.bShouldShowInStack;
}

int32 UNiagaraStackFunctionInputCollection::GetErrorCount() const
{
	return Errors.Num();
}

bool UNiagaraStackFunctionInputCollection::GetErrorFixable(int32 ErrorIdx) const
{
	return Errors[ErrorIdx].Fix.IsBound();
}

bool UNiagaraStackFunctionInputCollection::TryFixError(int32 ErrorIdx)
{
	Errors[ErrorIdx].Fix.Execute();
	return true;
}

FText UNiagaraStackFunctionInputCollection::GetErrorText(int32 ErrorIdx) const
{
	return Errors[ErrorIdx].ErrorText;
}

FText UNiagaraStackFunctionInputCollection::GetErrorSummaryText(int32 ErrorIdx) const
{
	return Errors[ErrorIdx].ErrorSummaryText;
}

UNiagaraStackFunctionInputCollection::FOnInputPinnedChanged& UNiagaraStackFunctionInputCollection::OnInputPinnedChanged()
{
	return InputPinnedChangedDelegate;
}

void UNiagaraStackFunctionInputCollection::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	TArray<const UEdGraphPin*> InputPins;
	FNiagaraStackGraphUtilities::GetStackFunctionInputPins(*InputFunctionCallNode, InputPins, FNiagaraStackGraphUtilities::ENiagaraGetStackFunctionInputPinsOptions::ModuleInputsOnly);
	const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
	TArray<FName> ValidAliasedInputNames;
	TArray<FName> ProcessedInputNames;
	TArray<FName> DuplicateInputNames;
	TArray<const UEdGraphPin*> PinsWithInvalidTypes;
	for(const UEdGraphPin* InputPin : InputPins)
	{
		if (ProcessedInputNames.Contains(InputPin->PinName) == false)
		{
			UNiagaraStackFunctionInput* Input = FindCurrentChildOfTypeByPredicate<UNiagaraStackFunctionInput>(CurrentChildren,
				[=](UNiagaraStackFunctionInput* CurrentFunctionInput) { return CurrentFunctionInput->GetInputParameterHandle().GetParameterHandleString() == InputPin->PinName; });
			
			if (Input == nullptr)
			{
				FNiagaraTypeDefinition InputType = NiagaraSchema->PinToTypeDefinition(InputPin);
				if (InputType.IsValid())
				{
					Input = NewObject<UNiagaraStackFunctionInput>(this);
					Input->Initialize(GetSystemViewModel(), GetEmitterViewModel(), *StackEditorData, *ModuleNode, *InputFunctionCallNode, InputPin->PinName, NiagaraSchema->PinToTypeDefinition(InputPin));
					Input->SetItemIndentLevel(DisplayOptions.ChildItemIndentLevel);
					Input->OnPinnedChanged().AddUObject(this, &UNiagaraStackFunctionInputCollection::ChildPinnedChanged);
				}
				else
				{
					PinsWithInvalidTypes.Add(InputPin);
				}
			}

			if (Input != nullptr)
			{
				NewChildren.Add(Input);
			}
			
			ValidAliasedInputNames.Add(FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(FNiagaraParameterHandle(InputPin->PinName), InputFunctionCallNode).GetParameterHandleString());
			ProcessedInputNames.Add(InputPin->PinName);
		}
		else
		{
			DuplicateInputNames.AddUnique(InputPin->PinName);
		}
	}

	// Try to find function input overrides which are no longer valid so we can generate errors for them.
	Errors.Empty();
	UNiagaraNodeParameterMapSet* OverrideNode = FNiagaraStackGraphUtilities::GetStackFunctionOverrideNode(*InputFunctionCallNode);
	if (OverrideNode != nullptr)
	{
		TArray<UEdGraphPin*> OverridePins;
		OverrideNode->GetInputPins(OverridePins);
		for (UEdGraphPin* OverridePin : OverridePins)
		{
			// If the pin isn't in the misc category for the add pin, and not the parameter map pin, and it's for this function call,
			// check to see if it's in the list of valid input names, and if not generate an error.
			if (OverridePin->PinType.PinCategory != UEdGraphSchema_Niagara::PinCategoryMisc &&
				OverridePin->PinType.PinSubCategoryObject != FNiagaraTypeDefinition::GetParameterMapStruct() &&
				FNiagaraParameterHandle(OverridePin->PinName).GetNamespace().ToString() == InputFunctionCallNode->GetFunctionName() &&
				ValidAliasedInputNames.Contains(OverridePin->PinName) == false)
			{
				FError InvalidInputError;
				InvalidInputError.ErrorSummaryText = FText::Format(LOCTEXT("InvalidInputSummaryFormat", "Invalid Input: {0}"), FText::FromString(OverridePin->PinName.ToString()));
				InvalidInputError.ErrorText = FText::Format(LOCTEXT("InvalidInputFormat", "The input {0} was previously overriden but is no longer exposed by the function {1}.\nPress the fix button to remove this unused override data,\nor check the function definition to see why this input is no longer exposed."),
					FText::FromString(OverridePin->PinName.ToString()), FText::FromString(InputFunctionCallNode->GetFunctionName()));
				InvalidInputError.Fix.BindLambda([=]()
				{
					FScopedTransaction ScopedTransaction(LOCTEXT("RemoveInvalidInputTransaction", "Remove invalid input override."));
					TArray<TWeakObjectPtr<UNiagaraDataInterface>> RemovedDataObjects;
					FNiagaraStackGraphUtilities::RemoveNodesForStackFunctionInputOverridePin(*OverridePin, RemovedDataObjects);
					for (TWeakObjectPtr<UNiagaraDataInterface> RemovedDataObject : RemovedDataObjects)
					{
						if (RemovedDataObject.IsValid())
						{
							OnDataObjectModified().Broadcast(RemovedDataObject.Get());
						}
					}
					OverridePin->GetOwningNode()->RemovePin(OverridePin);
				});
				Errors.Add(InvalidInputError);
			}
		}
	}

	for (const FName& DuplicateInputName : DuplicateInputNames)
	{
		FError DuplicateInputError;

		FString DuplicateInputNameString = DuplicateInputName.ToString();

		DuplicateInputError.ErrorSummaryText = FText::Format(LOCTEXT("DuplicateInputSummaryFormat", "Duplicate Input: {0}"), FText::FromString(DuplicateInputNameString));
		DuplicateInputError.ErrorText = FText::Format(LOCTEXT("DuplicateInputFormat", "There are multiple inputs with the same name {0}, but different types exposed by the function {1}.\nThis is not suppored and must be fixed in the script that defines this function."),
			FText::FromString(DuplicateInputNameString), FText::FromString(InputFunctionCallNode->GetFunctionName()));
		Errors.Add(DuplicateInputError);
	}

	for (const UEdGraphPin* PinWithInvalidType : PinsWithInvalidTypes)
	{
		FError InputWithInvalidTypeError;
		InputWithInvalidTypeError.ErrorSummaryText = FText::Format(LOCTEXT("InputWithInvalidTypeSummaryFormat", "Input has an invalid type: {0}"), FText::FromString(PinWithInvalidType->PinName.ToString()));
		InputWithInvalidTypeError.ErrorText = FText::Format(LOCTEXT("InputWithInvalidTypeFormat", "The input {0} on function {1} has a type which is invalid.\nThe type of this input likely doesn't exist anymore.\nThis input must be fixed in the script before this module can be used."),
			FText::FromString(PinWithInvalidType->PinName.ToString()), FText::FromString(InputFunctionCallNode->GetFunctionName()));
		Errors.Add(InputWithInvalidTypeError);
	}
}

void UNiagaraStackFunctionInputCollection::ChildPinnedChanged()
{
	InputPinnedChangedDelegate.Broadcast();
}

#undef LOCTEXT_NAMESPACE