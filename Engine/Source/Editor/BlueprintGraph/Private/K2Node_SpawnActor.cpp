// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

#include "EngineKismetLibraryClasses.h"

static FString WorldContextPinName(TEXT("WorldContextObject"));
static FString BlueprintPinName(TEXT("Blueprint"));
static FString SpawnTransformPinName(TEXT("SpawnTransform"));
static FString NoCollisionFailPinName(TEXT("SpawnEvenIfColliding"));


#define LOCTEXT_NAMESPACE "K2Node_SpawnActor"

UK2Node_SpawnActor::UK2Node_SpawnActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Attempts to spawn a new Actor with the specified transform").ToString();
}

void UK2Node_SpawnActor::AllocateDefaultPins()
{
	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add execution pins
	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	// If required add the world context pin
	if (GetBlueprint()->ParentClass->HasMetaData(FBlueprintMetadata::MD_ShowHiddenSelfPins))
	{
		CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UObject::StaticClass(), false, false, WorldContextPinName);
	}

	// Add blueprint pin
	UEdGraphPin* BlueprintPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UBlueprint::StaticClass(), false, false, BlueprintPinName);
	K2Schema->ConstructBasicPinTooltip(*BlueprintPin, LOCTEXT("BlueprintPinDescription", "The blueprint Actor you want to spawn").ToString(), BlueprintPin->PinToolTip);

	// Transform pin
	UScriptStruct* TransformStruct = FindObjectChecked<UScriptStruct>(UObject::StaticClass(), TEXT("Transform"));
	UEdGraphPin* TransformPin = CreatePin(EGPD_Input, K2Schema->PC_Struct, TEXT(""), TransformStruct, false, false, SpawnTransformPinName);
	K2Schema->ConstructBasicPinTooltip(*TransformPin, LOCTEXT("TransformPinDescription", "The transform to spawn the Actor with").ToString(), TransformPin->PinToolTip);

	// bNoCollisionFail pin
	UEdGraphPin* NoCollisionFailPin = CreatePin(EGPD_Input, K2Schema->PC_Boolean, TEXT(""), NULL, false, false, NoCollisionFailPinName);
	K2Schema->ConstructBasicPinTooltip(*NoCollisionFailPin, LOCTEXT("NoCollisionFailPinDescription", "Determines if the Actor should be spawned when the location is blocked by a collision").ToString(), NoCollisionFailPin->PinToolTip);

	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Object, TEXT(""), AActor::StaticClass(), false, false, K2Schema->PN_ReturnValue);
	K2Schema->ConstructBasicPinTooltip(*ResultPin, LOCTEXT("ResultPinDescription", "The spawned Actor").ToString(), ResultPin->PinToolTip);

	Super::AllocateDefaultPins();
}

void UK2Node_SpawnActor::CreatePinsForClass(UClass* InClass)
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

UClass* UK2Node_SpawnActor::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
{
	UClass* UseSpawnClass = NULL;
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* BlueprintPin = GetBlueprintPin(PinsToSearch);
	if(BlueprintPin && BlueprintPin->DefaultObject != NULL && BlueprintPin->LinkedTo.Num() == 0)
	{
		UBlueprint* BP = CastChecked<UBlueprint>(BlueprintPin->DefaultObject);
		UseSpawnClass = BP->GeneratedClass;
	}

	return UseSpawnClass;
}

void UK2Node_SpawnActor::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) 
{
	AllocateDefaultPins();
	UClass* UseSpawnClass = GetClassToSpawn(&OldPins);
	if( UseSpawnClass != NULL )
	{
		CreatePinsForClass(UseSpawnClass);
	}
}

bool UK2Node_SpawnActor::IsSpawnVarPin(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	return(	Pin->PinName != K2Schema->PN_Execute &&
			Pin->PinName != K2Schema->PN_Then &&
			Pin->PinName != K2Schema->PN_ReturnValue &&
			Pin->PinName != BlueprintPinName &&
			Pin->PinName != WorldContextPinName &&
			Pin->PinName != NoCollisionFailPinName &&
			Pin->PinName != SpawnTransformPinName );
}


void UK2Node_SpawnActor::PinDefaultValueChanged(UEdGraphPin* Pin) 
{
	if(Pin->PinName == BlueprintPinName)
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

FString UK2Node_SpawnActor::GetTooltip() const
{
	return NodeTooltip;
}

UEdGraphPin* UK2Node_SpawnActor::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActor::GetBlueprintPin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
{
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* Pin = NULL;
	for( auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt )
	{
		UEdGraphPin* TestPin = *PinIt;
		if( TestPin && TestPin->PinName == BlueprintPinName )
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActor::GetSpawnTransformPin()const
{
	UEdGraphPin* Pin = FindPinChecked(SpawnTransformPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActor::GetNoCollisionFailPin()const
{
	UEdGraphPin* Pin = FindPinChecked(NoCollisionFailPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActor::GetWorldContextPin() const
{
	UEdGraphPin* Pin = FindPin(WorldContextPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnActor::GetResultPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

FLinearColor UK2Node_SpawnActor::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}


FString UK2Node_SpawnActor::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraphPin* BlueprintPin = GetBlueprintPin();

	FString SpawnString = NSLOCTEXT("K2Node", "None", "NONE").ToString();
	if(BlueprintPin != NULL)
	{
		if(BlueprintPin->LinkedTo.Num() > 0)
		{
			// Blueprint will be determined dynamically, so we don't have the name in this case
			SpawnString = TEXT("");
		}
		else if(BlueprintPin->DefaultObject != NULL)
		{
			SpawnString = BlueprintPin->DefaultObject->GetName();
		}
	}

	return FString::Printf(*NSLOCTEXT("K2Node", "SpawnActor", "SpawnActor %s").ToString(), *SpawnString);
}

bool UK2Node_SpawnActor::CanPasteHere( const UEdGraph* TargetGraph, const UEdGraphSchema* Schema ) const 
{
	UBlueprint* Blueprint = Cast<UBlueprint>(TargetGraph->GetOuter());
	return CanCreateUnderSpecifiedSchema(Schema) && (!Blueprint || FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph);
}

FNodeHandlingFunctor* UK2Node_SpawnActor::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FNodeHandlingFunctor(CompilerContext);
}

void UK2Node_SpawnActor::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		static FName BeginSpawningBlueprintFuncName = GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginSpawningActorFromBlueprint);
		static FString BlueprintParamName = FString(TEXT("Blueprint"));
		static FString WorldContextParamName = FString(TEXT("WorldContextObject"));

		static FName FinishSpawningFuncName = GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor);
		static FString ActorParamName = FString(TEXT("Actor"));
		static FString TransformParamName = FString(TEXT("SpawnTransform"));
		static FString NoCollisionFailParamName = FString(TEXT("bNoCollisionFail"));

		static FString ObjectParamName = FString(TEXT("Object"));
		static FString ValueParamName = FString(TEXT("Value"));
		static FString PropertyNameParamName = FString(TEXT("PropertyName"));

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		UEdGraphPin* SpawnNodeExec = GetExecPin();
		UEdGraphPin* SpawnNodeTransform = GetSpawnTransformPin();
		UEdGraphPin* SpawnNodeNoCollisionFail = GetNoCollisionFailPin();
		UEdGraphPin* SpawnWorldContextPin = GetWorldContextPin();
		UEdGraphPin* SpawnBlueprintPin = GetBlueprintPin();
		UEdGraphPin* SpawnNodeThen = GetThenPin();
		UEdGraphPin* SpawnNodeResult = GetResultPin();

		UBlueprint* SpawnBlueprint = NULL;
		if(SpawnBlueprintPin != NULL)
		{
			SpawnBlueprint = Cast<UBlueprint>(SpawnBlueprintPin->DefaultObject);
		}

		if(0 == SpawnBlueprintPin->LinkedTo.Num())	
		{
			if(NULL == SpawnBlueprint)
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("SpawnActorNodeMissingBlueprint_Error", "Spawn node @@ must have a blueprint specified.").ToString(), this);
				// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
				BreakAllNodeLinks();
				return;
			}

			// check if default blueprint is based on Actor
			const UClass* GeneratedClass = SpawnBlueprint->GeneratedClass;
			bool bInvalidBase = GeneratedClass && !GeneratedClass->IsChildOf(AActor::StaticClass());

			const UClass* SkeletonGeneratedClass = Cast<UClass>(SpawnBlueprint->SkeletonGeneratedClass);
			bInvalidBase |= SkeletonGeneratedClass && !SkeletonGeneratedClass->IsChildOf(AActor::StaticClass());

			if(bInvalidBase)
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("SpawnActorNodeInvalidBlueprint_Error", "Spawn node @@ must have a blueprint based on Actor specified.").ToString(), this);
				// we break exec links so this is the only error we get, don't want the SpawnActor node being considered and giving 'unexpected node' type warnings
				BreakAllNodeLinks();
				return;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// create 'begin spawn' call node
		UK2Node_CallFunction* CallBeginSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallBeginSpawnNode->FunctionReference.SetExternalMember(BeginSpawningBlueprintFuncName, UGameplayStatics::StaticClass());
		CallBeginSpawnNode->AllocateDefaultPins();

		UEdGraphPin* CallBeginExec = CallBeginSpawnNode->GetExecPin();
		UEdGraphPin* CallBeginWorldContextPin = CallBeginSpawnNode->FindPinChecked(WorldContextParamName);
		UEdGraphPin* CallBeginBlueprintPin = CallBeginSpawnNode->FindPinChecked(BlueprintParamName);
		UEdGraphPin* CallBeginTransform = CallBeginSpawnNode->FindPinChecked(TransformParamName);
		UEdGraphPin* CallBeginNoCollisionFail = CallBeginSpawnNode->FindPinChecked(NoCollisionFailParamName);
		UEdGraphPin* CallBeginResult = CallBeginSpawnNode->GetReturnValuePin();

		// Move 'exec' connection from spawn node to 'begin spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeExec, *CallBeginExec), this);

		if(SpawnBlueprintPin->LinkedTo.Num() > 0)
		{
			// Copy the 'blueprint' connection from the spawn node to 'begin spawn'
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnBlueprintPin, *CallBeginBlueprintPin), this);
		}
		else
		{
			// Copy blueprint literal onto begin spawn call 
			CallBeginBlueprintPin->DefaultObject = SpawnBlueprint;
		}

		// Copy the world context connection from the spawn node to 'begin spawn' if necessary
		if (SpawnWorldContextPin)
		{
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnWorldContextPin, *CallBeginWorldContextPin), this);
		}

		// Copy the 'transform' connection from the spawn node to 'begin spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeTransform, *CallBeginTransform), this);
		
		// Copy the 'bNoCollisionFail' connection from the spawn node to 'begin spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeNoCollisionFail, *CallBeginNoCollisionFail), this);

		//////////////////////////////////////////////////////////////////////////
		// create 'finish spawn' call node
		UK2Node_CallFunction* CallFinishSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallFinishSpawnNode->FunctionReference.SetExternalMember(FinishSpawningFuncName, UGameplayStatics::StaticClass());
		CallFinishSpawnNode->AllocateDefaultPins();

		UEdGraphPin* CallFinishExec = CallFinishSpawnNode->GetExecPin();
		UEdGraphPin* CallFinishThen = CallFinishSpawnNode->GetThenPin();
		UEdGraphPin* CallFinishActor = CallFinishSpawnNode->FindPinChecked(ActorParamName);
		UEdGraphPin* CallFinishTransform = CallFinishSpawnNode->FindPinChecked(TransformParamName);
		UEdGraphPin* CallFinishResult = CallFinishSpawnNode->GetReturnValuePin();

		// Move 'then' connection from spawn node to 'finish spawn'
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeThen, *CallFinishThen), this);
			
		// Copy transform connection
		CompilerContext.CheckConnectionResponse(Schema->CopyPinLinks(*CallBeginTransform, *CallFinishTransform), this);
		
		// Connect output actor from 'begin' to 'finish'
		CallBeginResult->MakeLinkTo(CallFinishActor);

		// Move result connection from spawn node to 'finish spawn'
		CallFinishResult->PinType = SpawnNodeResult->PinType; // Copy type so it uses the right actor subclass
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnNodeResult, *CallFinishResult), this);

		//////////////////////////////////////////////////////////////////////////
		// create 'set var' nodes

		// Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
		UK2Node_CallFunction* LastNode = CallBeginSpawnNode;

		// Create 'set var by name' nodes and hook them up
		for(int32 PinIdx=0; PinIdx < Pins.Num(); PinIdx++)
		{
			// Only create 'set param by name' node if this pin is linked to something
			UEdGraphPin* SpawnVarPin = Pins[PinIdx];
			if(SpawnVarPin->LinkedTo.Num() > 0)
			{
				UFunction* SetByNameFunction = Schema->FindSetVariableByNameFunction(SpawnVarPin->PinType);
				if(SetByNameFunction)
				{
					UK2Node_CallFunction* SetVarNode = NULL;
					if(SpawnVarPin->PinType.bIsArray)
					{
						SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
					}
					else
					{
						SetVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
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
					CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*SpawnVarPin, *ValuePin), this);
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
		BreakAllNodeLinks();
	}
}

bool UK2Node_SpawnActor::HasExternalBlueprintDependencies() const
{
	const UClass* SourceClass = GetClassToSpawn();
	const UBlueprint* SourceBlueprint = GetBlueprint();
	return (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
}

bool UK2Node_SpawnActor::IsDeprecated() const
{
	return false;
}

bool UK2Node_SpawnActor::ShouldWarnOnDeprecation() const
{
	return false;
}

FString UK2Node_SpawnActor::GetDeprecationMessage() const
{
	return LOCTEXT("SpawnActorNodeOnlyDefaultBlueprint_Deprecatio", "Spawn Actor @@ is DEPRECATED and should be replaced by SpawnActorFromClass").ToString();
}

#undef LOCTEXT_NAMESPACE
