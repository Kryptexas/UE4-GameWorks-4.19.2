// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

#include "EngineKismetLibraryClasses.h"

#define LOCTEXT_NAMESPACE "K2Node_Message"

UK2Node_Message::UK2Node_Message(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UK2Node_Message::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString NodeName;
	if (UFunction* Function = GetTargetFunction())
	{
		NodeName = UK2Node_CallFunction::GetUserFacingFunctionName(Function);
		NodeName += FString(TEXT("\n"));
		NodeName += FString::Printf(*NSLOCTEXT("K2Node", "CallInterfaceContext", "Using Interface %s").ToString(), *(Function->GetOuterUClass()->GetName()));
	}
	else
	{
		NodeName = NSLOCTEXT("K2Node", "InvalidMessageNode", "Invalid Message Node").ToString();
	}

	return NodeName;
}

UEdGraphPin* UK2Node_Message::CreateSelfPin(const UFunction* Function)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	UEdGraphPin* SelfPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UObject::StaticClass(), false, false, K2Schema->PN_Self);
	SelfPin->bDefaultValueIsIgnored = true;
	return SelfPin;
}

void UK2Node_Message::EnsureFunctionIsInBlueprint()
{
	// Do nothing; the function either exists and works, or doesn't and doesn't
}

FName UK2Node_Message::GetCornerIcon() const
{
	return TEXT("Graph.Message.MessageIcon");
}

FNodeHandlingFunctor* UK2Node_Message::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FNodeHandlingFunctor(CompilerContext);
}

void UK2Node_Message::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		UEdGraphPin* ExecPin = Schema->FindExecutionPin(*this, EGPD_Input);
		const bool bExecPinConnected = ExecPin && (ExecPin->LinkedTo.Num() > 0);
		UEdGraphPin* ThenPin = Schema->FindExecutionPin(*this, EGPD_Output);
		const bool bThenPinConnected = ThenPin && (ThenPin->LinkedTo.Num() > 0);

		// Skip ourselves if our exec isn't wired up
		if( bExecPinConnected )
		{
			// Make sure our interface is valid
			if (FunctionReference.GetMemberParentClass(this) == NULL)
			{
				CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("MessageNodeInvalid_Error", "Message node @@ has an invalid interface.").ToString()), this);
				return;
			}

			UFunction* MessageNodeFunction = GetTargetFunction();
			if (MessageNodeFunction == NULL)
			{
				//@TODO: Why do this here in teh compiler, it's already done on AllocateDefaultPins() during on-load node reconstruction
				MessageNodeFunction = Cast<UFunction>(UK2Node::FindRemappedField(FunctionReference.GetMemberParentClass(this), FunctionReference.GetMemberName()));
			}

			if (MessageNodeFunction == NULL)
			{
				CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("MessageNodeInvalidFunction_Error", "Unable to find function with name %s for Message node @@.").ToString(), *(FunctionReference.GetMemberName().ToString())), this);
				return;
			}

			// Check to make sure we have a target
			UEdGraphPin* MessageSelfPin = Schema->FindSelfPin(*this, EGPD_Input);
			if( !MessageSelfPin || MessageSelfPin->LinkedTo.Num() == 0 )
			{
				CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("MessageNodeSelfPin_Error", "Message node @@ must have a self pin connection.").ToString()), this);
				return;
			}

			// First, create an internal cast-to-interface node
			UK2Node_CastToInterface* CastToInterfaceNode = CompilerContext.SpawnIntermediateNode<UK2Node_CastToInterface>(this, SourceGraph);
			CastToInterfaceNode->TargetType = MessageNodeFunction->GetOuterUClass();
			CastToInterfaceNode->AllocateDefaultPins();

			UEdGraphPin* CastToInterfaceValidPin = CastToInterfaceNode->GetValidCastPin();
			UEdGraphPin* CastToInterfaceResultPin = CastToInterfaceNode->GetCastResultPin();
			if( !CastToInterfaceResultPin )
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("InvalidInterfaceClass_Error", "Node @@ has an invalid target interface class").ToString(), this);
				return;
			}

			CastToInterfaceResultPin->PinType.PinSubCategoryObject = *CastToInterfaceNode->TargetType;

			// Wire up the connections
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*ExecPin, *CastToInterfaceNode->GetExecPin()), this);

			UEdGraphPin* CastToInterfaceSourceObjectPin = CastToInterfaceNode->GetCastSourcePin();
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*MessageSelfPin, *CastToInterfaceSourceObjectPin), this);

			// Next, create the function call node
			UK2Node_CallFunction* FunctionCallNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
			FunctionCallNode->bIsInterfaceCall = true;
			FunctionCallNode->FunctionReference = FunctionReference;
			FunctionCallNode->AllocateDefaultPins();

			// Wire up the connections
			UEdGraphPin* CallFunctionExecPin = Schema->FindExecutionPin(*FunctionCallNode, EGPD_Input);
			CastToInterfaceValidPin->MakeLinkTo(CallFunctionExecPin);

			// Self pin
			UEdGraphPin* FunctionCallSelfPin = Schema->FindSelfPin(*FunctionCallNode, EGPD_Input);
			CastToInterfaceResultPin->MakeLinkTo(FunctionCallSelfPin);

			UEdGraphPin* LastOutCastFaildPin = CastToInterfaceNode->GetInvalidCastPin();

			UFunction* ArrayClearFunction = UKismetArrayLibrary::StaticClass()->FindFunctionByName(FName(TEXT("Array_Clear")));
			check(ArrayClearFunction);

			// Variable pins - Try to associate variable inputs to the message node with the variable inputs and outputs to the call function node
			for( int32 i = 0; i < Pins.Num(); i++ )
			{
				UEdGraphPin* CurrentPin = Pins[i];
				if( CurrentPin && (CurrentPin->PinType.PinCategory != Schema->PC_Exec) && (CurrentPin->PinName != Schema->PN_Self) )
				{
					// Try to find a match for the pin on the function call node
					UEdGraphPin* FunctionCallPin = FunctionCallNode->FindPin(CurrentPin->PinName);
					if( FunctionCallPin )
					{
						// Move pin links if the pin is connected...
						CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*CurrentPin, *FunctionCallPin), this);

						// when cast fails all return values must be cleared.
						if (EEdGraphPinDirection::EGPD_Output == CurrentPin->Direction)
						{
							UK2Node* DefaultValueNode = NULL;
							UEdGraphPin* DefaultValueThenPin = NULL;
							if (CurrentPin->PinType.bIsArray)
							{
								UK2Node_CallArrayFunction* ClearArray = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
								DefaultValueNode = ClearArray;
								ClearArray->SetFromFunction(ArrayClearFunction);
								ClearArray->AllocateDefaultPins();

								UEdGraphPin* ArrayPin = ClearArray->GetTargetArrayPin();
								check(ArrayPin);
								Schema->TryCreateConnection(ArrayPin, FunctionCallPin);
								ClearArray->PinConnectionListChanged(ArrayPin);

								DefaultValueThenPin = ClearArray->GetThenPin();
							} 
							else
							{
								UK2Node_AssignmentStatement* AssignDefaultValue = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
								DefaultValueNode = AssignDefaultValue;
								AssignDefaultValue->AllocateDefaultPins();

								Schema->TryCreateConnection(AssignDefaultValue->GetVariablePin(), FunctionCallPin);
								AssignDefaultValue->PinConnectionListChanged(AssignDefaultValue->GetVariablePin());
								Schema->SetPinDefaultValueBasedOnType(AssignDefaultValue->GetValuePin());

								DefaultValueThenPin = AssignDefaultValue->GetThenPin();
							}
							UEdGraphPin* DefaultValueExecPin = DefaultValueNode->GetExecPin();
							check(DefaultValueExecPin);
							Schema->TryCreateConnection(DefaultValueExecPin, LastOutCastFaildPin);
							LastOutCastFaildPin = DefaultValueThenPin;
							check(LastOutCastFaildPin);
						}
					}
					else
					{
						UE_LOG(LogK2Compiler, Log, TEXT("%s"), *LOCTEXT("NoPinConnectionFound_Error", "Unable to find connection for pin!  Check AllocateDefaultPins() for consistency!").ToString());
					}
				}
			}

			if( bThenPinConnected )
			{
				// Failure case for the cast runs straight through to the exit
				CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*ThenPin, *LastOutCastFaildPin), this);

				// Copy all links from the invalid cast case above to the call function node
				if (UEdGraphPin* CallFunctionThenPin = Schema->FindExecutionPin(*FunctionCallNode, EGPD_Output))
				{
					for( int32 i = 0; i < LastOutCastFaildPin->LinkedTo.Num(); i++ )
					{
						UEdGraphPin* Pin = LastOutCastFaildPin->LinkedTo[i];
						FPinConnectionResponse Response = Schema->CanCreateConnection(CallFunctionThenPin, Pin);
						if(Response.CanSafeConnect())
						{
							CallFunctionThenPin->MakeLinkTo(Pin);
						}
						else
						{
							CompilerContext.CheckConnectionResponse(Response, this);
						}
					}
				}
				else
				{
					CompilerContext.MessageLog.Warning(*LOCTEXT("ExpectedThenPinOnInterfaceCall", "Expected a then pin on the call in @@.  Does the interface method still exist?").ToString(), FunctionCallNode);
				}
			}
		}

		// Break all connections to the original node, so it will be pruned
		BreakAllNodeLinks();
	}
}

#undef LOCTEXT_NAMESPACE