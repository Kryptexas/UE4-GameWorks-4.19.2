// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_VariableGet

class FKCHandler_VariableGet : public FNodeHandlingFunctor
{
public:
	FKCHandler_VariableGet(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) OVERRIDE
	{
		// This net is a variable read
		ResolveAndRegisterScopedTerm(Context, Net, Context.VariableReferences);
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		UK2Node_Variable* VarNode = Cast<UK2Node_Variable>(Node);
		if (VarNode)
		{
			VarNode->CheckForErrors(CompilerContext.GetSchema(), Context.MessageLog);
		}

		FNodeHandlingFunctor::RegisterNets(Context, Node);
	}
};

UK2Node_VariableGet::UK2Node_VariableGet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_VariableGet::AllocateDefaultPins()
{
	if(GetVarName() != NAME_None)
	{
		if(CreatePinForVariable(EGPD_Output))
		{
			CreatePinForSelf();
		}
	}

	Super::AllocateDefaultPins();
}

void UK2Node_VariableGet::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	if(GetVarName() != NAME_None)
	{
		if(!CreatePinForVariable(EGPD_Output))
		{
			if(!RecreatePinForVariable(EGPD_Output, OldPins))
			{
				return;
			}
		}
		CreatePinForSelf();
	}
}

FString UK2Node_VariableGet::GetTooltip() const
{
	FFormatNamedArguments Args;
	Args.Add( TEXT( "VarName" ), FText::FromString( GetVarNameString() ));
	Args.Add( TEXT( "TextPartition" ), FText::GetEmpty());
	Args.Add( TEXT( "MetaData" ), FText::GetEmpty());

	FName VarName = VariableReference.GetMemberName();
	if (VarName != NAME_None)
	{
		FString BPMetaData;
		FBlueprintEditorUtils::GetBlueprintVariableMetaData(GetBlueprint(), VarName, TEXT("tooltip"), BPMetaData);

		if( !BPMetaData.IsEmpty() )
		{
			Args.Add( TEXT( "TextPartition" ), FText::FromString( "\n" ));
			Args.Add( TEXT( "MetaData" ), FText::FromString( BPMetaData ));
		}
	}
	if(  UProperty* Property = GetPropertyForVariable() )
	{
		// discover if the variable property is a non blueprint user variable
		UClass* SourceClass = Property->GetOwnerClass();
		if( SourceClass && SourceClass->ClassGeneratedBy == NULL )
		{
			const FString MetaData = Property->GetToolTipText().ToString();

			if( !MetaData.IsEmpty() )
			{
				// See if the property associated with this editor has a tooltip
				FText PropertyMetaData = FText::FromString( *MetaData );
				FString TooltipName = FString::Printf( TEXT("%s.tooltip"), *(Property->GetName()));
				FText::FindText( *(Property->GetFullGroupName(true)), *TooltipName, PropertyMetaData );
				Args.Add( TEXT( "TextPartition" ), FText::FromString( "\n" ));
				Args.Add( TEXT( "MetaData" ), PropertyMetaData );
			}
		}
	}
	return FText::Format( NSLOCTEXT( "K2Node", "GetVariable_ToolTip", "Read the value of variable {VarName}{TextPartition}{MetaData}"), Args ).ToString();
}

FString UK2Node_VariableGet::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = TEXT("Get");

	// If there is only one variable being read, the title can be made the variable name
	FString OutputPinName;
	int32 NumOutputsFound = 0;

	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		if (Pin->Direction == EGPD_Output)
		{
			++NumOutputsFound;
			OutputPinName = Pin->PinName;
		}
	}

	if (NumOutputsFound == 1)
	{
		Result = Result + TEXT(" ") + OutputPinName;
	}

	return Result;
}

FNodeHandlingFunctor* UK2Node_VariableGet::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_VariableGet(CompilerContext);
}
