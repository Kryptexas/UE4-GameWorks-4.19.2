// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetBlueprintCompiler.h"
#include "Kismet2NameValidators.h"
#include "KismetReinstanceUtilities.h"

template<>
FString FNetNameMapping::MakeBaseName<UWidgetGraphNode_Base>(const UWidgetGraphNode_Base* Net)
{
	return FString::Printf(TEXT("%s_%s"), *Net->GetDescriptiveCompiledName(), *Net->NodeGuid.ToString());
}

///-------------------------------------------------------------

FWidgetBlueprintCompiler::FEvaluationHandlerRecord::FEvaluationHandlerRecord()
	: NodeVariableProperty(NULL)
	, EvaluationHandlerProperty(NULL)
	, HandlerFunctionName(NAME_None)
{
}

bool FWidgetBlueprintCompiler::FEvaluationHandlerRecord::IsValid() const
{
	return ( NodeVariableProperty != NULL ) && ( EvaluationHandlerProperty != NULL );
}

void FWidgetBlueprintCompiler::FEvaluationHandlerRecord::PatchFunctionNameInto(UObject* TargetObject) const
{
	EvaluationHandlerProperty->ContainerPtrToValuePtr<FExposedValueHandler>(
		NodeVariableProperty->ContainerPtrToValuePtr<void>(TargetObject)
		)->BoundFunction = HandlerFunctionName;
}

void FWidgetBlueprintCompiler::FEvaluationHandlerRecord::RegisterPin(UEdGraphPin* SourcePin, UProperty* AssociatedProperty, int32 AssociatedPropertyArrayIndex)
{
	FAnimNodeSinglePropertyHandler& Handler = ServicedProperties.FindOrAdd(AssociatedProperty->GetFName());

	if ( AssociatedPropertyArrayIndex != INDEX_NONE )
	{
		Handler.ArrayPins.Add(AssociatedPropertyArrayIndex, SourcePin);
	}
	else
	{
		check(Handler.SinglePin == NULL);
		Handler.SinglePin = SourcePin;
	}
}

///-------------------------------------------------------------

FWidgetBlueprintCompiler::FWidgetBlueprintCompiler(UWidgetBlueprint* SourceSketch, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions, TArray<UObject*>* InObjLoaded)
	: Super(SourceSketch, InMessageLog, InCompilerOptions, InObjLoaded)
{
}

FWidgetBlueprintCompiler::~FWidgetBlueprintCompiler()
{
}

void FWidgetBlueprintCompiler::MergeUbergraphPagesIn(UEdGraph* Ubergraph)
{
	Super::MergeUbergraphPagesIn(Ubergraph);

	// Build the raw node list
	TArray<UWidgetGraphNode_Base*> WidgetNodeList;
	ConsolidatedEventGraph->GetNodesOfClass<UWidgetGraphNode_Base>(/*out*/ WidgetNodeList);
	
	AllocateNodeIndexCounter = 0;

	// Process the remaining nodes
	for ( UWidgetGraphNode_Base* VisualWidgetNode : WidgetNodeList )
	{
		ProcessWidgetNode(VisualWidgetNode);
	}
}

void FWidgetBlueprintCompiler::ProcessWidgetNode(UWidgetGraphNode_Base* VisualWidgetNode)
{
	const UWidgetGraphSchema* Schema = GetDefault<UWidgetGraphSchema>();

	// Early out if this node has already been processed
	//if ( AllocatedAnimNodes.Contains(VisualWidgetNode) )
	//{
	//	return;
	//}

	// Make sure the visual node has a runtime node template
	UScriptStruct* NodeType = VisualWidgetNode->GetFNodeType();
	if ( NodeType == NULL )
	{
		MessageLog.Error(TEXT("@@ has no widget value node member"), VisualWidgetNode);
		return;
	}

	// Give the visual node a chance to do validation
	{
		const int32 PreValidationErrorCount = MessageLog.NumErrors;
		//VisualWidgetNode->ValidateAnimNodeDuringCompilation(VisualWidgetNode->TargetSkeleton, MessageLog);
		//VisualWidgetNode->BakeDataDuringCompilation(MessageLog);
		if ( MessageLog.NumErrors != PreValidationErrorCount )
		{
			return;
		}
	}

	// Create a property for the node
	const FString NodeVariableName = ClassScopeNetNameMap.MakeValidName(VisualWidgetNode);

	FEdGraphPinType NodeVariableType;
	NodeVariableType.PinCategory = Schema->PC_Struct;
	NodeVariableType.PinSubCategoryObject = NodeType;

	UStructProperty* NewProperty = Cast<UStructProperty>(CreateVariable(FName(*NodeVariableName), NodeVariableType));

	if ( NewProperty == NULL )
	{
		MessageLog.Error(TEXT("Failed to create node property for @@"), VisualWidgetNode);
	}

	// Register this node with the compile-time data structures
	const int32 AllocatedIndex = AllocateNodeIndexCounter++;
	AllocatedAnimNodes.Add(VisualWidgetNode, NewProperty);
	AllocatedNodePropertiesToNodes.Add(NewProperty, VisualWidgetNode);
	AllocatedAnimNodeIndices.Add(VisualWidgetNode, AllocatedIndex);
	AllocatedPropertiesByIndex.Add(AllocatedIndex, NewProperty);

	UWidgetGraphNode_Base* TrueSourceObject = MessageLog.FindSourceObjectTypeChecked<UWidgetGraphNode_Base>(VisualWidgetNode);
	SourceNodeToProcessedNodeMap.Add(TrueSourceObject, VisualWidgetNode);


	// TODO UMG add debug data.
	// Register the slightly more permanent debug information
//	NewAnimBlueprintClass->GetAnimBlueprintDebugData().NodePropertyToIndexMap.Add(TrueSourceObject, AllocatedIndex);
//	NewAnimBlueprintClass->GetDebugData().RegisterClassPropertyAssociation(TrueSourceObject, NewProperty);

	// Record pose pins for later patchup and gather pins that have an associated evaluation handler
	TMap<FName, FEvaluationHandlerRecord> EvaluationHandlers;

	for ( auto SourcePinIt = VisualWidgetNode->Pins.CreateIterator(); SourcePinIt; ++SourcePinIt )
	{
		UEdGraphPin* SourcePin = *SourcePinIt;
		bool bConsumed = false;

		// Does this pin have an associated evaluation handler?
		UProperty* SourcePinProperty;
		int32 SourceArrayIndex;
		VisualWidgetNode->GetPinAssociatedProperty(NodeType, SourcePin, /*out*/ SourcePinProperty, /*out*/ SourceArrayIndex);

		if ( SourcePinProperty != NULL )
		{
			if ( SourcePin->LinkedTo.Num() == 0 )
			{
				// Literal that can be pushed into the CDO instead of re-evaluated every frame
				new (ValidAnimNodePinConstants)FEffectiveConstantRecord(NewProperty, SourcePin, SourcePinProperty, SourceArrayIndex);
				bConsumed = true;
			}
			else
			{
				// Dynamic value that needs to be wired up and evaluated each frame
				FString EvaluationHandlerStr = SourcePinProperty->GetMetaData(Schema->NAME_OnEvaluate);
				FName EvaluationHandlerName(*EvaluationHandlerStr);
				if ( EvaluationHandlerName == NAME_None )
				{
					EvaluationHandlerName = Schema->DefaultEvaluationHandlerName;
				}

				EvaluationHandlers.FindOrAdd(EvaluationHandlerName).RegisterPin(SourcePin, SourcePinProperty, SourceArrayIndex);
				bConsumed = true;
			}
		}

		if ( !bConsumed && ( SourcePin->Direction == EGPD_Input ) )
		{
			//@TODO: ANIMREFACTOR: It's probably OK to have certain pins ignored eventually, but this is very helpful during development
			MessageLog.Note(TEXT("@@ was visible but ignored"), SourcePin);
		}
	}

	// Match the associated property to each evaluation handler
	for ( TFieldIterator<UProperty> NodePropIt(NodeType); NodePropIt; ++NodePropIt )
	{
		if ( UStructProperty* StructProp = Cast<UStructProperty>(*NodePropIt) )
		{
			if ( StructProp->Struct == FExposedWidgetValueHandler::StaticStruct() )
			{
				// Register this property to the list of pins that need to be updated
				// (it's OK if there isn't an entry for this handler, it means that the values are static and don't need to be calculated every frame)
				FName EvaluationHandlerName(StructProp->GetFName());
				if ( FEvaluationHandlerRecord* pRecord = EvaluationHandlers.Find(EvaluationHandlerName) )
				{
					pRecord->NodeVariableProperty = NewProperty;
					pRecord->EvaluationHandlerProperty = StructProp;
				}
			}
		}
	}

	// Generate a new event to update the value of these properties
	for ( auto HandlerIt = EvaluationHandlers.CreateIterator(); HandlerIt; ++HandlerIt )
	{
		FName EvaluationHandlerName = HandlerIt.Key();
		FEvaluationHandlerRecord& Record = HandlerIt.Value();

		if ( Record.IsValid() )
		{
			CreateEvaluationHandler(VisualWidgetNode, Record);
			ValidEvaluationHandlerList.Add(Record);
		}
		else
		{
			MessageLog.Error(*FString::Printf(TEXT("A property on @@ references a non-existent %s property named %s"), *( Schema->NAME_OnEvaluate.ToString() ), *( EvaluationHandlerName.ToString() )), VisualWidgetNode );
		}
	}
}

//void FWidgetBlueprintCompiler::CreateEvaluationHandler(UWidgetGraphNode_Base* VisualAnimNode, FEvaluationHandlerRecord& Record)
//{
//	// Shouldn't create a handler if there is nothing to work with
//	check(Record.ServicedProperties.Num() > 0);
//	check(Record.NodeVariableProperty != NULL);
//
//
//	//@TODO: Want to name these better
//	const FString FunctionName = FString::Printf(TEXT("%s_%s_%s"), *Record.EvaluationHandlerProperty->GetName(), *VisualAnimNode->GetOuter()->GetName(), *VisualAnimNode->GetName());
//	Record.HandlerFunctionName = FName(*FunctionName);
//	
//
//	//UWidgetBlueprint* Blueprint = WidgetBlueprint();
//
//
//	// Create the stub graph and add it to the list of functions to compile
//
//	UEdGraph* ChildStubGraph = NewNamedObject<UEdGraph>(Blueprint, Record.HandlerFunctionName);
//	Blueprint->EventGraphs.Add(ChildStubGraph);
//	ChildStubGraph->Schema = UEdGraphSchema_K2::StaticClass();
//	ChildStubGraph->SetFlags(RF_Transient);
//	MessageLog.NotifyIntermediateObjectCreation(ChildStubGraph, VisualAnimNode);
//
//	FKismetFunctionContext* StubContext = CreateFunctionContext();
//	StubContext->SourceGraph = ChildStubGraph;
//
//	// A stub graph has no visual representation and is thus not suited to be debugged via the debugger
//	StubContext->bCreateDebugData = false;
//	StubContext->MarkAsInternalOrCppUseOnly();
//
//
//
//
//
//
//
//
//
//
//
//	
//
//	// Add a custom event in the graph
//	//UK2Node_CustomEvent* EntryNode = SpawnIntermediateNode<UK2Node_CustomEvent>(VisualAnimNode, ConsolidatedEventGraph);
//	//EntryNode->bInternalEvent = true;
//	//EntryNode->CustomFunctionName = Record.HandlerFunctionName;
//	UK2Node_FunctionEntry* EntryNode = SpawnIntermediateNode<UK2Node_FunctionEntry>(VisualAnimNode, ChildStubGraph);
//	//EntryNode->SignatureClass = UObject::StaticClass();
//	//EntryNode->SignatureName = Schema->FN_ExecuteUbergraphBase;
//	EntryNode->CustomGeneratedFunctionName = Record.HandlerFunctionName;
//	EntryNode->AllocateDefaultPins();
//
//	// The ExecChain is the current exec output pin in the linear chain
//	UEdGraphPin* ExecChain = Schema->FindExecutionPin(*EntryNode, EGPD_Output);
//
//	// Create a struct member write node to store the parameters into the animation node
//	UK2Node_StructMemberSet* AssignmentNode = SpawnIntermediateNode<UK2Node_StructMemberSet>(VisualAnimNode, ChildStubGraph);
//	AssignmentNode->VariableReference.SetSelfMember(Record.NodeVariableProperty->GetFName());
//	AssignmentNode->StructType = Record.NodeVariableProperty->Struct;
//	AssignmentNode->AllocateDefaultPins();
//
//	// Wire up the variable node execution wires
//	//UEdGraphPin* ExecVariablesIn = Schema->FindExecutionPin(*AssignmentNode, EGPD_Input);
//	//ExecChain->MakeLinkTo(ExecVariablesIn);
//	//ExecChain = Schema->FindExecutionPin(*AssignmentNode, EGPD_Output);
//
//	UK2Node_FunctionResult* ReturnNode = SpawnIntermediateNode<UK2Node_FunctionResult>(VisualAnimNode, ChildStubGraph);
//	ReturnNode->AllocateDefaultPins();
//
//	// Wire up the variable node execution wires
//	UEdGraphPin* ExecReturnIn = Schema->FindExecutionPin(*ReturnNode, EGPD_Input);
//	ExecChain->MakeLinkTo(ExecReturnIn);
//	ExecChain = Schema->FindExecutionPin(*ReturnNode, EGPD_Output);
//
//	for ( auto TargetPinIt = AssignmentNode->Pins.CreateIterator(); TargetPinIt; ++TargetPinIt )
//	{
//		UEdGraphPin* TargetPin = *TargetPinIt;
//		FString PropertyNameStr = TargetPin->PinName;
//		FName PropertyName(*PropertyNameStr);
//
//		// Does it get serviced by this handler?
//		if ( FAnimNodeSinglePropertyHandler* SourceInfo = Record.ServicedProperties.Find(PropertyName) )
//		{
//			if ( TargetPin->PinType.bIsArray )
//			{
//				// Grab the array that we need to set members for
//				UK2Node_StructMemberGet* FetchArrayNode = SpawnIntermediateNode<UK2Node_StructMemberGet>(VisualAnimNode, ChildStubGraph);
//				FetchArrayNode->VariableReference.SetSelfMember(Record.NodeVariableProperty->GetFName());
//				FetchArrayNode->StructType = Record.NodeVariableProperty->Struct;
//				FetchArrayNode->AllocatePinsForSingleMemberGet(PropertyName);
//
//				UEdGraphPin* ArrayVariableNode = FetchArrayNode->FindPin(PropertyNameStr);
//
//				if ( SourceInfo->ArrayPins.Num() > 0 )
//				{
//					// Set each element in the array
//					for ( auto SourcePinIt = SourceInfo->ArrayPins.CreateIterator(); SourcePinIt; ++SourcePinIt )
//					{
//						int32 ArrayIndex = SourcePinIt.Key();
//						UEdGraphPin* SourcePin = SourcePinIt.Value();
//
//						// Create an array element set node
//						UK2Node_CallArrayFunction* ArrayNode = SpawnIntermediateNode<UK2Node_CallArrayFunction>(VisualAnimNode, ChildStubGraph);
//						ArrayNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Set), UKismetArrayLibrary::StaticClass());
//						ArrayNode->AllocateDefaultPins();
//
//						// Connect the execution chain
//						ExecChain->MakeLinkTo(ArrayNode->GetExecPin());
//						ExecChain = ArrayNode->GetThenPin();
//
//						// Connect the input array
//						UEdGraphPin* TargetArrayPin = ArrayNode->FindPinChecked(TEXT("TargetArray"));
//						TargetArrayPin->MakeLinkTo(ArrayVariableNode);
//						ArrayNode->PinConnectionListChanged(TargetArrayPin);
//
//						// Set the array index
//						UEdGraphPin* TargetIndexPin = ArrayNode->FindPinChecked(TEXT("Index"));
//						TargetIndexPin->DefaultValue = FString::FromInt(ArrayIndex);
//
//						// Wire up the data input
//						UEdGraphPin* TargetItemPin = ArrayNode->FindPinChecked(TEXT("Item"));
//						TargetItemPin->CopyPersistentDataFromOldPin(*SourcePin);
//						MessageLog.NotifyIntermediateObjectCreation(TargetItemPin, SourcePin);
//						SourcePin->BreakAllPinLinks();
//					}
//				}
//			}
//			else
//			{
//				// Single property
//				if ( SourceInfo->SinglePin != NULL )
//				{
//					UEdGraphPin* SourcePin = SourceInfo->SinglePin;
//
//					// First, add this pin to the user-defined pins
//					TSharedPtr<FUserPinInfo> NewPinInfo = MakeShareable(new FUserPinInfo());
//					NewPinInfo->PinName = TEXT("return");
//					NewPinInfo->PinType = SourcePin->PinType;
//					//UserDefinedPins.Add(NewPinInfo);
//					TargetPin = ReturnNode->CreatePinFromUserDefinition(NewPinInfo);
//
//					//PropertiesBeingSet.Add(FName(*SourcePin->PinName));
//					//TargetPin->CopyPersistentDataFromOldPin(*SourcePin);
//					MessageLog.NotifyIntermediateObjectCreation(TargetPin, SourcePin);
//					SourcePin->BreakAllPinLinks();
//				}
//			}
//		}
//	}
//
//	// Remove any unused pins from the assignment node to avoid smashing constant values
//	//for ( int32 PinIndex = 0; PinIndex < AssignmentNode->ShowPinForProperties.Num(); ++PinIndex )
//	//{
//	//	FOptionalPinFromProperty& TestProperty = AssignmentNode->ShowPinForProperties[PinIndex];
//	//	TestProperty.bShowPin = PropertiesBeingSet.Contains(TestProperty.PropertyName);
//	//}
//
//	AssignmentNode->ReconstructNode();
//	ReturnNode->ReconstructNode();
//
//	// TODO
//}

void FWidgetBlueprintCompiler::CreateEvaluationHandler(UWidgetGraphNode_Base* VisualAnimNode, FEvaluationHandlerRecord& Record)
{
	// Shouldn't create a handler if there is nothing to work with
	check(Record.ServicedProperties.Num() > 0);
	check(Record.NodeVariableProperty != NULL);

	//@TODO: Want to name these better
	const FString FunctionName = FString::Printf(TEXT("%s_%s_%s"), *Record.EvaluationHandlerProperty->GetName(), *VisualAnimNode->GetOuter()->GetName(), *VisualAnimNode->GetName());
	Record.HandlerFunctionName = FName(*FunctionName);

	// Add a custom event in the graph
	UK2Node_CustomEvent* EntryNode = SpawnIntermediateNode<UK2Node_CustomEvent>(VisualAnimNode, ConsolidatedEventGraph);
	EntryNode->bInternalEvent = true;
	EntryNode->CustomFunctionName = Record.HandlerFunctionName;
	EntryNode->AllocateDefaultPins();

	// The ExecChain is the current exec output pin in the linear chain
	UEdGraphPin* ExecChain = Schema->FindExecutionPin(*EntryNode, EGPD_Output);

	// Create a struct member write node to store the parameters into the animation node
	UK2Node_StructMemberSet* AssignmentNode = SpawnIntermediateNode<UK2Node_StructMemberSet>(VisualAnimNode, ConsolidatedEventGraph);
	AssignmentNode->VariableReference.SetSelfMember(Record.NodeVariableProperty->GetFName());
	AssignmentNode->StructType = Record.NodeVariableProperty->Struct;
	AssignmentNode->AllocateDefaultPins();

	// Wire up the variable node execution wires
	UEdGraphPin* ExecVariablesIn = Schema->FindExecutionPin(*AssignmentNode, EGPD_Input);
	ExecChain->MakeLinkTo(ExecVariablesIn);
	ExecChain = Schema->FindExecutionPin(*AssignmentNode, EGPD_Output);

	// Run thru each property
	TSet<FName> PropertiesBeingSet;

	//UK2Node_FunctionResult* ReturnNode = SpawnIntermediateNode<UK2Node_FunctionResult>(VisualAnimNode, ConsolidatedEventGraph);
	//ReturnNode->AllocateDefaultPins();

	//// Wire up the variable node execution wires
	//UEdGraphPin* ExecReturnIn = Schema->FindExecutionPin(*ReturnNode, EGPD_Input);
	//ExecChain->MakeLinkTo(ExecReturnIn);
	//ExecChain = Schema->FindExecutionPin(*ReturnNode, EGPD_Output);

	for ( auto TargetPinIt = AssignmentNode->Pins.CreateIterator(); TargetPinIt; ++TargetPinIt )
	{
		UEdGraphPin* TargetPin = *TargetPinIt;
		FString PropertyNameStr = TargetPin->PinName;
		FName PropertyName(*PropertyNameStr);

		// Does it get serviced by this handler?
		if ( FAnimNodeSinglePropertyHandler* SourceInfo = Record.ServicedProperties.Find(PropertyName) )
		{
			if ( TargetPin->PinType.bIsArray )
			{
				// Grab the array that we need to set members for
				UK2Node_StructMemberGet* FetchArrayNode = SpawnIntermediateNode<UK2Node_StructMemberGet>(VisualAnimNode, ConsolidatedEventGraph);
				FetchArrayNode->VariableReference.SetSelfMember(Record.NodeVariableProperty->GetFName());
				FetchArrayNode->StructType = Record.NodeVariableProperty->Struct;
				FetchArrayNode->AllocatePinsForSingleMemberGet(PropertyName);

				UEdGraphPin* ArrayVariableNode = FetchArrayNode->FindPin(PropertyNameStr);

				if ( SourceInfo->ArrayPins.Num() > 0 )
				{
					// Set each element in the array
					for ( auto SourcePinIt = SourceInfo->ArrayPins.CreateIterator(); SourcePinIt; ++SourcePinIt )
					{
						int32 ArrayIndex = SourcePinIt.Key();
						UEdGraphPin* SourcePin = SourcePinIt.Value();

						// Create an array element set node
						UK2Node_CallArrayFunction* ArrayNode = SpawnIntermediateNode<UK2Node_CallArrayFunction>(VisualAnimNode, ConsolidatedEventGraph);
						ArrayNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Set), UKismetArrayLibrary::StaticClass());
						ArrayNode->AllocateDefaultPins();

						// Connect the execution chain
						ExecChain->MakeLinkTo(ArrayNode->GetExecPin());
						ExecChain = ArrayNode->GetThenPin();

						// Connect the input array
						UEdGraphPin* TargetArrayPin = ArrayNode->FindPinChecked(TEXT("TargetArray"));
						TargetArrayPin->MakeLinkTo(ArrayVariableNode);
						ArrayNode->PinConnectionListChanged(TargetArrayPin);

						// Set the array index
						UEdGraphPin* TargetIndexPin = ArrayNode->FindPinChecked(TEXT("Index"));
						TargetIndexPin->DefaultValue = FString::FromInt(ArrayIndex);

						// Wire up the data input
						UEdGraphPin* TargetItemPin = ArrayNode->FindPinChecked(TEXT("Item"));
						TargetItemPin->CopyPersistentDataFromOldPin(*SourcePin);
						MessageLog.NotifyIntermediateObjectCreation(TargetItemPin, SourcePin);
						SourcePin->BreakAllPinLinks();
					}
				}
			}
			else
			{
				// Single property
				if ( SourceInfo->SinglePin != NULL )
				{
					UEdGraphPin* SourcePin = SourceInfo->SinglePin;

					PropertiesBeingSet.Add(FName(*SourcePin->PinName));
					TargetPin->CopyPersistentDataFromOldPin(*SourcePin);
					MessageLog.NotifyIntermediateObjectCreation(TargetPin, SourcePin);
					SourcePin->BreakAllPinLinks();


					//// First, add this pin to the user-defined pins
					//TSharedPtr<FUserPinInfo> NewPinInfo = MakeShareable(new FUserPinInfo());
					//NewPinInfo->PinName = TEXT("return");
					//NewPinInfo->PinType = SourcePin->PinType;

					//TargetPin = ReturnNode->CreatePinFromUserDefinition(NewPinInfo);
					//TargetPin->CopyPersistentDataFromOldPin(*SourcePin);
					//MessageLog.NotifyIntermediateObjectCreation(TargetPin, SourcePin);

					//SourcePin->BreakAllPinLinks();
				}
			}
		}
	}

	// Remove any unused pins from the assignment node to avoid smashing constant values
	for ( int32 PinIndex = 0; PinIndex < AssignmentNode->ShowPinForProperties.Num(); ++PinIndex )
	{
		FOptionalPinFromProperty& TestProperty = AssignmentNode->ShowPinForProperties[PinIndex];
		TestProperty.bShowPin = PropertiesBeingSet.Contains(TestProperty.PropertyName);
	}
	AssignmentNode->ReconstructNode();

}

void FWidgetBlueprintCompiler::CopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	Super::CopyTermDefaultsToDefaultObject(DefaultObject);

	// And patch evaluation function entry names
	for ( auto EvalLinkIt = ValidEvaluationHandlerList.CreateIterator(); EvalLinkIt; ++EvalLinkIt )
	{
		FEvaluationHandlerRecord& Record = *EvalLinkIt;
		Record.PatchFunctionNameInto(DefaultObject);
	}
}

void FWidgetBlueprintCompiler::ValidateWidgetNames()
{
	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	TSharedPtr<FKismetNameValidator> ParentBPNameValidator;
	if ( Blueprint->ParentClass != NULL )
	{
		UBlueprint* ParentBP = Cast<UBlueprint>(Blueprint->ParentClass->ClassGeneratedBy);
		if ( ParentBP != NULL )
		{
			ParentBPNameValidator = MakeShareable(new FKismetNameValidator(ParentBP));
		}
	}

	for ( USlateWrapperComponent* Widget : Blueprint->WidgetTree->WidgetTemplates )
	{
		if ( ParentBPNameValidator.IsValid() && ParentBPNameValidator->IsValid(Widget->GetName()) != EValidatorResult::Ok )
		{
			// TODO Support renaming items, similar to timelines.

			//// Use the viewer displayed Timeline name (without the _Template suffix) because it will be added later for appropriate checks.
			//FString TimelineName = UTimelineTemplate::TimelineTemplateNameToVariableName(TimelineTemplate->GetFName());

			//FName NewName = FBlueprintEditorUtils::FindUniqueKismetName(Blueprint, TimelineName);
			//MessageLog.Warning(*FString::Printf(*LOCTEXT("TimelineConflictWarning", "Found a timeline with a conflicting name (%s) - changed to %s.").ToString(), *TimelineTemplate->GetName(), *NewName.ToString()));
			//FBlueprintEditorUtils::RenameTimeline(Blueprint, FName(*TimelineName), NewName);
		}
	}
}

template<typename TOBJ>
struct FCullTemplateObjectsHelper
{
	const TArray<TOBJ*>& Templates;

	FCullTemplateObjectsHelper(const TArray<TOBJ*>& InComponentTemplates)
		: Templates(InComponentTemplates)
	{}

	bool operator()(const UObject* const RemovalCandidate) const
	{
		return ( NULL != Templates.FindByKey(RemovalCandidate) );
	}
};

void FWidgetBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO)
{
	Super::CleanAndSanitizeClass(ClassToClean, OldCDO);

	// Purge all subobjects (properties, functions, params) of the class, as they will be regenerated
	TArray<UObject*> ClassSubObjects;
	GetObjectsWithOuter(ClassToClean, ClassSubObjects, true);

	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	if ( 0 != Blueprint->WidgetTree->WidgetTemplates.Num() )
	{
		ClassSubObjects.RemoveAllSwap(FCullTemplateObjectsHelper<USlateWrapperComponent>(Blueprint->WidgetTree->WidgetTemplates));
	}
}

void FWidgetBlueprintCompiler::CreateClassVariablesFromBlueprint()
{
	Super::CreateClassVariablesFromBlueprint();

	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	ValidateWidgetNames();

	for ( USlateWrapperComponent* Widget : Blueprint->WidgetTree->WidgetTemplates )
	{
		// Skip non-variable widgets
		if ( !Widget->bIsVariable )
		{
			continue;
		}

		FString VariableName = Widget->GetName();

		FEdGraphPinType WidgetPinType(Schema->PC_Object, TEXT(""), Widget->GetClass(), false, false);
		UProperty* WidgetProperty = CreateVariable(FName(*VariableName), WidgetPinType);
		if ( WidgetProperty != NULL )
		{
			WidgetProperty->SetMetaData(TEXT("Category"), *Blueprint->GetName());
			WidgetProperty->SetPropertyFlags(CPF_BlueprintVisible);

			WidgetToMemberVariableMap.Add(Widget, WidgetProperty);
		}
	}
}

void FWidgetBlueprintCompiler::FinishCompilingClass(UClass* Class)
{
	UWidgetBlueprint* Blueprint = WidgetBlueprint();

	UWidgetBlueprintGeneratedClass* BPGClass = CastChecked<UWidgetBlueprintGeneratedClass>(Class);
	BPGClass->WidgetTree = NULL;
	BPGClass->WidgetTree = Blueprint->WidgetTree;
	BPGClass->Bindings.Reset();

	for ( const FDelegateEditorBinding& EditorBinding : Blueprint->Bindings )
	{
		BPGClass->Bindings.Add(EditorBinding.ToRuntimeBinding(Blueprint));
	}

	Super::FinishCompilingClass(Class);
}

void FWidgetBlueprintCompiler::Compile()
{
	Super::Compile();

	WidgetToMemberVariableMap.Empty();
}

void FWidgetBlueprintCompiler::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if ( TargetUClass && !( (UObject*)TargetUClass )->IsA(UWidgetBlueprintGeneratedClass::StaticClass()) )
	{
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = NULL;
	}
}

void FWidgetBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
{
	NewWidgetBlueprintClass = FindObject<UWidgetBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if ( NewWidgetBlueprintClass == NULL )
	{
		NewWidgetBlueprintClass = ConstructObject<UWidgetBlueprintGeneratedClass>(UWidgetBlueprintGeneratedClass::StaticClass(), Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		// Already existed, but wasn't linked in the Blueprint yet due to load ordering issues
		FBlueprintCompileReinstancer GeneratedClassReinstancer(NewWidgetBlueprintClass);
	}
	NewClass = NewWidgetBlueprintClass;
}

bool FWidgetBlueprintCompiler::ValidateGeneratedClass(UBlueprintGeneratedClass* Class)
{
	bool SuperResult = Super::ValidateGeneratedClass(Class);
	bool Result = UWidgetBlueprint::ValidateGeneratedClass(Class);

	return SuperResult && Result;
}
