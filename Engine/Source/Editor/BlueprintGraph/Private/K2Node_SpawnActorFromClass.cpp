// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

struct FK2Node_SpawnActorFromClassHelper
{
	static FString WorldContextPinName;
	static FString ClassPinName;
	static FString SpawnTransformPinName;
	static FString NoCollisionFailPinName;
};

FString FK2Node_SpawnActorFromClassHelper::WorldContextPinName(TEXT("WorldContextObject"));
FString FK2Node_SpawnActorFromClassHelper::ClassPinName(TEXT("Class"));
FString FK2Node_SpawnActorFromClassHelper::SpawnTransformPinName(TEXT("SpawnTransform"));
FString FK2Node_SpawnActorFromClassHelper::NoCollisionFailPinName(TEXT("SpawnEvenIfColliding"));

#define LOCTEXT_NAMESPACE "K2Node_SpawnActorFromClass"

UK2Node_SpawnActorFromClass::UK2Node_SpawnActorFromClass(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Attempts to spawn a new Actor with the specified transform").ToString();
}

void UK2Node_SpawnActorFromClass::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add execution pins
	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	// If required add the world context pin
	if (GetBlueprint()->ParentClass->HasMetaData(FBlueprintMetadata::MD_ShowHiddenSelfPins))
	{
		CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UObject::StaticClass(), false, false, FK2Node_SpawnActorFromClassHelper::WorldContextPinName);
	}

	// Add blueprint pin
	UEdGraphPin* ClassPin = CreatePin(EGPD_Input, K2Schema->PC_Class, TEXT(""), AActor::StaticClass(), false, false, FK2Node_SpawnActorFromClassHelper::ClassPinName);
	SetPinToolTip(*ClassPin, LOCTEXT("ClassPinDescription", "The Actor class you want to spawn"));

	// Transform pin
	UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
	UEdGraphPin* TransformPin = CreatePin(EGPD_Input, K2Schema->PC_Struct, TEXT(""), TransformStruct, false, false, FK2Node_SpawnActorFromClassHelper::SpawnTransformPinName);
	SetPinToolTip(*TransformPin, LOCTEXT("TransformPinDescription", "The transform to spawn the Actor with"));

	// bNoCollisionFail pin
	UEdGraphPin* NoCollisionFailPin = CreatePin(EGPD_Input, K2Schema->PC_Boolean, TEXT(""), NULL, false, false, FK2Node_SpawnActorFromClassHelper::NoCollisionFailPinName);
	SetPinToolTip(*NoCollisionFailPin, LOCTEXT("NoCollisionFailPinDescription", "Determines if the Actor should be spawned when the location is blocked by a collision"));

	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Object, TEXT(""), AActor::StaticClass(), false, false, K2Schema->PN_ReturnValue);
	SetPinToolTip(*ResultPin, LOCTEXT("ResultPinDescription", "The spawned Actor"));

	Super::AllocateDefaultPins();
}

void UK2Node_SpawnActorFromClass::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
{
	MutatablePin.PinToolTip = UEdGraphSchema_K2::TypeToString(MutatablePin.PinType);

	UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	if (K2Schema != nullptr)
	{
		MutatablePin.PinToolTip += TEXT(" ");
		MutatablePin.PinToolTip += K2Schema->GetPinDisplayName(&MutatablePin);
	}

	MutatablePin.PinToolTip += FString(TEXT("\n")) + PinDescription.ToString();
}


void UK2Node_SpawnActorFromClass::CreatePinsForClass(UClass* InClass)
{
	check(InClass != NULL);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	for (TFieldIterator<UProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		UClass* PropertyClass = CastChecked<UClass>(Property->GetOuter());
		const bool bIsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
		const bool bIsExposedToSpawn = Property->HasMetaData(FBlueprintMetadata::MD_ExposeOnSpawn);

		const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance)
			|| !Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly);//@TODO: Remove this after old content is fixed up

		if(	bIsExposedToSpawn &&
			!Property->HasAnyPropertyFlags(CPF_Parm) && 
			bIsSettableExternally &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
			!bIsDelegate )
		{
			UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, false, Property->GetName());
			const bool bPinGood = (Pin != NULL) && K2Schema->ConvertPropertyToPinType(Property, /*out*/ Pin->PinType);	
			Pin->bDefaultValueIsIgnored = true;
		}
	}

	// Change class of output pin
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = InClass;
}

UClass* UK2Node_SpawnActorFromClass::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
{
	UClass* UseSpawnClass = NULL;
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* ClassPin = GetClassPin(PinsToSearch);
	if(ClassPin && ClassPin->DefaultObject != NULL && ClassPin->LinkedTo.Num() == 0)
	{
		UseSpawnClass = CastChecked<UClass>(ClassPin->DefaultObject);
	}

	return UseSpawnClass;
}


void UK2Node_SpawnActorFromClass::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) 
{
	AllocateDefaultPins();
	UClass* UseSpawnClass = GetClassToSpawn(&OldPins);
	if( UseSpawnClass != NULL )
	{
		CreatePinsForClass(UseSpawnClass);
	}
}

bool UK2Node_SpawnActorFromClass::IsSpawnVarPin(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	return(	Pin->PinName != K2Schema->PN_Execute &&
			Pin->PinName != K2Schema->PN_Then &&
			Pin->PinName != K2Schema->PN_ReturnValue &&
			Pin->PinName != FK2Node_SpawnActorFromClassHelper::ClassPinName &&
			Pin->PinName != FK2Node_SpawnActorFromClassHelper::WorldContextPinName &&
			Pin->PinName != FK2Node_SpawnActorFromClassHelper::NoCollisionFailPinName &&
			Pin->PinName != FK2Node_SpawnActorFromClassHelper::SpawnTransformPinName );
}


void UK2Node_SpawnActorFromClass::PinDefaultValueChanged(UEdGraphPin* Pin) 
{
	if(Pin->PinName == FK2Node_SpawnActorFromClassHelper::ClassPinName)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		// Because the archetype has changed, we break the output link as the output pin type will change
		UEdGraphPin* ResultPin = GetResultPin();
		ResultPin->BreakAllPinLinks();

		// Remove all pins related to archetype variables
		TArray<UEdGraphPin*> OldPins = Pins;
		for(int32 i=0; i<OldPins.Num(); i++)
		{
			UEdGraphPin* OldPin = OldPins[i];
			if(	IsSpawnVarPin(OldPin) )
			{
				Pin->BreakAllPinLinks();
				Pins.Remove(OldPin);
			}
		}

		UClass* UseSpawnClass = GetClassToSpawn();
		if(UseSpawnClass != NULL)
		{
			CreatePinsForClass(UseSpawnClass);
		}

		// Refresh the UI for the graph so the pin changes show up
		UEdGraph* Graph = GetGraph();
		Graph->NotifyGraphChanged();

		// Mark dirty
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
	}
}

FString UK2Node_SpawnActorFromClass::GetTooltip() const
{
	return NodeTooltip;
}

UEdGraphPin* UK2Node_SpawnActorFromClass::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActorFromClass::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
{
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* Pin = NULL;
	for( auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt )
	{
		UEdGraphPin* TestPin = *PinIt;
		if( TestPin && TestPin->PinName == FK2Node_SpawnActorFromClassHelper::ClassPinName )
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActorFromClass::GetSpawnTransformPin() const
{
	UEdGraphPin* Pin = FindPinChecked(FK2Node_SpawnActorFromClassHelper::SpawnTransformPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}


UEdGraphPin* UK2Node_SpawnActorFromClass::GetNoCollisionFailPin() const
{
	UEdGraphPin* Pin = FindPinChecked(FK2Node_SpawnActorFromClassHelper::NoCollisionFailPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActorFromClass::GetWorldContextPin() const
{
	UEdGraphPin* Pin = FindPin(FK2Node_SpawnActorFromClassHelper::WorldContextPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActorFromClass::GetResultPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

FLinearColor UK2Node_SpawnActorFromClass::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}


FString UK2Node_SpawnActorFromClass::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraphPin* ClassPin = GetClassPin();

	FString SpawnString = NSLOCTEXT("K2Node", "None", "NONE").ToString();
	if(ClassPin != NULL)
	{
		if(ClassPin->LinkedTo.Num() > 0)
		{
			// Blueprint will be determined dynamically, so we don't have the name in this case
			SpawnString = TEXT("");
		}
		else if(ClassPin->DefaultObject != NULL)
		{
			SpawnString = ClassPin->DefaultObject->GetName();
		}
	}

	return FString::Printf(*NSLOCTEXT("K2Node", "SpawnActor", "SpawnActor %s").ToString(), *SpawnString);
}

bool UK2Node_SpawnActorFromClass::CanPasteHere( const UEdGraph* TargetGraph, const UEdGraphSchema* Schema ) const 
{
	UBlueprint* Blueprint = Cast<UBlueprint>(TargetGraph->GetOuter());
	return CanCreateUnderSpecifiedSchema(Schema) && (!Blueprint || FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph);
}

void UK2Node_SpawnActorFromClass::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		static FName BeginSpawningBlueprintFuncName = GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginSpawningActorFromClass);
		static FString ActorClassParamName = FString(TEXT("ActorClass"));
		static FString WorldContextParamName = FString(TEXT("WorldContextObject"));

		static FName FinishSpawningFuncName = GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor);
		static FString ActorParamName = FString(TEXT("Actor"));
		static FString TransformParamName = FString(TEXT("SpawnTransform"));
		static FString NoCollisionFailParamName = FString(TEXT("bNoCollisionFail"));

		static FString ObjectParamName = FString(TEXT("Object"));
		static FString ValueParamName = FString(TEXT("Value"));
		static FString PropertyNameParamName = FString(TEXT("PropertyName"));

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		UK2Node_SpawnActorFromClass* SpawnNode = this;
		UEdGraphPin* SpawnNodeExec = SpawnNode->GetExecPin();
		UEdGraphPin* SpawnNodeTransform = SpawnNode->GetSpawnTransformPin();
		UEdGraphPin* SpawnNodeNoCollisionFail = GetNoCollisionFailPin();
		UEdGraphPin* SpawnWorldContextPin = SpawnNode->GetWorldContextPin();
		UEdGraphPin* SpawnClassPin = SpawnNode->GetClassPin();
		UEdGraphPin* SpawnNodeThen = SpawnNode->GetThenPin();
		UEdGraphPin* SpawnNodeResult = SpawnNode->GetResultPin();

		UClass* SpawnClass = (SpawnClassPin != NULL) ? Cast<UClass>(SpawnClassPin->DefaultObject) : NULL;
		if((0 == SpawnClassPin->LinkedTo.Num()) && (NULL == SpawnClass))
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("SpawnActorNodeMissingClass_Error", "Spawn node @@ must have a class specified.").ToString(), SpawnNode);
			// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
			SpawnNode->BreakAllNodeLinks();
			return;
		}

		//////////////////////////////////////////////////////////////////////////
		// create 'begin spawn' call node
		UK2Node_CallFunction* CallBeginSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
		CallBeginSpawnNode->FunctionReference.SetExternalMember(BeginSpawningBlueprintFuncName, UGameplayStatics::StaticClass());
		CallBeginSpawnNode->AllocateDefaultPins();

		UEdGraphPin* CallBeginExec = CallBeginSpawnNode->GetExecPin();
		UEdGraphPin* CallBeginWorldContextPin = CallBeginSpawnNode->FindPinChecked(WorldContextParamName);
		UEdGraphPin* CallBeginActorClassPin = CallBeginSpawnNode->FindPinChecked(ActorClassParamName);
		UEdGraphPin* CallBeginTransform = CallBeginSpawnNode->FindPinChecked(TransformParamName);
		UEdGraphPin* CallBeginNoCollisionFail = CallBeginSpawnNode->FindPinChecked(NoCollisionFailParamName);
		UEdGraphPin* CallBeginResult = CallBeginSpawnNode->GetReturnValuePin();

		// Move 'exec' connection from spawn node to 'begin spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeExec, *CallBeginExec), SpawnNode);

		if(SpawnClassPin->LinkedTo.Num() > 0)
		{
			// Copy the 'blueprint' connection from the spawn node to 'begin spawn'
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnClassPin, *CallBeginActorClassPin), SpawnNode);
		}
		else
		{
			// Copy blueprint literal onto begin spawn call 
			CallBeginActorClassPin->DefaultObject = SpawnClass;
		}

		// Copy the world context connection from the spawn node to 'begin spawn' if necessary
		if (SpawnWorldContextPin)
		{
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnWorldContextPin, *CallBeginWorldContextPin), SpawnNode);
		}

		// Copy the 'transform' connection from the spawn node to 'begin spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeTransform, *CallBeginTransform), SpawnNode);

		// Copy the 'bNoCollisionFail' connection from the spawn node to 'begin spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeNoCollisionFail, *CallBeginNoCollisionFail), this);

		//////////////////////////////////////////////////////////////////////////
		// create 'finish spawn' call node
		UK2Node_CallFunction* CallFinishSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
		CallFinishSpawnNode->FunctionReference.SetExternalMember(FinishSpawningFuncName, UGameplayStatics::StaticClass());
		CallFinishSpawnNode->AllocateDefaultPins();

		UEdGraphPin* CallFinishExec = CallFinishSpawnNode->GetExecPin();
		UEdGraphPin* CallFinishThen = CallFinishSpawnNode->GetThenPin();
		UEdGraphPin* CallFinishActor = CallFinishSpawnNode->FindPinChecked(ActorParamName);
		UEdGraphPin* CallFinishTransform = CallFinishSpawnNode->FindPinChecked(TransformParamName);
		UEdGraphPin* CallFinishResult = CallFinishSpawnNode->GetReturnValuePin();

		// Move 'then' connection from spawn node to 'finish spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeThen, *CallFinishThen), SpawnNode);

		// Copy transform connection
		CompilerContext.CheckConnectionResponse(Schema->CopyPinLinks(*CallBeginTransform, *CallFinishTransform), SpawnNode);

		// Connect output actor from 'begin' to 'finish'
		CallBeginResult->MakeLinkTo(CallFinishActor);

		// Move result connection from spawn node to 'finish spawn'
		CallFinishResult->PinType = SpawnNodeResult->PinType; // Copy type so it uses the right actor subclass
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeResult, *CallFinishResult), SpawnNode);

		//////////////////////////////////////////////////////////////////////////
		// create 'set var' nodes

		// Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
		UK2Node_CallFunction* LastNode = CallBeginSpawnNode;

		// Create 'set var by name' nodes and hook them up
		for(int32 PinIdx=0; PinIdx < SpawnNode->Pins.Num(); PinIdx++)
		{
			// Only create 'set param by name' node if this pin is linked to something
			UEdGraphPin* SpawnVarPin = SpawnNode->Pins[PinIdx];
			if(SpawnVarPin->LinkedTo.Num() > 0)
			{
				UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(SpawnVarPin->PinType);
				if(SetByNameFunction)
				{
					UK2Node_CallFunction* SetVarNode = NULL;
					if(SpawnVarPin->PinType.bIsArray)
					{
						SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(SpawnNode, SourceGraph);
					}
					else
					{
						SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
					}
					SetVarNode->SetFromFunction(SetByNameFunction);
					SetVarNode->AllocateDefaultPins();

					// Connect this node into the exec chain
					UEdGraphPin* LastThen = LastNode->GetThenPin();
					UEdGraphPin* SetVarExec = SetVarNode->GetExecPin();
					LastThen->MakeLinkTo(SetVarExec);

					// Connect the new actor to the 'object' pin
					UEdGraphPin* ObjectPin = SetVarNode->FindPinChecked(ObjectParamName);
					CallBeginResult->MakeLinkTo(ObjectPin);

					// Fill in literal for 'property name' pin - name of pin is property name
					UEdGraphPin* PropertyNamePin = SetVarNode->FindPinChecked(PropertyNameParamName);
					PropertyNamePin->DefaultValue = SpawnVarPin->PinName;

					// Move connection from the variable pin on the spawn node to the 'value' pin
					UEdGraphPin* ValuePin = SetVarNode->FindPinChecked(ValueParamName);
					CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnVarPin, *ValuePin), SpawnNode);
					if(SpawnVarPin->PinType.bIsArray)
					{
						SetVarNode->PinConnectionListChanged(ValuePin);
					}

					// Update 'last node in sequence' var
					LastNode = SetVarNode;
				}
			}
		}

		// Make exec connection between 'then' on last node and 'finish'
		UEdGraphPin* LastThen = LastNode->GetThenPin();
		LastThen->MakeLinkTo(CallFinishExec);

		// Break any links to the expanded node
		SpawnNode->BreakAllNodeLinks();
	}
}

bool UK2Node_SpawnActorFromClass::HasExternalBlueprintDependencies() const
{
	const UClass* SourceClass = GetClassToSpawn();
	const UBlueprint* SourceBlueprint = GetBlueprint();
	return (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
}

#undef LOCTEXT_NAMESPACE