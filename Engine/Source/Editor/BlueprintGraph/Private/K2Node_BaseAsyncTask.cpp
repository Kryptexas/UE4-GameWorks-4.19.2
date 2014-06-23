// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "KismetCompiler.h"
#include "K2ActionMenuBuilder.h"

#define LOCTEXT_NAMESPACE "UK2Node_BaseAsyncTask"

UK2Node_BaseAsyncTask::UK2Node_BaseAsyncTask(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, ProxyFactoryFunctionName(NAME_None)
	, ProxyFactoryClass(NULL)
	, ProxyClass(NULL)
	, ProxyActivateFunctionName(NAME_None)
{
}

FString UK2Node_BaseAsyncTask::GetTooltip() const
{
	const FString FunctionToolTipText = UK2Node_CallFunction::GetDefaultTooltipForFunction(GetFactoryFunction());
	return FunctionToolTipText;
}

FText UK2Node_BaseAsyncTask::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const FString FunctionToolTipText = UK2Node_CallFunction::GetUserFacingFunctionName(GetFactoryFunction());
	return FText::FromString(FunctionToolTipText);
}

FString UK2Node_BaseAsyncTask::GetCategoryName()
{
	const FString GenericFunctionCategory = LOCTEXT("FunctionCategory", "Call Function").ToString();
	return UK2Node_CallFunction::GetDefaultCategoryForFunction(GetFactoryFunction(), GenericFunctionCategory);
}

void UK2Node_BaseAsyncTask::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	const bool bAllowLatentFuncs = (K2Schema->GetGraphType(ContextMenuBuilder.CurrentGraph) == GT_Ubergraph);

	if (bAllowLatentFuncs)
{
		UK2Node_BaseAsyncTask* NodeTemplate = NewObject<UK2Node_BaseAsyncTask>(ContextMenuBuilder.OwnerOfTemporaries, GetClass());
		CreateDefaultMenuEntry(NodeTemplate, ContextMenuBuilder);
	}
}

TSharedPtr<FEdGraphSchemaAction_K2NewNode> UK2Node_BaseAsyncTask::CreateDefaultMenuEntry(UK2Node_BaseAsyncTask* NodeTemplate, FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, NodeTemplate->GetCategoryName(), NodeTemplate->GetNodeTitle(ENodeTitleType::ListView), NodeTemplate->GetTooltip(), 0, NodeTemplate->GetKeywords());
	
	NodeAction->NodeTemplate = NodeTemplate;

	return NodeAction;
}

void UK2Node_BaseAsyncTask::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	UFunction* DelegateSignatureFunction = NULL;
	for (TFieldIterator<UProperty> PropertyIt(ProxyClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		if (UMulticastDelegateProperty* Property = Cast<UMulticastDelegateProperty>(*PropertyIt))
		{
		CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, *Property->GetName());
		if (!DelegateSignatureFunction)
		{
			DelegateSignatureFunction = Property->SignatureFunction;
		}
	}
	}

	if (DelegateSignatureFunction)
	{
		for (TFieldIterator<UProperty> PropIt(DelegateSignatureFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			UProperty* Param = *PropIt;
			const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
			if (bIsFunctionInput)
			{
				UEdGraphPin* Pin = CreatePin(EGPD_Output, TEXT(""), TEXT(""), NULL, false, false, Param->GetName());
				K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);
			}
		}
	}

	bool bAllPinsGood = true;
	UFunction* Function = ProxyFactoryClass->FindFunctionByName(ProxyFactoryFunctionName);
	if (Function)
	{
		TSet<FString> PinsToHide;
		FBlueprintEditorUtils::GetHiddenPinsForFunction(GetBlueprint(), Function, PinsToHide);
		for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			UProperty* Param = *PropIt;
			const bool bIsFunctionInput = !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm);
			if (!bIsFunctionInput)
			{
				// skip function output, it's internal node data 
				continue;
			}

			const bool bIsRefParam = Param->HasAnyPropertyFlags(CPF_ReferenceParm) && bIsFunctionInput;
			UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, bIsRefParam, Param->GetName());
			const bool bPinGood = (Pin != NULL) && K2Schema->ConvertPropertyToPinType(Param, /*out*/ Pin->PinType);

			if (bPinGood)
			{
				//Flag pin as read only for const reference property
				Pin->bDefaultValueIsIgnored = Param->HasAllPropertyFlags(CPF_ConstParm | CPF_ReferenceParm) && (!Function->HasMetaData(FBlueprintMetadata::MD_AutoCreateRefTerm) || Pin->PinType.bIsArray);

				const bool bAdvancedPin = Param->HasAllPropertyFlags(CPF_AdvancedDisplay);
				Pin->bAdvancedView = bAdvancedPin;
				if(bAdvancedPin && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
				{
					AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
				}

				K2Schema->SetPinDefaultValue(Pin, Function, Param);

				if (PinsToHide.Contains(Pin->PinName))
				{
					Pin->bHidden = true;
					Pin->DefaultValue = TEXT("0");
				}
			}

			bAllPinsGood = bAllPinsGood && bPinGood;
		}
	}


	Super::AllocateDefaultPins();
}

struct FBaseAsyncTaskHelper
{
	static bool ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction, const UEdGraphSchema_K2* Schema)
{
		check(Schema);
		const bool bValidDataPin = Pin
			&& (Pin->PinName != Schema->PN_Execute)
			&& (Pin->PinName != Schema->PN_Then)
			&& (Pin->PinType.PinCategory != Schema->PC_Exec);

		const bool bProperDirection = Pin && (Pin->Direction == Direction);

		return bValidDataPin && bProperDirection;
}

	static bool CreateDelegateForNewFunction(UEdGraphPin* DelegateInputPin, FName FunctionName, UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
	{
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		check(DelegateInputPin && Schema && CurrentNode && SourceGraph && (FunctionName != NAME_None));
		bool bResult = true;

		// WORKAROUND, so we can create delegate from nonexistent function by avoiding check at expanding step
		// instead simply: Schema->TryCreateConnection(AddDelegateNode->GetDelegatePin(), CurrentCENode->FindPinChecked(UK2Node_CustomEvent::DelegateOutputName));
		UK2Node_Self* SelfNode = CompilerContext.SpawnIntermediateNode<UK2Node_Self>(CurrentNode, SourceGraph);
		SelfNode->AllocateDefaultPins();

		UK2Node_CreateDelegate* CreateDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CreateDelegate>(CurrentNode, SourceGraph);
		CreateDelegateNode->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(DelegateInputPin, CreateDelegateNode->GetDelegateOutPin());
		bResult &= Schema->TryCreateConnection(SelfNode->FindPinChecked(Schema->PN_Self), CreateDelegateNode->GetObjectInPin());
		CreateDelegateNode->SetFunction(FunctionName);

		return bResult;
	}

	struct FOutputPinAndLocalVariable
	{
		UEdGraphPin* OutputPin;
		UK2Node_TemporaryVariable* TempVar;

		FOutputPinAndLocalVariable(UEdGraphPin* Pin, UK2Node_TemporaryVariable* Var) : OutputPin(Pin), TempVar(Var) {}
	};

	static bool CopyEventSignature(UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema)
	{
		check(CENode && Function && Schema);

		bool bResult = true;
		for (TFieldIterator<UProperty> PropIt(Function); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
		{
			const UProperty* Param = *PropIt;
			if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
		{
				FEdGraphPinType PinType;
				bResult &= Schema->ConvertPropertyToPinType(Param, /*out*/ PinType);
				bResult &= (NULL != CENode->CreateUserDefinedPin(Param->GetName(), PinType));
		}
	}

		return bResult;
	}

	static bool HandleDelegateImplementation(
		UMulticastDelegateProperty* CurrentProperty, const TArray<FBaseAsyncTaskHelper::FOutputPinAndLocalVariable>& VariableOutputs,
		UEdGraphPin* ProxyObjectPin, UEdGraphPin*& InOutLastThenPin,
		UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext)
	{
		bool bIsErrorFree = true;
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		check(CurrentProperty && ProxyObjectPin && InOutLastThenPin && CurrentNode && SourceGraph && Schema);

		UEdGraphPin* PinForCurrentDelegateProperty = CurrentNode->FindPin(CurrentProperty->GetName());
		if (!PinForCurrentDelegateProperty || (Schema->PC_Exec != PinForCurrentDelegateProperty->PinType.PinCategory))
		{
			FText ErrorMessage = FText::Format(LOCTEXT("WrongDelegateProperty", "BaseAsyncTask: Cannot find execution pin for delegate "), FText::FromString(CurrentProperty->GetName()));
			CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), CurrentNode);
			return false;
		}

		UK2Node_CustomEvent* CurrentCENode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(CurrentNode, SourceGraph);
		{
		UK2Node_AddDelegate* AddDelegateNode = CompilerContext.SpawnIntermediateNode<UK2Node_AddDelegate>(CurrentNode, SourceGraph);
		AddDelegateNode->SetFromProperty(CurrentProperty, false);
		AddDelegateNode->AllocateDefaultPins();
			bIsErrorFree &= Schema->TryCreateConnection(AddDelegateNode->FindPinChecked(Schema->PN_Self), ProxyObjectPin);
			bIsErrorFree &= Schema->TryCreateConnection(InOutLastThenPin, AddDelegateNode->FindPinChecked(Schema->PN_Execute));
			InOutLastThenPin = AddDelegateNode->FindPinChecked(Schema->PN_Then);
			CurrentCENode->CustomFunctionName = *FString::Printf(TEXT("%s_%s"), *CurrentProperty->GetName(), *CurrentCENode->NodeGuid.ToString());
			CurrentCENode->AllocateDefaultPins();

			bIsErrorFree &= FBaseAsyncTaskHelper::CreateDelegateForNewFunction(AddDelegateNode->GetDelegatePin(), CurrentCENode->GetFunctionName(), CurrentNode, SourceGraph, CompilerContext);
			bIsErrorFree &= FBaseAsyncTaskHelper::CopyEventSignature(CurrentCENode, AddDelegateNode->GetDelegateSignature(), Schema);
		}

		UEdGraphPin* LastActivatedNodeThen = CurrentCENode->FindPinChecked(Schema->PN_Then);
		for (auto OutputPair : VariableOutputs) // CREATE CHAIN OF ASSIGMENTS
		{
			UEdGraphPin* PinWithData = CurrentCENode->FindPin(OutputPair.OutputPin->PinName);
			if (PinWithData == NULL)
		{
				FText ErrorMessage = FText::Format(LOCTEXT("MissingDataPin", "ICE: Pin @@ was expecting a data output pin named {0} on @@ (each delegate must have the same signature)"), FText::FromString(OutputPair.OutputPin->PinName));
				CompilerContext.MessageLog.Error(*ErrorMessage.ToString(), OutputPair.OutputPin, CurrentCENode);
				return false;
		}

			UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(CurrentNode, SourceGraph);
			AssignNode->AllocateDefaultPins();
			bIsErrorFree &= Schema->TryCreateConnection(LastActivatedNodeThen, AssignNode->GetExecPin());
			bIsErrorFree &= Schema->TryCreateConnection(OutputPair.TempVar->GetVariablePin(), AssignNode->GetVariablePin());
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetVariablePin());
			bIsErrorFree &= Schema->TryCreateConnection(AssignNode->GetValuePin(), PinWithData);
			AssignNode->NotifyPinConnectionListChanged(AssignNode->GetValuePin());

			LastActivatedNodeThen = AssignNode->GetThenPin();
		}

		bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PinForCurrentDelegateProperty, *LastActivatedNodeThen).CanSafeConnect();
		return bIsErrorFree;
	}
};

void UK2Node_BaseAsyncTask::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
        Super::ExpandNode(CompilerContext, SourceGraph);

	if (!CompilerContext.bIsFullCompile)
	{
		return;
	}
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(SourceGraph && Schema);
	bool bIsErrorFree = true;

	// Create a call to factory the proxy object
	UK2Node_CallFunction* const CallCreateProxyObjectNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateProxyObjectNode->FunctionReference.SetExternalMember(ProxyFactoryFunctionName, ProxyFactoryClass);
	CallCreateProxyObjectNode->AllocateDefaultPins();
	bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Schema->PN_Execute), *CallCreateProxyObjectNode->FindPinChecked(Schema->PN_Execute)).CanSafeConnect();
	for (auto CurrentPin : Pins)
		{
		if (FBaseAsyncTaskHelper::ValidDataPin(CurrentPin, EGPD_Input, Schema))
			{
			UEdGraphPin* DestPin = CallCreateProxyObjectNode->FindPin(CurrentPin->PinName); // match function inputs, to pass data to function from CallFunction node
			bIsErrorFree &= DestPin && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *DestPin).CanSafeConnect();
			}
		}

	// GATHER OUTPUT PARAMETERS AND PAIR THEM WITH LOCAL VARIABLES
	TArray<FBaseAsyncTaskHelper::FOutputPinAndLocalVariable> VariableOutputs;
	for (auto CurrentPin : Pins)
	{
		if (FBaseAsyncTaskHelper::ValidDataPin(CurrentPin, EGPD_Output, Schema))
		{
			const FEdGraphPinType& PinType = CurrentPin->PinType;
			UK2Node_TemporaryVariable* TempVarOutput = CompilerContext.SpawnInternalVariable(
				this, PinType.PinCategory, PinType.PinSubCategory, PinType.PinSubCategoryObject.Get(), PinType.bIsArray);
			bIsErrorFree &= TempVarOutput->GetVariablePin() && CompilerContext.MovePinLinksToIntermediate(*CurrentPin, *TempVarOutput->GetVariablePin()).CanSafeConnect();
			VariableOutputs.Add(FBaseAsyncTaskHelper::FOutputPinAndLocalVariable(CurrentPin, TempVarOutput));
		}
	}

	// FOR EACH DELEGATE DEFINE EVENT, CONNECT IT TO DELEGATE AND IMPLEMENT A CHAIN OF ASSIGMENTS
	UEdGraphPin* LastThenPin = CallCreateProxyObjectNode->FindPinChecked(Schema->PN_Then);
	UEdGraphPin* const ProxyObjectPin = CallCreateProxyObjectNode->GetReturnValuePin();
	for (TFieldIterator<UMulticastDelegateProperty> PropertyIt(ProxyClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt && bIsErrorFree; ++PropertyIt)
		{
		bIsErrorFree &= FBaseAsyncTaskHelper::HandleDelegateImplementation(*PropertyIt, VariableOutputs, ProxyObjectPin, LastThenPin, this, SourceGraph, CompilerContext);
	}

	if (CallCreateProxyObjectNode->FindPinChecked(Schema->PN_Then) == LastThenPin)
			{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingDelegateProperties", "BaseAsyncTask: Proxy has no delegates defined. @@").ToString(), this);
		return;
			}

	// Create a call to activate the proxy object if necessary
	if (ProxyActivateFunctionName != NAME_None)
	{
		UK2Node_CallFunction* const CallActivateProxyObjectNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallActivateProxyObjectNode->FunctionReference.SetExternalMember(ProxyActivateFunctionName, ProxyClass);
		CallActivateProxyObjectNode->AllocateDefaultPins();

		// Hook up the self connection
		UEdGraphPin* ActivateCallSelfPin = Schema->FindSelfPin(*CallActivateProxyObjectNode, EGPD_Input);
		check(ActivateCallSelfPin);

		bIsErrorFree &= Schema->TryCreateConnection(ProxyObjectPin, ActivateCallSelfPin);

		// Hook the activate node up in the exec chain
		UEdGraphPin* ActivateExecPin = CallActivateProxyObjectNode->FindPinChecked(Schema->PN_Execute);
		UEdGraphPin* ActivateThenPin = CallActivateProxyObjectNode->FindPinChecked(Schema->PN_Then);

		bIsErrorFree &= Schema->TryCreateConnection(LastThenPin, ActivateExecPin);

		LastThenPin = ActivateThenPin;
	}

	// Move the connections from the original node then pin to the last internal then pin
	bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Schema->PN_Then), *LastThenPin).CanSafeConnect();
	if (!bIsErrorFree)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "BaseAsyncTask: Internal connection error. @@").ToString(), this);
		}

	// Make sure we caught everything
	BreakAllNodeLinks();
	}

bool UK2Node_BaseAsyncTask::HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	const UBlueprint* SourceBlueprint = GetBlueprint();

	const bool bProxyFactoryResult = (ProxyFactoryClass != NULL) && (ProxyFactoryClass->ClassGeneratedBy != NULL) && (ProxyFactoryClass->ClassGeneratedBy != SourceBlueprint);
	if (bProxyFactoryResult && OptionalOutput)
	{
		OptionalOutput->Add(ProxyFactoryClass);
	}

	const bool bProxyResult = (ProxyClass != NULL) && (ProxyClass->ClassGeneratedBy != NULL) && (ProxyClass->ClassGeneratedBy != SourceBlueprint);
	if (bProxyResult && OptionalOutput)
	{
		OptionalOutput->Add(ProxyClass);
	}

	return bProxyFactoryResult || bProxyResult || Super::HasExternalBlueprintDependencies(OptionalOutput);
}

FName UK2Node_BaseAsyncTask::GetCornerIcon() const
	{
	return TEXT("Graph.Latent.LatentIcon");
	}

UFunction* UK2Node_BaseAsyncTask::GetFactoryFunction() const
{
	check(ProxyFactoryClass);
	UFunction* FactoryFunction = ProxyFactoryClass->FindFunctionByName(ProxyFactoryFunctionName);
	check(FactoryFunction);
	return FactoryFunction;
}

#undef LOCTEXT_NAMESPACE
