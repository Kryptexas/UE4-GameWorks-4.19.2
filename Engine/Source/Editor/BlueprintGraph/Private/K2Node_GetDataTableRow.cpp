// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
//#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Kismet/DataTableFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "K2Node_GetDataTableRow"

struct UK2Node_GetDataTableRowHelper
{
	static FString DataTablePinName;
	static FString RowNamePinName;
	static FString RowNotFoundPinName;
};

FString UK2Node_GetDataTableRowHelper::DataTablePinName(LOCTEXT("DataTablePinName","DataTable").ToString());
FString UK2Node_GetDataTableRowHelper::RowNotFoundPinName(LOCTEXT("RowNotFoundPinName","RowNotFound").ToString());
FString UK2Node_GetDataTableRowHelper::RowNamePinName(LOCTEXT("RowNamePinName","RowName").ToString());

UK2Node_GetDataTableRow::UK2Node_GetDataTableRow(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Attempts to retrieve a TableRow from a DataTable via it's RowName").ToString();
}

void UK2Node_GetDataTableRow::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add execution pins
	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	UEdGraphPin* RowFoundPin = CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);
	RowFoundPin->PinFriendlyName = LOCTEXT("GetDataTableRow Row Found Exec pin", "Row Found");
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, UK2Node_GetDataTableRowHelper::RowNotFoundPinName);

	// Add DataTable pin
	UEdGraphPin* DataTablePin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UDataTable::StaticClass(), false, false, UK2Node_GetDataTableRowHelper::DataTablePinName);
	SetPinToolTip(*DataTablePin, LOCTEXT("DataTablePinDescription", "The DataTable you want to retreive a row from"));

	// Row Name pin
	UEdGraphPin* TransformPin = CreatePin(EGPD_Input, K2Schema->PC_Name, TEXT(""), NULL, false, false, UK2Node_GetDataTableRowHelper::RowNamePinName);
	SetPinToolTip(*TransformPin, LOCTEXT("RowNamePinDescription", "The name of the row to retrieve from the DataTable"));

	// Result pin
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Struct, TEXT(""), FTableRowBase::StaticStruct(), false, false, K2Schema->PN_ReturnValue);
	ResultPin->PinFriendlyName = LOCTEXT("GetDataTableRow Output Row", "Out Row");
	SetPinToolTip(*ResultPin, LOCTEXT("ResultPinDescription", "The returned TableRow, if found"));

	Super::AllocateDefaultPins();
}

void UK2Node_GetDataTableRow::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
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


void UK2Node_GetDataTableRow::SetReturnTypeForStruct(UScriptStruct* RowStruct)
{
	check(RowStruct != NULL);

    // Change class of output pin
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = RowStruct;
}

UScriptStruct* UK2Node_GetDataTableRow::GetDataTableRowStructType(const TArray<UEdGraphPin*>* InPinsToSearch /*=NULL*/) const
{
	UScriptStruct* RowStructType = NULL;
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* DataTablePin = GetDataTablePin(PinsToSearch);
	if(DataTablePin && DataTablePin->DefaultObject != NULL && DataTablePin->LinkedTo.Num() == 0)
	{
		if (DataTablePin->DefaultObject->IsA(UDataTable::StaticClass()))
		{
			UDataTable* DataTable = (UDataTable*)DataTablePin->DefaultObject;
			RowStructType = DataTable->RowStruct;
		}
	}

	return RowStructType;
}


void UK2Node_GetDataTableRow::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) 
{
	Super::ReallocatePinsDuringReconstruction(OldPins);
		
	UScriptStruct* RowStruct = GetDataTableRowStructType(&OldPins);
	if( RowStruct != NULL )
	{
		SetReturnTypeForStruct(RowStruct);
	}
}

bool UK2Node_GetDataTableRow::IsDataTablePin(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	return(	Pin->PinName != K2Schema->PN_Execute &&
			Pin->PinName != K2Schema->PN_Then &&
			Pin->PinName != K2Schema->PN_ReturnValue &&
			Pin->PinName != UK2Node_GetDataTableRowHelper::DataTablePinName &&
			Pin->PinName != UK2Node_GetDataTableRowHelper::RowNotFoundPinName &&
			Pin->PinName != UK2Node_GetDataTableRowHelper::RowNamePinName );
}


void UK2Node_GetDataTableRow::PinDefaultValueChanged(UEdGraphPin* ChangedPin) 
{
	if (ChangedPin->PinName == UK2Node_GetDataTableRowHelper::DataTablePinName)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		// Because the archetype has changed, we break the output link as the output pin type will change
		UEdGraphPin* ResultPin = GetResultPin();
		ResultPin->BreakAllPinLinks();

		// Remove all pins related to archetype variables
		TArray<UEdGraphPin*> OldPins = Pins;
		for (int32 i = 0; i < OldPins.Num(); i++)
		{
			UEdGraphPin* OldPin = OldPins[i];
			if (IsDataTablePin(OldPin))
			{
				OldPin->BreakAllPinLinks();
				Pins.Remove(OldPin);
			}
		}

		UScriptStruct* RowStruct = GetDataTableRowStructType();
		if (RowStruct != NULL)
		{
			SetReturnTypeForStruct(RowStruct);
		}

		// Refresh the UI for the graph so the pin changes show up
		UEdGraph* Graph = GetGraph();
		Graph->NotifyGraphChanged();

		// Mark dirty
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
	}
}

FString UK2Node_GetDataTableRow::GetTooltip() const
{
	return NodeTooltip;
}

UEdGraphPin* UK2Node_GetDataTableRow::GetThenPin()const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_GetDataTableRow::GetDataTablePin(const TArray<UEdGraphPin*>* InPinsToSearch /*= NULL*/) const
{
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;
    
	UEdGraphPin* Pin = NULL;
	for( auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt )
	{
		UEdGraphPin* TestPin = *PinIt;
		if( TestPin && TestPin->PinName == UK2Node_GetDataTableRowHelper::DataTablePinName)
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetDataTableRow::GetRowNamePin() const
{
	UEdGraphPin* Pin = FindPinChecked(UK2Node_GetDataTableRowHelper::RowNamePinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetDataTableRow::GetRowNotFoundPin() const
{
	UEdGraphPin* Pin = FindPinChecked(UK2Node_GetDataTableRowHelper::RowNotFoundPinName);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_GetDataTableRow::GetResultPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}


FText UK2Node_GetDataTableRow::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraphPin* DataTablePin = GetDataTablePin();

	FText DataTableString = NSLOCTEXT("K2Node", "None", "NONE");
	if(DataTablePin != NULL)
	{
		if(DataTablePin->LinkedTo.Num() > 0)
		{
			// Blueprint will be determined dynamically, so we don't have the name in this case
			DataTableString = FText::GetEmpty();
		}
		else if(DataTablePin->DefaultObject != NULL)
		{
			DataTableString = FText::FromString(DataTablePin->DefaultObject->GetName());
		}
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("DataTableName"), DataTableString);
	return FText::Format(NSLOCTEXT("K2Node", "DataTable", "Get Data Table Row {DataTableName}"), Args);
}

void UK2Node_GetDataTableRow::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);
    
	if (CompilerContext.bIsFullCompile)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
        
        UEdGraphPin* OriginalDataTableInPin = GetDataTablePin();
        UDataTable* Table = (OriginalDataTableInPin != NULL) ? Cast<UDataTable>(OriginalDataTableInPin->DefaultObject) : NULL;
        if((0 == OriginalDataTableInPin->LinkedTo.Num()) && (NULL == Table))
        {
        	CompilerContext.MessageLog.Error(*LOCTEXT("GetDataTableRowNoDataTable_Error", "GetDataTableRow must have a DataTable specified.").ToString(), this);
        	// we break exec links so this is the only error we get
            BreakAllNodeLinks();
            return;
        }

		// FUNCTION NODE
		const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UDataTableFunctionLibrary, GetDataTableRowFromName);
		UK2Node_CallFunction* GetDataTableRowFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		GetDataTableRowFunction->FunctionReference.SetExternalMember(FunctionName, UDataTableFunctionLibrary::StaticClass());
		GetDataTableRowFunction->AllocateDefaultPins();
        CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *(GetDataTableRowFunction->GetExecPin()));

		// Connect the input of our GetDataTableRow to the Input of our Function pin
        UEdGraphPin* DataTableInPin = GetDataTableRowFunction->FindPinChecked(TEXT("Table"));
		if(OriginalDataTableInPin->LinkedTo.Num() > 0)
		{
			// Copy the connection
			CompilerContext.MovePinLinksToIntermediate(*OriginalDataTableInPin, *DataTableInPin);
		}
		else
		{
			// Copy literal
			DataTableInPin->DefaultObject = OriginalDataTableInPin->DefaultObject;
		}
		UEdGraphPin* RowNameInPin = GetDataTableRowFunction->FindPinChecked(TEXT("RowName"));
		CompilerContext.MovePinLinksToIntermediate(*GetRowNamePin(), *RowNameInPin);

		// Get some pins to work with
		UEdGraphPin* OriginalOutRowPin = FindPinChecked(Schema->PN_ReturnValue);
		UEdGraphPin* FunctionOutRowPin = GetDataTableRowFunction->FindPinChecked(TEXT("OutRow"));
        UEdGraphPin* FunctionReturnPin = GetDataTableRowFunction->FindPinChecked(Schema->PN_ReturnValue);
        UEdGraphPin* FunctionThenPin = GetDataTableRowFunction->GetThenPin();
        
        // Set the type of the OutRow pin on this expanded mode to match original
        FunctionOutRowPin->PinType = OriginalOutRowPin->PinType;
		FunctionOutRowPin->PinType.PinSubCategoryObject = OriginalOutRowPin->PinType.PinSubCategoryObject;
        
        //BRANCH NODE
        UK2Node_IfThenElse* BranchNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
        BranchNode->AllocateDefaultPins();
        // Hook up inputs to branch
        FunctionThenPin->MakeLinkTo(BranchNode->GetExecPin());
        FunctionReturnPin->MakeLinkTo(BranchNode->GetConditionPin());
        
        // Hook up outputs
        CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *(BranchNode->GetThenPin()));
        CompilerContext.MovePinLinksToIntermediate(*GetRowNotFoundPin(), *(BranchNode->GetElsePin()));
        CompilerContext.MovePinLinksToIntermediate(*OriginalOutRowPin, *FunctionOutRowPin);

		BreakAllNodeLinks();
	}
}

#undef LOCTEXT_NAMESPACE