// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
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

#define GET_STRUCT_NAME_CHECKED(StructName) \
	((void)sizeof(StructName), TEXT(#StructName))

	bool CanUsePropertyType(const UProperty* TestProperty)
	{
		if (TestProperty->IsA(UNumericProperty::StaticClass()) ||
			TestProperty->IsA(UBoolProperty::StaticClass()) ||
			TestProperty->IsA(UNameProperty::StaticClass()))
		{
			return true;
		}

		const UStructProperty* StructProp = Cast<const UStructProperty>(TestProperty);
		if (StructProp)
		{
			FString CPPType = StructProp->GetCPPType(NULL, CPPF_None);
			if (CPPType.Contains(GET_STRUCT_NAME_CHECKED(FVector)) ||
				CPPType.Contains(GET_STRUCT_NAME_CHECKED(FRotator)))
			{
				return true;
			}
		}

		return false;
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
			if (TestProperty->HasAnyPropertyFlags(CPF_Transient) ||
				TestProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) == false)
			{
				continue;
			}

			// serialize only simple types
			if (CanUsePropertyType(TestProperty))
			{
				UE_LOG(LogBehaviorTree, Verbose, TEXT("> name: '%s'"), *GetNameSafe(TestProperty));
				PropertyData.Add(TestProperty);
			}
		}
	}

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
		return FString::Printf(TEXT("%s: %s"), *FName::NameToDisplayString(Prop->GetName(), bIsBool), *ExportedStringValue);
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
				TestProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
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
				PropertyData.Contains(TestProperty))
			{
				continue;
			}

			if (TestProperty->IsA(UClassProperty::StaticClass()) ||
				TestProperty->IsA(UStructProperty::StaticClass()) ||
				CanUsePropertyType(TestProperty))
			{
				if (RetString.Len())
				{
					RetString.AppendChar(TEXT('\n'));
				}

				const uint8* PropData = TestProperty->ContainerPtrToValuePtr<uint8>(Ob);
				RetString += DescribeProperty(TestProperty, PropData);
			}
		}

		return RetString;
	}

	void DescribeRuntimeValues(const UObject* Ob, const TArray<UProperty*>& PropertyData, TArray<FString>& RuntimeValues)
	{
		for (int32 i = 0; i < PropertyData.Num(); i++)
		{
			UProperty* TestProperty = PropertyData[i];
			const uint8* PropAddr = TestProperty->ContainerPtrToValuePtr<uint8>(Ob);

			RuntimeValues.Add(DescribeProperty(TestProperty, PropAddr));
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

	void AbortLatentActions(AActor* OwningActor, const UObject* Ob)
	{
		UWorld* MyWorld = OwningActor->GetWorld();
		MyWorld->GetLatentActionManager().RemoveActionsForObject(Ob);
		MyWorld->GetTimerManager().ClearAllTimersForObject(Ob);
	}
}
