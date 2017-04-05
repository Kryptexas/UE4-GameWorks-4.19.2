// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "K2Node_CallArrayFunction.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_GetArrayItem.h"
#include "BlueprintsObjectVersion.h"
#include "Kismet/KismetArrayLibrary.h" // for Array_Get()
#include "Kismet2/BlueprintEditorUtils.h"

UK2Node_CallArrayFunction::UK2Node_CallArrayFunction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UK2Node_CallArrayFunction::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* TargetArrayPin = GetTargetArrayPin();
	if (ensure(TargetArrayPin))
	{
		TargetArrayPin->PinType.bIsArray = true;
		TargetArrayPin->PinType.bIsReference = true;
		TargetArrayPin->PinType.PinCategory = Schema->PC_Wildcard;
		TargetArrayPin->PinType.PinSubCategory = TEXT("");
		TargetArrayPin->PinType.PinSubCategoryObject = NULL;
	}

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

	PropagateArrayTypeInfo(TargetArrayPin);
}

void UK2Node_CallArrayFunction::PostReconstructNode()
{
	// cannot use a ranged for here, as PinConnectionListChanged() might end up 
	// collapsing split pins (subtracting elements from Pins)
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		if (Pin->LinkedTo.Num() > 0)
		{
			PinConnectionListChanged(Pin);
		}
	}

	Super::PostReconstructNode();
}

void UK2Node_CallArrayFunction::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// Get the set of dynamically-typed pins.
	TArray<UEdGraphPin*> DynamicPins;
	GetDynamicallyTypedPins(DynamicPins);

	// We need to also check subpins here in case one of them was the target of the connection event.
	TArray<UEdGraphPin*> PinsToCheck = DynamicPins;
	for (int32 Index = 0; Index < PinsToCheck.Num(); ++Index)
	{
		UEdGraphPin* PinToCheck = PinsToCheck[Index];
		if (PinToCheck->SubPins.Num() > 0)
		{
			PinsToCheck.Append(PinToCheck->SubPins);
		}
	}

	// If the event target is one of the dynamically-typed pins, we may need to conform the node to match the linked pin's type.
	if (PinsToCheck.Contains(Pin))
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		// Determine whether or not the target pin is connected and/or has a valid pin type.
		const bool bTargetPinHasValidType = Pin->PinType.PinCategory != Schema->PC_Wildcard;
		const bool bTargetPinHasConnection = Pin->LinkedTo.Num() > 0;

		// If the target pin has a connection, but does not yet have a valid pin type, we need to set it based on the connected pin's type.
		if (bTargetPinHasConnection && !bTargetPinHasValidType)
		{
			// Find a linked pin with an authoritative pin type (it might be linked to one of the other pins, depending on which category it is).
			const UEdGraphPin* LinkedPin = FBlueprintEditorUtils::FindLinkedPinWithAuthoritativePinType(Pin, PinsToCheck);

			// Propagate the linked pin's type to the set of dynamically-typed pins (which will also include the target pin).
			check(LinkedPin);
			FBlueprintEditorUtils::PropagatePinTypeInfo(LinkedPin, DynamicPins);
		}
		else if (!bTargetPinHasConnection && bTargetPinHasValidType)
		{
			// If the target pin does not have a connection, but does have a valid type, we may need to reset it back to the wildcard type.
			bool bNeedToResetType = true;

			// Check to see if any of the other dynamically-typed pins still have a connection. If any of them do, then we can't reset the type.
			for (UEdGraphPin* PinToCheck : PinsToCheck)
			{
				if (PinToCheck->LinkedTo.Num() > 0)
				{
					bNeedToResetType = false;
					break;
				}
			}

			if (bNeedToResetType)
			{
				// Reset the pin's type.
				Pin->PinType.PinCategory = Schema->PC_Wildcard;
				Pin->PinType.PinSubCategory = TEXT("");
				Pin->PinType.PinSubCategoryObject = NULL;

				// Reset all other dynamically-typed pins to match.
				FBlueprintEditorUtils::PropagatePinTypeInfo(Pin, DynamicPins);
			}
		}
	}
}

void UK2Node_CallArrayFunction::ConvertDeprecatedNode(UEdGraph* Graph, bool bOnlySafeChanges)
{
	if (GetLinkerCustomVersion(FBlueprintsObjectVersion::GUID) < FBlueprintsObjectVersion::ArrayGetFuncsReplacedByCustomNode)
	{
		if (FunctionReference.GetMemberParentClass() == UKismetArrayLibrary::StaticClass() && 
			FunctionReference.GetMemberName() == GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Get))
		{
			UBlueprintNodeSpawner::FCustomizeNodeDelegate CustomizeToReturnByVal = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
				[](UEdGraphNode* NewNode, bool /*bIsTemplateNode*/)
				{
					UK2Node_GetArrayItem* ArrayGetNode = CastChecked<UK2Node_GetArrayItem>(NewNode);
					ArrayGetNode->SetDesiredReturnType(/*bAsReference =*/false);
				}
			);
			UBlueprintNodeSpawner* GetItemNodeSpawner = UBlueprintNodeSpawner::Create(UK2Node_GetArrayItem::StaticClass(), /*Outer =*/nullptr, CustomizeToReturnByVal);

			FVector2D NodePos(NodePosX, NodePosY);
			IBlueprintNodeBinder::FBindingSet Bindings;
			UK2Node_GetArrayItem* GetItemNode = Cast<UK2Node_GetArrayItem>(GetItemNodeSpawner->Invoke(Graph, Bindings, NodePos));

			const UEdGraphSchema_K2* K2Schema = Cast<UEdGraphSchema_K2>(Graph->GetSchema());
			if (K2Schema && GetItemNode)
			{
				TMap<FString, FString> OldToNewPinMap;
				for (UEdGraphPin* Pin : Pins)
				{
					if (Pin->ParentPin != nullptr)
					{
						// ReplaceOldNodeWithNew() will take care of mapping split pins (as long as the parents are properly mapped)
						continue;
					}
					else if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
					{
						// there's no analogous pin, signal that we're expecting this
						OldToNewPinMap.Add(Pin->PinName, TEXT(""));
					}
					else if (Pin->PinType.bIsArray)
					{
						OldToNewPinMap.Add(Pin->PinName, GetItemNode->GetTargetArrayPin()->PinName);
					}
					else if (Pin->Direction == EGPD_Output)
					{
						OldToNewPinMap.Add(Pin->PinName, GetItemNode->GetResultPin()->PinName);
					}
					else
					{
						OldToNewPinMap.Add(Pin->PinName, GetItemNode->GetIndexPin()->PinName);
					}
				}
				K2Schema->ReplaceOldNodeWithNew(this, GetItemNode, OldToNewPinMap);
			}
		}
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
	if (ensure(TargetFunction))
	{
		FString ArrayPointerMetaData = TargetFunction->GetMetaData(FBlueprintMetadata::MD_ArrayParam);
		TArray<FString> ArrayPinComboNames;
		ArrayPointerMetaData.ParseIntoArray(ArrayPinComboNames, TEXT(","), true);

		for (auto Iter = ArrayPinComboNames.CreateConstIterator(); Iter; ++Iter)
		{
			TArray<FString> ArrayPinNames;
			Iter->ParseIntoArray(ArrayPinNames, TEXT("|"), true);

			FArrayPropertyPinCombo ArrayInfo;
			ArrayInfo.ArrayPin = FindPin(ArrayPinNames[0]);
			if (ArrayPinNames.Num() > 1)
			{
				ArrayInfo.ArrayPropPin = FindPin(ArrayPinNames[1]);
			}

			if (ArrayInfo.ArrayPin)
			{
				OutArrayPinInfo.Add(ArrayInfo);
			}
		}
	}
}

bool UK2Node_CallArrayFunction::IsWildcardProperty(UFunction* InArrayFunction, const UProperty* InProperty)
{
	if(InArrayFunction && InProperty)
	{
		const FString& ArrayPointerMetaData = InArrayFunction->GetMetaData(FBlueprintMetadata::MD_ArrayParam);
		if (!ArrayPointerMetaData.IsEmpty())
		{
			TArray<FString> ArrayPinComboNames;
			ArrayPointerMetaData.ParseIntoArray(ArrayPinComboNames, TEXT(","), true);

			for (auto Iter = ArrayPinComboNames.CreateConstIterator(); Iter; ++Iter)
			{
				TArray<FString> ArrayPinNames;
				Iter->ParseIntoArray(ArrayPinNames, TEXT("|"), true);

				if (ArrayPinNames[0] == InProperty->GetName())
				{
					return true;
				}
			}
		}
	}
	return false;
}

void UK2Node_CallArrayFunction::PropagateArrayTypeInfo(const UEdGraphPin* SourcePin)
{
	TArray<UEdGraphPin*> DynamicPins;
	GetDynamicallyTypedPins(DynamicPins);
	FBlueprintEditorUtils::PropagatePinTypeInfo(SourcePin, DynamicPins);
}

void UK2Node_CallArrayFunction::GetArrayTypeDependentPins(TArray<UEdGraphPin*>& OutPins) const
{
	OutPins.Empty();

	UFunction* TargetFunction = GetTargetFunction();
	if (ensure(TargetFunction))
	{
		const FString DependentPinMetaData = TargetFunction->GetMetaData(FBlueprintMetadata::MD_ArrayDependentParam);
		TArray<FString> TypeDependentPinNames;
		DependentPinMetaData.ParseIntoArray(TypeDependentPinNames, TEXT(","), true);

		for (TArray<UEdGraphPin*>::TConstIterator it(Pins); it; ++it)
		{
			UEdGraphPin* CurrentPin = *it;
			int32 ItemIndex = 0;
			if (CurrentPin && TypeDependentPinNames.Find(CurrentPin->PinName, ItemIndex))
			{
				OutPins.Add(CurrentPin);
			}
		}
	}
}

void UK2Node_CallArrayFunction::GetDynamicallyTypedPins(TArray<UEdGraphPin *>& OutPins) const
{
	// Include all pins that are listed as type-dependent in the UFUNCTION metadata. This must be done first, as it will clear the array.
	GetArrayTypeDependentPins(OutPins);

	// Include the target array pin.
	OutPins.Add(GetTargetArrayPin());
}