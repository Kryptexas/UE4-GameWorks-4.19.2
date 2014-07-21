// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "K2Node_InputKeyEvent.h"
#include "CompilerResultsLog.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "UK2Node_InputKey"

UK2Node_InputKey::UK2Node_InputKey(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
	bOverrideParentBinding = true;
}

void UK2Node_InputKey::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerUE4Version() < VER_UE4_BLUEPRINT_INPUT_BINDING_OVERRIDES)
	{
		// Don't change existing behaviors
		bOverrideParentBinding = false;
	}
}

void UK2Node_InputKey::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, TEXT("Pressed"));
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, TEXT("Released"));

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_InputKey::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->EventNodeTitleColor;
}

FName UK2Node_InputKey::GetModifierName() const
{
	if ( bControl && !bAlt && !bShift && !bCommand )
	{
		return FName("Ctrl");
	}
	else if ( bControl && bAlt && !bShift && !bCommand )
	{
		return FName("Ctrl+Alt");
	}
	else if ( bControl && !bAlt && bShift && !bCommand )
	{
		return FName("Ctrl+Shift");
	}
	else if ( bControl && !bAlt && !bShift && bCommand )
	{
		return FName("Ctrl+Cmd");
	}
	else if ( bControl && bAlt && bShift && !bCommand )
	{
		return FName("Ctrl+Alt+Shift");
	}
	else if ( bControl && bAlt && !bShift && bCommand )
	{
		return FName("Ctrl+Cmd+Alt");
	}
	else if ( bControl && !bAlt && bShift && bCommand )
	{
		return FName("Ctrl+Cmd+Shift");
	}
	else if ( !bControl && bAlt && bShift && bCommand )
	{
		return FName("Cmd+Alt+Shift");
	}
	else if ( bControl && bAlt && bShift && bCommand )
	{
		return FName("Ctrl+Cmd+Alt+Shift");
	}
	else if ( !bControl && bAlt && !bShift && !bCommand )
	{
		return FName("Alt");
	}
	else if ( !bControl && bAlt && bShift && !bCommand )
	{
		return FName("Alt+Shift");
	}
	else if ( !bControl && bAlt && !bShift && bCommand )
	{
		return FName("Cmd+Alt");
	}
	else if ( !bControl && !bAlt && bShift && bCommand )
	{
		return FName("Cmd+Shift");
	}
	else if ( !bControl && !bAlt && bShift && !bCommand )
	{
		return FName("Shift");
	}
	else if ( !bControl && !bAlt && !bShift && bCommand )
	{
		return FName("Cmd");
	}

	return NAME_None;
}

FText UK2Node_InputKey::GetModifierText() const
{
	//@todo This should be unified with other places in the editor [10/11/2013 justin.sargent]
	if ( bControl && !bAlt && !bShift && !bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command", "Cmd");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control", "Ctrl");
#endif
	}
	else if ( bControl && bAlt && !bShift && !bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Alt", "Cmd+Alt");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Alt", "Ctrl+Alt");
#endif
	}
	else if ( bControl && !bAlt && bShift && !bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Shift", "Cmd+Shift");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Shift", "Ctrl+Shift");
#endif
	}
	else if ( bControl && !bAlt && !bShift && bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Control", "Cmd+Ctrl");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Command", "Ctrl+Cmd");
#endif
	}
	else if ( !bControl && bAlt && !bShift && bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Alt", "Ctrl+Alt");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Alt", "Cmd+Alt");
#endif
	}
	else if ( !bControl && !bAlt && bShift && bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Shift", "Ctrl+Shift");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Shift", "Cmd+Shift");
#endif
	}
	else if ( bControl && bAlt && bShift && !bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Alt + KeyName_Shift", "Cmd+Alt+Shift");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Alt + KeyName_Shift", "Ctrl+Alt+Shift");
#endif
	}
	else if ( bControl && bAlt && !bShift && bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Control + KeyName_Alt", "Cmd+Ctrl+Alt");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Command + KeyName_Alt", "Ctrl+Cmd+Alt");
#endif
	}
	else if ( bControl && !bAlt && bShift && bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Control + KeyName_Shift", "Cmd+Ctrl+Shift");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Command + KeyName_Shift", "Ctrl+Cmd+Shift");
#endif
	}
	else if ( bControl && bAlt && bShift && bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Control + KeyName_Alt + KeyName_Shift", "Cmd+Ctrl+Alt+Shift");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Command + KeyName_Alt + KeyName_Shift", "Ctrl+Cmd+Alt+Shift");
#endif
	}
	else if ( !bControl && bAlt && !bShift && !bCommand )
	{
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Alt", "Alt");
	}
	else if ( !bControl && bAlt && bShift && !bCommand )
	{
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Alt + KeyName_Shift", "Alt+Shift");
	}
	else if ( !bControl && bAlt && !bShift && bCommand )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Alt", "Ctrl+Alt");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Alt", "Cmd+Alt");
#endif
	}
	else if ( !bControl && !bAlt && bShift && !bCommand )
	{
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Shift", "Shift");
	}
	else if ( !bControl && !bAlt && !bShift && bCommand )
	{
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command", "Cmd");
	}

	return FText::GetEmpty();
}

FText UK2Node_InputKey::GetKeyText() const
{
	return InputKey.GetDisplayName();
}

FText UK2Node_InputKey::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (bControl || bAlt || bShift)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ModifierKey"), GetModifierText());
		Args.Add(TEXT("Key"), GetKeyText());
		return FText::Format(NSLOCTEXT("K2Node", "InputKey_Name_WithModifiers", "{ModifierKey} {Key}"), Args);
	}
	else
	{
		return GetKeyText();
	}
}

FString UK2Node_InputKey::GetTooltip() const
{
	FText ModifierText = GetModifierText();
	FText KeyText = GetKeyText();

	if ( !ModifierText.IsEmpty() )
	{
		return FText::Format(NSLOCTEXT("K2Node", "InputKey_Tooltip_Modifiers", "Events for when the {0} key is pressed or released while {1} is also held."), KeyText, ModifierText).ToString();
	}

	return FText::Format(NSLOCTEXT("K2Node", "InputKey_Tooltip", "Events for when the {0} key is pressed or released."), KeyText).ToString();
}

FName UK2Node_InputKey::GetPaletteIcon(FLinearColor& OutColor) const
{
	if (InputKey.IsMouseButton())
	{
		return TEXT("GraphEditor.MouseEvent_16x");
	}
	else if (InputKey.IsGamepadKey())
	{
		return TEXT("GraphEditor.PadEvent_16x");
	}
	else
	{
		return TEXT("GraphEditor.KeyEvent_16x");
	}
}

UEdGraphPin* UK2Node_InputKey::GetPressedPin() const
{
	return FindPin(TEXT("Pressed"));
}

UEdGraphPin* UK2Node_InputKey::GetReleasedPin() const
{
	return FindPin(TEXT("Released"));
}

void UK2Node_InputKey::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);
	
	if (!InputKey.IsValid())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "Invalid_InputKey_Warning", "InputKey Event specifies invalid FKey'{0}' for @@"), FText::FromString(InputKey.ToString())).ToString(), this);
	}
	else if (InputKey.IsFloatAxis())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "Axis_InputKey_Warning", "InputKey Event specifies axis FKey'{0}' for @@"), FText::FromString(InputKey.ToString())).ToString(), this);
	}
	else if (!InputKey.IsBindableInBlueprints())
	{
		MessageLog.Warning( *FText::Format( NSLOCTEXT("KismetCompiler", "NotBindanble_InputKey_Warning", "InputKey Event specifies FKey'{0}' that is not blueprint bindable for @@"), FText::FromString(InputKey.ToString())).ToString(), this);
	}
}

void UK2Node_InputKey::CreateInputKeyEvent(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* InputKeyPin, const EInputEvent KeyEvent)
{
	if (InputKeyPin->LinkedTo.Num() > 0)
	{
		UK2Node_InputKeyEvent* InputKeyEvent = CompilerContext.SpawnIntermediateNode<UK2Node_InputKeyEvent>(this, SourceGraph);
		const FName ModifierName = GetModifierName();
		if ( ModifierName != NAME_None )
		{
			InputKeyEvent->CustomFunctionName = FName( *FString::Printf(TEXT("InpActEvt_%s_%s_%s"), *ModifierName.ToString(), *InputKey.ToString(), *InputKeyEvent->GetName()));
		}
		else
		{
			InputKeyEvent->CustomFunctionName = FName( *FString::Printf(TEXT("InpActEvt_%s_%s"), *InputKey.ToString(), *InputKeyEvent->GetName()));
		}
		InputKeyEvent->InputChord.Key = InputKey;
		InputKeyEvent->InputChord.bCtrl = bControl;
		InputKeyEvent->InputChord.bAlt = bAlt;
		InputKeyEvent->InputChord.bShift = bShift;
		InputKeyEvent->bConsumeInput = bConsumeInput;
		InputKeyEvent->bExecuteWhenPaused = bExecuteWhenPaused;
		InputKeyEvent->bOverrideParentBinding = bOverrideParentBinding;
		InputKeyEvent->InputKeyEvent = KeyEvent;
		InputKeyEvent->EventSignatureName = TEXT("InputActionHandlerDynamicSignature__DelegateSignature");
		InputKeyEvent->EventSignatureClass = UInputComponent::StaticClass();
		InputKeyEvent->bInternalEvent = true;
		InputKeyEvent->AllocateDefaultPins();

		// Move any exec links from the InputActionNode pin to the InputActionEvent node
		UEdGraphPin* EventOutput = CompilerContext.GetSchema()->FindExecutionPin(*InputKeyEvent, EGPD_Output);

		if(EventOutput != NULL)
		{
			CompilerContext.MovePinLinksToIntermediate(*InputKeyPin, *EventOutput);
		}
	}
}

void UK2Node_InputKey::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		CreateInputKeyEvent(CompilerContext, SourceGraph, GetPressedPin(), IE_Pressed);
		CreateInputKeyEvent(CompilerContext, SourceGraph, GetReleasedPin(), IE_Released);
	}
}

void UK2Node_InputKey::GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const
{
	TArray<FKey> AllKeys;
	EKeys::GetAllKeys(AllKeys);

	for (FKey const Key : AllKeys)
	{
		// these will be handled by UK2Node_GetInputAxisKeyValue and UK2Node_GetInputVectorAxisValue respectively
		if (!Key.IsBindableInBlueprints() || Key.IsFloatAxis() || Key.IsVectorAxis())
		{
			continue;
		}

		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		auto CustomizeInputNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FKey Key)
		{
			UK2Node_InputKey* InputNode = CastChecked<UK2Node_InputKey>(NewNode);
			InputNode->InputKey = Key;
		};

		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeInputNodeLambda, Key);
		ActionListOut.Add(NodeSpawner);
	}
}

FText UK2Node_InputKey::GetMenuCategory() const
{
	FText SubCategory;
	if (InputKey.IsGamepadKey())
	{
		SubCategory = LOCTEXT("GamepadCategory", "Gamepad Events");
	}
	else if (InputKey.IsMouseButton())
	{
		SubCategory = LOCTEXT("MouseCategory", "Mouse Events");
	}
	else
	{
		SubCategory = LOCTEXT("KeyEventsCategory", "Key Events");
	}

	return FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::Input, SubCategory);

}

#undef LOCTEXT_NAMESPACE
