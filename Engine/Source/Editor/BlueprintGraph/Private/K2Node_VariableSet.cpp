// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "VariableSetHandler.h"

UK2Node_VariableSet::UK2Node_VariableSet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_VariableSet::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	if (GetVarName() != NAME_None)
	{
		if(CreatePinForVariable(EGPD_Input))
		{
			CreatePinForSelf();
		}
	}

	Super::AllocateDefaultPins();
}

void UK2Node_VariableSet::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	if (GetVarName() != NAME_None)
	{
		if(!CreatePinForVariable(EGPD_Input))
		{
			if(!RecreatePinForVariable(EGPD_Input, OldPins))
			{
				return;
			}
		}
		CreatePinForSelf();
	}
}

FString UK2Node_VariableSet::GetTooltip() const
{
	FFormatNamedArguments Args;
	Args.Add( TEXT( "VarName" ), FText::FromString( GetVarNameString() ));
	Args.Add( TEXT( "ReplicationCall" ), FText::GetEmpty());
	Args.Add( TEXT( "ReplicationNotifyName" ), FText::GetEmpty());
	Args.Add( TEXT( "TextPartition" ), FText::GetEmpty());
	Args.Add( TEXT( "MetaData" ), FText::GetEmpty());
		
	if(	HasLocalRepNotify() )
	{
		Args.Add( TEXT( "ReplicationCall" ), NSLOCTEXT( "K2Node", "VariableSet_ReplicationCall", " and call " ));
		Args.Add( TEXT( "ReplicationNotifyName" ), FText::FromString( GetRepNotifyName().ToString() ));
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
	return FText::Format( NSLOCTEXT( "K2Node", "SetValueOfVariable", "Set the value of variable {VarName}{ReplicationCall}{ReplicationNotifyName}{TextPartition}{MetaData}"), Args ).ToString();
}

FString UK2Node_VariableSet::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	FString Result = HasLocalRepNotify() ? NSLOCTEXT("K2Node", "SetWithNotify", "Set with Notify").ToString() : NSLOCTEXT("K2Node", "Set", "Set").ToString();

	// If there is only one variable being written (one non-meta input pin), the title can be made the variable name
	FString InputPinName;
	int32 NumInputsFound = 0;

	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		if ((Pin->Direction == EGPD_Input) && (!K2Schema->IsMetaPin(*Pin)))
		{
			++NumInputsFound;
			InputPinName = Pin->PinName;
		}
	}

	if (NumInputsFound == 1)
	{
		Result = Result + TEXT(" ") + InputPinName;
	}

	return Result;
}


/** Returns true if the variable we are setting has a RepNotify AND was defined in a blueprint
 *		The 'defined in a blueprint' is to avoid natively defined RepNotifies being called unintentionally.
 *		Most (all?) native rep notifies are intended to be client only. We are moving away from this paradigm in blueprints
 *		So for now this is somewhat of a hold over to avoid nasty bugs where a K2 set node is calling a native function that the
 *		designer has no idea what it is doing.
 */
bool UK2Node_VariableSet::HasLocalRepNotify() const
{
	UProperty *Property = NULL;
	{
		// Find the property, but only look in blueprint classes for it
		UBlueprintGeneratedClass *SearchClass = Cast<UBlueprintGeneratedClass>(GetVariableSourceClass());
		while(Property == NULL && SearchClass != NULL)
		{
			for (TFieldIterator<UProperty> It(SearchClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
			{
				if (It->GetFName() == GetVarName())
				{
					Property = *It;
					break;
				}
			}
			SearchClass = Cast<UBlueprintGeneratedClass>(SearchClass->GetSuperStruct());
		}
	}

	// If valid property with RepNotify
	if (Property != NULL && Property->RepNotifyFunc != NAME_None)
	{
		// Find function (ok if its defined in native class)
		UFunction* Function = GetVariableSourceClass()->FindFunctionByName(Property->RepNotifyFunc);

		// If valid repnotify func
		if( Function != NULL && Function->NumParms == 0 && Function->GetReturnProperty() == NULL )
		{
			return true;
		}
	}

	return false;
}

bool UK2Node_VariableSet::ShouldFlushDormancyOnSet() const
{
	if (!GetVariableSourceClass()->IsChildOf(AActor::StaticClass()))
	{
		return false;
	}

	// Flush net dormancy before setting a replicated property
	UProperty *Property = FindField<UProperty>(GetVariableSourceClass(), GetVarName());
	return (Property != NULL && (Property->PropertyFlags & CPF_Net));
}

FName UK2Node_VariableSet::GetRepNotifyName() const
{
	UProperty * Property = GetPropertyForVariable();
	if (Property)
	{
		return Property->RepNotifyFunc;
	}
	return NAME_None;
}


FNodeHandlingFunctor* UK2Node_VariableSet::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_VariableSet(CompilerContext);
}
