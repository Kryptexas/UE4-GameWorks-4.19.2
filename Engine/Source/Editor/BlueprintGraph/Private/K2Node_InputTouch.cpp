// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "KismetCompiler.h"

UK2Node_InputTouch::UK2Node_InputTouch(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
	bOverrideParentBinding = true;
}

void UK2Node_InputTouch::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerUE4Version() < VER_UE4_BLUEPRINT_INPUT_BINDING_OVERRIDES)
	{
		// Don't change existing behaviors
		bOverrideParentBinding = false;
	}
}

UEnum* UK2Node_InputTouch::GetTouchIndexEnum()
{
	static UEnum* TouchIndexEnum = NULL;
	if(NULL == TouchIndexEnum)
	{
		FName TouchIndexEnumName(TEXT("ETouchIndex::Touch1"));
		UEnum::LookupEnumName(TouchIndexEnumName, &TouchIndexEnum);
		check(TouchIndexEnum);
	}
	return TouchIndexEnum;
}

void UK2Node_InputTouch::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, TEXT("Pressed"));
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, TEXT("Released"));

	UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));
	CreatePin(EGPD_Output, K2Schema->PC_Struct, TEXT(""), VectorStruct, false, false, TEXT("Location"));

	CreatePin(EGPD_Output, K2Schema->PC_Byte, TEXT(""), GetTouchIndexEnum(), false, false, TEXT("FingerIndex"));

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_InputTouch::GetNodeTitleColor() const
{
	return GEditor->AccessEditorUserSettings().EventNodeTitleColor;
}

FString UK2Node_InputTouch::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "InputTouch_Name", "InputTouch").ToString();
}

UEdGraphPin* UK2Node_InputTouch::GetPressedPin() const
{
	return FindPin(TEXT("Pressed"));
}

UEdGraphPin* UK2Node_InputTouch::GetReleasedPin() const
{
	return FindPin(TEXT("Released"));
}

UEdGraphPin* UK2Node_InputTouch::GetLocationPin() const
{
	return FindPin(TEXT("Location"));
}

UEdGraphPin* UK2Node_InputTouch::GetFingerIndexPin() const
{
	return FindPin(TEXT("FingerIndex"));
}

void UK2Node_InputTouch::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		UEdGraphPin* InputTouchPressedPin = GetPressedPin();
		UEdGraphPin* InputTouchReleasedPin = GetReleasedPin();

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// If both are linked we have to do more complicated behaviors
		if (InputTouchPressedPin->LinkedTo.Num() > 0 && InputTouchReleasedPin->LinkedTo.Num() > 0)
		{
			// Create the input touch events
			UK2Node_InputTouchEvent* InputTouchPressedEvent = CompilerContext.SpawnIntermediateNode<UK2Node_InputTouchEvent>(this, SourceGraph);
			InputTouchPressedEvent->CustomFunctionName = FName( *FString::Printf(TEXT("InpTchEvt_%s"), *InputTouchPressedEvent->GetName()));
			InputTouchPressedEvent->bConsumeInput = bConsumeInput;
			InputTouchPressedEvent->bExecuteWhenPaused = bExecuteWhenPaused;
			InputTouchPressedEvent->bOverrideParentBinding = bOverrideParentBinding;
			InputTouchPressedEvent->InputKeyEvent = IE_Pressed;
			InputTouchPressedEvent->EventSignatureName = TEXT("InputTouchHandlerDynamicSignature__DelegateSignature");
			InputTouchPressedEvent->EventSignatureClass = UInputComponent::StaticClass();
			InputTouchPressedEvent->bInternalEvent = true;
			InputTouchPressedEvent->AllocateDefaultPins();

			UK2Node_InputTouchEvent* InputTouchReleasedEvent = CompilerContext.SpawnIntermediateNode<UK2Node_InputTouchEvent>(this, SourceGraph);
			InputTouchReleasedEvent->CustomFunctionName = FName( *FString::Printf(TEXT("InpTchEvt_%s"),  *InputTouchReleasedEvent->GetName()));
			InputTouchReleasedEvent->bConsumeInput = bConsumeInput;
			InputTouchReleasedEvent->bExecuteWhenPaused = bExecuteWhenPaused;
			InputTouchReleasedEvent->bOverrideParentBinding = bOverrideParentBinding;
			InputTouchReleasedEvent->InputKeyEvent = IE_Released;
			InputTouchReleasedEvent->EventSignatureName = TEXT("InputTouchHandlerDynamicSignature__DelegateSignature");
			InputTouchReleasedEvent->EventSignatureClass = UInputComponent::StaticClass();
			InputTouchReleasedEvent->bInternalEvent = true;
			InputTouchReleasedEvent->AllocateDefaultPins();

			// Create a temporary variable to copy location in to
			static UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Vector"));

			UK2Node_TemporaryVariable* TouchLocationVar = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
			TouchLocationVar->VariableType.PinCategory = Schema->PC_Struct;
			TouchLocationVar->VariableType.PinSubCategoryObject = VectorStruct;
			TouchLocationVar->AllocateDefaultPins();

			// Create assignment nodes to assign the location
			UK2Node_AssignmentStatement* TouchPressedLocationInitialize = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
			TouchPressedLocationInitialize->AllocateDefaultPins();
			Schema->TryCreateConnection(TouchLocationVar->GetVariablePin(), TouchPressedLocationInitialize->GetVariablePin());
			Schema->TryCreateConnection(TouchPressedLocationInitialize->GetValuePin(), InputTouchPressedEvent->FindPinChecked(TEXT("Location")));

			UK2Node_AssignmentStatement* TouchReleasedLocationInitialize = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
			TouchReleasedLocationInitialize->AllocateDefaultPins();
			Schema->TryCreateConnection(TouchLocationVar->GetVariablePin(), TouchReleasedLocationInitialize->GetVariablePin());
			Schema->TryCreateConnection(TouchReleasedLocationInitialize->GetValuePin(), InputTouchReleasedEvent->FindPinChecked(TEXT("Location")));

			// Connect the events to the assign location nodes
			Schema->TryCreateConnection(Schema->FindExecutionPin(*InputTouchPressedEvent, EGPD_Output), TouchPressedLocationInitialize->GetExecPin());
			Schema->TryCreateConnection(Schema->FindExecutionPin(*InputTouchReleasedEvent, EGPD_Output), TouchReleasedLocationInitialize->GetExecPin());

			// Create a temporary variable to copy finger index in to
			UK2Node_TemporaryVariable* TouchFingerVar = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
			TouchFingerVar->VariableType.PinCategory = Schema->PC_Byte;
			TouchFingerVar->VariableType.PinSubCategoryObject = UK2Node_InputTouch::GetTouchIndexEnum();
			TouchFingerVar->AllocateDefaultPins();

			// Create assignment nodes to assign the finger index
			UK2Node_AssignmentStatement* TouchPressedFingerInitialize = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
			TouchPressedFingerInitialize->AllocateDefaultPins();
			Schema->TryCreateConnection(TouchFingerVar->GetVariablePin(), TouchPressedFingerInitialize->GetVariablePin());
			Schema->TryCreateConnection(TouchPressedFingerInitialize->GetValuePin(), InputTouchPressedEvent->FindPinChecked(TEXT("FingerIndex")));

			UK2Node_AssignmentStatement* TouchReleasedFingerInitialize = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
			TouchReleasedFingerInitialize->AllocateDefaultPins();
			Schema->TryCreateConnection(TouchFingerVar->GetVariablePin(), TouchReleasedFingerInitialize->GetVariablePin());
			Schema->TryCreateConnection(TouchReleasedFingerInitialize->GetValuePin(), InputTouchReleasedEvent->FindPinChecked(TEXT("FingerIndex")));

			// Connect the assign location to the assign finger index nodes
			Schema->TryCreateConnection(TouchPressedLocationInitialize->GetThenPin(), TouchPressedFingerInitialize->GetExecPin());
			Schema->TryCreateConnection(TouchReleasedLocationInitialize->GetThenPin(), TouchReleasedFingerInitialize->GetExecPin());

			// Move the original event connections to the then pin of the finger index assign
			Schema->MovePinLinks(*GetPressedPin(), *TouchPressedFingerInitialize->GetThenPin());
			Schema->MovePinLinks(*GetReleasedPin(), *TouchReleasedFingerInitialize->GetThenPin());

			// Move the original event variable connections to the intermediate nodes
			Schema->MovePinLinks(*GetLocationPin(), *TouchLocationVar->GetVariablePin());
			Schema->MovePinLinks(*GetFingerIndexPin(), *TouchFingerVar->GetVariablePin());
		}
		else
		{
			UEdGraphPin* InputTouchPin; 
			EInputEvent InputEvent;
			if (InputTouchPressedPin->LinkedTo.Num() > 0)
			{
				InputTouchPin = InputTouchPressedPin;
				InputEvent = IE_Pressed;
			}
			else
			{
				InputTouchPin = InputTouchReleasedPin;
				InputEvent = IE_Released;
			}

			if (InputTouchPin->LinkedTo.Num() > 0)
			{
				UK2Node_InputTouchEvent* InputTouchEvent = CompilerContext.SpawnIntermediateNode<UK2Node_InputTouchEvent>(this, SourceGraph);
				InputTouchEvent->CustomFunctionName = FName( *FString::Printf(TEXT("InpTchEvt_%s"), *InputTouchEvent->GetName()));
				InputTouchEvent->InputKeyEvent = InputEvent;
				InputTouchEvent->bConsumeInput = bConsumeInput;
				InputTouchEvent->bExecuteWhenPaused = bExecuteWhenPaused;
				InputTouchEvent->bOverrideParentBinding = bOverrideParentBinding;
				InputTouchEvent->EventSignatureName = TEXT("InputTouchHandlerDynamicSignature__DelegateSignature");
				InputTouchEvent->EventSignatureClass = UInputComponent::StaticClass();
				InputTouchEvent->bInternalEvent = true;
				InputTouchEvent->AllocateDefaultPins();

				Schema->MovePinLinks(*InputTouchPin, *Schema->FindExecutionPin(*InputTouchEvent, EGPD_Output));
				Schema->MovePinLinks(*GetLocationPin(), *InputTouchEvent->FindPin(TEXT("Location")));
				Schema->MovePinLinks(*GetFingerIndexPin(), *InputTouchEvent->FindPin(TEXT("FingerIndex")));
			}
		}
	}
}