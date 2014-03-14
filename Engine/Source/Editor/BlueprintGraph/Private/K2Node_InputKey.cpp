// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "KismetCompiler.h"

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
	return GEditor->AccessEditorUserSettings().EventNodeTitleColor;
}

FName UK2Node_InputKey::GetModifierName() const
{
	if ( bControl && !bAlt && !bShift )
	{
		return FName("Ctrl");
	}
	else if ( bControl && bAlt && !bShift )
	{
		return FName("Ctrl+Alt");
	}
	else if ( bControl && !bAlt && bShift )
	{
		return FName("Ctrl+Shift");
	}
	else if ( bControl && bAlt && bShift )
	{
		return FName("Ctrl+Alt+Shift");
	}
	else if ( !bControl && bAlt && !bShift )
	{
		return FName("Alt");
	}
	else if ( !bControl && bAlt && bShift )
	{
		return FName("Alt+Shift");
	}
	else if ( !bControl && !bAlt && bShift )
	{
		return FName("Shift");
	}

	return NAME_None;
}

FText UK2Node_InputKey::GetModifierText() const
{
	//@todo This should be unified with other places in the editor [10/11/2013 justin.sargent]
	if ( bControl && !bAlt && !bShift )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command", "Command");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control", "Ctrl");
#endif
	}
	else if ( bControl && bAlt && !bShift )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Alt", "Command+Alt");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Alt", "Ctrl+Alt");
#endif
	}
	else if ( bControl && !bAlt && bShift )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Shift", "Command+Shift");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Shift", "Ctrl+Shift");
#endif
	}
	else if ( bControl && bAlt && bShift )
	{
#if PLATFORM_MAC
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Command + KeyName_Alt + KeyName_Shift", "Command+Alt+Shift");
#else
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Control + KeyName_Alt + KeyName_Shift", "Ctrl+Alt+Shift");
#endif
	}
	else if ( !bControl && bAlt && !bShift )
	{
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Alt", "Alt");
	}
	else if ( !bControl && bAlt && bShift )
	{
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Alt + KeyName_Shift", "Alt+Shift");
	}
	else if ( !bControl && !bAlt && bShift )
	{
		return NSLOCTEXT("UK2Node_InputKey", "KeyName_Shift", "Shift");
	}

	return FText::GetEmpty();
}

FText UK2Node_InputKey::GetKeyText() const
{
	return InputKey.GetDisplayName();
}

FString UK2Node_InputKey::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (bControl || bAlt || bShift)
	{
		return FText::Format(NSLOCTEXT("K2Node", "InputKey_Name_WithModifiers", "{0} {1}"), GetModifierText(), GetKeyText()).ToString();
	}
	else
	{
		return FText::Format(NSLOCTEXT("K2Node", "InputKey_Name", "{0}"), GetKeyText()).ToString();
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
	else if (InputKey.IsAxis())
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
			CompilerContext.CheckConnectionResponse(CompilerContext.GetSchema()->MovePinLinks(*InputKeyPin, *EventOutput), this);
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