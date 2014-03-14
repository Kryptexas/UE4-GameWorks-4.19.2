// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

UK2Node_MatineeController::UK2Node_MatineeController(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_MatineeController::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Preload the matinee data, if needed, so that we can have all the event tracks we need
	if (MatineeActor != NULL)
	{
		PreloadObject(MatineeActor);
		PreloadObject(MatineeActor->MatineeData);
	}

	// Create the "finished" playing pin
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_MatineeFinished);

	// Create pins for each event
	if(MatineeActor != NULL && MatineeActor->MatineeData != NULL)
	{
		TArray<FName> EventNames;
		MatineeActor->MatineeData->GetAllEventNames(EventNames);

		for(int32 i=0; i<EventNames.Num(); i++)
		{
			FName EventName = EventNames[i];
			CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, EventName.ToString());			
		}
	}

	Super::AllocateDefaultPins();
}


FLinearColor UK2Node_MatineeController::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.51f, 0.0f);
}


FString UK2Node_MatineeController::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if(MatineeActor != NULL)
	{
		return MatineeActor->GetActorLabel();
	}
	else
	{
		return NSLOCTEXT("K2Node", "InvalidMatineeController", "INVALID MATINEECONTROLLER").ToString();
	}
}

AActor* UK2Node_MatineeController::GetReferencedLevelActor() const
{
	return MatineeActor;
}

UEdGraphPin* UK2Node_MatineeController::GetFinishedPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	return FindPin(K2Schema->PN_MatineeFinished);
}

void UK2Node_MatineeController::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	if (SourceGraph != CompilerContext.ConsolidatedEventGraph)
	{
		CompilerContext.MessageLog.Error(*FString::Printf(*NSLOCTEXT("KismetCompiler", "InvalidNodeOutsideUbergraph_Error", "Unexpected node @@ found outside ubergraph.").ToString()), this);
		return;
	}

	Super::ExpandNode(CompilerContext, SourceGraph);

	if (MatineeActor != NULL)
	{
		UFunction* MatineeEventSig = FindObject<UFunction>(AMatineeActor::StaticClass(), TEXT("OnMatineeEvent__DelegateSignature"));
		check(MatineeEventSig != NULL);

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// Create event for each exec output pin
		for(int32 PinIdx=0; PinIdx<Pins.Num(); PinIdx++)
		{
			UEdGraphPin* MatineePin = Pins[PinIdx];
			if(MatineePin->Direction == EGPD_Output && MatineePin->PinType.PinCategory == Schema->PC_Exec)
			{
				FName EventFuncName = MatineeActor->GetFunctionNameForEvent( FName(*(MatineePin->PinName)) );

				UK2Node_Event* MatineeEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_Event>(this, SourceGraph);
				MatineeEventNode->EventSignatureName = MatineeEventSig->GetFName();
				MatineeEventNode->EventSignatureClass = AMatineeActor::StaticClass();
				MatineeEventNode->CustomFunctionName = EventFuncName;
				MatineeEventNode->bInternalEvent = true;
				MatineeEventNode->AllocateDefaultPins();

				// Move connection from matinee output to event node output
				UEdGraphPin* EventOutputPin = Schema->FindExecutionPin(*MatineeEventNode, EGPD_Output);
				check(EventOutputPin != NULL);
				CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*MatineePin, *EventOutputPin), this);
			}
		}
	}

}