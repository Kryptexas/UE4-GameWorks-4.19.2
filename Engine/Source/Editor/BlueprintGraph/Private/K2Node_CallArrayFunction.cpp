// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

UK2Node_CallArrayFunction::UK2Node_CallArrayFunction(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_CallArrayFunction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* TargetArrayPin = GetTargetArrayPin();
	check(TargetArrayPin);
	TargetArrayPin->PinType.bIsArray = true;
	TargetArrayPin->PinType.bIsReference = true;
	TargetArrayPin->PinType.PinCategory = Schema->PC_Wildcard;
	TargetArrayPin->PinType.PinSubCategory = TEXT("");
	TargetArrayPin->PinType.PinSubCategoryObject = NULL;

	TArray< FArrayPropertyPinCombo > ArrayPins;
	GetArrayPins(ArrayPins);
	for(auto Iter = ArrayPins.CreateConstIterator(); Iter; ++Iter)
	{
		if(Iter->ArrayPropPin)
		{
			Iter->ArrayPropPin->bHidden = true;
			Iter->ArrayPropPin->bNotConnectable = true;
			Iter->ArrayPropPin->bDefaultValueIsReadOnly = true;
		}
	}

	PropagateArrayTypeInfo();
}

void UK2Node_CallArrayFunction::PostReconstructNode()
{
	PinConnectionListChanged( GetTargetArrayPin() );
}

void UK2Node_CallArrayFunction::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if( Pin == GetTargetArrayPin() )
	{
		if( Pin->LinkedTo.Num() > 0 )
		{
			UEdGraphPin* LinkedTo = Pin->LinkedTo[0];
			check(LinkedTo);
			check(LinkedTo->PinType.bIsArray);

			Pin->PinType = LinkedTo->PinType;
		}
		else
		{
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
			Pin->PinType.PinCategory = Schema->PC_Wildcard;
			Pin->PinType.PinSubCategory = TEXT("");
			Pin->PinType.PinSubCategoryObject = NULL;
		}

		PropagateArrayTypeInfo();
	}
}

UEdGraphPin* UK2Node_CallArrayFunction::GetTargetArrayPin() const
{
	TArray< FArrayPropertyPinCombo > ArrayParmPins;

	GetArrayPins(ArrayParmPins);

	if(ArrayParmPins.Num())
	{
		return ArrayParmPins[0].ArrayPin;
	}
	return nullptr;
}

void UK2Node_CallArrayFunction::GetArrayPins(TArray< FArrayPropertyPinCombo >& OutArrayPinInfo ) const
{
	OutArrayPinInfo.Empty();

	UFunction* TargetFunction = GetTargetFunction();
	check(TargetFunction);
	FString ArrayPointerMetaData = TargetFunction->GetMetaData(TEXT("ArrayParm"));
	TArray<FString> ArrayPinComboNames;
	ArrayPointerMetaData.ParseIntoArray(&ArrayPinComboNames, TEXT(","), true);

	for(auto Iter = ArrayPinComboNames.CreateConstIterator(); Iter; ++Iter)
	{
		TArray<FString> ArrayPinNames;
		Iter->ParseIntoArray(&ArrayPinNames, TEXT("|"), true);

		FArrayPropertyPinCombo ArrayInfo;
		ArrayInfo.ArrayPin = FindPin(ArrayPinNames[0]);
		if(ArrayPinNames.Num() > 1)
		{
			ArrayInfo.ArrayPropPin = FindPin(ArrayPinNames[1]);
		}

		if(ArrayInfo.ArrayPin)
		{
			OutArrayPinInfo.Add(ArrayInfo);
		}
	}
}

bool UK2Node_CallArrayFunction::IsWildcardProperty(UFunction* InArrayFunction, const UProperty* InProperty)
{
	if(InArrayFunction && InProperty)
	{
		FString ArrayPointerMetaData = InArrayFunction->GetMetaData(TEXT("ArrayParm"));
		TArray<FString> ArrayPinComboNames;
		ArrayPointerMetaData.ParseIntoArray(&ArrayPinComboNames, TEXT(","), true);

		for(auto Iter = ArrayPinComboNames.CreateConstIterator(); Iter; ++Iter)
		{
			TArray<FString> ArrayPinNames;
			Iter->ParseIntoArray(&ArrayPinNames, TEXT("|"), true);

			if(ArrayPinNames[0] == InProperty->GetName())
			{
				return true;
			}
		}
	}
	return false;
}

void UK2Node_CallArrayFunction::GetArrayTypeDependentPins(TArray<UEdGraphPin*>& OutPins) const
{
	OutPins.Empty();

	UFunction* TargetFunction = GetTargetFunction();
	check(TargetFunction);

	const FString DependentPinMetaData = TargetFunction->GetMetaData(TEXT("ArrayTypeDependentParams"));
	TArray<FString> TypeDependentPinNames;
	DependentPinMetaData.ParseIntoArray(&TypeDependentPinNames, TEXT(","), true);

	for(TArray<UEdGraphPin*>::TConstIterator it(Pins); it; ++it)
	{
		UEdGraphPin* CurrentPin = *it;
		int32 ItemIndex = 0;
		if( CurrentPin && TypeDependentPinNames.Find(CurrentPin->PinName, ItemIndex) )
		{
			OutPins.Add(CurrentPin);
		}
	}
}

void UK2Node_CallArrayFunction::PropagateArrayTypeInfo()
{
	const UEdGraphPin* ArrayTargetPin = GetTargetArrayPin();

	if( ArrayTargetPin )
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		TArray<UEdGraphPin*> DependentPins;
		GetArrayTypeDependentPins(DependentPins);
	
		// Propagate pin type info (except for array info!) to pins with dependent types
		for( TArray<UEdGraphPin*>::TIterator it(DependentPins); it; ++it )
		{
			UEdGraphPin* CurrentPin = *it;
			CurrentPin->PinType.PinCategory = ArrayTargetPin->PinType.PinCategory;
			CurrentPin->PinType.PinSubCategory = ArrayTargetPin->PinType.PinSubCategory;
			CurrentPin->PinType.PinSubCategoryObject = ArrayTargetPin->PinType.PinSubCategoryObject;

			// Verify that all previous connections to this pin are still valid with the new type
			for( TArray<UEdGraphPin*>::TIterator ConnectionIt(CurrentPin->LinkedTo); ConnectionIt; ++ConnectionIt )
			{
				UEdGraphPin* ConnectedPin = *ConnectionIt;
				if( !Schema->ArePinsCompatible(CurrentPin, ConnectedPin) )
				{
					CurrentPin->BreakLinkTo(ConnectedPin);
				}
			}
		}
	}
}
