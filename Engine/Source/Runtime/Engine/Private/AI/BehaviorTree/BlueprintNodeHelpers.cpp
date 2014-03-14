// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintNodeHelpers.h"

namespace BlueprintNodeHelpers
{
	uint16 GetPropertiesMemorySize(const TArray<UProperty*>& PropertyData)
	{
		int32 TotalMem = 0;
		for (int32 i = 0; i < PropertyData.Num(); i++)
		{
			TotalMem += PropertyData[i]->GetSize();
		}

		if (TotalMem > MAX_uint16)
		{
			TotalMem = 0;
		}

		return TotalMem;
	}

	void CollectPropertyData(const UObject* Ob, const UClass* StopAtClass, TArray<UProperty*>& PropertyData)
	{
		UE_LOG(LogBehaviorTree, Verbose, TEXT("Looking for runtime properties of class: %s"), *GetNameSafe(Ob->GetClass()));

		PropertyData.Reset();
		for (UProperty* TestProperty = Ob->GetClass()->PropertyLink; TestProperty; TestProperty = TestProperty->PropertyLinkNext)
		{
			// stop when reaching base class
			if (TestProperty->GetOuter() == StopAtClass)
			{
				break;
			}

			// skip properties without any setup data
			// object properties are not safe to serialize here (it saves only pointer's address which is not possible to verify before using)
			if (TestProperty->HasAnyPropertyFlags(CPF_Transient) ||
				TestProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) == false ||
				TestProperty->IsA(UFunction::StaticClass()) ||
				TestProperty->IsA(UDelegateProperty::StaticClass()) ||
				TestProperty->IsA(UMulticastDelegateProperty::StaticClass()) ||
				TestProperty->IsA(UObjectPropertyBase::StaticClass()))
			{
				continue;
			}

			UE_LOG(LogBehaviorTree, Verbose, TEXT("> name: '%s'"), *GetNameSafe(TestProperty));
			PropertyData.Add(TestProperty);
		}
	}

#define GET_STRUCT_NAME_CHECKED(StructName) \
	((void)sizeof(StructName), TEXT(#StructName))

	FString DescribeProperty(const UProperty* Prop, const uint8* PropertyAddr)
	{
		FString ExportedStringValue;
		const UStructProperty* StructProp = Cast<const UStructProperty>(Prop);
		const UFloatProperty* FloatProp = Cast<const UFloatProperty>(Prop);

		if (StructProp && StructProp->GetCPPType(NULL, CPPF_None).Contains(GET_STRUCT_NAME_CHECKED(FBlackboardKeySelector)))
		{
			// special case for blackboard key selectors
			const FBlackboardKeySelector* PropertyValue = (const FBlackboardKeySelector*)PropertyAddr;
			ExportedStringValue = PropertyValue->SelectedKeyName.ToString();
		}
		else if (FloatProp)
		{
			// special case for floats to remove unnecessary zeros
			const float FloatValue = FloatProp->GetPropertyValue(PropertyAddr);
			ExportedStringValue = FString::SanitizeFloat(FloatValue);
		}
		else
		{
			Prop->ExportTextItem(ExportedStringValue, PropertyAddr, NULL, NULL, 0, NULL);
		}

		const bool bIsBool = Prop->IsA(UBoolProperty::StaticClass());
		return FString::Printf(TEXT("%s: %s"), *EngineUtils::SanitizeDisplayName(Prop->GetName(), bIsBool), *ExportedStringValue);
	}

	void CollectBlackboardSelectors(const UObject* Ob, const UClass* StopAtClass, TArray<FName>& KeyNames)
	{
		for (UProperty* TestProperty = Ob->GetClass()->PropertyLink; TestProperty; TestProperty = TestProperty->PropertyLinkNext)
		{
			// stop when reaching base class
			if (TestProperty->GetOuter() == StopAtClass)
			{
				break;
			}

			// skip properties without any setup data	
			if (TestProperty->HasAnyPropertyFlags(CPF_Transient) ||
				TestProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ||
				TestProperty->IsA(UFunction::StaticClass()) ||
				TestProperty->IsA(UDelegateProperty::StaticClass()) ||
				TestProperty->IsA(UMulticastDelegateProperty::StaticClass()))
			{
				continue;
			}

			const UStructProperty* StructProp = Cast<const UStructProperty>(TestProperty);
			if (StructProp && StructProp->GetCPPType(NULL, CPPF_None).Contains(GET_STRUCT_NAME_CHECKED(FBlackboardKeySelector)))
			{
				const FBlackboardKeySelector* PropData = TestProperty->ContainerPtrToValuePtr<FBlackboardKeySelector>(Ob);
				KeyNames.AddUnique(PropData->SelectedKeyName);
			}
		}
	}

#undef GET_STRUCT_NAME_CHECKED

	FString CollectPropertyDescription(const UObject* Ob, const UClass* StopAtClass, const TArray<UProperty*>& PropertyData)
	{
		FString RetString;
		for (UProperty* TestProperty = Ob->GetClass()->PropertyLink; TestProperty; TestProperty = TestProperty->PropertyLinkNext)
		{
			// stop when reaching base class
			if (TestProperty->GetOuter() == StopAtClass)
			{
				break;
			}

			// skip properties without any setup data	
			if (TestProperty->HasAnyPropertyFlags(CPF_Transient) ||
				TestProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ||
				TestProperty->IsA(UFunction::StaticClass()) ||
				TestProperty->IsA(UDelegateProperty::StaticClass()) ||
				TestProperty->IsA(UMulticastDelegateProperty::StaticClass()) ||
				PropertyData.Contains(TestProperty))
			{
				continue;
			}

			if (RetString.Len())
			{
				RetString.AppendChar(TEXT('\n'));
			}

			const uint8* PropData = TestProperty->ContainerPtrToValuePtr<uint8>(Ob);
			RetString += DescribeProperty(TestProperty, PropData);
		}

		return RetString;
	}

	void DescribeRuntimeValues(const UObject* Ob, const UClass* StopAtClass,
		const TArray<UProperty*>& PropertyData, uint8* ContextMemory,
		EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& RuntimeValues)
	{
		int32 ContextOffset = 0;
		for (int32 i = 0; i < PropertyData.Num(); i++)
		{
			UProperty* TestProperty = PropertyData[i];

			const uint8* PropAddr = ContextMemory + ContextOffset;
			RuntimeValues.Add(DescribeProperty(TestProperty, PropAddr));

			ContextOffset += TestProperty->GetSize();
		}
	}

	void CopyPropertiesToContext(const TArray<UProperty*>& PropertyData, uint8* ObjectMemory, uint8* ContextMemory)
	{
		if (ContextMemory == NULL)
		{
			// allow null memory pointer on saving back properties
			// (blueprint event finished, but subtree was already removed from context)
			return;
		}

		int32 ContextOffset = 0;
		for (int32 i = 0; i < PropertyData.Num(); i++)
		{
			UProperty* TestProperty = PropertyData[i];

			const uint8* PropAddr = TestProperty->ContainerPtrToValuePtr<uint8>(ObjectMemory);
			uint8* ContextAddr = ContextMemory + ContextOffset;
			TestProperty->CopyCompleteValue(ContextAddr, PropAddr);

			ContextOffset += TestProperty->GetSize();
		}
	}

	void CopyPropertiesFromContext(const TArray<UProperty*>& PropertyData, uint8* ObjectMemory, uint8* ContextMemory)
	{
		int32 ContextOffset = 0;
		for (int32 i = 0; i < PropertyData.Num(); i++)
		{
			UProperty* TestProperty = PropertyData[i];

			uint8* PropAddr = TestProperty->ContainerPtrToValuePtr<uint8>(ObjectMemory);
			const uint8* ContextAddr = ContextMemory + ContextOffset;
			TestProperty->CopyCompleteValue(PropAddr, ContextAddr);

			ContextOffset += TestProperty->GetSize();
		}
	}

	bool FindNodeOwner(AActor* OwningActor, class UBTNode* Node, UBehaviorTreeComponent*& OwningComp, int32& OwningInstanceIdx)
	{
		bool bFound = false;

		APawn* OwningPawn = Cast<APawn>(OwningActor);
		if (OwningPawn && OwningPawn->Controller)
		{
			bFound = FindNodeOwner(OwningPawn->Controller, Node, OwningComp, OwningInstanceIdx);
		}

		if (OwningActor && !bFound)
		{
			TArray<UBehaviorTreeComponent*> Components;
			OwningActor->GetComponents(Components);
			for (int32 i = 0; i < Components.Num(); i++)
			{
				UBehaviorTreeComponent* BTComp = Components[i];
				if (BTComp)
				{
					const int32 InstanceIdx = BTComp->FindInstanceContainingNode(Node);
					if (InstanceIdx != INDEX_NONE)
					{
						OwningComp = BTComp;
						OwningInstanceIdx = InstanceIdx;
						bFound = true;
						break;
					}
				}
			}
		}

		return bFound;
	}
}
