// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "K2Node_MultiGate"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_ExecutionSequence

class FKCHandler_ExecutionSequence : public FNodeHandlingFunctor
{
public:
	FKCHandler_ExecutionSequence(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		// Make sure that the input pin is connected and valid for this block
		UEdGraphPin* ExecTriggeringPin = Context.FindRequiredPinByName(Node, CompilerContext.GetSchema()->PN_Execute, EGPD_Input);
		if ((ExecTriggeringPin == NULL) || !Context.ValidatePinType(ExecTriggeringPin, CompilerContext.GetSchema()->PC_Exec))
		{
			CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("NoValidExecutionPinForExecSeq_Error", "@@ must have a valid execution pin @@").ToString()), Node, ExecTriggeringPin);
			return;
		}
		else if (ExecTriggeringPin->LinkedTo.Num() == 0)
		{
			CompilerContext.MessageLog.Warning(*FString::Printf(*LOCTEXT("NodeNeverExecuted_Warning", "@@ will never be executed").ToString()), Node);
			return;
		}

		// Find the valid, connected output pins, and add them to the processing list
		TArray<UEdGraphPin*> OutputPins;
		for (int32 i = 0; i < Node->Pins.Num(); i++)
		{
			UEdGraphPin* CurrentPin = Node->Pins[i];
			if ((CurrentPin->Direction == EGPD_Output) && (CurrentPin->PinName.StartsWith(CompilerContext.GetSchema()->PN_Then)) && (CurrentPin->LinkedTo.Num() > 0))
			{
				OutputPins.Add(CurrentPin);
			}
		}

		//@TODO: Sort the pins by the number appended to the pin!

		// Process the pins, if there are any valid entries
		if (OutputPins.Num() > 0)
		{
			const bool bGenerateDebugSites = true;
			if ((KismetCompilerDebugOptions::EmitDebuggingPlaceholders) && (OutputPins.Num() > 1))
			{
				const FString NodeComment = Node->NodeComment.IsEmpty() ? Node->GetName() : Node->NodeComment;

				// Assuming sequence X goes to A, B, C, we want to emit:
				//   X: push X1
				//      goto A
				//  X1: debug site
				//      push X2
				//      goto B
				//  X2: debug site
				//      goto C

				// A push statement we need to patch up on the next pass (e.g., push X1 before we know where X1 is)
				FBlueprintCompiledStatement* LastPushStatement = NULL;

				for (int32 i = 0; i < OutputPins.Num(); ++i)
				{
					UEdGraphNode* StateNode = OutputPins[i]->LinkedTo[0]->GetOwningNode();

					// Emit the debug site and patch up the previous jump if we're on subsequent steps
					const bool bNotFirstIndex = i > 0;
					if (bNotFirstIndex)
					{
						// Emit a debug site
						FBlueprintCompiledStatement& DebugSiteAndJumpTarget = Context.AppendStatementForNode(Node);
						DebugSiteAndJumpTarget.Type = KCST_DebugSite;
						DebugSiteAndJumpTarget.Comment = NodeComment;
						DebugSiteAndJumpTarget.bIsJumpTarget = true;

						// Patch up the previous push jump target
						check(LastPushStatement);
						LastPushStatement->TargetLabel = &DebugSiteAndJumpTarget;
					}

					// Emit a push to get to the next step in the sequence, unless we're the last one
					const bool bNotLastIndex = (i + 1) < OutputPins.Num();
					if (bNotLastIndex)
					{
						FBlueprintCompiledStatement& PushExecutionState = Context.AppendStatementForNode(Node);
						PushExecutionState.Type = KCST_PushState;
						LastPushStatement = &PushExecutionState;
					}

					// Emit the goto to the actual state
					FBlueprintCompiledStatement& GotoSequenceLinkedState = Context.AppendStatementForNode(Node);
					GotoSequenceLinkedState.Type = KCST_UnconditionalGoto;
					Context.GotoFixupRequestMap.Add(&GotoSequenceLinkedState, StateNode);
				}
			}
			else
			{
				// Directly emit pushes to execute the remaining branches
				for (int32 i = OutputPins.Num() - 1; i > 0; i--)
				{
					UEdGraphNode* StateNode = OutputPins[i]->LinkedTo[0]->GetOwningNode();
					FBlueprintCompiledStatement& PushExecutionState = Context.AppendStatementForNode(Node);
					PushExecutionState.Type = KCST_PushState;
					Context.GotoFixupRequestMap.Add(&PushExecutionState, StateNode);
				}

				// Immediately jump to the first pin
				UEdGraphNode* NextNode = OutputPins[0]->LinkedTo[0]->GetOwningNode();
				FBlueprintCompiledStatement& NextExecutionState = Context.AppendStatementForNode(Node);
				NextExecutionState.Type = KCST_UnconditionalGoto;
				Context.GotoFixupRequestMap.Add(&NextExecutionState, NextNode);
			}
		}
	}
};

UK2Node_ExecutionSequence::UK2Node_ExecutionSequence(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_ExecutionSequence::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);

	// Add two default pins
	for (int32 i = 0; i < 2; ++i)
	{
		CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, GetPinNameGivenIndex(i));
	}

	Super::AllocateDefaultPins();
}

FString UK2Node_ExecutionSequence::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "Sequence", "Sequence").ToString();
}

FLinearColor UK2Node_ExecutionSequence::GetNodeTitleColor() const
{
	return FLinearColor::White;
}

FString UK2Node_ExecutionSequence::GetTooltip() const
{
	return NSLOCTEXT("K2Node", "ExecutePinInOrder_Tooltip", "Executes a series of pins in order").ToString();
}

FString UK2Node_ExecutionSequence::GetUniquePinName()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	FString NewPinName;
	int32 i = 0;
	while (true)
	{
		NewPinName = GetPinNameGivenIndex(i++);
		if (!FindPin(NewPinName))
		{
			break;
		}
	}

	return NewPinName;
}

void UK2Node_ExecutionSequence::AddPinToExecutionNode()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, GetUniquePinName());
}

void UK2Node_ExecutionSequence::RemovePinFromExecutionNode(UEdGraphPin* TargetPin) 
{
	UK2Node_ExecutionSequence* OwningSeq = Cast<UK2Node_ExecutionSequence>( TargetPin->GetOwningNode() );
	if (OwningSeq)
	{
		TargetPin->BreakAllPinLinks();
		OwningSeq->Pins.Remove(TargetPin);

		// Renumber the pins so the numbering is compact
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		int32 ThenIndex = 0;
		for (int32 i = 0; i < OwningSeq->Pins.Num(); ++i)
		{
			UEdGraphPin* PotentialPin = OwningSeq->Pins[i];
			if (K2Schema->IsExecPin(*PotentialPin) && (PotentialPin->Direction == EGPD_Output))
			{
				PotentialPin->PinName = GetPinNameGivenIndex(ThenIndex);
				++ThenIndex;
			}
		}
	}
}

bool UK2Node_ExecutionSequence::CanRemoveExecutionPin() const
{
	int32 NumOutPins = 0;

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	for (int32 i = 0; i < Pins.Num(); ++i)
	{
		UEdGraphPin* PotentialPin = Pins[i];
		if (K2Schema->IsExecPin(*PotentialPin) && (PotentialPin->Direction == EGPD_Output))
		{
			NumOutPins++;
		}
	}

	return (NumOutPins > 2);
}

FString UK2Node_ExecutionSequence::GetPinNameGivenIndex(int32 Index) const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	return K2Schema->PN_Then + FString::Printf(TEXT("_%d"), Index);
}

void UK2Node_ExecutionSequence::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::AllocateDefaultPins();

	// Create the execution input pin
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);

	// Create a new pin for each old execution output pin, and coerce the names to match on both sides
	int32 ExecOutPinCount = 0;
	for (int32 i = 0; i < OldPins.Num(); ++i)
	{
		UEdGraphPin* TestPin = OldPins[i];
		if (K2Schema->IsExecPin(*TestPin) && (TestPin->Direction == EGPD_Output))
		{
			FString NewPinName = GetPinNameGivenIndex(ExecOutPinCount);
			ExecOutPinCount++;

			// Make sure the old pin and new pin names match
			TestPin->PinName = NewPinName;

			// Create the new output pin to match
			CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, NewPinName);
		}
	}
}

UEdGraphPin * UK2Node_ExecutionSequence::GetThenPinGivenIndex(int32 Index)
{
	FString PinName = GetPinNameGivenIndex(Index);
	return FindPin(PinName);
}

FNodeHandlingFunctor* UK2Node_ExecutionSequence::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_ExecutionSequence(CompilerContext);
}

#undef LOCTEXT_NAMESPACE
