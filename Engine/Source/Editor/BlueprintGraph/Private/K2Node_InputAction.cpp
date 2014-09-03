// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "K2Node_InputActionEvent.h"
#include "CompilerResultsLog.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintActionDatabaseRegistrar.h"

#define LOCTEXT_NAMESPACE "K2Node_InputAction"

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
	return GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
}

FText UK2Node_InputAction::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView)
	{
		return FText::FromName(InputActionName);
	}
	else if (CachedNodeTitle.IsOutOfDate())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("InputActionName"), FText::FromName(InputActionName));

		FText LocFormat = NSLOCTEXT("K2Node", "InputAction_Name", "InputAction {InputActionName}");
		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitle = FText::Format(LocFormat, Args);
	}

	return CachedNodeTitle;
}

FText UK2Node_InputAction::GetTooltipText() const
{
	if (CachedTooltip.IsOutOfDate())
	{
		// FText::Format() is slow, so we cache this to save on performance
		CachedTooltip = FText::Format(NSLOCTEXT("K2Node", "InputAction_Tooltip", "Event for when the keys bound to input action {0} are pressed or released."), FText::FromName(InputActionName));
	}
	return CachedTooltip;
}

bool UK2Node_InputAction::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);

	UEdGraphSchema_K2 const* K2Schema = Cast<UEdGraphSchema_K2>(Graph->GetSchema());
	bool const bIsConstructionScript = (K2Schema != nullptr) ? K2Schema->IsConstructionScript(Graph) : false;

	return (Blueprint != nullptr) && Blueprint->SupportsInputEvents() && !bIsConstructionScript && Super::IsCompatibleWithGraph(Graph);
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
			CompilerContext.MovePinLinksToIntermediate(*InputActionPin, *EventOutput);
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

void UK2Node_InputAction::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	TArray<FName> ActionNames;
	GetDefault<UInputSettings>()->GetActionNames(ActionNames);

	for (FName const InputActionName : ActionNames)
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		auto CustomizeInputNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FName ActionName)
		{
			UK2Node_InputAction* InputNode = CastChecked<UK2Node_InputAction>(NewNode);
			InputNode->InputActionName = ActionName;
		};

		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeInputNodeLambda, InputActionName);
		ActionRegistrar.AddBlueprintAction(NodeSpawner);
	}
}

FText UK2Node_InputAction::GetMenuCategory() const
{
	return FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::Input, LOCTEXT("ActionMenuCategory", "Action Events"));
}

#undef LOCTEXT_NAMESPACE
