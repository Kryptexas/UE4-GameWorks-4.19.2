// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeDataSetBase.h"
#include "UObject/UnrealType.h"
#include "INiagaraCompiler.h"
#include "NiagaraEvents.h"
#include "EdGraphSchema_Niagara.h"

#define LOCTEXT_NAMESPACE "UNiagaraNodeDataSetBase"

UNiagaraNodeDataSetBase::UNiagaraNodeDataSetBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool UNiagaraNodeDataSetBase::InitializeFromStruct(const UStruct* PayloadStruct)
{
	if (InitializeFromStructInternal(PayloadStruct))
	{
		//ReallocatePins();
		return true;
	}
	return false;
}

bool UNiagaraNodeDataSetBase::InitializeFromStructInternal(const UStruct* PayloadStruct)
{
	if (!PayloadStruct)
	{
		return false;
	}

	Variables.Empty();
	DataSet = FNiagaraDataSetID();

	// TODO: need to add valid as a variable separately for now; compiler support to validate index is missing	
	//Variables.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), "Valid"));

	//UGH! Why do we have our own custom type rep again. Why aren't we just using the UPropertySystem?
	//
	// [OP] not really different from anywhere else, nodes everywhere else hold FNiagaraVariables as their outputs; 
	//  just traversing the ustruct here to build an array of those; this is temporary and should be genericised, of course
	for (TFieldIterator<UProperty> PropertyIt(PayloadStruct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		const UProperty* Property = *PropertyIt;
		const UStructProperty* StructProp = Cast<UStructProperty>(Property);
		if (Property->IsA(UFloatProperty::StaticClass()))
		{
			Variables.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), *Property->GetName()));
		}
		else if (Property->IsA(UBoolProperty::StaticClass()))
		{
			Variables.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), *Property->GetName()));
		}
		else if (Property->IsA(UIntProperty::StaticClass()))
		{
			Variables.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), *Property->GetName()));
		}
		else if (StructProp && StructProp->Struct == FNiagaraTypeDefinition::GetVec2Struct())
		{
			Variables.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), *Property->GetName()));
		}
		else if (StructProp && StructProp->Struct == FNiagaraTypeDefinition::GetVec3Struct())
		{
			Variables.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), *Property->GetName()));
		}
		else if (StructProp && StructProp->Struct == FNiagaraTypeDefinition::GetVec4Struct())
		{
			Variables.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), *Property->GetName()));
		}
		else
		{
			UE_LOG(LogNiagaraEditor, Warning, TEXT("Could not add property %s in struct %s to NiagaraNodeDataSetBase, class %s"), *Property->GetName(), *PayloadStruct->GetName(), PayloadStruct->GetClass() ? *PayloadStruct->GetClass()->GetName() : *FString("nullptr"));
		}
	}

	DataSet.Name = *PayloadStruct->GetName();
	return true;
}


void UNiagaraNodeDataSetBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		ReallocatePins();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}


FLinearColor UNiagaraNodeDataSetBase::GetNodeTitleColor() const
{
	check(DataSet.Type == ENiagaraDataSetType::Event);//Implement other datasets
	return CastChecked<UEdGraphSchema_Niagara>(GetSchema())->NodeTitleColor_Event;
}



#undef LOCTEXT_NAMESPACE



