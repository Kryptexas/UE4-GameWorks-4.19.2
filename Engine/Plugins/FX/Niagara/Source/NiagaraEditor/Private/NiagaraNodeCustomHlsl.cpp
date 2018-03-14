// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeCustomHlsl.h"
#include "NiagaraCustomVersion.h"
#include "SNiagaraGraphNodeCustomHlsl.h"
#include "EdGraphSchema_Niagara.h"
#include "ScopedTransaction.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraGraph.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeCustomHlsl"

UNiagaraNodeCustomHlsl::UNiagaraNodeCustomHlsl(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanRenameNode = true;
	ScriptUsage = ENiagaraScriptUsage::Function;

	Signature.Name = TEXT("Custom Hlsl");
	FunctionDisplayName = Signature.Name.ToString();
}

TSharedPtr<SGraphNode> UNiagaraNodeCustomHlsl::CreateVisualWidget()
{
	return SNew(SNiagaraGraphNodeCustomHlsl, this);
}

void UNiagaraNodeCustomHlsl::OnRenameNode(const FString& NewName)
{
	Signature.Name = *NewName;
	FunctionDisplayName = NewName;
}

FText UNiagaraNodeCustomHlsl::GetHlslText() const
{
	return FText::FromString(CustomHlsl);
}

void UNiagaraNodeCustomHlsl::OnCustomHlslTextCommitted(const FText& InText, ETextCommit::Type InType)
{
	FScopedTransaction Transaction(LOCTEXT("CustomHlslCommit", "Edited Custom Hlsl"));
	Modify();
	CustomHlsl = InText.ToString();
	RefreshFromExternalChanges();
	GetNiagaraGraph()->NotifyGraphNeedsRecompile();
}

FLinearColor UNiagaraNodeCustomHlsl::GetNodeTitleColor() const
{
	return UEdGraphSchema_Niagara::NodeTitleColor_CustomHlsl;
}

#if WITH_EDITOR

void UNiagaraNodeCustomHlsl::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraNodeCustomHlsl, CustomHlsl))
	{
		RefreshFromExternalChanges();
		GetNiagaraGraph()->NotifyGraphNeedsRecompile();
	}
}
#endif

/** Called when a new typed pin is added by the user. */
void UNiagaraNodeCustomHlsl::OnNewTypedPinAdded(UEdGraphPin* NewPin)
{
	FScopedTransaction Transaction(LOCTEXT("AddedTypedPin", "New Pin Added"));
	UNiagaraNodeWithDynamicPins::OnNewTypedPinAdded(NewPin);
	RebuildSignatureFromPins();
}

/** Called when a pin is renamed. */
void UNiagaraNodeCustomHlsl::OnPinRenamed(UEdGraphPin* RenamedPin, const FString& OldPinName)
{
	FScopedTransaction Transaction(LOCTEXT("PinRenamed", "Pin Rename"));
	UNiagaraNodeWithDynamicPins::OnPinRenamed(RenamedPin, OldPinName);
	RebuildSignatureFromPins();
}

/** Removes a pin from this node with a transaction. */
void UNiagaraNodeCustomHlsl::RemoveDynamicPin(UEdGraphPin* Pin)
{
	FScopedTransaction Transaction(LOCTEXT("PinRemoved", "Remove Pin"));
	UNiagaraNodeWithDynamicPins::RemoveDynamicPin(Pin);
	RebuildSignatureFromPins();
}

void UNiagaraNodeCustomHlsl::MoveDynamicPin(UEdGraphPin* Pin, int32 DirectionToMove)
{
	FScopedTransaction Transaction(LOCTEXT("PinMoved", "Moved Pin"));
	UNiagaraNodeWithDynamicPins::MoveDynamicPin(Pin, DirectionToMove);
	RebuildSignatureFromPins();
}

void UNiagaraNodeCustomHlsl::RebuildSignatureFromPins()
{
	Modify();
	FNiagaraFunctionSignature Sig = Signature;
	Sig.Inputs.Empty();
	Sig.Outputs.Empty();

	TArray<UEdGraphPin*> InputPins;
	TArray<UEdGraphPin*> OutputPins;
	GetInputPins(InputPins);
	GetOutputPins(OutputPins);

	const UEdGraphSchema_Niagara* Schema = Cast<UEdGraphSchema_Niagara>(GetSchema());

	for (UEdGraphPin* Pin : InputPins)
	{
		if (IsAddPin(Pin))
		{
			continue;
		}
		Sig.Inputs.Add(Schema->PinToNiagaraVariable(Pin, true));
	}

	for (UEdGraphPin* Pin : OutputPins)
	{
		if (IsAddPin(Pin))
		{
			continue;
		}
		Sig.Outputs.Add(Schema->PinToNiagaraVariable(Pin, false));
	}

	Signature = Sig;

	RefreshFromExternalChanges();
	GetNiagaraGraph()->NotifyGraphNeedsRecompile();
}

#undef LOCTEXT_NAMESPACE

