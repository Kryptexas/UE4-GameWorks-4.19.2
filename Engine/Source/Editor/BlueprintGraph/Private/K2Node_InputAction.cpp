// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "KismetCompiler.h"

UK2Node_InputAction::UK2Node_InputAction(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
	bOverrideParentBinding = true;
}

void UK2Node_InputAction::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerUE4Version() < VER_UE4_BLUEPRINT_INPUT_BINDING_OVERRIDES)
	{
		// Don't change existing behaviors
		bOverrideParentBinding = false;
	}
}

void UK2Node_InputAction::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, TEXT("Pressed"));
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, TEXT("Released"));

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_InputAction::GetNodeTitleColor() const
{
	return GEditor->AccessEditorUserSettings().EventNodeTitleColor;
}

FString UK2Node_InputAction::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "InputAction_Name", "InputAction %s").ToString(), *InputActionName.ToString());
}

FString UK2Node_InputAction::GetTooltip() const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "InputAction_Tooltip", "Event for when the keys bound to input action %s are pressed or released.").ToString(), *InputActionName.ToString());
}

UEdGraphPin* UK2Node_InputAction::GetPressedPin() const
{
	return FindPin(TEXT("Pressed"));
}

UEdGraphPin* UK2Node_InputAction::GetReleasedPin() const
{
	return FindPin(TEXT("Released"));
}

void UK2Node_InputAction::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	TArray<FName> ActionNames;
	GetDefault<UInputSettings>()->GetActionNames(ActionNames);
	if (!ActionNames.Contains(InputActionName))
	{
		MessageLog.Warning(*FString::Printf(*NSLOCTEXT("KismetCompiler", "MissingInputAction_Warning", "InputAction Event references unknown Action '%s' for @@").ToString(), *InputActionName.ToString()), this);
	}
}

void UK2Node_InputAction::CreateInputActionEvent(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* InputActionPin, const EInputEvent InputKeyEvent)
{
	if (InputActionPin->LinkedTo.Num() > 0)
	{
		UK2Node_InputActionEvent* InputActionEvent = CompilerContext.SpawnIntermediateNode<UK2Node_InputActionEvent>(this, SourceGraph);
		InputActionEvent->CustomFunctionName = FName( *FString::Printf(TEXT("InpActEvt_%s_%s"), *InputActionName.ToString(), *InputActionEvent->GetName()));
		InputActionEvent->InputActionName = InputActionName;
		InputActionEvent->bConsumeInput = bConsumeInput;
		InputActionEvent->bExecuteWhenPaused = bExecuteWhenPaused;
		InputActionEvent->bOverrideParentBinding = bOverrideParentBinding;
		InputActionEvent->InputKeyEvent = InputKeyEvent;
		InputActionEvent->EventSignatureName = TEXT("InputActionHandlerDynamicSignature__DelegateSignature");
		InputActionEvent->EventSignatureClass = UInputComponent::StaticClass();
		InputActionEvent->bInternalEvent = true;
		InputActionEvent->AllocateDefaultPins();

		// Move any exec links from the InputActionNode pin to the InputActionEvent node
		UEdGraphPin* EventOutput = CompilerContext.GetSchema()->FindExecutionPin(*InputActionEvent, EGPD_Output);

		if(EventOutput != NULL)
		{
			CompilerContext.CheckConnectionResponse(CompilerContext.GetSchema()->MovePinLinks(*InputActionPin, *EventOutput), this);
		}
	}
}


void UK2Node_InputAction::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		CreateInputActionEvent(CompilerContext, SourceGraph, GetPressedPin(), IE_Pressed);
		CreateInputActionEvent(CompilerContext, SourceGraph, GetReleasedPin(), IE_Released);
	}
}