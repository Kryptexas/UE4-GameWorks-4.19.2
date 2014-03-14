// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetDebugUtilities.h" // for HasDebuggingData(), GetWatchText()

#define LOCTEXT_NAMESPACE "K2Node"

// File-Scoped Globals
static const uint32 MaxArrayPinTooltipLineCount = 10;

/////////////////////////////////////////////////////
// UK2Node

UK2Node::UK2Node(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RemovedPinArrayIndex = INDEX_NONE;
}

FText UK2Node::GetToolTipHeading() const
{
	FText Heading = FText::GetEmpty();
	if (UBreakpoint* ExistingBreakpoint = FKismetDebugUtilities::FindBreakpointForNode(GetBlueprint(), this))
	{
		if (ExistingBreakpoint->IsEnabled())
		{
			Heading = LOCTEXT("EnabledBreakpoint", "Active Breakpoint");
			FText ActiveBreakpointToolTipText = GetActiveBreakpointToolTipText();
			if (!ActiveBreakpointToolTipText.IsEmpty())
			{
				Heading = FText::Format(FText::FromString("{0} - {1}"), Heading, ActiveBreakpointToolTipText);
			}
		}
		else 
		{
			Heading = LOCTEXT("DisabledBreakpoint", "Disabled Breakpoint");
		}
	}

	return Heading;
}

FText UK2Node::GetActiveBreakpointToolTipText() const
{
	return LOCTEXT("ActiveBreakpointToolTip", "Execution will break at this location.");
}

bool UK2Node::CreatePinsForFunctionEntryExit(const UFunction* Function, bool bForFunctionEntry)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create the inputs and outputs
	bool bAllPinsGood = true;
	for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
	{
		UProperty* Param = *PropIt;

		const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);

		if (bIsFunctionInput == bForFunctionEntry)
		{
			const EEdGraphPinDirection Direction = bForFunctionEntry ? EGPD_Output : EGPD_Input;

			UEdGraphPin* Pin = CreatePin(Direction, TEXT(""), TEXT(""), NULL, false, false, Param->GetName());
			const bool bPinGood = K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);

			bAllPinsGood = bAllPinsGood && bPinGood;
		}
	}

	return bAllPinsGood;
}

void UK2Node::AutowireNewNode(UEdGraphPin* FromPin)
{
	const UEdGraphSchema_K2* K2Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());
	
	// Do some auto-connection
	if (FromPin != NULL)
	{
		TSet<UEdGraphNode*> NodeList;

		// If not dragging an exec pin, auto-connect from dragged pin to first compatible pin on the new node
		for (int32 i=0; i<Pins.Num(); i++)
		{
			UEdGraphPin* Pin = Pins[i];
			check(Pin);
			if (ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE == K2Schema->CanCreateConnection(FromPin, Pin).Response)
			{
				if (K2Schema->TryCreateConnection(FromPin, Pin))
				{
					NodeList.Add(FromPin->GetOwningNode());
					NodeList.Add(this);
				}
				break;
			}
			else if(FromPin->PinType.PinCategory == K2Schema->PC_Exec && ECanCreateConnectionResponse::CONNECT_RESPONSE_BREAK_OTHERS_A == K2Schema->CanCreateConnection(FromPin, Pin).Response)
			{
				InsertNewNode(FromPin, Pin, NodeList);				
				break;
			}
		}

		// If we were not dragging an exec pin, but it was an output pin, try and connect the Then and Execute pins
		if ((FromPin->PinType.PinCategory != K2Schema->PC_Exec  && FromPin->Direction == EGPD_Output))
		{
			UEdGraphNode* FromPinNode = FromPin->GetOwningNode();
			UEdGraphPin* FromThenPin = FromPinNode->FindPin(K2Schema->PN_Then);

			UEdGraphPin* ToExecutePin = FindPin(K2Schema->PN_Execute);

			if ((FromThenPin != NULL) && (FromThenPin->LinkedTo.Num() == 0) && (ToExecutePin != NULL) && K2Schema->ArePinsCompatible(FromThenPin, ToExecutePin, NULL))
			{
				if (K2Schema->TryCreateConnection(FromThenPin, ToExecutePin))
				{
					NodeList.Add(FromPinNode);
					NodeList.Add(this);
				}
			}
		}

		// Send all nodes that received a new pin connection a notification
		for (auto It = NodeList.CreateConstIterator(); It; ++It)
		{
			UEdGraphNode* Node = (*It);
			Node->NodeConnectionListChanged();
		}
	}
}

void UK2Node::InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList)
{
	const UEdGraphSchema_K2* K2Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());

	// The pin we are creating from already has a connection that needs to be broken. Being an exec pin, we want to "insert" the new node in between, so that the output of the new node is hooked up to 
	UEdGraphPin* OldLinkedPin = FromPin->LinkedTo[0];
	check(OldLinkedPin);

	FromPin->BreakAllPinLinks();

	// Hook up the old linked pin to the first valid output pin on the new node
	for (int32 OutpinPinIdx=0; OutpinPinIdx<Pins.Num(); OutpinPinIdx++)
	{
		UEdGraphPin* OutputExecPin = Pins[OutpinPinIdx];
		check(OutputExecPin);
		if (ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE == K2Schema->CanCreateConnection(OldLinkedPin, OutputExecPin).Response)
		{
			if (K2Schema->TryCreateConnection(OldLinkedPin, OutputExecPin))
			{
				OutNodeList.Add(OldLinkedPin->GetOwningNode());
				OutNodeList.Add(this);
			}
			break;
		}
	}

	if (K2Schema->TryCreateConnection(FromPin, NewLinkPin))
	{
		OutNodeList.Add(FromPin->GetOwningNode());
		OutNodeList.Add(this);
	}
}

void UK2Node::PinConnectionListChanged(UEdGraphPin* Pin) 
{
	// If the pin has been connected, clear the default values so that we don't hold on to references
	if(Pin->LinkedTo.Num() > 0) 
	{
		// Verify that we have a proper outer at this point, such that the schema will be valid
		UEdGraph* OuterGraph = GetGraph();
		if( OuterGraph && OuterGraph->Schema )
		{
			const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(GetSchema());

			Pin->DefaultObject = NULL;
			Schema->SetPinDefaultValueBasedOnType(Pin);
		}
	}

	NotifyPinConnectionListChanged(Pin);
}

void UK2Node::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& /*OldPins*/)
{
	AllocateDefaultPins();
}

void UK2Node::PostReconstructNode()
{
	if (!IsTemplate())
	{
		// Make sure we're not dealing with a menu node
		UEdGraph* OuterGraph = GetGraph();
		if( OuterGraph && OuterGraph->Schema )
		{
			const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
			// fix up any pin data if it needs to 
			for( auto PinIt = Pins.CreateIterator(); PinIt; ++PinIt )
			{
				UEdGraphPin* CurrentPin = *PinIt;
				const FString& PinCategory = CurrentPin->PinType.PinCategory;

				// fix up enum names if it exists in enum redirects
				if (PinCategory == Schema->PC_Byte)
				{
					UEnum* EnumPtr = Cast<UEnum>(CurrentPin->PinType.PinSubCategoryObject.Get());
					if (EnumPtr)
					{
						const FString & PinValue = CurrentPin->DefaultValue;
						// see if this enum is in EnumRedirects
						int32 EnumIndex = UEnum::FindEnumRedirects(EnumPtr, *PinValue);
						if (EnumIndex != INDEX_NONE)
						{
							FString EnumName = EnumPtr->GetEnumName(EnumIndex);

							// if the name does not match with pin value, override pin value
							if (EnumName != PinValue)
							{
								// I'm not marking package as dirty 
								// as I know that's not going to work during serialize or post load
								CurrentPin->DefaultValue = EnumName;
								continue;
							}
						}
					}
				}
			}
		}
	}
}

void UK2Node::RemovePinsFromOldPins(TArray<UEdGraphPin*>& OldPins, int32 RemovedArrayIndex)
{
	TArray<FString> RemovedPropertyNames;
	TArray<FString> NewPinNames;

	// Store new pin names to compare with old pin names
	for(int32 NewPinIndx=0; NewPinIndx < Pins.Num(); NewPinIndx++)
	{
		NewPinNames.Add(Pins[NewPinIndx]->PinName);
	}

	// don't know which pins are removed yet so find removed pins comparing NewPins and OldPins
	for(int32 OldPinIdx=0; OldPinIdx < OldPins.Num(); OldPinIdx++)
	{
		FString& OldPinName = OldPins[OldPinIdx]->PinName;
		if(!NewPinNames.Contains(OldPinName))
		{
			int32 UnderscoreIndex = OldPinName.Find(TEXT("_"));
			if (UnderscoreIndex != INDEX_NONE)
			{
				FString PropertyName = OldPinName.Left(UnderscoreIndex);
				RemovedPropertyNames.Add(PropertyName);
			}
		}
	}

	for(int32 PinIdx=0; PinIdx < OldPins.Num(); PinIdx++)
	{
		// Separate the pin name into property name and index
		FString PropertyName;
		int32 ArrayIndex = -1;
		FString& OldPinName = OldPins[PinIdx]->PinName;

		int32 UnderscoreIndex = OldPinName.Find(TEXT("_"));
		if (UnderscoreIndex != INDEX_NONE)
		{
			PropertyName = OldPinName.Left(UnderscoreIndex);
			ArrayIndex = FCString::Atoi(*(OldPinName.Mid(UnderscoreIndex + 1)));

			if(RemovedPropertyNames.Contains(PropertyName))
			{
				// if array index is matched, removes pins 
				// and if array index is greater than removed index, decrease index
				if(ArrayIndex == RemovedArrayIndex)
				{
					OldPins.RemoveAt(PinIdx);
					--PinIdx;
				}
				else
				if(ArrayIndex > RemovedArrayIndex)
				{
					OldPinName = FString::Printf(TEXT("%s_%d"), *PropertyName, ArrayIndex-1);
				}
			}
		}
	}
}

void UK2Node::ReconstructNode()
{							  
	Modify();

	UBlueprint* Blueprint = GetBlueprint();

	// Break any links to 'orphan' pins
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		TArray<class UEdGraphPin*> LinkedToCopy = Pin->LinkedTo;
		for (int32 LinkIdx = 0; LinkIdx < LinkedToCopy.Num(); LinkIdx++)
		{
			UEdGraphPin* OtherPin = LinkedToCopy[LinkIdx];
			// If we are linked to a pin that its owner doesn't know about, break that link
			if ((OtherPin == NULL) || !OtherPin->GetOwningNode()->Pins.Contains(OtherPin))
			{
				Pin->LinkedTo.Remove(OtherPin);
			}
		}
	}

	// Move the existing pins to a saved array
	TArray<UEdGraphPin*> OldPins(Pins);
	Pins.Empty();

	// Recreate the new pins
	ReallocatePinsDuringReconstruction(OldPins);

	// Delete Pins by removed pin info 
	if(RemovedPinArrayIndex != INDEX_NONE)
	{
		RemovePinsFromOldPins(OldPins, RemovedPinArrayIndex);
		// Clears removed pin info to avoid to remove multiple times
		// @TODO : Considering receiving RemovedPinArrayIndex as an argument of ReconstructNode()
		RemovedPinArrayIndex = INDEX_NONE;
	}

	bool bDestroyOldPins = true;

	if (Pins.Num() == 0)
	{
		//keep old pins on callfunction so that graph doesn't get broken up just because function is missing
		if (IsA(UK2Node_CallFunction::StaticClass()) || IsA(UK2Node_MacroInstance::StaticClass()))
		{
			Pins = OldPins;
			bDestroyOldPins = false;
		}
	}
	else
	{
		// Rewire any connection to pins that are matched by name (O(N^2) right now)
		//@TODO: Can do moderately smart things here if only one pin changes name by looking at it's relative position, etc...,
		// rather than just failing to map it and breaking the links
		for (int32 OldPinIndex = 0; OldPinIndex < OldPins.Num(); ++OldPinIndex)
		{
			UEdGraphPin* OldPin = OldPins[OldPinIndex];

			for (int32 NewPinIndex = 0; NewPinIndex < Pins.Num(); ++NewPinIndex)
			{
				UEdGraphPin* NewPin = Pins[NewPinIndex];

				const ERedirectType RedirectType = DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
				if (RedirectType != ERedirectType_None)
				{
					ReconstructSinglePin(NewPin, OldPin, RedirectType);
					break;
				}
			}
		}
	}


	if (bDestroyOldPins)
	{
		// Throw away the original pins
		for (int32 OldPinIndex = 0; OldPinIndex < OldPins.Num(); ++OldPinIndex)
		{
			UEdGraphPin* OldPin = OldPins[OldPinIndex];
			OldPin->Modify();
			OldPin->BreakAllPinLinks();
			
			// just in case this pin was set to watch (don't want to save PinWatches with dead pins)
			Blueprint->PinWatches.Remove(OldPin);
#if 0
			UEdGraphNode::ReturnPinToPool(OldPin);
#else
			OldPin->Rename(NULL, GetTransientPackage(), (Blueprint->bIsRegeneratingOnLoad ? REN_ForceNoResetLoaders : REN_None));
			OldPin->RemoveFromRoot();
			OldPin->MarkPendingKill();
#endif
		}
	}

	// Let subclasses do any additional work
	PostReconstructNode();

	GetGraph()->NotifyGraphChanged();
}

void UK2Node::GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const
{
	RedirectPinNames.Add(Pin.PinName);
}

UK2Node::ERedirectType UK2Node::ShouldRedirectParam(const TArray<FString>& OldPinNames, FName & NewPinName, const UK2Node * NewPinNode) const 
{
	if ( ensure(NewPinNode) )
	{
		InitFieldRedirectMap();

		if ( ParamRedirectMap.Num() > 0 )
		{
			// convert TArray<FString> to TArray<FName>, faster to search
			TArray<FName> OldPinFNames;
			for (auto NameIter=OldPinNames.CreateConstIterator(); NameIter; ++NameIter)
			{
				const FString & Name = *NameIter;
				OldPinFNames.AddUnique(*Name);
			}

			// go through for the NewPinNode
			for(TMultiMap<UClass*, FParamRemapInfo>::TConstKeyIterator ParamIter(ParamRedirectMap, NewPinNode->GetClass()); ParamIter; ++ParamIter)
			{
				const FParamRemapInfo & ParamRemap = ParamIter.Value();

				// if it has it, return true
				if (OldPinFNames.Contains(ParamRemap.OldParam)
					&& (ParamRemap.NodeTitle == NAME_None || ParamRemap.NodeTitle.ToString() == NewPinNode->GetNodeTitle(ENodeTitleType::FullTitle)))
				{
					NewPinName = ParamRemap.NewParam;
					return (ParamRemap.bCustomValueMapping ? ERedirectType_Custom : (ParamRemap.ParamValueMap.Num() ? ERedirectType_Value : ERedirectType_Name));
				}
			}
		}
	}

	return ERedirectType_None;
}

UK2Node::ERedirectType UK2Node::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	ERedirectType RedirectType = ERedirectType_None;

	// if the pin names do match
	if (FCString::Stricmp(*(NewPin->PinName), *(OldPin->PinName)) == 0)
	{
		// Make sure we're not dealing with a menu node
		UEdGraph* OuterGraph = GetGraph();
		if( OuterGraph && OuterGraph->Schema )
		{
			const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
			if( !K2Schema || K2Schema->IsSelfPin(*NewPin) || K2Schema->ArePinTypesCompatible(OldPin->PinType, NewPin->PinType) )
			{
				RedirectType = ERedirectType_Name;
			}
			else
			{
				RedirectType = ERedirectType_None;
			}
		}
	}
	else
	{
		// try looking for a redirect if it's a K2 node
		if (UK2Node* Node = Cast<UK2Node>(NewPin->GetOwningNode()))
		{	
			// if you don't have matching pin, now check if there is any redirect param set
			TArray<FString> OldPinNames;
			GetRedirectPinNames(*OldPin, OldPinNames);

			FName NewPinName;
			RedirectType = ShouldRedirectParam(OldPinNames, /*out*/ NewPinName, Node);

			// make sure they match
			if ((RedirectType != ERedirectType_None) && FCString::Stricmp(*(NewPin->PinName), *(NewPinName.ToString())) != 0)
			{
				RedirectType = ERedirectType_None;
			}
		}
	}

	return RedirectType;
}

void UK2Node::CustomMapParamValue(UEdGraphPin& Pin)
{
}

void UK2Node::ReconstructSinglePin(UEdGraphPin* NewPin, UEdGraphPin* OldPin, ERedirectType RedirectType)
{
	UBlueprint* Blueprint = GetBlueprint();

	// Copy over modified persistent data
	NewPin->CopyPersistentDataFromOldPin(*OldPin);

	if (NewPin->DefaultValue != NewPin->AutogeneratedDefaultValue)
	{
		if (RedirectType == ERedirectType_Value)
		{
			TArray<FString> OldPinNames;
			GetRedirectPinNames(*OldPin, OldPinNames);

			// convert TArray<FString> to TArray<FName>, faster to search
			TArray<FName> OldPinFNames;
			for (auto NameIter=OldPinNames.CreateConstIterator(); NameIter; ++NameIter)
			{
				const FString & Name = *NameIter;
				OldPinFNames.AddUnique(*Name);
			}

			// go through for the NewPinNode
			for(TMultiMap<UClass*, FParamRemapInfo>::TConstKeyIterator ParamIter(ParamRedirectMap, Cast<UK2Node>(NewPin->GetOwningNode())->GetClass()); ParamIter; ++ParamIter)
			{
				const FParamRemapInfo & ParamRemap = ParamIter.Value();

				// once we find it, see about remapping the value
				if (OldPinFNames.Contains(ParamRemap.OldParam))
				{
					const FString* NewValue = ParamRemap.ParamValueMap.Find(NewPin->DefaultValue);
					if (NewValue)
					{
						NewPin->DefaultValue = *NewValue;
					}
					break;
				}
			}
		}
		else if (RedirectType == ERedirectType_Custom)
		{
			CustomMapParamValue(*NewPin);
		}
	}

	// Update the blueprints watched pins as the old pin will be going the way of the dodo
	for (int32 WatchIndex = 0; WatchIndex < Blueprint->PinWatches.Num(); ++WatchIndex)
	{
		UEdGraphPin*& WatchedPin = Blueprint->PinWatches[WatchIndex];
		if( WatchedPin == OldPin )
		{
			WatchedPin = NewPin;
			break;
		}
	}

	// Try to preserve the object name of the old pin to reduce clutter in diffs
	FString OldPinName = OldPin->GetName();
	OldPin->Rename(NULL, GetTransientPackage(), (REN_DontCreateRedirectors|(Blueprint->bIsRegeneratingOnLoad ? REN_ForceNoResetLoaders : REN_None)));
	NewPin->Rename(*OldPinName, NewPin->GetOuter(), (REN_DontCreateRedirectors|(Blueprint->bIsRegeneratingOnLoad ? REN_ForceNoResetLoaders : REN_None)));
}

bool UK2Node::HasValidBlueprint() const
{
	// Perform an unchecked search here, so we don't crash if this is a transient node from a list refresh without a valid outer blueprint
	return (FBlueprintEditorUtils::FindBlueprintForNode(this) != NULL);
}

UBlueprint* UK2Node::GetBlueprint() const
{
	return FBlueprintEditorUtils::FindBlueprintForNodeChecked(this);
}

FLinearColor UK2Node::GetNodeTitleColor() const
{
	// Different color for pure operations
	if (IsNodePure())
	{
		return GEditor->AccessEditorUserSettings().PureFunctionCallNodeTitleColor;
	}
	else
	{
		return GEditor->AccessEditorUserSettings().FunctionCallNodeTitleColor;
	}
}


TMap<FName, FFieldRemapInfo> UK2Node::FieldRedirectMap;
TMultiMap<UClass*, FParamRemapInfo> UK2Node::ParamRedirectMap;

bool UK2Node::bFieldRedirectMapInitialized = false;

bool UK2Node::FindReplacementFieldName(UClass* Class, FName FieldName, FFieldRemapInfo& RemapInfo)
{
	check(Class != NULL);
	InitFieldRedirectMap();

	// Reset the property remap info
	RemapInfo = FFieldRemapInfo();

	FString FullOldField = FString::Printf(TEXT("%s.%s"), *Class->GetName(), *FieldName.ToString());
	FFieldRemapInfo* NewFieldInfoPtr = FieldRedirectMap.Find(FName(*FullOldField));
	if (NewFieldInfoPtr != NULL)
	{
		RemapInfo = *NewFieldInfoPtr;
		return true;
	}
	else
	{
		return false;
	}
}

void UK2Node::InitFieldRedirectMap()
{
	if (!bFieldRedirectMapInitialized)
	{
		if (GConfig)
		{
			FConfigSection* PackageRedirects = GConfig->GetSectionPrivate( TEXT("/Script/Engine.Engine"), false, true, GEngineIni );
			for (FConfigSection::TIterator It(*PackageRedirects); It; ++It)
			{
				if (It.Key() == TEXT("K2FieldRedirects"))
				{
					FName OldFieldName = NAME_None;
					FString NewFieldPathString;

					FParse::Value( *It.Value(), TEXT("OldFieldName="), OldFieldName );
					FParse::Value( *It.Value(), TEXT("NewFieldName="), NewFieldPathString );

					// Handle both cases of just a field being renamed (just one FName), as well as a class and field name (ClassName.FieldName)
					TArray<FString> NewFieldPath;
					NewFieldPathString.ParseIntoArray(&NewFieldPath, TEXT("."), true);

					FFieldRemapInfo FieldRemap;
					if( NewFieldPath.Num() == 1 )
					{
						// Only the new property name is specified
						FieldRemap.FieldName = FName(*NewFieldPath[0]);
					}
					else if( NewFieldPath.Num() == 2 )
					{
						// Property name and new class are specified
						FieldRemap.FieldClass = FName(*NewFieldPath[0]);
						FieldRemap.FieldName = FName(*NewFieldPath[1]);
					}

					FieldRedirectMap.Add(OldFieldName, FieldRemap);
				}			
				if (It.Key() == TEXT("K2ParamRedirects"))
				{
					FName NodeName = NAME_None;
					FName OldParam = NAME_None;
					FName NewParam = NAME_None;
					FName NodeTitle = NAME_None;

					FString OldParamValues;
					FString NewParamValues;
					FString CustomValueMapping;

					FParse::Value( *It.Value(), TEXT("NodeName="), NodeName );
					FParse::Value( *It.Value(), TEXT("NodeTitle="), NodeTitle );
					FParse::Value( *It.Value(), TEXT("OldParamName="), OldParam );
					FParse::Value( *It.Value(), TEXT("NewParamName="), NewParam );
					FParse::Value( *It.Value(), TEXT("OldParamValues="), OldParamValues );
					FParse::Value( *It.Value(), TEXT("NewParamValues="), NewParamValues );
					FParse::Value( *It.Value(), TEXT("CustomValueMapping="), CustomValueMapping );

					FParamRemapInfo ParamRemap;
					ParamRemap.NewParam = NewParam;
					ParamRemap.OldParam = OldParam;
					ParamRemap.NodeTitle = NodeTitle;
					ParamRemap.bCustomValueMapping = (FCString::Stricmp(*CustomValueMapping,TEXT("true")) == 0);

					if (CustomValueMapping.Len() > 0 && !ParamRemap.bCustomValueMapping)
					{
						UE_LOG(LogBlueprint, Warning, TEXT("Value other than true specified for CustomValueMapping for '%s' node param redirect '%s' to '%s'."), *(NodeName.ToString()), *(OldParam.ToString()), *(NewParam.ToString()));
					}

					TArray<FString> OldParamValuesList;
					TArray<FString> NewParamValuesList;
					OldParamValues.ParseIntoArray(&OldParamValuesList, TEXT(";"), false);
					NewParamValues.ParseIntoArray(&NewParamValuesList, TEXT(";"), false);

					if (OldParamValuesList.Num() != NewParamValuesList.Num())
					{
						UE_LOG(LogBlueprint, Warning, TEXT("Unequal lengths for old and new param values for '%s' node param redirect '%s' to '%s'."), *(NodeName.ToString()), *(OldParam.ToString()), *(NewParam.ToString()));
					}

					if (CustomValueMapping.Len() > 0 && (OldParamValuesList.Num() > 0 || NewParamValuesList.Num() > 0))
					{
						UE_LOG(LogBlueprint, Warning, TEXT("Both Custom and Automatic param value remapping specified for '%s' node param redirect '%s' to '%s'.  Only Custom will be applied."), *(NodeName.ToString()), *(OldParam.ToString()), *(NewParam.ToString()));
					}

					for (int32 i = FMath::Min(OldParamValuesList.Num(), NewParamValuesList.Num()) - 1; i >= 0; --i)
					{
						int32 CurSize = ParamRemap.ParamValueMap.Num();
						ParamRemap.ParamValueMap.Add(OldParamValuesList[i], NewParamValuesList[i]);
						if (CurSize == ParamRemap.ParamValueMap.Num())
						{
							UE_LOG(LogBlueprint, Warning, TEXT("Duplicate old param value '%s' for '%s' node param redirect '%s' to '%s'."), *(OldParamValuesList[i]), *(NodeName.ToString()), *(OldParam.ToString()), *(NewParam.ToString()));
						}
					}

					// load class
					UClass* NodeClass = LoadClass<UK2Node>(NULL, *NodeName.ToString(), NULL, LOAD_None, NULL);
					if (NodeClass )
					{
						ParamRedirectMap.Add(NodeClass, ParamRemap);
					}
				}			
			}

			bFieldRedirectMapInitialized = true;
		}
	}
}

UField* UK2Node::FindRemappedField(UClass* InitialScope, FName InitialName)
{
	FFieldRemapInfo NewFieldInfo;

	bool bFoundReplacement = false;
	
	// Step up the class chain to check if us or any of our parents specify a redirect
	UClass* TestRemapClass = InitialScope;
	while( TestRemapClass != NULL )
	{
		if( FindReplacementFieldName(TestRemapClass, InitialName, NewFieldInfo) )
		{
			// Found it, stop our search
			bFoundReplacement = true;
			break;
		}

		TestRemapClass = TestRemapClass->GetSuperClass();
	}

	if( bFoundReplacement )
	{
		const FName NewFieldName = NewFieldInfo.FieldName;
		UClass* SearchClass = (NewFieldInfo.FieldClass != NAME_None) ? (UClass*)StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *NewFieldInfo.FieldClass.ToString()) : (UClass*)TestRemapClass;

		// Find the actual field specified by the redirector, so we can return it and update the node that uses it
		UField* NewField = FindField<UField>(SearchClass, NewFieldInfo.FieldName);
		if( NewField != NULL )
		{
			UE_LOG(LogBlueprint, Log, TEXT("UK2Node:  Fixed up old field '%s' to new name '%s' on class '%s'."), *InitialName.ToString(), *NewFieldInfo.FieldName.ToString(), *SearchClass->GetName());
			return NewField;
		}
		else if (SearchClass != NULL)
		{
			UE_LOG(LogBlueprint, Log, TEXT("UK2Node:  Unable to find updated field name for '%s' on class '%s'."), *InitialName.ToString(), *SearchClass->GetName());
		}
		else
		{
			UE_LOG(LogBlueprint, Log, TEXT("UK2Node:  Unable to find updated field name for '%s' on unknown class '%s'."), *InitialName.ToString(), *NewFieldInfo.FieldClass.ToString());
		}
	}

	return NULL;
}

ERenamePinResult UK2Node::RenameUserDefinedPin(const FString& OldName, const FString& NewName, bool bTest)
{
	UEdGraphPin* Pin = NULL;
	for (int32 PinIdx=0; PinIdx<Pins.Num(); PinIdx++)
	{
		if (OldName == Pins[PinIdx]->PinName)
		{
			Pin = Pins[PinIdx];
		}
		else if(NewName == Pins[PinIdx]->PinName)
		{
			return ERenamePinResult::ERenamePinResult_NameCollision;
		}
	}

	if(!Pin)
	{
		return ERenamePinResult::ERenamePinResult_NoSuchPin;
	}

	if(!bTest)
	{
		Pin->PinName = NewName;
		if(!Pin->DefaultTextValue.IsEmpty())
		{
			Pin->GetSchema()->TrySetDefaultText(*Pin, Pin->DefaultTextValue);
		}
	}

	return ERenamePinResult::ERenamePinResult_Success;
}

/////////////////////////////////////////////////////
// FOptionalPinManager

void FOptionalPinManager::GetRecordDefaults(UProperty* TestProperty, FOptionalPinFromProperty& Record) const
{
	Record.bShowPin = true;
	Record.bCanToggleVisibility = true;
}

bool FOptionalPinManager::CanTreatPropertyAsOptional(UProperty* TestProperty) const
{
	return TestProperty->HasAnyPropertyFlags(CPF_Edit|CPF_BlueprintVisible); // TODO: ANIMREFACTOR: Maybe only CPF_Edit?
}

void FOptionalPinManager::RebuildPropertyList(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct)
{
	// Save the old visibility
	TMap<FName, bool> OldVisibility;
	for (auto ExtraPropertyIt = Properties.CreateIterator(); ExtraPropertyIt; ++ExtraPropertyIt)
	{
		FOptionalPinFromProperty& PropertyEntry = *ExtraPropertyIt;

		OldVisibility.Add(PropertyEntry.PropertyName, PropertyEntry.bShowPin);
	}

	// Rebuild the property list
	Properties.Empty();

	for (TFieldIterator<UProperty> It(SourceStruct, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		UProperty* TestProperty = *It;

		if (CanTreatPropertyAsOptional(TestProperty))
		{
			FOptionalPinFromProperty* Record = new (Properties) FOptionalPinFromProperty;
			Record->PropertyName = TestProperty->GetFName();
			const FString FriendlyName = TestProperty->GetMetaData(TEXT("FriendlyName"));
			Record->PropertyFriendlyName = FriendlyName.IsEmpty() ? Record->PropertyName.ToString() : FriendlyName;

			// Get the defaults
			GetRecordDefaults(TestProperty, *Record);

			// If this is a refresh, propagate the old visibility
			if (Record->bCanToggleVisibility)
			{
				if (bool* pShowHide = OldVisibility.Find(Record->PropertyName))
				{
					Record->bShowPin = *pShowHide;
				}
			}
		}
	}
}

void FOptionalPinManager::CreateVisiblePins(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct, EEdGraphPinDirection Direction, UK2Node* TargetNode, uint8* StructBasePtr)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	for (auto ExtraPropertyIt = Properties.CreateIterator(); ExtraPropertyIt; ++ExtraPropertyIt)
	{
		FOptionalPinFromProperty& PropertyEntry = *ExtraPropertyIt;

		if (UProperty* OuterProperty = FindFieldChecked<UProperty>(SourceStruct, PropertyEntry.PropertyName))
		{
			// Do we treat an array property as one pin, or a pin per entry in the array?
			// Depends on if we have an instance of the struct to work with.
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(OuterProperty);
			if ((ArrayProperty != NULL) && (StructBasePtr != NULL))
			{
				UProperty* InnerProperty = ArrayProperty->Inner;

				FEdGraphPinType PinType;
				if (Schema->ConvertPropertyToPinType(InnerProperty, /*out*/ PinType))
				{
					FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, StructBasePtr);

					for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
					{
						// Create the pin
						UEdGraphPin* NewPin = NULL;
						if (PropertyEntry.bShowPin)
						{
							const FString PinName = FString::Printf(TEXT("%s_%d"), *(PropertyEntry.PropertyName.ToString()), Index);
							const FString PinFriendlyName = PropertyEntry.PropertyFriendlyName.IsEmpty() ? PinName : 
								FString::Printf(TEXT("%s_%d"), *PropertyEntry.PropertyFriendlyName, Index);
							NewPin = TargetNode->CreatePin(Direction, PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(), PinType.bIsArray, PinType.bIsReference, PinName);
							NewPin->PinFriendlyName = PinFriendlyName;

							// Allow the derived class to customize the created pin
							CustomizePinData(NewPin, PropertyEntry.PropertyName, Index, InnerProperty);
						}

						// Let derived classes take a crack at transferring default values
						uint8* ValuePtr = ArrayHelper.GetRawPtr(Index);
						if (NewPin != NULL)
						{
							PostInitNewPin(NewPin, PropertyEntry, Index, ArrayProperty->Inner, ValuePtr);
						}
						else
						{
							PostRemovedOldPin(PropertyEntry, Index, ArrayProperty->Inner, ValuePtr);
						}
					}
				}
			}
			else
			{
				// Not an array property

				FEdGraphPinType PinType;
				if (Schema->ConvertPropertyToPinType(OuterProperty, /*out*/ PinType))
				{
					// Create the pin
					UEdGraphPin* NewPin = NULL;
					if (PropertyEntry.bShowPin)
					{
						const FString PinName = PropertyEntry.PropertyName.ToString();
						NewPin = TargetNode->CreatePin(Direction, PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(), PinType.bIsArray, PinType.bIsReference, PinName);
						NewPin->PinFriendlyName = PropertyEntry.PropertyFriendlyName.IsEmpty() ? PinName : PropertyEntry.PropertyFriendlyName;

						// Allow the derived class to customize the created pin
						CustomizePinData(NewPin, PropertyEntry.PropertyName, INDEX_NONE, OuterProperty);
					}

					// Let derived classes take a crack at transferring default values
					if (StructBasePtr != NULL)
					{
						uint8* ValuePtr = OuterProperty->ContainerPtrToValuePtr<uint8>(StructBasePtr);
						if (NewPin != NULL)
						{
							PostInitNewPin(NewPin, PropertyEntry, INDEX_NONE, OuterProperty, ValuePtr);
						}
						else
						{
							PostRemovedOldPin(PropertyEntry, INDEX_NONE, OuterProperty, ValuePtr);
						}
					}
				}
			}
		}
	}
}

UEdGraphPin* UK2Node::GetExecPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_Execute);
	check(Pin == NULL || Pin->Direction == EGPD_Input); // If pin exists, it must be input
	return Pin;
}

bool UK2Node::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	return DesiredSchema->GetClass()->IsChildOf(UEdGraphSchema_K2::StaticClass());
}

void UK2Node::Message_Note(const FString& Message)
{
	UBlueprint* OwningBP = GetBlueprint();
	if( OwningBP )
	{
		OwningBP->Message_Note(Message);
	}
	else
	{
		UE_LOG(LogBlueprint, Log, TEXT("%s"), *Message);
	}
}

void UK2Node::Message_Warn(const FString& Message)
{
	UBlueprint* OwningBP = GetBlueprint();
	if( OwningBP )
	{
		OwningBP->Message_Warn(Message);
	}
	else
	{
		UE_LOG(LogBlueprint, Warning, TEXT("%s"), *Message);
	}
}

void UK2Node::Message_Error(const FString& Message)
{
	UBlueprint* OwningBP = GetBlueprint();
	if( OwningBP )
	{
		OwningBP->Message_Error(Message);
	}
	else
	{
		UE_LOG(LogBlueprint, Error, TEXT("%s"), *Message);
	}
}

FString UK2Node::GetDocumentationLink() const
{
	return TEXT("Shared/GraphNodes/Blueprint");
}

void UK2Node::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	// start with the default hover text (from the pin's tool-tip)
	Super::GetPinHoverText(Pin, HoverTextOut);

	// if the Pin wasn't initialized with a tool-tip of its own
	if (HoverTextOut.IsEmpty())
	{
		UEdGraphSchema const* Schema = GetSchema();
		check(Schema != NULL);
		Schema->ConstructBasicPinTooltip(Pin, TEXT(""), HoverTextOut);
	}	

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNodeChecked(this);
	check(Blueprint != NULL);

	// If this resides in an intermediate graph, show the UObject name for debug purposes
	if(Blueprint->IntermediateGeneratedGraphs.Contains(GetGraph()))
	{
		HoverTextOut = FString::Printf(TEXT("%s\n\n%s"), *Pin.GetName(), *HoverTextOut);
	}

	UObject* ActiveObject = Blueprint->GetObjectBeingDebugged();
	// if there is no object being debugged, then we don't need to tack on any of the following data
	if (ActiveObject == NULL)
	{
		return;
	}

	// Switch the blueprint to the one that generated the object being debugged (e.g. in case we're inside a Macro BP while debugging)
	Blueprint = Cast<UBlueprint>(ActiveObject->GetClass()->ClassGeneratedBy);
	if (Blueprint == NULL)
	{
		return;
	}

	// if the blueprint doesn't have debug data, notify the user
	if (!FKismetDebugUtilities::HasDebuggingData(Blueprint))
	{
		HoverTextOut += TEXT("\n(NO DEBUGGING INFORMATION GENERATED, NEED TO RECOMPILE THE BLUEPRINT)");
	}

	//@TODO: For exec pins, show when they were last executed


	// grab the debug value of the pin
	FString WatchText;
	const FKismetDebugUtilities::EWatchTextResult WatchStatus = FKismetDebugUtilities::GetWatchText(/*inout*/ WatchText, Blueprint, ActiveObject, &Pin);

	// if this is an array pin, then we possibly have too many lines (too many entries)
	if (Pin.PinType.bIsArray)
	{
		int32 LineCounter = 0;
		int32 OriginalWatchTextLen = WatchText.Len();

		// walk the string, finding line breaks (counting lines)
		for (int32 NewWatchTextLen = 0; NewWatchTextLen < OriginalWatchTextLen; )
		{
			++LineCounter;

			int32 NewLineIndex = WatchText.Find("\n", ESearchCase::IgnoreCase,  ESearchDir::FromStart, NewWatchTextLen);
			// if we've reached the end of the string (it's not to long)
			if (NewLineIndex == INDEX_NONE)
			{
				break;
			}

			NewWatchTextLen = NewLineIndex + 1;
			// if we're at the end of the string (but it ends with a newline)
			if (NewWatchTextLen >= OriginalWatchTextLen)
			{
				break;
			}

			// if we've hit the max number of lines allowed in a tooltip
			if (LineCounter >= MaxArrayPinTooltipLineCount)
			{
				// truncate WatchText so it contains a finite number of lines
				WatchText  = WatchText.Left(NewWatchTextLen);
				WatchText += "..."; // WatchText should already have a trailing newline (no need to prepend this with one)
				break;
			}
		}
	} // if Pin.PinType.bIsArray...


	switch (WatchStatus)
	{
	case FKismetDebugUtilities::EWTR_Valid:
		HoverTextOut += FString::Printf(TEXT("\nCurrent value = %s"), *WatchText); //@TODO: Print out object being debugged name?
		break;
	case FKismetDebugUtilities::EWTR_NotInScope:
		HoverTextOut += TEXT("\n(Variable is not in scope)");
		break;

	default:
	case FKismetDebugUtilities::EWTR_NoDebugObject:
	case FKismetDebugUtilities::EWTR_NoProperty:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
// FMemberReference

void FMemberReference::SetExternalMember(FName InMemberName, TSubclassOf<class UObject> InMemberParentClass)
{
	MemberName = InMemberName;
	MemberParentClass = InMemberParentClass;
	bSelfContext = false;
}

void FMemberReference::SetSelfMember(FName InMemberName)
{
	MemberName = InMemberName;
	MemberParentClass = NULL;
	bSelfContext = true;
}

void FMemberReference::SetDirect(const FName InMemberName, const FGuid InMemberGuid, TSubclassOf<class UObject> InMemberParentClass, bool bIsConsideredSelfContext)
{
	MemberName = InMemberName;
	MemberGuid = InMemberGuid;
	bSelfContext = bIsConsideredSelfContext;
	MemberParentClass = InMemberParentClass;
}

void FMemberReference::SetGivenSelfScope(const FName InMemberName, const FGuid InMemberGuid, TSubclassOf<class UObject> InMemberParentClass, TSubclassOf<class UObject> SelfScope) const
{
	MemberName = InMemberName;
	MemberGuid = InMemberGuid;
	MemberParentClass = InMemberParentClass;
	bSelfContext = (SelfScope->IsChildOf(InMemberParentClass)) || (SelfScope->ClassGeneratedBy == InMemberParentClass->ClassGeneratedBy);

	if (bSelfContext)
	{
		MemberParentClass = NULL;
	}
}

void FMemberReference::InvalidateSelfScope()
{
	if( IsSelfContext() )
	{
		MemberParentClass = NULL;
	}
}

UClass* FMemberReference::GetBlueprintClassFromNode(const UK2Node* Node)
{
	UClass* BPClass = NULL;
	if(Node != NULL && Node->HasValidBlueprint())
	{
		BPClass =  Node->GetBlueprint()->SkeletonGeneratedClass;
	}
	return BPClass;
}

#undef LOCTEXT_NAMESPACE
