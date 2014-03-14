// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "ClassIconFinder.h"

#define LOCTEXT_NAMESPACE "K2Node_Literal"

class FKCHandler_LiteralStatement : public FNodeHandlingFunctor
{
public:
	FKCHandler_LiteralStatement(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) OVERRIDE
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		UK2Node_Literal* LiteralNode = Cast<UK2Node_Literal>(Net->GetOwningNode());
		check(LiteralNode);

		UObject* TargetObject = LiteralNode->GetObjectRef();

		if( !TargetObject )
		{
			CompilerContext.MessageLog.Warning(*LOCTEXT("InvalidLevelActor_Warning", "Node @@ is not referencing a valid level actor").ToString(), LiteralNode);
		}

		const FString TargetObjectName = TargetObject ? TargetObject->GetPathName() : TEXT("None");

		// First, try to see if we already have a term for this object
		FBPTerminal* Term = NULL;
		for( int32 i = 0; i < Context.LevelActorReferences.Num(); i++ )
		{
			FBPTerminal& CurrentTerm = Context.LevelActorReferences[i];
			if( CurrentTerm.PropertyDefault == TargetObjectName )
			{
				Term = &CurrentTerm;
				break;
			}
		}

		// If we didn't find one, then create a new term
		if( !Term )
		{
			FString RefPropName = (TargetObject ? TargetObject->GetName() : TEXT("None")) + TEXT("_") + (Context.SourceGraph ? *Context.SourceGraph->GetName() : TEXT("None")) + TEXT("_RefProperty");
			Term = new (Context.LevelActorReferences) FBPTerminal();
			Term->CopyFromPin(Net, Context.NetNameMap->MakeValidName(Net));
			Term->Name = RefPropName;
			Term->PropertyDefault = TargetObjectName;
		}

		check(Term);

		Context.NetMap.Add(Net, Term);
	}
};

UK2Node_Literal::UK2Node_Literal(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

static FString ValuePinName(TEXT("Value"));

void UK2Node_Literal::AllocateDefaultPins()
{
	// The literal node only has one pin:  an output of the desired value, on a wildcard pin type
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Output, Schema->PC_Object, TEXT(""), NULL, false, false, *ValuePinName);

	// After allocating the pins, try to coerce pin type
	SetObjectRef( ObjectRef );
}

void UK2Node_Literal::PostReconstructNode()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	UEdGraphPin* ValuePin = GetValuePin();

	// See if we need to fix up the value pin to have a valid type after reconstruction.  Could be invalid with a stale object ref
	if( ValuePin && !ObjectRef && (ValuePin->LinkedTo.Num() > 0) )
	{
		// Loop over the node, and figure out the most derived class connected to this pin, and use that, so everyone is happy
		UClass* PinSubtype = NULL;
		for( TArray<UEdGraphPin*>::TIterator PinIt(ValuePin->LinkedTo); PinIt; ++PinIt )
		{
			UClass* TestType = Cast<UClass>((*PinIt)->PinType.PinSubCategoryObject.Get());
			if( !TestType )
			{
				// If this isn't a class, we're connected to something we shouldn't be.  Bail and make the scripter fix it up.
				return;
			}
			else
			{
				if( TestType && (!PinSubtype || (PinSubtype != TestType && TestType->IsChildOf(PinSubtype))) )
				{
					PinSubtype = TestType;
				}
			}
		}

		const UEdGraphPin* ConnectedPin = ValuePin->LinkedTo[0];
		ValuePin->PinType = ConnectedPin->PinType;
		ValuePin->PinType.PinSubCategoryObject = PinSubtype;
	}
}

FString UK2Node_Literal::GetTooltip() const
{
	return NSLOCTEXT("K2Node", "Literal_Tooltip", "Stores a reference to an actor in the level").ToString();
}

FString UK2Node_Literal::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if( ObjectRef != NULL )
	{
		AActor* Actor = Cast<AActor>(ObjectRef);
		if(Actor != NULL)
		{
			return Actor->GetActorLabel();
		}
		else
		{
			return ObjectRef->GetName();
		}
	}
	else
	{
		return NSLOCTEXT("K2Node", "Unknown", "Unknown").ToString();
	}
}

FLinearColor UK2Node_Literal::GetNodeTitleColor() const
{
	UEdGraphPin* ValuePin = GetValuePin();
	if( ValuePin )
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		return Schema->GetPinTypeColor(ValuePin->PinType);		
	}
	else
	{
		return Super::GetNodeTitleColor();
	}
}

UK2Node::ERedirectType UK2Node_Literal::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	// This allows the value pin (the only pin) to stay connected through reconstruction, even if the name changes due to an actor in the renamed, etc
	return ERedirectType_Name;
}

AActor* UK2Node_Literal::GetReferencedLevelActor() const
{
	return Cast<AActor>(ObjectRef);
}

UEdGraphPin* UK2Node_Literal::GetValuePin() const
{
	return (Pins.Num() > 0) ? Pins[0] : NULL;
}

void UK2Node_Literal::SetObjectRef(UObject* NewValue)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	UEdGraphPin* ValuePin = GetValuePin();

	// First, see if this is an object
	if( NewValue )
	{
		ObjectRef = NewValue;

		// Set the pin type to reflect the object we're referencing
		if( ValuePin )
		{
			ValuePin->PinType.PinCategory = Schema->PC_Object;
			ValuePin->PinType.PinSubCategory = TEXT("");
			ValuePin->PinType.PinSubCategoryObject = ObjectRef->GetClass();
		}
	}

	if( ValuePin )
	{
		ValuePin->PinName = GetNodeTitle(ENodeTitleType::ListView);
	}
}

FNodeHandlingFunctor* UK2Node_Literal::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_LiteralStatement(CompilerContext);
}

FName UK2Node_Literal::GetPaletteIcon(FLinearColor& OutColor) const
{
	if(ObjectRef != NULL)
	{
		AActor* Actor = Cast<AActor>(ObjectRef);
		if(Actor != NULL)
		{
			return FClassIconFinder::FindIconNameForActor(Actor);
		}
		else
		{
			return FClassIconFinder::FindIconNameForClass(ObjectRef->GetClass());
		}	
	}
	return Super::GetPaletteIcon(OutColor);
}

#undef LOCTEXT_NAMESPACE
